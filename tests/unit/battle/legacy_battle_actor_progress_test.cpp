#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "test.hpp"

void test_battle_actor_progress(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorProgressState;
    using openswd3::battle::LegacyBattleActorProgressThresholdSyncStatus;
    using openswd3::battle::LegacyBattleActorProgressWidthStatus;
    using openswd3::battle::LegacyBattleTimingState;
    using openswd3::battle::advance_legacy_battle_actor_progress;
    using openswd3::battle::query_legacy_battle_actor_progress_width;
    using openswd3::battle::synchronize_legacy_battle_actor_progress_threshold;

    {
        LegacyBattleActorProgressState actor{.progress = 0xFACE0011U};
        LegacyBattleTimingState timing{.action_threshold = -100};
        const auto result = synchronize_legacy_battle_actor_progress_threshold(
            &actor,
            &timing,
            {
                .actor_token = 0x005029D0U,
                .entry_eax = 0xABCD1234U,
                .entry_edx = 0xA5A55A5AU,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProgressThresholdSyncStatus::completed &&
                actor.progress == 0xFACEFF9CU &&
                result.return_eax == 0xABCDFF9CU &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xA5A55A5AU &&
                result.threshold_word == 0xFF9CU &&
                result.threshold_reads == 1U && result.progress_writes == 1U,
            "actor progress threshold sync copies only the signed threshold low word and preserves register residues"
        );
    }

    {
        LegacyBattleActorProgressState actor{.progress = 0xFACE0011U};
        LegacyBattleTimingState timing{
            .action_threshold = 0x5678,
            .action_threshold_read_accessible = false,
        };
        const auto result = synchronize_legacy_battle_actor_progress_threshold(
            &actor,
            &timing,
            {
                .actor_token = 0x005029D0U,
                .entry_eax = 0xABCD1234U,
                .entry_edx = 0x11223344U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProgressThresholdSyncStatus::
                        action_threshold_read_typed_stop &&
                actor.progress == 0xFACE0011U &&
                result.return_eax == 0xABCD1234U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x11223344U &&
                result.threshold_reads == 0U && result.progress_writes == 0U,
            "actor progress threshold sync stops at the first threshold read without a suffix write"
        );
    }

    {
        LegacyBattleActorProgressState actor{
            .progress = 0xFACE0011U,
            .progress_write_accessible = false,
        };
        LegacyBattleTimingState timing{.action_threshold = 0x5678};
        const auto result = synchronize_legacy_battle_actor_progress_threshold(
            &actor,
            &timing,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0xABCD1234U,
                .entry_edx = 0x55667788U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProgressThresholdSyncStatus::
                        actor_progress_write_typed_stop &&
                actor.progress == 0xFACE0011U &&
                result.return_eax == 0xABCD5678U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x55667788U &&
                result.threshold_reads == 1U && result.progress_writes == 0U,
            "actor progress threshold sync keeps the completed threshold-read prefix at the actor write stop"
        );
    }

    {
        LegacyBattleActorProgressState actor{.progress = 0xABCD003CU};
        LegacyBattleTimingState timing;
        const auto result = query_legacy_battle_actor_progress_width(
            &actor,
            &timing,
            {.actor_token = 0x005029D0U, .entry_edx = 0xA5A55A5AU}
        );
        test.expect_true(
            result.status == LegacyBattleActorProgressWidthStatus::completed &&
                result.progress_value == 60U && result.return_eax == 4U &&
                result.return_ecx == 0x005029D0U && result.return_edx == 0U &&
                result.truncate_calls == 1U && result.x87_stack_depth == 0U,
            "actor progress width zero-extends the low word and returns the truncated signed qword"
        );
    }

    {
        LegacyBattleActorProgressState actor{.progress = 60U};
        LegacyBattleTimingState timing{.action_threshold = -900};
        const auto result = query_legacy_battle_actor_progress_width(
            &actor, &timing, {.actor_token = 0x00525508U}
        );
        test.expect_true(
            result.return_eax == 0xFFFFFFFCU &&
                result.return_edx == 0xFFFFFFFFU &&
                result.return_ecx == 0x00525508U,
            "negative action threshold preserves the signed qword high half"
        );
    }

    {
        LegacyBattleActorProgressState actor{.progress = 60U};
        LegacyBattleTimingState timing{.action_threshold = 0};
        const auto result = query_legacy_battle_actor_progress_width(
            &actor, &timing, {.actor_token = 0x00525508U}
        );
        test.expect_true(
            result.return_eax == 0U && result.return_edx == 0x80000000U &&
                result.truncate_calls == 1U && result.x87_stack_depth == 0U,
            "zero action threshold reaches x87 integer-indefinite conversion"
        );
    }

    {
        LegacyBattleActorProgressState actor{
            .progress = 60U,
            .progress_read_accessible = false,
        };
        LegacyBattleTimingState timing;
        const auto result = query_legacy_battle_actor_progress_width(
            &actor,
            &timing,
            {.actor_token = 0x00525508U, .entry_edx = 0x11223344U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProgressWidthStatus::
                        actor_progress_read_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0x11223344U &&
                result.truncate_calls == 0U && result.x87_stack_depth == 0U,
            "inaccessible actor progress stops after xor eax with entry EDX intact"
        );
    }

    {
        LegacyBattleActorProgressState actor{.progress = 0xCAFE003CU};
        LegacyBattleTimingState timing{
            .action_threshold = 900,
            .action_threshold_read_accessible = false,
        };
        const auto result = query_legacy_battle_actor_progress_width(
            &actor,
            &timing,
            {.actor_token = 0x005029D0U, .entry_edx = 0x55667788U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorProgressWidthStatus::
                        action_threshold_read_typed_stop &&
                result.return_eax == 60U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x55667788U &&
                result.truncate_calls == 0U && result.x87_stack_depth == 1U,
            "inaccessible threshold stops after the progress fild prefix"
        );
    }

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
            .progress = 0xABCD000AU,
            .delay_mode = 0x20000000U,
            .base_speed = 400U,
        };
        const auto result =
            advance_legacy_battle_actor_progress(state, 0, 1000, 0x005029D0U);
        test.expect_true(
            result.return_eax == 0U && state.progress == 0xABCD008CU &&
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

    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorGroupBProgressStatus;
    using openswd3::battle::advance_legacy_battle_actor_group_b_progress;

    {
        LegacyBattleActorProgressState state{
            .mode_gate = 0x4000U,
            .progress = 12U,
        };
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, nullptr, 0, 10, 0x00525508U, 0xA5A55A5AU
        );
        test.expect_true(
            result.status == LegacyBattleActorGroupBProgressStatus::completed &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0xA5A55A5AU && state.progress == 12U,
            "group B progress status word exits before the resource access"
        );
    }

    {
        LegacyBattleActorProgressState state{
            .progress = 20U,
            .frame_started = 1U,
            .post_action_value = 9U,
            .transition_value = 8U,
        };
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, nullptr, 0, 20, 0x00525508U, 0x12345678U
        );
        test.expect_true(
            result.return_eax == 1U && result.return_edx == 1U &&
                state.action_complete == 1U && state.transition_value == 0U &&
                state.frame_started == 0U && state.post_action_value == 0U,
            "group B completion clears the started-frame suffix without reading the resource"
        );
    }

    {
        LegacyBattleActorProgressState state{
            .progress = 20U,
            .frame_started = 7U,
            .post_action_value = 9U,
            .transition_value = 8U,
        };
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, nullptr, 0, 20, 0x00525508U
        );
        test.expect_true(
            result.return_eax == 1U && result.return_edx == 7U &&
                state.action_complete == 1U && state.transition_value == 0U &&
                state.frame_started == 7U && state.post_action_value == 9U,
            "group B completion preserves the non-one frame state and post value"
        );
    }

    {
        LegacyBattleActorGroupBElementState element{
            .resource_token = 0U,
        };
        LegacyBattleActorProgressState state{
            .action_complete = 7U,
            .progress = 0xA5A51234U,
            .delay_mode = 0xDEADBEEFU,
        };
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, &element, 0, 0x2000, 0x00525508U, 0x12345678U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorGroupBProgressStatus::
                        resource_typed_stop &&
                result.return_eax == 0x000012EFU &&
                result.return_ecx == 0x00525508U && result.return_edx == 0U &&
                state.action_complete == 7U && state.progress == 0xA5A51234U,
            "group B progress stops at the resource word read with exact register state"
        );
    }

    {
        LegacyBattleActorGroupBElementState element{
            .resource_token = 0x73000000U,
        };
        element.resource_bytes[0x5AU] = 0x90U;
        element.resource_bytes[0x5BU] = 0x01U;
        LegacyBattleActorProgressState state{
            .progress = 0xCAFE000AU,
            .delay_mode = 0x20000040U,
        };
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, &element, 1, 1000, 0x00525508U
        );
        test.expect_true(
            result.return_eax == 0U && result.return_edx == 18U &&
                state.progress == 0xCAFE005BU && result.base_increment == 63U &&
                result.positive_adjustment == 18U &&
                result.negative_adjustment == 0U,
            "group B progress applies delay, argument boost, and positive adjustment"
        );
    }

    {
        LegacyBattleActorGroupBElementState element{
            .resource_token = 0x73000000U,
        };
        element.resource_bytes[0x5AU] = 0x90U;
        element.resource_bytes[0x5BU] = 0x01U;
        LegacyBattleActorProgressState state{
            .progress = 0xBEEF000AU,
            .delay_mode = 0x88000000U,
        };
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, &element, 0, 1000, 0x00525508U
        );
        test.expect_true(
            result.return_edx == 10U && state.progress == 0xBEEF0046U &&
                result.base_increment == 100U &&
                result.positive_adjustment == 0U &&
                result.negative_adjustment == 40U,
            "group B progress combines the thirty and ten percent penalties"
        );
    }

    {
        LegacyBattleActorGroupBElementState element{
            .resource_token = 0x73000000U,
        };
        element.resource_bytes[0x5AU] = 4U;
        LegacyBattleActorProgressState state{};
        const auto result = advance_legacy_battle_actor_group_b_progress(
            state, &element, 0, 1000, 0x00525508U, 0xA5A55A5AU
        );
        test.expect_true(
            result.return_edx == 0x73000000U && state.progress == 1U,
            "group B progress returns the stale resource token when no arithmetic overwrites EDX"
        );
    }
}
