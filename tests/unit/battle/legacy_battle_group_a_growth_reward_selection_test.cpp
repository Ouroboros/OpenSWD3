#include "openswd3/battle/legacy_battle_group_a_growth_reward_selection.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>

namespace {

using openswd3::battle::LegacyBattleGroupASummonProfileRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void set_profile_word(
    LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u16 value
) {
    profile[offset] = static_cast<std::byte>(static_cast<u8>(value));
    profile[offset + 1U] = static_cast<std::byte>(static_cast<u8>(value >> 8U));
}

[[nodiscard]] std::array<LegacyBattleGroupASummonProfileRecord, 2>
profiles(const u16 item_id, const u16 maximum) {
    std::array<LegacyBattleGroupASummonProfileRecord, 2> value{};
    set_profile_word(value[0U], 0x04U, maximum);
    set_profile_word(value[0U], 0x10U, item_id);
    return value;
}

}  // namespace

void test_battle_group_a_growth_reward_selection(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupAGrowthRewardSelectionStatus;
    using openswd3::battle::LegacyBattleGroupARewardProfileNode;
    using openswd3::battle::LegacyBattleGroupARewardProfileState;
    using openswd3::battle::kLegacyBattleGroupARewardProfileListToken;
    using openswd3::battle::select_legacy_battle_group_a_growth_reward;

    constexpr u32 actor_token = 0x005029D0U;

    {
        LegacyBattleGroupARewardProfileState state;
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state, nullptr, 0U, kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::
                        actor_profile_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x1A8U,
            "growth reward selection stops at the first actor profile read after zeroing eax and publishing the profile cursor"
        );
    }

    {
        const std::array<LegacyBattleGroupASummonProfileRecord, 2> empty{};
        const auto result = select_legacy_battle_group_a_growth_reward(
            nullptr, &empty, actor_token, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.profiles_visited == 2U && result.return_eax == 0U &&
                result.return_ecx == actor_token &&
                result.return_edx == actor_token + 0x2F0U,
            "two zero growth profiles never touch the absent compact list and leave the advanced cursor"
        );
    }

    {
        const auto source = profiles(7U, 10U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            nullptr, &source, actor_token, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::
                        profile_list_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == actor_token + 0x1A8U,
            "a nonzero growth profile stops at the first compact list-head read"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 20U;
        const auto source = profiles(7U, 10U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.return_eax == 7U && result.matching_nodes == 1U &&
                result.quantity_writes == 1U && result.blocking_writes == 1U &&
                state.head.quantity == 10U && state.head.blocking_flag == 1U &&
                result.return_ecx ==
                    kLegacyBattleGroupARewardProfileListToken &&
                result.return_edx == actor_token + 0x2F0U,
            "growth reward selection clamps the first sufficient unblocked match and returns its item id"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 9U;
        const auto source = profiles(7U, 10U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.return_eax == 0U && result.insufficient_nodes == 1U &&
                state.head.quantity == 9U && state.head.blocking_flag == 0U &&
                result.return_ecx == 0U,
            "an insufficient matching growth node does not clamp or block and scanning continues to list end"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 20U;
        state.head.blocking_flag = 1U;
        state.head.legacy_next_token = 0x00600000U;
        state.nodes.push_back(
            LegacyBattleGroupARewardProfileNode{
                .legacy_token = 0x00600000U,
                .item_id = 7U,
                .quantity = 30U,
            }
        );
        const auto source = profiles(7U, 10U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.return_eax == 7U && result.matching_nodes == 2U &&
                result.blocked_nodes == 1U && result.traversed_nodes == 1U &&
                state.head.quantity == 20U &&
                state.nodes.front().quantity == 10U &&
                state.nodes.front().blocking_flag == 1U,
            "a blocked duplicate does not stop growth selection from choosing a later sufficient duplicate"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        state.head.legacy_next_token = 0x00DEAD00U;
        const auto source = profiles(7U, 10U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::
                        profile_node_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x00DEAD00U &&
                result.return_edx == actor_token + 0x1A8U,
            "a broken compact growth token stops at the original next-node dereference"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 1U;
        std::array<LegacyBattleGroupASummonProfileRecord, 2> source{};
        set_profile_word(source[0U], 0x04U, 1U);
        set_profile_word(source[0U], 0x10U, 7U);
        set_profile_word(source[1U], 0x04U, 2U);
        set_profile_word(source[1U], 0x10U, 8U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.return_eax == 7U && result.profiles_visited == 2U &&
                result.nonzero_profiles == 2U &&
                result.return_edx == actor_token + 0x24CU,
            "a first-profile success reads the second item id then returns before any second list access"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 100U;
        const auto source = profiles(7U, 0U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.return_eax == 7U && state.head.quantity == 0U &&
                state.head.blocking_flag == 1U,
            "a zero maximum is immediately sufficient and clamps the selected compact node to zero"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 8U;
        state.head.quantity = 5U;
        std::array<LegacyBattleGroupASummonProfileRecord, 2> source{};
        set_profile_word(source[0U], 0x04U, 10U);
        set_profile_word(source[0U], 0x10U, 7U);
        set_profile_word(source[1U], 0x04U, 5U);
        set_profile_word(source[1U], 0x10U, 8U);
        const auto result = select_legacy_battle_group_a_growth_reward(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAGrowthRewardSelectionStatus::completed &&
                result.return_eax == 8U && result.profiles_visited == 2U &&
                state.head.quantity == 5U && state.head.blocking_flag == 1U,
            "growth selection advances from an absent first profile item to a sufficient second profile item"
        );
    }
}
