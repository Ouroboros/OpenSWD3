#pragma once

#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/compat/types.hpp"

#include <vector>

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
struct LegacyBattleActionDispatchState;

struct LegacyBattleGroupAActionExecutionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAActionExecutionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    slot_typed_stop,
    resource_typed_stop,
};

struct LegacyBattleGroupAActionExecutionResult {
    LegacyBattleGroupAActionExecutionStatus status{
        LegacyBattleGroupAActionExecutionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 record_clears{};
    compat::u32 color_calls{};
    compat::u32 draw_calls{};
    compat::u32 target_calls{};
    compat::u32 completion_writes{};
    std::vector<compat::u32> call_trace;
};

// sub_46F8C0.
[[nodiscard]] LegacyBattleGroupAActionExecutionResult
advance_legacy_battle_group_a_action_execution(
    LegacyBattleGroupAActionExecutionState* state,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    LegacyBattleActionDispatchState& dispatch,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAItemEffectApplicationState& item_effect,
    compat::u32 actor_token,
    compat::u32 target_token,
    compat::u32 slot_index,
    compat::u32 skip_primary,
    compat::u32 skip_secondary,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleGroupAActionExecutionRequest& request = {}
);

}  // namespace openswd3::battle
