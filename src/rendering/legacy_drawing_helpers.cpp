#include "openswd3/rendering/legacy_drawing_helpers.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>

namespace openswd3::rendering {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_subtract(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] bool pixel_is_available(
    const std::span<const u16> destination,
    const i32 x,
    const i32 y,
    const i32 pitch_pixels
) noexcept {
    const std::int64_t index =
        static_cast<std::int64_t>(y) * pitch_pixels + x;
    return index >= 0 &&
        static_cast<std::uint64_t>(index) < destination.size();
}

[[nodiscard]] bool border_destination_is_available(
    const LegacyAnimatedBorderRequest& request
) noexcept {
    if (request.width > kLegacyFramebufferWidth ||
        request.height > kLegacyFramebufferHeight) {
        return false;
    }

    for (i32 offset = 0; offset < request.width; ++offset) {
        if (!pixel_is_available(
                request.destination,
                wrapping_add(request.x, offset),
                request.y,
                request.pitch_pixels
            ) ||
            !pixel_is_available(
                request.destination,
                wrapping_subtract(
                    wrapping_add(request.x, request.width),
                    offset
                ),
                wrapping_add(request.y, request.height),
                request.pitch_pixels
            )) {
            return false;
        }
    }

    for (i32 offset = 0; offset < request.height; ++offset) {
        if (!pixel_is_available(
                request.destination,
                wrapping_add(request.x, request.width),
                wrapping_add(request.y, offset),
                request.pitch_pixels
            ) ||
            !pixel_is_available(
                request.destination,
                request.x,
                wrapping_subtract(
                    wrapping_add(request.y, request.height),
                    offset
                ),
                request.pitch_pixels
            )) {
            return false;
        }
    }

    return true;
}

void write_border_pixel(
    const LegacyAnimatedBorderRequest& request,
    const LegacyPixelConversionState& format,
    const i32 x,
    const i32 y,
    u32& local_phase,
    LegacyAnimatedBorderResult& result
) noexcept {
    const u32 level = (0x3FU - local_phase) & 0x1FU;
    const u32 pair = legacy_pack_color_pair(
        format,
        static_cast<i32>(level),
        static_cast<i32>(level),
        static_cast<i32>(level)
    );
    const std::int64_t index =
        static_cast<std::int64_t>(y) * request.pitch_pixels + x;
    request.destination[static_cast<std::size_t>(index)] =
        static_cast<u16>(pair);
    local_phase = (local_phase + 1U) & 0x1FU;
    ++result.pixel_writes;
}

[[nodiscard]] constexpr LegacyBlitClipRectangle current_clip(
    const LegacyRasterGeometryState& raster
) noexcept {
    return LegacyBlitClipRectangle{
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const LegacyBlitExecutionStatus status
) noexcept {
    return status == LegacyBlitExecutionStatus::completed ||
        status == LegacyBlitExecutionStatus::clipped_out ||
        status == LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] bool load_number_piece(
    LegacyFramePieceProvider& provider,
    const u32 resource_id,
    const u32 piece_index,
    LegacyFramePiece& piece,
    LegacyDecoratedNumberResult& result
) noexcept {
    result.requested_resource_id = resource_id;
    result.requested_piece_index = piece_index;
    ++result.piece_request_count;
    if (!provider.load_frame_piece(resource_id, piece_index, piece)) {
        result.status = LegacyDecoratedNumberStatus::piece_unavailable;
        return false;
    }
    if (piece.width == 0U || piece.height == 0U) {
        result.status = LegacyDecoratedNumberStatus::invalid_piece_geometry;
        return false;
    }
    return true;
}

[[nodiscard]] bool draw_number_piece(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyFramePiece& piece,
    const i32 x,
    const i32 y,
    const i32 opacity_step,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter,
    LegacyDecoratedNumberResult& result
) noexcept {
    const LegacyBlitResult blit = blit_legacy_copy_paths(
        framebuffer,
        current_clip(raster),
        piece.source,
        LegacyBlitRequest{
            .destination_x = x,
            .destination_y = y,
            .source_width = static_cast<i32>(piece.width),
            .source_height = static_cast<i32>(piece.height),
            .flags = 0x14U,
            .opacity_step = opacity_step,
        },
        effects,
        jitter
    );
    ++result.draw_call_count;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyDecoratedNumberStatus::blit_failed;
        return false;
    }
    return true;
}

}  // namespace

