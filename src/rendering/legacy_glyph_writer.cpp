#include "openswd3/rendering/legacy_glyph_writer.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>

namespace openswd3::rendering {
namespace {

struct GlyphPixelWrite {
    compat::i32 x_offset{};
    compat::i32 y_offset{};
    compat::u16 color{};
};

[[nodiscard]] constexpr compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::u16
advance_row_color(const compat::u16 color, const compat::i32 delta) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u32>(color) + std::bit_cast<compat::u32>(delta)
    );
}

[[nodiscard]] constexpr compat::i32
row_color_delta(const compat::u32 flags) noexcept {
    if ((flags & 0x100U) != 0U) {
        return -1;
    }
    return (flags & 0x80U) != 0U ? 1 : 0;
}

[[nodiscard]] bool valid_mask_geometry(
    const LegacyGlyphWriterState& state, std::size_t& required_bytes
) noexcept {
    if (state.glyph_height < 0 ||
        state.mask_row_bytes >
            static_cast<std::size_t>(std::numeric_limits<compat::i32>::max()) /
                8U) {
        return false;
    }

    const std::size_t height = static_cast<std::size_t>(state.glyph_height);
    if (state.mask_row_bytes != 0U &&
        height >
            std::numeric_limits<std::size_t>::max() / state.mask_row_bytes) {
        return false;
    }

    required_bytes = height * state.mask_row_bytes;
    return true;
}

[[nodiscard]] bool mask_pixel_set(
    const std::span<const compat::u8> mask,
    const std::size_t row,
    const std::size_t column,
    const std::size_t row_bytes
) noexcept {
    const compat::u8 byte = mask[row * row_bytes + column / 8U];
    const auto bit = static_cast<compat::u8>(
        0x80U >> static_cast<unsigned int>(column & 7U)
    );
    return (byte & bit) != 0U;
}

[[nodiscard]] bool framebuffer_contains(
    const LegacyFramebuffer& framebuffer,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    const LegacySurfaceGeometry& surface = framebuffer.geometry().surface;
    return x >= 0 && y >= 0 && x < surface.width && y < surface.height;
}

template <std::size_t Size>
[[nodiscard]] bool write_pixels(
    LegacyFramebuffer& framebuffer,
    const compat::i32 x,
    const compat::i32 y,
    const std::array<GlyphPixelWrite, Size>& writes
) noexcept {
    for (const GlyphPixelWrite& write : writes) {
        if (!framebuffer_contains(
                framebuffer,
                wrapping_add(x, write.x_offset),
                wrapping_add(y, write.y_offset)
            )) {
            return false;
        }
    }

    for (const GlyphPixelWrite& write : writes) {
        const compat::i32 target_x = wrapping_add(x, write.x_offset);
        const compat::i32 target_y = wrapping_add(y, write.y_offset);
        framebuffer.row_pixels(
            static_cast<compat::u32>(target_y)
        )[static_cast<std::size_t>(target_x)] = write.color;
    }
    return true;
}

[[nodiscard]] bool draw_single_overlay(
    LegacyFramebuffer& framebuffer,
    const std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request,
    compat::u16 foreground
) noexcept {
    const compat::i32 right = wrapping_add(state.clip.left, state.clip.width);
    const compat::i32 bottom = wrapping_add(state.clip.top, state.clip.height);
    const compat::i32 delta = row_color_delta(request.flags);
    const std::size_t columns = state.mask_row_bytes * 8U;

    for (compat::i32 row = 0; row < state.glyph_height; ++row) {
        const compat::i32 y = wrapping_add(request.destination_y, row);
        for (std::size_t column = 0U; column < columns; ++column) {
            if (!mask_pixel_set(
                    mask,
                    static_cast<std::size_t>(row),
                    column,
                    state.mask_row_bytes
                )) {
                continue;
            }

            const compat::i32 x = wrapping_add(
                request.destination_x, static_cast<compat::i32>(column)
            );
            if (y < state.clip.top || y >= bottom || x < state.clip.left ||
                wrapping_add(x, 1) >= right) {
                continue;
            }

            const std::array writes{
                GlyphPixelWrite{.color = foreground},
            };
            if (!write_pixels(framebuffer, x, y, writes)) {
                return false;
            }
        }
        foreground = advance_row_color(foreground, delta);
    }
    return true;
}

