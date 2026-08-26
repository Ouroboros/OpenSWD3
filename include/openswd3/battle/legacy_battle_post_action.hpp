#pragma once

#include "openswd3/battle/legacy_battle_final_actor_step.hpp"

#include <array>

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
    compat::u32 source_group_a_index,
    compat::u32 target_group_b_index
);

}  // namespace openswd3::battle
