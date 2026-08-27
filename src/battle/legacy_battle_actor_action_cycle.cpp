#include "openswd3/battle/legacy_battle_actor_action_cycle.hpp"

#include <array>

#include "openswd3/battle/legacy_battle_actor_action_commit.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr std::array<u32, 4> kActionCycleStarts{11U, 8U, 9U, 10U};

}  // namespace

LegacyBattleActorActionCycleResult cycle_legacy_battle_actor_action(
    const LegacyBattleActorActionCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionCycleRequest& request
) {
    LegacyBattleActorActionCycleResult result;
    u32 eax = bindings.final_actor.queued_actor_code;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    bindings.final_actor.pre_frame_gate_b = 0U;
    eax += 0xFFFFFFF8U;
    if (eax > 3U) {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    result.available_actor_cycle = cycle_legacy_battle_available_actor(
        {
            .final_actor = bindings.final_actor,
            .metrics = bindings.metrics,
        },
        port,
        {
            .starting_actor_code = kActionCycleStarts[eax],
            .entry_eax = eax,
            .entry_ecx = ecx,
            .entry_edx = edx,
        }
    );
    ++result.resolve_calls;
    result.port_calls += result.available_actor_cycle.port_calls;
    eax = result.available_actor_cycle.return_eax;
    ecx = result.available_actor_cycle.return_ecx;
    edx = result.available_actor_cycle.return_edx;
    if (result.available_actor_cycle.status !=
        LegacyBattleAvailableActorCycleStatus::completed) {
        result.status = LegacyBattleActorActionCycleStatus::
            available_actor_cycle_typed_stop;
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    }

    const auto commit = commit_legacy_battle_actor_action(
        {
            .startup_reset = bindings.startup_reset,
            .final_actor = bindings.final_actor,
            .metrics = bindings.metrics,
            .input_dispatch = bindings.input_dispatch,
            .message_state = bindings.message_state,
        },
        port,
        {.actor_code = eax,
         .entry_eax = eax,
         .entry_ecx = ecx,
         .entry_edx = edx}
    );
    result.port_calls += commit.port_calls;
    ++result.commit_calls;
    result.return_eax = commit.return_eax;
    result.return_ecx = commit.return_ecx;
    result.return_edx = commit.return_edx;
    if (commit.status != LegacyBattleActorActionCommitStatus::completed) {
        result.status =
            LegacyBattleActorActionCycleStatus::action_commit_typed_stop;
    }
    return result;
}

}  // namespace openswd3::battle
