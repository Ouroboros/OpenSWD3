#include "openswd3/battle/legacy_battle_post_action.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::compat::u32;

class PostActionPort final
    : public LegacyBattleActionDispatchPort,
      public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found == replies.end() || found->second.empty()) {
            return {};
        }
        const auto reply = found->second.front();
        found->second.pop_front();
        return reply;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(
            std::ranges::count_if(calls, [callee](const auto& request) {
                return request.callee_token == callee;
            })
        );
    }

    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        last_random_bound = bound;
        ++random_calls;
        return 0U;
    }

    u32 last_random_bound{};
    u32 random_calls{};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
};

[[nodiscard]] openswd3::battle::LegacyBattleActionDispatchResult
advance_legacy_battle_post_action(
    openswd3::battle::LegacyBattlePostActionState& state,
    openswd3::battle::LegacyBattleFinalActorStepState& final_actor,
    openswd3::battle::LegacyBattleActionDispatchState& action,
    PostActionPort& port,
    openswd3::battle::LegacyBattleStartupState* const startup,
    const u32 source_group_a_index,
    const u32 target_group_b_index,
    const openswd3::battle::LegacyBattleActorActionTargetRequest&
        action_target_request = {},
    const openswd3::battle::LegacyBattleActorActionModeRequest&
        action_mode_request = {},
    const openswd3::battle::LegacyBattleActorGateDecayCallRequests&
        gate_decay_requests = {}
) {
    startup->group_b_lifecycle = std::make_shared<std::array<
        openswd3::battle::LegacyBattleActorGroupBElementState,
        openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    return openswd3::battle::advance_legacy_battle_post_action(
        state,
        final_actor,
        action,
        port,
        port,
        startup,
        {},
        source_group_a_index,
        target_group_b_index,
        action_target_request,
        action_mode_request,
        0U,
        {},
        0U,
        gate_decay_requests
    );
}

}  // namespace

