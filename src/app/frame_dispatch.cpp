#include "openswd3/app/frame_dispatch.hpp"

namespace openswd3::app {

IdleAction select_idle_action(const IdleState& state) noexcept {
    if (state.frame_execution_gate != 0U) {
        if ((state.process_flags & kProcessVideoActive) != 0U) {
            return IdleAction::step_video_then_audio;
        }
        if ((state.process_flags & kProcessIdleSuppression) != 0U) {
            return IdleAction::yield;
        }
        if (state.transition_suppression != 0U) {
            return IdleAction::yield;
        }
        return IdleAction::step_game_frame;
    }

    if (state.display_active == 1U) {
        return IdleAction::present_pause;
    }
    return IdleAction::yield;
}

FrameEntryAction select_frame_entry_action(const FrameEntryState& state) noexcept {
    if ((state.process_flags & kProcessFrameEntrySuppression) != 0U) {
        return FrameEntryAction::return_immediately;
    }
    if (state.display_active == 0U) {
        return FrameEntryAction::yield;
    }
    return FrameEntryAction::sample_time;
}

ActiveFrameBranch select_active_frame_branch(
    const ActiveFrameState& state
) noexcept {
    if (state.high_priority_state != 0U) {
        return ActiveFrameBranch::high_priority;
    }
    if (state.battle_active != 0U) {
        return ActiveFrameBranch::battle;
    }
    if (state.special_mode_state == 0U) {
        return ActiveFrameBranch::world;
    }
    return ActiveFrameBranch::special_mode;
}

bool should_step_story(const StoryGateState& state) noexcept {
    return state.frame_execution_gate != 0U &&
           state.transition_suppression == 0U &&
           state.special_mode_state == 0U && state.battle_active == 0U &&
           state.high_priority_state == 0U;
}

SpecialModeHandler select_special_mode_handler(
    const compat::u32 tagged_mode_value
) noexcept {
    const compat::u32 mode = tagged_mode_value & kSpecialModeValueMask;
    if (mode == 2U) {
        return SpecialModeHandler::shop_mode_2;
    }
    if (mode == 1U || (mode >= 3U && mode <= 6U)) {
        return SpecialModeHandler::standard_modes_1_3_4_5_6;
    }
    return SpecialModeHandler::none;
}

BattleExitAction select_battle_exit_action(const compat::i32 result) noexcept {
    switch (result) {
        case 0:
            return BattleExitAction::restore_world_for_result_0;
        case 2:
            return BattleExitAction::request_special_mode_4_for_result_2;
        case 3:
            return BattleExitAction::remap_world_for_result_3;
        default:
            return BattleExitAction::keep_battle_active_and_return;
    }
}

bool should_request_close(const compat::u32 process_flags) noexcept {
    return (process_flags & kProcessCloseRequested) != 0U;
}

}  // namespace openswd3::app
