#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::app {

inline constexpr compat::u32 kProcessIdleSuppression = 0x01U;
inline constexpr compat::u32 kProcessCloseRequested = 0x04U;
inline constexpr compat::u32 kProcessFrameEntrySuppression = 0x10U;
inline constexpr compat::u32 kProcessVideoActive = 0x20U;
inline constexpr compat::u32 kSpecialModeValueMask = 0x0FFFFFFFU;

struct IdleState {
    compat::u32 frame_execution_gate{};
    compat::u32 process_flags{};
    compat::u32 transition_suppression{};
    compat::u32 display_active{};
};

enum class IdleAction {
    step_video_then_audio,
    yield,
    step_game_frame,
    present_pause,
};

[[nodiscard]] IdleAction select_idle_action(const IdleState& state) noexcept;

struct FrameEntryState {
    compat::u32 process_flags{};
    compat::u32 display_active{};
};

enum class FrameEntryAction {
    return_immediately,
    yield,
    sample_time,
};

[[nodiscard]] FrameEntryAction
select_frame_entry_action(const FrameEntryState& state) noexcept;

struct ActiveFrameState {
    compat::u32 high_priority_state{};
    compat::u32 battle_active{};
    compat::u32 special_mode_state{};
};

enum class ActiveFrameBranch {
    high_priority,
    battle,
    world,
    special_mode,
};

[[nodiscard]] ActiveFrameBranch
select_active_frame_branch(const ActiveFrameState& state) noexcept;

struct StoryGateState {
    compat::u32 frame_execution_gate{};
    compat::u32 transition_suppression{};
    compat::u32 special_mode_state{};
    compat::u32 battle_active{};
    compat::u32 high_priority_state{};
};

[[nodiscard]] bool should_step_story(const StoryGateState& state) noexcept;

enum class SpecialModeHandler {
    none,
    standard_modes_1_3_4_5_6,
    shop_mode_2,
};

[[nodiscard]] SpecialModeHandler
select_special_mode_handler(compat::u32 tagged_mode_value) noexcept;

enum class BattleExitAction {
    restore_world_for_result_0,
    request_special_mode_4_for_result_2,
    remap_world_for_result_3,
    keep_battle_active_and_return,
};

[[nodiscard]] BattleExitAction
select_battle_exit_action(compat::i32 result) noexcept;

[[nodiscard]] bool should_request_close(compat::u32 process_flags) noexcept;

}  // namespace openswd3::app
