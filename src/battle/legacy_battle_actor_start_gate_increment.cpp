#include "openswd3/battle/legacy_battle_actor_start_gate_increment.hpp"

namespace openswd3::battle {

LegacyBattleActorStartGateIncrementResult
increment_legacy_battle_actor_start_gate(
    compat::u16* const start_gate,
    compat::u32* const start_gate_latch,
    const LegacyBattleActorStartGateIncrementAccess access
) noexcept {
    LegacyBattleActorStartGateIncrementResult result;
    if (start_gate == nullptr || !access.start_gate_readable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            start_gate_read_typed_stop;
        return result;
    }

    result.previous_start_gate = *start_gate;
    if (!access.start_gate_writable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            start_gate_write_typed_stop;
        return result;
    }

    *start_gate = static_cast<compat::u16>(*result.previous_start_gate + 1U);
    result.incremented_start_gate = *start_gate;
    if (start_gate_latch == nullptr || !access.start_gate_latch_writable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            start_gate_latch_write_typed_stop;
        return result;
    }

    *start_gate_latch = 1U;
    result.start_gate_latch_committed = true;
    if (!access.return_address_readable) {
        result.status = LegacyBattleActorStartGateIncrementStatus::
            return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
