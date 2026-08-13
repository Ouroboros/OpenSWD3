#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <span>

namespace openswd3::rendering {

inline constexpr compat::i32 kLegacyFixedTileExtent = 16;

enum class LegacyFixedTileWriteStatus : compat::u8 {
    completed,
    invalid_geometry,
    source_out_of_bounds,
    palette_out_of_bounds,
    destination_out_of_bounds,
};

// sub_4174D0.
[[nodiscard]] LegacyFixedTileWriteStatus write_legacy_direct_16x16_tile(
    LegacyFramebuffer& framebuffer,
    compat::i32 destination_x,
    compat::i32 destination_y,
    std::span<const compat::u8> source
) noexcept;

// sub_417530.
[[nodiscard]] LegacyFixedTileWriteStatus write_legacy_direct_keyed_16x16_tile(
    LegacyFramebuffer& framebuffer,
    compat::i32 destination_x,
    compat::i32 destination_y,
    std::span<const compat::u8> source,
    compat::u16 transparent_color
) noexcept;

// sub_4175B0.
[[nodiscard]] LegacyFixedTileWriteStatus write_legacy_indexed_16x16_tile(
    LegacyFramebuffer& framebuffer,
    compat::i32 destination_x,
    compat::i32 destination_y,
    std::span<const compat::u8> source,
    std::span<const compat::u16> palette
) noexcept;

// sub_417650. Palette index 1 is the fixed transparent value.
[[nodiscard]] LegacyFixedTileWriteStatus write_legacy_indexed_keyed_16x16_tile(
    LegacyFramebuffer& framebuffer,
    compat::i32 destination_x,
    compat::i32 destination_y,
    std::span<const compat::u8> source,
    std::span<const compat::u16> palette
) noexcept;

}  // namespace openswd3::rendering
