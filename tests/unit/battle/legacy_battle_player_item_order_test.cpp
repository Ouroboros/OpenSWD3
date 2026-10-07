#include "openswd3/battle/legacy_battle_player_item_order.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include "test.hpp"

#include <iterator>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldItemListState;
using openswd3::world_map::LegacyWorldItemNode;

LegacyWorldItemNode& append_node(
    LegacyWorldItemListState& state,
    const u32 token,
    const u32 next_token,
    const u16 item_id,
    const u16 selected_count
) {
    auto& node = state.player_inventory.emplace_back();
    node.legacy_token = token;
    node.legacy_next_token = next_token;
    node.item_id = item_id;
    node.selected_count = selected_count;
    return node;
}

[[nodiscard]] std::vector<u16> item_ids(const LegacyWorldItemListState& state) {
    std::vector<u16> values;
    for (const auto& node : state.player_inventory) {
        values.push_back(node.item_id);
    }
    return values;
}

void test_startup_item_order(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleStartupStatus;
    using openswd3::battle::order_legacy_battle_startup_items;

    {
        LegacyWorldItemListState items;
        items.player_inventory_head_token = 0x600000U;
        auto& high = append_node(items, 0x600000U, 0x6000B0U, 0xFFFFU, 11U);
        auto& low = append_node(items, 0x6000B0U, 0x600160U, 0U, 12U);
        auto& equal_a = append_node(items, 0x600160U, 0x600210U, 0x8000U, 13U);
        auto& equal_b = append_node(items, 0x600210U, 0U, 0x8000U, 14U);
        for (u32 index = 0U; index < 4U; ++index) {
            auto& list = items.party_item_lists[index].emplace();
            const u32 token = 0x700000U + index * 0x1000U;
            list.sentinel.legacy_token = token;
            list.legacy_head_token = token;
            list.sentinel.legacy_next_token = token + 0xB0U;
            auto& first = list.nodes.emplace_back();
            first.legacy_token = token + 0xB0U;
            first.legacy_next_token = token + 0x160U;
            first.item_id = 0x8000U;
            first.selected_count = 21U;
            auto& second = list.nodes.emplace_back();
            second.legacy_token = token + 0x160U;
            second.item_id = 0x7FFFU;
            second.selected_count = 22U;
        }

        const auto result = order_legacy_battle_startup_items(items);
        test.expect_true(
            result.status == LegacyBattleStartupStatus::completed &&
                result.party_item_order.lists_visited == 4U &&
                result.party_item_order.swaps == 4U &&
                items.player_inventory_head_token == low.legacy_token &&
                &items.player_inventory.front() == &low &&
                &items.player_inventory.back() == &high &&
                low.legacy_next_token == equal_a.legacy_token &&
                equal_a.legacy_next_token == equal_b.legacy_token &&
                equal_b.legacy_next_token == high.legacy_token &&
                high.legacy_next_token == 0U &&
                item_ids(items) ==
                    std::vector<u16>{0U, 0x8000U, 0x8000U, 0xFFFFU},
            "startup sorts the borrowed inventory stably by unsigned IDs without copying node owners"
        );
        for (u32 index = 0U; index < 4U; ++index) {
            const auto& list = *items.party_item_lists[index];
            const u32 token = 0x700000U + index * 0x1000U;
            test.expect_true(
                list.sentinel.legacy_token == token &&
                    list.sentinel.legacy_next_token == token + 0x160U &&
                    list.nodes.front().legacy_next_token == token + 0xB0U &&
                    list.nodes.front().selected_count == 22U &&
                    list.nodes.back().legacy_next_token == 0U &&
                    list.nodes.back().selected_count == 21U,
                "startup sorts all four party lists while retaining sentinel identity and selected counts"
            );
        }

        high.selected_count = 77U;
        const auto repeated = order_legacy_battle_startup_items(items);
        test.expect_true(
            repeated.status == LegacyBattleStartupStatus::completed &&
                repeated.player_item_order.swaps == 0U &&
                repeated.party_item_order.swaps == 0U &&
                high.selected_count == 77U && low.selected_count == 0U &&
                &items.player_inventory.front() == &low &&
                &items.player_inventory.back() == &high,
            "repeated startup reuses sorted nodes and preserves the unexamined inventory tail"
        );
    }

    {
        LegacyWorldItemListState items;
        items.player_inventory_head_token = 0x600000U;
        auto& current = append_node(items, 0x600000U, 0x6000B0U, 9U, 7U);
        const auto stopped = order_legacy_battle_startup_items(items);
        test.expect_true(
            stopped.status ==
                    LegacyBattleStartupStatus::player_item_order_typed_stop &&
                stopped.player_item_order.fault_token == 0x6000B0U &&
                current.selected_count == 0U &&
                stopped.party_item_order.lists_visited == 0U,
            "inventory failure retains the selected-count store and does not visit any party root"
        );
    }

    {
        LegacyWorldItemListState items;
        items.player_inventory_head_token = 0x600000U;
        append_node(items, 0x600000U, 0U, 9U, 7U);
        items.party_item_lists[0U].reset();
        items.party_item_lists[1U].reset();
        const auto stopped = order_legacy_battle_startup_items(items);
        test.expect_true(
            stopped.status ==
                    LegacyBattleStartupStatus::party_item_order_typed_stop &&
                stopped.party_item_order.return_eax == 0x600000U &&
                stopped.party_item_order.fault_list_index == 0U,
            "first missing party root receives the preceding singleton inventory EAX"
        );

        auto& list = items.party_item_lists[0U].emplace();
        list.sentinel.legacy_token = 0x700000U;
        list.legacy_head_token = list.sentinel.legacy_token;
        list.sentinel.legacy_next_token = 0x7000B0U;
        auto& first = list.nodes.emplace_back();
        first.legacy_token = 0x7000B0U;
        first.legacy_next_token = 0x700160U;
        first.item_id = 2U;
        auto& second = list.nodes.emplace_back();
        second.legacy_token = 0x700160U;
        second.item_id = 1U;
        const auto later = order_legacy_battle_startup_items(items);
        test.expect_true(
            later.status ==
                    LegacyBattleStartupStatus::party_item_order_typed_stop &&
                later.party_item_order.fault_list_index == 1U &&
                later.party_item_order.lists_visited == 2U &&
                later.party_item_order.return_eax == 0U &&
                list.sentinel.legacy_next_token == second.legacy_token &&
                second.legacy_next_token == first.legacy_token &&
                first.legacy_next_token == 0U,
            "later missing party root preserves the already sorted prefix without visiting remaining roots"
        );
    }
}

}  // namespace

