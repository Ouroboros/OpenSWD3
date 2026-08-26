#include "openswd3/battle/legacy_battle_actor_frame_sequence.hpp"
#include "test.hpp"

void test_battle_actor_frame_sequence(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorFrameSequenceStatus;
    using openswd3::battle::LegacyBattleActorMetricState;
    using openswd3::battle::advance_legacy_battle_actor_frame_sequence;

    {
        LegacyBattleActorMetricState state;
        state.actor_order[0] = 77U;
        auto result = advance_legacy_battle_actor_frame_sequence(
            state, nullptr, 0x12345678U
        );
        test.expect_true(
            result.status == LegacyBattleActorFrameSequenceStatus::completed &&
                result.initial_count == 0U && result.scanned_slots == 0U &&
                result.return_value == 0U && state.entry_ecx == 0U &&
                state.entry_edx == 0x12345678U && state.actor_order[0] == 77U,
            "zero wrapped total returns before source or frame context access"
        );

        state.group_b_count = 0x80000000U;
        result = advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.initial_count == 0x80000000U &&
                result.return_value == 0x80000000U &&
                result.scanned_slots == 0U,
            "signed-negative total preserves original jle early return"
        );

        state.group_b_count = 0xFFFFFFFFU;
        state.group_a_count = 1U;
        result = advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.initial_count == 0U && result.scanned_slots == 0U,
            "group count addition keeps low-thirty-two-bit zero wrap"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 1U;
        state.actor_order[0] = 8U;
        const auto result =
            advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.status == LegacyBattleActorFrameSequenceStatus::completed &&
                result.scanned_slots == 1U && result.group_a_calls == 0U &&
                result.group_b_calls == 0U && result.return_value == 0U,
            "group-A code subtracts eight and skips when relative index reaches dynamic count"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 1U;
        state.actor_order[0] = 0U;
        auto result =
            advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSequenceStatus::
                        frame_context_typed_stop &&
                result.scanned_slots == 1U && result.return_value == 0U,
            "eligible group-B code stops at the actual typed frame context boundary"
        );

        state.actor_order[0] = 0xFFFFFFFFU;
        result = advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSequenceStatus::
                        frame_context_typed_stop &&
                result.return_value == 0xFFFFFFFFU,
            "signed-negative actor code follows the group-B branch and signed count comparison"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_a_count = 1U;
        state.actor_order[0] = 8U;
        const auto result =
            advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSequenceStatus::
                        frame_context_typed_stop &&
                result.scanned_slots == 1U && result.return_value == 0U,
            "eligible group-A code stops only after subtracting eight and reloading group-A count"
        );
    }

    {
        LegacyBattleActorMetricState state;
        state.group_b_count = 19U;
        state.actor_order.fill(8U);
        const auto result =
            advance_legacy_battle_actor_frame_sequence(state, nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSequenceStatus::
                        actor_order_typed_stop &&
                result.initial_count == 19U && result.scanned_slots == 18U &&
                result.group_a_calls == 0U && result.group_b_calls == 0U,
            "fixed initial count reaches the nineteenth source read without a modern loop cap"
        );
    }
}
