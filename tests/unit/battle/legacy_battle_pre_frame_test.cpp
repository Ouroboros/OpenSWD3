#include "openswd3/battle/legacy_battle_pre_frame.hpp"

#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <map>
#include <memory>
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

class LivePreFramePort final
    : public openswd3::battle::LegacyBattlePreFramePort {
public:
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleActionDispatchState action;
    std::vector<LegacyBattlePreFrameCallRequest> calls;

    LegacyBattlePreFrameCallReply
    invoke_pre_frame(const LegacyBattlePreFrameCallRequest& request) override {
        calls.push_back(request);
        const auto actor =
            openswd3::battle::resolve_legacy_battle_actor_runtime_reset(
                {.action = &action, .startup = &startup}, request.actor_token
            );
        return openswd3::battle::invoke_legacy_battle_pre_frame_actor_call(
            actor, request
        );
    }
};

}  // namespace

void test_battle_pre_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleFinalActorStepState;
    using openswd3::battle::LegacyBattlePreFrameStatus;
    using openswd3::battle::advance_legacy_battle_pre_frame;
    using openswd3::battle::invoke_legacy_battle_pre_frame_actor_call;

    for (const bool unavailable : {false, true}) {
        auto port = std::make_unique<LivePreFramePort>();
        auto final_actor = std::make_unique<LegacyBattleFinalActorStepState>();
        port->battle_terminal_latch() = 1U;
        port->battle_message_state() = 0U;
        final_actor->active_actor_code = 8U;
        final_actor->source_actor_code = 1U;
        auto& progress = port->startup.party[0U].progress;
        progress.mode_gate = 2U;
        progress.progress = 0x11223344U;
        progress.transition_value = 0x55U;
        (*port->startup.group_a_runtime_reset)[0U].field_2670 = 0xAABBCCDDU;
        if (unavailable) {
            port->startup.group_a_runtime_reset.reset();
        }

        const auto result =
            advance_legacy_battle_pre_frame(*final_actor, port->action, *port);
        if (unavailable) {
            test.expect_true(
                result.status ==
                        LegacyBattlePreFrameStatus::actor_call_typed_stop &&
                    result.actor_call.stopped_instruction == 0x00481FC2U &&
                    port->calls.size() == 1U &&
                    final_actor->active_actor_code == 0U &&
                    final_actor->action_execution_active == 1U &&
                    port->action.opponent_workspace[10U] == 1U &&
                    progress.progress == 0x11223344U &&
                    progress.transition_value == 0x55U,
                "SDL-style actor resolution propagates the pre-frame fault after shared prefix writes"
            );
        } else {
            test.expect_true(
                result.status == LegacyBattlePreFrameStatus::completed &&
                    port->calls.size() == 2U &&
                    final_actor->active_actor_code == 0U &&
                    final_actor->secondary_actor_code == 8U &&
                    final_actor->action_execution_active == 5U &&
                    port->action.opponent_workspace[10U] == 5U &&
                    final_actor->actor_runtime_records[0U][0U] == 1U &&
                    progress.progress == 0x11220000U &&
                    progress.transition_value == 1U &&
                    (*port->startup.group_a_runtime_reset)[0U].field_2670 == 0U,
                "full pre-frame processing reaches the same live party records used by SDL"
            );
        }
    }

    {
        openswd3::battle::LegacyBattleActorProgressState progress;
        const openswd3::battle::LegacyBattleActorRuntimeResetView actor{
            .progress = &progress,
        };
        LegacyBattlePreFrameCallRequest request{
            .call = LegacyBattlePreFrameCall::query_group_a_actor,
            .actor_token = 0x005029D0U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_edx = 0xBBBBBBBBU,
        };
        progress.mode_gate = 0xAABBCC02U;
        const auto set = invoke_legacy_battle_pre_frame_actor_call(actor, request);
        progress.mode_gate = 0xAABBCCFDU;
        const auto clear = invoke_legacy_battle_pre_frame_actor_call(actor, request);
        const auto absent = invoke_legacy_battle_pre_frame_actor_call({}, request);
        test.expect_true(
            set.eax == 1U && clear.eax == 0U && !set.typed_stop &&
                set.ecx == request.actor_token && set.edx == request.entry_edx &&
                absent.typed_stop && absent.eax == 0U &&
                absent.stopped_instruction == 0x00481FC2U,
            "group-A query reads low-byte bit one after clearing EAX and preserves ECX/EDX"
        );
        request.call = LegacyBattlePreFrameCall::query_group_b_actor;
        progress.special_ready = 0x00010001U;
        progress.mode_gate = 0xFFFF2000U;
        const auto mode = invoke_legacy_battle_pre_frame_actor_call(actor, request);
        progress.mode_gate = 0x20000020U;
        const auto wrong_byte =
            invoke_legacy_battle_pre_frame_actor_call(actor, request);
        progress.special_ready = 1U;
        const auto ready = invoke_legacy_battle_pre_frame_actor_call(actor, request);
        progress.special_ready_read_accessible = false;
        const auto unreadable =
            invoke_legacy_battle_pre_frame_actor_call(actor, request);
        test.expect_true(
            mode.eax == 1U && mode.edx == 0x00010001U &&
                wrong_byte.eax == 0U && ready.eax == 1U && ready.edx == 1U &&
                unreadable.typed_stop &&
                unreadable.stopped_instruction == 0x0047CE80U &&
                unreadable.eax == request.entry_eax &&
                unreadable.edx == request.entry_edx,
            "group-B query compares the full ready dword then tests bit 0x20 in byte 0x26D1"
        );
    }

    for (u32 fault = 0U; fault < 4U; ++fault) {
        openswd3::battle::LegacyBattleActorProgressState progress;
        openswd3::battle::LegacyBattleActorRuntimeResetState residual;
        progress.progress = 0xBEEF1234U;
        progress.transition_value = 7U;
        progress.progress_read_accessible = false;
        progress.progress_write_accessible = fault != 1U;
        residual.field_2670 = 55U;
        const auto reply = invoke_legacy_battle_pre_frame_actor_call(
            {
                .residual = fault == 2U ? nullptr : &residual,
                .progress = fault == 0U ? nullptr : &progress,
            },
            {
                .call = LegacyBattlePreFrameCall::notify_group_a_actor,
                .actor_token = 0x005029D0U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_edx = 0xBBBBBBBBU,
            }
        );
        const u32 instruction = fault == 0U ? 0x0047D7D2U
            : fault == 1U ? 0x0047D7DCU : fault == 2U ? 0x0047D7E3U : 0U;
        test.expect_true(
            reply.typed_stop == (fault != 3U) &&
                reply.stopped_instruction == instruction &&
                reply.eax == 0U && reply.ecx == 0x005029D0U &&
                reply.edx == 0xBBBBBBBBU &&
                progress.transition_value == (fault == 0U ? 7U : 1U) &&
                progress.progress == (fault < 2U ? 0xBEEF1234U : 0xBEEF0000U) &&
                residual.field_2670 == (fault == 3U ? 0U : 55U),
            "notification writes transition, progress word and residual in order with exact stop prefixes"
        );
    }

    for (u32 fault = 0U; fault < 4U; ++fault) {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        port.actor_metric_state().group_b_count = 2U;
        final_actor.active_actor_code = 9U;
        final_actor.source_actor_code = 2U;
        final_actor.actor_runtime_records[1U][0U] = 77U;
        port.push(LegacyBattlePreFrameCall::query_group_a_actor, {
            .eax = fault < 2U ? 1U : 0U,
            .typed_stop = fault == 0U,
            .stopped_instruction = fault == 0U ? 0x00481FC2U : 0U,
        });
        port.push(LegacyBattlePreFrameCall::notify_group_a_actor, {
            .typed_stop = true,
            .stopped_instruction = 0x0047D7DCU,
        });
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {
            .eax = 1U,
            .typed_stop = fault == 2U,
            .stopped_instruction = fault == 2U ? 0x0047CE80U : 0U,
        });
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {
            .typed_stop = true,
            .stopped_instruction = 0x0047CE80U,
        });
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::actor_call_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_call.typed_stop &&
                final_actor.actor_runtime_records[1U][0U] == 77U &&
                final_actor.published_actor_code == 2U &&
                port.battle_terminal_latch() == 1U,
            "all four physical callback stops suppress their pre-frame suffix without rolling back earlier writes"
        );
    }

    {
        using openswd3::battle::LegacyBattlePreFrameEntryStatus;
        using openswd3::battle::run_legacy_battle_pre_frame_entry_prefix;
        u32 terminal = 7U;
        u32 active_actor = 8U;
        u32 message_state = 3U;
        auto result = run_legacy_battle_pre_frame_entry_prefix(
            terminal, active_actor, message_state
        );
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameEntryStatus::
                        returned_before_next_call &&
                result.return_eax == 7U && !result.message_read,
            "pre-frame terminal gate returns loaded non-one latch before actor and message reads"
        );
        terminal = 1U;
        active_actor = 0U;
        result = run_legacy_battle_pre_frame_entry_prefix(
            terminal, active_actor, message_state
        );
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameEntryStatus::
                        returned_before_next_call &&
                result.return_eax == 0U && !result.message_read,
            "pre-frame empty active actor returns zero before message read"
        );
        active_actor = 8U;
        result = run_legacy_battle_pre_frame_entry_prefix(
            terminal, active_actor, message_state
        );
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameEntryStatus::
                        returned_before_next_call &&
                result.return_eax == 8U && result.message_read,
            "pre-frame message three returns the active actor code"
        );
        message_state = 2U;
        result = run_legacy_battle_pre_frame_entry_prefix(
            terminal, active_actor, message_state
        );
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameEntryStatus::read_source_actor &&
                result.return_eax == 8U && result.message_read,
            "pre-frame active non-three branch stops before source actor read"
        );
    }

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
                result.return_edx == 0x33334444U,
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
            result.return_edx == 0x33334444U;
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
                port.battle_message_state() == 2U,
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
                final_actor.active_actor_code == 8U,
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
            LegacyBattlePreFrameCall::query_group_a_actor,
            {.eax = 1U, .ecx = 21U, .edx = 22U}
        );
        port.push(
            LegacyBattlePreFrameCall::notify_group_a_actor,
            {.eax = 30U, .ecx = 31U, .edx = 32U}
        );
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 9U && result.return_ecx == 0x00505904U &&
                result.return_edx == 5U &&
                result.actor_availability_block_calls == 2U &&
                final_actor.group_a_availability_blocks[1U].value == 1U &&
                final_actor.active_actor_code == 0U &&
                final_actor.secondary_actor_code == 9U &&
                final_actor.published_actor_code == 2U &&
                final_actor.auxiliary_gate == 1U &&
                final_actor.action_execution_active == 5U &&
                port.battle_message_state() == 0U &&
                action.opponent_workspace[11U] == 5U &&
                final_actor.actor_runtime_records[1U][0U] == 1U &&
                port.calls[0].call ==
                    LegacyBattlePreFrameCall::query_group_a_actor &&
                port.calls[0].actor_token == 0x00505904U &&
                port.calls[0].entry_eax == 3021U &&
                port.calls[0].entry_edx == 9U &&
                port.calls[1].call ==
                    LegacyBattlePreFrameCall::notify_group_a_actor &&
                port.calls[1].actor_token == 0x00505904U &&
                port.calls[1].entry_eax == 1007U &&
                port.calls[1].entry_edx == 3021U,
            "current group-A actor success performs both typed writes around the remaining query and notify calls"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 7U;
        final_actor.source_actor_code = 1U;
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.return_value == 1U && result.return_ecx == 0x004FFA9CU &&
                result.return_edx == 1U &&
                action.opponent_workspace[9U] == 1U &&
                final_actor.active_actor_code == 0U &&
                final_actor.secondary_actor_code == 7U,
            "actor code seven stops at the typed write after the caller prefix and before every query or notify suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 9U;
        final_actor.source_actor_code = 2U;
        final_actor.group_a_availability_blocks[1U].write_accessible = false;
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status ==
                    LegacyBattlePreFrameStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_value == 1U && result.return_ecx == 0x00505904U &&
                result.return_edx == 2U &&
                action.opponent_workspace[11U] == 1U &&
                final_actor.secondary_actor_code == 9U &&
                final_actor.active_actor_code == 0U &&
                final_actor.auxiliary_gate == 1U && port.calls.empty(),
            "valid actor typed write stop preserves the source-actor EDX and suppresses query and notify suffixes"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 1U;
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
                result.actor_availability_block_calls == 1U &&
                result.return_value == 5U && result.return_ecx == 116U &&
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
            LegacyBattlePreFrameCall::query_group_a_actor,
            {.eax = 0U, .edx = 0xAABBCCDDU}
        );
        port.push(
            LegacyBattlePreFrameCall::query_group_b_actor,
            {.eax = 1U, .edx = 0x12345678U}
        );
        port.push(
            LegacyBattlePreFrameCall::query_group_b_actor,
            {.eax = 2U, .edx = 0xFFFFFFFFU}
        );
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 0U});
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 0U &&
                result.actor_availability_block_calls == 1U &&
                result.group_b_iterations == 2U &&
                final_actor.published_actor_code == 2U &&
                action.opponent_workspace[10U] == 1U &&
                port.calls[1].actor_token == 0x00525508U &&
                port.calls[1].entry_eax == 345U &&
                port.calls[1].entry_edx == 0xAABBCCDDU &&
                port.calls[2].actor_token == 0x00525508U &&
                port.calls[2].entry_eax == 3U &&
                port.calls[2].entry_edx == 0x12345678U &&
                port.calls[3].actor_token == 0x00528030U &&
                port.calls[3].entry_eax == 3U &&
                port.calls[3].entry_edx == 0xFFFFFFFFU,
            "the source code drives the one-based group-B query before the zero-based scan and first zero publishes index plus one"
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
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.return_value == 0U && result.return_ecx == 8U &&
                result.return_edx == 8U &&
                result.actor_availability_block_calls == 2U &&
                result.group_b_iterations == 1U &&
                port.actor_metric_state().group_b_count == 1U &&
                port.battle_terminal_latch() == 0U &&
                final_actor.source_actor_code == 0xFFFFFFFFU &&
                final_actor.published_actor_code == 1U &&
                final_actor.action_execution_active == 0U &&
                final_actor.group_a_availability_blocks[0U].value == 0U &&
                action.opponent_workspace[10U] == 0U,
            "dynamic group-B count contraction enters teardown and clears the typed actor owner with the exact zero return"
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
        port.push(LegacyBattlePreFrameCall::query_group_a_actor, {.eax = 0U});
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 1U});
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                result.actor_availability_block_calls == 2U &&
                final_actor.action_execution_active == 0U &&
                final_actor.published_actor_code == 1U &&
                final_actor.source_actor_code == 0xFFFFFFFFU &&
                final_actor.group_a_availability_blocks[0U].value == 0U &&
                port.battle_terminal_latch() == 0U &&
                action.opponent_workspace[10U] == 0U,
            "zero live group-B count reaches the typed clear and completes the teardown suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        PreFramePort port;
        port.battle_terminal_latch() = 1U;
        final_actor.active_actor_code = 8U;
        final_actor.source_actor_code = 0x80000000U;
        port.push(
            LegacyBattlePreFrameCall::query_group_a_actor,
            {.eax = 0U, .edx = 0xCAFEBABEU}
        );
        port.push(LegacyBattlePreFrameCall::query_group_b_actor, {.eax = 0U});
        const auto result =
            advance_legacy_battle_pre_frame(final_actor, action, port);
        test.expect_true(
            result.status == LegacyBattlePreFrameStatus::completed &&
                port.calls.size() == 2U &&
                port.calls[1].actor_token == 0x005229E0U &&
                port.calls[1].entry_eax == 0x80000000U &&
                port.calls[1].entry_edx == 0xCAFEBABEU,
            "one-based group-B address arithmetic wraps EAX and ECX independently while preserving EDX"
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
