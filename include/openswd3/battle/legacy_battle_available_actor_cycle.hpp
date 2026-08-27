#pragma once

#include "openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr std::array<compat::u32, 8>
    kLegacyBattleActorCyclePhysicalCandidates{
        10U,
        9U,
        8U,
        11U,
        2U,
        1U,
        0U,
        3U,
    };

struct LegacyBattleAvailableActorCycleBindings {
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
};

struct LegacyBattleAvailableActorCycleRequest {
    compat::u32 starting_actor_code{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleAvailableActorCycleStatus : compat::u8 {
    completed,
    candidate_availability_typed_stop,
};

struct LegacyBattleAvailableActorCycleResult {
    LegacyBattleAvailableActorCycleStatus status{
        LegacyBattleAvailableActorCycleStatus::completed
    };
    LegacyBattleActorActionCandidateAvailabilityResult candidate_availability{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    std::array<compat::u32, 4> candidate_codes{};
    compat::u32 candidate_calls{};
    compat::u32 port_calls{};
};

// Typed closure of legacy 0x00464DD0. Cycles the physical candidate table
// from the requested actor code and returns the first available code.
[[nodiscard]] LegacyBattleAvailableActorCycleResult
cycle_legacy_battle_available_actor(
    LegacyBattleAvailableActorCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleAvailableActorCycleRequest& request
);

}  // namespace openswd3::battle
