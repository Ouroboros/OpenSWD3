#pragma once

#include "openswd3/compat/types.hpp"

#include <optional>

namespace openswd3::battle {

enum class LegacyBattleActorStartGateIncrementStatus : compat::u8 {
    completed,
    start_gate_read_typed_stop,
    start_gate_write_typed_stop,
    start_gate_latch_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorStartGateIncrementAccess {
    bool start_gate_readable{true};
    bool start_gate_writable{true};
    bool start_gate_latch_writable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorStartGateIncrementResult {
    LegacyBattleActorStartGateIncrementStatus status{
        LegacyBattleActorStartGateIncrementStatus::completed
    };
    std::optional<compat::u16> previous_start_gate;
    std::optional<compat::u16> incremented_start_gate;
    bool start_gate_latch_committed{};
};

[[nodiscard]] LegacyBattleActorStartGateIncrementResult
increment_legacy_battle_actor_start_gate(
    compat::u16* start_gate,
    compat::u32* start_gate_latch,
    LegacyBattleActorStartGateIncrementAccess access = {}
) noexcept;

}  // namespace openswd3::battle
