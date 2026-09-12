#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleDebugHotkeyState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorActionTargetAddress =
    0x004786E0U;
inline constexpr compat::u32 kLegacyBattleDebugSpecialActorToken = 0x004E80FCU;

struct LegacyBattleActorActionTargetState {
    compat::u16 action_target{};  // actor + 0x29A2
};

struct LegacyBattleActorActionTargetView {
    compat::u16* action_target{};
};

struct LegacyBattleActorActionTargetOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
    LegacyBattleDebugHotkeyState* debug_hotkeys{};
};

[[nodiscard]] LegacyBattleActorActionTargetView
resolve_legacy_battle_actor_action_target(
    const LegacyBattleActorActionTargetOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorActionTargetStatus : compat::u8 {
    completed,
    action_target_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionTargetAccess {
    bool action_target_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorActionTargetRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorActionTargetAccess access{};
};

struct LegacyBattleActorActionTargetResult {
    LegacyBattleActorActionTargetStatus status{
        LegacyBattleActorActionTargetStatus::completed
    };
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 field_token{};
    compat::u32 action_target_reads{};
    compat::u32 return_address_reads{};
    compat::u32 stack_read_count{};
    bool returned{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004786E0. The getter replaces only AX with the
// word at actor + 0x29A2 and then performs a plain RET without changing ECX,
// EDX, or flags.
[[nodiscard]] LegacyBattleActorActionTargetResult
query_legacy_battle_actor_action_target(
    LegacyBattleActorActionTargetView actor,
    const LegacyBattleActorActionTargetRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
