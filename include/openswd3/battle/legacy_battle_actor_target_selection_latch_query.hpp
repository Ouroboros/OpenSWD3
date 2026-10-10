#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorTargetSelectionLatchQueryStatus : compat::u8 {
    completed,
    target_selection_latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionLatchQueryAccess {
    bool target_selection_latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionLatchQueryResult {
    LegacyBattleActorTargetSelectionLatchQueryStatus status{
        LegacyBattleActorTargetSelectionLatchQueryStatus::completed
    };
    std::optional<compat::u32> value;
};

[[nodiscard]] LegacyBattleActorTargetSelectionLatchQueryResult
query_legacy_battle_actor_target_selection_latch(
    const compat::u32* target_selection_latch,
    LegacyBattleActorTargetSelectionLatchQueryAccess access = {}
) noexcept;

}  // namespace openswd3::battle
