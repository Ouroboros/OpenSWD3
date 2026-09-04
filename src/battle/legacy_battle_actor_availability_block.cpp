#include "openswd3/battle/legacy_battle_actor_availability_block.hpp"

namespace openswd3::battle {

LegacyBattleActorAvailabilityBlockResult
set_legacy_battle_actor_availability_block(
    LegacyBattleActorAvailabilityBlockState* const actor,
    const LegacyBattleActorAvailabilityBlockRequest& request
) noexcept {
    LegacyBattleActorAvailabilityBlockResult result{
        .return_eax = request.value,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };

    if (actor == nullptr || !actor->write_accessible) {
        result.status =
            LegacyBattleActorAvailabilityBlockStatus::actor_write_typed_stop;
        return result;
    }

    actor->value = result.return_eax;
    result.actor_writes = 1U;
    return result;
}

}  // namespace openswd3::battle
