#include "openswd3/battle/legacy_battle_player_item_order.hpp"

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

}  // namespace

void test_battle_player_item_order(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattlePlayerItemOrderStatus;
    using openswd3::battle::order_legacy_battle_player_items;

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
