#include "openswd3/battle/legacy_battle_reward_item_slot.hpp"
#include "test.hpp"

#include <array>

void test_battle_reward_item_slot(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleOutcomeFinalizationState;
    using openswd3::battle::LegacyBattleRewardItemSlotStatus;
    using openswd3::battle::write_legacy_battle_reward_item_slot;
    using openswd3::compat::u16;

    LegacyBattleOutcomeFinalizationState state;
    state.reward_item_slot_prefix = 0xAAAAU;
    state.player_reward_item_ids = {0xBBBBU, 0xCCCCU};

    const auto prefix = write_legacy_battle_reward_item_slot(
        state, 0U, 0xFFFF1234U, 0xABCD5678U, 0x89ABCDEFU
    );
    test.expect_true(
        prefix.status == LegacyBattleRewardItemSlotStatus::completed &&
            prefix.store_address == 0x004FF2EAU && prefix.stored &&
            prefix.return_eax == 0xABCD1234U && prefix.return_ecx == 0U &&
            prefix.return_edx == 0x89ABCDEFU &&
            state.reward_item_slot_prefix == 0x1234U &&
            state.player_reward_item_ids ==
                std::array<u16, 2>{0xBBBBU, 0xCCCCU},
        "slot zero ignores the argument high word, writes the physical prefix, replaces only AX, and preserves EDX"
    );

    const auto first = write_legacy_battle_reward_item_slot(
        state, 1U, 0x2345U, 0x13579BDFU, 0x2468ACE0U
    );
    const auto second = write_legacy_battle_reward_item_slot(
        state, 2U, 0x3456U, first.return_eax, first.return_edx
    );
    test.expect_true(
        first.store_address == 0x004FF2ECU && first.return_eax == 0x13572345U &&
            first.return_ecx == 1U && first.return_edx == 0x2468ACE0U &&
            second.stored && second.store_address == 0x004FF2EEU &&
            second.return_eax == 0x13573456U && second.return_ecx == 2U &&
            second.return_edx == 0x2468ACE0U &&
            state.reward_item_slot_prefix == 0x1234U &&
            state.player_reward_item_ids ==
                std::array<u16, 2>{0x2345U, 0x3456U},
        "slots one and two alias the two outcome reward words and preserve the entry register high bits"
    );

    const auto before = state;
    const auto adjacent = write_legacy_battle_reward_item_slot(
        state, 3U, 0x4567U, 0xFEDCBA98U, 0x76543210U
    );
    test.expect_true(
        adjacent.status == LegacyBattleRewardItemSlotStatus::slot_typed_stop &&
            !adjacent.stored && adjacent.store_address == 0x004FF2F0U &&
            adjacent.return_eax == 0xFEDC4567U && adjacent.return_ecx == 3U &&
            adjacent.return_edx == 0x76543210U &&
            state.reward_item_slot_prefix == before.reward_item_slot_prefix &&
            state.player_reward_item_ids == before.player_reward_item_ids,
        "the first adjacent-global store stops after mov ECX and mov AX without touching the reward owner"
    );

    const auto wrapped = write_legacy_battle_reward_item_slot(
        state, 0xFFFFFFFFU, 0x5678U, 0x11112222U, 0x33334444U
    );
    test.expect_true(
        wrapped.status == LegacyBattleRewardItemSlotStatus::slot_typed_stop &&
            wrapped.store_address == 0x004FF2E8U &&
            wrapped.return_eax == 0x11115678U &&
            wrapped.return_ecx == 0xFFFFFFFFU &&
            wrapped.return_edx == 0x33334444U &&
            state.reward_item_slot_prefix == before.reward_item_slot_prefix &&
            state.player_reward_item_ids == before.player_reward_item_ids,
        "slot address multiplication and addition wrap in u32 before the unknown-owner store stops"
    );
}
