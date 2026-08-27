#include "openswd3/battle/legacy_battle_available_actor_cycle.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

}  // namespace

LegacyBattleAvailableActorCycleResult cycle_legacy_battle_available_actor(
    const LegacyBattleAvailableActorCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleAvailableActorCycleRequest& request
) {
    LegacyBattleAvailableActorCycleResult result;
    u32 eax = 0x004A7960U;
    u32 ecx = request.starting_actor_code;
    u32 edx = request.entry_edx;
    u32 candidate_index = 0U;
    u32 failed_count = 0U;

    while (candidate_index < 4U &&
           kLegacyBattleActorCyclePhysicalCandidates[candidate_index] != ecx) {
        eax += 4U;
        ++candidate_index;
    }

    const auto query_candidate = [&](const u32 candidate) {
        result.candidate_codes[result.candidate_calls] = candidate;
        ++result.candidate_calls;
        const auto queried =
            query_legacy_battle_actor_action_candidate_availability(
                {
                    .final_actor = bindings.final_actor,
                    .metrics = bindings.metrics,
                },
                port,
                {
                    .actor_code = candidate,
                    .entry_eax = eax,
                    .entry_ecx = ecx,
                    .entry_edx = edx,
                }
            );
        result.candidate_availability = queried;
        result.port_calls += queried.port_calls;
        eax = queried.return_eax;
        ecx = queried.return_ecx;
        edx = queried.return_edx;
        if (queried.status !=
            LegacyBattleActorActionCandidateAvailabilityStatus::completed) {
            result.status = LegacyBattleAvailableActorCycleStatus::
                candidate_availability_typed_stop;
            return false;
        }
        return true;
    };

    u32 candidate = kLegacyBattleActorCyclePhysicalCandidates[candidate_index];
    ++candidate_index;
    if (!query_candidate(candidate)) {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }
    if (eax == 1U) {
        result.return_eax = candidate;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    while (true) {
        if (candidate_index == 4U) {
            candidate_index = 0U;
        }
        ++failed_count;
        if (failed_count >= 4U) {
            result.return_eax = 0U;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        candidate = kLegacyBattleActorCyclePhysicalCandidates[candidate_index];
        ++candidate_index;
        if (!query_candidate(candidate)) {
            result.return_eax = eax;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
        if (eax == 1U) {
            result.return_eax = candidate;
            result.return_ecx = ecx;
            result.return_edx = edx;
            return result;
        }
    }
}

}  // namespace openswd3::battle
