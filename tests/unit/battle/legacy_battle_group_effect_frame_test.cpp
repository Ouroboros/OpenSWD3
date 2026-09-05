#include "openswd3/battle/legacy_battle_group_effect_frame.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::compat::u32;

class GroupEffectPort final
    : public openswd3::battle::LegacyBattleEffectCallPort {
public:
    GroupEffectPort() {
        actor_coordinate_bindings().group_b[0U] =
            openswd3::battle::view_legacy_battle_actor_coordinates(target);
    }

    openswd3::battle::LegacyBattleActorCoordinatesState target{};

    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        return default_reply;
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

    LegacyBattleEffectCallReply default_reply{};
    std::unordered_map<u32, std::deque<LegacyBattleEffectCallReply>> replies;
    std::vector<LegacyBattleEffectCallRequest> calls;
};

[[nodiscard]] LegacyBattleEffectCallReply resource_reply(
    const u32 owner,
    const u32 value,
    const u32 width,
    const u32 height,
    const u32 edx = 0U
) {
    LegacyBattleEffectCallReply reply{.eax = owner, .edx = edx};
    reply.outputs[0] = value;
    reply.outputs[1] = width;
    reply.outputs[2] = height;
    return reply;
}

[[nodiscard]] LegacyBattleEffectCallReply
pair_reply(const u32 first, const u32 second, const u32 eax = 0U) {
    LegacyBattleEffectCallReply reply{.eax = eax};
    reply.outputs[0] = first;
    reply.outputs[1] = second;
    return reply;
}

[[nodiscard]] LegacyBattleEffectCallReply
reward_reply(const u32 reward, const u32 auxiliary = 0U, const u32 high = 0U) {
    LegacyBattleEffectCallReply reply{.eax = reward};
    reply.outputs[0] = auxiliary;
    reply.outputs[1] = high;
    return reply;
}

[[nodiscard]] bool has_argument(
    const GroupEffectPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleEffectCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

[[nodiscard]] std::vector<u32> arguments_for(
    const GroupEffectPort& port, const u32 callee, const std::size_t argument
) {
    std::vector<u32> values;
    for (const auto& call : port.calls) {
        if (call.callee_token == callee) {
            values.push_back(call.arguments[argument]);
        }
    }
    return values;
}

}  // namespace

