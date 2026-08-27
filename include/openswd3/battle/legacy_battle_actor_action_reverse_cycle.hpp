#pragma once

#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

namespace openswd3::battle {

struct LegacyBattleActorActionReverseCycleBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleActorActionReverseCycleRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorActionReverseCycleStatus : compat::u8 {
    completed,
    action_commit_typed_stop,
};

struct LegacyBattleActorActionReverseCycleResult {
    LegacyBattleActorActionReverseCycleStatus status{
        LegacyBattleActorActionReverseCycleStatus::completed
    };
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
