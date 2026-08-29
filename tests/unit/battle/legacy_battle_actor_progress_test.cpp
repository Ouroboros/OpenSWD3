#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "test.hpp"

void test_battle_actor_progress(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorProgressState;
    using openswd3::battle::advance_legacy_battle_actor_progress;

    {
        LegacyBattleActorProgressState state{
            .mode_gate = 0x40U,
            .progress = 12U,
        };
        const auto result =
            advance_legacy_battle_actor_progress(state, 0, 10, 0x005029D0U);
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x005029D0U &&
                state.progress == 12U,
            "actor progress status bit exits without modifying the actor"
        );
    }

    {
        LegacyBattleActorProgressState state{
            .progress = 20U,
            .frame_started = 1U,
            .scene_identity = 0U,
            .post_action_value = 9U,
            .transition_value = 8U,
            .cache_x = 7U,
            .cache_y = 6U,
        };
        const auto result =
            advance_legacy_battle_actor_progress(state, 0, 20, 0x005029D0U);
        test.expect_true(
            result.return_eax == 1U && state.action_complete == 1U &&
                state.transition_value == 0U && state.frame_started == 0U &&
                state.post_action_value == 0U && state.cache_x == 0U &&
                state.cache_y == 0U && state.update_ready == 1U,
            "actor progress completion publishes all original completion fields"
        );
    }

    {
        LegacyBattleActorProgressState state{
            .progress = 10U,
            .delay_mode = 0x20000000U,
            .base_speed = 400U,
        };
        const auto result =
            advance_legacy_battle_actor_progress(state, 0, 1000, 0x005029D0U);
        test.expect_true(
            result.return_eax == 0U && state.progress == 140U &&
                state.action_complete == 0U && result.base_increment == 100U &&
                result.positive_adjustment == 30U &&
                result.negative_adjustment == 0U,
            "actor progress applies the thirty-percent positive adjustment"
        );
    }

    {
        LegacyBattleActorProgressState state{
            .progress = 10U,
            .delay_mode = 0x80000000U,
            .base_speed = 400U,
            .progress_multiplier = 200U,
        };
        const auto result =
            advance_legacy_battle_actor_progress(state, 0, 1000, 0x005029D0U);
        test.expect_true(
            state.progress == 6U && result.base_increment == 100U &&
                result.negative_adjustment == 104U,
            "actor progress preserves multiplier penalty and fixed four-point loss"
        );
    }
}
