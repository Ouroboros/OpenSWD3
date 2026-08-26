#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

namespace openswd3::battle {

enum class LegacyBattlePartyItemOrderStatus : compat::u8 {
    completed,
    list_root_typed_stop,
    item_node_typed_stop,
};

struct LegacyBattlePartyItemOrderResult {
    LegacyBattlePartyItemOrderStatus status{
        LegacyBattlePartyItemOrderStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 fault_list_index{
        static_cast<compat::u32>(world_map::kLegacyPartyItemListCount)
    };
    compat::u32 fault_token{};
    compat::u32 lists_visited{};
    compat::u32 comparisons{};
    compat::u32 swaps{};
    compat::u32 restarts{};
};

[[nodiscard]] LegacyBattlePartyItemOrderResult
order_legacy_battle_party_item_lists(
    world_map::LegacyWorldItemListState& state, compat::u32 entry_eax
) noexcept;

}  // namespace openswd3::battle
