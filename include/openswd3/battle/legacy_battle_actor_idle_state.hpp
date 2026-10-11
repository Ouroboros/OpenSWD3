#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorIdleStateStatus : compat::u8 {
    completed,
    latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorIdleStateAccess {
    bool latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorIdleStateResult {
    LegacyBattleActorIdleStateStatus status{
        LegacyBattleActorIdleStateStatus::completed
    };
    std::optional<compat::u32> value;
};

[[nodiscard]] LegacyBattleActorIdleStateResult
query_legacy_battle_actor_idle_state(
    const compat::u32* idle_state, LegacyBattleActorIdleStateAccess access = {}
) noexcept;

}  // namespace openswd3::battle
