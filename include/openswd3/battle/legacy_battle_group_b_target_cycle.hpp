#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupBTargetCycleBindings {
    LegacyBattleFrameInputResolutionState& frame_input;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleTargetSelectionRuntimeState& target_runtime;
    compat::u32& message_state;
};

struct LegacyBattleGroupBTargetCycleRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupBTargetCycleStatus : compat::u8 {
    completed,
    target_order_typed_stop,
    group_b_actor_typed_stop,
};

struct LegacyBattleGroupBTargetCycleResult {
    LegacyBattleGroupBTargetCycleStatus status{
        LegacyBattleGroupBTargetCycleStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 completion_queries{};
    compat::u32 reset_calls{};
    compat::u32 target_order_reads{};
    compat::u32 skipped_targets{};
};

// Typed closure of legacy 0x00465090.
[[nodiscard]] LegacyBattleGroupBTargetCycleResult
cycle_legacy_battle_group_b_target(
    LegacyBattleGroupBTargetCycleBindings bindings,
    LegacyBattleTargetSelectionRuntimePort& port,
    const LegacyBattleGroupBTargetCycleRequest& request
);

}  // namespace openswd3::battle
