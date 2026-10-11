#include "openswd3/battle/legacy_battle_actor_start_gate.hpp"

namespace openswd3::battle {

LegacyBattleActorStartGateResult query_legacy_battle_actor_start_gate(
    const compat::u16* start_gate, const LegacyBattleActorStartGateAccess access
) noexcept {
    LegacyBattleActorStartGateResult result;
    if (start_gate == nullptr || !access.start_gate_readable) {
        result.status =
            LegacyBattleActorStartGateStatus::start_gate_read_typed_stop;
        return result;
    }

    result.value = *start_gate;
    if (!access.return_address_readable) {
        result.status =
            LegacyBattleActorStartGateStatus::return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
