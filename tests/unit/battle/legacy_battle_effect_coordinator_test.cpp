#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::battle::LegacyBattleEffectCoordinatorState;
using openswd3::battle::LegacyBattleEffectCoordinatorStatus;
using openswd3::battle::LegacyBattleEffectFrameState;
using openswd3::battle::LegacyBattleGroupEffectFrameState;
using openswd3::compat::u32;

constexpr u32 kFeedback = 0x0047F150U;

class EffectCoordinatorPort final
    : public openswd3::battle::LegacyBattleEffectCallPort {
public:
    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        calls.push_back(request);
        auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        if (request.callee_token == kFeedback) {
            return {.eax = feedback_return};
        }
        return {};
    }

    void push(const u32 callee, const LegacyBattleEffectCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleEffectCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    std::vector<LegacyBattleEffectCallRequest> calls;
    std::map<u32, std::deque<LegacyBattleEffectCallReply>> replies;
    u32 feedback_return{};
};

void seed_completed_records(LegacyBattleEffectCoordinatorState& state) {
    for (auto& record : state.primary) {
        record.complete = 1U;
    }
}

[[nodiscard]] openswd3::battle::LegacyBattleEffectCoordinatorResult
run(LegacyBattleEffectCoordinatorState& state,
    EffectCoordinatorPort& port,
    openswd3::rendering::LegacyFramebuffer& framebuffer,
    const u32 ui_state = 0x8000U,
    const u32 focus_actor = 0U) {
    return openswd3::battle::advance_legacy_battle_effect_coordinator(
        state, port, framebuffer, ui_state, focus_actor
    );
}

}  // namespace