[[nodiscard]] bool draw_double_overlay(
    LegacyFramebuffer& framebuffer,
    const std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request,
    compat::u16 foreground
) noexcept {
    const compat::i32 right = wrapping_add(state.clip.left, state.clip.width);
    const compat::i32 bottom = wrapping_add(state.clip.top, state.clip.height);
    const compat::i32 delta = row_color_delta(request.flags);
    const std::size_t columns = state.mask_row_bytes * 8U;

    for (compat::i32 row = 0; row < state.glyph_height; ++row) {
        const compat::i32 y = wrapping_add(request.destination_y, row);
        for (std::size_t column = 0U; column < columns; ++column) {
            if (!mask_pixel_set(
                    mask,
                    static_cast<std::size_t>(row),
                    column,
                    state.mask_row_bytes
                )) {
                continue;
            }

            const compat::i32 x = wrapping_add(
                request.destination_x, static_cast<compat::i32>(column)
            );
            if (y < state.clip.top || y >= bottom || x < state.clip.left ||
                wrapping_add(x, 2) >= right) {
                continue;
            }

            const std::array writes{
                GlyphPixelWrite{.color = foreground},
                GlyphPixelWrite{.x_offset = 1, .color = foreground},
            };
            if (!write_pixels(framebuffer, x, y, writes)) {
                return false;
            }
        }
        foreground = advance_row_color(foreground, delta);
    }
    return true;
}

