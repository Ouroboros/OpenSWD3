#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorPresentationActivationAddress =
    0x004787F0U;
inline constexpr std::array<compat::u32, 7>
    kLegacyBattleActorPresentationActivationCallerAddresses{
        0x004540E4U,
        0x004546B5U,
        0x0045472EU,
        0x00456286U,
        0x00456460U,
        0x0046AF55U,
        0x0046AFA8U,
    };
inline constexpr std::array<compat::u32, 7>
    kLegacyBattleActorPresentationActivationReturnAddresses{
        0x004540E9U,
        0x004546BAU,
        0x00454733U,
        0x0045628BU,
        0x00456465U,
        0x0046AF5AU,
        0x0046AFADU,
    };
inline constexpr std::size_t
    kLegacyBattleActorPresentationActivationMaximumCallTrace = 16U;

struct LegacyBattleActorPresentationActivationOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorPresentationActivationView {
    compat::u32* special_ready{};
    compat::u8* marker{};
    compat::u32* presentation_enabled{};
    compat::u32* source_runtime_value{};
    compat::u32* live_record_token{};
    compat::u32* live_record_value_04{};
};

[[nodiscard]] LegacyBattleActorPresentationActivationView
resolve_legacy_battle_actor_presentation_activation(
    const LegacyBattleActorPresentationActivationOwners& owners,
    compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorPresentationActivationAccess : compat::u8 {
    special_ready_write,
    marker_read,
    presentation_enabled_write,
    marker_write,
    source_runtime_value_read,
    live_record_token_read,
    live_record_value_write,
};

struct LegacyBattleActorPresentationActivationMemoryAccess {
    bool argument_readable{true};
    bool special_ready_writable{true};
    bool marker_readable{true};
    bool presentation_enabled_writable{true};
    bool marker_writable{true};
    bool source_runtime_value_readable{true};
    bool live_record_token_readable{true};
    bool live_record_value_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorPresentationActivationStatus : compat::u8 {
    completed,
    argument_read_typed_stop,
    special_ready_write_typed_stop,
    marker_read_typed_stop,
    presentation_enabled_write_typed_stop,
    marker_write_typed_stop,
    source_runtime_value_read_typed_stop,
    live_record_token_read_typed_stop,
    live_record_value_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorPresentationActivationRequest {
    compat::u32 actor_token{};
    compat::u32 value{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorPresentationActivationMemoryAccess access{};
};

struct LegacyBattleActorPresentationActivationResult {
    LegacyBattleActorPresentationActivationStatus status{
        LegacyBattleActorPresentationActivationStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorPresentationActivationAddress};
    compat::u32 argument_token{};
    compat::u32 special_ready_token{};
    compat::u32 marker_token{};
    compat::u32 presentation_enabled_token{};
    compat::u32 source_runtime_value_token{};
    compat::u32 live_record_token_token{};
    compat::u32 live_record_value_token{};
    compat::u32 return_address_token{};
    compat::u32 argument_value{};
    compat::u8 original_marker{};
    compat::u32 source_runtime_value{};
    compat::u32 resolved_live_record_token{};
    compat::u32 argument_reads{};
    compat::u32 special_ready_writes{};
    compat::u32 marker_reads{};
    compat::u32 presentation_enabled_writes{};
    compat::u32 marker_writes{};
    compat::u32 source_runtime_value_reads{};
    compat::u32 live_record_token_reads{};
    compat::u32 live_record_value_writes{};
    compat::u32 return_address_reads{};
    std::array<LegacyBattleActorPresentationActivationAccess, 7>
        actor_accesses{};
    compat::u32 actor_access_count{};
    std::array<compat::u32, 2> stack_read_tokens{};
    std::array<compat::u32, 2> stack_reads{};
    compat::u32 stack_read_count{};
    bool flags_known{};
    LegacyBattleActorCoordinateFlags flags{};
    bool returned{};
};

struct LegacyBattleActorPresentationActivationCallRequests {
    std::array<
        LegacyBattleActorPresentationActivationRequest,
        kLegacyBattleActorPresentationActivationMaximumCallTrace>
        calls{};
};

struct LegacyBattleActorPresentationActivationCallTrace {
    LegacyBattleActorPresentationActivationResult last{};
    std::array<
        compat::u32,
        kLegacyBattleActorPresentationActivationMaximumCallTrace>
        call_addresses{};
    std::array<
        compat::u32,
        kLegacyBattleActorPresentationActivationMaximumCallTrace>
        return_addresses{};
    compat::u32 calls{};
};

// sub_4787F0. ECX is the actor token. The sole dword argument is stored at
// actor+0x2AB8 before the remaining actor fields are read or written, and
// RETN 4 consumes both the return address and argument.
[[nodiscard]] LegacyBattleActorPresentationActivationResult
activate_legacy_battle_actor_presentation(
    LegacyBattleActorPresentationActivationView actor,
    const LegacyBattleActorPresentationActivationRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_presentation_activation_call(
    const LegacyBattleActorPresentationActivationOwners& owners,
    LegacyBattleActorPresentationActivationCallTrace& trace,
    const LegacyBattleActorPresentationActivationCallRequests& requests,
    compat::u32 actor_token,
    compat::u32 value,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    compat::u32 call_address,
    compat::u32 return_address,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true
) noexcept;

}  // namespace openswd3::battle
