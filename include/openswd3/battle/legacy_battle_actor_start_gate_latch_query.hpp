#pragma once

#include "openswd3/battle/legacy_battle_actor_start_gate_increment.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorStartGateLatchQueryAddress =
    0x00478B50U;
inline constexpr compat::u32
    kLegacyBattleActorStartGateLatchQueryReturnInstruction = 0x00478B56U;

inline constexpr std::array<compat::u32, 1>
    kLegacyBattleActorStartGateLatchQueryCallAddresses{
        0x00457842U,
    };

inline constexpr std::array<compat::u32, 1>
    kLegacyBattleActorStartGateLatchQueryReturnAddresses{
        0x00457847U,
    };

enum class LegacyBattleActorStartGateLatchQueryStatus : compat::u8 {
    completed,
    start_gate_latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorStartGateLatchQueryAccess {
    bool start_gate_latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorStartGateLatchQueryRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorStartGateLatchQueryAccess access{};
};

struct LegacyBattleActorStartGateLatchQueryResult {
    LegacyBattleActorStartGateLatchQueryStatus status{
        LegacyBattleActorStartGateLatchQueryStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 start_gate_latch_field_token{};
    compat::u32 start_gate_latch_reads{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorStartGateLatchQueryAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorStartGateLatchQueryCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorStartGateLatchQueryRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorStartGateLatchQueryTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorStartGateLatchQueryResult last{};
};

[[nodiscard]] LegacyBattleActorStartGateLatchQueryResult
query_legacy_battle_actor_start_gate_latch(
    LegacyBattleActorStartGateIncrementView actor,
    const LegacyBattleActorStartGateLatchQueryRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_start_gate_latch_query_call(
    LegacyBattleActorStartGateLatchQueryTrace& trace,
    const LegacyBattleActorStartGateLatchQueryCallRequests& requests,
    const LegacyBattleActorStartGateIncrementOwners& owners,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true,
    std::size_t request_offset = 0U
) noexcept;

}  // namespace openswd3::battle
