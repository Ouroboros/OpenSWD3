#include "openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp"

namespace openswd3::battle {

LegacyBattleActorTargetSelectionLatchSetStatus
set_legacy_battle_actor_target_selection_latch(
    compat::u32* target_selection_latch,
    const LegacyBattleActorTargetSelectionLatchSetAccess access
) noexcept {
    if (target_selection_latch == nullptr ||
        !access.target_selection_latch_writable) {
        return LegacyBattleActorTargetSelectionLatchSetStatus::
            target_selection_latch_write_typed_stop;
    }

    *target_selection_latch = 1U;
    if (!access.return_address_readable) {
        return LegacyBattleActorTargetSelectionLatchSetStatus::
            return_address_read_typed_stop;
    }

    return LegacyBattleActorTargetSelectionLatchSetStatus::completed;
}

}  // namespace openswd3::battle
