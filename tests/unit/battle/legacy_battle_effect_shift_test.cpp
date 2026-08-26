#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_effect_shift.hpp"

#include <deque>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleEffectCallPort;
using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::battle::LegacyBattleEffectShiftStatus;
using openswd3::battle::advance_legacy_battle_effect_shift;
using openswd3::compat::u32;

class ShiftPort final : public LegacyBattleEffectCallPort {
public:
    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        requests.push_back(request);
        if (shrink_group_a_after_first_write &&
            request.callee_token == 0x004785C0U && write_calls++ == 0U) {
            actor_metric_state().group_a_count = 1U;
        }
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    void push_read(const u32 value, const u32 scratch) {
        LegacyBattleEffectCallReply reply{};
        reply.outputs[0] = value;
        reply.outputs[1] = scratch;
        reply.output_write_mask = 3U;
        replies.push_back(reply);
        replies.push_back({});
    }

    std::deque<LegacyBattleEffectCallReply> replies;
    std::vector<LegacyBattleEffectCallRequest> requests;
    bool shrink_group_a_after_first_write{};
    u32 write_calls{};
};

}  // namespace

void test_battle_effect_shift(openswd3::test::Context& test) {
    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.phase_word = 8U;
        state.invocation_counter = 7U;
        state.direction_mode = 0U;
        state.accumulated_step = 2U;
        state.packed_reward = 0xAABBCCDDU;
        state.actor_delta = 99;
        port.actor_metric_state().group_a_count = 2U;
        port.actor_metric_state().group_b_count = 1U;
        port.push_read(100U, 10U);
        port.push_read(200U, 20U);
        port.push_read(300U, 30U);

        const auto result = advance_legacy_battle_effect_shift(
            port, 0xAAAA0001U, 0U, 0x12345678U, 0xCAFEBABEU
        );

        test.expect_true(
            result.status == LegacyBattleEffectShiftStatus::completed &&
                result.return_value == 0U && result.phase_halved &&
                result.port_calls == 6U && result.group_a_iterations == 2U &&
                result.group_b_iterations == 1U && state.phase_word == 4U &&
                state.invocation_counter == 0U &&
                state.accumulated_step == 6U && state.actor_delta == -4 &&
                state.packed_reward == 0xAABBCCDDU,
            "positive phase halves, resets the counter, accumulates the low word, and chooses the direction-zero negative actor delta"
        );
        test.expect_true(
            port.requests.size() == 6U &&
                port.requests[0].callee_token == 0x00478600U &&
                port.requests[0].arguments[0] == 0x005029D0U &&
                port.requests[1].callee_token == 0x004785C0U &&
                port.requests[1].arguments[1] == 96U &&
                port.requests[1].arguments[2] == 10U &&
                port.requests[2].arguments[0] == 0x00505904U &&
                port.requests[3].arguments[1] == 196U &&
                port.requests[4].arguments[0] == 0x00525508U &&
                port.requests[5].arguments[1] == 296U,
            "actor shift walks group A then group B with fixed tokens and carries getter outputs across each setter"
        );
        test.expect_true(
            result.argument_value == 296U && result.scratch_value == 30U &&
                result.final_ecx == 0x12345678U,
            "actor shift preserves the entry ECX stack slot and returns the final mutable argument pair"
        );
    }

    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.phase_word = 1U;
        state.invocation_counter = 0xFFFFU;
        state.direction_mode = 1U;
        state.threshold_word = 5U;

        const auto result = advance_legacy_battle_effect_shift(
            port, 4U, 1U, 0x89ABCDEFU, 0x10203040U
        );

        test.expect_true(
            result.return_value == 1U && result.phase_halved &&
                result.port_calls == 0U && state.invocation_counter == 0U &&
                state.actor_delta == 0 && state.accumulated_step == 0U &&
                state.phase_word == 0x01A4U && state.completion_latch == 0U &&
                result.final_ecx == 0x89ABCDEFU && result.final_edx == 5U,
            "phase one halves to zero, bypasses the threshold-complete latch, restores ECX, and rearms only for exact mode one"
        );
    }

    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.threshold_word = 3U;
        state.accumulated_step = 65U;
        state.direction_mode = 0U;
        port.actor_metric_state().group_a_count = 1U;
        port.push_read(0xFFFFFFF0U, 0x55667788U);

        const auto result = advance_legacy_battle_effect_shift(
            port, 4U, 0U, 0x11223344U, 0xAABBCCDDU
        );

        test.expect_true(
            result.return_value == 1U && result.port_calls == 2U &&
                state.accumulated_step == 35U && state.actor_delta == 30 &&
                state.completion_latch == 1U &&
                result.completion_latch_published &&
                port.requests[1].arguments[1] == 14U &&
                port.requests[1].arguments[2] == 0x55667788U,
            "completion consumes at most thirty, chooses the direction-zero positive delta, applies it, then publishes the latch"
        );
    }

    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.threshold_word = 0xFFFFU;
        state.accumulated_step = 7U;
        state.direction_mode = 1U;

        const auto result = advance_legacy_battle_effect_shift(
            port, 0U, 2U, 0xAABBCCDDU, 0x12340000U
        );

        test.expect_true(
            result.return_value == 1U && state.accumulated_step == 0U &&
                state.actor_delta == -7 && state.completion_latch == 1U &&
                state.phase_word == 0U && result.final_edx == 0xFFFFFFF9U,
            "negative signed threshold forces completion work, nonzero direction negates the consumed step, and non-one mode does not rearm"
        );
    }

    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.actor_delta = 1;
        port.actor_metric_state().group_a_count = 11U;
        for (u32 index = 0U; index < 10U; ++index) {
            port.push_read(index, index + 100U);
        }

        const auto result =
            advance_legacy_battle_effect_shift(port, 0U, 0U, 0U, 0U);

        test.expect_true(
            result.status ==
                    LegacyBattleEffectShiftStatus::group_a_actor_typed_stop &&
                result.return_value == 0U && result.group_a_iterations == 10U &&
                result.group_b_iterations == 0U && result.port_calls == 20U &&
                port.requests.back().arguments[0] == 0x0051D2A4U,
            "group A stops at the eleventh real actor dereference after preserving the first ten getter and setter effects"
        );
    }

    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.actor_delta = 2;
        port.actor_metric_state().group_a_count = 3U;
        port.shrink_group_a_after_first_write = true;
        port.replies.push_back({});
        port.replies.push_back({});

        const auto result = advance_legacy_battle_effect_shift(
            port, 0xFFFFFFFEU, 0U, 0xDEADBEEFU, 0x01020304U
        );

        test.expect_true(
            result.status == LegacyBattleEffectShiftStatus::completed &&
                result.return_value == 0U && result.group_a_iterations == 1U &&
                result.port_calls == 2U && result.argument_value == 0U &&
                result.scratch_value == 0xDEADBEEFU &&
                port.requests[1].arguments[1] == 0U &&
                port.requests[1].arguments[2] == 0xDEADBEEFU,
            "actor shift reloads the dynamic count after the setter, preserves unwritten getter outputs, and wraps argument addition"
        );
    }

    {
        ShiftPort port;
        auto& state = port.effect_shift_state();
        state.actor_delta = -1;
        port.actor_metric_state().group_b_count = 9U;
        for (u32 index = 0U; index < 8U; ++index) {
            port.push_read(index, index);
        }

        const auto result =
            advance_legacy_battle_effect_shift(port, 0U, 0U, 0U, 0U);

        test.expect_true(
            result.status ==
                    LegacyBattleEffectShiftStatus::group_b_actor_typed_stop &&
                result.group_a_iterations == 0U &&
                result.group_b_iterations == 8U && result.port_calls == 16U,
            "group B stops at its ninth real actor dereference after the eight physical actors"
        );
    }
}
