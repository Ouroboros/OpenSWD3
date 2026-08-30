#pragma once

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorRetreatReadyRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorRetreatReadyStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleActorRetreatReadyResult {
    LegacyBattleActorRetreatReadyStatus status{
        LegacyBattleActorRetreatReadyStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_4728D0.
[[nodiscard]] LegacyBattleActorRetreatReadyResult
query_legacy_battle_actor_retreat_ready(
    const LegacyBattleGroupAActionExecutionState* actor,
    const LegacyBattleActorRetreatReadyRequest& request
) noexcept;

}  // namespace openswd3::battle
