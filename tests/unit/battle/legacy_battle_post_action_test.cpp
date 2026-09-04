#include "openswd3/battle/legacy_battle_post_action.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::compat::u32;

class PostActionPort final : public LegacyBattleActionDispatchPort {
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

    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
};

}  // namespace

void test_battle_post_action(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleFinalActorStepState;
    using openswd3::battle::LegacyBattlePostActionState;
    using openswd3::battle::advance_legacy_battle_post_action;

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        action.selected_target_index = 2U;
        PostActionPort port;
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, 0U, 3U
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
        action.selected_target_index = 1U;
        action.group_a_count = 0;
        PostActionPort port;
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, 0U, 1U
        );
        test.expect_true(
            result.return_value == 0U && result.port_calls == 1U &&
                port.calls[0].callee_token == 0x00478850U &&
                port.calls[0].arguments[0] == 0x00528030U,
            "matching target resets group B before the unsigned zero actor-count exit"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        action.selected_target_index = 2U;
        action.group_a_count = 3;
        action.group_b_count = 3;
        PostActionPort port;
        port.push(0x004786E0U, {.eax = 0xFFFFU});
        port.push(0x004786E0U, {.eax = 2U});
        port.push(0x0047CE80U, {.eax = 0U});
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, 1U, 2U
        );
        test.expect_true(
            result.return_value == 3U && result.group_a_iterations == 3U &&
                state.selection_rebuild_pending == 1U &&
                port.count(0x004786E0U) == 2U &&
                port.count(0x00478B20U) == 1U &&
                port.count(0x00478AE0U) == 1U &&
                port.count(0x00478A70U) == 1U &&
                port.calls.back().arguments[1] == 0U,
            "first nonterminal alternate target rebuilds the matching actor relation"
        );
    }

    {
        LegacyBattlePostActionState state;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        action.selected_target_index = 1U;
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
        port.push(0x004786E0U, {.eax = 1U});
        port.push(0x0047CE80U, {.eax = 1U});
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, 0U, 1U
        );
        test.expect_true(
            result.return_value == 2U && result.group_a_iterations == 2U &&
                port.count(0x00478B20U) == 1U &&
                port.count(0x00478AE0U) == 1U &&
                port.count(0x00478710U) == 1U &&
                port.count(0x00478330U) == 0U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_ecx == 0x00505904U &&
                final_actor.group_a_availability_blocks[1U].value == 0U &&
                port.count(0x00478850U) == 2U &&
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
        action.selected_target_index = 1U;
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
        port.push(0x004786E0U, {.eax = 1U});
        port.push(0x0047CE80U, {.eax = 1U});
        const auto result = advance_legacy_battle_post_action(
            state, final_actor, action, port, 0U, 1U
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
                port.count(0x00478850U) == 1U &&
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
