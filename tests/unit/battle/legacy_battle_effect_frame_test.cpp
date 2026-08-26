#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::compat::u32;

class EffectPort final : public openswd3::battle::LegacyBattleEffectCallPort {
public:
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

[[nodiscard]] LegacyBattleEffectCallReply
pair_reply(const u32 first, const u32 second, const u32 eax = 0U) {
    LegacyBattleEffectCallReply reply{.eax = eax};
    reply.outputs[0] = first;
    reply.outputs[1] = second;
    return reply;
}

[[nodiscard]] LegacyBattleEffectCallReply resource_reply(
    const u32 owner,
    const u32 value,
    const u32 width,
    const u32 height,
    const u32 data
) {
    LegacyBattleEffectCallReply reply{.eax = owner};
    reply.outputs[0] = value;
    reply.outputs[1] = width;
    reply.outputs[2] = height;
    reply.outputs[3] = data;
    return reply;
}

[[nodiscard]] bool has_argument(
    const EffectPort& port,
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

}  // namespace

void test_battle_effect_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleEffectFrameState;
    using openswd3::battle::LegacyBattleEffectFrameStatus;

    {
        LegacyBattleEffectFrameState state;
        EffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 18U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleEffectFrameStatus::slot_index_typed_stop &&
                result.port_calls == 0U,
            "effect frame stops at first primary record access"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.alternate[0].source_value = 9U;
        state.alternate_active[0] = 1U;
        EffectPort port;
        port.push(0x004321E0U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 3U, 0x1000U, 0U, 0x77U, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                state.primary[0].source_value == 0x77U &&
                state.alternate[0].source_value == 0U &&
                state.alternate_active[0] == 0U &&
                port.count(0x00431760U) == 0U,
            "primary initialization failure clears alternate record and returns one"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].lookup_key_a = 2U;
        state.primary[0].lookup_key_b = 3U;
        EffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0U, 0U, 0U, 0U, 0U));
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleEffectFrameStatus::resource_owner_typed_stop &&
                port.count(0x00478400U) == 0U,
            "primary resource stops at first owner dereference"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        EffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(
            0x00431760U, resource_reply(0x1111U, 0x2222U, 100U, 20U, 0x3333U)
        );
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleEffectFrameStatus::argument_object_typed_stop &&
                state.current_resource_value_token == 0x2222U &&
                port.count(0x00478400U) == 0U,
            "argument object stops only after primary resource owner fields publish"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        auto& record = state.primary[0];
        record.base_offset = 20U;
        record.base_y_offset = 10U;
        record.render_flags = 0x12340000U;
        record.lookup_key_a = 2U;
        record.lookup_key_b = 3U;
        record.pan_value = 0x44U;
        state.sample_handle_value = 0x88U;
        EffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(
            0x00431760U,
            resource_reply(0x11110000U, 0x2222U, 200U, 30U, 0x3333U)
        );
        port.push(0x00478400U, pair_reply(0U, 0U));
        port.push(0x004783B0U, pair_reply(100U, 200U));
        port.push(0x00485610U, {.eax = 0xAAAA0000U, .ecx = 0xBBBB0000U});
        port.push(0x00481FD0U, pair_reply(10U, 20U));
        port.push(0x00483840U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 1U, 0xABCD1000U, 0U, 0x55U, 0U
            );
        test.expect_true(
            result.status == LegacyBattleEffectFrameStatus::completed &&
                result.return_value == 0U && record.pan_value == 0U &&
                state.shared_x == 10 && state.shared_y == 20 &&
                state.current_resource_value_token == 0x2222U &&
                has_argument(port, 0x00485650U, 0U, 0xAAAA0044U) &&
                has_argument(port, 0x00485650U, 1U, 0xFFFFFFF0U) &&
                has_argument(port, 0x004170E0U, 4U, 0x12340001U) &&
                port.count(0x004885A0U) == 2U,
            "primary setup preserves pan return high word and mirrored render parity"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.animation_mode = 1U;
        state.animation_counter[0] = 999U;
        state.sample_handle_value = 4U;
        EffectPort port;
        LegacyBattleEffectCallReply mode{.eax = 1U};
        mode.outputs[0] = 5U;
        port.push(0x00483840U, mode);
        port.push(0x004783B0U, pair_reply(120U, 240U));
        port.push(0x004783B0U, pair_reply(20U, 30U));
        port.push(0x0045D810U, {.eax = 1U});
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 2U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.animation_counter[0] == 1001U &&
                state.shared_x == 100 && state.shared_y == 240 &&
                state.primary[0].status_flags == 0U &&
                result.primary_animation_steps == 1U &&
                port.count(0x00485610U) == 1U &&
                port.count(0x00430D10U) == 1U &&
                port.count(0x00478780U) == 1U && port.count(0x00481A40U) == 1U,
            "mode-one collision jumps counter to thousand then runs common animation suffix"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.animation_mode = 1U;
        EffectPort port;
        port.push(0x00483840U, {.eax = 0U});
        port.push(0x004783B0U, pair_reply(100U, 200U));
        port.push(0x00439070U, {.eax = 7U});
        port.push(0x00439070U, {.eax = 8U});
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.animation_counter[0] == 1U &&
                port.count(0x00439070U) == 2U &&
                has_argument(port, 0x00430FF0U, 1U, 68U) &&
                has_argument(port, 0x00430FF0U, 2U, 57U) &&
                port.count(0x00485610U) == 1U,
            "alternate animation mode emits cadence-zero sound and two random particle values"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.alternate_active[0] = 1U;
        state.alternate[0].pan_value = 0x22U;
        EffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0U, 0U, 0U, 0U, 0U));
        port.push(0x00485610U, {.edx = 0xDEAD0000U});
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleEffectFrameStatus::resource_owner_typed_stop &&
                state.alternate[0].pan_value == 0U &&
                has_argument(port, 0x00485650U, 0U, 0xDEAD0022U) &&
                port.count(0x004885A0U) == 0U,
            "alternate owner dereference occurs after play and stale-EDX pan side effects"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].lookup_key_b = 0x1234U;
        state.alternate_active[0] = 1U;
        state.alternate[0].complete = 1U;
        state.alternate[0].pan_value = 0x33U;
        state.alternate[0].render_flags = 2U;
        EffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0x12340000U, 0U, 10U, 11U, 0U));
        port.push(0x00485610U, {.eax = 0xAAAA0000U});
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.alternate_active[0] == 0U &&
                has_argument(port, 0x00485610U, 0U, 0x12340033U) &&
                has_argument(port, 0x004170E0U, 4U, 3U) &&
                port.effect_shift_state().completion_latch == 1U &&
                port.effect_shift_state().phase_word == 0x01A4U &&
                port.count(0x0045BD90U) == 0U && port.count(0x004885A0U) == 2U,
            "alternate completion keeps owner high word, toggles AL parity and releases both tokens"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 0xFC0CU;
        state.primary[0].action_values = {1U, 2U, 3U, 4U, 5U, 6U, 7U};
        EffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 4U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.battle_gate == 0U &&
                state.battle_mode_latch == 1U && state.seven_value_gate == 1U &&
                state.primary[0].status_flags == 0U &&
                port.count(0x00482080U) == 3U &&
                has_argument(port, 0x0045D3E0U, 6U, 7U) &&
                has_argument(port, 0x00478780U, 0U, 4U),
            "negative packed flags publish three modes, seven values, actor and clear word"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 0x11U;
        EffectPort port;
        port.effect_shift_state().packed_reward = 0x11112222U;
        port.push(0x0047D8F0U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x77U});
        LegacyBattleEffectCallReply reward{.eax = 12000U};
        reward.outputs[0] = 0xFF80U;
        reward.outputs[1] = 2U;
        port.push(0x00481010U, reward);
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 6U, 0x22220000U, 1U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.reward_value == 9999 &&
                state.auxiliary_reward == 0xFF80U &&
                port.effect_shift_state().packed_reward == 0x00022222U &&
                state.reward_auxiliary[0] == 0xFFFFFF80U &&
                state.reward_total[0] == 9999U && state.reward_high[0] == 2U &&
                state.reward_display_total == 9999U &&
                state.pending_step[0] == 0U && port.count(0x004787D0U) == 2U &&
                port.count(0x0047D640U) == 3U && port.count(0x0047CEC0U) == 3U,
            "reward path caps base row, sign extends auxiliary and writes high-word row"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].lookup_key_b = 0x1234U;
        state.primary[0].resource_aux_value = 0xABCD0000U;
        state.pending_step[0] = 1U;
        EffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.pending_step[0] == 0U &&
                port.count(0x00459BF0U) == 0U &&
                port.count(0x0045BD90U) == 0U &&
                port.effect_shift_state().completion_latch == 1U &&
                port.effect_shift_state().phase_word == 0x01A4U &&
                state.animation_counter[0] == 0U,
            "typed pending completion preserves caller EDX high word for final gate"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].lookup_key_b = 2U;
        EffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        port.effect_shift_state().actor_delta = 1;
        port.actor_metric_state().group_a_count = 11U;
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleEffectFrameStatus::group_a_actor_typed_stop &&
                result.return_value == 0U && port.count(0x00478600U) == 10U &&
                port.count(0x004785C0U) == 10U &&
                state.primary[0].complete == 1U,
            "direct final shift propagates the eleventh group-A actor stop before effect cleanup"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.primary[0].complete = 1U;
        EffectPort port;
        port.effect_shift_state().threshold_word = 1U;
        port.effect_shift_state().actor_delta = 1;
        const auto result =
            openswd3::battle::advance_legacy_battle_effect_frame(
                state, port, 0U, 0x1000U, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.primary[0].complete == 1U,
            "final gate zero returns before successful slot cleanup"
        );
    }
}
