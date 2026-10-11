#include "openswd3/battle/legacy_battle_actor_display_kind.hpp"

namespace openswd3::battle {

LegacyBattleActorDisplayKindResult query_legacy_battle_actor_display_kind(
    const compat::u16* display_kind,
    const LegacyBattleActorDisplayKindAccess access
) noexcept {
    LegacyBattleActorDisplayKindResult result;
    if (display_kind == nullptr || !access.display_kind_readable) {
        result.status =
            LegacyBattleActorDisplayKindStatus::display_kind_read_typed_stop;
        return result;
    }

    result.value = *display_kind;
    if (!access.return_address_readable) {
        result.status =
            LegacyBattleActorDisplayKindStatus::return_address_read_typed_stop;
    }

    return result;
}

}  // namespace openswd3::battle
