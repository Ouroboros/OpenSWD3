#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <cstddef>
#include <span>

namespace openswd3::rendering {

enum class LegacyGlyphStyle : compat::u8 {
    none = 0x00U,
    single = 0x01U,
    shadow_below = 0x02U,
    doubled_shadow = 0x04U,
    outlined_single = 0x08U,
    outlined_double = 0x10U,
};

struct LegacyGlyphClipRectangle {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 width{};
    compat::i32 height{};
};

struct LegacyGlyphWriterState {
    compat::i32 glyph_height{};
    std::size_t mask_row_bytes{};
    compat::u16 secondary_color{};
    LegacyGlyphClipRectangle clip{};
};

struct LegacyGlyphDrawRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::u16 foreground_color{};
    compat::u32 flags{};
};

enum class LegacyGlyphWriteStatus : compat::u8 {
    completed,
    no_style,
    invalid_geometry,
    mask_out_of_bounds,
    destination_out_of_bounds,
};

struct LegacyGlyphWriteResult {
    LegacyGlyphWriteStatus status{LegacyGlyphWriteStatus::no_style};
    LegacyGlyphStyle style{LegacyGlyphStyle::none};
};

[[nodiscard]] LegacyGlyphStyle select_legacy_glyph_style(
    compat::u32 flags
) noexcept;

[[nodiscard]] LegacyGlyphWriteResult draw_legacy_glyph(
    LegacyFramebuffer& framebuffer,
    std::span<const compat::u8> mask,
    const LegacyGlyphWriterState& state,
    const LegacyGlyphDrawRequest& request
) noexcept;

enum class LegacyGlyphBackgroundStatus : compat::u8 {
    completed,
    disabled,
    destination_out_of_bounds,
};

struct LegacyGlyphBackgroundRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::u16 color{0xFFFEU};
};

[[nodiscard]] LegacyGlyphBackgroundStatus fill_legacy_glyph_background(
    LegacyFramebuffer& framebuffer,
    const LegacyGlyphBackgroundRequest& request
) noexcept;

}  // namespace openswd3::rendering
