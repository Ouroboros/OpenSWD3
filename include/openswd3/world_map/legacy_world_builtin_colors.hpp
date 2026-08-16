#pragma once

#include "../compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::rendering {
struct LegacyPixelConversionState;
}

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldBuiltinColorCount = 16U;

// 0x0049E0C8..0x0049E107: convert the startup BGR888 table to duplicated
// native 16-bit color lanes.
[[nodiscard]] std::array<compat::u32, kLegacyWorldBuiltinColorCount>
legacy_world_builtin_color_pairs(
    const rendering::LegacyPixelConversionState& format
) noexcept;

// Bounded low-word lookup used by the ordinary-role 12-point label adapter.
[[nodiscard]] compat::u16 legacy_world_builtin_color(
    const rendering::LegacyPixelConversionState& format, compat::u32 index
) noexcept;

}  // namespace openswd3::world_map
