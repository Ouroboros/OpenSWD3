#pragma once

#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

namespace openswd3::battle {

struct LegacyBattleActorActionCandidateAvailabilityBindings {
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
};

struct LegacyBattleActorActionCandidateAvailabilityRequest {
    compat::u32 actor_code{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActorActionCandidateAvailabilityStatus : compat::u8 {
    completed,
    actor_order_typed_stop,
    group_a_actor_typed_stop,
};

struct LegacyBattleActorActionCandidateAvailabilityResult {
    LegacyBattleActorActionCandidateAvailabilityStatus status{
        LegacyBattleActorActionCandidateAvailabilityStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 actor_order_reads{};
    compat::u32 actor_query_calls{};
    compat::u32 port_calls{};
};

// Typed closure of legacy 0x004624C0.
[[nodiscard]] LegacyBattleActorActionCandidateAvailabilityResult
query_legacy_battle_actor_action_candidate_availability(
    LegacyBattleActorActionCandidateAvailabilityBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionCandidateAvailabilityRequest& request
);

}  // namespace openswd3::battle
