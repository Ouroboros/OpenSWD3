#pragma once

#include "openswd3/battle/legacy_battle_actor_target_selection.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorGateDecayAddress = 0x00478AE0U;
inline constexpr compat::u32 kLegacyBattleActorGateDecayEndAddress =
    0x00478B10U;
inline constexpr compat::u32 kLegacyBattleActorGateDecayFixedPreGroupBToken =
    0x005229E0U;

inline constexpr std::array<compat::u32, 12>
    kLegacyBattleActorGateDecayCallAddresses{
        0x00454A5DU,
        0x00454B06U,
        0x00456FA4U,
        0x0045715AU,
        0x004571ACU,
        0x0045720CU,
        0x00457EEEU,
        0x00458002U,
        0x00458025U,
        0x0045AEDFU,
        0x0045AF75U,
        0x0045DC03U,
    };

inline constexpr std::array<compat::u32, 12>
    kLegacyBattleActorGateDecayReturnAddresses{
        0x00454A62U,
        0x00454B0BU,
        0x00456FA9U,
        0x0045715FU,
        0x004571B1U,
        0x00457211U,
        0x00457EF3U,
        0x00458007U,
        0x0045802AU,
        0x0045AEE4U,
        0x0045AF7AU,
        0x0045DC08U,
    };

struct LegacyBattleActorGateDecayView {
    compat::u16* start_gate{};
    compat::u16* target_selection_count{};
    compat::u32* packed_start_gate_and_target_selection_count{};
    compat::u32* start_gate_latch{};
};

struct LegacyBattleActorGateDecayOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
    compat::u32* fixed_pre_group_b_packed_counts{};
    compat::u32* fixed_pre_group_b_start_gate_latch{};
};

enum class LegacyBattleActorGateDecayStatus : compat::u8 {
    completed,
    start_gate_read_typed_stop,
    start_gate_write_typed_stop,
    target_selection_count_read_typed_stop,
    target_selection_count_write_typed_stop,
    start_gate_latch_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorGateDecayAccess {
    bool start_gate_readable{true};
    bool start_gate_writable{true};
    bool target_selection_count_readable{true};
    bool target_selection_count_writable{true};
    bool start_gate_latch_writable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorGateDecayRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorGateDecayAccess access{};
};

struct LegacyBattleActorGateDecayResult {
    LegacyBattleActorGateDecayStatus status{
        LegacyBattleActorGateDecayStatus::completed
    };
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u32 start_gate_field_token{};
    compat::u32 target_selection_count_field_token{};
    compat::u32 start_gate_latch_field_token{};
    compat::u16 previous_start_gate{};
    compat::u16 decayed_start_gate{};
    compat::u16 previous_target_selection_count{};
    compat::u16 decayed_target_selection_count{};
    compat::u32 start_gate_reads{};
    compat::u32 start_gate_writes{};
    compat::u32 target_selection_count_reads{};
    compat::u32 target_selection_count_writes{};
    compat::u32 start_gate_latch_writes{};
    std::array<compat::u32, 1> stack_reads{};
    compat::u32 stack_read_count{};
    compat::u32 return_address_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorGateDecayAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
};

struct LegacyBattleActorGateDecayCallRequests {
    LegacyBattleActorTargetSelectionLazyArray<LegacyBattleActorGateDecayRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorGateDecayTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    std::size_t calls{};
    LegacyBattleActorGateDecayResult last{};
};

[[nodiscard]] LegacyBattleActorGateDecayView
resolve_legacy_battle_actor_gate_decay(
    const LegacyBattleActorGateDecayOwners& owners, compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorGateDecayResult decay_legacy_battle_actor_gates(
    LegacyBattleActorGateDecayView actor,
    const LegacyBattleActorGateDecayRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_gate_decay_call(
    LegacyBattleActorGateDecayTrace& trace,
    const LegacyBattleActorGateDecayCallRequests& requests,
    const LegacyBattleActorGateDecayOwners& owners,
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
