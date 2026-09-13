#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorField26b8HighBitClearAddress =
    0x00478770U;
inline constexpr compat::u32
    kLegacyBattleActorField26b8HighBitClearDeferredParentAddress = 0x00478B60U;
inline constexpr compat::u32
    kLegacyBattleActorField26b8HighBitClearCallerAddress = 0x00478CC8U;
inline constexpr compat::u32
    kLegacyBattleActorField26b8HighBitClearCallerReturnAddress = 0x00478CCDU;

struct LegacyBattleActorField26b8HighBitClearOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorField26b8HighBitClearView {
    compat::u32* field_26b8{};
};

[[nodiscard]] LegacyBattleActorField26b8HighBitClearView
resolve_legacy_battle_actor_field_26b8_high_bit_clear(
    const LegacyBattleActorField26b8HighBitClearOwners& owners,
    compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorField26b8HighBitClearAccess : compat::u8 {
    field_read,
    field_write,
};

struct LegacyBattleActorField26b8HighBitClearMemoryAccess {
    bool field_readable{true};
    bool field_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorField26b8HighBitClearStatus : compat::u8 {
    completed,
    field_read_typed_stop,
    field_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorField26b8HighBitClearRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorField26b8HighBitClearMemoryAccess access{};
};

struct LegacyBattleActorField26b8HighBitClearResult {
    LegacyBattleActorField26b8HighBitClearStatus status{
        LegacyBattleActorField26b8HighBitClearStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorField26b8HighBitClearAddress};
    compat::u32 field_token{};
    compat::u32 return_address_token{};
    compat::u32 field_read_value{};
    compat::u32 field_write_value{};
    compat::u32 field_reads{};
    compat::u32 field_writes{};
    compat::u32 return_address_reads{};
    std::array<LegacyBattleActorField26b8HighBitClearAccess, 2>
        actor_accesses{};
    compat::u32 actor_access_count{};
    std::array<compat::u32, 1> stack_read_tokens{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    bool flags_known{};
    LegacyBattleActorCoordinateFlags flags{};
    bool returned{};
};

// sub_478770. ECX is the actor token. The function always performs the
// actor+0x26B8 dword read-modify-write, clears only bit 31, and returns with a
// plain RET while preserving EAX, ECX, and EDX.
[[nodiscard]] LegacyBattleActorField26b8HighBitClearResult
clear_legacy_battle_actor_field_26b8_high_bit(
    LegacyBattleActorField26b8HighBitClearView actor,
    const LegacyBattleActorField26b8HighBitClearRequest& request
) noexcept;

}  // namespace openswd3::battle
