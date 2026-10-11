#include "openswd3/battle/legacy_battle_actor_action_kind.hpp"

namespace openswd3::battle {

LegacyBattleActorActionKindResult query_legacy_battle_actor_action_kind(
    const compat::u16* const action_kind,
    const LegacyBattleActorActionKindAccess access
) noexcept {
    LegacyBattleActorActionKindResult result;
    if (action_kind == nullptr || !access.action_kind_readable) {
        result.status =
            LegacyBattleActorActionKindStatus::action_kind_read_typed_stop;
        return result;
    }

    result.value = *action_kind;
    if (!access.return_address_readable) {
        result.status =
            LegacyBattleActorActionKindStatus::return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
