#include "openswd3/battle/legacy_battle_actor_target_selection_latch_query.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_target_selection_latch_query(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorRuntimeResetState;
    using openswd3::battle::LegacyBattleActorTargetSelectionLatchQueryStatus;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::kLegacyBattleActorGroupAElementCount;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::query_legacy_battle_actor_target_selection_latch;
    using openswd3::compat::u32;

    for (const u32 value : std::array<u32, 7>{
             0U, 1U, 2U, 0x00010001U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU
         }) {
        const u32 target_selection_latch = value;
        const auto result = query_legacy_battle_actor_target_selection_latch(
            &target_selection_latch
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorTargetSelectionLatchQueryStatus::
                        completed &&
                result.value == value && target_selection_latch == value,
            "target-selection latch reads the entire shared DWORD without changing or normalizing it"
        );
    }

    {
        const u32 target_selection_latch = 0xA5A5A5A5U;
        const auto missing =
            query_legacy_battle_actor_target_selection_latch(nullptr);
        const auto read_stop = query_legacy_battle_actor_target_selection_latch(
            &target_selection_latch, {.target_selection_latch_readable = false}
        );
        const auto return_stop =
            query_legacy_battle_actor_target_selection_latch(
                &target_selection_latch, {.return_address_readable = false}
            );
        test.expect_true(
            missing.status ==
                    LegacyBattleActorTargetSelectionLatchQueryStatus::
                        target_selection_latch_read_typed_stop &&
                !missing.value.has_value() &&
                read_stop.status ==
                    LegacyBattleActorTargetSelectionLatchQueryStatus::
                        target_selection_latch_read_typed_stop &&
                !read_stop.value.has_value() &&
                return_stop.status ==
                    LegacyBattleActorTargetSelectionLatchQueryStatus::
                        return_address_read_typed_stop &&
                return_stop.value == target_selection_latch &&
                target_selection_latch == 0xA5A5A5A5U,
            "target-selection latch failure distinguishes an unread field from the actual value read before the return fault"
        );
    }

    {
        LegacyBattleStartupState startup;
        startup.group_a_runtime_reset = std::make_shared<std::array<
            LegacyBattleActorRuntimeResetState,
            kLegacyBattleActorGroupAElementCount>>();
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        auto& party = (*startup.group_a_runtime_reset)[2U];
        auto& enemy = (*startup.group_b_lifecycle)[3U].runtime_reset;
        party.target_selection_latch = 0x00010001U;
        enemy.target_selection_latch = 0x80000000U;
        const auto party_read =
            query_legacy_battle_actor_target_selection_latch(
                &party.target_selection_latch
            );
        const auto enemy_read =
            query_legacy_battle_actor_target_selection_latch(
                &enemy.target_selection_latch
            );
        party.target_selection_latch = 1U;
        const auto later_read =
            query_legacy_battle_actor_target_selection_latch(
                &party.target_selection_latch
            );
        test.expect_true(
            party_read.value == 0x00010001U &&
                enemy_read.value == 0x80000000U && later_read.value == 1U &&
                enemy.target_selection_latch == 0x80000000U &&
                (*startup.group_a_runtime_reset)[0U].target_selection_latch ==
                    0U &&
                (*startup.group_b_lifecycle)[0U]
                        .runtime_reset.target_selection_latch == 0U,
            "target-selection latch reads the actual independent party and enemy fields and observes subsequent writes"
        );
    }
}
