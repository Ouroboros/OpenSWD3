#include "openswd3/battle/legacy_battle_actor_retreat_ready.hpp"

namespace openswd3::battle {

LegacyBattleActorRetreatReadyResult query_legacy_battle_actor_retreat_ready(
    const LegacyBattleGroupAActionExecutionState* actor,
    const LegacyBattleActorRetreatReadyRequest& request
) noexcept {
    LegacyBattleActorRetreatReadyResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleActorRetreatReadyStatus::actor_state_typed_stop;
        return result;
    }

    result.return_eax = (result.return_eax & 0xFFFF0000U) |
        static_cast<compat::u32>(actor->retreat_ready_flags);
    result.return_eax = (~result.return_eax >> 11U) & 1U;
    return result;
}

}  // namespace openswd3::battle
