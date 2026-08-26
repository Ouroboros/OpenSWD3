#include "openswd3/battle/legacy_battle_scale_scan.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u32 to_bits(const compat::i32 value) noexcept {
    return std::bit_cast<compat::u32>(value);
}

[[nodiscard]] compat::i32 from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

void publish_blitter_normal_epilogue(
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects
) noexcept {
    shared_request.target_height = 0;
    shared_request.horizontal_resample_displacement = 0;
    shared_request.vertical_resample_phase_10_10 = 0U;
    shared_request.opacity_step = 0;
    shared_effects.red_offset = 0;
    shared_effects.green_offset = 0;
    shared_effects.blue_offset = 0;
    shared_effects.skip_every_third_row = false;
}

void set_scan_clip(
    LegacyBattleScaleScanState& state,
    compat::i32 left,
    compat::i32 top,
    compat::i32 right,
    compat::i32 bottom
) noexcept {
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right > state.screen_width) {
        right = state.screen_width;
    }
    if (bottom > state.screen_height) {
        bottom = state.screen_height;
    }
    state.shared_clip = {
        .left = left,
        .top = top,
        .width = wrapping_subtract(right, left),
        .height = wrapping_subtract(bottom, top),
    };
}

[[nodiscard]] std::span<const compat::u8>
palette_bytes(const std::span<const compat::u16> palette) noexcept {
    if (palette.empty()) {
        return {};
    }
    return {
        reinterpret_cast<const compat::u8*>(palette.data()),
        palette.size_bytes(),
    };
}

[[nodiscard]] bool load_and_draw_scan_frame(
    LegacyBattleScaleScanState& state,
    LegacyBattleScaleScanResult& result,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 frame_index,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(0x234FU, frame_index, piece);
    ++result.frame_load_calls;
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleScaleScanStatus::frame_unavailable;
        return false;
    }
    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;

    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    request.auxiliary = palette_bytes(piece.source.palette);
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer,
        state.shared_clip,
        state.current_source,
        request,
        shared_effects,
        jitter
    );
    ++result.frame_draw_calls;
    result.last_blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleScaleScanStatus::blit_typed_stop;
        return false;
    }
    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return true;
}

}  // namespace

LegacyBattleScaleScanResult draw_legacy_battle_scale_scan(
    LegacyBattleScaleScanState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    LegacyBattleScaleScanResult result;
    if (!load_and_draw_scan_frame(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            frame_provider,
            0U,
            x,
            y
        )) {
        return result;
    }
    state.selection_marker = 0U;

    for (compat::u32 index = 0U; index < state.thresholds.size(); ++index) {
        const compat::i32 threshold =
            static_cast<compat::i32>(state.thresholds[index]);
        const compat::i32 clip_left = wrapping_add(x, threshold);
        set_scan_clip(
            state,
            clip_left,
            y,
            wrapping_add(clip_left, 1),
            wrapping_add(
                y, static_cast<compat::i32>(state.current_frame.height)
            )
        );
        ++result.clip_set_calls;
        if (!load_and_draw_scan_frame(
                state,
                result,
                framebuffer,
                shared_request,
                shared_effects,
                jitter,
                frame_provider,
                1U,
                x,
                y
            )) {
            return result;
        }
        ++result.threshold_iterations;

        const compat::u32 half_step =
            (static_cast<compat::u32>(
                 static_cast<compat::u16>(state.scan_counter)
             ) >>
             1U) +
            1U;
        if (half_step < static_cast<compat::u32>(state.thresholds[index]) ||
            half_step >
                static_cast<compat::u32>(state.thresholds[index]) + 2U) {
            continue;
        }

        const compat::u16 candidate = static_cast<compat::u16>(index + 1U);
        state.selection_marker = candidate;
        ++result.selection_hits;
        if (static_cast<compat::u16>(state.target_selection) == candidate) {
            state.selection_marker = 0U;
        } else {
            state.target_selection &= 0xFFFF0000U;
        }
    }

    const compat::u32 scan_half =
        static_cast<compat::u32>(
            static_cast<compat::u16>(state.scan_counter)
        ) >>
        1U;
    const compat::i32 scan_x =
        wrapping_add(x, static_cast<compat::i32>(scan_half));
    result.final_scan_x = wrapping_add(scan_x, 1);
    set_scan_clip(
        state,
        result.final_scan_x,
        y,
        wrapping_add(scan_x, 2),
        wrapping_add(y, static_cast<compat::i32>(state.current_frame.height))
    );
    ++result.clip_set_calls;
    if (!load_and_draw_scan_frame(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            frame_provider,
            1U,
            x,
            y
        )) {
        return result;
    }

    set_scan_clip(state, 0, 0, 640, 480);
    ++result.clip_set_calls;
    const compat::u16 incremented =
        static_cast<compat::u16>(state.scan_counter + 1U);
    state.scan_counter = (state.scan_counter & 0xFFFF0000U) |
        static_cast<compat::u32>(incremented);
    const compat::u32 completed_half_step =
        (static_cast<compat::u32>(incremented) >> 1U) + 1U;
    if (completed_half_step == 62U) {
        state.scan_counter = (state.scan_counter & 0xFFFF0000U) | 0x8000U;
        result.return_value = 1U;
    }
    return result;
}

}  // namespace openswd3::battle
