#include "openswd3/world_map/legacy_world_background.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

struct PixelRectangle {
    i32 left{};
    i32 top{};
    i32 right{};
    i32 bottom{};
};

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 floor_divide_16(const i32 value) noexcept {
    const u32 bits = to_bits(value);
    u32 shifted = bits >> 4U;
    if ((bits & 0x80000000U) != 0U) {
        shifted |= 0xF0000000U;
    }
    return from_bits(shifted);
}

[[nodiscard]] constexpr i32 low_nibble(const i32 value) noexcept {
    return static_cast<i32>(to_bits(value) & 0x0FU);
}

[[nodiscard]] constexpr i32 round_up_to_16(const i32 value) noexcept {
    return from_bits((to_bits(value) + 15U) & 0xFFFFFFF0U);
}

[[nodiscard]] PixelRectangle refresh_rectangle(
    const rendering::LegacyFramebuffer& framebuffer,
    const LegacyWorldBackgroundView& view
) noexcept {
    const auto& surface = framebuffer.geometry().surface;
    PixelRectangle rectangle{0, 0, surface.width, surface.height};
    if (!view.partial_refresh) {
        return rectangle;
    }

    const i32 center_x = round_up_to_16(view.partial_focus_x);
    const i32 center_y = round_up_to_16(view.partial_focus_y);
    rectangle.left = std::max(wrapping_subtract(center_x, 0xC0), 0);
    rectangle.top = std::max(wrapping_subtract(center_y, 0xC0), 0);
    rectangle.right = std::min(wrapping_add(center_x, 0xC0), surface.width);
    rectangle.bottom = std::min(wrapping_add(center_y, 0xC0), surface.height);
    return rectangle;
}

[[nodiscard]] bool read_cell_flags(
    const std::span<const u8> bytes, const std::size_t cell_index, u32& value
) noexcept {
    if (cell_index > std::numeric_limits<std::size_t>::max() / 4U) {
        return false;
    }
    const std::size_t offset = cell_index * 4U;
    if (offset > bytes.size() || bytes.size() - offset < 4U) {
        return false;
    }
    value = static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
    return true;
}

[[nodiscard]] bool read_direct_pixel(
    const LegacyWorldBackgroundSource& source,
    const u16 tile_index,
    const u32 tile_x,
    const u32 tile_y,
    u16& value
) noexcept {
    constexpr std::size_t kTileBytes = 0x200U;
    const std::size_t tile_offset =
        static_cast<std::size_t>(tile_index) * kTileBytes;
    const std::size_t pixel_offset = tile_offset +
        (static_cast<std::size_t>(tile_y) * kLegacyWorldTilePixels + tile_x) *
            2U;
    if (pixel_offset > source.tile_bytes.size() ||
        source.tile_bytes.size() - pixel_offset < 2U) {
        return false;
    }
    value = static_cast<u16>(
        static_cast<u16>(source.tile_bytes[pixel_offset]) |
        static_cast<u16>(
            static_cast<u16>(source.tile_bytes[pixel_offset + 1U]) << 8U
        )
    );
    return true;
}

[[nodiscard]] bool read_indexed_pixel(
    const LegacyWorldBackgroundSource& source,
    const u16 tile_index,
    const u32 tile_x,
    const u32 tile_y,
    u8& palette_index,
    u16& value
) noexcept {
    constexpr std::size_t kIndexedHeaderBytes = 0x200U;
    constexpr std::size_t kTileBytes = 0x100U;
    const std::size_t pixel_offset = kIndexedHeaderBytes +
        static_cast<std::size_t>(tile_index) * kTileBytes +
        static_cast<std::size_t>(tile_y) * kLegacyWorldTilePixels + tile_x;
    if (pixel_offset >= source.tile_bytes.size()) {
        return false;
    }
    palette_index = source.tile_bytes[pixel_offset];
    if (static_cast<std::size_t>(palette_index) >= source.palette.size()) {
        return false;
    }
    value = source.palette[palette_index];
    return true;
}

}  // namespace

