#include "openswd3/battle/legacy_battle_group_a_reward_profile_application.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::battle::LegacyBattleGroupASummonProfileRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct AllocationPort final : LegacyBattleActionDispatchPort {
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        requests.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    std::deque<LegacyBattleActionCallReply> replies;
    std::vector<LegacyBattleActionCallRequest> requests;
};

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

void test_battle_group_a_reward_profile_application(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupARewardProfileApplicationStatus;
    using openswd3::battle::LegacyBattleGroupARewardProfileNode;
    using openswd3::battle::LegacyBattleGroupARewardProfileState;
    using openswd3::battle::apply_legacy_battle_group_a_reward_profiles;
    using openswd3::battle::kLegacyBattleGroupARewardProfileListToken;
    using openswd3::battle::kLegacyBattleGroupARewardProfileNodeSize;

    constexpr u32 actor_token = 0x005029D0U;

    {
        LegacyBattleGroupARewardProfileState state;
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            nullptr,
            0U,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 5U, .entry_eax = 0x12345678U, .entry_edx = 0xABCDEF01U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        actor_profile_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0xABCDEF01U && port.requests.empty(),
            "group-A reward profiles stop at the first actor profile access with entry registers intact"
        );
    }

    {
        const std::array<LegacyBattleGroupASummonProfileRecord, 2> empty{};
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            nullptr,
            &empty,
            actor_token,
            0U,
            port,
            {.quantity = 5U, .entry_eax = 0x11111111U, .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                result.profiles_visited == 2U &&
                result.nonzero_profiles == 0U && result.return_eax == 0U &&
                result.return_ecx == 0x00500000U &&
                result.return_edx == 0x22222222U,
            "two zero embedded profile ids return zero without touching the absent reward list"
        );
    }

    {
        const auto source = profiles(7U, 100U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            nullptr,
            &source,
            actor_token,
            0U,
            port,
            {.quantity = 5U, .entry_eax = 0x33333333U, .entry_edx = 0x44444444U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        profile_list_typed_stop &&
                result.profiles_visited == 1U &&
                result.nonzero_profiles == 1U &&
                result.return_eax == 0x33333333U &&
                result.return_ecx == 0x00500007U &&
                result.return_edx == 0x44444444U,
            "a nonzero profile stops at the first list-head read after publishing the composite item register"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 90U;
        auto source = profiles(7U, 100U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 20U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                result.return_eax == 1U && result.matched_profiles == 1U &&
                result.quantity_writes == 1U &&
                result.percentage_writes == 1U && state.head.quantity == 100U &&
                state.head.percentage == 100U && result.return_ecx == 0U &&
                result.return_edx == 0U && port.requests.empty(),
            "an existing reward node wraps then unsigned-clamps quantity before storing truncated percentage"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 65530U;
        auto source = profiles(7U, 100U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 10U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                state.head.quantity == 4U && state.head.percentage == 4U,
            "reward quantity preserves low-word overflow before the unsigned maximum comparison"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 9U;
        state.head.percentage = 33U;
        state.head.blocking_flag = 1U;
        auto source = profiles(7U, 100U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 10U, .entry_edx = 0x87654321U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                result.matched_profiles == 1U &&
                result.blocked_profiles == 1U && result.quantity_writes == 0U &&
                state.head.quantity == 9U && state.head.percentage == 33U &&
                result.return_ecx == 0x00500000U &&
                result.return_edx == 0x87654321U,
            "a matching blocked reward node reports success while preserving quantities and stale edx"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        state.head.legacy_next_token = 0x00600000U;
        state.nodes.push_back(
            LegacyBattleGroupARewardProfileNode{
                .legacy_token = 0x00600000U,
                .item_id = 7U,
                .quantity = 1U,
            }
        );
        auto source = profiles(7U, 10U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 2U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                result.traversed_nodes == 1U && result.matched_profiles == 1U &&
                state.nodes.front().quantity == 3U &&
                state.nodes.front().percentage == 30U,
            "reward profile lookup follows the explicit legacy token chain to a matching node"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        state.head.legacy_next_token = 0x00DEAD00U;
        auto source = profiles(7U, 10U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 2U, .entry_edx = 0x12345678U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        profile_node_typed_stop &&
                result.return_eax == 0x00DEAD00U &&
                result.return_ecx == 0x00500007U &&
                result.return_edx == 0x12345678U,
            "a broken reward token stops at the original next-node dereference"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        auto source = profiles(7U, 80U);
        AllocationPort port;
        port.replies.push_back({
            .eax = 0x00600000U,
            .ecx = 0xAABBCCDDU,
            .edx = 0x11223344U,
        });
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 20U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                result.allocation_calls == 1U && result.created_nodes == 1U &&
                result.head_item_id_increments == 1U &&
                state.head.item_id == 2U &&
                state.head.legacy_next_token == 0x00600000U &&
                state.nodes.size() == 1U &&
                state.nodes.front().legacy_token == 0x00600000U &&
                state.nodes.front().item_id == 7U &&
                state.nodes.front().quantity == 20U &&
                state.nodes.front().percentage == 25U &&
                port.requests.size() == 1U &&
                port.requests[0U].callee_token == 0x00487C10U &&
                port.requests[0U].arguments[0U] ==
                    kLegacyBattleGroupARewardProfileNodeSize &&
                port.requests[0U].eax == 0U,
            "a missing reward item appends one zeroed compact node and preserves the original head item-id increment"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        auto source = profiles(7U, 80U);
        AllocationPort port;
        port.replies.push_back({.ecx = 0xAABBCCDDU, .edx = 0x11223344U});
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 20U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        allocation_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0xAABBCCDDU &&
                result.return_edx == 0U && state.nodes.empty() &&
                state.head.legacy_next_token == 0U,
            "a zero allocation token stops after the allocator and tail-link prefix"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        auto source = profiles(7U, 5U);
        AllocationPort port;
        port.replies.push_back({.eax = 0x00600000U});
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 21U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                state.nodes.size() == 1U &&
                state.nodes.front().quantity == 21U &&
                state.nodes.front().percentage == 419U,
            "reward percentage preserves the x87 intermediate division rounding below an exact mathematical integer"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 50U;
        auto source = profiles(7U, 0U);
        AllocationPort port;
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 10U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                state.head.quantity == 0U && state.head.percentage == 0U &&
                result.return_eax == 1U && result.return_edx == 0x80000000U,
            "a zero profile maximum preserves masked x87 divide-by-zero indefinite registers"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        std::array<LegacyBattleGroupASummonProfileRecord, 2> source{};
        set_profile_word(source[0U], 0x04U, 80U);
        set_profile_word(source[0U], 0x10U, 7U);
        set_profile_word(source[1U], 0x04U, 10U);
        set_profile_word(source[1U], 0x10U, 2U);
        AllocationPort port;
        port.replies.push_back({.eax = 0x00600000U});
        const auto result = apply_legacy_battle_group_a_reward_profiles(
            &state,
            &source,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            port,
            {.quantity = 3U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupARewardProfileApplicationStatus::
                        completed &&
                result.created_nodes == 1U && result.matched_profiles == 1U &&
                state.head.item_id == 2U && state.head.quantity == 3U &&
                state.head.percentage == 30U && state.nodes.size() == 1U,
            "the first append's anomalous head-id increment remains visible to the second profile scan"
        );
    }
}
