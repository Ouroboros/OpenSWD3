#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"

namespace openswd3::battle {

LegacyBattleRuntimeShutdownResult shutdown_legacy_battle_runtime(
    LegacyBattleStartupState& startup,
    LegacyBattleRenderAuxiliaryBufferReleaser& render_resources,
    LegacyBattleGroupAStorage* party_resources,
    LegacyBattleGroupBStorage* enemy_resources
) noexcept {
    LegacyBattleRuntimeShutdownResult result;
    result.render_cleanup = release_legacy_battle_render_resources(
        startup.render_geometry, render_resources
    );

    compat::u32 object_token = kLegacyBattleGroupAObjectBaseToken;
    for (compat::u32 index = 0U; index < kLegacyBattleGroupAObjectCount;
         ++index) {
        result.group_a_resource_cleanups[index] =
            release_legacy_battle_group_a_resources(
                startup.party[index].configuration.actor_record_token,
                startup.party[index].secondary_resource_token,
                party_resources,
                object_token
            );
        object_token += kLegacyBattleGroupAObjectStride;
    }

    object_token = kLegacyBattleGroupBObjectBaseToken;
    for (compat::u32 index = 0U; index < kLegacyBattleGroupBObjectCount;
         ++index) {
        auto* const actor = startup.group_b_lifecycle == nullptr
            ? nullptr
            : &(*startup.group_b_lifecycle)[index];
        result.group_b_resource_cleanups[index] =
            release_legacy_battle_group_b_resource(
                actor, enemy_resources, object_token
            );
        if (result.group_b_resource_cleanups[index].status !=
            LegacyBattleGroupBResourceCleanupStatus::completed) {
            result.status =
                LegacyBattleRuntimeShutdownStatus::group_b_resource_typed_stop;
            result.stopped_group_b_index = index;
            return result;
        }

        object_token += kLegacyBattleGroupBObjectStride;
    }

    return result;
}

}  // namespace openswd3::battle
