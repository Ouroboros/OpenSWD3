#ifdef _WIN32

#include "openswd3/rendering/legacy_glyph_atlas.hpp"

#include <windows.h>

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int kSurfaceSize = 64;

struct GlyphSample {
    std::array<char, 3> bytes{};
    int consumed_bytes{};
    int width{};
    int height{};
    int row_bytes{};
    std::filesystem::path mask_file;
};

struct DibFormat {
    const char* name;
    std::uint16_t bits_per_pixel;
    DWORD compression;
    std::array<DWORD, 3> masks;
};

struct BitmapInfoWithMasks {
    BITMAPINFOHEADER header{};
    std::array<DWORD, 3> masks{};
};

struct FontSelection {
    std::string face_name;
    int requested_size{};
    LONG height{};
    LONG ascent{};
    LONG descent{};
    LONG internal_leading{};
    LONG average_char_width{};
    LONG maximum_char_width{};
    BYTE pitch_and_family{};
    BYTE charset{};
};

struct ProbeResult {
    int exact{};
    int different_bits{};
};

[[nodiscard]] std::string utf8_from_wide(const std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        result.data(),
        size,
        nullptr,
        nullptr
    );
    return result;
}

[[nodiscard]] std::vector<std::string_view> split_tabs(
    const std::string_view line
) {
    std::vector<std::string_view> fields;
    std::size_t begin = 0;
    while (begin <= line.size()) {
        const std::size_t end = line.find('\t', begin);
        if (end == std::string_view::npos) {
            fields.push_back(line.substr(begin));
            break;
        }
        fields.push_back(line.substr(begin, end - begin));
        begin = end + 1;
    }
    return fields;
}

template <typename Integer>
[[nodiscard]] bool parse_integer(
    const std::string_view text,
    Integer& value,
    const int base = 10
) {
    const auto result = std::from_chars(
        text.data(),
        text.data() + text.size(),
        value,
        base
    );
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

[[nodiscard]] int hex_nibble(const char value) {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }
    return -1;
}

[[nodiscard]] bool parse_character_bytes(
    const std::string_view text,
    std::array<char, 3>& bytes
) {
    if (text.size() != 6U) {
        return false;
    }
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        const int high = hex_nibble(text[index * 2U]);
        const int low = hex_nibble(text[index * 2U + 1U]);
        if (high < 0 || low < 0) {
            return false;
        }
        bytes[index] = static_cast<char>((high << 4) | low);
    }
    return true;
}

[[nodiscard]] std::vector<std::byte> read_binary(
    const std::filesystem::path& path
) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        return {};
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
    if (!stream && !bytes.empty()) {
        return {};
    }
    return bytes;
}

[[nodiscard]] std::vector<GlyphSample> load_samples(
    const std::filesystem::path& run_directory
) {
    std::ifstream stream(run_directory / "glyph-masks.tsv");
    if (!stream) {
        throw std::runtime_error("cannot open glyph-masks.tsv");
    }

    std::vector<GlyphSample> samples;
    std::string line;
    std::getline(stream, line);
    while (std::getline(stream, line)) {
        const auto fields = split_tabs(line);
        if (fields.size() != 11U) {
            throw std::runtime_error("invalid glyph-masks.tsv row");
        }

        GlyphSample sample;
        if (!parse_character_bytes(fields[3], sample.bytes)
            || !parse_integer(fields[4], sample.consumed_bytes)
            || !parse_integer(fields[5], sample.width)
            || !parse_integer(fields[6], sample.height)
            || !parse_integer(fields[7], sample.row_bytes)) {
            throw std::runtime_error("invalid glyph sample field");
        }
        sample.mask_file = run_directory / std::filesystem::path(fields[10]);
        samples.push_back(sample);
    }
    return samples;
}

