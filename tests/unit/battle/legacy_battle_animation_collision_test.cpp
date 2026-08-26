#include "openswd3/battle/legacy_battle_animation_collision.hpp"

#include "test.hpp"

#include <limits>

namespace {

using openswd3::battle::LegacyBattleAnimationCollisionRequest;
using openswd3::battle::LegacyBattleAnimationCollisionState;
using openswd3::battle::LegacyBattleAnimationCollisionStatus;
using openswd3::compat::u32;

[[nodiscard]] auto advance(
    LegacyBattleAnimationCollisionState& state,
    const u32 start_x,
    const u32 start_y,
    const u32 end_x,
    const u32 end_y,
    const u32 multiplier,
    const u32 slot
) {
    return openswd3::battle::advance_legacy_battle_animation_collision(
        state,
        LegacyBattleAnimationCollisionRequest{
            .start_x = start_x,
            .start_y = start_y,
            .end_x = end_x,
            .end_y = end_y,
            .step_multiplier = multiplier,
            .counter_index = slot,
        }
    );
}

}  // namespace

void test_battle_animation_collision(openswd3::test::Context& test) {
    {
        LegacyBattleAnimationCollisionState state;
        state.animation_collision_counter.fill(0x1234U);
        state.shared_x = 77;
        state.shared_y = 88;
        const auto result = advance(state, 1U, 2U, 3U, 4U, 5U, 8U);
        test.expect_true(
            result.status ==
                    LegacyBattleAnimationCollisionStatus::
                        counter_index_typed_stop &&
                result.line_raster_calls == 0U && state.shared_x == 77 &&
                state.shared_y == 88 &&
                state.animation_collision_counter[7] == 0x1234U,
            "the ninth counter stops at the original increment without changing outputs"
        );
    }

    {
        LegacyBattleAnimationCollisionState state;
        const auto result = advance(state, 10U, 20U, 30U, 40U, 0U, 0U);
        test.expect_true(
            result.status == LegacyBattleAnimationCollisionStatus::completed &&
                result.return_eax == 0U && result.return_ecx == 20U &&
                result.return_edx == 20U && result.line_raster_calls == 0U &&
                result.iteration_index == 0U && result.counter_after == 1U &&
                state.animation_collision_counter[0] == 1U &&
                state.shared_x == 10 && state.shared_y == 20,
            "zero scaled count still increments the selected word and publishes the start point"
        );
    }

    {
        LegacyBattleAnimationCollisionState state;
        state.animation_collision_counter[3] = 0xFFFFU;
        state.animation_collision_counter[4] = 9U;
        const auto result = advance(state, 7U, 8U, 20U, 30U, 5U, 3U);
        test.expect_true(
            result.return_eax == 0U && result.line_raster_calls == 0U &&
                result.counter_after == 0U &&
                state.animation_collision_counter[3] == 0U &&
                state.animation_collision_counter[4] == 9U &&
                state.shared_x == 7 && state.shared_y == 8,
            "the selected u16 counter wraps before multiplying and leaves adjacent slots untouched"
        );
    }

    {
        LegacyBattleAnimationCollisionState state;
        const auto result = advance(state, 10U, 20U, 20U, 20U, 2U, 1U);
        test.expect_true(
            result.return_eax == 0U && result.line_raster_calls == 2U &&
                result.iteration_index == 2U && result.counter_after == 1U &&
                !result.target_reached && !result.counter_cleared &&
                state.shared_x == 12 && state.shared_y == 20,
            "positive scaled count bounds line steps and preserves the counter when X does not reach the target"
        );
    }

    {
        LegacyBattleAnimationCollisionState state;
        state.animation_collision_counter[2] = 1U;
        const auto result = advance(state, 10U, 20U, 13U, 20U, 2U, 2U);
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 20U &&
                result.return_edx == 20U && result.line_raster_calls == 3U &&
                result.iteration_index == 0xFFFFFFFFU &&
                result.target_reached && result.counter_cleared &&
                result.counter_after == 0U &&
                state.animation_collision_counter[2] == 0U &&
                state.shared_x == 13 && state.shared_y == 20,
            "horizontal target completion clears only the active counter and returns one"
        );
    }

    {
        LegacyBattleAnimationCollisionState state;
        const auto result = advance(state, 5U, 10U, 5U, 20U, 10U, 0U);
        test.expect_true(
            result.return_eax == 1U && result.line_raster_calls == 1U &&
                state.shared_x == 5 && state.shared_y == 11 &&
                state.animation_collision_counter[0] == 0U,
            "completion compares only X after stepping and can finish a vertical line after one Y step"
        );
    }

    {
        LegacyBattleAnimationCollisionState state;
        const auto result =
            advance(state, 0xFFFFFFFEU, 0x80000000U, 5U, 6U, 0xFFFFFFFFU, 7U);
        test.expect_true(
            result.return_eax == 0U && result.line_raster_calls == 0U &&
                result.counter_after == 1U && state.shared_x == -2 &&
                state.shared_y ==
                    std::numeric_limits<openswd3::compat::i32>::min(),
            "negative signed scaled count skips stepping while preserving input bit patterns"
        );
    }
}
