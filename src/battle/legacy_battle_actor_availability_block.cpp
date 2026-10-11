#include "openswd3/battle/legacy_battle_actor_availability_block.hpp"

namespace openswd3::battle {

LegacyBattleActorAvailabilityBlockResult
set_legacy_battle_actor_availability_block(
    LegacyBattleActorAvailabilityBlockState* const actor,
    const compat::u32 value
) noexcept {
    if (actor == nullptr || !actor->write_accessible) {
        return {
            LegacyBattleActorAvailabilityBlockStatus::actor_write_typed_stop
        };
    }

    actor->value = value;
    return {};
}

}  // namespace openswd3::battle
