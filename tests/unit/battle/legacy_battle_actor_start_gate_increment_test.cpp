#include "openswd3/battle/legacy_battle_actor_start_gate_increment.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>

void test_battle_actor_start_gate_increment(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    using openswd3::compat::u16;
    using openswd3::compat::u32;
    using Status = LegacyBattleActorStartGateIncrementStatus;

    {
        constexpr std::array<u16, 6> previous{
            0U, 1U, 15U, 0x7FFFU, 0x8000U, 0xFFFFU
        };
        constexpr std::array<u16, 6> expected{
            1U, 2U, 16U, 0x8000U, 0x8001U, 0U
        };
        constexpr std::array<u32, 4> latches{0U, 1U, 0x80000000U, 0xFFFFFFFFU};
        for (std::size_t index = 0U; index < previous.size(); ++index) {
            for (const u32 latch : latches) {
                std::array<u16, 3> words{0x1234U, previous[index], 0xABCDU};
                std::array<u32, 3> markers{0x11223344U, latch, 0x55667788U};
                const auto result = increment_legacy_battle_actor_start_gate(
                    &words[1], &markers[1]
                );
                test.expect_true(
                    result.status == Status::completed &&
                        result.previous_start_gate == previous[index] &&
                        result.incremented_start_gate == expected[index] &&
                        result.start_gate_latch_committed &&
                        words ==
                            std::array<u16, 3>{
                                0x1234U, expected[index], 0xABCDU
                            } &&
                        markers ==
                            std::array<u32, 3>{0x11223344U, 1U, 0x55667788U},
                    "start count wraps at WORD width and publishes exactly DWORD one without touching neighbors"
                );
            }
        }
    }

    {
        constexpr std::array<LegacyBattleActorStartGateIncrementAccess, 4>
            accesses{{
                {.start_gate_readable = false},
                {.start_gate_writable = false},
                {.start_gate_latch_writable = false},
                {.return_address_readable = false},
            }};
        constexpr std::array<Status, 4> statuses{
            Status::start_gate_read_typed_stop,
            Status::start_gate_write_typed_stop,
            Status::start_gate_latch_write_typed_stop,
            Status::return_address_read_typed_stop,
        };
        for (std::size_t phase = 0U; phase < accesses.size(); ++phase) {
            u16 count = 0xFFFFU;
            u32 latch = 0xCAFEBABEU;
            const auto result = increment_legacy_battle_actor_start_gate(
                &count, &latch, accesses[phase]
            );
            test.expect_true(
                result.status == statuses[phase] &&
                    result.previous_start_gate.has_value() == (phase > 0U) &&
                    result.incremented_start_gate.has_value() == (phase > 1U) &&
                    result.start_gate_latch_committed == (phase == 3U) &&
                    count == (phase > 1U ? 0U : 0xFFFFU) &&
                    latch == (phase == 3U ? 1U : 0xCAFEBABEU),
                "each access failure keeps only the original observed and committed prefix"
            );
        }
    }

    {
        u32 latch = 99U;
        const auto missing_count =
            increment_legacy_battle_actor_start_gate(nullptr, &latch);
        u16 count = 0x7FFFU;
        const auto missing_latch =
            increment_legacy_battle_actor_start_gate(&count, nullptr);
        test.expect_true(
            missing_count.status == Status::start_gate_read_typed_stop &&
                !missing_count.previous_start_gate.has_value() &&
                !missing_count.incremented_start_gate.has_value() &&
                !missing_count.start_gate_latch_committed && latch == 99U &&
                missing_latch.status ==
                    Status::start_gate_latch_write_typed_stop &&
                missing_latch.previous_start_gate == 0x7FFFU &&
                missing_latch.incremented_start_gate == 0x8000U &&
                !missing_latch.start_gate_latch_committed && count == 0x8000U,
            "unavailable fields stop at their original access rather than producing a default success"
        );
    }

    {
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        LegacyBattleStartupState startup;
        startup.group_b_lifecycle = std::make_shared<
            std::array<LegacyBattleActorGroupBElementState, 8>>();
        auto& party = action->group_a_action_execution[9];
        auto& enemy = (*startup.group_b_lifecycle)[7].action_execution;
        party.start_gate = 0x7FFFU;
        enemy.start_gate = 0xFFFFU;
        const auto first = increment_legacy_battle_actor_start_gate(
            &party.start_gate, &party.start_gate_latch
        );
        const auto second = increment_legacy_battle_actor_start_gate(
            &enemy.start_gate, &enemy.start_gate_latch
        );
        party.start_gate = 10U;
        party.start_gate_latch = 0x80000000U;
        const auto live = increment_legacy_battle_actor_start_gate(
            &party.start_gate, &party.start_gate_latch
        );
        test.expect_true(
            first.incremented_start_gate == 0x8000U &&
                second.incremented_start_gate == 0U &&
                live.previous_start_gate == 10U &&
                live.incremented_start_gate == 11U && party.start_gate == 11U &&
                party.start_gate_latch == 1U && enemy.start_gate == 0U &&
                enemy.start_gate_latch == 1U &&
                action->group_a_action_execution[8].start_gate == 0U &&
                (*startup.group_b_lifecycle)[6].action_execution.start_gate ==
                    0U,
            "both actual owners share committed fields and repeated calls observe live writes"
        );
    }
}
