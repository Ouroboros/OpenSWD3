#pragma once

#include "openswd3/battle/legacy_battle_actor_gate_decay.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattlePostActionState {
    std::array<compat::u32, 0x7E> selection_workspace{};
    compat::u32 published_target_token{};
    compat::u32 selection_rebuild_pending{};
};

// Typed closure of legacy 0x0045ADF0. One call scans every group-A actor other
// than the source and either rebuilds one target relation or clears the fixed
// battle selection workspaces.
[[nodiscard]] LegacyBattleActionDispatchResult
advance_legacy_battle_post_action(
    LegacyBattlePostActionState& state,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleBoundedRandomPort& random,
    LegacyBattleStartupState* startup,
    const LegacyBattleActorRuntimeResetCallRequests& runtime_reset_requests,
    compat::u32 source_group_a_index,
    compat::u32 target_group_b_index,
    const LegacyBattleActorActionTargetRequest& action_target_request = {},
    const LegacyBattleActorActionModeRequest& action_mode_request = {},
    std::size_t runtime_reset_request_offset = 0U,
    const LegacyBattleActorTargetSelectionRequestList&
        target_selection_requests = {},
    std::size_t target_selection_request_offset = 0U,
    const LegacyBattleActorGateDecayCallRequests& gate_decay_requests = {},
    std::size_t gate_decay_request_offset = 0U
);

}  // namespace openswd3::battle
