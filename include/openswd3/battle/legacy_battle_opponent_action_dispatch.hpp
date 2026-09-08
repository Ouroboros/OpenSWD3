#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp"

namespace openswd3::battle {

// Adapter seam used by the opponent dispatcher for the action-seventeen
// audio calls. Reserved coordinate slots return an empty reply without
// reaching the generic action port.
[[nodiscard]] LegacyBattleGroupBActionSeventeenFrameCallReply
invoke_legacy_battle_opponent_action_seventeen_frame_call(
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleGroupBActionSeventeenFrameCallRequest& request
);

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
