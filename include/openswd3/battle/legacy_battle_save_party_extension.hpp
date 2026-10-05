#pragma once

#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/resource_io/legacy_save_container.hpp"

namespace openswd3::battle {

// 0x00408A52–0x00408A6E: four adjacent 0x60-byte group-A source records.
// Call only with a complete save container; this does not enter battle.
void restore_legacy_save_party_extension_b(
    const resource_io::LegacySaveContainer& save,
    LegacyBattleStartupState& battle
) noexcept;

}  // namespace openswd3::battle
