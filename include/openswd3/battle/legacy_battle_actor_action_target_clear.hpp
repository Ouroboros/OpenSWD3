#pragma once

#include "openswd3/battle/legacy_battle_actor_action_target.hpp"
#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorActionTargetClearAddress =
    0x00478B20U;
inline constexpr compat::u32
    kLegacyBattleActorActionTargetClearReturnInstruction = 0x00478B29U;

inline constexpr std::array<compat::u32, 5>
    kLegacyBattleActorActionTargetClearCallAddresses{
        0x00456F94U,
        0x004570F5U,
        0x00457EBCU,
        0x0045AEC2U,
        0x0045AF58U,
    };

inline constexpr std::array<compat::u32, 5>
    kLegacyBattleActorActionTargetClearReturnAddresses{
        0x00456F99U,
        0x004570FAU,
        0x00457EC1U,
        0x0045AEC7U,
        0x0045AF5DU,
    };

enum class LegacyBattleActorActionTargetClearStatus : compat::u8 {
    completed,
    action_target_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionTargetClearAccess {
    bool action_target_writable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorActionTargetClearRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorActionTargetClearAccess access{};
};

struct LegacyBattleActorActionTargetClearResult {
    LegacyBattleActorActionTargetClearStatus status{
        LegacyBattleActorActionTargetClearStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 action_target_field_token{};
    compat::u32 action_target_writes{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorActionTargetClearAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorActionTargetClearCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorActionTargetClearRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorActionTargetClearTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorActionTargetClearResult last{};
};

[[nodiscard]] LegacyBattleActorActionTargetClearResult
clear_legacy_battle_actor_action_target(
    LegacyBattleActorActionTargetView actor,
    const LegacyBattleActorActionTargetClearRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_action_target_clear_call(
    LegacyBattleActorActionTargetClearTrace& trace,
    const LegacyBattleActorActionTargetClearCallRequests& requests,
    const LegacyBattleActorActionTargetOwners& owners,
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