class GdiGlyphSurface {
public:
    GdiGlyphSurface(const int size, const DibFormat& format)
        : size_(size), format_(format) {
        dc_ = CreateCompatibleDC(nullptr);
        if (dc_ == nullptr) {
            throw std::runtime_error("CreateCompatibleDC failed");
        }

        LOGFONTA font_description{};
        font_description.lfHeight = -size;
        font_description.lfWeight = FW_NORMAL;
        font_description.lfCharSet = CHINESEBIG5_CHARSET;
        constexpr std::array<unsigned char, 7> kFaceName{
            0xB2U, 0xD3U, 0xA9U, 0xFAU, 0xC5U, 0xE9U, 0x00U,
        };
        for (std::size_t index = 0; index < kFaceName.size(); ++index) {
            font_description.lfFaceName[index] =
                static_cast<char>(kFaceName[index]);
        }
        font_ = CreateFontIndirectA(&font_description);
        if (font_ == nullptr) {
            throw std::runtime_error("CreateFontIndirectA failed");
        }

        BitmapInfoWithMasks info{};
        info.header.biSize = sizeof(BITMAPINFOHEADER);
        info.header.biWidth = kSurfaceSize;
        info.header.biHeight = -kSurfaceSize;
        info.header.biPlanes = 1;
        info.header.biBitCount = format.bits_per_pixel;
        info.header.biCompression = format.compression;
        info.masks = format.masks;
        bitmap_ = CreateDIBSection(
            dc_,
            reinterpret_cast<const BITMAPINFO*>(&info),
            DIB_RGB_COLORS,
            &pixels_,
            nullptr,
            0
        );
        if (bitmap_ == nullptr || pixels_ == nullptr) {
            throw std::runtime_error("CreateDIBSection failed");
        }

        old_bitmap_ = SelectObject(dc_, bitmap_);
        old_font_ = SelectObject(dc_, font_);
        SetBkColor(dc_, RGB(0, 0, 0));
        SetBkMode(dc_, OPAQUE);
        SetTextColor(dc_, RGB(255, 255, 255));

        std::array<wchar_t, LF_FACESIZE> face_name{};
        TEXTMETRICW metrics{};
        if (GetTextFaceW(
                dc_,
                static_cast<int>(face_name.size()),
                face_name.data()
            ) <= 0
            || GetTextMetricsW(dc_, &metrics) == FALSE) {
            throw std::runtime_error("cannot query selected font");
        }
        selection_ = FontSelection{
            .face_name = utf8_from_wide(face_name.data()),
            .requested_size = size,
            .height = metrics.tmHeight,
            .ascent = metrics.tmAscent,
            .descent = metrics.tmDescent,
            .internal_leading = metrics.tmInternalLeading,
            .average_char_width = metrics.tmAveCharWidth,
            .maximum_char_width = metrics.tmMaxCharWidth,
            .pitch_and_family = metrics.tmPitchAndFamily,
            .charset = metrics.tmCharSet,
        };
    }

    GdiGlyphSurface(const GdiGlyphSurface&) = delete;
    GdiGlyphSurface& operator=(const GdiGlyphSurface&) = delete;

    ~GdiGlyphSurface() {
        if (dc_ != nullptr) {
            if (old_font_ != nullptr) {
                SelectObject(dc_, old_font_);
            }
            if (old_bitmap_ != nullptr) {
                SelectObject(dc_, old_bitmap_);
            }
        }
        if (font_ != nullptr) {
            DeleteObject(font_);
        }
        if (bitmap_ != nullptr) {
            DeleteObject(bitmap_);
        }
        if (dc_ != nullptr) {
            DeleteDC(dc_);
        }
    }

    [[nodiscard]] std::vector<std::byte> render(
        const std::array<char, 3>& character,
        const int character_length
    ) {
        const int bytes_per_pixel = format_.bits_per_pixel / 8;
        const int pitch = ((kSurfaceSize * bytes_per_pixel) + 3) & ~3;
        std::memset(pixels_, 0, static_cast<std::size_t>(pitch * kSurfaceSize));
        constexpr char kSpaces[] = "  ";
        TextOutA(dc_, 0, 0, kSpaces, 2);
        TextOutA(dc_, 0, 0, character.data(), character_length);
        GdiFlush();

        const int row_bytes = (size_ + 7) / 8;
        std::vector<std::byte> mask(
            static_cast<std::size_t>(row_bytes * size_),
            std::byte{0}
        );
        const auto* source = static_cast<const std::byte*>(pixels_);
        for (int y = 0; y < size_; ++y) {
            for (int x = 0; x < size_; ++x) {
                bool nonzero = false;
                for (int component = 0; component < bytes_per_pixel; ++component) {
                    nonzero = nonzero
                        || source[static_cast<std::size_t>(
                            y * pitch + x * bytes_per_pixel + component
                        )] != std::byte{0};
                }
                if (nonzero) {
                    mask[static_cast<std::size_t>(y * row_bytes + x / 8)] |=
                        static_cast<std::byte>(0x80U >> (x % 8));
                }
            }
        }
        return mask;
    }

    [[nodiscard]] const FontSelection& selection() const noexcept {
        return selection_;
    }

private:
    int size_{};
    DibFormat format_{};
    HDC dc_{};
    HFONT font_{};
    HBITMAP bitmap_{};
    HGDIOBJ old_font_{};
    HGDIOBJ old_bitmap_{};
    void* pixels_{};
    FontSelection selection_{};
};

