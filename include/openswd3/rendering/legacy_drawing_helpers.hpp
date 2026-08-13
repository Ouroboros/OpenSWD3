#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <span>

namespace openswd3::rendering {

struct LegacyAnimatedBorderState {
    compat::u32 phase{};
};

struct LegacyAnimatedBorderRequest {
    std::span<compat::u16> destination{};
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::i32 pitch_pixels{};
};

enum class LegacyAnimatedBorderStatus : compat::u8 {
    completed,
    rejected_bounds,
    destination_out_of_bounds,
};

struct LegacyAnimatedBorderResult {
    LegacyAnimatedBorderStatus status{LegacyAnimatedBorderStatus::completed};
    compat::u32 pixel_writes{};
};

[[nodiscard]] LegacyAnimatedBorderResult draw_legacy_animated_border(
    LegacyAnimatedBorderState& state,
    const LegacyPixelConversionState& format,
    const LegacyAnimatedBorderRequest& request
) noexcept;

inline constexpr compat::i32 kLegacyThumbnailWidth = 160;
inline constexpr compat::i32 kLegacyThumbnailHeight = 120;
inline constexpr compat::u32 kLegacyThumbnailPixels = 0x4B00U;

enum class LegacyThumbnailDownsampleStatus : compat::u8 {
    completed,
    source_too_small,
};

[[nodiscard]] LegacyThumbnailDownsampleStatus
downsample_legacy_thumbnail_in_place(std::span<compat::u16> pixels) noexcept;

inline constexpr compat::u32 kLegacyNumberDigitResourceId = 0x2354U;
inline constexpr compat::u32 kLegacyNumberDecorationResourceId = 0x245EU;

struct LegacyDecoratedNumberRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 unused_legacy_argument{};
    compat::u32 value{};
    compat::i32 opacity_step{};
};

enum class LegacyDecoratedNumberStatus : compat::u8 {
    completed,
    piece_unavailable,
    invalid_piece_geometry,
    blit_failed,
};

struct LegacyDecoratedNumberResult {
    LegacyDecoratedNumberStatus status{LegacyDecoratedNumberStatus::completed};
    compat::u32 requested_resource_id{};
    compat::u32 requested_piece_index{};
    compat::u32 piece_request_count{};
    compat::u32 digit_count{};
    compat::u32 draw_call_count{};
    compat::i32 final_x{};
    compat::i32 final_y{};
    LegacyBlitExecutionStatus blit_status{LegacyBlitExecutionStatus::completed};
};

[[nodiscard]] LegacyDecoratedNumberResult draw_legacy_decorated_number(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    LegacyFramePieceProvider& provider,
    const LegacyDecoratedNumberRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

}  // namespace openswd3::rendering
