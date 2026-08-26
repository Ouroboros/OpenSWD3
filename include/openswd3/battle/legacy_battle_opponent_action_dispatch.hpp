#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

namespace openswd3::battle {

// Typed closure of legacy 0x00455D60. The first index selects a group-B
// actor. The second index is consumed only by action branches that touch a
// group-A or group-B target.
[[nodiscard]] LegacyBattleActionDispatchResult
dispatch_legacy_battle_opponent_action(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    compat::u32 group_b_index,
    compat::u32 target_index
);

}  // namespace openswd3::battle
