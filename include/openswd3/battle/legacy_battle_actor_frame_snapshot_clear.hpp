#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;

inline constexpr std::size_t kLegacyBattleActorFrameSnapshotClearDwords =
    asset_runtime::kLegacyActionRecordSize / sizeof(compat::u32);

struct LegacyBattleActorFrameSnapshotClearView {
    asset_runtime::LegacyActionRecord* frame_source_action_record{};
    // Optional physical byte window for DF=1. Byte zero represents
    // actor+0x020C and the final dword represents actor+0x02A0.
    std::byte* reverse_destination_bytes{};
};

struct LegacyBattleActorFrameSnapshotClearOwners {
    LegacyBattleActionDispatchState* action{};
};

[[nodiscard]] LegacyBattleActorFrameSnapshotClearView
resolve_legacy_battle_actor_frame_snapshot_clear(
    const LegacyBattleActorFrameSnapshotClearOwners& owners,
    compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorFrameSnapshotClearStatus : compat::u8 {
    completed,
    push_edi_write_typed_stop,
    destination_dword_write_typed_stop,
    pop_edi_read_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorFrameSnapshotClearStackAccess {
    bool push_edi_writable{true};
    bool pop_edi_readable{true};
    bool return_address_readable{true};
};

struct LegacyBattleActorFrameSnapshotClearRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_edi{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    bool direction_flag{};
    std::array<bool, kLegacyBattleActorFrameSnapshotClearDwords>
        destination_dword_writable = [] {
            std::array<bool, kLegacyBattleActorFrameSnapshotClearDwords>
                values{};
            values.fill(true);
            return values;
        }();
    LegacyBattleActorFrameSnapshotClearStackAccess stack_access{};
};

struct LegacyBattleActorFrameSnapshotClearResult {
    LegacyBattleActorFrameSnapshotClearStatus status{
        LegacyBattleActorFrameSnapshotClearStatus::completed
    };
    std::array<compat::u32, 1> stack_writes{};
    std::array<compat::u32, 2> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_edi{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 region_start_token{};
    compat::u32 current_destination_token{};
    compat::u32 destination_writes{};
    compat::u32 cleared_dwords{};
    compat::u32 stack_write_count{};
    compat::u32 stack_read_count{};
    std::size_t fault_dword_index{kLegacyBattleActorFrameSnapshotClearDwords};
    bool returned{};
    bool flags_known{true};
    bool direction_flag{};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004786F0. It saves EDI, zeroes 0x26 dwords
// beginning at actor+0x02A0 with the current DF direction, restores EDI, and
// performs a plain RET. Each STOSD commits independently.
[[nodiscard]] LegacyBattleActorFrameSnapshotClearResult
clear_legacy_battle_actor_frame_snapshot(
    LegacyBattleActorFrameSnapshotClearView actor,
    const LegacyBattleActorFrameSnapshotClearRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