void test_battle_group_effect_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupEffectFrameState;
    using openswd3::battle::LegacyBattleGroupEffectFrameStatus;

    {
        LegacyBattleGroupEffectFrameState state;
        state.reward_value = 7;
        GroupEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 18U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupEffectFrameStatus::slot_index_typed_stop &&
                result.port_calls == 0U && state.reward_value == 7,
            "group effect frame stops before reward reset at invalid slot"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.alternate[0].source_value = 9U;
        state.alternate_active[0] = 1U;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0x77U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U &&
                state.primary[0].source_value == 0x77U &&
                state.alternate[0].source_value == 9U &&
                state.alternate_active[0] == 1U,
            "primary initialization failure returns zero without clearing alternate"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].pan_value = 0x33U;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0U, 0U, 0U, 0U, 0xAAAA0000U));
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupEffectFrameStatus::
                        resource_owner_typed_stop &&
                state.primary[0].pan_value == 0U &&
                has_argument(port, 0x00485610U, 0U, 0xAAAA0033U) &&
                port.count(0x00478400U) == 0U,
            "primary null owner stops after sample with lookup EDX high word"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0x1000U, 0x2222U, 40U, 20U));
        port.push(0x00478400U, pair_reply(0U, 0U));
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupEffectFrameStatus::
                        argument_object_typed_stop &&
                state.current_resource_value_token == 0x2222U &&
                port.count(0x00485610U) == 1U && port.count(0x00478400U) == 1U,
            "argument object stops after sample, owner value and offsets"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        auto& record = state.primary[0];
        record.base_offset = 20U;
        record.render_flags = 0x12340000U;
        record.pan_value = 0x44U;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(
            0x00431760U, resource_reply(0x1111U, 0U, 200U, 30U, 0xAAAA0000U)
        );
        port.push(0x00478400U, pair_reply(9U, 0U));
        port.push(0x00483840U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.shared_x == 300 &&
                state.shared_y == 0 && state.rendered_primary_count == 1U &&
                port.count(0x00478470U) == 0U &&
                has_argument(port, 0x00485610U, 0U, 0xAAAA0044U) &&
                has_argument(port, 0x004170E0U, 4U, 0x12340001U) &&
                port.count(0x004885A0U) == 2U,
            "primary path requires both offsets and keeps object-mode coordinate asymmetry"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].base_offset = 10U;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0x1000U, 0U, 100U, 20U));
        port.push(0x00478400U, pair_reply(2U, 3U));
        port.push(0x00478470U, pair_reply(30U, 40U));
        LegacyBattleEffectCallReply animation{.eax = 1U};
        animation.outputs[0] = 5U;
        port.push(0x00483840U, animation);
        port.target.position_x = 155U;
        port.target.position_y = 99U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0x005029D0U, 0x00525508U, 1U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.primary[0].complete == 1U &&
                result.coordinate_query_calls == 1U &&
                port.count(0x004783B0U) == 0U,
            "collision completion publishes the primary completion state"
        );
        test.expect_true(
            result.animation_collision_calls == 1U &&
                result.animation_collision.return_eax == 1U &&
                result.animation_collision.line_raster_calls == 5U &&
                state.animation_collision_counter[0] == 0U,
            "collision uses fixed zero Y coordinates and fixed slot zero"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].pan_value = 0x77U;
        state.alternate_active[0] = 1U;
        state.alternate[0].pan_value = 0x22U;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0U, 0U, 0U, 0U));
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupEffectFrameStatus::
                        resource_owner_typed_stop &&
                state.primary[0].pan_value == 0U &&
                state.alternate[0].pan_value == 0x22U &&
                has_argument(port, 0x00485610U, 0U, 0x22U),
            "alternate sample clears primary pan but preserves alternate pan"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.alternate_active[0] = 1U;
        state.alternate[0].complete = 1U;
        state.alternate[0].pan_value = 0x55U;
        state.alternate[0].render_flags = 2U;
        GroupEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0xABCD1000U, 0x20U, 10U, 11U));
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.alternate_active[0] == 0U &&
                state.alternate[0].source_value == 0U &&
                has_argument(port, 0x00485610U, 0U, 0xABCD0055U) &&
                has_argument(port, 0x004170E0U, 4U, 3U) &&
                port.count(0x004885A0U) == 2U,
            "alternate completion toggles AL, releases zero-capable value and clears at final success"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 0x1408U;
        state.primary[0].action_values = {1U, 2U, 3U, 4U, 5U, 6U, 7U};
        GroupEffectPort port;
        port.actor_metric_state().group_b_count = 2U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 1U, 0U, 0U, 1U
            );
        const auto tokens = arguments_for(port, 0x00482080U, 0U);
        test.expect_true(
            result.return_value == 1U && result.status_iterations == 2U &&
                tokens == std::vector<u32>({0x00525508U, 0x00528030U}) &&
                result.color_initialization_calls == 1U &&
                port.battle_color_initialization_gate() == 1U &&
                port.battle_color_accumulation_state().countdown == 7 &&
                port.battle_color_accumulation_state().current_red == 1.0F &&
                port.battle_color_accumulation_state().step_red ==
                    (3.0F / 7.0F) &&
                state.primary[0].status_flags == 8U,
            "group-wide status mode follows argument mode and publishes typed color initialization"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 4U;
        state.group_a_special_mode = 1U;
        state.group_a[1].guard_ac0 = 1U;
        GroupEffectPort port;
        port.actor_metric_state().group_a_count = 2U;
        port.push(0x0047CE80U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 1U
            );
        test.expect_true(
            result.return_value == 1U && port.count(0x0047CE80U) == 1U &&
                port.count(0x00478780U) == 1U &&
                has_argument(port, 0x00478780U, 0U, 0x005029D0U) &&
                state.primary[0].status_flags == 0U,
            "group-A status publish honors both direct guard fields"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 0x11U;
        state.group_a_reward_mode = 1U;
        state.reward_summary_gate = 1U;
        GroupEffectPort port;
        port.actor_metric_state().group_a_count = 2U;
        port.push(0x0047CEA0U, {.eax = 0U});
        port.push(0x0047CEA0U, {.eax = 0U});
        port.push(0x00481010U, reward_reply(10U, 2U, 3U));
        port.push(0x00481010U, reward_reply(20U));
        port.push(0x0047CEC0U, {});
        port.push(0x0047CEC0U, {});
        port.push(0x0047CEC0U, {.eax = 0xAAAAFFFFU, .ecx = 0xBBBBFFFFU});
        port.push(0x0047CEC0U, {});
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 1U
            );
        test.expect_true(
            result.return_value == 1U && result.reward_iterations == 2U &&
                state.reward_total[0] == 10U && state.reward_total[1] == 20U &&
                state.reward_auxiliary[0] == 0U && state.reward_high[0] == 0U &&
                port.battle_pair_secondary_value() == 0U &&
                (port.effect_shift_state().packed_reward & 0xFFFF0000U) == 0U &&
                port.count(0x0047F150U) == 2U &&
                has_argument(port, 0x0047F150U, 2U, 0xBBBB0000U) &&
                has_argument(port, 0x0047F150U, 3U, 0xAAAA0000U),
            "group-A reward resets row globals before arrays and publishes per-actor summary"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 0x10U;
        GroupEffectPort port;
        port.actor_metric_state().group_b_count = 2U;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x00481010U, reward_reply(0xFFFFFFFFU, 2U, 3U));
        port.push(0x00481010U, reward_reply(0xFFFFFFFFU, 4U, 5U));
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 1U
            );
        const auto offsets = arguments_for(port, 0x0047CF00U, 1U);
        test.expect_true(
            result.return_value == 1U && result.reward_iterations == 2U &&
                offsets == std::vector<u32>({0U, 8U, 8U, 16U}) &&
                state.reward_total[0] == 0U && state.reward_total[1] == 0U &&
                port.count(0x0047F150U) == 0U,
            "group-B reward preserves cumulative row offset across actors and minus-one bases"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 1U;
        GroupEffectPort port;
        port.battle_pair_secondary_value() = 2U;
        port.effect_shift_state().packed_reward = 3U << 16U;
        port.push(0x00481A40U, reward_reply(5U));
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0x3333U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.battle_gate == 0U &&
                state.reward_display_total == 5U &&
                port.battle_pair_secondary_value() == 0U &&
                (port.effect_shift_state().packed_reward & 0xFFFF0000U) == 0U &&
                port.count(0x00478780U) == 1U && port.count(0x00481A40U) == 1U,
            "single actor reward publishes actor, clears gate and consumes stale rows"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 4U;
        state.group_a_special_mode = 1U;
        GroupEffectPort port;
        port.actor_metric_state().group_a_count = 11U;
        for (int index = 0; index < 10; ++index) {
            port.push(0x0047CE80U, {.eax = 1U});
        }
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 1U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupEffectFrameStatus::
                        group_a_actor_typed_stop &&
                result.status_iterations == 10U &&
                port.count(0x0047CE80U) == 10U,
            "eleventh group-A actor stops after ten complete actor prefixes"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].lookup_key_b = 0x1234U;
        state.rendered_primary_count = 8U;
        state.alternate[0].source_value = 9U;
        GroupEffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                port.effect_shift_state().completion_latch == 1U &&
                port.effect_shift_state().phase_word == 0x00D2U &&
                port.effect_shift_state().accumulated_step == 0x00D2U &&
                port.effect_shift_state().actor_delta == -0x00D2 &&
                port.count(0x0045BD90U) == 0U &&
                state.rendered_primary_count == 0U &&
                state.alternate[0].source_value == 0U,
            "two direct final gates restore first-call ECX, rearm to 420, then consume the second call as a direction-zero half step"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].lookup_key_b = 2U;
        state.rendered_primary_count = 8U;
        GroupEffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        port.effect_shift_state().actor_delta = 1;
        port.actor_metric_state().group_b_count = 9U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupEffectFrameStatus::
                        effect_shift_group_b_typed_stop &&
                result.return_value == 0U &&
                port.effect_shift_state().completion_latch == 1U &&
                port.count(0x00478600U) == 8U &&
                port.count(0x004785C0U) == 8U &&
                state.rendered_primary_count == 8U,
            "direct group final shift propagates the ninth group-B actor stop before group cleanup"
        );
    }

    {
        LegacyBattleGroupEffectFrameState state;
        state.primary[0].complete = 1U;
        GroupEffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        port.effect_shift_state().actor_delta = 1;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U &&
                port.effect_shift_state().completion_latch == 1U &&
                port.count(0x0045BD90U) == 0U,
            "first final gate zero returns before completion cleanup"
        );
    }
}
