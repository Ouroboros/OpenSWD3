#include "openswd3/battle/legacy_battle_party_item_order.hpp"

#include "test.hpp"

#include <iterator>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldItemListState;
using openswd3::world_map::LegacyWorldItemNode;
using openswd3::world_map::LegacyWorldSentinelItemList;

LegacyWorldItemNode& append_node(
    LegacyWorldSentinelItemList& list,
    const u32 token,
    const u32 next_token,
    const u16 item_id,
    const u16 selected_count
) {
    auto& node = list.nodes.emplace_back();
    node.legacy_token = token;
    node.legacy_next_token = next_token;
    node.item_id = item_id;
    node.selected_count = selected_count;
    return node;
}

[[nodiscard]] std::vector<u16>
item_ids(const LegacyWorldSentinelItemList& list) {
    std::vector<u16> values;
    for (const auto& node : list.nodes) {
        values.push_back(node.item_id);
    }
    return values;
}

}  // namespace

void test_battle_party_item_order(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattlePartyItemOrderStatus;
    using openswd3::battle::order_legacy_battle_party_item_lists;

    {
        LegacyWorldItemListState state;
        const auto result =
            order_legacy_battle_party_item_lists(state, 0xA5A5A5A5U);
        test.expect_true(
            result.status == LegacyBattlePartyItemOrderStatus::completed &&
                result.return_eax == 0U && result.lists_visited == 4U &&
                result.comparisons == 0U && result.swaps == 0U,
            "four empty party item roots each overwrite EAX with a null head"
        );
    }

    {
        LegacyWorldItemListState state;
        auto& last = *state.party_item_lists[3U];
        last.sentinel.legacy_next_token = 0x00600000U;
        auto& only = append_node(last, 0x00600000U, 0U, 7U, 9U);
        const auto result =
            order_legacy_battle_party_item_lists(state, 0x11223344U);
        test.expect_true(
            result.status == LegacyBattlePartyItemOrderStatus::completed &&
                result.return_eax == 0x00600000U && result.comparisons == 0U &&
                only.selected_count == 9U,
            "single node in the final party list returns its head and leaves all fields untouched"
        );
    }

    {
        LegacyWorldItemListState state;
        auto& first_list = *state.party_item_lists[0U];
        first_list.sentinel.legacy_next_token = 0x00610000U;
        append_node(first_list, 0x00610000U, 0x006100B0U, 4U, 40U);
        append_node(first_list, 0x006100B0U, 0x00610160U, 1U, 10U);
        append_node(first_list, 0x00610160U, 0U, 3U, 30U);

        auto& second_list = *state.party_item_lists[1U];
        second_list.sentinel.legacy_next_token = 0x00620000U;
        append_node(second_list, 0x00620000U, 0x006200B0U, 5U, 50U);
        append_node(second_list, 0x006200B0U, 0U, 5U, 60U);

        const auto result = order_legacy_battle_party_item_lists(state, 0U);
        const auto& first = first_list.nodes.front();
        const auto& middle = *std::next(first_list.nodes.begin());
        const auto& tail = first_list.nodes.back();
        test.expect_true(
            result.status == LegacyBattlePartyItemOrderStatus::completed &&
                result.return_eax == 0U && result.comparisons == 6U &&
                result.swaps == 2U && result.restarts == 2U &&
                first_list.sentinel.legacy_next_token == 0x006100B0U &&
                item_ids(first_list) == std::vector<u16>{1U, 3U, 4U} &&
                first.legacy_next_token == 0x00610160U &&
                middle.legacy_next_token == 0x00610000U &&
                tail.legacy_next_token == 0U &&
                item_ids(second_list) == std::vector<u16>{5U, 5U} &&
                second_list.nodes.front().legacy_token == 0x00620000U &&
                first.selected_count == 10U && middle.selected_count == 30U &&
                tail.selected_count == 40U,
            "each party list sorts stably by unsigned item id without clearing selected counts"
        );
    }

    {
        LegacyWorldItemListState state;
        state.party_item_lists[0U].reset();
        const auto result =
            order_legacy_battle_party_item_lists(state, 0xDEADBEEFU);
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemOrderStatus::list_root_typed_stop &&
                result.return_eax == 0xDEADBEEFU &&
                result.fault_list_index == 0U && result.fault_token == 0U &&
                result.lists_visited == 1U,
            "missing first root stops on its unconditional dereference and preserves entry EAX"
        );
    }

    {
        LegacyWorldItemListState state;
        auto& prefix = *state.party_item_lists[0U];
        prefix.sentinel.legacy_next_token = 0x00630000U;
        append_node(prefix, 0x00630000U, 0x006300B0U, 2U, 2U);
        append_node(prefix, 0x006300B0U, 0U, 1U, 1U);
        state.party_item_lists[1U].reset();
        const auto result = order_legacy_battle_party_item_lists(state, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemOrderStatus::list_root_typed_stop &&
                result.return_eax == 0U && result.fault_list_index == 1U &&
                result.lists_visited == 2U && result.swaps == 1U &&
                item_ids(prefix) == std::vector<u16>{1U, 2U},
            "later missing root preserves all completed earlier-list sorting side effects"
        );
    }

    {
        LegacyWorldItemListState state;
        auto& list = *state.party_item_lists[0U];
        list.sentinel.legacy_next_token = 0x00700000U;
        const auto result = order_legacy_battle_party_item_lists(state, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemOrderStatus::item_node_typed_stop &&
                result.return_eax == 0x00700000U &&
                result.fault_list_index == 0U &&
                result.fault_token == 0x00700000U,
            "unknown nonnull party head stops on the initial next-link read"
        );
    }

    {
        LegacyWorldItemListState state;
        auto& list = *state.party_item_lists[0U];
        list.sentinel.legacy_next_token = 0x00710000U;
        auto& current = append_node(list, 0x00710000U, 0x007100B0U, 8U, 6U);
        const auto result = order_legacy_battle_party_item_lists(state, 0U);
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemOrderStatus::item_node_typed_stop &&
                result.return_eax == 0x007100B0U &&
                result.fault_token == 0x007100B0U && result.comparisons == 0U &&
                current.selected_count == 6U &&
                current.legacy_next_token == 0x007100B0U,
            "unknown next node stops at its item-id read without modifying the current node"
        );
    }
}
