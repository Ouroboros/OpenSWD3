#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorStartGateLatchQueryStatus : compat::u8 {
    completed,
    start_gate_latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorStartGateLatchQueryAccess {
    bool start_gate_latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorStartGateLatchQueryResult {
    LegacyBattleActorStartGateLatchQueryStatus status{
        LegacyBattleActorStartGateLatchQueryStatus::completed
    };
    std::optional<compat::u32> value{};
};

[[nodiscard]] LegacyBattleActorStartGateLatchQueryResult
query_legacy_battle_actor_start_gate_latch(
    const compat::u32* start_gate_latch,
    LegacyBattleActorStartGateLatchQueryAccess access = {}
) noexcept;

}  // namespace openswd3::battle
