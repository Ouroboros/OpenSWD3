#pragma once

#include "openswd3/battle/legacy_battle_actor_target_selection_latch_set.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionLatchQueryAddress = 0x00478B40U;
inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionLatchQueryReturnInstruction = 0x00478B46U;

inline constexpr std::array<compat::u32, 4>
    kLegacyBattleActorTargetSelectionLatchQueryCallAddresses{
        0x00456B60U,
        0x00456F12U,
        0x00457140U,
        0x00457EC3U,
    };

inline constexpr std::array<compat::u32, 4>
    kLegacyBattleActorTargetSelectionLatchQueryReturnAddresses{
        0x00456B65U,
        0x00456F17U,
        0x00457145U,
        0x00457EC8U,
    };

enum class LegacyBattleActorTargetSelectionLatchQueryStatus : compat::u8 {
    completed,
    target_selection_latch_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionLatchQueryAccess {
    bool target_selection_latch_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionLatchQueryRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorTargetSelectionLatchQueryAccess access{};
};

struct LegacyBattleActorTargetSelectionLatchQueryResult {
    LegacyBattleActorTargetSelectionLatchQueryStatus status{
        LegacyBattleActorTargetSelectionLatchQueryStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 target_selection_latch_field_token{};
    compat::u32 target_selection_latch_reads{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorTargetSelectionLatchQueryAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorTargetSelectionLatchQueryCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorTargetSelectionLatchQueryRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorTargetSelectionLatchQueryTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorTargetSelectionLatchQueryResult last{};
};

[[nodiscard]] LegacyBattleActorTargetSelectionLatchQueryResult
query_legacy_battle_actor_target_selection_latch(
    LegacyBattleActorTargetSelectionLatchView actor,
    const LegacyBattleActorTargetSelectionLatchQueryRequest& request
) noexcept;

[[nodiscard]] bool
execute_legacy_battle_actor_target_selection_latch_query_call(
    LegacyBattleActorTargetSelectionLatchQueryTrace& trace,
    const LegacyBattleActorTargetSelectionLatchQueryCallRequests& requests,
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