[[nodiscard]] int count_different_bits(
    const std::span<const std::byte> left,
    const std::span<const std::byte> right
) {
    if (left.size() != right.size()) {
        return 1'000'000;
    }
    int count = 0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        unsigned int value = std::to_integer<unsigned int>(left[index] ^ right[index]);
        while (value != 0U) {
            count += static_cast<int>(value & 1U);
            value >>= 1U;
        }
    }
    return count;
}

[[nodiscard]] ProbeResult probe_format(
    const std::vector<GlyphSample>& samples,
    const DibFormat& format
) {
    GdiGlyphSurface glyph12(12, format);
    GdiGlyphSurface glyph16(16, format);
    GdiGlyphSurface glyph20(20, format);
    for (const GdiGlyphSurface* surface : {&glyph12, &glyph16, &glyph20}) {
        const FontSelection& selection = surface->selection();
        std::cout << format.name << ": face=" << selection.face_name
                  << "; requested_size=" << selection.requested_size
                  << "; tm_height=" << selection.height
                  << "; ascent=" << selection.ascent
                  << "; descent=" << selection.descent
                  << "; internal_leading=" << selection.internal_leading
                  << "; average_width=" << selection.average_char_width
                  << "; maximum_width=" << selection.maximum_char_width
                  << "; pitch_and_family="
                  << static_cast<unsigned int>(selection.pitch_and_family)
                  << "; charset=" << static_cast<unsigned int>(selection.charset)
                  << '\n';
    }
    ProbeResult result{};
    for (const GlyphSample& sample : samples) {
        GdiGlyphSurface* surface = nullptr;
        if (sample.width == 12 && sample.height == 12) {
            surface = &glyph12;
        } else if (sample.width == 16 && sample.height == 16) {
            surface = &glyph16;
        } else if (sample.width == 20 && sample.height == 20) {
            surface = &glyph20;
        } else {
            throw std::runtime_error("unexpected glyph geometry");
        }

        const std::vector<std::byte> expected = read_binary(sample.mask_file);
        const std::vector<std::byte> actual = surface->render(
            sample.bytes,
            sample.consumed_bytes
        );
        const int difference = count_different_bits(expected, actual);
        result.different_bits += difference;
        result.exact += difference == 0 ? 1 : 0;
    }
    std::cout << format.name << ": exact=" << result.exact << '/'
              << samples.size() << "; different_bits="
              << result.different_bits << '\n';
    return result;
}

void write_u16_le(
    const std::span<std::uint8_t> bytes,
    const std::size_t offset,
    const std::uint16_t value
) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value >> 8U);
}

void write_u32_le(
    const std::span<std::uint8_t> bytes,
    const std::size_t offset,
    const std::uint32_t value
) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<std::uint8_t>(value >> 24U);
}

[[nodiscard]] std::array<char, 3> character_for_atlas_index(
    const std::uint32_t index,
    int& character_length
) {
    std::array<char, 3> character{};
    if (index < 128U) {
        character[0] = static_cast<char>(index);
        character_length = index == 0U ? 0 : 1;
        return character;
    }

    const std::uint32_t dbcs_index = index - 128U;
    character[0] = static_cast<char>(0x80U + dbcs_index / 256U);
    character[1] = static_cast<char>(dbcs_index % 256U);
    character_length = character[1] == '\0' ? 1 : 2;
    return character;
}

