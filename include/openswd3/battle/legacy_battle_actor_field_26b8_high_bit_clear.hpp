#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorField26b8HighBitClearMemoryAccess {
    bool field_readable{true};
    bool field_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorField26b8HighBitClearStatus : compat::u8 {
    completed,
    field_read_typed_stop,
    field_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorField26b8HighBitClearResult {
    LegacyBattleActorField26b8HighBitClearStatus status{
        LegacyBattleActorField26b8HighBitClearStatus::completed
    };
};

[[nodiscard]] LegacyBattleActorField26b8HighBitClearResult
clear_legacy_battle_actor_field_26b8_high_bit(
    compat::u32* field_26b8,
    LegacyBattleActorField26b8HighBitClearMemoryAccess access = {}
) noexcept;

}  // namespace openswd3::battle
