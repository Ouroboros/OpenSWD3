#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorTurnCompletionAddress =
    0x00478690U;

struct LegacyBattleActorTurnCompletionView {
    const compat::u32* latch{};
};

struct LegacyBattleActorTurnCompletionOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorTurnCompletionView
resolve_legacy_battle_actor_turn_completion(
    const LegacyBattleActorTurnCompletionOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorTurnCompletionStatus : compat::u8 {
    completed,
    latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTurnCompletionAccess {
    bool latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTurnCompletionRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorTurnCompletionAccess access{};
};

struct LegacyBattleActorTurnCompletionResult {
    LegacyBattleActorTurnCompletionStatus status{
        LegacyBattleActorTurnCompletionStatus::completed
    };
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 field_token{};
    compat::u32 latch_reads{};
    compat::u32 return_address_reads{};
    compat::u32 stack_read_count{};
    bool returned{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x00478690. The getter reads the complete dword at
// actor + 0x2AAC and then performs a plain RET without changing ECX, EDX, or
// flags.
[[nodiscard]] LegacyBattleActorTurnCompletionResult
query_legacy_battle_actor_turn_completion(
    LegacyBattleActorTurnCompletionView actor,
    const LegacyBattleActorTurnCompletionRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
