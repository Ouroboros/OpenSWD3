#pragma once

#include "openswd3/battle/legacy_battle_actor_target_selection_count_increment.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionCountQueryAddress = 0x00478AB0U;
inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionCountQueryEndAddress = 0x00478AB7U;
inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionCountQueryCallAddress = 0x0046D429U;
inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionCountQueryReturnAddress = 0x0046D42EU;

using LegacyBattleActorTargetSelectionCountQueryOwners =
    LegacyBattleActorTargetSelectionCountIncrementOwners;

struct LegacyBattleActorTargetSelectionCountQueryView {
    const compat::u16* target_selection_count{};
};

enum class LegacyBattleActorTargetSelectionCountQueryStatus : compat::u8 {
    completed,
    count_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionCountQueryAccess {
    bool count_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionCountQueryRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorTargetSelectionCountQueryAccess access{};
};

struct LegacyBattleActorTargetSelectionCountQueryResult {
    LegacyBattleActorTargetSelectionCountQueryStatus status{
        LegacyBattleActorTargetSelectionCountQueryStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 field_token{};
    compat::u16 count_value{};
    compat::u32 field_reads{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorTargetSelectionCountQueryAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorTargetSelectionCountQueryCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorTargetSelectionCountQueryRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorTargetSelectionCountQueryTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorTargetSelectionCountQueryResult last{};
};

[[nodiscard]] LegacyBattleActorTargetSelectionCountQueryView
resolve_legacy_battle_actor_target_selection_count_query(
    const LegacyBattleActorTargetSelectionCountQueryOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorTargetSelectionCountQueryResult
query_legacy_battle_actor_target_selection_count(
    LegacyBattleActorTargetSelectionCountQueryView actor,
    const LegacyBattleActorTargetSelectionCountQueryRequest& request
) noexcept;

[[nodiscard]] bool
execute_legacy_battle_actor_target_selection_count_query_call(
    LegacyBattleActorTargetSelectionCountQueryTrace& trace,
    const LegacyBattleActorTargetSelectionCountQueryCallRequests& requests,
    const LegacyBattleActorTargetSelectionCountQueryOwners& owners,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true
) noexcept;

}  // namespace openswd3::battle
