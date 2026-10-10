#include "openswd3/battle/legacy_battle_actor_target_selection_latch_query.hpp"

namespace openswd3::battle {

LegacyBattleActorTargetSelectionLatchQueryResult
query_legacy_battle_actor_target_selection_latch(
    const compat::u32* target_selection_latch,
    const LegacyBattleActorTargetSelectionLatchQueryAccess access
) noexcept {
    LegacyBattleActorTargetSelectionLatchQueryResult result;
    if (target_selection_latch == nullptr ||
        !access.target_selection_latch_readable) {
        result.status = LegacyBattleActorTargetSelectionLatchQueryStatus::
            target_selection_latch_read_typed_stop;
        return result;
    }

    result.value = *target_selection_latch;
    if (!access.return_address_readable) {
        result.status = LegacyBattleActorTargetSelectionLatchQueryStatus::
            return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
