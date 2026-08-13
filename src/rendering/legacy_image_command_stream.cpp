#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <cstddef>
#include <limits>

namespace openswd3::rendering {
namespace {

using compat::u8;
using compat::u16;

constexpr u16 kCommandTagMask = 0xC000U;
constexpr u16 kCommandCountMask = 0x3FFFU;
constexpr u16 kDecoderDepthMask = 0x3FFFU;
constexpr u16 kConverterDepthMask = 0x7FFFU;
constexpr std::size_t kHeaderBytes = 8U;
constexpr std::size_t kEmbeddedPaletteBytes = 512U;

[[nodiscard]] bool read_u16(
    const std::span<const u8> bytes, std::size_t& offset, u16& value
) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 2U) {
        return false;
    }
    value = static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
    offset += 2U;
    return true;
}

[[nodiscard]] bool read_u16_at(
    const std::span<const u8> bytes, const std::size_t offset, u16& value
) noexcept {
    std::size_t cursor = offset;
    return read_u16(bytes, cursor, value);
}

void write_u16_at(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) noexcept {
    bytes[offset] = static_cast<u8>(value & 0x00FFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void append_u16(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value & 0x00FFU));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

[[nodiscard]] bool checked_multiply(
    const std::size_t left, const std::size_t right, std::size_t& result
) noexcept {
    if (left != 0U && right > std::numeric_limits<std::size_t>::max() / left) {
        return false;
    }
    result = left * right;
    return true;
}

[[nodiscard]] LegacyImageCommandStreamStatus read_header(
    const std::span<const u8> bytes, LegacyImageCommandStreamHeader& header
) noexcept {
    std::size_t offset{};
    if (!read_u16(bytes, offset, header.magic) ||
        !read_u16(bytes, offset, header.width) ||
        !read_u16(bytes, offset, header.height) ||
        !read_u16(bytes, offset, header.format)) {
        return LegacyImageCommandStreamStatus::source_exhausted;
    }
    if (header.magic != kLegacyImageCommandStreamMagic) {
        return LegacyImageCommandStreamStatus::invalid_magic;
    }
    return LegacyImageCommandStreamStatus::completed;
}

void append_header(
    std::vector<u8>& bytes, const LegacyImageCommandStreamHeader& header
) {
    append_u16(bytes, header.magic);
    append_u16(bytes, header.width);
    append_u16(bytes, header.height);
    append_u16(bytes, header.format);
}

[[nodiscard]] u16 convert_pixel_forward(
    const LegacyPixelConversionState& state, u16 pixel
) noexcept {
    legacy_convert_pixels_forward(state, &pixel, 1);
    return pixel;
}

[[nodiscard]] bool
can_append(const std::vector<u8>& bytes, const std::size_t count) noexcept {
    return count <= bytes.max_size() - bytes.size();
}

[[nodiscard]] LegacyImageCommandStreamResult expand_indexed_stream(
    const std::span<const u8> command_stream,
    const std::span<const u16> palette,
    const LegacyPixelConversionState& pixel_conversion
) {
    LegacyImageCommandStreamResult result;
    result.status = read_header(command_stream, result.header);
    if (result.status != LegacyImageCommandStreamStatus::completed) {
        return result;
    }
    if ((result.header.format & kConverterDepthMask) != 8U) {
        result.status = LegacyImageCommandStreamStatus::unsupported_depth;
        return result;
    }
    if (palette.size() < 256U) {
        result.status = LegacyImageCommandStreamStatus::palette_too_small;
        return result;
    }

    result.header.format =
        static_cast<u16>((result.header.format & 0x8010U) | 0x0010U);
    append_header(result.bytes, result.header);

    std::size_t source_offset = kHeaderBytes;
    while (true) {
        u16 source_row_header{};
        if (!read_u16(command_stream, source_offset, source_row_header)) {
            result.status = LegacyImageCommandStreamStatus::source_exhausted;
            return result;
        }
        if (source_row_header == 0U) {
            append_u16(result.bytes, 0U);
            result.status = LegacyImageCommandStreamStatus::completed;
            return result;
        }

        const std::size_t output_row_offset = result.bytes.size();
        append_u16(result.bytes, 0U);
        while (true) {
            u16 command{};
            if (!read_u16(command_stream, source_offset, command)) {
                result.status =
                    LegacyImageCommandStreamStatus::source_exhausted;
                return result;
            }
            append_u16(result.bytes, command);
            if (command == 0U) {
                break;
            }
            if ((command & kCommandTagMask) != 0U) {
                continue;
            }

            const std::size_t count = command & kCommandCountMask;
            if (source_offset > command_stream.size() ||
                count > command_stream.size() - source_offset) {
                result.status =
                    LegacyImageCommandStreamStatus::source_exhausted;
                return result;
            }
            std::size_t output_bytes{};
            if (!checked_multiply(count, 2U, output_bytes) ||
                !can_append(result.bytes, output_bytes)) {
                result.status = LegacyImageCommandStreamStatus::size_overflow;
                return result;
            }
            for (std::size_t index = 0U; index < count; ++index) {
                const u8 palette_index = command_stream[source_offset];
                ++source_offset;
                append_u16(
                    result.bytes,
                    convert_pixel_forward(
                        pixel_conversion, palette[palette_index]
                    )
                );
            }
        }

        const std::size_t row_bytes = result.bytes.size() - output_row_offset;
        write_u16_at(
            result.bytes, output_row_offset, static_cast<u16>(row_bytes)
        );
    }
}

}  // namespace

