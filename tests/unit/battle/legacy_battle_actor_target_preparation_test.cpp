#include "openswd3/battle/legacy_battle_actor_target_preparation.hpp"

#include <deque>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorTargetPreparationCall;
using openswd3::battle::LegacyBattleActorTargetPreparationCallReply;
using openswd3::battle::LegacyBattleActorTargetPreparationCallRequest;
using openswd3::compat::u32;

class PreparationPort final
    : public openswd3::battle::LegacyBattleActorTargetPreparationPort {
public:
    [[nodiscard]] LegacyBattleActorTargetPreparationCallReply
    invoke_actor_target_preparation(
        const LegacyBattleActorTargetPreparationCallRequest& request
    ) override {
        calls.push_back(request);
        if (!replies.empty()) {
            const auto reply = replies.front();
            replies.pop_front();
            return reply;
        }
        return default_reply;
    }

    std::vector<LegacyBattleActorTargetPreparationCallRequest> calls;
    std::deque<LegacyBattleActorTargetPreparationCallReply> replies;
    LegacyBattleActorTargetPreparationCallReply default_reply{};
};

class PreparationRandom final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        return value;
    }

    u32 value{};
    std::vector<u32> bounds;
};

struct Fixture {
    [[nodiscard]]
    openswd3::battle::LegacyBattleActorTargetPreparationBindings bindings() {
        return {
            .debug_hotkeys = debug,
            .target_runtime = target,
            .action = action,
            .final_actor = final_actor,
            .metrics = metrics,
        };
    }

    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    PreparationRandom random;
    PreparationPort port;
};

}  // namespace

void test_battle_actor_target_preparation(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorTargetPreparationStatus;
    using openswd3::battle::prepare_legacy_battle_actor_target;

    {
        Fixture fixture;
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(),
            fixture.random,
            fixture.port,
            {.actor_code = 7U, .entry_edx = 0x12345678U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::
                        group_a_actor_typed_stop &&
                fixture.debug.committed_actor_code == 7U &&
                fixture.target.selected_action_kind == 1U &&
                fixture.target.actor_commit_gate == 1U &&
                fixture.action.opponent_workspace[9U] == 1U &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU &&
                result.return_edx == 0x12345678U && result.port_calls == 0U,
            "actor code seven writes the physically preceding workspace slot then stops at the first group-A object call"
        );
    }

    {
        Fixture fixture;
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(),
            fixture.random,
            fixture.port,
            {.actor_code = 124U, .entry_edx = 0x55667788U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::
                        action_workspace_typed_stop &&
                fixture.debug.committed_actor_code == 124U &&
                fixture.target.selected_action_kind == 1U &&
                fixture.target.actor_commit_gate == 1U &&
                result.return_eax == 7308U && result.return_ecx == 116U &&
                result.return_edx == 0x55667788U && result.port_calls == 0U,
            "workspace index one hundred twenty-six stops at its first write after preserving the three global publications"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.group_a_availability_blocks[0U].write_accessible =
            false;
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(),
            fixture.random,
            fixture.port,
            {.actor_code = 8U, .entry_edx = 0x11223344U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x11223344U &&
                fixture.debug.committed_actor_code == 8U &&
                fixture.target.selected_action_kind == 1U &&
                fixture.target.actor_commit_gate == 1U &&
                fixture.action.opponent_workspace[10U] == 1U &&
                fixture.random.bounds.empty() && fixture.port.calls.empty(),
            "typed actor write stop preserves the target-preparation prefix and suppresses every caller suffix"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 3U;
        fixture.action.opponent_processed_counter = 0xAABBCC05U;
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(),
            fixture.random,
            fixture.port,
            {.actor_code = 8U, .entry_edx = 8U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::completed &&
                result.port_calls == 0U && result.random_calls == 0U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    1U &&
                result.return_eax == 3U && result.return_ecx == 5U &&
                result.return_edx == 8U && fixture.port.calls.empty(),
            "processed group-B byte at or above the signed count writes the typed actor owner before returning the count and processed byte"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 0xFFFFFFFFU;
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(), fixture.random, fixture.port, {.actor_code = 8U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::completed &&
                result.return_eax == 0xFFFFFFFFU && result.return_ecx == 0U &&
                result.random_calls == 0U,
            "negative signed group-B count returns before random selection without unsigned normalization"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 5U;
        fixture.random.value = 2U;
        fixture.port.replies = {
            {.eax = 0xAAU, .ecx = 0xBBU, .edx = 0xCCU},
        };
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(),
            fixture.random,
            fixture.port,
            {.actor_code = 8U, .entry_edx = 8U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::completed &&
                fixture.random.bounds == std::vector<u32>{5U} &&
                fixture.final_actor.published_actor_code == 3U &&
                result.group_b_queries == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleActorTargetPreparationCall::
                        query_group_b_completion &&
                fixture.port.calls[0U].object_token == 0x0052AB58U &&
                fixture.port.calls[0U].eax == 0x102FU &&
                fixture.port.calls[0U].ecx == 0x0052AB58U &&
                fixture.port.calls[0U].edx == 0x09CFU &&
                result.return_eax == 0xAAU && result.return_ecx == 0xBBU &&
                result.return_edx == 0xCCU,
            "random result becomes a one-based group-B code with the original multiplication-register shape"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 3U;
        fixture.random.value = 2U;
        fixture.port.replies = {
            {.eax = 1U, .ecx = 0x11U, .edx = 0x21U},
            {.eax = 1U, .ecx = 0x12U, .edx = 0x22U},
            {.eax = 0U, .ecx = 0x13U, .edx = 0x23U},
        };
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(), fixture.random, fixture.port, {.actor_code = 8U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::completed &&
                fixture.final_actor.published_actor_code == 2U &&
                result.group_b_queries == 3U &&
                result.scanned_completed_targets == 2U &&
                result.return_eax == 0U && result.return_ecx == 0x13U &&
                result.return_edx == 0x23U,
            "completed random target wraps from three to one and continues until the first incomplete target"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 3U;
        fixture.random.value = 0U;
        fixture.port.replies = {
            {.eax = 1U, .edx = 0x31U},
            {.eax = 1U, .edx = 0x32U},
            {.eax = 1U, .edx = 0x33U},
        };
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(), fixture.random, fixture.port, {.actor_code = 8U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::completed &&
                fixture.final_actor.published_actor_code == 1U &&
                result.group_b_queries == 3U &&
                result.scanned_completed_targets == 3U &&
                result.return_eax == 3U && result.return_ecx == 1U &&
                result.return_edx == 0x33U,
            "all completed targets stop after the signed live count while preserving the last callee EDX"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_b_count = 9U;
        fixture.random.value = 8U;
        const auto result = prepare_legacy_battle_actor_target(
            fixture.bindings(), fixture.random, fixture.port, {.actor_code = 8U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetPreparationStatus::
                        group_b_actor_typed_stop &&
                fixture.final_actor.published_actor_code == 9U &&
                result.return_eax == 0x308DU &&
                result.return_ecx == 0x0053AE48U &&
                result.return_edx == 0x1D6DU && result.group_b_queries == 0U,
            "live count above the physical group permits the original random result and stops only at the ninth object query"
        );
    }
}
