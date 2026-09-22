#pragma once

#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorTargetSelectionLatchSetAddress =
    0x00478B30U;
inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionLatchSetReturnInstruction = 0x00478B3AU;

inline constexpr std::array<compat::u32, 3>
    kLegacyBattleActorTargetSelectionLatchSetCallAddresses{
        0x00454BAEU,
        0x00456B51U,
        0x004578FBU,
    };

inline constexpr std::array<compat::u32, 3>
    kLegacyBattleActorTargetSelectionLatchSetReturnAddresses{
        0x00454BB3U,
        0x00456B56U,
        0x00457900U,
    };

enum class LegacyBattleActorTargetSelectionLatchSetStatus : compat::u8 {
    completed,
    target_selection_latch_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionLatchSetAccess {
    bool target_selection_latch_writable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionLatchView {
    compat::u32* target_selection_latch{};
};

struct LegacyBattleActorTargetSelectionLatchOwners {
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorTargetSelectionLatchSetRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorTargetSelectionLatchSetAccess access{};
};

struct LegacyBattleActorTargetSelectionLatchSetResult {
    LegacyBattleActorTargetSelectionLatchSetStatus status{
        LegacyBattleActorTargetSelectionLatchSetStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 target_selection_latch_field_token{};
    compat::u32 target_selection_latch_writes{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorTargetSelectionLatchSetAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorTargetSelectionLatchSetCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorTargetSelectionLatchSetRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorTargetSelectionLatchSetTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorTargetSelectionLatchSetResult last{};
};

[[nodiscard]] LegacyBattleActorTargetSelectionLatchView
resolve_legacy_battle_actor_target_selection_latch(
    const LegacyBattleActorTargetSelectionLatchOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorTargetSelectionLatchSetResult
set_legacy_battle_actor_target_selection_latch(
    LegacyBattleActorTargetSelectionLatchView actor,
    const LegacyBattleActorTargetSelectionLatchSetRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_target_selection_latch_set_call(
    LegacyBattleActorTargetSelectionLatchSetTrace& trace,
    const LegacyBattleActorTargetSelectionLatchSetCallRequests& requests,
    const LegacyBattleActorTargetSelectionLatchOwners& owners,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true,
    std::size_t request_offset = 0U
) noexcept;

}  // namespace openswd3::battle