LegacyImageCommandStreamResult encode_legacy_image_command_stream(
    const std::span<const u8> pixels,
    const u16 width,
    const u16 height,
    const u16 format
) {
    LegacyImageCommandStreamResult result;
    result.header = {
        kLegacyImageCommandStreamMagic,
        width,
        height,
        format == 16U ? static_cast<u16>(16U) : format,
    };

    const std::size_t pixels_per_row = width == 0U ? 1U : width;
    std::size_t pixel_count{};
    if (!checked_multiply(pixels_per_row, height, pixel_count)) {
        result.status = LegacyImageCommandStreamStatus::size_overflow;
        return result;
    }
    const std::size_t bytes_per_pixel = format == 16U ? 2U : 1U;
    std::size_t required_source_bytes{};
    if (!checked_multiply(
            pixel_count, bytes_per_pixel, required_source_bytes
        )) {
        result.status = LegacyImageCommandStreamStatus::size_overflow;
        return result;
    }
    if (pixels.size() < required_source_bytes) {
        result.status = LegacyImageCommandStreamStatus::source_exhausted;
        return result;
    }

    append_header(result.bytes, result.header);
    std::size_t source_offset{};
    for (std::size_t row = 0U; row < height; ++row) {
        const std::size_t row_offset = result.bytes.size();
        append_u16(result.bytes, 0U);

        u16 row_flag{};
        if (format == 16U) {
            u16 first{};
            static_cast<void>(read_u16_at(pixels, source_offset, first));
            if (first != kLegacyImageMarker8000 &&
                first != kLegacyImageMarkerC000) {
                row_flag = 0x8000U;
            }

            std::size_t column{};
            while (column < pixels_per_row) {
                u16 pixel{};
                static_cast<void>(read_u16_at(pixels, source_offset, pixel));
                if (pixel == kLegacyImageMarker8000 ||
                    pixel == kLegacyImageMarkerC000) {
                    const u16 marker = pixel;
                    std::size_t run = 1U;
                    while (column + run < pixels_per_row) {
                        u16 candidate{};
                        static_cast<void>(read_u16_at(
                            pixels, source_offset + run * 2U, candidate
                        ));
                        if (candidate != marker) {
                            break;
                        }
                        ++run;
                    }
                    const u16 tag =
                        marker == kLegacyImageMarker8000 ? 0x8000U : 0xC000U;
                    append_u16(
                        result.bytes,
                        static_cast<u16>(static_cast<u16>(run) | tag)
                    );
                    source_offset += run * 2U;
                    column += run;
                    continue;
                }

                std::size_t literal_count = 1U;
                while (column + literal_count < pixels_per_row) {
                    u16 candidate{};
                    static_cast<void>(read_u16_at(
                        pixels, source_offset + literal_count * 2U, candidate
                    ));
                    if (candidate == kLegacyImageMarker8000 ||
                        candidate == kLegacyImageMarkerC000) {
                        break;
                    }
                    ++literal_count;
                }
                append_u16(result.bytes, static_cast<u16>(literal_count));
                for (std::size_t index = 0U; index < literal_count; ++index) {
                    u16 literal{};
                    static_cast<void>(
                        read_u16_at(pixels, source_offset + index * 2U, literal)
                    );
                    append_u16(result.bytes, literal);
                }
                source_offset += literal_count * 2U;
                column += literal_count;
            }
        } else {
            if (pixels[source_offset] != kLegacyIndexedMarkerC000) {
                row_flag = 0x8000U;
            }

            std::size_t column{};
            while (column < pixels_per_row) {
                const u8 pixel = pixels[source_offset];
                if (pixel == kLegacyIndexedMarker8000 ||
                    pixel == kLegacyIndexedMarkerC000) {
                    std::size_t run = 1U;
                    while (column + run < pixels_per_row &&
                           pixels[source_offset + run] == pixel) {
                        ++run;
                    }
                    const u16 tag =
                        pixel == kLegacyIndexedMarker8000 ? 0x8000U : 0xC000U;
                    append_u16(
                        result.bytes,
                        static_cast<u16>(static_cast<u16>(run) | tag)
                    );
                    source_offset += run;
                    column += run;
                    continue;
                }

                std::size_t literal_count = 1U;
                while (column + literal_count < pixels_per_row) {
                    const u8 candidate = pixels[source_offset + literal_count];
                    if (candidate == kLegacyIndexedMarker8000 ||
                        candidate == kLegacyIndexedMarkerC000) {
                        break;
                    }
                    ++literal_count;
                }
                append_u16(result.bytes, static_cast<u16>(literal_count));
                for (std::size_t index = 0U; index < literal_count; ++index) {
                    result.bytes.push_back(pixels[source_offset + index]);
                }
                source_offset += literal_count;
                column += literal_count;
            }
        }

        append_u16(result.bytes, 0U);
        const std::size_t row_bytes = result.bytes.size() - row_offset;
        write_u16_at(
            result.bytes,
            row_offset,
            static_cast<u16>(static_cast<u16>(row_bytes) | row_flag)
        );
    }
    append_u16(result.bytes, 0U);
    result.status = LegacyImageCommandStreamStatus::completed;
    return result;
}

