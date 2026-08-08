#include "test.hpp"

#include "openswd3/rendering/legacy_glyph_atlas.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyGlyphAtlasProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::kLegacyGlyphAtlasGeometries;
using openswd3::rendering::kLegacyGlyphAtlasHeaderSize;
using openswd3::rendering::kLegacyGlyphAtlasKeyCount;
using openswd3::rendering::kLegacyGlyphAtlasMagic;
using openswd3::rendering::kLegacyGlyphAtlasSectionCount;
using openswd3::rendering::kLegacyGlyphAtlasVersion;
using openswd3::rendering::legacy_glyph_atlas_key_index;

void write_u16_le(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32_le(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> make_atlas() {
    std::size_t total_size = kLegacyGlyphAtlasHeaderSize;
    for (const auto geometry : kLegacyGlyphAtlasGeometries) {
        const std::size_t row_bytes = (geometry[0] + 7U) / 8U;
        total_size += static_cast<std::size_t>(kLegacyGlyphAtlasKeyCount)
            * row_bytes * geometry[1];
    }

    std::vector<u8> atlas(total_size, 0U);
    std::ranges::copy(kLegacyGlyphAtlasMagic, atlas.begin());
    write_u32_le(atlas, 8U, kLegacyGlyphAtlasVersion);
    write_u32_le(atlas, 12U, kLegacyGlyphAtlasHeaderSize);
    write_u32_le(atlas, 16U, kLegacyGlyphAtlasKeyCount);
    write_u32_le(atlas, 20U, kLegacyGlyphAtlasSectionCount);

    std::size_t data_offset = kLegacyGlyphAtlasHeaderSize;
    for (std::size_t index = 0; index < kLegacyGlyphAtlasGeometries.size(); ++index) {
        const auto geometry = kLegacyGlyphAtlasGeometries[index];
        const u16 row_bytes = static_cast<u16>((geometry[0] + 7U) / 8U);
        const u16 mask_bytes = static_cast<u16>(row_bytes * geometry[1]);
        const u32 data_size = kLegacyGlyphAtlasKeyCount * mask_bytes;
        const std::size_t descriptor_offset = 32U + index * 16U;
        write_u16_le(atlas, descriptor_offset, geometry[0]);
        write_u16_le(atlas, descriptor_offset + 2U, geometry[1]);
        write_u16_le(atlas, descriptor_offset + 4U, row_bytes);
        write_u16_le(atlas, descriptor_offset + 6U, mask_bytes);
        write_u32_le(
            atlas,
            descriptor_offset + 8U,
            static_cast<u32>(data_offset)
        );
        write_u32_le(atlas, descriptor_offset + 12U, data_size);

        const std::size_t ascii_offset = data_offset
            + legacy_glyph_atlas_key_index(0x41U, 0U) * mask_bytes;
        const std::size_t dbcs_offset = data_offset
            + legacy_glyph_atlas_key_index(0xA4U, 0x40U) * mask_bytes;
        atlas[ascii_offset] = static_cast<u8>(0x80U >> index);
        atlas[dbcs_offset + mask_bytes - 1U] = static_cast<u8>(0x01U << index);
        data_offset += data_size;
    }
    return atlas;
}

[[nodiscard]] LegacyRawCharacter make_ascii() {
    return LegacyRawCharacter{
        .nul_terminated_bytes = {0x41U, 0U, 0U},
        .consumed_byte_count = 1U,
        .cache_key = 0x0041U,
    };
}

[[nodiscard]] LegacyRawCharacter make_dbcs() {
    return LegacyRawCharacter{
        .nul_terminated_bytes = {0xA4U, 0x40U, 0U},
        .consumed_byte_count = 2U,
        .cache_key = 0x40A4U,
    };
}

void test_valid_atlas_and_lookup(openswd3::test::Context& test) {
    const std::vector<u8> atlas = make_atlas();
    LegacyGlyphAtlasProvider provider(atlas);
    test.expect_true(provider.valid(), "valid atlas accepted");

    for (std::size_t index = 0; index < kLegacyGlyphAtlasGeometries.size(); ++index) {
        const auto geometry = kLegacyGlyphAtlasGeometries[index];
        const std::size_t mask_bytes =
            ((geometry[0] + 7U) / 8U) * geometry[1];
        std::vector<u8> destination(mask_bytes, 0xFFU);
        test.expect_equal(
            provider.provide_glyph_mask(
                make_ascii(), geometry[0], geometry[1], destination
            ),
            LegacyGlyphProviderStatus::completed,
            "ASCII lookup succeeds"
        );
        test.expect_equal(
            destination.front(),
            static_cast<u8>(0x80U >> index),
            "ASCII direct-index mask"
        );
        test.expect_true(
            std::ranges::all_of(
                destination.begin() + 1,
                destination.end(),
                [](const u8 value) { return value == 0U; }
            ),
            "ASCII mask copied exactly"
        );

        std::ranges::fill(destination, 0xFFU);
        test.expect_equal(
            provider.provide_glyph_mask(
                make_dbcs(), geometry[0], geometry[1], destination
            ),
            LegacyGlyphProviderStatus::completed,
            "DBCS lookup succeeds"
        );
        test.expect_equal(
            destination.back(),
            static_cast<u8>(0x01U << index),
            "DBCS direct-index mask"
        );
        test.expect_true(
            std::ranges::all_of(
                destination.begin(),
                destination.end() - 1,
                [](const u8 value) { return value == 0U; }
            ),
            "DBCS mask copied exactly"
        );
    }
}

void test_invalid_atlas_and_request_boundaries(openswd3::test::Context& test) {
    std::vector<u8> atlas = make_atlas();
    atlas[0] ^= 0x01U;
    LegacyGlyphAtlasProvider invalid_magic(atlas);
    test.expect_false(invalid_magic.valid(), "bad magic rejected");

    atlas = make_atlas();
    LegacyGlyphAtlasProvider truncated(
        std::span<const u8>(atlas).first(atlas.size() - 1U)
    );
    test.expect_false(truncated.valid(), "truncated payload rejected");

    LegacyGlyphAtlasProvider provider(atlas);
    std::array<u8, 24> destination{};
    test.expect_equal(
        provider.provide_glyph_mask(make_ascii(), 13, 13, destination),
        LegacyGlyphProviderStatus::failed,
        "unsupported geometry rejected"
    );
    test.expect_equal(
        provider.provide_glyph_mask(
            make_ascii(), 12, 12, std::span<u8>(destination).first(23U)
        ),
        LegacyGlyphProviderStatus::failed,
        "wrong destination size rejected"
    );

    LegacyRawCharacter inconsistent = make_dbcs();
    inconsistent.consumed_byte_count = 1U;
    test.expect_equal(
        provider.provide_glyph_mask(inconsistent, 12, 12, destination),
        LegacyGlyphProviderStatus::failed,
        "inconsistent raw character rejected"
    );
}

[[nodiscard]] std::vector<u8> read_binary_file(const char* path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        return {};
    }
    const std::streamoff size = stream.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<u8> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
    return stream ? bytes : std::vector<u8>{};
}

void expect_real_mask(
    openswd3::test::Context& test,
    LegacyGlyphAtlasProvider& provider,
    const LegacyRawCharacter& character,
    const u16 size,
    const std::span<const u8> expected
) {
    std::vector<u8> actual(expected.size(), 0U);
    test.expect_equal(
        provider.provide_glyph_mask(character, size, size, actual),
        LegacyGlyphProviderStatus::completed,
        "real atlas lookup succeeds"
    );
    test.expect_true(
        std::ranges::equal(actual, expected),
        "real atlas mask matches the original oracle"
    );
}

void test_real_atlas_asset(
    openswd3::test::Context& test,
    const char* atlas_path
) {
    const std::vector<u8> atlas = read_binary_file(atlas_path);
    LegacyGlyphAtlasProvider provider(atlas);
    test.expect_true(provider.valid(), "real atlas format and size");
    if (!provider.valid()) {
        return;
    }

    constexpr std::array<u8, 24> kMask12{
        0x01U, 0x40U, 0x7FU, 0xE0U, 0x41U, 0x00U, 0x7DU, 0x00U,
        0x49U, 0x60U, 0x7FU, 0x40U, 0x55U, 0x40U, 0x78U, 0x80U,
        0x4DU, 0xA0U, 0x52U, 0x60U, 0xACU, 0x20U, 0x00U, 0x00U,
    };
    constexpr std::array<u8, 32> kMask16{
        0x00U, 0x00U, 0x18U, 0x08U, 0x11U, 0xFCU, 0x20U, 0x88U,
        0x2CU, 0x88U, 0x48U, 0x90U, 0x78U, 0x92U, 0x10U, 0xFFU,
        0x24U, 0xA2U, 0x4EU, 0xA2U, 0x72U, 0x94U, 0x00U, 0x94U,
        0x52U, 0x88U, 0x49U, 0x14U, 0x29U, 0x22U, 0x22U, 0x41U,
    };
    constexpr std::array<u8, 60> kMask20{
        0x00U, 0x00U, 0x00U, 0x00U, 0xF0U, 0x00U, 0x01U, 0x08U,
        0x00U, 0x02U, 0x06U, 0x00U, 0x05U, 0xFDU, 0x80U, 0x18U,
        0x00U, 0x70U, 0x67U, 0xFFU, 0x20U, 0x04U, 0x01U, 0x00U,
        0x07U, 0xFFU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x3FU,
        0x00U, 0x0FU, 0xE0U, 0x00U, 0x00U, 0x20U, 0x00U, 0x1FU,
        0xFFU, 0x80U, 0x00U, 0x20U, 0x20U, 0x7FU, 0xFFU, 0xF0U,
        0x00U, 0x20U, 0x00U, 0x00U, 0x20U, 0x00U, 0x01U, 0xE0U,
        0x00U, 0x00U, 0x40U, 0x00U,
    };

    expect_real_mask(
        test,
        provider,
        LegacyRawCharacter{{0xABU, 0xC2U, 0U}, 2U, 0xC2ABU},
        12U,
        kMask12
    );
    expect_real_mask(
        test,
        provider,
        LegacyRawCharacter{{0xAFU, 0xC5U, 0U}, 2U, 0xC5AFU},
        16U,
        kMask16
    );
    expect_real_mask(
        test,
        provider,
        LegacyRawCharacter{{0xAEU, 0xB3U, 0U}, 2U, 0xB3AEU},
        20U,
        kMask20
    );
}

}  // namespace

int main(const int argument_count, const char* const* arguments) {
    openswd3::test::Context test;
    test_valid_atlas_and_lookup(test);
    test_invalid_atlas_and_request_boundaries(test);
    if (argument_count == 2) {
        test_real_atlas_asset(test, arguments[1]);
    } else {
        test.expect_equal(argument_count, 1, "optional real atlas path only");
    }
    return test.exit_code();
}