void build_atlas(
    const std::filesystem::path& output_file,
    const DibFormat& format
) {
    using openswd3::rendering::kLegacyGlyphAtlasGeometries;
    using openswd3::rendering::kLegacyGlyphAtlasHeaderSize;
    using openswd3::rendering::kLegacyGlyphAtlasKeyCount;
    using openswd3::rendering::kLegacyGlyphAtlasMagic;
    using openswd3::rendering::kLegacyGlyphAtlasSectionCount;
    using openswd3::rendering::kLegacyGlyphAtlasVersion;

    if (std::filesystem::exists(output_file)) {
        throw std::runtime_error("atlas output already exists");
    }

    std::size_t total_size = kLegacyGlyphAtlasHeaderSize;
    for (const auto geometry : kLegacyGlyphAtlasGeometries) {
        const std::size_t row_bytes = (geometry[0] + 7U) / 8U;
        total_size += static_cast<std::size_t>(kLegacyGlyphAtlasKeyCount)
            * row_bytes * geometry[1];
    }

    std::vector<std::uint8_t> atlas(total_size, 0U);
    std::ranges::copy(kLegacyGlyphAtlasMagic, atlas.begin());
    write_u32_le(atlas, 8U, kLegacyGlyphAtlasVersion);
    write_u32_le(atlas, 12U, kLegacyGlyphAtlasHeaderSize);
    write_u32_le(atlas, 16U, kLegacyGlyphAtlasKeyCount);
    write_u32_le(atlas, 20U, kLegacyGlyphAtlasSectionCount);

    std::size_t data_offset = kLegacyGlyphAtlasHeaderSize;
    for (std::size_t section_index = 0;
         section_index < kLegacyGlyphAtlasGeometries.size();
         ++section_index) {
        const auto geometry = kLegacyGlyphAtlasGeometries[section_index];
        const std::uint16_t row_bytes = static_cast<std::uint16_t>(
            (geometry[0] + 7U) / 8U
        );
        const std::uint16_t mask_bytes = static_cast<std::uint16_t>(
            row_bytes * geometry[1]
        );
        const std::uint32_t data_size = kLegacyGlyphAtlasKeyCount * mask_bytes;
        const std::size_t descriptor_offset = 32U + section_index * 16U;
        write_u16_le(atlas, descriptor_offset, geometry[0]);
        write_u16_le(atlas, descriptor_offset + 2U, geometry[1]);
        write_u16_le(atlas, descriptor_offset + 4U, row_bytes);
        write_u16_le(atlas, descriptor_offset + 6U, mask_bytes);
        write_u32_le(
            atlas,
            descriptor_offset + 8U,
            static_cast<std::uint32_t>(data_offset)
        );
        write_u32_le(atlas, descriptor_offset + 12U, data_size);

        GdiGlyphSurface surface(geometry[0], format);
        for (std::uint32_t key_index = 0;
             key_index < kLegacyGlyphAtlasKeyCount;
             ++key_index) {
            int character_length = 0;
            const auto character = character_for_atlas_index(
                key_index, character_length
            );
            const std::vector<std::byte> mask = surface.render(
                character, character_length
            );
            if (mask.size() != mask_bytes) {
                throw std::runtime_error("generated mask size mismatch");
            }
            std::memcpy(
                atlas.data() + data_offset
                    + static_cast<std::size_t>(key_index) * mask_bytes,
                mask.data(),
                mask.size()
            );
        }
        data_offset += data_size;
        std::cout << "atlas: generated " << geometry[0] << 'x'
                  << geometry[1] << '\n';
    }

    std::ofstream stream(output_file, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("cannot create atlas output");
    }
    stream.write(
        reinterpret_cast<const char*>(atlas.data()),
        static_cast<std::streamsize>(atlas.size())
    );
    if (!stream) {
        throw std::runtime_error("cannot write atlas output");
    }
    std::cout << "atlas: wrote " << output_file.string() << "; bytes="
              << atlas.size() << '\n';
}

}  // namespace

int main(const int argument_count, const char* const* arguments) {
    const bool build_requested = argument_count == 4
        && std::string_view(arguments[1]) == "--build-atlas";
    if (argument_count != 2 && !build_requested) {
        std::cerr
            << "usage: openswd3_probe_gdi_glyph_oracle <run-directory>\n"
            << "       openswd3_probe_gdi_glyph_oracle --build-atlas "
               "<run-directory> <atlas-file>\n";
        return 2;
    }
    try {
        const std::filesystem::path run_directory(
            arguments[build_requested ? 2 : 1]
        );
        const auto samples = load_samples(run_directory);
        constexpr std::array<DibFormat, 3> kFormats{{
            {"rgb555", 16, BI_BITFIELDS, {0x7C00U, 0x03E0U, 0x001FU}},
            {"rgb565", 16, BI_BITFIELDS, {0xF800U, 0x07E0U, 0x001FU}},
            {"bgra32", 32, BI_RGB, {0U, 0U, 0U}},
        }};
        bool all_exact = true;
        for (const DibFormat& format : kFormats) {
            const ProbeResult result = probe_format(samples, format);
            all_exact = all_exact
                && result.exact == static_cast<int>(samples.size());
        }
        if (!all_exact) {
            std::cerr << "GDI output does not exactly match the oracle\n";
            return 1;
        }
        if (build_requested) {
            build_atlas(std::filesystem::path(arguments[3]), kFormats[1]);
        }
    } catch (const std::exception& error) {
        std::cerr << "GDI glyph probe failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

#endif
