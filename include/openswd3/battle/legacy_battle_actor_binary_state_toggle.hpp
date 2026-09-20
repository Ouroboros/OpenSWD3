#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorBinaryStateToggleAddress =
    0x00478830U;
inline constexpr compat::u32 kLegacyBattleActorBinaryStateToggleWriteAddress =
    0x0047883DU;
inline constexpr compat::u32 kLegacyBattleActorBinaryStateToggleReturnAddress =
    0x00478843U;
inline constexpr std::array<compat::u32, 2U>
    kLegacyBattleActorBinaryStateToggleCallerAddresses{
        0x0046B04EU,
        0x0046B072U,
    };
inline constexpr std::array<compat::u32, 2U>
    kLegacyBattleActorBinaryStateToggleReturnAddresses{
        0x0046B053U,
        0x0046B077U,
    };

struct LegacyBattleActorBinaryStateToggleOwners {
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorBinaryStateToggleView {
    compat::u32* value{};
};

enum class LegacyBattleActorBinaryStateToggleAccessKind : compat::u8 {
    value_read,
    value_write,
};

struct LegacyBattleActorBinaryStateToggleAccess {
    compat::u32 actor_token{};
    LegacyBattleActorBinaryStateToggleAccessKind kind{};
    compat::u32 value{};
};

struct LegacyBattleActorBinaryStateToggleMemoryAccess {
    bool value_readable{true};
    bool value_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorBinaryStateToggleStatus : compat::u8 {
    completed,
    value_read_typed_stop,
    value_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorBinaryStateToggleRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{};
    LegacyBattleActorBinaryStateToggleMemoryAccess access{};
};

struct LegacyBattleActorBinaryStateToggleResult {
    LegacyBattleActorBinaryStateToggleStatus status{
        LegacyBattleActorBinaryStateToggleStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorBinaryStateToggleAddress};
    compat::u32 value_token{};
    compat::u32 return_address_token{};
    compat::u32 old_value{};
    compat::u32 new_value{};
    std::array<LegacyBattleActorBinaryStateToggleAccess, 2U> actor_accesses{};
    std::size_t actor_access_count{};
    std::array<compat::u32, 1U> stack_read_tokens{};
    std::array<compat::u32, 1U> stack_reads{};
    std::size_t stack_read_count{};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

inline constexpr std::size_t
    kLegacyBattleActorBinaryStateToggleMaximumCallTrace = 8U;

struct LegacyBattleActorBinaryStateToggleCallRequests {
    std::array<
        LegacyBattleActorBinaryStateToggleMemoryAccess,
        kLegacyBattleActorBinaryStateToggleMaximumCallTrace>
        access{};
    std::size_t count{};
};

struct LegacyBattleActorBinaryStateToggleCallTrace {
    LegacyBattleActorBinaryStateToggleResult last{};
    std::array<compat::u32, kLegacyBattleActorBinaryStateToggleMaximumCallTrace>
        call_addresses{};
    std::array<compat::u32, kLegacyBattleActorBinaryStateToggleMaximumCallTrace>
        return_addresses{};
    std::array<compat::u32, kLegacyBattleActorBinaryStateToggleMaximumCallTrace>
        arguments{};
    std::size_t calls{};
};

[[nodiscard]] LegacyBattleActorBinaryStateToggleView
resolve_legacy_battle_actor_binary_state_toggle(
    const LegacyBattleActorBinaryStateToggleOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorBinaryStateToggleResult
toggle_legacy_battle_actor_binary_state(
    LegacyBattleActorBinaryStateToggleView actor,
    const LegacyBattleActorBinaryStateToggleRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_binary_state_toggle_call(
    const LegacyBattleActorBinaryStateToggleOwners& owners,
    LegacyBattleActorBinaryStateToggleCallTrace& trace,
    const LegacyBattleActorBinaryStateToggleCallRequests& requests,
    compat::u32 actor_token,
    compat::u32 argument,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 call_address,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags = {},
    bool entry_flags_known = false
) noexcept;

}  // namespace openswd3::battle
