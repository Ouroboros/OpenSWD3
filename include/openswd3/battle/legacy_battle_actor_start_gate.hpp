#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorStartGateStatus : compat::u8 {
    completed,
    start_gate_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorStartGateAccess {
    bool start_gate_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorStartGateResult {
    LegacyBattleActorStartGateStatus status{
        LegacyBattleActorStartGateStatus::completed
    };
    std::optional<compat::u16> value;
};

[[nodiscard]] LegacyBattleActorStartGateResult
query_legacy_battle_actor_start_gate(
    const compat::u16* start_gate, LegacyBattleActorStartGateAccess access = {}
) noexcept;

}  // namespace openswd3::battle
