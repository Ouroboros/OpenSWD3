#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorActionKindAddress = 0x004786B0U;

struct LegacyBattleActorActionKindView {
    const compat::u16* action_kind{};
};

struct LegacyBattleActorActionKindOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorActionKindView
resolve_legacy_battle_actor_action_kind(
    const LegacyBattleActorActionKindOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorActionKindStatus : compat::u8 {
    completed,
    action_kind_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionKindAccess {
    bool action_kind_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorActionKindRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorActionKindAccess access{};
};

struct LegacyBattleActorActionKindResult {
    LegacyBattleActorActionKindStatus status{
        LegacyBattleActorActionKindStatus::completed
    };
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 field_token{};
    compat::u32 action_kind_reads{};
    compat::u32 return_address_reads{};
    compat::u32 stack_read_count{};
    bool returned{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004786B0. The getter replaces only AX with the
// word at actor + 0x2A6C and then performs a plain RET without changing ECX,
// EDX, or flags.
[[nodiscard]] LegacyBattleActorActionKindResult
query_legacy_battle_actor_action_kind(
    LegacyBattleActorActionKindView actor,
    const LegacyBattleActorActionKindRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
