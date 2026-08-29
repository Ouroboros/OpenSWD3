#include "openswd3/battle/legacy_battle_group_a_effect_reward_application.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::battle::LegacyBattleEffectCallPort;
using openswd3::battle::LegacyBattleGroupASummonProfileRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct EffectRewardPort final : LegacyBattleEffectCallPort {
    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        requests.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    std::deque<LegacyBattleEffectCallReply> replies;
    std::vector<LegacyBattleEffectCallRequest> requests;
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
profiles(const u16 item_id, const u16 maximum, const u16 kind = 51U) {
    std::array<LegacyBattleGroupASummonProfileRecord, 2> value{};
    set_profile_word(value[0U], 0x04U, maximum);
    set_profile_word(value[0U], 0x08U, kind);
    set_profile_word(value[0U], 0x10U, item_id);
    return value;
}

}  // namespace

void test_battle_group_a_effect_reward_application(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleGroupAEffectRewardApplicationStatus;
    using openswd3::battle::LegacyBattleGroupARewardProfileNode;
    using openswd3::battle::LegacyBattleGroupARewardProfileState;
    using openswd3::battle::apply_legacy_battle_group_a_effect_rewards;
    using openswd3::battle::kLegacyBattleGroupARewardProfileListToken;
    using openswd3::battle::kLegacyBattleGroupARewardProfileNodeSize;

    constexpr u32 actor_token = 0x005029D0U;
    constexpr u32 destination_token = 0x00526298U;

    {
        LegacyBattleGroupARewardProfileState state;
        u16 destination_word = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            nullptr,
            &destination_word,
            0U,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0xABCDEF01U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        actor_profile_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0xFFFFFE58U && port.requests.empty(),
            "effect reward stops at the first actor profile read after preserving the actor-delta edx prefix"
        );
    }

    {
        const std::array<LegacyBattleGroupASummonProfileRecord, 2> empty{};
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            nullptr,
            &empty,
            nullptr,
            actor_token,
            0U,
            0U,
            port,
            {.entry_eax = 0x11111111U, .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0x00500000U &&
                result.return_edx == 0xFFAFD488U &&
                result.nonzero_profiles == 0U,
            "two zero effect profile ids skip destination and list owners while resetting edx each iteration"
        );
    }

    {
        const auto source = profiles(7U, 100U, 50U);
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            nullptr, &source, nullptr, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.nonzero_profiles == 1U && result.kind_matches == 0U &&
                result.return_eax == 0U,
            "a non-51 embedded profile skips the destination record and reward list"
        );
    }

    {
        const auto source = profiles(7U, 100U);
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            nullptr, &source, nullptr, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        destination_record_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x00500007U &&
                result.return_edx == 0xFFAFD488U,
            "a type-51 profile stops at the destination word dereference after publishing its item id"
        );
    }

    for (const u16 gate : std::array<u16, 2>{0U, 10U}) {
        const auto source = profiles(7U, 100U);
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            nullptr, &source, &gate, actor_token, 0U, destination_token, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.destination_gate_matches == 0U &&
                result.return_eax == 0U,
            "destination gate values outside one through nine skip the compact reward list"
        );
    }

    {
        const auto source = profiles(7U, 100U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            nullptr, &source, &gate, actor_token, 0U, destination_token, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        profile_list_typed_stop &&
                result.destination_gate_matches == 1U &&
                result.address_gate_matches == 1U && result.return_eax == 1U &&
                result.return_ecx == 0x00500007U &&
                result.return_edx == actor_token + 0x1EAU,
            "an eligible effect profile stops at the list head after preserving the computed address register"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 90U;
        const auto source = profiles(7U, 100U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.return_eax == 1U && result.matched_profiles == 1U &&
                state.head.quantity == 100U && state.head.percentage == 100U &&
                result.return_ecx == 0U && result.return_edx == 0xFFAFD488U,
            "an existing effect reward adds fixed twelve clamps and then leaves the second zero-profile actor delta"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        const auto source = profiles(7U, 5U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        port.replies.push_back({.eax = 0x00600000U});
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.created_nodes == 1U && state.head.item_id == 2U &&
                state.nodes.size() == 1U && state.nodes.front().item_id == 7U &&
                state.nodes.front().quantity == 12U &&
                state.nodes.front().percentage == 240U &&
                port.requests.size() == 1U &&
                port.requests[0U].callee_token == 0x00487C10U &&
                port.requests[0U].arguments[0U] ==
                    kLegacyBattleGroupARewardProfileNodeSize,
            "an eligible missing effect reward appends a fixed-twelve compact node without maximum clamping"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        state.head.legacy_next_token = 0x00DEAD00U;
        const auto source = profiles(7U, 100U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        profile_node_typed_stop &&
                result.return_eax == 0x00DEAD00U &&
                result.return_ecx == 0x00500007U &&
                result.return_edx == actor_token + 0x1EAU,
            "an eligible broken effect reward token stops at the original next-node dereference"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        const auto source = profiles(7U, 100U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        allocation_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0U && state.nodes.empty(),
            "a zero compact allocation token stops after the tail-link and allocator prefix"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.quantity = 50U;
        const auto source = profiles(7U, 0U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                state.head.quantity == 0U && state.head.percentage == 0U &&
                result.return_edx == 0xFFAFD488U,
            "a zero maximum writes x87 indefinite before the second profile resets edx to the actor delta"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 1U;
        std::array<LegacyBattleGroupASummonProfileRecord, 2> source{};
        set_profile_word(source[0U], 0x04U, 80U);
        set_profile_word(source[0U], 0x08U, 51U);
        set_profile_word(source[0U], 0x10U, 7U);
        set_profile_word(source[1U], 0x04U, 100U);
        set_profile_word(source[1U], 0x08U, 51U);
        set_profile_word(source[1U], 0x10U, 2U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        port.replies.push_back({.eax = 0x00600000U});
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.created_nodes == 1U && result.matched_profiles == 1U &&
                state.head.item_id == 2U && state.head.quantity == 12U &&
                state.head.percentage == 12U,
            "the first fixed-twelve append keeps the root id alias visible to the second eligible profile"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        state.head.item_id = 7U;
        state.head.blocking_flag = 1U;
        const auto source = profiles(7U, 100U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port,
            {.entry_edx = 0x12345678U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.blocked_profiles == 1U && result.quantity_writes == 0U &&
                state.head.quantity == 0U,
            "an eligible blocked effect reward reports success without changing compact quantities"
        );
    }

    {
        LegacyBattleGroupARewardProfileState state;
        LegacyBattleGroupARewardProfileNode node{
            .legacy_token = 0x00600000U,
            .item_id = 7U,
            .quantity = 1U,
        };
        state.head.item_id = 1U;
        state.head.legacy_next_token = node.legacy_token;
        state.nodes.push_back(node);
        const auto source = profiles(7U, 20U);
        constexpr u16 gate = 1U;
        EffectRewardPort port;
        const auto result = apply_legacy_battle_group_a_effect_rewards(
            &state,
            &source,
            &gate,
            actor_token,
            kLegacyBattleGroupARewardProfileListToken,
            destination_token,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAEffectRewardApplicationStatus::
                        completed &&
                result.traversed_nodes == 1U &&
                state.nodes.front().quantity == 13U &&
                state.nodes.front().percentage == 65U,
            "effect reward lookup follows explicit compact tokens before adding fixed twelve"
        );
    }
}
