#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorField26b8HighBitQueryAddress =
    0x004787C0U;
inline constexpr compat::u32
    kLegacyBattleActorField26b8HighBitQueryCallerParentAddress = 0x00453200U;
inline constexpr compat::u32
    kLegacyBattleActorField26b8HighBitQueryCallerAddress = 0x00453409U;
inline constexpr compat::u32
    kLegacyBattleActorField26b8HighBitQueryCallerReturnAddress = 0x0045340EU;

struct LegacyBattleActorField26b8HighBitQueryOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorField26b8HighBitQueryView {
    compat::u32* field_26b8{};
};

[[nodiscard]] LegacyBattleActorField26b8HighBitQueryView
resolve_legacy_battle_actor_field_26b8_high_bit_query(
    const LegacyBattleActorField26b8HighBitQueryOwners& owners,
    compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorField26b8HighBitQueryAccess : compat::u8 {
    field_read,
};

struct LegacyBattleActorField26b8HighBitQueryMemoryAccess {
    bool field_readable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorField26b8HighBitQueryStatus : compat::u8 {
    completed,
    field_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorField26b8HighBitQueryRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    bool entry_overflow_defined{true};
    LegacyBattleActorField26b8HighBitQueryMemoryAccess access{};
};

struct LegacyBattleActorField26b8HighBitQueryResult {
    LegacyBattleActorField26b8HighBitQueryStatus status{
        LegacyBattleActorField26b8HighBitQueryStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorField26b8HighBitQueryAddress};
    compat::u32 field_token{};
    compat::u32 return_address_token{};
    compat::u32 field_read_value{};
    compat::u32 field_reads{};
    compat::u32 return_address_reads{};
    std::array<LegacyBattleActorField26b8HighBitQueryAccess, 1>
        actor_accesses{};
    compat::u32 actor_access_count{};
    std::array<compat::u32, 1> stack_read_tokens{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    bool flags_known{};
    bool overflow_defined{true};
    LegacyBattleActorCoordinateFlags flags{};
    bool returned{};
};

// sub_4787C0. ECX is the actor token. The function reads the complete
// actor+0x26B8 dword, shifts it right by 31, and returns the original bit 31
// as the complete EAX value through a plain RET.
[[nodiscard]] LegacyBattleActorField26b8HighBitQueryResult
query_legacy_battle_actor_field_26b8_high_bit(
    LegacyBattleActorField26b8HighBitQueryView actor,
    const LegacyBattleActorField26b8HighBitQueryRequest& request
) noexcept;

}  // namespace openswd3::battle
