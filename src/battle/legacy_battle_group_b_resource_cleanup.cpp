#include "openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp"

#include "openswd3/battle/legacy_battle_group_b_storage.hpp"

namespace openswd3::battle {

LegacyBattleGroupBResourceCleanupResult release_legacy_battle_group_b_resource(
    LegacyBattleActorGroupBElementState* const state,
    LegacyBattleGroupBStorage* const resources,
    const compat::u32 actor_token
) {
    LegacyBattleGroupBResourceCleanupResult result;
    if (state == nullptr || actor_token == 0U) {
        result.status =
            LegacyBattleGroupBResourceCleanupStatus::actor_state_typed_stop;
        return result;
    }

    if (state->resource_token == 0U) {
        return result;
    }

    if (!resources->release_heap_block(state->resource_token)) {
        throw std::bad_optional_access{};
    }

    state->resource_token = 0U;
    state->resource_bytes.fill(0U);
    state->resource_description.clear();
    result.resource_released = true;
    return result;
}

}  // namespace openswd3::battle
