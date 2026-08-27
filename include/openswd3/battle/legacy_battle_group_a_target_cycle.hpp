#pragma once

#include "openswd3/battle/legacy_battle_available_actor_cycle.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"

namespace openswd3::battle {

struct LegacyBattleGroupATargetCycleBindings {
    LegacyBattleFrameInputResolutionState& frame_input;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleTargetSelectionRuntimeState& target_runtime;
    const compat::u16& supplemental_count_word;
};

struct LegacyBattleGroupATargetCycleRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupATargetCycleStatus : compat::u8 {
    completed,
    target_order_typed_stop,
};

struct LegacyBattleGroupATargetCycleResult {
    LegacyBattleGroupATargetCycleStatus status{
        LegacyBattleGroupATargetCycleStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 target_order_reads{};
    compat::u32 loop_iterations{};
};

// Typed closure of legacy 0x00465170. Cycles the physically adjacent
// group-A target order until it matches the queued actor's zero-based index.
[[nodiscard]] LegacyBattleGroupATargetCycleResult
cycle_legacy_battle_group_a_target(
    LegacyBattleGroupATargetCycleBindings bindings,
    const LegacyBattleGroupATargetCycleRequest& request
);

}  // namespace openswd3::battle