LegacyAnimatedBorderResult draw_legacy_animated_border(
    LegacyAnimatedBorderState& state,
    const LegacyPixelConversionState& format,
    const LegacyAnimatedBorderRequest& request
) noexcept {
    LegacyAnimatedBorderResult result;
    const i32 right = wrapping_add(request.x, request.width);
    const i32 bottom = wrapping_add(request.y, request.height);
    if (request.x < 0 || request.y < 0 ||
        right >= kLegacyFramebufferWidth ||
        bottom >= kLegacyFramebufferHeight) {
        result.status = LegacyAnimatedBorderStatus::rejected_bounds;
        return result;
    }
    if (!border_destination_is_available(request)) {
        result.status =
            LegacyAnimatedBorderStatus::destination_out_of_bounds;
        return result;
    }

    u32 local_phase = state.phase;
    for (i32 offset = 0; offset < request.width; ++offset) {
        write_border_pixel(
            request,
            format,
            wrapping_add(request.x, offset),
            request.y,
            local_phase,
            result
        );
    }
    for (i32 offset = 0; offset < request.height; ++offset) {
        write_border_pixel(
            request,
            format,
            right,
            wrapping_add(request.y, offset),
            local_phase,
            result
        );
    }
    for (i32 offset = 0; offset < request.width; ++offset) {
        write_border_pixel(
            request,
            format,
            wrapping_subtract(right, offset),
            bottom,
            local_phase,
            result
        );
    }
    for (i32 offset = 0; offset < request.height; ++offset) {
        write_border_pixel(
            request,
            format,
            request.x,
            wrapping_subtract(bottom, offset),
            local_phase,
            result
        );
    }

    state.phase = (state.phase + 1U) & 0x1FU;
    return result;
}

LegacyThumbnailDownsampleStatus downsample_legacy_thumbnail_in_place(
    const std::span<u16> pixels
) noexcept {
    if (pixels.size() < kLegacyFixedCanvasPixels) {
        return LegacyThumbnailDownsampleStatus::source_too_small;
    }

    std::size_t destination = 0U;
    for (i32 output_y = 0; output_y < kLegacyThumbnailHeight; ++output_y) {
        const std::size_t source_y =
            static_cast<std::size_t>(output_y) * 4U *
            static_cast<std::size_t>(kLegacyFramebufferWidth);
        for (i32 output_x = 0; output_x < kLegacyThumbnailWidth;
             ++output_x) {
            const std::size_t source = source_y +
                static_cast<std::size_t>(output_x) * 4U;
            pixels[destination] = pixels[source];
            ++destination;
        }
    }
    return LegacyThumbnailDownsampleStatus::completed;
}

LegacyDecoratedNumberResult draw_legacy_decorated_number(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    LegacyFramePieceProvider& provider,
    const LegacyDecoratedNumberRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept {
    LegacyDecoratedNumberResult result{
        .final_x = request.destination_x,
        .final_y = request.destination_y,
    };
    i32 x = request.destination_x;
    const i32 digit_y = wrapping_add(request.destination_y, 1);
    u32 remaining = request.value;

    do {
        const u32 digit = remaining % 10U;
        LegacyFramePiece piece{};
        if (!load_number_piece(
                provider,
                kLegacyNumberDigitResourceId,
                digit,
                piece,
                result
            )) {
            result.final_x = x;
            return result;
        }

        x = wrapping_subtract(
            wrapping_subtract(x, 2),
            static_cast<i32>(piece.width)
        );
        result.final_x = x;
        result.final_y = digit_y;
        if (!draw_number_piece(
                framebuffer,
                raster,
                piece,
                x,
                digit_y,
                request.opacity_step,
                effects,
                jitter,
                result
            )) {
            return result;
        }
        ++result.digit_count;
        remaining /= 10U;
    } while (remaining != 0U);

    LegacyFramePiece decoration{};
    if (!load_number_piece(
            provider,
            kLegacyNumberDecorationResourceId,
            0U,
            decoration,
            result
        )) {
        result.final_x = x;
        return result;
    }

    x = wrapping_subtract(x, 40);
    const i32 decoration_y = wrapping_subtract(request.destination_y, 8);
    result.final_x = x;
    result.final_y = decoration_y;
    static_cast<void>(request.unused_legacy_argument);
    static_cast<void>(draw_number_piece(
        framebuffer,
        raster,
        decoration,
        x,
        decoration_y,
        request.opacity_step,
        effects,
        jitter,
        result
    ));
    return result;
}

}  // namespace openswd3::rendering
