#include "openswd3/battle/legacy_battle_actor_target_selection_count_query.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_target_selection_count_query(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorTargetSelectionCountQueryStatus;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::query_legacy_battle_actor_target_selection_count;
    using openswd3::compat::u16;

    for (const u16 value :
         std::array<u16, 7>{0U, 1U, 2U, 0x1234U, 0x7FFFU, 0x8000U, 0xFFFFU}) {
        const std::array<u16, 3> fields{0xABCDU, value, 0x5678U};
        const auto result =
            query_legacy_battle_actor_target_selection_count(&fields[1U]);
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        completed &&
                result.value == value &&
                fields == std::array<u16, 3>{0xABCDU, value, 0x5678U},
            "target count query reads the complete WORD and leaves adjacent fields unchanged"
        );

        const auto field_stop =
            query_legacy_battle_actor_target_selection_count(
                &fields[1U], {.count_readable = false}
            );
        const auto return_stop =
            query_legacy_battle_actor_target_selection_count(
                &fields[1U], {.return_address_readable = false}
            );
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        count_read_typed_stop &&
                !field_stop.value.has_value() &&
                return_stop.status ==
                    LegacyBattleActorTargetSelectionCountQueryStatus::
                        return_address_read_typed_stop &&
                return_stop.value == value,
            "target count read fault has no value while return fault retains the actual WORD"
        );
    }

    {
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        auto startup = std::make_unique<LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        auto& party = action->group_a_action_execution;
        auto& enemies = *startup->group_b_lifecycle;
        party[0U].target_selection_count = 0xABCDU;
        party[2U].target_selection_count = 0x1234U;
        enemies[0U].action_execution.target_selection_count = 0x5678U;
        enemies[3U].action_execution.target_selection_count = 0x8000U;
        enemies[3U].action_execution.start_gate = 0x4567U;
        const auto party_result =
            query_legacy_battle_actor_target_selection_count(
                &party[2U].target_selection_count
            );
        const auto enemy_result =
            query_legacy_battle_actor_target_selection_count(
                &enemies[3U].action_execution.target_selection_count
            );
        party[2U].target_selection_count = 0xFFFFU;
        const auto later_result =
            query_legacy_battle_actor_target_selection_count(
                &party[2U].target_selection_count
            );
        test.expect_true(
            party_result.value == 0x1234U && enemy_result.value == 0x8000U &&
                later_result.value == 0xFFFFU &&
                party[0U].target_selection_count == 0xABCDU &&
                party[2U].target_selection_count == 0xFFFFU &&
                enemies[0U].action_execution.target_selection_count ==
                    0x5678U &&
                enemies[3U].action_execution.target_selection_count ==
                    0x8000U &&
                enemies[3U].action_execution.start_gate == 0x4567U,
            "target count query borrows distinct actual actor fields and rereads later shared writes"
        );
    }

    const auto missing =
        query_legacy_battle_actor_target_selection_count(nullptr);
    test.expect_true(
        missing.status ==
                LegacyBattleActorTargetSelectionCountQueryStatus::
                    count_read_typed_stop &&
            !missing.value.has_value(),
        "missing target count retains the original field-read fault boundary"
    );
}
