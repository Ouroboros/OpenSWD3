#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleGroupAActionExecutionState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorEffectResourceSlotWriteAddress =
    0x004787D0U;
inline constexpr std::size_t kLegacyBattleActorEffectResourceSlotCount = 35U;
inline constexpr std::array<compat::u32, 40>
    kLegacyBattleActorEffectResourceSlotWriteCallerAddresses{
        0x00453B85U, 0x0045815AU, 0x00458C83U, 0x00458CC1U, 0x0045958CU,
        0x004595DBU, 0x0045971AU, 0x00459769U, 0x00459879U, 0x004598C3U,
        0x0045C9E7U, 0x0045CB3BU, 0x0045CD86U, 0x0045CF55U, 0x0045D6C7U,
        0x0045D72FU, 0x0045D744U, 0x0045D7B5U, 0x0045D7CAU, 0x0046DB01U,
        0x0046EEDFU, 0x0046EF72U, 0x0046EFFEU, 0x00473508U, 0x00473533U,
        0x00475BEEU, 0x00475E25U, 0x0047E5D2U, 0x0047E5E8U, 0x00481061U,
        0x0048130DU, 0x004818D2U, 0x004818F9U, 0x00481924U, 0x0048194FU,
        0x004819D1U, 0x004819E5U, 0x00481A6DU, 0x00481EF8U, 0x00481F19U,
    };
inline constexpr std::array<compat::u32, 40>
    kLegacyBattleActorEffectResourceSlotWriteReturnAddresses{
        0x00453B8AU, 0x0045815FU, 0x00458C88U, 0x00458CC6U, 0x00459591U,
        0x004595E0U, 0x0045971FU, 0x0045976EU, 0x0045987EU, 0x004598C8U,
        0x0045C9ECU, 0x0045CB40U, 0x0045CD8BU, 0x0045CF5AU, 0x0045D6CCU,
        0x0045D734U, 0x0045D749U, 0x0045D7BAU, 0x0045D7CFU, 0x0046DB06U,
        0x0046EEE4U, 0x0046EF77U, 0x0046F003U, 0x0047350DU, 0x00473538U,
        0x00475BF3U, 0x00475E2AU, 0x0047E5D7U, 0x0047E5EDU, 0x00481066U,
        0x00481312U, 0x004818D7U, 0x004818FEU, 0x00481929U, 0x00481954U,
        0x004819D6U, 0x004819EAU, 0x00481A72U, 0x00481EFDU, 0x00481F1EU,
    };
inline constexpr std::size_t
    kLegacyBattleActorEffectResourceSlotWriteMaximumCallTrace = 64U;

struct LegacyBattleActorEffectResourceSlotWriteOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorEffectResourceSlotWriteView {
    std::array<compat::u16, kLegacyBattleActorEffectResourceSlotCount>* slots{};
    compat::u16* cursor{};
    compat::u32* execution_complete{};
};

[[nodiscard]] LegacyBattleActorEffectResourceSlotWriteView
resolve_legacy_battle_actor_effect_resource_slot_write(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorEffectResourceSlotWriteView
resolve_legacy_battle_actor_effect_resource_slot_write(
    LegacyBattleGroupAActionExecutionState* actor, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorEffectResourceSlotWriteAccess : compat::u8 {
    cursor_read,
    target_write,
};

struct LegacyBattleActorEffectResourceSlotWriteMemoryAccess {
    bool argument_readable{true};
    bool cursor_readable{true};
    bool target_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorEffectResourceSlotWriteStatus : compat::u8 {
    completed,
    argument_read_typed_stop,
    cursor_read_typed_stop,
    target_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorEffectResourceSlotWriteRequest {
    compat::u32 actor_token{};
    compat::u16 value{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    bool entry_overflow_defined{true};
    LegacyBattleActorEffectResourceSlotWriteMemoryAccess access{};
};

struct LegacyBattleActorEffectResourceSlotWriteResult {
    LegacyBattleActorEffectResourceSlotWriteStatus status{
        LegacyBattleActorEffectResourceSlotWriteStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorEffectResourceSlotWriteAddress};
    compat::u32 argument_token{};
    compat::u32 cursor_token{};
    compat::u32 target_token{};
    compat::u32 return_address_token{};
    compat::u16 argument_value{};
    compat::u16 cursor_value{};
    compat::u16 target_write_value{};
    compat::u32 argument_reads{};
    compat::u32 cursor_reads{};
    compat::u32 target_writes{};
    compat::u32 return_address_reads{};
    std::array<LegacyBattleActorEffectResourceSlotWriteAccess, 2>
        actor_accesses{};
    compat::u32 actor_access_count{};
    std::array<compat::u32, 2> stack_read_tokens{};
    std::array<compat::u32, 2> stack_reads{};
    compat::u32 stack_read_count{};
    bool flags_known{};
    bool overflow_defined{true};
    LegacyBattleActorCoordinateFlags flags{};
    bool returned{};
};

struct LegacyBattleActorEffectResourceSlotWriteCallRequests {
    std::array<
        LegacyBattleActorEffectResourceSlotWriteRequest,
        kLegacyBattleActorEffectResourceSlotWriteMaximumCallTrace>
        calls{};
};

struct LegacyBattleActorEffectResourceSlotWriteCallTrace {
    LegacyBattleActorEffectResourceSlotWriteResult last{};
    std::array<
        compat::u32,
        kLegacyBattleActorEffectResourceSlotWriteMaximumCallTrace>
        call_addresses{};
    std::array<
        compat::u32,
        kLegacyBattleActorEffectResourceSlotWriteMaximumCallTrace>
        return_addresses{};
    compat::u32 calls{};
};

void append_legacy_battle_actor_effect_resource_slot_write_trace(
    LegacyBattleActorEffectResourceSlotWriteCallTrace& destination,
    const LegacyBattleActorEffectResourceSlotWriteCallTrace& nested
) noexcept;

// sub_4787D0. The function reads a word argument into DX, clears EAX, reads the
// actor+0x2A7C cursor into AX, writes DX to actor+0x29C4+2*EAX, and executes
// RETN 4.
[[nodiscard]] LegacyBattleActorEffectResourceSlotWriteResult
write_legacy_battle_actor_effect_resource_slot(
    LegacyBattleActorEffectResourceSlotWriteView actor,
    const LegacyBattleActorEffectResourceSlotWriteRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_effect_resource_slot_write_call(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    LegacyBattleActorEffectResourceSlotWriteCallTrace& trace,
    const LegacyBattleActorEffectResourceSlotWriteCallRequests& requests,
    compat::u32 actor_token,
    compat::u16 value,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 call_address,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true,
    bool entry_overflow_defined = true
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_effect_resource_slot_write_call(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorEffectResourceSlotWriteCallTrace& trace,
    const LegacyBattleActorEffectResourceSlotWriteCallRequests& requests,
    compat::u32 actor_token,
    compat::u16 value,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 call_address,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true,
    bool entry_overflow_defined = true
) noexcept;

void reset_legacy_battle_actor_effect_resource_slots(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    compat::u32 actor_token
) noexcept;

void reset_legacy_battle_actor_effect_resource_slots(
    LegacyBattleGroupAActionExecutionState* actor
) noexcept;

void synchronize_legacy_battle_actor_effect_resource_cursor_update(
    const LegacyBattleActorEffectResourceSlotWriteOwners& owners,
    compat::u32 actor_token,
    compat::u32 mode
) noexcept;

void synchronize_legacy_battle_actor_effect_resource_cursor_update(
    LegacyBattleGroupAActionExecutionState* actor, compat::u32 mode
) noexcept;

}  // namespace openswd3::battle
