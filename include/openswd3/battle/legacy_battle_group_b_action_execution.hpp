#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
struct LegacyBattleActionDispatchContext;
struct LegacyBattleActionDispatchState;
struct LegacyBattleActorGroupBElementState;
struct LegacyBattleGroupAActionExecutionSharedState;

enum class LegacyBattleGroupBActionExecutionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    action_resource_typed_stop,
    render_source_typed_stop,
};

struct LegacyBattleGroupBActionExecutionRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

struct LegacyBattleGroupBActionExecutionResult {
    LegacyBattleGroupBActionExecutionStatus status{
        LegacyBattleGroupBActionExecutionStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 actor_update_calls{};
    compat::u32 action_record_calls{};
    compat::u32 secondary_record_calls{};
    compat::u32 color_initialization_calls{};
    compat::u32 frame_refresh_calls{};
    compat::u32 action_record_clears{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4758A0.
[[nodiscard]] LegacyBattleGroupBActionExecutionResult
advance_legacy_battle_group_b_action_execution(
    LegacyBattleActorGroupBElementState* actor,
    LegacyBattleGroupAActionExecutionSharedState& shared,
    LegacyBattleActionDispatchState& dispatch,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleGroupBActionExecutionRequest& request
);

}  // namespace openswd3::battle
