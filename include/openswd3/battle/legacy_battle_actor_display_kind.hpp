#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorDisplayKindStatus : compat::u8 {
    completed,
    display_kind_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorDisplayKindAccess {
    bool display_kind_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorDisplayKindResult {
    LegacyBattleActorDisplayKindStatus status{
        LegacyBattleActorDisplayKindStatus::completed
    };
    std::optional<compat::u16> value;
};

[[nodiscard]] LegacyBattleActorDisplayKindResult
query_legacy_battle_actor_display_kind(
    const compat::u16* display_kind,
    LegacyBattleActorDisplayKindAccess access = {}
) noexcept;

}  // namespace openswd3::battle
