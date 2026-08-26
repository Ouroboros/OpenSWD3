#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;

inline constexpr compat::u32 kLegacyBattlePlayerItemHeadToken = 0x004A9940U;
inline constexpr compat::u32 kLegacyBattlePlayerItemNodeSize = 0xB0U;
inline constexpr compat::u32 kLegacyBattlePlayerItemPayloadOffset = 0x0CU;

enum class LegacyBattlePlayerItemQuantityStatus : compat::u8 {
    completed,
    item_node_typed_stop,
    allocation_typed_stop,
    host_allocation_typed_stop,
};

struct LegacyBattlePlayerItemQuantityResult {
    LegacyBattlePlayerItemQuantityStatus status{
        LegacyBattlePlayerItemQuantityStatus::completed
    };
    compat::u32 return_token{};
    compat::u32 port_calls{};
    compat::u32 traversed_nodes{};
    bool created{};
};

[[nodiscard]] LegacyBattlePlayerItemQuantityResult
advance_legacy_battle_player_item_quantity(
    LegacyBattleActionDispatchPort& port,
    compat::u32 item_id,
    compat::u32 quantity_selector
);

}  // namespace openswd3::battle
