#pragma once

#include "openswd3/battle/legacy_battle_available_actor_cycle.hpp"

namespace openswd3::battle {

// Typed closure of legacy 0x00464E40. Cycles the shared physical candidate
// table in reverse and returns the first available actor code.
[[nodiscard]] LegacyBattleAvailableActorCycleResult
reverse_cycle_legacy_battle_available_actor(
    LegacyBattleAvailableActorCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleAvailableActorCycleRequest& request
);

}  // namespace openswd3::battle
