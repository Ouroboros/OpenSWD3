#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

enum class LegacyBattleActorTargetSelectionLatchSetStatus : compat::u8 {
    completed,
    target_selection_latch_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionLatchSetAccess {
    bool target_selection_latch_writable{true};
    bool return_address_readable{true};
};

[[nodiscard]] LegacyBattleActorTargetSelectionLatchSetStatus
set_legacy_battle_actor_target_selection_latch(
    compat::u32* target_selection_latch,
    LegacyBattleActorTargetSelectionLatchSetAccess access = {}
) noexcept;

}  // namespace openswd3::battle
