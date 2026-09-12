#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorStartGateAddress = 0x004786D0U;

struct LegacyBattleActorStartGateView {
    const compat::u16* start_gate{};
};

struct LegacyBattleActorStartGateOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorStartGateView
resolve_legacy_battle_actor_start_gate(
    const LegacyBattleActorStartGateOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorStartGateStatus : compat::u8 {
    completed,
    start_gate_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorStartGateAccess {
    bool start_gate_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorStartGateRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorStartGateAccess access{};
};

struct LegacyBattleActorStartGateResult {
    LegacyBattleActorStartGateStatus status{
        LegacyBattleActorStartGateStatus::completed
    };
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 field_token{};
    compat::u32 start_gate_reads{};
    compat::u32 return_address_reads{};
    compat::u32 stack_read_count{};
    bool returned{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004786D0. The getter replaces only AX with the
// word at actor + 0x2A74 and then performs a plain RET without changing ECX,
// EDX, or flags.
[[nodiscard]] LegacyBattleActorStartGateResult
query_legacy_battle_actor_start_gate(
    LegacyBattleActorStartGateView actor,
    const LegacyBattleActorStartGateRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
