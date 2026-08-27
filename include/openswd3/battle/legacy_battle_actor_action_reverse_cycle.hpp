#pragma once

#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

namespace openswd3::battle {

struct LegacyBattleActorActionReverseCycleBindings {
    LegacyBattleFinalActorStepState& final_actor;
};

struct LegacyBattleActorActionReverseCycleRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleActorActionReverseCycleResult {
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 resolve_calls{};
    compat::u32 commit_calls{};
};

// Typed closure of legacy 0x004623A0.
[[nodiscard]] LegacyBattleActorActionReverseCycleResult
reverse_cycle_legacy_battle_actor_action(
    LegacyBattleActorActionReverseCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionReverseCycleRequest& request
);

}  // namespace openswd3::battle
