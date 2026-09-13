#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorActionModeSetAddress =
    0x00478710U;

struct LegacyBattleActorActionModeOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorActionModeView {
    compat::u16* action_kind{};
    compat::u16* display_kind{};
    compat::u8* mode_flags{};
};

[[nodiscard]] LegacyBattleActorActionModeView
resolve_legacy_battle_actor_action_mode(
    const LegacyBattleActorActionModeOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorActionModeAccess : compat::u8 {
    mode_flags_read,
    mode_flags_write,
    display_kind_write,
    action_kind_write,
};

struct LegacyBattleActorActionModeMemoryAccess {
    bool argument_readable{true};
    bool mode_flags_readable{true};
    bool mode_flags_writable{true};
    bool display_kind_writable{true};
    bool action_kind_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorActionModeStatus : compat::u8 {
    completed,
    argument_read_typed_stop,
    mode_flags_read_typed_stop,
    mode_flags_write_typed_stop,
    display_kind_write_typed_stop,
    action_kind_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionModeRequest {
    compat::u32 actor_token{};
    compat::u32 mode{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorActionModeMemoryAccess access{};
};

struct LegacyBattleActorActionModeResult {
    LegacyBattleActorActionModeStatus status{
        LegacyBattleActorActionModeStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorActionModeSetAddress};
    compat::u32 argument_token{};
    compat::u32 mode_flags_token{};
    compat::u32 display_kind_token{};
    compat::u32 action_kind_token{};
    compat::u32 argument_reads{};
    compat::u32 mode_flags_reads{};
    compat::u32 mode_flags_writes{};
    compat::u32 display_kind_writes{};
    compat::u32 action_kind_writes{};
    compat::u32 return_address_reads{};
    std::array<compat::u32, 2> stack_read_tokens{};
    std::array<compat::u32, 2> stack_reads{};
    compat::u32 stack_read_count{};
    std::array<LegacyBattleActorActionModeAccess, 4> actor_accesses{};
    compat::u32 actor_access_count{};
    bool flags_known{};
    LegacyBattleActorCoordinateFlags flags{};
    bool returned{};
};

// sub_478710. ECX is the actor token; the sole dword argument is read from
// [ESP+4], EAX is forced to one on the completed write path, and RETN 4 pops
// both the return address and argument.
[[nodiscard]] LegacyBattleActorActionModeResult
set_legacy_battle_actor_action_mode(
    LegacyBattleActorActionModeView actor,
    const LegacyBattleActorActionModeRequest& request
) noexcept;

}  // namespace openswd3::battle
