#pragma once

#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

namespace openswd3::battle {

struct LegacyBattleActorActionCommitBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleActorActionCommitRequest {
    compat::u32 actor_code{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorActionCommitStatus : compat::u8 {
    completed,
    actor_order_typed_stop,
    group_a_actor_typed_stop,
};

struct LegacyBattleActorActionCommitResult {
    LegacyBattleActorActionCommitStatus status{
        LegacyBattleActorActionCommitStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 actor_order_reads{};
    compat::u32 actor_query_calls{};
    compat::u32 port_calls{};
    bool actor_swapped{};
};

// Typed closure of legacy 0x00462420.
[[nodiscard]] LegacyBattleActorActionCommitResult
commit_legacy_battle_actor_action(
    LegacyBattleActorActionCommitBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionCommitRequest& request
);

}  // namespace openswd3::battle
