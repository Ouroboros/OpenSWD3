#include "openswd3/battle/legacy_battle_pre_frame.hpp"

#include "test.hpp"

#include <map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattlePreFrameCall;
using openswd3::battle::LegacyBattlePreFrameCallReply;
using openswd3::battle::LegacyBattlePreFrameCallRequest;
using openswd3::compat::u32;

class PreFramePort final : public openswd3::battle::LegacyBattlePreFramePort {
public:
    [[nodiscard]] LegacyBattlePreFrameCallReply
    invoke_pre_frame(const LegacyBattlePreFrameCallRequest& request) override {
        calls.push_back(request);
        auto& values = replies[request.call];
        if (values.empty()) {
            return {};
        }
        const auto reply = values.front();
        values.erase(values.begin());
        return reply;
    }

    void push(
        const LegacyBattlePreFrameCall call,
        const LegacyBattlePreFrameCallReply reply
    ) {
        replies[call].push_back(reply);
    }

    std::vector<LegacyBattlePreFrameCallRequest> calls;
    std::map<
        LegacyBattlePreFrameCall,
        std::vector<LegacyBattlePreFrameCallReply>>
        replies;
};

}  // namespace

void test_battle_pre_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleFinalActorStepState;
    using openswd3::battle::LegacyBattlePreFrameStatus;
    using openswd3::battle::advance_legacy_battle_pre_frame;

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 7U;
        const auto result = advance_legacy_battle_pre_frame(
            final_actor, action, port, 0x11112222U, 0x33334444U
        );
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 7U && result.return_ecx == 0x11112222U &&
                result.return_edx == 0x33334444U && result.port_calls == 0U,
            "terminal latch mismatch returns the loaded latch with caller ECX and EDX untouched"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 0U;
        auto result = advance_legacy_battle_pre_frame(
            final_actor, action, port, 0x11112222U, 0x33334444U
        );
        const bool zero_actor = result.return_value == 0U &&
            result.return_ecx == 0x11112222U &&
            result.return_edx == 0x33334444U && result.port_calls == 0U;
        final_actor.active_actor_code = 8U;
        port.battle_message_state() = 3U;
        result = advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            zero_actor && result.return_value == 8U &&
                result.return_ecx == 3U && result.return_edx == 3U &&
                final_actor.action_execution_active == 0U &&
                action.opponent_workspace[10U] == 0U,
            "zero active actor and message three return before execution and workspace publications"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 124U;
        port.battle_message_state() = 2U;
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameStatus::opponent_workspace_typed_stop &&
                final_actor.action_execution_active == 1U &&
                port.battle_message_state() == 2U && result.port_calls == 0U,
            "first actor workspace write stops only after publishing action execution active"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 0xFFFFFFFFU;
        port.battle_message_state() = 0U;
        const auto result = advance_legacy_battle_pre_frame(
            final_actor, action, port, 0xAAAA0000U, 0xBBBB0000U
        );
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 8U && result.return_ecx == 3U &&
                result.return_edx == 0xFFFFFFFFU &&
                final_actor.action_execution_active == 1U &&
                action.opponent_workspace[10U] == 1U &&
                final_actor.pre_frame_gate_a == 1U &&
                port.battle_message_state() == 3U &&
                final_actor.active_actor_code == 8U && result.port_calls == 0U,
            "missing source actor keeps the active actor and publishes workspace execution gate and message three"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 9U;
        final_actor.source_actor_code = 2U;
        port.battle_message_state() = 2U;
        port.push(
            LegacyBattlePreFrameCall::configure_group_a_actor,
            {.eax = 10U, .ecx = 11U, .edx = 12U}
        );
        port.push(
            LegacyBattlePreFrameCall::query_group_a_actor,
            {.eax = 1U, .ecx = 21U, .edx = 22U}
        );
        port.push(
            LegacyBattlePreFrameCall::notify_group_a_actor,
            {.eax = 30U, .ecx = 31U, .edx = 32U}
        );
        port.push(
            LegacyBattlePreFrameCall::configure_group_a_actor,
            {.eax = 40U, .ecx = 41U, .edx = 42U}
        );
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 9U && result.return_ecx == 41U &&
                result.return_edx == 5U && result.port_calls == 4U &&
                final_actor.active_actor_code == 0U &&
                final_actor.secondary_actor_code == 9U &&
                final_actor.published_actor_code == 2U &&
                final_actor.auxiliary_gate == 1U &&
                final_actor.action_execution_active == 5U &&
                port.battle_message_state() == 0U &&
                action.opponent_workspace[11U] == 5U &&
                final_actor.actor_runtime_records[1U][0U] == 1U &&
                port.calls[0].actor_token == 0x00505904U &&
                port.calls[0].argument == 1U,
            "current group-A actor success preserves four callee order and publishes the first five-dword runtime marker"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 7U;
        final_actor.source_actor_code = 1U;
        port.push(LegacyBattlePreFrameCall::configure_group_a_actor, {});
        port.push(LegacyBattlePreFrameCall::query_group_a_actor, {.eax = 1U});
        port.push(LegacyBattlePreFrameCall::notify_group_a_actor, {});
        port.push(LegacyBattlePreFrameCall::configure_group_a_actor, {});
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameStatus::
                        actor_runtime_record_typed_stop &&
                result.port_calls == 4U &&
                action.opponent_workspace[9U] == 5U &&
                final_actor.active_actor_code == 0U &&
                final_actor.secondary_actor_code == 7U,
            "out-of-range runtime marker stops after both configure calls notify and all preceding publications"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 1U;
        port.push(LegacyBattlePreFrameCall::configure_group_a_actor, {});
        port.push(
            LegacyBattlePreFrameCall::query_group_a_actor,
            {
                .eax = 1U,
                .publish_secondary_actor_code = true,
                .secondary_actor_code = 124U,
            }
        );
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameStatus::opponent_workspace_typed_stop &&
                result.port_calls == 2U &&
                final_actor.action_execution_active == 5U &&
                final_actor.secondary_actor_code == 124U &&
                action.opponent_workspace[10U] == 1U,
            "current query secondary rewrite stops at the second workspace store after publishing action mode five"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 1U;
        port.actor_metric_state().group_b_count = 3U;
        port.push(
            LegacyBattlePreFrameCall::configure_group_a_actor,
            {
                .publish_source_actor_code = true,
                .source_actor_code = 2U,
            }
        );
        port.push(LegacyBattlePreFrameCall::query_group_a_actor, {.eax = 0U});
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 1U});
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 2U});
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 0U});
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 0U && result.port_calls == 5U &&
                result.group_b_iterations == 2U &&
                final_actor.published_actor_code == 2U &&
                action.opponent_workspace[10U] == 1U &&
                port.calls[2].actor_token == 0x00528030U &&
                port.calls[3].actor_token == 0x00525508U &&
                port.calls[4].actor_token == 0x00528030U,
            "callee-updated source drives one-based group-B query before zero-based scan and first zero publishes index plus one"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 1U;
        port.actor_metric_state().group_b_count = 2U;
        port.push(LegacyBattlePreFrameCall::configure_group_a_actor, {});
        port.push(LegacyBattlePreFrameCall::query_group_a_actor, {.eax = 0U});
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 1U});
        port.push(
            LegacyBattlePreFrameCall::query_group_b_actor,
            {
                .eax = 2U,
                .publish_group_b_count = true,
                .group_b_count = 1U,
            }
        );
        port.push(
            LegacyBattlePreFrameCall::configure_group_a_actor,
            {.eax = 0xAABBCCDDU, .ecx = 0x11112222U, .edx = 0x33334444U}
        );
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 0xAABBCCDDU && result.return_ecx == 8U &&
                result.return_edx == 0x33334444U &&
                result.group_b_iterations == 1U &&
                port.actor_metric_state().group_b_count == 1U &&
                port.battle_terminal_latch() == 0U &&
                final_actor.source_actor_code == 0xFFFFFFFFU &&
                final_actor.published_actor_code == 1U &&
                final_actor.action_execution_active == 0U &&
                action.opponent_workspace[10U] == 0U &&
                port.calls.back().argument == 0U,
            "dynamic group-B count contraction enters teardown and preserves configure EAX EDX with secondary actor ECX"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 1U;
        port.actor_metric_state().group_b_count = 0U;
        port.push(LegacyBattlePreFrameCall::configure_group_a_actor, {});
        port.push(LegacyBattlePreFrameCall::query_group_a_actor, {.eax = 0U});
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 1U});
        port.push(
            LegacyBattlePreFrameCall::configure_group_a_actor,
            {
                .publish_secondary_actor_code = true,
                .secondary_actor_code = 124U,
            }
        );
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameStatus::opponent_workspace_typed_stop &&
                result.port_calls == 4U &&
                final_actor.action_execution_active == 0U &&
                final_actor.published_actor_code == 1U &&
                final_actor.source_actor_code == 0xFFFFFFFFU &&
                port.battle_terminal_latch() == 1U &&
                action.opponent_workspace[10U] == 1U,
            "final configure secondary rewrite stops after teardown scalar prefix but before terminal clear"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 0xFFFFFFFFU;
        final_actor.source_actor_code = 0xFFFFFFFFU;
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                action.opponent_workspace[1U] == 1U &&
                port.battle_message_state() == 3U &&
                result.return_value == 0xFFFFFFFFU,
            "wrapped actor code writes the original wrapped workspace slot before missing-source completion"
        );
    }
}
