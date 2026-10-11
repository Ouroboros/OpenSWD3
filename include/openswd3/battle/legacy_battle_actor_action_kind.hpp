#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorActionKindStatus : compat::u8 {
    completed,
    action_kind_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionKindAccess {
    bool action_kind_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorActionKindResult {
    LegacyBattleActorActionKindStatus status{
        LegacyBattleActorActionKindStatus::completed
    };
    std::optional<compat::u16> value;
};

[[nodiscard]] LegacyBattleActorActionKindResult
query_legacy_battle_actor_action_kind(
    const compat::u16* action_kind,
    LegacyBattleActorActionKindAccess access = {}
) noexcept;

}  // namespace openswd3::battle
