#pragma once

#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"
#include "openswd3/resource_io/legacy_save_container.hpp"

#include <cstddef>

namespace openswd3::battle {

enum class LegacyBattleSaveFameStatus : compat::u8 {
    ready,
    malformed_groups,
    invalid_existing_chain,
    allocation_failed,
    identity_conflict,
};

struct LegacyBattleSaveFameResult {
    LegacyBattleSaveFameStatus status{LegacyBattleSaveFameStatus::ready};
    std::size_t nodes_published{};
    std::size_t nodes_released{};
};

// sub_478110 followed by sub_477F10. Uses the same fixed-object roots and
// process-wide guest address reservation as other battle allocations.
[[nodiscard]] LegacyBattleSaveFameResult restore_legacy_battle_save_fame(
    const resource_io::LegacySaveContainer& save,
    LegacyBattleFixedObjectState& state
);

}  // namespace openswd3::battle
