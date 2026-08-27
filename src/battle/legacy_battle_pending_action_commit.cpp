#include "openswd3/battle/legacy_battle_pending_action_commit.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32
wrapping_multiply(const u32 left, const u32 right) noexcept {
    return left * right;
}

[[nodiscard]] LegacyBattlePendingActionCallReply invoke(
    LegacyBattlePendingActionPort& port,
    LegacyBattlePendingActionResult& result,
    const LegacyBattlePendingActionCallRequest& request
) {
    const auto reply = port.invoke_pending_action(request);
    result.return_value = reply.eax;
    result.final_ecx = reply.ecx;
    result.final_edx = reply.edx;
    ++result.port_calls;
    return reply;
}

}  // namespace

LegacyBattlePendingActionResult commit_legacy_battle_pending_actions(
    const LegacyBattlePendingActionBindings bindings,
    LegacyBattlePendingActionPort& port,
    const u32 caller_edx
) {
    LegacyBattlePendingActionResult result;
    auto& metrics = port.actor_metric_state();
    result.initial_count = metrics.group_b_count + metrics.group_a_count;
    result.return_value = result.initial_count;
    result.final_ecx = metrics.group_a_count;
    result.final_edx = caller_edx;
    if (as_i32(result.initial_count) <= 0) {
        return result;
    }

    u32 remaining = result.initial_count;
    while (remaining != 0U) {
        const std::size_t slot = result.scanned_slots;
        if (slot >= metrics.actor_order.size()) {
            result.status =
                LegacyBattlePendingActionStatus::actor_order_typed_stop;
            return result;
        }

        const u32 prepare_code = metrics.actor_order[slot];
        ++result.actor_order_reads;
        const bool group_a = as_i32(prepare_code) >= 8;
        const u32 prepare_index = group_a ? prepare_code - 8U : prepare_code;
        const u32 prepare_eax =
            wrapping_multiply(prepare_index, group_a ? 0x03EFU : 0x0565U);
        const u32 prepare_edx =
            wrapping_multiply(prepare_index, group_a ? 0x0BCDU : 0x0159U);
        const u32 prepare_token =
            (group_a ? kLegacyBattlePendingActionGroupABaseToken
                     : kLegacyBattlePendingActionGroupBBaseToken) +
            wrapping_multiply(
                prepare_index,
                group_a ? kLegacyBattlePendingActionGroupAStride
                        : kLegacyBattlePendingActionGroupBStride
            );
        const auto prepare = invoke(
            port,
            result,
            {
                .call = LegacyBattlePendingActionCall::prepare_actor,
                .actor_token = prepare_token,
                .actor_code = prepare_code,
                .actor_index = prepare_index,
                .actor_group = group_a ? 1U : 0U,
                .eax = prepare_eax,
                .ecx = prepare_token,
                .edx = prepare_edx,
            }
        );
        ++result.prepare_calls;

        const u32 ready_code = metrics.actor_order[slot];
        ++result.actor_order_reads;
        const u32 ready_index = group_a ? ready_code - 8U : ready_code;
        result.last_ready = query_legacy_battle_actor_ready(
            {
                .global_mode = bindings.global_mode,
                .caller_edx = prepare.edx,
            },
            port,
            ready_index,
            group_a ? 1U : 0U
        );
        ++result.ready_calls;
        result.port_calls += result.last_ready.port_calls;
        result.return_value = result.last_ready.return_value;
        result.final_ecx = result.last_ready.final_ecx;
        result.final_edx = result.last_ready.final_edx;

        u32 group_b_commit_edx = result.last_ready.final_edx;
        if (result.last_ready.return_value == 1U) {
            const u32 ready_store_code = metrics.actor_order[slot];
            ++result.actor_order_reads;
            const u32 ready_store_index =
                group_a ? ready_store_code - 8U : ready_store_code;
            if (group_a) {
                result.final_ecx = ready_store_code;
            } else {
                result.final_edx = ready_store_code;
                group_b_commit_edx = ready_store_code;
            }
            metrics.pending_action_activation_latch = 1U;
            if (ready_store_index >= bindings.ready_actor_slots.size()) {
                result.status = LegacyBattlePendingActionStatus::
                    ready_actor_slot_typed_stop;
                return result;
            }
            bindings.ready_actor_slots[ready_store_index] = 0xFFFFFFFFU;
            ++result.ready_slot_writes;
        }

        const u32 commit_code = metrics.actor_order[slot];
        ++result.actor_order_reads;
        const u32 commit_index = group_a ? commit_code - 8U : commit_code;
        const u32 commit_eax =
            wrapping_multiply(commit_index, group_a ? 0x03EFU : 0x0159U);
        const u32 commit_edx = group_a
            ? wrapping_multiply(commit_index, 0x0BCDU)
            : group_b_commit_edx;
        const u32 commit_token =
            (group_a ? kLegacyBattlePendingActionGroupABaseToken
                     : kLegacyBattlePendingActionGroupBBaseToken) +
            wrapping_multiply(
                commit_index,
                group_a ? kLegacyBattlePendingActionGroupAStride
                        : kLegacyBattlePendingActionGroupBStride
            );
        const auto commit = invoke(
            port,
            result,
            {
                .call = LegacyBattlePendingActionCall::commit_actor,
                .actor_token = commit_token,
                .actor_code = commit_code,
                .actor_index = commit_index,
                .actor_group = group_a ? 1U : 0U,
                .eax = commit_eax,
                .ecx = commit_token,
                .edx = commit_edx,
            }
        );
        ++result.commit_calls;

        if (commit.eax == 1U) {
            const u32 publication_code = metrics.actor_order[slot];
            ++result.actor_order_reads;
            const u32 publication_index =
                group_a ? publication_code - 8U : publication_code;
            result.return_value = publication_index;
            result.final_ecx = commit.ecx;
            result.final_edx = commit.edx;
            auto& publication = port.actor_publication_state().slots;
            if (publication_index >= publication.size()) {
                result.status = LegacyBattlePendingActionStatus::
                    actor_publication_slot_typed_stop;
                return result;
            }
            publication[publication_index] = publication_index;
            ++result.publication_writes;

            static_cast<void>(invoke(
                port,
                result,
                {
                    .call = LegacyBattlePendingActionCall::remove_actor_record,
                    .actor_code = publication_code,
                    .actor_index = publication_index,
                    .actor_group = group_a ? 1U : 0U,
                    .arguments = {publication_index, 0U},
                    .eax = publication_index,
                    .ecx = commit.ecx,
                    .edx = commit.edx,
                }
            ));
            ++result.remove_calls;
        }

        ++result.scanned_slots;
        --remaining;
    }
    return result;
}

}  // namespace openswd3::battle
