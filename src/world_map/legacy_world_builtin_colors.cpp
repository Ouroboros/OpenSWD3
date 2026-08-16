#include "../../include/openswd3/world_map/legacy_world_builtin_colors.hpp"

#include "../../include/openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <cstddef>

namespace openswd3::world_map {
namespace {

constexpr std::array<compat::u32, kLegacyWorldBuiltinColorCount>
    kLegacyWorldBuiltinBgr888Colors{
        0x00FFFFFFU,
        0x00000000U,
        0x000C31ECU,
        0x000080FFU,
        0x002C577BU,
        0x00FFE6E6U,
        0x00ACCFE9U,
        0x00002CECU,
        0x00FF0000U,
        0x00800000U,
        0x00606060U,
        0x002C577BU,
        0x00E9C8C0U,
        0x00ACCFE9U,
        0x000D31ECU,
        0x00002CECU,
    };

}  // namespace

std::array<compat::u32, kLegacyWorldBuiltinColorCount>
legacy_world_builtin_color_pairs(
    const rendering::LegacyPixelConversionState& format
) noexcept {
    std::array<compat::u32, kLegacyWorldBuiltinColorCount> pairs{};
    for (std::size_t index = 0U; index < pairs.size(); ++index) {
        const compat::u32 color = kLegacyWorldBuiltinBgr888Colors[index];
        pairs[index] = rendering::legacy_pack_color_pair(
            format,
            static_cast<compat::i32>((color >> 3U) & 0x1FU),
            static_cast<compat::i32>((color >> 11U) & 0x1FU),
            static_cast<compat::i32>((color >> 19U) & 0x1FU)
        );
    }
    return pairs;
}

compat::u16 legacy_world_builtin_color(
    const rendering::LegacyPixelConversionState& format, const compat::u32 index
) noexcept {
    if (index >= kLegacyWorldBuiltinColorCount) {
        return 0U;
    }
    return static_cast<compat::u16>(
        legacy_world_builtin_color_pairs(format)[index]
    );
}

}  // namespace openswd3::world_map
