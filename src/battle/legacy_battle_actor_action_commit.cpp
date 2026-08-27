#include "openswd3/battle/legacy_battle_actor_action_commit.hpp"

namespace openswd3::battle {
namespace {

using compat::u32;

[[nodiscard]] constexpr bool valid_group_a_index(const u32 index) noexcept {
    return index < 10U;
}

}  // namespace

LegacyBattleActorActionCommitResult commit_legacy_battle_actor_action(
    const LegacyBattleActorActionCommitBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActorActionCommitRequest& request
) {
    LegacyBattleActorActionCommitResult result;
    u32 eax = bindings.message_state;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;
    u32 actor_code{};
    u32 order_index{};

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto clear_selection_cache = [&]() {
        ecx = 0U;
        bindings.startup_reset.value_4ff0b0 = 0U;
        bindings.startup_reset.value_4ff0b4 = 0U;
        bindings.startup_reset.value_4ff0b8 = 0U;
        bindings.startup_reset.value_53bf22 = 0U;
    };

    if (eax != 0U && bindings.input_dispatch.selected_option_word == 0xFFFFU) {
        return finish();
    }

    eax = bindings.metrics.group_a_count;
    eax -= 1U;
    if (eax == 0U) {
        clear_selection_cache();
        return finish();
    }

    while (true) {
        if (order_index >= bindings.final_actor.actor_order.size()) {
            result.status =
                LegacyBattleActorActionCommitStatus::actor_order_typed_stop;
            return finish();
        }
        actor_code = bindings.final_actor.actor_order[order_index];
        ++result.actor_order_reads;
        if (actor_code != 0U && actor_code == request.actor_code) {
            const u32 actor_index = actor_code - 8U;
            eax = actor_index;
            eax <<= 6U;
            eax -= actor_index;
            eax <<= 4U;
            eax -= actor_index;
            ecx = kLegacyBattleActionGroupABaseToken + (eax + eax * 2U) * 4U;
            if (!valid_group_a_index(actor_index)) {
                result.status = LegacyBattleActorActionCommitStatus::
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
            eax = reply.eax;
            ecx = reply.ecx;
            edx = reply.edx;
            if (eax != 1U) {
                eax = bindings.final_actor.queued_actor_code;
                bindings.final_actor.queued_actor_code = actor_code;
                bindings.final_actor.actor_order[order_index] = eax;
                result.actor_swapped = true;
                clear_selection_cache();
                return finish();
            }
        }

        edx = bindings.metrics.group_a_count;
        ++order_index;
        --edx;
        if (order_index >= edx) {
            clear_selection_cache();
            return finish();
        }
    }
}

}  // namespace openswd3::battle
