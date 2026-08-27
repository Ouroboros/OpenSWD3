#include "openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

constexpr u32 kLegacyBattleActorOrderToken = 0x00520DD0U;

[[nodiscard]] constexpr bool valid_group_a_index(const u32 index) noexcept {
    return index < 10U;
}

}  // namespace

LegacyBattleActorActionCandidateAvailabilityResult
query_legacy_battle_actor_action_candidate_availability(
    const LegacyBattleActorActionCandidateAvailabilityBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionCandidateAvailabilityRequest& request
) {
    LegacyBattleActorActionCandidateAvailabilityResult result;
    u32 edx = bindings.metrics.group_a_count;
    u32 eax = 0U;
    u32 ecx = request.entry_ecx;
    u32 order_index = 0U;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };

    if (edx == 0U) {
        return finish();
    }

    ecx = kLegacyBattleActorOrderToken;
    while (true) {
        if (order_index >= bindings.final_actor.actor_order.size()) {
            result.status = LegacyBattleActorActionCandidateAvailabilityStatus::
                actor_order_typed_stop;
            return finish();
        }
        const u32 actor_code = bindings.final_actor.actor_order[order_index];
        ++result.actor_order_reads;
        if (actor_code == request.actor_code) {
            const u32 actor_index = request.actor_code - 8U;
            ecx = actor_index;
            eax = ecx;
            eax <<= 6U;
            eax -= ecx;
            eax <<= 4U;
            eax -= ecx;
            eax = eax + eax * 2U;
            ecx = kLegacyBattleActionGroupABaseToken + eax * 4U;
            if (!valid_group_a_index(actor_index)) {
                result.status =
                    LegacyBattleActorActionCandidateAvailabilityStatus::
                        group_a_actor_typed_stop;
                return finish();
            }
            const auto reply = port.invoke_input_dispatch({
                .call = LegacyBattleInputDispatchCall::query_active_actor,
                .eax = eax,
                .ecx = ecx,
                .edx = edx,
            });
            ++result.port_calls;
            ++result.actor_query_calls;
            eax = reply.eax == 0U ? 1U : 0U;
            ecx = reply.ecx;
            edx = reply.edx;
            return finish();
        }

        ++eax;
        ++order_index;
        ecx += 4U;
        if (eax >= edx) {
            eax = 0U;
            return finish();
        }
    }
}

}  // namespace openswd3::battle
