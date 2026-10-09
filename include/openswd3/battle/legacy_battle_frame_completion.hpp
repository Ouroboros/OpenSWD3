#pragma once

#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_outcome_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleFrameCompletionBindings {
    LegacyBattleActorMetricState& actors;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleOutcomeResolutionState& outcome;
    LegacyBattleStartupState& startup;
    compat::u32& message_state;
    std::span<const LegacyBattleActorFrameLinkedNode> action_nodes;
};

enum class LegacyBattleFrameCompletionStatus : compat::u8 {
    completed,
    group_a_fields_typed_stop,
    group_b_fields_typed_stop,
    action_node_typed_stop,
};

struct LegacyBattleFrameCompletionResult {
    LegacyBattleFrameCompletionStatus status{
        LegacyBattleFrameCompletionStatus::completed
    };
    compat::u32 stopped_index{};
    compat::u32 missing_action_node{};
    compat::u8 group_a_ready_count{};
    compat::u8 group_b_ready_count{};
    bool group_a_committed{};
    bool group_b_committed{};
};

[[nodiscard]] LegacyBattleFrameCompletionResult
update_legacy_battle_frame_completion(
    LegacyBattleFrameCompletionBindings bindings
);

}  // namespace openswd3::battle
