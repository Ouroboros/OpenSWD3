#include "openswd3/battle/legacy_battle_actor_start_gate.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_start_gate(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    using openswd3::compat::u16;
    using Status = LegacyBattleActorStartGateStatus;

    for (const u16 value : std::array<u16, 7>{
             0U, 1U, 0x00FFU, 0x0100U, 0x7FFFU, 0x8000U, 0xFFFFU
         }) {
        const std::array<u16, 3> fields{0x1234U, value, 0x5678U};
        const auto result = query_legacy_battle_actor_start_gate(&fields[1U]);
        test.expect_true(
            result.status == Status::completed && result.value == value &&
                fields == std::array<u16, 3>{0x1234U, value, 0x5678U},
            "start count query reads the entire unsigned WORD without changing adjacent fields"
        );
    }

    {
        const u16 value = 0xBEEFU;
        const auto missing = query_legacy_battle_actor_start_gate(nullptr);
        const auto unreadable = query_legacy_battle_actor_start_gate(
            &value, {.start_gate_readable = false}
        );
        const auto return_stop = query_legacy_battle_actor_start_gate(
            &value, {.return_address_readable = false}
        );
        const auto both = query_legacy_battle_actor_start_gate(
            &value,
            {.start_gate_readable = false, .return_address_readable = false}
        );
        test.expect_true(
            missing.status == Status::start_gate_read_typed_stop &&
                !missing.value.has_value() &&
                unreadable.status == Status::start_gate_read_typed_stop &&
                !unreadable.value.has_value() &&
                return_stop.status == Status::return_address_read_typed_stop &&
                return_stop.value == value &&
                both.status == Status::start_gate_read_typed_stop &&
                !both.value.has_value() && value == 0xBEEFU,
            "read failure has no value and return failure retains the observed WORD without writes"
        );
    }

    {
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        auto startup = std::make_unique<LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        auto& party = action->group_a_action_execution[2U];
        auto& enemy = (*startup->group_b_lifecycle)[3U].action_execution;
        party.start_gate = 0x1122U;
        party.target_selection_count = 0x7788U;
        party.start_gate_latch = 0xCAFEBABEU;
        enemy.start_gate = 0x5566U;
        enemy.target_selection_count = 0x99AAU;
        enemy.start_gate_latch = 0x12345678U;
        const auto party_result =
            query_legacy_battle_actor_start_gate(&party.start_gate);
        const auto enemy_result =
            query_legacy_battle_actor_start_gate(&enemy.start_gate);
        party.start_gate = 0xFFFFU;
        const auto reread =
            query_legacy_battle_actor_start_gate(&party.start_gate);
        test.expect_true(
            party_result.value == 0x1122U && enemy_result.value == 0x5566U &&
                reread.value == 0xFFFFU && enemy.start_gate == 0x5566U &&
                party.target_selection_count == 0x7788U &&
                enemy.target_selection_count == 0x99AAU &&
                party.start_gate_latch == 0xCAFEBABEU &&
                enemy.start_gate_latch == 0x12345678U &&
                action->group_a_action_execution[0U].start_gate == 0U &&
                (*startup->group_b_lifecycle)[0U].action_execution.start_gate ==
                    0U,
            "queries borrow distinct actual actors and reread live counts without copying or touching other counters and latches"
        );
    }
}
