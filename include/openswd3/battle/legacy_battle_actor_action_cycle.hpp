#pragma once

#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

namespace openswd3::battle {

struct LegacyBattleActorActionCycleBindings {
    LegacyBattleFinalActorStepState& final_actor;
};

struct LegacyBattleActorActionCycleRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorActionCycleResult {
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 resolve_calls{};
    compat::u32 commit_calls{};
};

// Typed closure of legacy 0x00462320.
[[nodiscard]] LegacyBattleActorActionCycleResult
cycle_legacy_battle_actor_action(
    LegacyBattleActorActionCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionCycleRequest& request
);

}  // namespace openswd3::battle
