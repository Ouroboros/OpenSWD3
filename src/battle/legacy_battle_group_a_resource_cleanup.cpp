#include "openswd3/battle/legacy_battle_group_a_resource_cleanup.hpp"

#include "openswd3/battle/legacy_battle_group_a_storage.hpp"

namespace openswd3::battle {
namespace {

bool release_resource(
    compat::u32& resource_token, LegacyBattleGroupAStorage* resources
) {
    if (resource_token == 0U) {
        return false;
    }

    static_cast<void>(resources->release_heap_block(resource_token).value());
    resource_token = 0U;
    return true;
}

}  // namespace

LegacyBattleGroupAResourceCleanupResult release_legacy_battle_group_a_resources(
    compat::u32& primary_resource_token,
    compat::u32& secondary_resource_token,
    LegacyBattleGroupAStorage* resources,
    const compat::u32 actor_token
) {
    LegacyBattleGroupAResourceCleanupResult result;
    if (actor_token == 0U) {
        result.status =
            LegacyBattleGroupAResourceCleanupStatus::actor_state_typed_stop;
        return result;
    }

    result.secondary_resource_released =
        release_resource(secondary_resource_token, resources);
    result.primary_resource_released =
        release_resource(primary_resource_token, resources);
    return result;
}

LegacyBattleGroupAResourceCleanupResult release_legacy_battle_group_a_resources(
    LegacyBattleGroupAResourceCleanupState* const state,
    LegacyBattleGroupAStorage* resources,
    const compat::u32 actor_token
) {
    if (state == nullptr) {
        return {
            .status =
                LegacyBattleGroupAResourceCleanupStatus::actor_state_typed_stop,
        };
    }

    return release_legacy_battle_group_a_resources(
        state->primary_resource_token,
        state->secondary_resource_token,
        resources,
        actor_token
    );
}

}  // namespace openswd3::battle
