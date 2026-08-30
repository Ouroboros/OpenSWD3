#include "openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp"

namespace openswd3::battle {
namespace {

void release_resource(
    compat::u32& resource_token,
    const compat::u32 resource_offset,
    LegacyBattleGroupAResourceReleasePort& port,
    const LegacyBattleGroupAResourceCleanupRequest& request,
    LegacyBattleGroupAResourceCleanupResult& result,
    bool& released
) {
    result.return_eax = resource_token;
    if (result.return_eax == 0U) {
        return;
    }

    const auto reply = port.release_group_a_resource({
        .callee_token = kLegacyBattleResourceReleaseCalleeToken,
        .actor_token = request.actor_token,
        .actor_index = request.actor_index,
        .resource_token = result.return_eax,
        .resource_offset = resource_offset,
        .eax = result.return_eax,
        .ecx = result.return_ecx,
        .edx = result.return_edx,
    });
    ++result.resource_release_calls;
    result.return_eax = reply.eax;
    result.return_ecx = reply.ecx;
    result.return_edx = reply.edx;
    resource_token = 0U;
    released = true;
}

}  // namespace

LegacyBattleGroupAResourceCleanupResult release_legacy_battle_group_a_resources(
    LegacyBattleGroupAResourceCleanupState* const state,
    LegacyBattleGroupAResourceReleasePort& port,
    const LegacyBattleGroupAResourceCleanupRequest& request
) {
    LegacyBattleGroupAResourceCleanupResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.entry_ecx,
        .return_edx = request.entry_edx,
    };
    if (state == nullptr || request.actor_token == 0U) {
        result.status =
            LegacyBattleGroupAResourceCleanupStatus::actor_state_typed_stop;
        return result;
    }

    release_resource(
        state->secondary_resource_token,
        kLegacyBattleGroupASecondaryResourceOffset,
        port,
        request,
        result,
        result.secondary_resource_released
    );
    release_resource(
        state->primary_resource_token,
        kLegacyBattleGroupAPrimaryResourceOffset,
        port,
        request,
        result,
        result.primary_resource_released
    );
    return result;
}

}  // namespace openswd3::battle
