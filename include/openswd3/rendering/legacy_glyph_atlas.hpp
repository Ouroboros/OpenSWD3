#pragma once

#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::rendering {

inline constexpr std::array<compat::u8, 8> kLegacyGlyphAtlasMagic{
    'O',
    'S',
    'W',
    '3',
    'G',
    'L',
    'Y',
    'F',
};
inline constexpr compat::u32 kLegacyGlyphAtlasVersion = 1U;
inline constexpr compat::u32 kLegacyGlyphAtlasHeaderSize = 80U;
inline constexpr compat::u32 kLegacyGlyphAtlasKeyCount = 32'896U;
inline constexpr compat::u32 kLegacyGlyphAtlasSectionCount = 3U;
inline constexpr std::array<std::array<compat::u16, 2>, 3>
    kLegacyGlyphAtlasGeometries{{{12U, 12U}, {16U, 16U}, {20U, 20U}}};

[[nodiscard]] constexpr compat::u32 legacy_glyph_atlas_key_index(
    const compat::u8 first_byte, const compat::u8 second_byte
) noexcept {
    if (first_byte < 0x80U) {
        return first_byte;
    }
    return 128U + (static_cast<compat::u32>(first_byte) - 0x80U) * 256U +
        second_byte;
}

class LegacyGlyphAtlasProvider final : public LegacyGlyphProvider {
public:
    explicit LegacyGlyphAtlasProvider(
        std::span<const compat::u8> atlas_bytes
    ) noexcept;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter& character,
        compat::i32 glyph_width,
        compat::i32 glyph_height,
        std::span<compat::u8> destination
    ) noexcept override;

private:
    struct Section {
        compat::i32 width{};
        compat::i32 height{};
        std::size_t mask_bytes{};
        std::size_t data_offset{};
    };

    std::span<const compat::u8> atlas_bytes_{};
    std::array<Section, kLegacyGlyphAtlasSectionCount> sections_{};
    bool valid_{};
};

}  // namespace openswd3::rendering
