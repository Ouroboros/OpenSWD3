#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

namespace openswd3::battle {

enum class LegacyBattlePlayerItemOrderStatus : compat::u8 {
    completed,
    item_node_typed_stop,
};

struct LegacyBattlePlayerItemOrderResult {
    LegacyBattlePlayerItemOrderStatus status{
        LegacyBattlePlayerItemOrderStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 fault_token{};
    compat::u32 comparisons{};
    compat::u32 selected_count_clears{};
    compat::u32 swaps{};
    compat::u32 restarts{};
};

[[nodiscard]] LegacyBattlePlayerItemOrderResult
order_legacy_battle_player_items(
    world_map::LegacyWorldItemListState& state
) noexcept;

}  // namespace openswd3::battle
