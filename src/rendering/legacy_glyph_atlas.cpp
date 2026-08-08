#include "openswd3/rendering/legacy_glyph_atlas.hpp"

#include <algorithm>
#include <cstddef>

namespace openswd3::rendering {
namespace {

[[nodiscard]] compat::u16 read_u16_le(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset])
        | static_cast<compat::u16>(
            static_cast<compat::u16>(bytes[offset + 1U]) << 8U
        )
    );
}

[[nodiscard]] compat::u32 read_u32_le(
    const std::span<const compat::u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset])
        | (static_cast<compat::u32>(bytes[offset + 1U]) << 8U)
        | (static_cast<compat::u32>(bytes[offset + 2U]) << 16U)
        | (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacyGlyphAtlasProvider::LegacyGlyphAtlasProvider(
    const std::span<const compat::u8> atlas_bytes
) noexcept
    : atlas_bytes_(atlas_bytes) {
    if (atlas_bytes_.size() < kLegacyGlyphAtlasHeaderSize
        || !std::ranges::equal(
            kLegacyGlyphAtlasMagic,
            atlas_bytes_.first(kLegacyGlyphAtlasMagic.size())
        )
        || read_u32_le(atlas_bytes_, 8U) != kLegacyGlyphAtlasVersion
        || read_u32_le(atlas_bytes_, 12U) != kLegacyGlyphAtlasHeaderSize
        || read_u32_le(atlas_bytes_, 16U) != kLegacyGlyphAtlasKeyCount
        || read_u32_le(atlas_bytes_, 20U) != kLegacyGlyphAtlasSectionCount
        || read_u32_le(atlas_bytes_, 24U) != 0U
        || read_u32_le(atlas_bytes_, 28U) != 0U) {
        return;
    }

    std::size_t expected_data_offset = kLegacyGlyphAtlasHeaderSize;
    for (std::size_t index = 0; index < sections_.size(); ++index) {
        const std::size_t descriptor_offset = 32U + index * 16U;
        const compat::u16 width = read_u16_le(
            atlas_bytes_, descriptor_offset
        );
        const compat::u16 height = read_u16_le(
            atlas_bytes_, descriptor_offset + 2U
        );
        const compat::u16 row_bytes = read_u16_le(
            atlas_bytes_, descriptor_offset + 4U
        );
        const compat::u16 mask_bytes = read_u16_le(
            atlas_bytes_, descriptor_offset + 6U
        );
        const compat::u32 data_offset = read_u32_le(
            atlas_bytes_, descriptor_offset + 8U
        );
        const compat::u32 data_size = read_u32_le(
            atlas_bytes_, descriptor_offset + 12U
        );
        const auto expected_geometry = kLegacyGlyphAtlasGeometries[index];
        const compat::u16 expected_row_bytes = static_cast<compat::u16>(
            (expected_geometry[0] + 7U) / 8U
        );
        const compat::u16 expected_mask_bytes = static_cast<compat::u16>(
            expected_row_bytes * expected_geometry[1]
        );
        const std::size_t expected_data_size =
            static_cast<std::size_t>(kLegacyGlyphAtlasKeyCount)
            * expected_mask_bytes;

        if (width != expected_geometry[0]
            || height != expected_geometry[1]
            || row_bytes != expected_row_bytes
            || mask_bytes != expected_mask_bytes
            || data_offset != expected_data_offset
            || data_size != expected_data_size
            || expected_data_size > atlas_bytes_.size() - expected_data_offset) {
            return;
        }

        sections_[index] = Section{
            .width = width,
            .height = height,
            .mask_bytes = mask_bytes,
            .data_offset = expected_data_offset,
        };
        expected_data_offset += expected_data_size;
    }

    valid_ = expected_data_offset == atlas_bytes_.size();
}

bool LegacyGlyphAtlasProvider::valid() const noexcept {
    return valid_;
}

LegacyGlyphProviderStatus LegacyGlyphAtlasProvider::provide_glyph_mask(
    const LegacyRawCharacter& character,
    const compat::i32 glyph_width,
    const compat::i32 glyph_height,
    const std::span<compat::u8> destination
) noexcept {
    if (!valid_ || character.nul_terminated_bytes[2] != 0U) {
        return LegacyGlyphProviderStatus::failed;
    }

    const compat::u8 first_byte = character.nul_terminated_bytes[0];
    const compat::u8 second_byte = character.nul_terminated_bytes[1];
    const compat::u16 expected_key = static_cast<compat::u16>(
        static_cast<compat::u16>(first_byte)
        | static_cast<compat::u16>(
            static_cast<compat::u16>(second_byte) << 8U
        )
    );
    if ((first_byte < 0x80U
            && (character.consumed_byte_count != 1U || second_byte != 0U))
        || (first_byte >= 0x80U && character.consumed_byte_count != 2U)
        || character.cache_key != expected_key) {
        return LegacyGlyphProviderStatus::failed;
    }

    const auto section = std::ranges::find_if(
        sections_,
        [glyph_width, glyph_height](const Section& candidate) {
            return candidate.width == glyph_width
                && candidate.height == glyph_height;
        }
    );
    if (section == sections_.end()
        || destination.size() != section->mask_bytes) {
        return LegacyGlyphProviderStatus::failed;
    }

    const std::size_t source_offset = section->data_offset
        + static_cast<std::size_t>(legacy_glyph_atlas_key_index(
            first_byte, second_byte
        )) * section->mask_bytes;
    std::ranges::copy(
        atlas_bytes_.subspan(source_offset, section->mask_bytes),
        destination.begin()
    );
    return LegacyGlyphProviderStatus::completed;
}

}  // namespace openswd3::rendering
