#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

namespace openswd3::world_map {

namespace {

void release_description(
    LegacyWorldItemNode& node, LegacyWorldItemListReleaseResult& result
) noexcept {
    ++result.description_release_calls;
    if (node.description.capacity() != 0U) {
        ++result.description_owners_released;
    }
    std::vector<compat::u8>{}.swap(node.description);
}

void drain_nodes(
    std::list<LegacyWorldItemNode>& nodes,
    compat::u32& released_count,
    LegacyWorldItemListReleaseResult& result
) noexcept {
    while (!nodes.empty()) {
        release_description(nodes.front(), result);
        nodes.pop_front();
        ++released_count;
    }
}

void release_sentinel_list(
    std::optional<LegacyWorldSentinelItemList>& list,
    compat::u32& released_node_count,
    compat::u32& released_sentinel_count,
    LegacyWorldItemListReleaseResult& result
) noexcept {
    drain_nodes(list->nodes, released_node_count, result);
    release_description(list->sentinel, result);
    list.reset();
    ++released_sentinel_count;
}

}  // namespace

LegacyWorldSentinelItemList::LegacyWorldSentinelItemList() noexcept {
    sentinel.item_id = kLegacyItemSentinelId;
    sentinel.quantity_a = 1U;
    sentinel.definition_snapshot[0U] = kLegacyItemSentinelNameBytes[0U];
    sentinel.definition_snapshot[1U] = kLegacyItemSentinelNameBytes[1U];
}

LegacyWorldItemListState::LegacyWorldItemListState() noexcept {
    for (auto& list : party_item_lists) {
        list.emplace();
    }
    for (auto& list : role_item_lists) {
        list.emplace();
    }
}

LegacyWorldItemListReleaseResult
release_legacy_world_item_lists(LegacyWorldItemListState& state) noexcept {
    LegacyWorldItemListReleaseResult result;

    // The original unconditionally dereferences these four roots. Reject a
    // malformed modern owner before reproducing any of its partial frees.
    for (std::size_t index = 0U; index < state.party_item_lists.size();
         ++index) {
        if (!state.party_item_lists[index].has_value()) {
            result.missing_party_list_index = static_cast<compat::u32>(index);
            return result;
        }
    }

    drain_nodes(state.player_inventory, result.player_nodes_released, result);

    for (auto& list : state.party_item_lists) {
        release_sentinel_list(
            list,
            result.party_nodes_released,
            result.party_sentinels_released,
            result
        );
    }

    for (auto& list : state.role_item_lists) {
        if (!list.has_value()) {
            continue;
        }
        release_sentinel_list(
            list,
            result.role_nodes_released,
            result.role_sentinels_released,
            result
        );
    }

    result.status = LegacyWorldItemListReleaseStatus::ready;
    return result;
}

}  // namespace openswd3::world_map