void test_battle_effect_coordinator(openswd3::test::Context& test) {
    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        const auto first = run(state, port, framebuffer, 0U);
        const auto second = run(state, port, framebuffer, 0x8001U);
        test.expect_true(
            first.status == LegacyBattleEffectCoordinatorStatus::completed &&
                first.return_value == 0U && second.return_value == 0U &&
                port.calls.empty(),
            "effect coordinator requires UI bit fifteen and rejects active low bit before any actor access"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        const auto* single_primary =
            &static_cast<LegacyBattleEffectFrameState&>(state).primary;
        const auto* group_primary =
            &static_cast<LegacyBattleGroupEffectFrameState&>(state).primary;
        EffectCoordinatorPort port;
        state.primary[17].complete = 1U;
        const auto single =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 1U, 0U, 0U, 17U
            );
        state.primary[17].complete = 1U;
        const auto group =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 1U, 0U, 0U, 17U, 0U
            );
        const auto stopped =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 1U, 0U, 0U, 18U
            );
        test.expect_true(
            single_primary == group_primary && single.return_value == 1U &&
                group.return_value == 1U &&
                stopped.status ==
                    openswd3::battle::LegacyBattleEffectFrameStatus::
                        slot_index_typed_stop,
            "single and group effects share one physical eighteen-record workspace and stop on slot eighteen"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        port.actor_metric_state().priority_actor_index = 18U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.status ==
                    LegacyBattleEffectCoordinatorStatus::
                        current_group_a_actor_typed_stop &&
                result.actor_query_calls == 0U && port.calls.empty(),
            "current group-A overflow stops at the first actor query"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        metrics.group_a_mode = 0U;
        seed_completed_records(state);
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.status == LegacyBattleEffectCoordinatorStatus::completed &&
                result.return_value == 1U && result.effect_frame_calls == 1U &&
                result.group_effect_frame_calls == 0U &&
                result.actor_query_calls == 2U &&
                state.processed_actor_slots[0] == 0U &&
                std::ranges::all_of(
                    state.primary,
                    [](const auto& record) { return record.complete == 0U; }
                ),
            "current group-A single-target group-B path queries twice composes the single effect and clears all eighteen records"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        metrics.group_a_mode = 1U;
        seed_completed_records(state);
        state.group_a_effect_mode = 1U;
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.return_value == 1U &&
                result.group_effect_frame_calls == 1U &&
                result.effect_frame_calls == 0U &&
                state.processed_actor_slots[0] == 0U,
            "current group-A group-effect path targets group A and composes the closed group helper"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        metrics.group_a_mode = 0U;
        metrics.group_b_count = 1U;
        seed_completed_records(state);
        state.group_a_global_gate = 1U;
        state.group_a_effect_mode = 0U;
        state.scan_limit = 1U;
        state.required_completion_count = 1U;
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.return_value == 1U && result.effect_frame_calls == 1U &&
                result.group_b_iterations == 1U &&
                result.actor_status_calls == 1U &&
                state.completed_count == 0U && state.scan_limit == 1U &&
                std::ranges::all_of(
                    state.feedback_primary,
                    [](const u32 value) { return value == 0U; }
                ),
            "group-A staged single effects drain one group-B actor and clear the eighteen-slot feedback arrays"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        metrics.group_a_mode = 0U;
        metrics.group_b_count = 9U;
        seed_completed_records(state);
        state.group_a_global_gate = 1U;
        state.group_a_effect_mode = 1U;
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.status ==
                    LegacyBattleEffectCoordinatorStatus::
                        group_b_actor_typed_stop &&
                result.group_effect_frame_calls == 1U &&
                result.group_b_iterations == 8U &&
                result.actor_status_calls == 8U && state.completed_count == 8U,
            "group-wide group-A path preserves eight group-B prefixes and stops at the ninth actor access"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_b_mode = 0U;
        seed_completed_records(state);
        port.feedback_return = 1U;
        const auto result = run(state, port, framebuffer, 0x8000U, 8U);
        test.expect_true(
            result.return_value == 1U && result.effect_frame_calls == 1U &&
                result.actor_query_calls == 2U &&
                (state.selected_actor_pair & 0xFFFFU) == 0U &&
                result.framebuffer_fill_calls == 1U &&
                state.framebuffer_dirty_latch == 1U &&
                port.actor_publication_state().slots[0] == 0U &&
                port.count(0x0046F6E0U) == 1U && port.count(0x0045D690U) == 1U,
            "current group-B single-target group-A path preserves copy and pair-finalization order"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_b_mode = 1U;
        seed_completed_records(state);
        port.feedback_return = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.return_value == 1U && result.effect_frame_calls == 1U &&
                result.framebuffer_fill_calls == 1U &&
                state.group_b_feedback_actor == 0U &&
                state.framebuffer_dirty_latch == 0U &&
                port.count(0x0046F6E0U) == 0U,
            "current group-B single-target group-B feedback fills without publishing the group-A dirty latch"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_b_mode = 1U;
        seed_completed_records(state);
        state.group_b_effect_mode = 1U;
        port.feedback_return = 1U;
        state.selected_actor_pair = 0xAAAABBBBU;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.return_value == 1U &&
                result.group_effect_frame_calls == 1U &&
                result.effect_frame_calls == 0U &&
                state.selected_actor_pair == 0xAAAA0000U &&
                port.count(0x0046F6E0U) == 1U && port.count(0x00472C70U) == 0U,
            "current group-B group effect always targets group A regardless of the single-effect side mode"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_a_count = 1U;
        seed_completed_records(state);
        state.group_b_global_gate = 1U;
        state.group_b_effect_mode = 1U;
        state.completion_target_count = 1U;
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.return_value == 1U &&
                result.group_effect_frame_calls == 1U &&
                result.group_a_iterations == 1U &&
                result.actor_status_calls == 1U && state.completed_count == 0U,
            "group-B group-wide mode drains one eligible group-A actor and resets the exact completion counter"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_a_count = 1U;
        seed_completed_records(state);
        state.group_b_global_gate = 1U;
        state.group_b_effect_mode = 0U;
        state.scan_limit = 1U;
        state.completion_target_count = 1U;
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.return_value == 1U && result.effect_frame_calls == 1U &&
                result.group_a_iterations == 1U && state.scan_limit == 1U &&
                state.completed_count == 0U,
            "group-B staged single mode scans group A with the shared low-word limit and exact completion latch"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_a_count = 10U;
        seed_completed_records(state);
        state.group_b_global_gate = 1U;
        state.scan_limit = 11U;
        state.completion_target_count = 99U;
        state.primary_suppression = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.status ==
                    LegacyBattleEffectCoordinatorStatus::
                        group_a_actor_typed_stop &&
                result.group_a_iterations == 10U &&
                result.effect_frame_calls == 10U &&
                state.completed_count == 10U,
            "group-B staged group-A scan preserves ten successful prefixes and stops on the eleventh actor access"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        metrics.group_a_mode = 0U;
        seed_completed_records(state);
        port.feedback_return = 1U;
        const auto result = run(state, port, framebuffer);
        test.expect_true(
            result.status == LegacyBattleEffectCoordinatorStatus::completed &&
                result.return_value == 1U &&
                result.framebuffer_fill_calls == 1U &&
                state.group_a_render_count == 1U &&
                state.framebuffer_dirty_latch == 1U &&
                framebuffer.physical_pixels().front() == 0xFFFFU &&
                framebuffer.physical_pixels().back() == 0xFFFFU,
            "successful feedback fills the complete physical framebuffer with all-ones pixels"
        );
    }
}
