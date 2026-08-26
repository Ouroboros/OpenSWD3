#include "openswd3/battle/legacy_battle_status_indicator.hpp"

#include <bit>
#include <cstring>

namespace openswd3::battle {
namespace {

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

[[nodiscard]] compat::i32 from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

}  // namespace

LegacyBattleStatusIndicatorResult advance_legacy_battle_status_indicator(
    LegacyBattleStatusIndicatorState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleIndicatorSoundPort& sound,
    const compat::u32 action_update_eax_snapshot
) {
    LegacyBattleStatusIndicatorResult result;
    if (state.tick_counter == 0U) {
        if (state.completed_hold_count >= 1U) {
            state.side_state = 0U;
            state.completed_hold_count = 0U;
        } else {
            state.side_state =
                static_cast<compat::u16>(random.random_bounded(2U));
            result.random_calls = 1U;
        }
    }

    state.action_record.action_id = 0x2329U;
    state.action_record.base_variant = state.side_state == 1U ? 3U : 2U;
    state.action_update_attempted = true;
    state.action_update = action_updater.update(state.action_record);
    result.action_update_calls = 1U;
    if (state.action_update.return_value == 0U) {
        result.status = LegacyBattleStatusIndicatorStatus::action_update_failed;
        return result;
    }

    state.frame_resource_id = (action_update_eax_snapshot & 0xFFFF0000U) |
        static_cast<compat::u32>(state.action_record.field_4a);
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(state.frame_resource_id, 0U, piece);
    result.frame_load_calls = 1U;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleStatusIndicatorStatus::frame_unavailable;
        return result;
    }
    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;

    const compat::u32 side = static_cast<compat::u32>(state.side_state);
    const compat::u32 side_times_fifty = side * 5U * 5U * 2U;
    result.draw_x = from_bits(side_times_fifty + 0x104U);
    result.draw_y = 200;
    result.request_flags = state.action_record.mode_flags;
    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = result.draw_x;
    request.destination_y = result.draw_y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = result.request_flags;
    request.auxiliary = {};
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleStatusIndicatorStatus::blit_typed_stop;
        return result;
    }
    publish_blitter_normal_epilogue(shared_request, shared_effects);

    state.tick_counter += 1U;
    const compat::i32 signed_tick = from_bits(state.tick_counter);
    result.tick_multiple_of_25 = signed_tick % 25 == 0;
    compat::u16 intensity_for_compare = state.intensity;
    if (result.tick_multiple_of_25) {
        const compat::u32 packed_intensity =
            static_cast<compat::u32>(state.intensity) |
            (static_cast<compat::u32>(state.intensity_countdown) << 16U);
        const compat::u16 doubled =
            static_cast<compat::u16>(packed_intensity * 2U);
        state.intensity = doubled;
        state.intensity_countdown = doubled;
        intensity_for_compare = doubled;
    }

    if (intensity_for_compare >= 0x40U) {
        state.intensity = 1U;
        state.intensity_countdown = 1U;
        state.tick_counter = 0U;
        if (state.side_state == 1U) {
            state.completed_hold_count =
                static_cast<compat::u16>(state.completed_hold_count + 1U);
        }
        std::memset(
            &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
        );
        result.action_record_cleared = true;
        result.return_value = 1U;
        return result;
    }

    state.intensity_countdown =
        static_cast<compat::u16>(state.intensity_countdown - 1U);
    if (state.intensity_countdown != 0U) {
        return result;
    }

    state.intensity_countdown = intensity_for_compare;
    state.side_state = state.side_state == 0U ? 1U : 0U;
    result.state_toggled = true;
    sound.play_indicator_sound(0x2EU, 1U);
    result.sound_calls = 1U;
    return result;
}

}  // namespace openswd3::battle
