#include "openswd3/battle/legacy_battle_actor_start_gate_latch_query.hpp"

namespace openswd3::battle {

LegacyBattleActorStartGateLatchQueryResult
query_legacy_battle_actor_start_gate_latch(
    const compat::u32* const start_gate_latch,
    const LegacyBattleActorStartGateLatchQueryAccess access
) noexcept {
    LegacyBattleActorStartGateLatchQueryResult result;
    if (start_gate_latch == nullptr || !access.start_gate_latch_readable) {
        result.status = LegacyBattleActorStartGateLatchQueryStatus::
            start_gate_latch_read_typed_stop;
        return result;
    }

    result.value = *start_gate_latch;
    if (!access.return_address_readable) {
        result.status = LegacyBattleActorStartGateLatchQueryStatus::
            return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
