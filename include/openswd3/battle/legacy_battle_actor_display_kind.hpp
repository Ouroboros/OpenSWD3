#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorDisplayKindAddress = 0x004786C0U;

struct LegacyBattleActorDisplayKindView {
    const compat::u16* display_kind{};
};

struct LegacyBattleActorDisplayKindOwners {
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorDisplayKindView
resolve_legacy_battle_actor_display_kind(
    const LegacyBattleActorDisplayKindOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorDisplayKindStatus : compat::u8 {
    completed,
    display_kind_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorDisplayKindAccess {
    bool display_kind_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorDisplayKindRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorDisplayKindAccess access{};
};

struct LegacyBattleActorDisplayKindResult {
    LegacyBattleActorDisplayKindStatus status{
        LegacyBattleActorDisplayKindStatus::completed
    };
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 field_token{};
    compat::u32 display_kind_reads{};
    compat::u32 return_address_reads{};
    compat::u32 stack_read_count{};
    bool returned{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004786C0. The getter replaces only AX with the
// word at actor + 0x2A70 and then performs a plain RET without changing ECX,
// EDX, or flags.
[[nodiscard]] LegacyBattleActorDisplayKindResult
query_legacy_battle_actor_display_kind(
    LegacyBattleActorDisplayKindView actor,
    const LegacyBattleActorDisplayKindRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
