#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::battle::LegacyBattleEffectCoordinatorState;
using openswd3::battle::LegacyBattleEffectCoordinatorStatus;
using openswd3::battle::LegacyBattleEffectFrameState;
using openswd3::battle::LegacyBattleGroupEffectFrameState;
using openswd3::compat::u8;
using openswd3::compat::u16;
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

void set_profile_word(
    openswd3::battle::LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u16 value
) {
    profile[offset] = static_cast<std::byte>(static_cast<u8>(value));
    profile[offset + 1U] = static_cast<std::byte>(static_cast<u8>(value >> 8U));
}

[[nodiscard]] openswd3::battle::LegacyBattleEffectCoordinatorResult run(
    LegacyBattleEffectCoordinatorState& state,
    EffectCoordinatorPort& port,
    openswd3::rendering::LegacyFramebuffer& framebuffer,
    const u32 ui_state = 0x8000U,
    const u32 focus_actor = 0U,
    openswd3::battle::LegacyBattleStartupState* startup_state = nullptr,
    std::array<openswd3::battle::LegacyBattleRewardScaleActorState, 8>*
        reward_state = nullptr,
    openswd3::battle::LegacyBattleActionDispatchState* action_state = nullptr,
    const openswd3::battle::LegacyBattleEffectCoordinatorRequest& request = {}
) {
    openswd3::battle::LegacyBattleStartupState fallback_startup;
    std::array<openswd3::battle::LegacyBattleRewardScaleActorState, 8>
        fallback_reward{};
    auto& startup =
        startup_state == nullptr ? fallback_startup : *startup_state;
    if (startup.group_b_lifecycle == nullptr) {
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }
    auto fallback_action =
        std::make_unique<openswd3::battle::LegacyBattleActionDispatchState>();
    auto& action = action_state == nullptr ? *fallback_action : *action_state;
    auto& reward = reward_state == nullptr ? fallback_reward : *reward_state;
    return openswd3::battle::advance_legacy_battle_effect_coordinator(
        state,
        reward,
        startup,
        port,
        framebuffer,
        {.action = &action, .startup = &startup},
        ui_state,
        focus_actor,
        request
    );
}

}  // namespace