LegacyImageCommandStreamResult
decode_legacy_image_command_stream(const std::span<const u8> command_stream) {
    LegacyImageCommandStreamResult result;
    result.status = read_header(command_stream, result.header);
    if (result.status != LegacyImageCommandStreamStatus::completed) {
        return result;
    }

    const u16 depth = result.header.format & kDecoderDepthMask;
    result.header.format = depth;
    if (depth != 8U && depth != 16U) {
        result.status = LegacyImageCommandStreamStatus::unsupported_depth;
        return result;
    }

    std::size_t expected_bytes{};
    if (!checked_multiply(
            result.header.width, result.header.height, expected_bytes
        ) ||
        (depth == 16U &&
         !checked_multiply(expected_bytes, 2U, expected_bytes))) {
        result.status = LegacyImageCommandStreamStatus::size_overflow;
        return result;
    }

    std::size_t source_offset = kHeaderBytes;
    while (true) {
        u16 row_header{};
        if (!read_u16(command_stream, source_offset, row_header)) {
            result.status = LegacyImageCommandStreamStatus::source_exhausted;
            return result;
        }
        if (row_header == 0U) {
            break;
        }

        while (true) {
            u16 command{};
            if (!read_u16(command_stream, source_offset, command)) {
                result.status =
                    LegacyImageCommandStreamStatus::source_exhausted;
                return result;
            }
            if (command == 0U) {
                break;
            }

            const u16 tag = command & kCommandTagMask;
            const std::size_t count = command & kCommandCountMask;
            if (tag == 0U) {
                std::size_t literal_bytes = count;
                if (depth == 16U &&
                    !checked_multiply(count, 2U, literal_bytes)) {
                    result.status =
                        LegacyImageCommandStreamStatus::size_overflow;
                    return result;
                }
                if (source_offset > command_stream.size() ||
                    literal_bytes > command_stream.size() - source_offset) {
                    result.status =
                        LegacyImageCommandStreamStatus::source_exhausted;
                    return result;
                }
                if (literal_bytes > expected_bytes - result.bytes.size()) {
                    result.status =
                        LegacyImageCommandStreamStatus::pixel_count_mismatch;
                    return result;
                }
                for (std::size_t index = 0U; index < literal_bytes; ++index) {
                    result.bytes.push_back(
                        command_stream[source_offset + index]
                    );
                }
                source_offset += literal_bytes;
                continue;
            }
            if (tag == 0x4000U) {
                continue;
            }

            const std::size_t logical_count = count == 0U ? 65536U : count;
            std::size_t repeated_bytes = logical_count;
            if (depth == 16U &&
                !checked_multiply(logical_count, 2U, repeated_bytes)) {
                result.status = LegacyImageCommandStreamStatus::size_overflow;
                return result;
            }
            if (repeated_bytes > expected_bytes - result.bytes.size()) {
                result.status =
                    LegacyImageCommandStreamStatus::pixel_count_mismatch;
                return result;
            }

            if (depth == 8U) {
                const u8 marker = tag == 0x8000U ? kLegacyIndexedMarker8000
                                                 : kLegacyIndexedMarkerC000;
                for (std::size_t index = 0U; index < logical_count; ++index) {
                    result.bytes.push_back(marker);
                }
            } else {
                const u16 marker = tag == 0x8000U ? kLegacyImageMarker8000
                                                  : kLegacyImageMarkerC000;
                for (std::size_t index = 0U; index < logical_count; ++index) {
                    append_u16(result.bytes, marker);
                }
            }
        }
    }

    result.status = result.bytes.size() == expected_bytes
        ? LegacyImageCommandStreamStatus::completed
        : LegacyImageCommandStreamStatus::pixel_count_mismatch;
    return result;
}

