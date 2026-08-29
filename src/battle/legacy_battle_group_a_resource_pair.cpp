#include "openswd3/battle/legacy_battle_group_a_resource_pair.hpp"

namespace openswd3::battle {

LegacyBattleGroupAResourcePairResult
publish_legacy_battle_group_a_resource_pair(
    LegacyBattleGroupAResourcePairState& state,
    const compat::u32 object_token,
    const compat::u32 resource_token,
    const compat::u32 entry_edx
) noexcept {
    LegacyBattleGroupAResourcePairResult result{
        .return_eax = resource_token,
        .return_ecx = object_token,
        .return_edx = entry_edx,
    };
    if (object_token == 0U) {
        result.status = LegacyBattleGroupAResourcePairStatus::actor_typed_stop;
        return result;
    }

    state.primary_token = resource_token;
    result.writes = 1U;
    state.secondary_token = resource_token;
    result.writes = 2U;
    return result;
}

}  // namespace openswd3::battle
