#include "openswd3/battle/legacy_battle_group_a_action_execution.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "test.hpp"

#include <deque>
#include <map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::compat::u32;

struct ExecutionPort final : LegacyBattleActionDispatchPort {
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        requests.push_back(request);
        auto found = replies.find(request.callee_token);
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
        std::size_t value = 0U;
        for (const auto& request : requests) {
            if (request.callee_token == callee) {
                ++value;
            }
        }
        return value;
    }

    std::vector<LegacyBattleActionCallRequest> requests;
    std::map<u32, std::deque<LegacyBattleActionCallReply>> replies;
};

}  // namespace

void test_battle_group_a_action_execution(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorProgressState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionStatus;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationState;
    using openswd3::battle::advance_legacy_battle_group_a_action_execution;

    constexpr u32 actor_token = 0x005029D0U;
    constexpr u32 target_token = 0x00525508U;

    {
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            nullptr,
            shared,
            dispatch,
            progress,
            item,
            0U,
            target_token,
            0U,
            0U,
            0U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0xABCDEF01U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0xABCDEF01U && port.requests.empty(),
            "group-A action execution stops at the first actor field read with entry registers intact"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.start_gate = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::completed &&
                result.return_eax == 0U && port.requests.empty(),
            "start gate returns zero before any target or record side effect"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.render_flags = 0x18U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.push(
            0x0047C950U, {.eax = 0U, .ecx = 0x11111111U, .edx = 0x22222222U}
        );
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            1U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x11111111U &&
                result.return_edx == 0x22222222U && state.early_latch == 1U &&
                port.count(0x0047C950U) == 1U,
            "precheck zero sets the early latch and returns the callee register prefix"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.completion_gate = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress{.action_complete = 1U};
        LegacyBattleGroupAItemEffectApplicationState item{.effect_flags = 1U};
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::completed &&
                result.return_eax == 1U && result.return_edx == 1U &&
                result.record_clears == 5U && dispatch.action_pending == 1U &&
                dispatch.action_runtime_flags == 0U &&
                progress.action_complete == 0U && item.effect_flags == 0U &&
                state.motion_aux_word == 1U &&
                state.target_indices[0U] == 0xFFFFFFFFU &&
                shared.completion_counter == 1U &&
                port.count(0x004831C0U) == 1U,
            "completed action clears five records shared actor fields and publishes one"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.completion_gate = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item{
            .effect_flags = 1U,
            .activation_latch = 2U,
        };
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U && item.activation_latch == 1U &&
                item.effect_flags == 1U && shared.completion_counter == 0U &&
                result.record_clears == 5U,
            "positive activation latch decrements only after the full cleanup prefix and withholds completion"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.action_flags = 0x0202U;
        state.primary_value = 0x11111111U;
        state.secondary_value = 0x22222222U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.push(0x00483B30U, {.eax = 1U});
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U && state.primary_value == 0U &&
                state.secondary_value == 0U && state.force_gate == 0U &&
                state.secondary_record.dwords[0U] == 0x11111111U &&
                state.secondary_record.dwords[2U] == 0x22222222U &&
                (state.action_flags & 0x0202U) == 0U &&
                (dispatch.action_runtime_flags & 0x4000U) == 0U,
            "secondary record handoff consumes bits two and two-hundred then clears the runtime flag on exact-one completion"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.action_flags = 0x0408U;
        state.completion_gate = 1U;
        state.color_values = {-1, 2, -3, 4, -5, 6, -7};
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 1U && result.color_calls == 1U &&
                result.record_clears == 6U && shared.color_gate == 1U &&
                port.count(0x0045D3E0U) == 1U &&
                port.requests[1U].arguments[0U] == 0x0000FFFFU,
            "color flag publishes seven signed words clears its bit and clears the selected slot before completion"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.action_flags = 1U;
        state.completion_gate = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 1U && result.target_calls == 1U &&
                port.count(0x00474FC0U) == 1U && result.record_clears == 6U &&
                shared.shared_motion_word == 0U,
            "action bit one clears the slot calls target mode one and then completes"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.copied_runtime_word = 5U;
        state.render_flags = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        port.push(0x0047F940U, {.eax = 1U});
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.return_eax == 1U && state.completion_gate == 1U &&
                port.count(0x0047F940U) == 1U &&
                port.count(0x00474FC0U) == 1U && result.target_calls == 1U,
            "render-mode one publishes completion and consumes the slot low-bit through target mode one"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.copied_runtime_word = 5U;
        state.motion_word = 0U;
        state.slot_records[0U].dwords[0x8CU / 4U] = 1U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        dispatch.action_runtime_flags = 0x8000U;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            0U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAActionExecutionStatus::
                        resource_typed_stop &&
                result.draw_calls == 0U && port.count(0x004838D0U) == 1U &&
                state.motion_word == 0U,
            "non-render-mode active motion stops at the original resource record dereference"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.action_flags = 8U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAItemEffectApplicationState item;
        ExecutionPort port;
        const auto result = advance_legacy_battle_group_a_action_execution(
            &state,
            shared,
            dispatch,
            progress,
            item,
            actor_token,
            target_token,
            10U,
            0U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                LegacyBattleGroupAActionExecutionStatus::slot_typed_stop,
            "slot ten stops at the first selected 0x98 record clear"
        );
    }
}
