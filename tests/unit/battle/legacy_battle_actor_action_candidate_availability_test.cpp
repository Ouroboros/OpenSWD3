#include "openswd3/battle/legacy_battle_actor_action_candidate_availability.hpp"

#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorActionCandidateAvailabilityBindings;
using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;

class CandidateAvailabilityPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        if (reply.has_value()) {
            return *reply;
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::optional<LegacyBattleInputDispatchCallReply> reply;
    std::vector<LegacyBattleInputDispatchCallRequest> calls;
};

struct Fixture {
    openswd3::battle::LegacyBattleFinalActorStepState actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    CandidateAvailabilityPort port;

    [[nodiscard]] LegacyBattleActorActionCandidateAvailabilityBindings
    bindings() {
        return {.final_actor = actor, .metrics = metrics};
    }
};

}  // namespace

void test_battle_actor_action_candidate_availability(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorActionCandidateAvailabilityStatus;
    using openswd3::battle::
        query_legacy_battle_actor_action_candidate_availability;

    {
        Fixture fixture;
        const auto result =
            query_legacy_battle_actor_action_candidate_availability(
                fixture.bindings(),
                fixture.port,
                {
                    .actor_code = 8U,
                    .entry_eax = 0x11U,
                    .entry_ecx = 0x22U,
                    .entry_edx = 0x33U,
                }
            );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x22U &&
                result.return_edx == 0U && result.actor_order_reads == 0U &&
                result.actor_query_calls == 0U && fixture.port.calls.empty(),
            "zero group-A count returns false without loading the queue pointer"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 3U;
        fixture.actor.actor_order = {8U, 9U, 10U};
        const auto result =
            query_legacy_battle_actor_action_candidate_availability(
                fixture.bindings(), fixture.port, {.actor_code = 11U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionCandidateAvailabilityStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0x00520DDCU &&
                result.return_edx == 3U && result.actor_order_reads == 3U &&
                result.actor_query_calls == 0U && fixture.port.calls.empty(),
            "missing candidate scans exactly the fixed count and returns the one-past queue token"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 3U;
        fixture.actor.actor_order = {9U, 8U, 10U};
        fixture.port.reply = LegacyBattleInputDispatchCallReply{
            .eax = 0U, .ecx = 0xAAU, .edx = 0xBBU
        };
        const auto result =
            query_legacy_battle_actor_action_candidate_availability(
                fixture.bindings(), fixture.port, {.actor_code = 9U}
            );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0xAAU &&
                result.return_edx == 0xBBU && result.actor_order_reads == 1U &&
                result.actor_query_calls == 1U && result.port_calls == 1U &&
                fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::query_active_actor &&
                fixture.port.calls[0U].eax == 0x00000BCDU &&
                fixture.port.calls[0U].ecx == 0x00505904U &&
                fixture.port.calls[0U].edx == 3U,
            "matching candidate returns true only when its actor query returns zero"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.actor.actor_order = {8U, 9U};
        fixture.port.reply = LegacyBattleInputDispatchCallReply{
            .eax = 0xFFFFFFFFU, .ecx = 0xCCU, .edx = 0xDDU
        };
        const auto result =
            query_legacy_battle_actor_action_candidate_availability(
                fixture.bindings(), fixture.port, {.actor_code = 9U}
            );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0xCCU &&
                result.return_edx == 0xDDU && result.actor_order_reads == 2U &&
                result.actor_query_calls == 1U,
            "any nonzero actor query result normalizes the matching candidate to false"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 11U;
        fixture.actor.actor_order.fill(1U);
        const auto result =
            query_legacy_battle_actor_action_candidate_availability(
                fixture.bindings(), fixture.port, {.actor_code = 8U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionCandidateAvailabilityStatus::
                        actor_order_typed_stop &&
                result.return_eax == 10U && result.return_ecx == 0x00520DF8U &&
                result.return_edx == 11U && result.actor_order_reads == 10U &&
                result.actor_query_calls == 0U && fixture.port.calls.empty(),
            "fixed count above ten stops at the eleventh real queue read after preserving ten prefixes"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.actor.actor_order[0U] = 7U;
        const auto result =
            query_legacy_battle_actor_action_candidate_availability(
                fixture.bindings(), fixture.port, {.actor_code = 7U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionCandidateAvailabilityStatus::
                        group_a_actor_typed_stop &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 1U &&
                result.actor_order_reads == 1U &&
                result.actor_query_calls == 0U && fixture.port.calls.empty(),
            "one-before-base candidate stops at the first actor query after exact wrapped arithmetic"
        );
    }
}
