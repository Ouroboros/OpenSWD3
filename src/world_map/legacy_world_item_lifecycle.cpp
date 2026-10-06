#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

namespace openswd3::world_map {

namespace {

bool release_description(
    LegacyWorldItemNode& node, LegacyWorldItemListReleaseResult& result
) noexcept {
    ++result.description_release_calls;
    const bool had_storage = node.description.bytes().capacity() != 0U;
    if (!node.description.release()) {
        result.status =
            LegacyWorldItemListReleaseStatus::description_release_typed_stop;
        return false;
    }

    if (had_storage) {
        ++result.description_owners_released;
    }

    return true;
}

bool drain_nodes(
    std::list<LegacyWorldItemNode>& nodes,
    compat::u32& head_token,
    compat::u32& released_count,
    LegacyWorldItemListReleaseResult& result
) noexcept {
    while (!nodes.empty()) {
        // 0x0040F41E/0x0040F451/0x0040F4AD unlink before freeing text.
        head_token = nodes.front().legacy_next_token;
        if (!release_description(nodes.front(), result)) {
            return false;
        }

        nodes.pop_front();
        ++released_count;
    }

    return true;
}

bool release_sentinel_list(
    std::optional<LegacyWorldSentinelItemList>& list,
    compat::u32& released_node_count,
    compat::u32& released_sentinel_count,
    LegacyWorldItemListReleaseResult& result
) noexcept {
    if (!drain_nodes(
            list->nodes,
            list->sentinel.legacy_next_token,
            released_node_count,
            result
        ) ||
        !release_description(list->sentinel, result)) {
        return false;
    }

    list.reset();
    ++released_sentinel_count;
    return true;
}

}  // namespace

LegacyWorldSentinelItemList::LegacyWorldSentinelItemList() noexcept {
    sentinel.item_id = kLegacyItemSentinelId;
    sentinel.quantity_a = 1U;
    sentinel.definition_snapshot[0U] = kLegacyItemSentinelNameBytes[0U];
    sentinel.definition_snapshot[1U] = kLegacyItemSentinelNameBytes[1U];
}

LegacyWorldItemListState::LegacyWorldItemListState() noexcept {
    for (std::size_t index = 0U; index < party_item_lists.size(); ++index) {
        auto& list = party_item_lists[index].emplace();
        list.sentinel.legacy_token = kLegacyPartyItemSentinelTokenBase +
            static_cast<compat::u32>(index) * 0xB0U;
        list.legacy_head_token = list.sentinel.legacy_token;
    }
    for (std::size_t index = 0U; index < role_item_lists.size(); ++index) {
        auto& list = role_item_lists[index].emplace();
        list.sentinel.legacy_token = kLegacyRoleItemSentinelTokenBase +
            static_cast<compat::u32>(index) * 0xB0U;
        list.legacy_head_token = list.sentinel.legacy_token;
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

    if (!drain_nodes(
            state.player_inventory,
            state.player_inventory_head_token,
            result.player_nodes_released,
            result
        )) {
        return result;
    }

    state.player_inventory_head_token = 0U;

    for (auto& list : state.party_item_lists) {
        if (!release_sentinel_list(
                list,
                result.party_nodes_released,
                result.party_sentinels_released,
                result
            )) {
            return result;
        }
    }

    for (auto& list : state.role_item_lists) {
        if (!list.has_value()) {
            continue;
        }
        if (!release_sentinel_list(
                list,
                result.role_nodes_released,
                result.role_sentinels_released,
                result
            )) {
            return result;
        }
    }

    result.status = LegacyWorldItemListReleaseStatus::ready;
    return result;
}

}  // namespace openswd3::world_map
