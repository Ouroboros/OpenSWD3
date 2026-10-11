#include "openswd3/battle/legacy_battle_actor_target_selection_count_increment.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_target_selection_count_increment(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::
        LegacyBattleActorTargetSelectionCountIncrementStatus;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::
        increment_legacy_battle_actor_target_selection_count;
    using openswd3::compat::u16;

    {
        constexpr std::array<u16, 8> previous_values{
            0U,
            1U,
            2U,
            0x1234U,
            0x7FFFU,
            0x8000U,
            0xFFFEU,
            0xFFFFU,
        };
        constexpr std::array<u16, 8> expected_values{
            1U,
            2U,
            3U,
            0x1235U,
            0x8000U,
            0x8001U,
            0xFFFFU,
            0U,
        };
        for (std::size_t index = 0U; index < previous_values.size(); ++index) {
            std::array<u16, 3> fields{
                0xABCDU,
                previous_values[index],
                0x5678U,
            };
            const auto result =
                increment_legacy_battle_actor_target_selection_count(
                    &fields[1U]
                );
            test.expect_true(
                result.status ==
                        LegacyBattleActorTargetSelectionCountIncrementStatus::
                            completed &&
                    result.previous_value == previous_values[index] &&
                    result.incremented_value == expected_values[index] &&
                    fields[1U] == expected_values[index] &&
                    fields[0U] == 0xABCDU && fields[2U] == 0x5678U,
                "target count increments the actual WORD with unsigned wrap and leaves adjacent fields unchanged"
            );
        }
    }

    {
        u16 count = 0xFFFFU;
        const auto first =
            increment_legacy_battle_actor_target_selection_count(&count);
        const auto second =
            increment_legacy_battle_actor_target_selection_count(&count);
        test.expect_true(
            first.previous_value == 0xFFFFU && first.incremented_value == 0U &&
                second.previous_value == 0U && second.incremented_value == 1U &&
                count == 1U,
            "repeated target-count increments read the current shared WORD"
        );
    }

    {
        u16 count = 0x1234U;
        const auto result =
            increment_legacy_battle_actor_target_selection_count(
                &count, {.count_readable = false}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        count_read_typed_stop &&
                !result.previous_value.has_value() &&
                !result.incremented_value.has_value() && count == 0x1234U,
            "target-count read fault has no observed value and preserves the field"
        );
    }

    {
        u16 count = 0x1234U;
        const auto result =
            increment_legacy_battle_actor_target_selection_count(
                &count, {.count_writable = false}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        count_write_typed_stop &&
                result.previous_value == 0x1234U &&
                !result.incremented_value.has_value() && count == 0x1234U,
            "target-count write fault retains the actual read and has no successful publication"
        );
    }

    {
        u16 count = 0xFFFFU;
        const auto result =
            increment_legacy_battle_actor_target_selection_count(
                &count, {.return_address_readable = false}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        return_address_read_typed_stop &&
                result.previous_value == 0xFFFFU &&
                result.incremented_value == 0U && count == 0U,
            "target-count return fault retains the completed wrapped WORD write"
        );
    }

    {
        const auto result =
            increment_legacy_battle_actor_target_selection_count(nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionCountIncrementStatus::
                        count_read_typed_stop &&
                !result.previous_value.has_value() &&
                !result.incremented_value.has_value(),
            "unavailable target field stops at its original access without a manufactured count"
        );
    }

    {
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        auto startup = std::make_unique<LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<
            std::array<LegacyBattleActorGroupBElementState, 8>>();
        action->group_a_action_execution[0U].target_selection_count = 0xABCDU;
        action->group_a_action_execution[2U].target_selection_count = 0xFFFFU;
        action->group_a_action_execution[2U].start_gate = 0x4567U;
        (*startup->group_b_lifecycle)[0U]
            .action_execution.target_selection_count = 0x5678U;
        (*startup->group_b_lifecycle)[3U]
            .action_execution.target_selection_count = 0x7FFFU;
        (*startup->group_b_lifecycle)[3U].action_execution.start_gate = 0x89ABU;

        const auto group_a =
            increment_legacy_battle_actor_target_selection_count(
                &action->group_a_action_execution[2U].target_selection_count
            );
        const auto group_b =
            increment_legacy_battle_actor_target_selection_count(
                &(*startup->group_b_lifecycle)[3U]
                     .action_execution.target_selection_count
            );
        action->group_a_action_execution[2U].target_selection_count = 0x1234U;
        const auto later = increment_legacy_battle_actor_target_selection_count(
            &action->group_a_action_execution[2U].target_selection_count
        );
        test.expect_true(
            group_a.previous_value == 0xFFFFU &&
                group_a.incremented_value == 0U &&
                group_b.previous_value == 0x7FFFU &&
                group_b.incremented_value == 0x8000U &&
                later.previous_value == 0x1234U &&
                later.incremented_value == 0x1235U &&
                action->group_a_action_execution[2U].target_selection_count ==
                    0x1235U &&
                (*startup->group_b_lifecycle)[3U]
                        .action_execution.target_selection_count == 0x8000U &&
                action->group_a_action_execution[0U].target_selection_count ==
                    0xABCDU &&
                (*startup->group_b_lifecycle)[0U]
                        .action_execution.target_selection_count == 0x5678U &&
                action->group_a_action_execution[2U].start_gate == 0x4567U &&
                (*startup->group_b_lifecycle)[3U].action_execution.start_gate ==
                    0x89ABU,
            "target increments borrow distinct canonical actor fields, preserve other indices and gates, and read later writes"
        );
    }
}
