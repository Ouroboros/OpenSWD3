#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorTargetSelectionCountQueryStatus : compat::u8 {
    completed,
    count_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionCountQueryAccess {
    bool count_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionCountQueryResult {
    LegacyBattleActorTargetSelectionCountQueryStatus status{
        LegacyBattleActorTargetSelectionCountQueryStatus::completed
    };
    std::optional<compat::u16> value;
};

[[nodiscard]] LegacyBattleActorTargetSelectionCountQueryResult
query_legacy_battle_actor_target_selection_count(
    const compat::u16* target_selection_count,
    LegacyBattleActorTargetSelectionCountQueryAccess access = {}
) noexcept;

}  // namespace openswd3::battle
