#include "openswd3/battle/legacy_battle_group_b_order.hpp"
#include "test.hpp"

#include <algorithm>

void test_battle_group_b_order(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorMetricState;
    using openswd3::battle::LegacyBattleGroupBOrderStatus;
    using openswd3::battle::rebuild_legacy_battle_group_b_order;

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 2U;
        state.actor_order = {
            9U,
            1U,
            8U,
            0U,
            10U,
            11U,
            12U,
            13U,
            14U,
            15U,
            16U,
            17U,
            18U,
            19U,
            20U,
            21U,
            22U,
            23U,
        };
        state.group_b_order.fill(99U);
        const auto result = rebuild_legacy_battle_group_b_order(state);
        test.expect_true(
            result.status == LegacyBattleGroupBOrderStatus::completed &&
                result.scanned_slots == 4U && result.copied_slots == 2U &&
                result.reached_requested_count && result.return_value == 0U &&
                result.final_ecx == 2U && result.final_edx == 0x00520E00U &&
                state.group_b_order[0] == 1U && state.group_b_order[1] == 0U &&
                std::ranges::all_of(
                    state.group_b_order.begin() + 2,
                    state.group_b_order.end(),
                    [](const auto value) { return value == 99U; }
                ),
            "signed group-B entries copy in source order and stop at the entry count without clearing tail"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 1U;
        state.actor_order.fill(8U);
        state.actor_order[1] = 7U;
        const auto result = rebuild_legacy_battle_group_b_order(state);
        test.expect_true(
            result.scanned_slots == 2U && result.copied_slots == 1U &&
                result.return_value == 7U && state.group_b_order[0] == 7U,
            "non-group entry skips without advancing output or copied count"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 10U;
        for (std::size_t index = 0U; index < 8U; ++index) {
            state.actor_order[index] =
                static_cast<openswd3::compat::u32>(index);
        }
        for (std::size_t index = 8U; index < state.actor_order.size();
             ++index) {
            state.actor_order[index] =
                static_cast<openswd3::compat::u32>(index);
        }
        const auto result = rebuild_legacy_battle_group_b_order(state);
        test.expect_true(
            result.status == LegacyBattleGroupBOrderStatus::completed &&
                result.scanned_slots == 18U && result.copied_slots == 8U &&
                !result.reached_requested_count && result.return_value == 17U &&
                result.final_ecx == 8U && result.final_edx == 0x00520E18U,
            "requested count above qualifying source exhausts all eighteen slots with the last EAX"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 0U;
        state.actor_order.fill(8U);
        for (std::size_t index = 0U; index < 8U; ++index) {
            state.actor_order[index] =
                static_cast<openswd3::compat::u32>(index);
        }
        state.actor_order[8] = 0xFFFFFFFFU;
        const auto result = rebuild_legacy_battle_group_b_order(state);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOrderStatus::output_store_typed_stop &&
                result.scanned_slots == 9U && result.copied_slots == 8U &&
                result.return_value == 0xFFFFFFFFU && result.final_ecx == 8U &&
                result.final_edx == 0x00520E18U && state.group_b_order[7] == 7U,
            "zero requested count never early-stops and signed negative ninth entry faults only at the real output store"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 0U;
        state.actor_order.fill(9U);
        state.actor_order[5] = 3U;
        state.actor_order[17] = 22U;
        state.group_b_order.fill(77U);
        const auto result = rebuild_legacy_battle_group_b_order(state);
        test.expect_true(
            result.status == LegacyBattleGroupBOrderStatus::completed &&
                result.scanned_slots == 18U && result.copied_slots == 1U &&
                !result.reached_requested_count && result.return_value == 22U &&
                state.group_b_order[0] == 3U && state.group_b_order[1] == 77U,
            "zero requested count scans the full source and preserves untouched destination tail"
        );
    }
}
