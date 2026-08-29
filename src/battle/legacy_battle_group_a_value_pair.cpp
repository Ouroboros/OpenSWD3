#include "openswd3/battle/legacy_battle_group_a_value_pair.hpp"

namespace openswd3::battle {

LegacyBattleGroupAValuePairResult publish_legacy_battle_group_a_value_pair(
    LegacyBattleGroupAValuePairState& state,
    const compat::u32 object_token,
    const compat::u32 value,
    const compat::u32 entry_edx
) noexcept {
    LegacyBattleGroupAValuePairResult result{
        .return_eax = value,
        .return_ecx = object_token,
        .return_edx = entry_edx,
    };
    if (object_token == 0U) {
        result.status = LegacyBattleGroupAValuePairStatus::actor_typed_stop;
        return result;
    }

    state.primary_value = value;
    result.writes = 1U;
    state.secondary_value = value;
    result.writes = 2U;
    return result;
}

}  // namespace openswd3::battle
