#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_post_action.hpp"

#include <array>

namespace openswd3::battle {

using LegacyBattleGroupAActorRuntime = LegacyBattleActorProgressState;

struct LegacyBattleGroupAFrameState {
    LegacyBattleActionDispatchState action{};

    compat::u32 global_effect_override{};
    compat::u32 ai_coordination_enabled{};
    compat::u32 actor_gate_argument{};
    compat::i32 actor_progress_threshold{};
    std::array<compat::u32, 10> actor_enabled{};
    std::array<compat::u32, 10> actor_ai_primary{};
    std::array<compat::u32, 10> actor_ai_secondary{};
    std::array<LegacyBattleGroupAActorRuntime, 10> actors{};

    compat::u32 selected_opponent_one_based{1U};
    compat::u32 selected_actor_one_based{1U};
    compat::u32 selection_mode{};
    compat::u32 selection_aux_gate{};
    compat::u32 target_ready_gate{};
    compat::u32 target_cleanup_gate{};
    compat::u32 ui_gate_a{};
    compat::u32 ui_gate_b{};
    compat::u32 ui_gate_c{};
    compat::u32 ui_gate_d{};

    compat::u32 action_side{};
    compat::u32 action_block_gate{};
    compat::u32 action_aux_gate{};
    compat::u32 action_runtime_word{};
    compat::u16 action_stage_word{};
    compat::u16 action_stage_word_b{};
    std::array<compat::u32, 5> action_text_runtime{};
    std::array<compat::u8, 10> actor_text_present{};
    compat::u32 actor_text_token{0x00505000U};

    std::array<compat::u32, 5> active_effect_tail{};
    compat::u32 battle_byte_flags{};
    compat::u16 cleanup_word{};
    compat::u32 global_phase_countdown{};
    compat::u32 shared_gate_4ff578{};
    compat::u32 shared_gate_4ff57c{};
    compat::u32 shared_gate_4ff580{};
    compat::u32 shared_gate_4ff584{};
    compat::u32 shared_value_52544c{};
    compat::u32 shared_value_525450{};
    compat::u32 shared_value_525454{};
    compat::u32 shared_value_525458{};

    compat::u16 turn_resolution_bits{};
    compat::u16 actor_start_guard_word{};
    compat::u16 action_target_guard_high_word{};
    compat::u16 excluded_actor_count{};
    compat::u32 defeated_actor_packed{};
    compat::u32 message_suppressed{};
    compat::u32 sample_handle_value{};
    LegacyBattleFinalActorStepState final_actor_step{};
    LegacyBattlePostActionState post_action{};
    compat::u16 queued_selection_word{0xFFFFU};
    compat::u16 final_selected_word{0xFFFFU};
};

// sub_4714B0.
[[nodiscard]] LegacyBattleTurnCommitChanceResult
evaluate_legacy_battle_turn_commit_chance(
    const LegacyBattleGroupAConfigurationState* first_actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleTurnCommitChanceRequest& request
);

// sub_471540.
[[nodiscard]] LegacyBattleTurnAdvanceResult advance_legacy_battle_turn_gate(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActorProgressState* progress,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTurnAdvanceRequest& request
);

// Typed closure of legacy 0x00456680. One call advances the selected group-A
// actor and always returns one unless a typed boundary stops at the original
// first access.
[[nodiscard]] LegacyBattleActionDispatchResult
advance_legacy_battle_group_a_frame(
    LegacyBattleGroupAFrameState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    compat::u32 group_a_index
);

}  // namespace openswd3::battle
