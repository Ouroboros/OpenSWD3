#include "openswd3/battle/legacy_battle_actor_start_gate_latch_query.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_start_gate_latch_query(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleActorStartGateLatchQueryStatus;
    using openswd3::battle::LegacyBattleStartupState;
    using openswd3::battle::kLegacyBattleActorGroupBElementCount;
    using openswd3::battle::query_legacy_battle_actor_start_gate_latch;
    using openswd3::compat::u32;

    for (const u32 value : std::array<u32, 6>{
             0U, 1U, 2U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU
         }) {
        const u32 start_gate_latch = value;
        const auto result =
            query_legacy_battle_actor_start_gate_latch(&start_gate_latch);
        test.expect_true(
            result.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::completed &&
                result.value == value && start_gate_latch == value,
            "start-gate latch reads the complete shared dword without changing it"
        );
    }

    {
        const u32 start_gate_latch = 0xA5A5A5A5U;
        const auto unavailable =
            query_legacy_battle_actor_start_gate_latch(nullptr);
        const auto read_stop = query_legacy_battle_actor_start_gate_latch(
            &start_gate_latch, {.start_gate_latch_readable = false}
        );
        const auto return_stop = query_legacy_battle_actor_start_gate_latch(
            &start_gate_latch, {.return_address_readable = false}
        );
        test.expect_true(
            unavailable.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::
                        start_gate_latch_read_typed_stop &&
                !unavailable.value &&
                read_stop.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::
                        start_gate_latch_read_typed_stop &&
                !read_stop.value &&
                return_stop.status ==
                    LegacyBattleActorStartGateLatchQueryStatus::
                        return_address_read_typed_stop &&
                return_stop.value == 0xA5A5A5A5U &&
                start_gate_latch == 0xA5A5A5A5U,
            "original access failures distinguish an unread latch from an already-read latch"
        );
    }

    {
        LegacyBattleActionDispatchState action;
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        auto& party_latch =
            action.group_a_action_execution[2U].start_gate_latch;
        auto& enemy_latch =
            (*startup.group_b_lifecycle)[3U].action_execution.start_gate_latch;
        party_latch = 0x12340001U;
        enemy_latch = 0x80000000U;
        const auto first_party =
            query_legacy_battle_actor_start_gate_latch(&party_latch);
        const auto enemy =
            query_legacy_battle_actor_start_gate_latch(&enemy_latch);
        party_latch = 1U;
        const auto second_party =
            query_legacy_battle_actor_start_gate_latch(&party_latch);
        test.expect_true(
            first_party.value == 0x12340001U && enemy.value == 0x80000000U &&
                second_party.value == 1U && enemy_latch == 0x80000000U,
            "party and enemy queries borrow their real fields and observe later writes"
        );
    }
}
