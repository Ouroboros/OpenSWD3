#include "openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp"

#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"

namespace openswd3::battle {

LegacyBattleGroupBResourceCleanupResult release_legacy_battle_group_b_resource(
    LegacyBattleActorGroupBElementState* const state,
    LegacyBattleGroupBResourceReleasePort& port,
    const LegacyBattleGroupBResourceCleanupRequest& request
) {
    LegacyBattleGroupBResourceCleanupResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (state == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleGroupBResourceCleanupStatus::actor_state_typed_stop;
        return result;
    }

    result.return_eax = state->resource_token;
    if (result.return_eax == 0U) {
        return result;
    }

    const auto reply = port.release_group_b_resource({
        .callee_token = kLegacyBattleResourceReleaseCalleeToken,
        .actor_token = request.actor_token,
        .actor_index = request.actor_index,
        .resource_token = result.return_eax,
        .resource_offset = kLegacyBattleGroupBResourceOffset,
        .eax = result.return_eax,
        .ecx = result.return_ecx,
        .edx = result.return_edx,
    });
    ++result.resource_release_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    state->resource_token = 0U;
    state->resource_bytes.fill(0U);
    result.resource_released = true;
    return result;
}

}  // namespace openswd3::battle
