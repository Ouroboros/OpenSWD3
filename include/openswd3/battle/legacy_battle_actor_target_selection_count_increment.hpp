#pragma once

#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionCountIncrementAddress = 0x00478AA0U;
inline constexpr compat::u32
    kLegacyBattleActorTargetSelectionCountIncrementEndAddress = 0x00478AA7U;

inline constexpr std::array<compat::u32, 3>
    kLegacyBattleActorTargetSelectionCountIncrementCallAddresses{
        0x00456A1FU,
        0x00456CDDU,
        0x00456D71U,
    };

inline constexpr std::array<compat::u32, 3>
    kLegacyBattleActorTargetSelectionCountIncrementReturnAddresses{
        0x00456A24U,
        0x00456CE2U,
        0x00456D76U,
    };

struct LegacyBattleActorTargetSelectionCountIncrementView {
    compat::u16* target_selection_count{};
};

struct LegacyBattleActorTargetSelectionCountIncrementOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

enum class LegacyBattleActorTargetSelectionCountIncrementStatus : compat::u8 {
    completed,
    count_read_typed_stop,
    count_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionCountIncrementAccess {
    bool count_readable{true};
    bool count_writable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorTargetSelectionCountIncrementRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorTargetSelectionCountIncrementAccess access{};
};

struct LegacyBattleActorTargetSelectionCountIncrementResult {
    LegacyBattleActorTargetSelectionCountIncrementStatus status{
        LegacyBattleActorTargetSelectionCountIncrementStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 field_token{};
    compat::u16 previous_value{};
    compat::u16 incremented_value{};
    compat::u32 field_reads{};
    compat::u32 field_writes{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{
        kLegacyBattleActorTargetSelectionCountIncrementAddress
    };
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorTargetSelectionCountIncrementCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorTargetSelectionCountIncrementRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorTargetSelectionCountIncrementTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorTargetSelectionCountIncrementResult last{};
};

[[nodiscard]] LegacyBattleActorTargetSelectionCountIncrementView
resolve_legacy_battle_actor_target_selection_count_increment(
    const LegacyBattleActorTargetSelectionCountIncrementOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorTargetSelectionCountIncrementResult
increment_legacy_battle_actor_target_selection_count(
    LegacyBattleActorTargetSelectionCountIncrementView actor,
    const LegacyBattleActorTargetSelectionCountIncrementRequest& request
) noexcept;

[[nodiscard]] bool
execute_legacy_battle_actor_target_selection_count_increment_call(
    LegacyBattleActorTargetSelectionCountIncrementTrace& trace,
    const LegacyBattleActorTargetSelectionCountIncrementCallRequests& requests,
    const LegacyBattleActorTargetSelectionCountIncrementOwners& owners,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u32 entry_eax,
    compat::u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    bool entry_flags_known = true
) noexcept;

}  // namespace openswd3::battle