void test_battle_player_item_order(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattlePlayerItemOrderStatus;
    using openswd3::battle::order_legacy_battle_player_items;

    test_startup_item_order(test);

    {
        LegacyWorldItemListState state;
        const auto result = order_legacy_battle_player_items(state);
        test.expect_true(
            result.status == LegacyBattlePlayerItemOrderStatus::completed &&
                result.return_eax == 0U && result.comparisons == 0U &&
                result.selected_count_clears == 0U && result.swaps == 0U,
            "empty player item head returns the loaded null EAX without touching nodes"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00600000U;
        auto& only = append_node(state, 0x00600000U, 0U, 7U, 9U);
        const auto result = order_legacy_battle_player_items(state);
        test.expect_true(
            result.status == LegacyBattlePlayerItemOrderStatus::completed &&
                result.return_eax == 0x00600000U &&
                result.selected_count_clears == 0U && only.selected_count == 9U,
            "single player item returns the entry head and preserves its selected count"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00610000U;
        auto& first = append_node(state, 0x00610000U, 0x006100B0U, 1U, 10U);
        auto& second = append_node(state, 0x006100B0U, 0x00610160U, 2U, 20U);
        auto& tail = append_node(state, 0x00610160U, 0U, 3U, 30U);
        const auto result = order_legacy_battle_player_items(state);
        test.expect_true(
            result.status == LegacyBattlePlayerItemOrderStatus::completed &&
                result.return_eax == 0U && result.comparisons == 2U &&
                result.selected_count_clears == 2U && result.swaps == 0U &&
                first.selected_count == 0U && second.selected_count == 0U &&
                tail.selected_count == 30U &&
                item_ids(state) == std::vector<u16>{1U, 2U, 3U},
            "already ordered items clear every compared node but preserve the untouched final tail"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00620000U;
        append_node(state, 0x00620000U, 0x006200B0U, 3U, 30U);
        append_node(state, 0x006200B0U, 0x00620160U, 1U, 10U);
        append_node(state, 0x00620160U, 0U, 2U, 20U);
        const auto result = order_legacy_battle_player_items(state);
        const auto& first = state.player_inventory.front();
        const auto& second = *std::next(state.player_inventory.begin());
        const auto& third = state.player_inventory.back();
        test.expect_true(
            result.status == LegacyBattlePlayerItemOrderStatus::completed &&
                result.return_eax == 0U && result.comparisons == 5U &&
                result.selected_count_clears == 5U && result.swaps == 2U &&
                result.restarts == 2U &&
                state.player_inventory_head_token == 0x006200B0U &&
                item_ids(state) == std::vector<u16>{1U, 2U, 3U} &&
                first.legacy_next_token == 0x00620160U &&
                second.legacy_next_token == 0x00620000U &&
                third.legacy_next_token == 0U && first.selected_count == 0U &&
                second.selected_count == 0U && third.selected_count == 0U,
            "out-of-order items swap physical links stably and restart each pass from the global head"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00630000U;
        auto& first = append_node(state, 0x00630000U, 0x006300B0U, 5U, 1U);
        auto& second = append_node(state, 0x006300B0U, 0U, 5U, 2U);
        const auto result = order_legacy_battle_player_items(state);
        test.expect_true(
            result.swaps == 0U && first.legacy_next_token == 0x006300B0U &&
                first.selected_count == 0U && second.selected_count == 2U,
            "equal item ids preserve physical order and use the unsigned less-or-equal path"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00700000U;
        const auto result = order_legacy_battle_player_items(state);
        test.expect_true(
            result.status ==
                    LegacyBattlePlayerItemOrderStatus::item_node_typed_stop &&
                result.return_eax == 0x00700000U &&
                result.fault_token == 0x00700000U &&
                result.selected_count_clears == 0U,
            "unknown nonnull head stops on the initial next-link read"
        );
    }

    {
        LegacyWorldItemListState state;
        state.player_inventory_head_token = 0x00710000U;
        auto& current = append_node(state, 0x00710000U, 0x007100B0U, 8U, 6U);
        const auto result = order_legacy_battle_player_items(state);
        test.expect_true(
            result.status ==
                    LegacyBattlePlayerItemOrderStatus::item_node_typed_stop &&
                result.return_eax == 0x007100B0U &&
                result.fault_token == 0x007100B0U && result.comparisons == 0U &&
                result.selected_count_clears == 1U &&
                current.selected_count == 0U &&
                current.legacy_next_token == 0x007100B0U,
            "unknown next node stops only after the current selected-count store"
        );
    }
}
