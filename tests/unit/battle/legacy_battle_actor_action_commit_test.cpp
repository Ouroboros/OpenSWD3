#include "openswd3/battle/legacy_battle_actor_action_commit.hpp"

#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorActionCommitBindings;
using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::compat::u32;

class ActorActionCommitPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        const std::size_t index = calls.size() - 1U;
        if (index < replies.size() && replies[index].has_value()) {
            return *replies[index];
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::vector<std::optional<LegacyBattleInputDispatchCallReply>> replies;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    u32 message{};
    ActorActionCommitPort port;

    Fixture() {
        startup.value_4ff0b0 = 9U;
        startup.value_4ff0b4 = 9U;
        startup.value_4ff0b8 = 9U;
        startup.value_53bf22 = 9U;
    }

    [[nodiscard]] LegacyBattleActorActionCommitBindings bindings() {
        return {
            .startup_reset = startup,
            .final_actor = final_actor,
            .metrics = metrics,
            .input_dispatch = input,
            .message_state = message,
        };
    }

    [[nodiscard]] bool caches_cleared() const {
        return startup.value_4ff0b0 == 0U && startup.value_4ff0b4 == 0U &&
            startup.value_4ff0b8 == 0U && startup.value_53bf22 == 0U;
    }
};

}  // namespace

void test_battle_actor_action_commit(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorActionCommitStatus;
    using openswd3::battle::commit_legacy_battle_actor_action;

    {
        Fixture fixture;
        fixture.message = 5U;
        fixture.input.selected_option_word = 0xFFFFU;
        fixture.metrics.group_a_count = 2U;
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 8U,
             .entry_eax = 0x11U,
             .entry_ecx = 0x22U,
             .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 5U && result.return_ecx == 0x22U &&
                result.return_edx == 0x33U && result.actor_order_reads == 0U &&
                !fixture.caches_cleared() && fixture.port.calls.empty(),
            "nonzero message with an absent option returns before queue and cache access"
        );
    }

    {
        Fixture fixture;
        fixture.message = 5U;
        fixture.input.selected_option_word = 3U;
        fixture.metrics.group_a_count = 1U;
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 8U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x33U && fixture.caches_cleared(),
            "present option bypasses the nonzero-message gate and clears caches for a one-actor queue"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 3U;
        fixture.final_actor.actor_order[0U] = 8U;
        fixture.final_actor.actor_order[1U] = 0U;
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 9U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.actor_order_reads == 2U && result.actor_query_calls == 0U &&
                result.return_eax == 2U && result.return_ecx == 0U &&
                result.return_edx == 2U && fixture.caches_cleared(),
            "unmatched queue scans exactly count minus one slots and returns the live loop registers"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.final_actor.actor_order[0U] = 8U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.port.replies.push_back(
            LegacyBattleInputDispatchCallReply{
                .eax = 1U,
                .ecx = 0xAAU,
                .edx = 0xBBU,
            }
        );
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 8U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.actor_order_reads == 1U && result.actor_query_calls == 1U &&
                !result.actor_swapped && fixture.port.calls.size() == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::query_active_actor &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == 0x005029D0U &&
                fixture.port.calls[0U].edx == 0x33U &&
                fixture.final_actor.queued_actor_code == 9U &&
                fixture.final_actor.actor_order[0U] == 8U &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 1U && fixture.caches_cleared(),
            "completed actor query keeps the queue and overwrites callee EDX with live count minus one"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.final_actor.actor_order[0U] = 9U;
        fixture.final_actor.queued_actor_code = 12U;
        fixture.port.replies.push_back(
            LegacyBattleInputDispatchCallReply{
                .eax = 0U,
                .ecx = 0xAAU,
                .edx = 0xBBU,
            }
        );
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 9U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.actor_swapped &&
                fixture.final_actor.queued_actor_code == 9U &&
                fixture.final_actor.actor_order[0U] == 12U &&
                fixture.port.calls[0U].eax == 0x3EFU &&
                fixture.port.calls[0U].ecx == 0x00505904U &&
                fixture.port.calls[0U].edx == 0x33U &&
                result.return_eax == 12U && result.return_ecx == 0U &&
                result.return_edx == 0xBBU && fixture.caches_cleared(),
            "unfinished actor query swaps the current actor with the matching queue slot before cache clear"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 3U;
        fixture.final_actor.actor_order[0U] = 8U;
        fixture.final_actor.actor_order[1U] = 8U;
        fixture.final_actor.queued_actor_code = 10U;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{.eax = 1U},
            LegacyBattleInputDispatchCallReply{
                .eax = 0U,
                .ecx = 0xAAU,
                .edx = 0xBBU,
            },
        };
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 8U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.actor_order_reads == 2U && result.actor_query_calls == 2U &&
                fixture.port.calls[0U].edx == 0x33U &&
                fixture.port.calls[1U].edx == 2U && result.actor_swapped &&
                fixture.final_actor.actor_order[0U] == 8U &&
                fixture.final_actor.actor_order[1U] == 10U &&
                fixture.final_actor.queued_actor_code == 8U,
            "live loop reload reaches a second matching slot with count-minus-one EDX after the first completed query"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 12U;
        fixture.final_actor.actor_order.fill(1U);
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 2U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionCommitStatus::
                        actor_order_typed_stop &&
                result.actor_order_reads == 10U &&
                result.actor_query_calls == 0U && result.return_eax == 11U &&
                result.return_ecx == 0x22U && result.return_edx == 11U &&
                !fixture.caches_cleared(),
            "live count beyond the ten-slot physical queue stops on the eleventh real read without cache clear"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.final_actor.actor_order[0U] = 7U;
        const auto result = commit_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.actor_code = 7U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionCommitStatus::
                        group_a_actor_typed_stop &&
                result.actor_order_reads == 1U &&
                result.actor_query_calls == 0U &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU &&
                result.return_edx == 0x33U && !fixture.caches_cleared() &&
                fixture.port.calls.empty(),
            "matching code seven stops at the first real group-A actor query after preserving address arithmetic"
        );
    }
}