LegacyImageCommandStreamStatus
convert_legacy_image_command_stream_literals_in_place(
    const std::span<u8> command_stream,
    const LegacyPixelConversionState& pixel_conversion,
    LegacyImageCommandStreamHeader* const decoded_header
) {
    LegacyImageCommandStreamHeader header;
    const std::span<const u8> read_only{command_stream};
    LegacyImageCommandStreamStatus status = read_header(read_only, header);
    if (status != LegacyImageCommandStreamStatus::completed) {
        return status;
    }
    if (decoded_header != nullptr) {
        *decoded_header = header;
        decoded_header->format =
            static_cast<u16>(decoded_header->format & kConverterDepthMask);
    }
    if ((header.format & kConverterDepthMask) != 16U) {
        return LegacyImageCommandStreamStatus::unsupported_depth;
    }

    std::size_t source_offset = kHeaderBytes;
    while (true) {
        u16 row_header{};
        if (!read_u16(read_only, source_offset, row_header)) {
            return LegacyImageCommandStreamStatus::source_exhausted;
        }
        if (row_header == 0U) {
            return LegacyImageCommandStreamStatus::completed;
        }

        while (true) {
            u16 command{};
            if (!read_u16(read_only, source_offset, command)) {
                return LegacyImageCommandStreamStatus::source_exhausted;
            }
            if (command == 0U) {
                break;
            }
            if ((command & kCommandTagMask) != 0U) {
                continue;
            }

            const std::size_t count = command & kCommandCountMask;
            std::size_t literal_bytes{};
            if (!checked_multiply(count, 2U, literal_bytes)) {
                return LegacyImageCommandStreamStatus::size_overflow;
            }
            if (source_offset > command_stream.size() ||
                literal_bytes > command_stream.size() - source_offset) {
                return LegacyImageCommandStreamStatus::source_exhausted;
            }
            for (std::size_t index = 0U; index < count; ++index) {
                const std::size_t pixel_offset = source_offset + index * 2U;
                u16 pixel{};
                static_cast<void>(read_u16_at(read_only, pixel_offset, pixel));
                write_u16_at(
                    command_stream,
                    pixel_offset,
                    convert_pixel_forward(pixel_conversion, pixel)
                );
            }
            source_offset += literal_bytes;
        }
    }
}

LegacyImageCommandStreamResult convert_legacy_image_command_stream(
    const std::span<const u8> command_stream,
    const std::span<const u16> palette,
    const LegacyPixelConversionState& pixel_conversion
) {
    LegacyImageCommandStreamHeader header;
    const LegacyImageCommandStreamStatus status =
        read_header(command_stream, header);
    if (status != LegacyImageCommandStreamStatus::completed) {
        LegacyImageCommandStreamResult result;
        result.status = status;
        result.header = header;
        return result;
    }

    if ((header.format & kConverterDepthMask) == 8U) {
        return expand_indexed_stream(command_stream, palette, pixel_conversion);
    }

    LegacyImageCommandStreamResult result;
    result.header = header;
    result.bytes.assign(command_stream.begin(), command_stream.end());
    result.status = convert_legacy_image_command_stream_literals_in_place(
        result.bytes, pixel_conversion
    );
    return result;
}

LegacyImageCommandStreamResult
convert_legacy_embedded_palette_image_command_stream(
    const std::span<const u8> palette_and_command_stream,
    const LegacyPixelConversionState& pixel_conversion
) {
    LegacyImageCommandStreamResult result;
    if (palette_and_command_stream.size() < kEmbeddedPaletteBytes) {
        result.status = LegacyImageCommandStreamStatus::source_exhausted;
        return result;
    }

    const std::span<const u8> command_stream =
        palette_and_command_stream.subspan(kEmbeddedPaletteBytes);
    result.status = read_header(command_stream, result.header);
    if (result.status != LegacyImageCommandStreamStatus::completed) {
        return result;
    }
    if ((result.header.format & kConverterDepthMask) != 8U) {
        result.status = LegacyImageCommandStreamStatus::unsupported_depth;
        return result;
    }

    std::array<u16, 256> palette{};
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        std::size_t offset = index * 2U;
        static_cast<void>(
            read_u16(palette_and_command_stream, offset, palette[index])
        );
    }

    result = expand_indexed_stream(command_stream, palette, pixel_conversion);
    if (result.status == LegacyImageCommandStreamStatus::completed) {
        result.bytes.resize(static_cast<u16>(result.bytes.size()));
    }
    return result;
}

}  // namespace openswd3::rendering
