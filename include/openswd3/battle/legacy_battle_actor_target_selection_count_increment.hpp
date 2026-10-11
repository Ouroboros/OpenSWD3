#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorTargetSelectionCountIncrementStatus : compat::u8 {
    completed,
    count_read_typed_stop,
    count_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionCountIncrementAccess {
    bool count_readable{true};
    bool count_writable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionCountIncrementResult {
    LegacyBattleActorTargetSelectionCountIncrementStatus status{
        LegacyBattleActorTargetSelectionCountIncrementStatus::completed
    };
    std::optional<compat::u16> previous_value;
    std::optional<compat::u16> incremented_value;
};

[[nodiscard]] LegacyBattleActorTargetSelectionCountIncrementResult
increment_legacy_battle_actor_target_selection_count(
    compat::u16* target_selection_count,
    LegacyBattleActorTargetSelectionCountIncrementAccess access = {}
) noexcept;

}  // namespace openswd3::battle
