#include "openswd3/rendering/legacy_countdown.hpp"

#include <bit>

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

[[nodiscard]] constexpr i32 wrapping_multiply(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] constexpr u32 countdown_ticks(
    const i32 minutes,
    const i32 seconds
) noexcept {
    const i32 total_seconds = wrapping_add(
        wrapping_multiply(minutes, 60),
        seconds
    );
    return to_bits(wrapping_multiply(total_seconds, 30));
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

class CountdownDrawer final {
public:
    CountdownDrawer(
        LegacyFramebuffer& framebuffer,
        const LegacyRasterGeometryState& raster,
        LegacyCountdownPieceProvider& provider,
        const LegacyBlitEffectState& effects,
        LegacyRleRowJitterState& jitter,
        const i32 destination_y,
        const i32 destination_x
    ) noexcept
        : framebuffer_(framebuffer),
          raster_(raster),
          provider_(provider),
          effects_(effects),
          jitter_(jitter),
          destination_y_(destination_y),
          destination_x_(destination_x) {}

    [[nodiscard]] bool draw(const i32 action_index) noexcept {
        result_.piece_index = action_index;
        ++result_.piece_request_count;

        LegacyFramePiece piece{};
        if (!provider_.load_countdown_piece(
                kLegacyCountdownActionId,
                action_index,
                piece
            )) {
            result_.status =
                LegacyCountdownDisplayStatus::piece_unavailable;
            return false;
        }
        if (piece.width == 0U || piece.height == 0U) {
            result_.status =
                LegacyCountdownDisplayStatus::invalid_piece_geometry;
            return false;
        }

        const LegacyBlitResult blit = blit_legacy_copy_paths(
            framebuffer_,
            current_clip(raster_),
            piece.source,
            LegacyBlitRequest{
                .destination_x = destination_x_,
                .destination_y = destination_y_,
                .source_width = static_cast<i32>(piece.width),
                .source_height = static_cast<i32>(piece.height),
                .flags = 0U,
                .opacity_step = 0,
            },
            effects_,
            jitter_
        );
        ++result_.draw_call_count;
        result_.blit_status = blit.status;
        if (!accepted_blit_status(blit.status)) {
            result_.status = LegacyCountdownDisplayStatus::blit_failed;
            return false;
        }

        destination_x_ = wrapping_add(
            destination_x_,
            static_cast<i32>(piece.width)
        );
        return true;
    }

    [[nodiscard]] LegacyCountdownDisplayResult result() const noexcept {
        return result_;
    }

    void set_displayed_seconds(const i32 value) noexcept {
        result_.displayed_seconds = value;
    }

private:
    LegacyFramebuffer& framebuffer_;
    const LegacyRasterGeometryState& raster_;
    LegacyCountdownPieceProvider& provider_;
    const LegacyBlitEffectState& effects_;
    LegacyRleRowJitterState& jitter_;
    i32 destination_y_{};
    i32 destination_x_{};
    LegacyCountdownDisplayResult result_{};
};

}  // namespace

void initialize_legacy_countdown(
    LegacyCountdownState& state,
    LegacyCountdownFlagPorts& flags,
    const LegacyCountdownInitializationRequest& request
) noexcept {
    const u32 ticks = countdown_ticks(request.minutes, request.seconds);
    if (request.mode == 0) {
        state.primary_value_004c97e8 = 0U;
        state.primary_value_004c97ec = 0U;
        state.primary_ticks = ticks;
        state.primary_transition_value = request.primary_transition_value;
        flags.set_internal_flag(kLegacyPrimaryCountdownFlag);
        flags.set_internal_flag(kLegacyPrimaryCountdownCompanionFlag);
        return;
    }

    state.secondary_value_004bab78 = 0U;
    state.secondary_value_004bab7c = 0U;
    state.secondary_ticks = ticks;
    flags.set_internal_flag(kLegacySecondaryCountdownFlag);
}

LegacyCountdownDisplayResult draw_legacy_countdown(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyCountdownState& state,
    LegacyCountdownFlagPorts& flags,
    LegacyCountdownPieceProvider& provider,
    const LegacyCountdownDisplayRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept {
    if (request.mode == 0) {
        if (!flags.query_internal_flag(kLegacyPrimaryCountdownFlag)) {
            LegacyCountdownDisplayResult result;
            result.status = LegacyCountdownDisplayStatus::hidden_inactive;
            return result;
        }
    } else if (request.mode == 1 &&
               !flags.query_internal_flag(kLegacySecondaryCountdownFlag)) {
        LegacyCountdownDisplayResult result;
        result.status = LegacyCountdownDisplayStatus::hidden_inactive;
        return result;
    }

    if (flags.query_internal_flag(kLegacyCountdownSuppressionFlag)) {
        LegacyCountdownDisplayResult result;
        result.status = LegacyCountdownDisplayStatus::hidden_suppressed;
        return result;
    }

    const u32 raw_ticks = request.mode == 0
        ? state.primary_ticks
        : state.secondary_ticks;
    i32 displayed_seconds = from_bits(raw_ticks) / 30;
    if (displayed_seconds < 0) {
        displayed_seconds = 0;
    }

    CountdownDrawer drawer{
        framebuffer,
        raster,
        provider,
        effects,
        jitter,
        request.destination_y,
        request.destination_x,
    };
    drawer.set_displayed_seconds(displayed_seconds);

    const i32 leading_minutes_digit = displayed_seconds / 600;
    if (leading_minutes_digit != 0 &&
        !drawer.draw(leading_minutes_digit)) {
        return drawer.result();
    }
    if (!drawer.draw((displayed_seconds / 60) % 10) ||
        !drawer.draw(10) ||
        !drawer.draw((displayed_seconds % 60) / 10) ||
        !drawer.draw((displayed_seconds % 60) % 10)) {
        return drawer.result();
    }
    return drawer.result();
}

}  // namespace openswd3::rendering
