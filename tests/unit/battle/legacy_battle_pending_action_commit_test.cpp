#include "openswd3/battle/legacy_battle_pending_action_commit.hpp"
#include "test.hpp"

#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorReadyCallReply;
using openswd3::battle::LegacyBattleActorReadyRequest;
using openswd3::battle::LegacyBattlePendingActionCall;
using openswd3::battle::LegacyBattlePendingActionCallReply;
using openswd3::battle::LegacyBattlePendingActionCallRequest;
using openswd3::compat::u32;

class PendingActionPort final
    : public openswd3::battle::LegacyBattlePendingActionPort {
public:
    [[nodiscard]] LegacyBattlePendingActionCallReply invoke_pending_action(
        const LegacyBattlePendingActionCallRequest& request
    ) override {
        requests.push_back(request);
        switch (request.call) {
        case LegacyBattlePendingActionCall::prepare_actor:
            events.emplace_back("prepare");
            if (order_after_prepare.has_value()) {
                actor_metric_state().actor_order[0] = *order_after_prepare;
                order_after_prepare.reset();
            }
            break;
        case LegacyBattlePendingActionCall::commit_actor:
            events.emplace_back("commit");
            if (order_after_commit.has_value()) {
                actor_metric_state().actor_order[0] = *order_after_commit;
                order_after_commit.reset();
            }
            break;
        case LegacyBattlePendingActionCall::reserved_remove_actor_record:
            events.emplace_back("reserved-remove");
            break;
        }
        auto& queue = replies[request.call];
        if (queue.empty()) {
            return {};
        }
        const auto reply = queue.front();
        queue.pop_front();
        return reply;
    }

    [[nodiscard]] LegacyBattleActorReadyCallReply
    query_ready(const LegacyBattleActorReadyRequest& request) override {
        ready_requests.push_back(request);
        events.emplace_back("ready");
        if (order_after_ready.has_value()) {
            actor_metric_state().actor_order[0] = *order_after_ready;
            order_after_ready.reset();
        }
        if (ready_replies.empty()) {
            return {};
        }
        const auto reply = ready_replies.front();
        ready_replies.pop_front();
        return reply;
    }

    void push(
        const LegacyBattlePendingActionCall call,
        const LegacyBattlePendingActionCallReply reply
    ) {
        replies[call].push_back(reply);
    }

    std::map<
        LegacyBattlePendingActionCall,
        std::deque<LegacyBattlePendingActionCallReply>>
        replies;
    std::deque<LegacyBattleActorReadyCallReply> ready_replies;
    std::vector<LegacyBattlePendingActionCallRequest> requests;
    std::vector<LegacyBattleActorReadyRequest> ready_requests;
    std::vector<std::string> events;
    std::optional<u32> order_after_prepare;
    std::optional<u32> order_after_ready;
    std::optional<u32> order_after_commit;
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 18> records{};
    openswd3::battle::LegacyBattleIntensityEffectRecord adjacent_record{};
};

[[nodiscard]] openswd3::battle::LegacyBattlePendingActionBindings bindings(
    PendingActionPort& port, std::span<u32> ready_slots, const u32 global_mode
) {
    return {
        .ready_actor_slots = ready_slots,
        .attack_order_records = port.records,
        .attack_order_adjacent_record = &port.adjacent_record,
        .global_mode = global_mode,
    };
}

[[nodiscard]] const LegacyBattlePendingActionCallRequest& request_at(
    const PendingActionPort& port,
    const LegacyBattlePendingActionCall call,
    const std::size_t occurrence
) {
    std::size_t found = 0U;
    for (const auto& request : port.requests) {
        if (request.call != call) {
            continue;
        }
        if (found == occurrence) {
            return request;
        }
        ++found;
    }
    return port.requests.front();
}

}  // namespace