void test_battle_post_action(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleFinalActorStepState;
    using openswd3::battle::LegacyBattlePostActionState;
    using openswd3::battle::LegacyBattleStartupState;

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 2U;
        action.group_a_action_execution[0U].action_target = 2U;
        PostActionPort port;
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 0U, 3U
        );
        test.expect_true(
            result.return_value == 2U && result.port_calls == 0U,
            "complete target mismatch returns the zero-extended shared word"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        action.selected_target_index = 1U;
        action.group_a_count = 2;
        PostActionPort port;
        openswd3::battle::LegacyBattleActorRuntimeResetCallRequests requests;
        requests.count = 1U;
        requests.requests[0U].stop_before_access = 0U;
        const auto result = openswd3::battle::advance_legacy_battle_post_action(
            state, final_actor, action, port, port, &startup, requests, 0U, 1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionDispatchStatus::
                        actor_runtime_reset_typed_stop &&
                result.actor_runtime_reset.calls == 1U &&
                result.actor_runtime_reset.call_addresses[0U] == 0x0045AE1DU &&
                result.actor_runtime_reset.last.status ==
                    openswd3::battle::LegacyBattleActorRuntimeResetStatus::
                        stack_write_typed_stop &&
                result.group_a_iterations == 0U &&
                result.actor_action_target_calls == 0U && port.calls.empty(),
            "post-action runtime-reset stop suppresses group scan and relation cleanup suffixes"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 1U;
        action.group_a_action_execution[0U].action_target = 1U;
        action.group_a_count = 0;
        PostActionPort port;
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 0U, 1U
        );
        test.expect_true(
            result.return_value == 0U && result.port_calls == 0U &&
                result.actor_runtime_reset.calls == 1U &&
                result.actor_runtime_reset.call_addresses[0U] == 0x0045AE1DU &&
                result.actor_runtime_reset.actor_tokens[0U] == 0x00528030U,
            "matching target resets group B before the unsigned zero actor-count exit"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 1U;
        action.group_a_count = 2;
        action.group_a_action_execution[1U].action_target = 1U;
        PostActionPort port;
        port.push(0x00478850U, {.eax = 0xAABBCCDDU, .edx = 0x11223344U});
        openswd3::battle::LegacyBattleActorActionTargetRequest request;
        request.access.action_target_readable = false;
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 0U, 1U, request
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionDispatchStatus::
                        actor_action_target_typed_stop &&
                result.actor_action_target_calls == 1U &&
                result.actor_action_target.return_eax == 1U &&
                result.actor_action_target.return_ecx == 0x00505904U &&
                result.actor_action_target.return_edx == 0x00528660U &&
                result.actor_action_target.return_eip == 0x004786E0U &&
                result.actor_action_target.action_target_reads == 0U &&
                result.port_calls == 0U &&
                result.actor_runtime_reset.calls == 1U &&
                port.count(0x00478B20U) == 0U,
            "post-action target stop preserves the initial reset and suppresses relation cleanup"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 2U;
        action.group_a_action_execution[1U].action_target = 2U;
        action.group_a_action_execution[2U].action_target = 2U;
        action.group_a_count = 3;
        action.group_b_count = 3;
        PostActionPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        openswd3::battle::LegacyBattleActorGateDecayCallRequests requests;
        requests.count = 1U;
        requests.calls[0U].access.start_gate_readable = false;
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 1U, 2U, {}, {}, requests
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionDispatchStatus::
                        actor_gate_decay_typed_stop &&
                result.actor_gate_decay.calls == 1U &&
                result.actor_gate_decay.call_addresses[0U] == 0x0045AEDFU &&
                result.actor_gate_decay.last.status ==
                    openswd3::battle::LegacyBattleActorGateDecayStatus::
                        start_gate_read_typed_stop &&
                action.group_a_action_execution[2U].action_target == 0xFFFFU &&
                result.actor_target_selection.calls == 0U &&
                state.selection_rebuild_pending == 0U,
            "post-action gate-decay read stop preserves actor clearing and suppresses relation publication"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 2U;
        action.group_a_action_execution[1U].action_target = 2U;
        action.group_a_action_execution[2U].action_target = 2U;
        action.group_a_count = 3;
        action.group_b_count = 3;
        PostActionPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 1U, 2U
        );
        test.expect_true(
            result.return_value == 3U && result.group_a_iterations == 3U &&
                state.selection_rebuild_pending == 1U &&
                result.actor_action_target_calls == 2U &&
                result.actor_action_target.return_eax == 2U &&
                result.actor_action_target.return_ecx == 0x00508838U &&
                result.actor_action_target.return_edx == 0x0052B188U &&
                result.actor_action_target.return_eip == 0x0045AE51U &&
                result.actor_action_target.flags_known &&
                !result.actor_action_target.flags.zero &&
                action.group_a_action_execution[2U].action_target == 0U &&
                port.count(0x004786E0U) == 0U &&
                port.count(0x00478B20U) == 1U &&
                port.count(0x00478AE0U) == 0U &&
                result.actor_gate_decay.calls == 1U &&
                result.actor_gate_decay.call_addresses[0U] == 0x0045AEDFU &&
                result.actor_gate_decay.return_addresses[0U] == 0x0045AEE4U &&
                result.actor_gate_decay.actor_tokens[0U] == 0x0052AB58U &&
                result.actor_gate_decay.last.returned &&
                result.actor_target_selection.calls == 1U &&
                result.actor_target_selection.call_addresses[0U] ==
                    0x0045AF7DU &&
                result.actor_target_selection.argument_values[0U] == 0U &&
                port.calls.back().arguments[1] == 0U,
            "post-action preserves its physical target query and rebuilds the canonical actor relation"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 1U;
        action.group_a_action_execution[0U].action_target = 1U;
        action.group_a_action_execution[1U].action_target = 1U;
        action.group_a_count = 2;
        action.group_b_count = 2;
        action.packed_actor_counter = 1U;
        final_actor.actor_order.fill(7U);
        final_actor.secondary_actor_code = 8U;
        final_actor.queued_actor_code = 9U;
        final_actor.active_actor_code = 10U;
        state.selection_workspace.fill(0xFFFFFFFFU);
        state.published_target_token = 0x1234U;
        PostActionPort port;
        port.push(0x0047CE80U, {.eax = 1U});
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 0U, 1U
        );
        test.expect_true(
            result.return_value == 2U && result.group_a_iterations == 2U &&
                port.count(0x00478B20U) == 1U &&
                port.count(0x00478AE0U) == 0U &&
                result.actor_gate_decay.calls == 1U &&
                result.actor_gate_decay.call_addresses[0U] == 0x0045AF75U &&
                result.actor_gate_decay.return_addresses[0U] == 0x0045AF7AU &&
                result.actor_gate_decay.actor_tokens[0U] == 0x00528030U &&
                result.actor_gate_decay.last.returned &&
                port.count(0x00478710U) == 0U &&
                result.actor_action_mode_calls == 1U &&
                result.actor_action_mode.return_eip == 0x0045AEECU &&
                action.group_a_action_execution[1U].action_kind == 0U &&
                port.count(0x00478330U) == 0U &&
                result.actor_runtime_reset.calls == 2U &&
                result.actor_runtime_reset.call_addresses[1U] == 0x0045AEF6U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_ecx == 0x00505904U &&
                final_actor.group_a_availability_blocks[1U].value == 0U &&
                action.group_a_action_execution[1U].action_target == 0xFFFFU &&
                port.count(0x00478850U) == 0U &&
                std::ranges::all_of(
                    final_actor.actor_order,
                    [](const u32 value) { return value == 0U; }
                ) &&
                final_actor.secondary_actor_code == 0U &&
                final_actor.queued_actor_code == 0U &&
                final_actor.active_actor_code == 0xFFFFFFFFU &&
                state.published_target_token == 0U &&
                std::ranges::all_of(
                    state.selection_workspace,
                    [](const u32 value) { return value == 0U; }
                ),
            "all terminal alternates and packed completion clear both fixed workspaces"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        action.selected_target_index = 1U;
        action.group_a_action_execution[0U].action_target = 1U;
        action.group_a_action_execution[1U].action_target = 1U;
        action.group_a_count = 2;
        action.group_b_count = 2;
        action.packed_actor_counter = 1U;
        final_actor.actor_order.fill(7U);
        final_actor.secondary_actor_code = 8U;
        final_actor.queued_actor_code = 9U;
        final_actor.active_actor_code = 10U;
        final_actor.group_a_availability_blocks[1U].value = 0xAABBCCDDU;
        final_actor.group_a_availability_blocks[1U].write_accessible = false;
        state.selection_workspace.fill(0xFFFFFFFFU);
        state.published_target_token = 0x1234U;
        PostActionPort port;
        port.push(0x0047CE80U, {.eax = 1U});
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, &startup, 0U, 1U
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_value == 0U &&
                result.actor_availability_block.return_ecx == 0x00505904U &&
                final_actor.group_a_availability_blocks[1U].value ==
                    0xAABBCCDDU &&
                result.actor_runtime_reset.calls == 1U &&
                port.count(0x00478850U) == 0U &&
                final_actor.actor_order[0U] == 7U &&
                final_actor.secondary_actor_code == 8U &&
                final_actor.queued_actor_code == 9U &&
                final_actor.active_actor_code == 10U &&
                state.published_target_token == 0x1234U &&
                state.selection_workspace[0U] == 0xFFFFFFFFU,
            "terminal typed write stop preserves the reached calls and suppresses every cleanup suffix"
        );
    }
}