LegacyWorldBackgroundRenderResult render_legacy_world_background(
    rendering::LegacyFramebuffer& framebuffer,
    const LegacyWorldBackgroundSource& source,
    const LegacyWorldBackgroundView& view
) noexcept {
    LegacyWorldBackgroundRenderResult result;
    const auto& surface = framebuffer.geometry().surface;
    if (surface.width != rendering::kLegacyFramebufferWidth ||
        surface.height != rendering::kLegacyFramebufferHeight ||
        surface.pitch_bytes < rendering::kLegacyFramebufferPitchBytes) {
        result.status = LegacyWorldBackgroundRenderStatus::invalid_framebuffer;
        return result;
    }
    if (source.map_width == 0U || source.map_height == 0U ||
        source.map_width >
            std::numeric_limits<std::size_t>::max() / source.map_height) {
        result.status = LegacyWorldBackgroundRenderStatus::invalid_map_geometry;
        return result;
    }

    const std::size_t cell_count =
        static_cast<std::size_t>(source.map_width) * source.map_height;
    if (source.tile_layer_offset > source.tile_indices.size() ||
        cell_count > source.tile_indices.size() - source.tile_layer_offset) {
        result.status =
            LegacyWorldBackgroundRenderStatus::tile_grid_out_of_bounds;
        return result;
    }
    if (cell_count > std::numeric_limits<std::size_t>::max() / 4U ||
        source.cell_flags.size() < cell_count * 4U) {
        result.status =
            LegacyWorldBackgroundRenderStatus::cell_grid_out_of_bounds;
        return result;
    }
    if (source.pixel_layout == LegacyWorldBackgroundPixelLayout::indexed_8 &&
        source.palette.size() < 256U) {
        result.status =
            LegacyWorldBackgroundRenderStatus::palette_out_of_bounds;
        return result;
    }

    const PixelRectangle redraw = refresh_rectangle(framebuffer, view);
    if (redraw.left >= redraw.right || redraw.top >= redraw.bottom) {
        result.status = LegacyWorldBackgroundRenderStatus::completed;
        return result;
    }

    const i32 first_screen_x = -low_nibble(view.camera_left);
    const i32 first_screen_y = -low_nibble(view.camera_top);
    const i32 first_cell_x = floor_divide_16(view.camera_left);
    const i32 first_cell_y = floor_divide_16(view.camera_top);

    for (i32 screen_y = first_screen_y, cell_y = first_cell_y;
         screen_y < redraw.bottom;
         screen_y += static_cast<i32>(kLegacyWorldTilePixels), ++cell_y) {
        if (screen_y + static_cast<i32>(kLegacyWorldTilePixels) <= redraw.top) {
            continue;
        }
        for (i32 screen_x = first_screen_x, cell_x = first_cell_x;
             screen_x < redraw.right;
             screen_x += static_cast<i32>(kLegacyWorldTilePixels), ++cell_x) {
            if (screen_x + static_cast<i32>(kLegacyWorldTilePixels) <=
                redraw.left) {
                continue;
            }
            if (cell_x < 0 || cell_y < 0 ||
                static_cast<u32>(cell_x) >= source.map_width ||
                static_cast<u32>(cell_y) >= source.map_height) {
                continue;
            }

            const std::size_t cell_index =
                static_cast<std::size_t>(cell_y) * source.map_width +
                static_cast<u32>(cell_x);
            u32 flags{};
            if (!read_cell_flags(source.cell_flags, cell_index, flags)) {
                result.status =
                    LegacyWorldBackgroundRenderStatus::cell_grid_out_of_bounds;
                return result;
            }
            ++result.visited_cells;
            if ((flags & kLegacyWorldCellHidden) != 0U) {
                ++result.hidden_cells;
                continue;
            }

            const bool transparent =
                (flags & kLegacyWorldCellTransparent) != 0U;
            if (transparent) {
                ++result.transparent_cells;
            } else {
                ++result.opaque_cells;
            }
            const u16 tile_index =
                source.tile_indices
                    [static_cast<std::size_t>(source.tile_layer_offset) +
                     cell_index];

            const i32 clipped_left = std::max(screen_x, redraw.left);
            const i32 clipped_top = std::max(screen_y, redraw.top);
            const i32 clipped_right = std::min(
                screen_x + static_cast<i32>(kLegacyWorldTilePixels),
                redraw.right
            );
            const i32 clipped_bottom = std::min(
                screen_y + static_cast<i32>(kLegacyWorldTilePixels),
                redraw.bottom
            );
            for (i32 destination_y = clipped_top;
                 destination_y < clipped_bottom;
                 ++destination_y) {
                std::span<u16> row =
                    framebuffer.row_pixels(static_cast<u32>(destination_y));
                const u32 tile_y = static_cast<u32>(destination_y - screen_y);
                for (i32 destination_x = clipped_left;
                     destination_x < clipped_right;
                     ++destination_x) {
                    const u32 tile_x =
                        static_cast<u32>(destination_x - screen_x);
                    u16 pixel{};
                    if (source.pixel_layout ==
                        LegacyWorldBackgroundPixelLayout::direct_16) {
                        if (!read_direct_pixel(
                                source, tile_index, tile_x, tile_y, pixel
                            )) {
                            result.status = LegacyWorldBackgroundRenderStatus::
                                tile_source_out_of_bounds;
                            return result;
                        }
                        if (transparent && pixel == source.transparent_pixel) {
                            continue;
                        }
                    } else {
                        u8 palette_index{};
                        if (!read_indexed_pixel(
                                source,
                                tile_index,
                                tile_x,
                                tile_y,
                                palette_index,
                                pixel
                            )) {
                            result.status = LegacyWorldBackgroundRenderStatus::
                                tile_source_out_of_bounds;
                            return result;
                        }
                        if (transparent && palette_index == 1U) {
                            continue;
                        }
                    }
                    row[static_cast<std::size_t>(destination_x)] = pixel;
                    ++result.written_pixels;
                }
            }
        }
    }

    result.status = LegacyWorldBackgroundRenderStatus::completed;
    return result;
}

}  // namespace openswd3::world_map
