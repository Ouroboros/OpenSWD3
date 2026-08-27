#include "openswd3/battle/legacy_battle_actor_ready.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

namespace openswd3::battle {

LegacyBattleActorReadyResult query_legacy_battle_actor_ready(
    const LegacyBattleActorReadyState& state,
    LegacyBattleActorReadyPort& port,
    const compat::u32 actor_index,
    const compat::u32 actor_group
) {
    LegacyBattleActorReadyResult result{};
    if (actor_group == 1U) {
        result.actor_token = kLegacyBattleActionGroupABaseToken +
            actor_index * kLegacyBattleActionGroupAStride;
        result.stale_eax =
            actor_index * (state.global_mode == 0U ? 0xBCDU : 0x3EFU);
        result.stale_edx = state.caller_edx;
    } else {
        result.actor_token = kLegacyBattleActionGroupBBaseToken +
            actor_index * kLegacyBattleActionGroupBStride;
        result.stale_eax = actor_index * 0x565U;
        result.stale_edx = actor_index * 0x159U;
    }
    ++result.port_calls;
    const auto queried = port.query_ready({
        .actor_token = result.actor_token,
        .stale_eax = result.stale_eax,
        .stale_edx = result.stale_edx,
    });
    result.return_value = queried.eax == 1U ? queried.eax : 0U;
    result.final_ecx = queried.ecx;
    result.final_edx = queried.edx;
    return result;
}

}  // namespace openswd3::battle
