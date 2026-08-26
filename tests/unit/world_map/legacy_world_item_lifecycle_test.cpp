#include "test.hpp"

#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>

namespace {

using openswd3::compat::u8;
using openswd3::world_map::kLegacyItemSentinelId;
using openswd3::world_map::kLegacyItemSentinelNameBytes;
using openswd3::world_map::kLegacyPartyItemListCount;
using openswd3::world_map::kLegacyRoleItemListCount;
using openswd3::world_map::LegacyWorldItemListReleaseStatus;
using openswd3::world_map::LegacyWorldItemListState;
using openswd3::world_map::LegacyWorldItemNode;
using openswd3::world_map::release_legacy_world_item_lists;

[[nodiscard]] LegacyWorldItemNode
node_with_description(const std::initializer_list<u8> bytes) {
    LegacyWorldItemNode node;
    node.description.assign(bytes);
    return node;
}

void test_default_sentinel_layout(openswd3::test::Context& test) {
    const LegacyWorldItemListState state;
    const auto is_original_sentinel = [](const auto& list) {
        return list.has_value() &&
            list->sentinel.item_id == kLegacyItemSentinelId &&
            list->sentinel.quantity_a == 1U &&
            list->sentinel.definition_snapshot[0U] ==
            kLegacyItemSentinelNameBytes[0U] &&
            list->sentinel.definition_snapshot[1U] ==
            kLegacyItemSentinelNameBytes[1U] &&
            list->sentinel.definition_snapshot[2U] == 0U && list->nodes.empty();
    };

    test.expect_true(
        std::ranges::all_of(state.party_item_lists, is_original_sentinel) &&
            std::ranges::all_of(state.role_item_lists, is_original_sentinel),
        "post-initialization roots use sub_44D5D0 sentinel defaults"
    );
}

void test_three_release_regions(openswd3::test::Context& test) {
    LegacyWorldItemListState state;
    state.player_inventory_head_token = 0x00600000U;
    state.player_inventory.push_back(node_with_description({1U, 2U}));
    state.player_inventory.push_back(LegacyWorldItemNode{});

    state.party_item_lists[0U]->nodes.push_back(node_with_description({3U}));
    state.party_item_lists[1U]->nodes.push_back(LegacyWorldItemNode{});
    state.party_item_lists[1U]->nodes.push_back(
        node_with_description({4U, 5U, 6U})
    );
    state.party_item_lists[3U]->sentinel.description = {7U};

    state.role_item_lists[1U].reset();
    state.role_item_lists[2U]->nodes.push_back(node_with_description({8U}));
    state.role_item_lists[2U]->sentinel.description = {9U};
    state.role_item_lists[63U]->nodes.push_back(LegacyWorldItemNode{});

    const auto result = release_legacy_world_item_lists(state);
    test.expect_true(
        result.status == LegacyWorldItemListReleaseStatus::ready &&
            result.player_nodes_released == 2U &&
            result.party_nodes_released == 3U &&
            result.party_sentinels_released == kLegacyPartyItemListCount &&
            result.role_nodes_released == 2U &&
            result.role_sentinels_released == kLegacyRoleItemListCount - 1U,
        "sub_40F410 drains inventory, four required lists, then 64 slots"
    );
    test.expect_true(
        result.description_release_calls == 74U &&
            result.description_owners_released == 6U,
        "free is called for every node description, including null owners"
    );
    test.expect_true(
        state.player_inventory.empty() &&
            state.player_inventory_head_token == 0U &&
            std::ranges::none_of(
                state.party_item_lists,
                [](const auto& list) { return list.has_value(); }
            ) &&
            std::ranges::none_of(
                state.role_item_lists,
                [](const auto& list) { return list.has_value(); }
            ),
        "all visited roots are null after their sentinel is released"
    );
}

void test_missing_required_sentinel_is_transactional(
    openswd3::test::Context& test
) {
    LegacyWorldItemListState state;
    state.player_inventory_head_token = 0x00600000U;
    state.player_inventory.push_back(node_with_description({1U}));
    state.party_item_lists[2U].reset();
    state.role_item_lists[0U]->nodes.push_back(node_with_description({2U}));

    const auto result = release_legacy_world_item_lists(state);
    test.expect_true(
        result.status ==
                LegacyWorldItemListReleaseStatus::
                    required_party_sentinel_missing &&
            result.missing_party_list_index == 2U &&
            result.description_release_calls == 0U &&
            state.player_inventory.size() == 1U &&
            state.player_inventory_head_token == 0x00600000U &&
            state.party_item_lists[0U].has_value() &&
            !state.party_item_lists[2U].has_value() &&
            state.role_item_lists[0U]->nodes.size() == 1U,
        "a null required root is isolated before any original partial free"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_default_sentinel_layout(test);
    test_three_release_regions(test);
    test_missing_required_sentinel_is_transactional(test);
    return test.exit_code();
}
