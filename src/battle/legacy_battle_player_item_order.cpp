#include "openswd3/battle/legacy_battle_player_item_order.hpp"

#include <list>

namespace openswd3::battle {
namespace {

using compat::u32;
using world_map::LegacyWorldItemListState;
using world_map::LegacyWorldItemNode;
using PlayerItemIterator = std::list<LegacyWorldItemNode>::iterator;

[[nodiscard]] PlayerItemIterator
find_node_by_token(LegacyWorldItemListState& state, const u32 token) noexcept {
    for (auto iterator = state.player_inventory.begin();
         iterator != state.player_inventory.end();
         ++iterator) {
        if (iterator->legacy_token == token) {
            return iterator;
        }
    }
    return state.player_inventory.end();
}

void stop_at_node_access(
    LegacyBattlePlayerItemOrderResult& result, const u32 token
) noexcept {
    result.status = LegacyBattlePlayerItemOrderStatus::item_node_typed_stop;
    result.fault_token = token;
}

}  // namespace

LegacyBattlePlayerItemOrderResult
order_legacy_battle_player_items(LegacyWorldItemListState& state) noexcept {
    LegacyBattlePlayerItemOrderResult result;
    const u32 entry_head_token = state.player_inventory_head_token;
    result.return_eax = entry_head_token;
    if (entry_head_token == 0U) {
        return result;
    }

    const auto entry_head = find_node_by_token(state, entry_head_token);
    if (entry_head == state.player_inventory.end()) {
        stop_at_node_access(result, entry_head_token);
        return result;
    }
    if (entry_head->legacy_next_token == 0U) {
        return result;
    }

    u32 link_owner_token{};
    for (;;) {
        u32 current_token{};
        PlayerItemIterator link_owner = state.player_inventory.end();
        if (link_owner_token == 0U) {
            current_token = state.player_inventory_head_token;
        } else {
            link_owner = find_node_by_token(state, link_owner_token);
            if (link_owner == state.player_inventory.end()) {
                stop_at_node_access(result, link_owner_token);
                return result;
            }
            current_token = link_owner->legacy_next_token;
        }

        const auto current = find_node_by_token(state, current_token);
        if (current == state.player_inventory.end()) {
            stop_at_node_access(result, current_token);
            return result;
        }

        const u32 next_token = current->legacy_next_token;
        result.return_eax = next_token;
        if (next_token == 0U) {
            return result;
        }

        const compat::u16 current_item_id = current->item_id;
        current->selected_count = 0U;
        ++result.selected_count_clears;

        const auto next = find_node_by_token(state, next_token);
        if (next == state.player_inventory.end()) {
            stop_at_node_access(result, next_token);
            return result;
        }
        ++result.comparisons;
        if (current_item_id <= next->item_id) {
            link_owner_token = current_token;
            continue;
        }

        current->legacy_next_token = next->legacy_next_token;
        next->legacy_next_token = current_token;
        if (link_owner_token == 0U) {
            state.player_inventory_head_token = next_token;
        } else {
            link_owner->legacy_next_token = next_token;
        }
        state.player_inventory.splice(current, state.player_inventory, next);
        ++result.swaps;
        ++result.restarts;
        link_owner_token = 0U;
    }
}

}  // namespace openswd3::battle
