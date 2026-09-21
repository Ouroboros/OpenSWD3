#pragma once

#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorStartGateIncrementAddress =
    0x00478AC0U;
inline constexpr compat::u32 kLegacyBattleActorStartGateIncrementEndAddress =
    0x00478AD1U;

inline constexpr std::array<compat::u32, 5>
    kLegacyBattleActorStartGateIncrementCallAddresses{
        0x00456FE1U,
        0x0045707AU,
        0x0045786BU,
        0x00457E1CU,
        0x0046DD4AU,
    };

inline constexpr std::array<compat::u32, 5>
    kLegacyBattleActorStartGateIncrementReturnAddresses{
        0x00456FE6U,
        0x0045707FU,
        0x00457870U,
        0x00457E21U,
        0x0046DD4FU,
    };

struct LegacyBattleActorStartGateIncrementView {
    compat::u16* start_gate{};
    compat::u32* start_gate_latch{};
};

struct LegacyBattleActorStartGateIncrementOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

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

struct LegacyBattleActorStartGateIncrementRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorStartGateIncrementAccess access{};
};

struct LegacyBattleActorStartGateIncrementResult {
    LegacyBattleActorStartGateIncrementStatus status{
        LegacyBattleActorStartGateIncrementStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 start_gate_field_token{};
    compat::u32 start_gate_latch_field_token{};
    compat::u16 previous_start_gate{};
    compat::u16 incremented_start_gate{};
    compat::u32 start_gate_reads{};
    compat::u32 start_gate_writes{};
    compat::u32 start_gate_latch_writes{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorStartGateIncrementAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorStartGateIncrementCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorStartGateIncrementRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorStartGateIncrementTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorStartGateIncrementResult last{};
};

[[nodiscard]] LegacyBattleActorStartGateIncrementView
resolve_legacy_battle_actor_start_gate_increment(
    const LegacyBattleActorStartGateIncrementOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorStartGateIncrementResult
increment_legacy_battle_actor_start_gate(
    LegacyBattleActorStartGateIncrementView actor,
    const LegacyBattleActorStartGateIncrementRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_start_gate_increment_call(
    LegacyBattleActorStartGateIncrementTrace& trace,
    const LegacyBattleActorStartGateIncrementCallRequests& requests,
    const LegacyBattleActorStartGateIncrementOwners& owners,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true
) noexcept;

}  // namespace openswd3::battle
