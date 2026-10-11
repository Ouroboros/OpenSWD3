#include "openswd3/battle/legacy_battle_actor_target_selection_count_increment.hpp"

namespace openswd3::battle {

LegacyBattleActorTargetSelectionCountIncrementResult
increment_legacy_battle_actor_target_selection_count(
    compat::u16* target_selection_count,
    const LegacyBattleActorTargetSelectionCountIncrementAccess access
) noexcept {
    LegacyBattleActorTargetSelectionCountIncrementResult result;
    if (target_selection_count == nullptr || !access.count_readable) {
        result.status = LegacyBattleActorTargetSelectionCountIncrementStatus::
            count_read_typed_stop;
        return result;
    }

    result.previous_value = *target_selection_count;
    const compat::u16 incremented =
        static_cast<compat::u16>(*result.previous_value + 1U);
    if (!access.count_writable) {
        result.status = LegacyBattleActorTargetSelectionCountIncrementStatus::
            count_write_typed_stop;
        return result;
    }

    *target_selection_count = incremented;
    result.incremented_value = incremented;
    if (!access.return_address_readable) {
        result.status = LegacyBattleActorTargetSelectionCountIncrementStatus::
            return_address_read_typed_stop;
        return result;
    }

    return result;
}

}  // namespace openswd3::battle