void test_battle_pending_action_commit(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattlePendingActionBindings;
    using openswd3::battle::LegacyBattlePendingActionStatus;

    {
        PendingActionPort port;
        port.actor_metric_state().group_b_count = 0x80000000U;
        port.actor_metric_state().group_a_count = 0U;
        std::array<u32, 18> ready_slots{};

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                bindings(port, ready_slots, 0U), port, 0xAABBCCDDU
            );

        test.expect_true(
            result.status == LegacyBattlePendingActionStatus::completed &&
                result.return_value == 0x80000000U && result.final_ecx == 0U &&
                result.final_edx == 0xAABBCCDDU && result.port_calls == 0U,
            "signed nonpositive wrapped actor total returns before the order array"
        );
    }

    {
        PendingActionPort port;
        auto& metrics = port.actor_metric_state();
        metrics.group_b_count = 1U;
        metrics.group_a_count = 1U;
        metrics.actor_order[0] = 2U;
        metrics.actor_order[1] = 9U;
        std::array<u32, 18> ready_slots{};
        port.ready_replies = {
            {.eax = 1U, .ecx = 0x10101010U, .edx = 0x20202020U},
            {.eax = 2U, .ecx = 0x30303030U, .edx = 0x40404040U},
        };
        port.push(
            LegacyBattlePendingActionCall::prepare_actor,
            {.eax = 7U, .ecx = 8U, .edx = 0x90909090U}
        );
        port.push(
            LegacyBattlePendingActionCall::prepare_actor,
            {.eax = 9U, .ecx = 10U, .edx = 0xA0A0A0A0U}
        );
        port.push(
            LegacyBattlePendingActionCall::commit_actor,
            {.eax = 1U, .ecx = 0x11112222U, .edx = 0x33334444U}
        );
        port.push(
            LegacyBattlePendingActionCall::commit_actor,
            {.eax = 2U, .ecx = 0x55556666U, .edx = 0x77778888U}
        );
        port.records[0].value_00 = 2U;

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                bindings(port, ready_slots, 1U), port
            );
        const auto& prepare_b =
            request_at(port, LegacyBattlePendingActionCall::prepare_actor, 0U);
        const auto& prepare_a =
            request_at(port, LegacyBattlePendingActionCall::prepare_actor, 1U);
        const auto& commit_b =
            request_at(port, LegacyBattlePendingActionCall::commit_actor, 0U);
        const auto& commit_a =
            request_at(port, LegacyBattlePendingActionCall::commit_actor, 1U);

        test.expect_true(
            result.status == LegacyBattlePendingActionStatus::completed &&
                result.scanned_slots == 2U && result.prepare_calls == 2U &&
                result.ready_calls == 2U && result.commit_calls == 2U &&
                result.remove_calls == 1U && result.port_calls == 6U &&
                prepare_b.actor_token == 0x0052AB58U &&
                prepare_b.eax == 0x00000ACAU && prepare_b.edx == 0x000002B2U &&
                commit_b.eax == 0x000002B2U && commit_b.edx == 2U &&
                prepare_a.actor_token == 0x00505904U &&
                prepare_a.eax == 0x000003EFU && prepare_a.edx == 0x00000BCDU &&
                commit_a.eax == 0x000003EFU && commit_a.edx == 0x00000BCDU &&
                ready_slots[2] == 0xFFFFFFFFU &&
                metrics.pending_action_activation_latch == 1U &&
                port.actor_publication_state().slots[2] == 2U &&
                result.return_value == 2U && result.final_ecx == 0x55556666U &&
                result.final_edx == 0x77778888U,
            "mixed actor order preserves group-specific stale registers strict ready and commit gates and the last callee return"
        );
    }

    {
        PendingActionPort port;
        auto& metrics = port.actor_metric_state();
        metrics.group_b_count = 1U;
        metrics.actor_order[0] = 0U;
        std::array<u32, 18> ready_slots{};
        port.order_after_prepare = 3U;
        port.order_after_ready = 4U;
        port.order_after_commit = 5U;
        port.ready_replies.push_back(
            {.eax = 1U, .ecx = 0x11111111U, .edx = 0x22222222U}
        );
        port.push(LegacyBattlePendingActionCall::prepare_actor, {});
        port.push(
            LegacyBattlePendingActionCall::commit_actor,
            {.eax = 1U, .ecx = 0x33333333U, .edx = 0x44444444U}
        );
        port.records[0].value_00 = 5U;

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                bindings(port, ready_slots, 0U), port
            );
        const auto& prepare =
            request_at(port, LegacyBattlePendingActionCall::prepare_actor, 0U);
        const auto& commit =
            request_at(port, LegacyBattlePendingActionCall::commit_actor, 0U);
        test.expect_true(
            result.status == LegacyBattlePendingActionStatus::completed &&
                port.events ==
                    std::vector<std::string>{"prepare", "ready", "commit"} &&
                prepare.actor_code == 0U &&
                port.ready_requests.front().actor_token == 0x0052D680U &&
                ready_slots[4] == 0xFFFFFFFFU && commit.actor_code == 4U &&
                commit.actor_token == 0x005301A8U && commit.edx == 4U &&
                port.actor_publication_state().slots[5] == 5U &&
                result.attack_order_remove.matched &&
                result.attack_order_remove.matched_index == 0U &&
                port.records[0].value_00 == 0xFFFFFFFFU &&
                result.return_value == 0xFFFFFFFFU && result.final_ecx == 0U &&
                result.final_edx == 5U,
            "each callee observes the live actor order while the initial signed group branch remains fixed"
        );
    }

    {
        PendingActionPort port;
        auto& metrics = port.actor_metric_state();
        metrics.group_b_count = 1U;
        metrics.actor_order[0] = 0U;
        std::array<u32, 18> ready_slots{};
        port.order_after_ready = 0xFFFFFFFFU;
        port.ready_replies.push_back(
            {.eax = 1U, .ecx = 0x12345678U, .edx = 0x87654321U}
        );
        port.push(LegacyBattlePendingActionCall::prepare_actor, {});

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                bindings(port, ready_slots, 0U), port
            );

        test.expect_true(
            result.status ==
                    LegacyBattlePendingActionStatus::
                        ready_actor_slot_typed_stop &&
                metrics.pending_action_activation_latch == 1U &&
                result.return_value == 1U && result.final_ecx == 0x12345678U &&
                result.final_edx == 0xFFFFFFFFU && result.commit_calls == 0U,
            "ready slot overflow stops after publishing the activation latch and reloading the live actor code"
        );
    }

    {
        PendingActionPort port;
        auto& metrics = port.actor_metric_state();
        metrics.group_a_count = 1U;
        metrics.actor_order[0] = 8U;
        std::array<u32, 18> ready_slots{};
        port.ready_replies.push_back({.eax = 0U, .edx = 0xDEADBEEFU});
        port.order_after_commit = 0xFFFFFFFFU;
        port.push(LegacyBattlePendingActionCall::prepare_actor, {});
        port.push(
            LegacyBattlePendingActionCall::commit_actor,
            {.eax = 1U, .ecx = 0x01020304U, .edx = 0x05060708U}
        );

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                bindings(port, ready_slots, 0U), port
            );

        test.expect_true(
            result.status ==
                    LegacyBattlePendingActionStatus::
                        actor_publication_slot_typed_stop &&
                result.return_value == 0xFFFFFFF7U &&
                result.final_ecx == 0x01020304U &&
                result.final_edx == 0x05060708U &&
                result.publication_writes == 0U && result.remove_calls == 0U,
            "publication overflow stops after the actor commit and normalized live order reload"
        );
    }

    {
        PendingActionPort port;
        auto& metrics = port.actor_metric_state();
        metrics.group_b_count = 1U;
        metrics.actor_order[0] = 0U;
        std::array<u32, 18> ready_slots{};
        port.ready_replies.push_back({.eax = 1U});
        port.push(LegacyBattlePendingActionCall::prepare_actor, {});
        port.push(LegacyBattlePendingActionCall::commit_actor, {.eax = 1U});
        port.records[17].value_00 = 0U;
        auto call_bindings = bindings(port, ready_slots, 0U);
        call_bindings.attack_order_adjacent_record = nullptr;

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                call_bindings, port
            );

        test.expect_true(
            result.status ==
                    LegacyBattlePendingActionStatus::
                        attack_order_remove_typed_stop &&
                result.publication_writes == 1U && result.remove_calls == 1U &&
                result.scanned_slots == 0U && result.port_calls == 3U &&
                result.return_value == 0x00524980U && result.final_ecx == 7U &&
                result.final_edx == 0U &&
                result.attack_order_remove.status ==
                    openswd3::battle::LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop,
            "pending action removal stop preserves actor publication then blocks slot completion"
        );
    }

    {
        PendingActionPort port;
        auto& metrics = port.actor_metric_state();
        metrics.group_b_count = 19U;
        metrics.actor_order.fill(0U);
        std::array<u32, 18> ready_slots{};

        const auto result =
            openswd3::battle::commit_legacy_battle_pending_actions(
                bindings(port, ready_slots, 0U), port
            );

        test.expect_true(
            result.status ==
                    LegacyBattlePendingActionStatus::actor_order_typed_stop &&
                result.scanned_slots == 18U && result.prepare_calls == 18U &&
                result.ready_calls == 18U && result.commit_calls == 18U &&
                result.port_calls == 54U && result.actor_order_reads == 54U,
            "the original count has no modern cap and stops only at the nineteenth real order read"
        );
    }
}
