#include "openswd3/battle/legacy_battle_actor_action_reverse_cycle.hpp"

#include <array>

#include "openswd3/battle/legacy_battle_actor_action_commit.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

inline constexpr std::array<u32, 4> kReverseCycleStarts{9U, 10U, 11U, 8U};

}  // namespace

LegacyBattleActorActionReverseCycleResult
reverse_cycle_legacy_battle_actor_action(
    const LegacyBattleActorActionReverseCycleBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionReverseCycleRequest& request
) {
    LegacyBattleActorActionReverseCycleResult result;
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

    const auto resolve = port.invoke_input_dispatch({
        .call = LegacyBattleInputDispatchCall::
            actor_action_resolve_available_reverse,
        .arguments = {kReverseCycleStarts[eax]},
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.port_calls;
    ++result.resolve_calls;
    eax = resolve.eax;
    ecx = resolve.ecx;
    edx = resolve.edx;

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
            LegacyBattleActorActionReverseCycleStatus::action_commit_typed_stop;
    }
    return result;
}

}  // namespace openswd3::battle
