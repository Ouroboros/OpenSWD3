#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldTilePixels = 16U;
inline constexpr compat::u32 kLegacyWorldCellHidden = 0x08000000U;
inline constexpr compat::u32 kLegacyWorldCellTransparent = 0x04000000U;

enum class LegacyWorldBackgroundPixelLayout : compat::u8 {
    direct_16,
    indexed_8,
};

struct LegacyWorldBackgroundSource {
    compat::u32 map_width{};
    compat::u32 map_height{};
    compat::u32 tile_layer_offset{};
    std::span<const compat::u16> tile_indices{};
    std::span<const compat::u8> cell_flags{};
    std::span<const compat::u8> tile_bytes{};
    LegacyWorldBackgroundPixelLayout pixel_layout{
        LegacyWorldBackgroundPixelLayout::direct_16
    };
    std::span<const compat::u16> palette{};
    compat::u16 transparent_pixel{};
};

struct LegacyWorldBackgroundView {
    compat::i32 camera_left{};
    compat::i32 camera_top{};
    // The four background functions use this for their internal service-13
    // tile bounds. They round focus upward to 16 independently of 00412930's
    // exact-coordinate outer raster clip.
    bool partial_refresh{};
    compat::i32 partial_focus_x{};
    compat::i32 partial_focus_y{};
};

enum class LegacyWorldBackgroundRenderStatus : compat::u8 {
    completed,
    invalid_framebuffer,
    invalid_map_geometry,
    tile_grid_out_of_bounds,
    cell_grid_out_of_bounds,
    tile_source_out_of_bounds,
    palette_out_of_bounds,
};

struct LegacyWorldBackgroundRenderResult {
    LegacyWorldBackgroundRenderStatus status{
        LegacyWorldBackgroundRenderStatus::invalid_map_geometry
    };
    compat::u32 visited_cells{};
    compat::u32 opaque_cells{};
    compat::u32 transparent_cells{};
    compat::u32 hidden_cells{};
    compat::u32 written_pixels{};
};

// Pixel-equivalent recovery of the four background paths selected by
// 0x00412930: 0x00412BE0/0x00412D30 for 16-bit CM tiles and
// 0x00413220/0x00413370 for indexed CM tiles. The original splits aligned
// interior tiles from clipped edge tiles; this interface exposes their common
// final framebuffer result while preserving the cell flags and source layout.
[[nodiscard]] LegacyWorldBackgroundRenderResult render_legacy_world_background(
    rendering::LegacyFramebuffer& framebuffer,
    const LegacyWorldBackgroundSource& source,
    const LegacyWorldBackgroundView& view
) noexcept;

}  // namespace openswd3::world_map