[[nodiscard]] bool draw_shadow_below(
    LegacyFramebuffer& framebuffer,
    const std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request
) noexcept {
    const compat::i32 right = wrapping_add(state.clip.left, state.clip.width);
    const compat::i32 bottom = wrapping_add(state.clip.top, state.clip.height);
    const compat::i32 delta = row_color_delta(request.flags);
    const std::size_t columns = state.mask_row_bytes * 8U;
    compat::u16 foreground = request.foreground_color;

    for (compat::i32 row = 0; row < state.glyph_height; ++row) {
        const compat::i32 y = wrapping_add(request.destination_y, row);
        for (std::size_t column = 0U; column < columns; ++column) {
            if (!mask_pixel_set(
                    mask,
                    static_cast<std::size_t>(row),
                    column,
                    state.mask_row_bytes
                )) {
                continue;
            }

            const compat::i32 x = wrapping_add(
                request.destination_x, static_cast<compat::i32>(column)
            );
            if (y < state.clip.top || wrapping_add(y, 1) >= bottom ||
                x < state.clip.left || wrapping_add(x, 1) >= right) {
                continue;
            }

            const std::array writes{
                GlyphPixelWrite{.color = foreground},
                GlyphPixelWrite{
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = 1,
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
            };
            if (!write_pixels(framebuffer, x, y, writes)) {
                return false;
            }
        }
        foreground = advance_row_color(foreground, delta);
    }
    return true;
}

[[nodiscard]] bool draw_doubled_shadow(
    LegacyFramebuffer& framebuffer,
    const std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request
) noexcept {
    const compat::i32 right = wrapping_add(state.clip.left, state.clip.width);
    const compat::i32 bottom = wrapping_add(state.clip.top, state.clip.height);
    const compat::i32 delta = row_color_delta(request.flags);
    const std::size_t columns = state.mask_row_bytes * 8U;
    compat::u16 foreground = request.foreground_color;

    for (compat::i32 row = 0; row < state.glyph_height; ++row) {
        const compat::i32 y = wrapping_add(request.destination_y, row);
        for (std::size_t column = 0U; column < columns; ++column) {
            if (!mask_pixel_set(
                    mask,
                    static_cast<std::size_t>(row),
                    column,
                    state.mask_row_bytes
                )) {
                continue;
            }

            const compat::i32 x = wrapping_add(
                request.destination_x, static_cast<compat::i32>(column)
            );
            if (y < state.clip.top || wrapping_add(y, 1) >= bottom ||
                x < state.clip.left || wrapping_add(x, 2) >= right) {
                continue;
            }

            const std::array writes{
                GlyphPixelWrite{.color = foreground},
                GlyphPixelWrite{.x_offset = 1, .color = foreground},
                GlyphPixelWrite{
                    .x_offset = 1,
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = 2,
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
            };
            if (!write_pixels(framebuffer, x, y, writes)) {
                return false;
            }
        }
        foreground = advance_row_color(foreground, delta);
    }
    return true;
}

[[nodiscard]] bool draw_outline_prepass(
    LegacyFramebuffer& framebuffer,
    const std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request,
    const bool doubled
) noexcept {
    const compat::i32 right = wrapping_add(state.clip.left, state.clip.width);
    const compat::i32 bottom = wrapping_add(state.clip.top, state.clip.height);
    const std::size_t columns = state.mask_row_bytes * 8U;

    for (compat::i32 row = 0; row < state.glyph_height; ++row) {
        const compat::i32 y = wrapping_add(request.destination_y, row);
        for (std::size_t column = 0U; column < columns; ++column) {
            if (!mask_pixel_set(
                    mask,
                    static_cast<std::size_t>(row),
                    column,
                    state.mask_row_bytes
                )) {
                continue;
            }

            const compat::i32 x = wrapping_add(
                request.destination_x, static_cast<compat::i32>(column)
            );
            const compat::i32 required_right = doubled ? 2 : 1;
            if (y <= state.clip.top || wrapping_add(y, 1) >= bottom ||
                x < wrapping_subtract(state.clip.left, 1) ||
                wrapping_add(x, required_right) >= right) {
                continue;
            }

            if (!doubled) {
                const std::array writes{
                    GlyphPixelWrite{
                        .y_offset = -1,
                        .color = state.secondary_color,
                    },
                    GlyphPixelWrite{
                        .x_offset = -1,
                        .color = state.secondary_color,
                    },
                    GlyphPixelWrite{.color = state.secondary_color},
                    GlyphPixelWrite{
                        .x_offset = -1,
                        .y_offset = 1,
                        .color = state.secondary_color,
                    },
                    GlyphPixelWrite{
                        .y_offset = 1,
                        .color = state.secondary_color,
                    },
                    GlyphPixelWrite{
                        .x_offset = 1,
                        .y_offset = 1,
                        .color = state.secondary_color,
                    },
                };
                if (!write_pixels(framebuffer, x, y, writes)) {
                    return false;
                }
                continue;
            }

            const std::array writes{
                GlyphPixelWrite{
                    .y_offset = -1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = -1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = 2,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = -1,
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = 1,
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
                GlyphPixelWrite{
                    .x_offset = 2,
                    .y_offset = 1,
                    .color = state.secondary_color,
                },
            };
            if (!write_pixels(framebuffer, x, y, writes)) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] compat::u16 foreground_after_prepass(
    compat::u16 foreground, const compat::i32 height, const compat::i32 delta
) noexcept {
    for (compat::i32 row = 0; row < height; ++row) {
        foreground = advance_row_color(foreground, delta);
    }
    return foreground;
}

}  // namespace

LegacyGlyphStyle select_legacy_glyph_style(const compat::u32 flags) noexcept {
    constexpr std::array<LegacyGlyphStyle, 5> kPriority{
        LegacyGlyphStyle::single,
        LegacyGlyphStyle::shadow_below,
        LegacyGlyphStyle::doubled_shadow,
        LegacyGlyphStyle::outlined_single,
        LegacyGlyphStyle::outlined_double,
    };
    for (const LegacyGlyphStyle style : kPriority) {
        if ((flags & static_cast<compat::u32>(style)) != 0U) {
            return style;
        }
    }
    return LegacyGlyphStyle::none;
}

LegacyGlyphWriteResult draw_legacy_glyph(
    LegacyFramebuffer& framebuffer,
    const std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request
) noexcept {
    const LegacyGlyphStyle style = select_legacy_glyph_style(request.flags);
    if (style == LegacyGlyphStyle::none) {
        return LegacyGlyphWriteResult{
            .status = LegacyGlyphWriteStatus::no_style,
            .style = style,
        };
    }

    std::size_t required_bytes{};
    if (!valid_mask_geometry(state, required_bytes)) {
        return LegacyGlyphWriteResult{
            .status = LegacyGlyphWriteStatus::invalid_geometry,
            .style = style,
        };
    }
    if (mask.size() < required_bytes) {
        return LegacyGlyphWriteResult{
            .status = LegacyGlyphWriteStatus::mask_out_of_bounds,
            .style = style,
        };
    }

    bool completed = false;
    switch (style) {
    case LegacyGlyphStyle::single:
        completed = draw_single_overlay(
            framebuffer, mask, state, request, request.foreground_color
        );
        break;

    case LegacyGlyphStyle::shadow_below:
        completed = draw_shadow_below(framebuffer, mask, state, request);
        break;

    case LegacyGlyphStyle::doubled_shadow:
        completed = draw_doubled_shadow(framebuffer, mask, state, request);
        break;

    case LegacyGlyphStyle::outlined_single: {
        completed =
            draw_outline_prepass(framebuffer, mask, state, request, false);
        if (completed) {
            completed = draw_single_overlay(
                framebuffer,
                mask,
                state,
                request,
                foreground_after_prepass(
                    request.foreground_color,
                    state.glyph_height,
                    row_color_delta(request.flags)
                )
            );
        }
        break;
    }

    case LegacyGlyphStyle::outlined_double: {
        completed =
            draw_outline_prepass(framebuffer, mask, state, request, true);
        if (completed) {
            completed = draw_double_overlay(
                framebuffer,
                mask,
                state,
                request,
                foreground_after_prepass(
                    request.foreground_color,
                    state.glyph_height,
                    row_color_delta(request.flags)
                )
            );
        }
        break;
    }

    case LegacyGlyphStyle::none:
        break;
    }

    return LegacyGlyphWriteResult{
        .status = completed ? LegacyGlyphWriteStatus::completed
                            : LegacyGlyphWriteStatus::destination_out_of_bounds,
        .style = style,
    };
}

LegacyGlyphBackgroundStatus fill_legacy_glyph_background(
    LegacyFramebuffer& framebuffer, const LegacyGlyphBackgroundRequest& request
) noexcept {
    if (request.color == 0xFFFEU) {
        return LegacyGlyphBackgroundStatus::disabled;
    }

    const LegacySurfaceGeometry& surface = framebuffer.geometry().surface;
    compat::i32 left = request.destination_x;
    compat::i32 top = request.destination_y;
    compat::i32 right = wrapping_add(request.destination_x, request.width);
    compat::i32 bottom = wrapping_add(request.destination_y, request.height);

    if (left < 0) {
        left = 0;
    }
    if (right >= surface.width) {
        right = surface.width;
    }
    if (top < 0) {
        top = 0;
    }
    if (bottom >= surface.height) {
        bottom = surface.height;
    }

    if (left >= right) {
        return LegacyGlyphBackgroundStatus::completed;
    }
    if (top < 0 || top >= surface.height || left < 0 || right > surface.width) {
        return LegacyGlyphBackgroundStatus::destination_out_of_bounds;
    }

    std::span<compat::u16> first_row =
        framebuffer.row_pixels(static_cast<compat::u32>(top))
            .subspan(
                static_cast<std::size_t>(left),
                static_cast<std::size_t>(right - left)
            );
    std::ranges::fill(first_row, request.color);

    for (compat::i32 row = wrapping_add(top, 1); row < bottom; ++row) {
        if (row < 0 || row >= surface.height) {
            return LegacyGlyphBackgroundStatus::destination_out_of_bounds;
        }
        std::span<compat::u16> destination =
            framebuffer.row_pixels(static_cast<compat::u32>(row))
                .subspan(
                    static_cast<std::size_t>(left),
                    static_cast<std::size_t>(right - left)
                );
        std::ranges::copy(first_row, destination.begin());
    }

    return LegacyGlyphBackgroundStatus::completed;
}

}  // namespace openswd3::rendering
