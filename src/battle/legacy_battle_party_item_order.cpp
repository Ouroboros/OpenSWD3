#include "openswd3/battle/legacy_battle_party_item_order.hpp"

#include <list>

namespace openswd3::battle {
namespace {

using compat::u32;
using world_map::LegacyWorldItemNode;
using world_map::LegacyWorldSentinelItemList;
using PartyItemIterator = std::list<LegacyWorldItemNode>::iterator;

[[nodiscard]] PartyItemIterator find_node_by_token(
    LegacyWorldSentinelItemList& list, const u32 token
) noexcept {
    for (auto iterator = list.nodes.begin(); iterator != list.nodes.end();
         ++iterator) {
        if (iterator->legacy_token == token) {
            return iterator;
        }
    }
    return list.nodes.end();
}

void stop_at_item_node(
    LegacyBattlePartyItemOrderResult& result,
    const u32 list_index,
    const u32 token
) noexcept {
    result.status = LegacyBattlePartyItemOrderStatus::item_node_typed_stop;
    result.fault_list_index = list_index;
    result.fault_token = token;
}

}  // namespace

LegacyBattlePartyItemOrderResult order_legacy_battle_party_item_lists(
    world_map::LegacyWorldItemListState& state, const u32 entry_eax
) noexcept {
    LegacyBattlePartyItemOrderResult result;
    result.return_eax = entry_eax;

    for (u32 list_index = 0U; list_index < state.party_item_lists.size();
         ++list_index) {
        ++result.lists_visited;
        auto& owner = state.party_item_lists[list_index];
        if (!owner.has_value()) {
            result.status =
                LegacyBattlePartyItemOrderStatus::list_root_typed_stop;
            result.fault_list_index = list_index;
            return result;
        }

        auto& list = *owner;
        const u32 entry_head_token = list.sentinel.legacy_next_token;
        result.return_eax = entry_head_token;
        if (entry_head_token == 0U) {
            continue;
        }

        const auto entry_head = find_node_by_token(list, entry_head_token);
        if (entry_head == list.nodes.end()) {
            stop_at_item_node(result, list_index, entry_head_token);
            return result;
        }
        if (entry_head->legacy_next_token == 0U) {
            continue;
        }

        bool link_is_root = true;
        u32 link_owner_token{};
        for (;;) {
            u32 current_token{};
            PartyItemIterator link_owner = list.nodes.end();
            if (link_is_root) {
                current_token = list.sentinel.legacy_next_token;
            } else {
                link_owner = find_node_by_token(list, link_owner_token);
                if (link_owner == list.nodes.end()) {
                    stop_at_item_node(result, list_index, link_owner_token);
                    return result;
                }
                current_token = link_owner->legacy_next_token;
            }

            const auto current = find_node_by_token(list, current_token);
            if (current == list.nodes.end()) {
                stop_at_item_node(result, list_index, current_token);
                return result;
            }

            const u32 next_token = current->legacy_next_token;
            result.return_eax = next_token;
            if (next_token == 0U) {
                break;
            }

            const compat::u16 current_item_id = current->item_id;
            const auto next = find_node_by_token(list, next_token);
            if (next == list.nodes.end()) {
                stop_at_item_node(result, list_index, next_token);
                return result;
            }
            ++result.comparisons;
            if (current_item_id <= next->item_id) {
                link_is_root = false;
                link_owner_token = current_token;
                continue;
            }

            current->legacy_next_token = next->legacy_next_token;
            next->legacy_next_token = current_token;
            if (link_is_root) {
                list.sentinel.legacy_next_token = next_token;
            } else {
                link_owner->legacy_next_token = next_token;
            }
            list.nodes.splice(current, list.nodes, next);
            ++result.swaps;
            ++result.restarts;
            link_is_root = true;
        }
    }

    return result;
}

}  // namespace openswd3::battle
