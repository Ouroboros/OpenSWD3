#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleGroupAActionExecutionState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorField26b8HighBitSetAddress =
    0x00478780U;
inline constexpr std::array<compat::u32, 27>
    kLegacyBattleActorField26b8HighBitSetClosedCallerAddresses{
        0x004541BEU, 0x00454469U, 0x00454D81U, 0x00458161U, 0x00458BAAU,
        0x00458BD7U, 0x00459404U, 0x00459438U, 0x00459451U, 0x00459513U,
        0x004596B0U, 0x004597F1U, 0x0046DAD8U, 0x0046FB7BU, 0x00472B65U,
        0x004734A3U, 0x00473A94U, 0x00473E0FU, 0x0047470BU, 0x00474943U,
        0x00475035U, 0x0047548FU, 0x00475B19U, 0x00475B98U, 0x00475BE2U,
        0x00475DD6U, 0x00475E2CU,
    };
inline constexpr std::array<compat::u32, 8>
    kLegacyBattleActorField26b8HighBitSetDeferredCallerAddresses{
        0x00478BD6U,
        0x0047E611U,
        0x00480058U,
        0x004811E6U,
        0x00481BC3U,
        0x00481C22U,
        0x00483B0DU,
        0x00483F61U,
    };
inline constexpr std::size_t
    kLegacyBattleActorField26b8HighBitSetMaximumCallTrace = 64U;

struct LegacyBattleActorField26b8HighBitSetOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorField26b8HighBitSetView {
    compat::u32* field_26c0{};
    compat::u32* field_26b8{};
    compat::u16* summon_completion_word{};
    compat::u16* special_target_command_cursor{};
};

[[nodiscard]] LegacyBattleActorField26b8HighBitSetView
resolve_legacy_battle_actor_field_26b8_high_bit_set(
    const LegacyBattleActorField26b8HighBitSetOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorField26b8HighBitSetView
resolve_legacy_battle_actor_field_26b8_high_bit_set(
    LegacyBattleGroupAActionExecutionState* actor, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorField26b8HighBitSetAccess : compat::u8 {
    field_26c0_read,
    field_26b8_read,
    summon_completion_word_write,
    special_target_command_cursor_write,
    field_26b8_write,
};

struct LegacyBattleActorField26b8HighBitSetMemoryAccess {
    bool field_26c0_readable{true};
    bool field_26b8_readable{true};
    bool summon_completion_word_writable{true};
    bool special_target_command_cursor_writable{true};
    bool field_26b8_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorField26b8HighBitSetStatus : compat::u8 {
    completed,
    field_26c0_read_typed_stop,
    field_26b8_read_typed_stop,
    summon_completion_word_write_typed_stop,
    special_target_command_cursor_write_typed_stop,
    field_26b8_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorField26b8HighBitSetRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorField26b8HighBitSetMemoryAccess access{};
};

struct LegacyBattleActorField26b8HighBitSetResult {
    LegacyBattleActorField26b8HighBitSetStatus status{
        LegacyBattleActorField26b8HighBitSetStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorField26b8HighBitSetAddress};
    compat::u32 field_26c0_token{};
    compat::u32 field_26b8_token{};
    compat::u32 summon_completion_word_token{};
    compat::u32 special_target_command_cursor_token{};
    compat::u32 return_address_token{};
    compat::u32 field_26c0_read_value{};
    compat::u32 field_26b8_read_value{};
    compat::u32 field_26b8_write_value{};
    compat::u32 field_26c0_reads{};
    compat::u32 field_26b8_reads{};
    compat::u32 summon_completion_word_writes{};
    compat::u32 special_target_command_cursor_writes{};
    compat::u32 field_26b8_writes{};
    compat::u32 return_address_reads{};
    std::array<LegacyBattleActorField26b8HighBitSetAccess, 5> actor_accesses{};
    compat::u32 actor_access_count{};
    std::array<compat::u32, 1> stack_read_tokens{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    bool gate_blocked{};
    bool high_bit_was_set{};
    bool flags_known{};
    LegacyBattleActorCoordinateFlags flags{};
    bool returned{};
};

struct LegacyBattleActorField26b8HighBitSetCallRequests {
    std::array<
        LegacyBattleActorField26b8HighBitSetRequest,
        kLegacyBattleActorField26b8HighBitSetMaximumCallTrace>
        calls{};
};

struct LegacyBattleActorField26b8HighBitSetCallTrace {
    LegacyBattleActorField26b8HighBitSetResult last{};
    std::array<
        compat::u32,
        kLegacyBattleActorField26b8HighBitSetMaximumCallTrace>
        return_addresses{};
    compat::u32 calls{};
};

void append_legacy_battle_actor_field_26b8_high_bit_set_trace(
    LegacyBattleActorField26b8HighBitSetCallTrace& destination,
    const LegacyBattleActorField26b8HighBitSetCallTrace& nested
) noexcept;

// sub_478780. ECX is the actor token. The function first gates on the full
// actor+0x26C0 dword, conditionally clears two words when actor+0x26B8 bit 31
// was already set, then always ORs bit 31 into actor+0x26B8 and returns with a
// plain RET.
[[nodiscard]] LegacyBattleActorField26b8HighBitSetResult
set_legacy_battle_actor_field_26b8_high_bit(
    LegacyBattleActorField26b8HighBitSetView actor,
    const LegacyBattleActorField26b8HighBitSetRequest& request
) noexcept;

// Shared caller boilerplate. Each call keeps its physical return address in
// the trace; callers remain responsible for mapping a typed stop to their own
// status and suppressing their post-CALL suffix.
[[nodiscard]] bool execute_legacy_battle_actor_field_26b8_high_bit_set_call(
    const LegacyBattleActorField26b8HighBitSetOwners& owners,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_field_26b8_high_bit_set_call(
    const LegacyBattleActorField26b8HighBitSetOwners& owners,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    compat::u32 actor_token,
    compat::u32 return_address
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_field_26b8_high_bit_set_call(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorField26b8HighBitSetCallTrace& trace,
    const LegacyBattleActorField26b8HighBitSetCallRequests& requests,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true
) noexcept;

}  // namespace openswd3::battle
