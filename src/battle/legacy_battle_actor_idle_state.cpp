#include "openswd3/battle/legacy_battle_actor_idle_state.hpp"

namespace openswd3::battle {

LegacyBattleActorIdleStateResult query_legacy_battle_actor_idle_state(
    const compat::u32* idle_state, const LegacyBattleActorIdleStateAccess access
) noexcept {
    LegacyBattleActorIdleStateResult result;
    if (idle_state == nullptr || !access.latch_readable) {
        result.status = LegacyBattleActorIdleStateStatus::latch_read_typed_stop;
        return result;
    }

    result.value = *idle_state;
    if (!access.return_address_readable) {
        result.status =
            LegacyBattleActorIdleStateStatus::return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
