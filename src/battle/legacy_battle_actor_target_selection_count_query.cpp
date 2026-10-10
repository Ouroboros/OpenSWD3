#include "openswd3/battle/legacy_battle_actor_target_selection_count_query.hpp"

namespace openswd3::battle {

LegacyBattleActorTargetSelectionCountQueryResult
query_legacy_battle_actor_target_selection_count(
    const compat::u16* const target_selection_count,
    const LegacyBattleActorTargetSelectionCountQueryAccess access
) noexcept {
    LegacyBattleActorTargetSelectionCountQueryResult result;
    if (target_selection_count == nullptr || !access.count_readable) {
        result.status = LegacyBattleActorTargetSelectionCountQueryStatus::
            count_read_typed_stop;
        return result;
    }

    result.value = *target_selection_count;
    if (!access.return_address_readable) {
        result.status = LegacyBattleActorTargetSelectionCountQueryStatus::
            return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
