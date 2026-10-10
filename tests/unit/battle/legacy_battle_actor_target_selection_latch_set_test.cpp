#include "openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_target_selection_latch_set(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorRuntimeResetState;
    using openswd3::battle::LegacyBattleActorTargetSelectionLatchSetStatus;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::kLegacyBattleActorGroupAElementCount;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::set_legacy_battle_actor_target_selection_latch;
    using openswd3::compat::u32;

    for (const u32 initial : std::array<u32, 7>{
             0U, 1U, 2U, 0x00010001U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU
         }) {
        std::array<u32, 3> fields{0x12345678U, initial, 0x89ABCDEFU};
        const auto status =
            set_legacy_battle_actor_target_selection_latch(&fields[1U]);
        const auto repeated =
            set_legacy_battle_actor_target_selection_latch(&fields[1U]);
        test.expect_true(
            status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::completed &&
                repeated ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::completed &&
                fields == std::array<u32, 3>{0x12345678U, 1U, 0x89ABCDEFU},
            "selection latch setter replaces every full DWORD with one and repeated writes leave adjacent fields unchanged"
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
        auto& party = *startup.group_a_runtime_reset;
        auto& enemies = *startup.group_b_lifecycle;
        party[0U].target_selection_latch = 0x12345678U;
        party[2U].target_selection_latch = 0x00010001U;
        enemies[0U].runtime_reset.target_selection_latch = 0x89ABCDEFU;
        enemies[3U].runtime_reset.target_selection_latch = 0x80000000U;
        enemies[3U].action_execution.start_gate_latch = 0xA5A5A5A5U;
        const auto party_status =
            set_legacy_battle_actor_target_selection_latch(
                &party[2U].target_selection_latch
            );
        const auto enemy_status =
            set_legacy_battle_actor_target_selection_latch(
                &enemies[3U].runtime_reset.target_selection_latch
            );
        party[2U].target_selection_latch = 0xFFFFFFFFU;
        const auto later_status =
            set_legacy_battle_actor_target_selection_latch(
                &party[2U].target_selection_latch
            );
        test.expect_true(
            party_status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::completed &&
                enemy_status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::completed &&
                later_status ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::completed &&
                party[0U].target_selection_latch == 0x12345678U &&
                party[2U].target_selection_latch == 1U &&
                enemies[0U].runtime_reset.target_selection_latch ==
                    0x89ABCDEFU &&
                enemies[3U].runtime_reset.target_selection_latch == 1U &&
                enemies[3U].action_execution.start_gate_latch == 0xA5A5A5A5U,
            "selection latch setter borrows independent actual party and enemy fields and sees subsequent shared writes"
        );
    }

    for (const u32 initial : std::array<u32, 7>{
             0U, 1U, 2U, 0x00010001U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU
         }) {
        u32 latch = initial;
        const auto write_stop = set_legacy_battle_actor_target_selection_latch(
            &latch, {.target_selection_latch_writable = false}
        );
        const u32 after_write_stop = latch;
        const auto return_stop = set_legacy_battle_actor_target_selection_latch(
            &latch, {.return_address_readable = false}
        );
        test.expect_true(
            write_stop ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::
                        target_selection_latch_write_typed_stop &&
                after_write_stop == initial &&
                return_stop ==
                    LegacyBattleActorTargetSelectionLatchSetStatus::
                        return_address_read_typed_stop &&
                latch == 1U,
            "selection latch field fault leaves every old DWORD intact while return fault retains the complete write of one"
        );
    }

    test.expect_true(
        set_legacy_battle_actor_target_selection_latch(nullptr) ==
            LegacyBattleActorTargetSelectionLatchSetStatus::
                target_selection_latch_write_typed_stop,
        "missing selection latch stops at the original field write boundary"
    );
}