void test_battle_effect_coordinator(openswd3::test::Context& test) {
    {
        EffectCoordinatorPort port;
        openswd3::battle::LegacyBattleRewardScaleActorState actor;
        u32 unchanged = 0xDEADBEEFU;
        const auto disabled = openswd3::battle::scale_legacy_battle_reward(
            &actor, &unchanged, port, {.actor_token = 0x00525508U}
        );
        actor.status_bits = 0x10U;
        actor.percent = 101U;
        u32 wrapped = 0xFFFFFFFFU;
        const auto scaled = openswd3::battle::scale_legacy_battle_reward(
            &actor, &wrapped, port, {.actor_token = 0x00525508U}
        );
        actor.percent = 3U;
        const auto stopped = openswd3::battle::scale_legacy_battle_reward(
            &actor, nullptr, port, {.actor_token = 0x00525508U}
        );
        const auto missing = openswd3::battle::scale_legacy_battle_reward(
            nullptr, &wrapped, port, {.actor_token = 0x00525508U}
        );
        test.expect_true(
            disabled.return_eax == 0U && unchanged == 0xDEADBEEFU &&
                disabled.port_calls == 0U && scaled.return_eax == 1U &&
                actor.percent == 1U && wrapped == 1U &&
                scaled.scaled_value == 1U && scaled.port_calls == 2U &&
                stopped.status ==
                    openswd3::battle::LegacyBattleRewardScaleStatus::
                        value_typed_stop &&
                stopped.port_calls == 2U && missing.port_calls == 0U &&
                port.count(0x00482F10U) == 2U && port.count(0x004830A0U) == 2U,
            "reward scale preserves status gating signed wrapped multiply and the post-callee value access stop"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        port.actor_metric_state().priority_actor_index = 0U;
        state.group_b_effect_mode = 1U;
        port.battle_pair_primary_value() = 200U;
        std::array<openswd3::battle::LegacyBattleRewardScaleActorState, 8>
            reward{};
        reward[0U].status_bits = 0x10U;
        reward[0U].percent = 200U;
        const auto result =
            run(state, port, framebuffer, 0x8000U, 0U, nullptr, &reward);
        test.expect_true(
            result.status == LegacyBattleEffectCoordinatorStatus::completed &&
                result.return_value == 0U && result.reward_scale_calls == 1U &&
                result.reward_scale.return_eax == 1U &&
                reward[0U].percent == 100U &&
                port.battle_pair_primary_value() == 201U &&
                port.count(0x00472C70U) == 0U &&
                port.count(0x00482F10U) == 1U && port.count(0x004830A0U) == 1U,
            "group-B incomplete group effect scales the live pair value through the typed reward path"
        );
    }

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
        const auto* single_collision_counters =
            &static_cast<LegacyBattleEffectFrameState&>(state)
                 .animation_collision_counter;
        const auto* group_collision_counters =
            &static_cast<LegacyBattleGroupEffectFrameState&>(state)
                 .animation_collision_counter;
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
            single_primary == group_primary &&
                single_collision_counters == group_collision_counters &&
                single.return_value == 1U && group.return_value == 1U &&
                stopped.status ==
                    openswd3::battle::LegacyBattleEffectFrameStatus::
                        slot_index_typed_stop,
            "single and group effects share the physical records and collision counters and stop on slot eighteen"
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
        openswd3::battle::LegacyBattleActionDispatchState action;
        action.group_a_action_execution[0U].action_target = 0U;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        openswd3::battle::LegacyBattleEffectCoordinatorRequest request;
        request.action_target_requests[0U].entry_edx = 0x11223344U;
        const auto result =
            run(state,
                port,
                framebuffer,
                0x8000U,
                0U,
                nullptr,
                nullptr,
                &action,
                request);
        test.expect_true(
            result.actor_action_target_calls >= 1U &&
                result.actor_action_targets[0U].return_eax == 0U &&
                result.actor_action_targets[0U].return_ecx == 0x005029D0U &&
                result.actor_action_targets[0U].return_edx == 0x11223344U &&
                result.actor_action_targets[0U].return_eip == 0x0045C064U &&
                result.actor_action_targets[0U].flags_known &&
                result.actor_action_targets[0U].flags.zero,
            "current Group-A actor preserves the first physical target caller"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        openswd3::battle::LegacyBattleActionDispatchState action;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        openswd3::battle::LegacyBattleEffectCoordinatorRequest request;
        request.action_target_requests[0U].access.action_target_readable =
            false;
        const auto result =
            run(state,
                port,
                framebuffer,
                0x8000U,
                0U,
                nullptr,
                nullptr,
                &action,
                request);
        test.expect_true(
            result.status ==
                    LegacyBattleEffectCoordinatorStatus::
                        actor_action_target_typed_stop &&
                result.actor_action_target_calls == 1U &&
                result.actor_action_target.return_eip == 0x004786E0U &&
                result.effect_frame_calls == 0U &&
                result.group_effect_frame_calls == 0U && port.calls.empty(),
            "effect target stop suppresses every child effect and publication suffix"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        openswd3::battle::LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        openswd3::battle::LegacyBattleEffectCoordinatorRequest request;
        request.action_target_requests[2U].entry_edx = 0x55667788U;
        const auto result =
            run(state,
                port,
                framebuffer,
                0x8000U,
                0U,
                &startup,
                nullptr,
                nullptr,
                request);
        test.expect_true(
            result.actor_action_target_calls >= 1U &&
                result.actor_action_targets[0U].return_eax == 0U &&
                result.actor_action_targets[0U].return_ecx == 0x00525508U &&
                result.actor_action_targets[0U].return_edx == 0x55667788U &&
                result.actor_action_targets[0U].return_eip == 0x0045C193U &&
                result.actor_action_targets[0U].flags_known &&
                result.actor_action_targets[0U].flags.zero,
            "current Group-B actor preserves the third physical target caller"
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
                result.actor_action_target_calls == 2U &&
                result.actor_action_targets[1U].return_eax == 0U &&
                result.actor_action_targets[1U].return_ecx == 0x005029D0U &&
                result.actor_action_targets[1U].return_edx == 0U &&
                result.actor_action_targets[1U].return_eip == 0x0045C366U &&
                result.actor_action_targets[1U].flags_known &&
                result.actor_action_targets[1U].flags.zero &&
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
                result.actor_action_target_calls == 2U &&
                result.actor_action_targets[1U].return_eax == 0U &&
                result.actor_action_targets[1U].return_ecx == 0x005029D0U &&
                result.actor_action_targets[1U].return_edx == 0U &&
                result.actor_action_targets[1U].return_eip == 0x0045C0D1U &&
                result.actor_action_targets[1U].flags_known &&
                result.actor_action_targets[1U].flags.zero &&
                state.processed_actor_slots[0] == 0U,
            "current Group-A group effect preserves its second physical target caller"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        openswd3::battle::LegacyBattleActionDispatchState action;
        action.group_a_action_execution[0U].action_target = 0U;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 8U;
        metrics.group_a_mode = 1U;
        seed_completed_records(state);
        state.group_a_effect_mode = 0U;
        state.primary_suppression = 1U;
        const auto result = run(
            state, port, framebuffer, 0x8000U, 0U, nullptr, nullptr, &action
        );
        test.expect_true(
            result.actor_action_target_calls == 2U,
            "current Group-A single effect targeting Group A performs two target queries"
        );
        test.expect_true(
            result.actor_action_targets[1U].return_eax == 0U &&
                result.actor_action_targets[1U].return_ecx == 0x005029D0U &&
                result.actor_action_targets[1U].return_edx == 0U &&
                result.actor_action_targets[1U].return_eip == 0x0045C458U &&
                result.actor_action_targets[1U].flags_known &&
                result.actor_action_targets[1U].flags.zero,
            "current Group-A single effect targeting Group A preserves the sixth physical target caller"
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
        openswd3::battle::LegacyBattleStartupState startup;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_b_mode = 0U;
        seed_completed_records(state);
        state.group_b_copy_argument_words[0U] = 1U;
        auto& profile =
            startup.party[0U].attribute_aggregation.embedded_profiles[0U];
        set_profile_word(profile, 0x04U, 100U);
        set_profile_word(profile, 0x08U, 51U);
        set_profile_word(profile, 0x10U, 7U);
        port.group_a_reward_profile_state().head.item_id = 7U;
        port.feedback_return = 1U;
        openswd3::battle::LegacyBattleEffectCoordinatorRequest request;
        request.action_target_requests[6U].entry_flags = {
            .carry = false,
            .parity = true,
            .auxiliary_carry = false,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = true,
            .overflow = true,
        };
        const auto result =
            run(state,
                port,
                framebuffer,
                0x8000U,
                8U,
                &startup,
                nullptr,
                nullptr,
                request);
        test.expect_true(
            result.return_value == 1U && result.effect_frame_calls == 1U &&
                result.actor_query_calls == 2U &&
                result.actor_action_target_calls == 2U &&
                result.actor_action_targets[1U].return_ecx == 0x00525508U &&
                result.actor_action_targets[1U].return_eip == 0x0045CA7AU &&
                (state.selected_actor_pair & 0xFFFFU) == 0U &&
                result.framebuffer_fill_calls == 1U &&
                state.framebuffer_dirty_latch == 1U &&
                port.actor_publication_state().slots[0] == 0U &&
                result.group_a_effect_reward_calls == 1U &&
                result.group_a_effect_reward.matched_profiles == 1U &&
                port.group_a_reward_profile_state().head.quantity == 12U &&
                port.group_a_reward_profile_state().head.percentage == 12U &&
                port.count(0x0046F6E0U) == 0U &&
                result.pair_transition_calls == 1U &&
                result.pair_transition.port_calls == 0U,
            "current group-B single-target group-A path directly merges eligible reward profiles before pair finalization"
        );
        test.expect_true(
            result.actor_action_targets[1U].return_eax ==
                (result.group_a_effect_reward.return_eax & 0xFFFF0000U),
            "current Group-B to Group-A caller preserves the reward EAX high word"
        );
        test.expect_true(
            result.actor_action_targets[1U].return_edx ==
                result.group_a_effect_reward.return_edx,
            "current Group-B to Group-A caller preserves reward EDX"
        );
        test.expect_true(
            result.actor_action_targets[1U].flags_known &&
                result.actor_action_targets[1U].flags.parity &&
                result.actor_action_targets[1U].flags.sign &&
                result.actor_action_targets[1U].flags.overflow &&
                !result.actor_action_targets[1U].flags.zero,
            "current Group-B to Group-A caller preserves supplied reward flags"
        );
    }

    {
        LegacyBattleEffectCoordinatorState state;
        openswd3::battle::LegacyBattleStartupState startup;
        EffectCoordinatorPort port;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        auto& metrics = port.actor_metric_state();
        metrics.priority_actor_index = 0U;
        metrics.group_b_mode = 0U;
        seed_completed_records(state);
        state.group_b_copy_argument_words[0U] = 1U;
        auto& profile =
            startup.party[0U].attribute_aggregation.embedded_profiles[0U];
        set_profile_word(profile, 0x04U, 100U);
        set_profile_word(profile, 0x08U, 51U);
        set_profile_word(profile, 0x10U, 7U);
        port.group_a_reward_profile_state().head.item_id = 1U;
        port.group_a_reward_profile_state().head.legacy_next_token =
            0x00DEAD00U;
        const auto result =
            run(state, port, framebuffer, 0x8000U, 0U, &startup);
        test.expect_true(
            result.status ==
                    LegacyBattleEffectCoordinatorStatus::
                        group_a_effect_reward_typed_stop &&
                result.group_a_effect_reward_calls == 1U &&
                result.group_a_effect_reward.status ==
                    openswd3::battle::
                        LegacyBattleGroupAEffectRewardApplicationStatus::
                            profile_node_typed_stop &&
                result.pair_transition_calls == 0U &&
                state.actor_activity_latch == 0U &&
                port.count(0x0046F6E0U) == 0U,
            "effect coordinator propagates the reclaimed reward-profile token stop before actor publication"
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
                result.actor_action_target_calls == 2U &&
                result.actor_action_targets[1U].return_eax == 0U &&
                result.actor_action_targets[1U].return_ecx == 0x00525508U &&
                result.actor_action_targets[1U].return_edx == 0U &&
                result.actor_action_targets[1U].return_eip == 0x0045CBC5U &&
                result.actor_action_targets[1U].flags_known &&
                result.actor_action_targets[1U].flags.zero &&
                result.framebuffer_fill_calls == 1U &&
                state.group_b_feedback_actor == 0U &&
                state.framebuffer_dirty_latch == 0U &&
                result.group_a_effect_reward_calls == 0U &&
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
        openswd3::battle::LegacyBattleEffectCoordinatorRequest request;
        request.action_target_requests[3U].entry_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = true,
            .overflow = false,
        };
        const auto result =
            run(state,
                port,
                framebuffer,
                0x8000U,
                0U,
                nullptr,
                nullptr,
                nullptr,
                request);
        test.expect_true(
            result.return_value == 1U &&
                result.group_effect_frame_calls == 1U &&
                result.effect_frame_calls == 0U &&
                state.selected_actor_pair == 0xAAAA0000U &&
                result.group_a_effect_reward_calls == 1U &&
                port.count(0x0046F6E0U) == 0U && port.count(0x00472C70U) == 0U,
            "current group-B group effect always targets group A regardless of the single-effect side mode"
        );
        test.expect_true(
            result.actor_action_target_calls == 2U,
            "current Group-B group effect performs two target queries"
        );
        test.expect_true(
            result.actor_action_targets[1U].return_eax ==
                    (result.group_a_effect_reward.return_eax & 0xFFFF0000U) &&
                result.actor_action_targets[1U].return_edx ==
                    result.group_a_effect_reward.return_edx &&
                result.actor_action_targets[1U].return_eip == 0x0045C1FEU,
            "current Group-B group effect preserves the fourth physical target return address and reward registers"
        );
        test.expect_true(
            result.actor_action_targets[1U].return_ecx == 0x00525508U,
            "current Group-B group effect preserves its source token"
        );
        test.expect_true(
            result.actor_action_targets[1U].flags_known &&
                result.actor_action_targets[1U].flags.carry &&
                result.actor_action_targets[1U].flags.auxiliary_carry &&
                result.actor_action_targets[1U].flags.sign &&
                !result.actor_action_targets[1U].flags.zero,
            "current Group-B group effect preserves reward-callee flags supplied at the physical caller"
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
                result.actor_status_calls == 1U &&
                result.pair_transition_calls == 1U &&
                state.completed_count == 0U,
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
                result.pair_transition_calls == 1U &&
                framebuffer.physical_pixels().front() == 0xFFFFU &&
                framebuffer.physical_pixels().back() == 0xFFFFU,
            "successful feedback fills the complete physical framebuffer with all-ones pixels"
        );
    }
}
