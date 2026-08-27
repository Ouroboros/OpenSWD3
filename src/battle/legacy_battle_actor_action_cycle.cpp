#include "openswd3/battle/legacy_battle_actor_action_cycle.hpp"

#include <array>

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

    const auto resolve = port.invoke_input_dispatch({
        .call = LegacyBattleInputDispatchCall::actor_action_resolve_available,
        .arguments = {kActionCycleStarts[eax]},
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.port_calls;
    ++result.resolve_calls;
    eax = resolve.eax;
    ecx = resolve.ecx;
    edx = resolve.edx;

    const auto commit = port.invoke_input_dispatch({
        .call = LegacyBattleInputDispatchCall::actor_action_commit_candidate,
        .arguments = {eax},
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
    });
    ++result.port_calls;
    ++result.commit_calls;
    result.return_eax = commit.eax;
    result.return_ecx = commit.ecx;
    result.return_edx = commit.edx;
    return result;
}

}  // namespace openswd3::battle
