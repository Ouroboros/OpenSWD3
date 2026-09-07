#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

struct LegacyBattleActorFrameSnapshotView {
    compat::u32* special_ready{};         // actor + 0x2AB8
    compat::u32* source_runtime_value{};  // actor + 0x2AA0
    compat::u16* profile_value{};         // actor + 0x2A0C
    compat::u32* frame_anchor_x{};        // actor + 0x02A8
    compat::u32* mirror_mode{};           // actor + 0x2B08
    compat::u32* frame_token{};           // actor + 0x254C
    compat::u16* position_x{};            // actor + 0x0D66
    compat::u16* position_y{};            // actor + 0x0D68

    const bool* special_ready_read_accessible{};
    const bool* source_runtime_value_read_accessible{};
    const bool* profile_value_read_accessible{};
    const bool* frame_anchor_x_read_accessible{};
    const bool* mirror_mode_read_accessible{};
    const bool* frame_token_read_accessible{};
    const bool* frame_token_write_accessible{};
    const bool* position_x_read_accessible{};
    const bool* position_y_read_accessible{};
};

struct LegacyBattleActorFrameSnapshotOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorFrameSnapshotView
resolve_legacy_battle_actor_frame_snapshot(
    const LegacyBattleActorFrameSnapshotOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorFrameSnapshotStatus : compat::u8 {
    completed,
    special_ready_read_typed_stop,
    source_runtime_value_read_typed_stop,
    profile_value_read_typed_stop,
    frame_anchor_x_read_typed_stop,
    overlapping_frame_dword_read_typed_stop,
    overlapping_resource_dword_read_typed_stop,
    mirror_mode_read_typed_stop,
    frame_token_write_typed_stop,
    mirror_frame_width_read_typed_stop,
    position_x_read_typed_stop,
    output_pointer_read_typed_stop,
    output_x_write_typed_stop,
    position_y_read_typed_stop,
    output_y_write_typed_stop,
    first_frame_token_read_typed_stop,
    frame_width_read_typed_stop,
    output_width_write_typed_stop,
    second_frame_token_read_typed_stop,
    frame_height_read_typed_stop,
    output_height_write_typed_stop,
};

struct LegacyBattleActorFrameSnapshotRequest {
    compat::u32 actor_token{};
    compat::u32 output_token{};
    compat::u32 entry_edx{};
    compat::u32 action_updater_return_ecx{};
    compat::u32 action_updater_return_edx{};
    LegacyBattleActorCoordinateFlags action_updater_flags{};
    compat::u32 frame_provider_return_eax{1U};
    compat::u32 frame_provider_return_ecx{};
    compat::u32 frame_provider_return_edx{};
    LegacyBattleActorCoordinateFlags frame_provider_flags{};
    std::array<compat::u32, 4> initial_output{};
    bool action_updater_flags_known{};
    bool frame_provider_flags_known{};
    bool overlapping_frame_dword_readable{true};
    bool overlapping_resource_dword_readable{true};
    bool output_pointer_readable{true};
    std::array<bool, 4> output_writable{true, true, true, true};
    bool first_frame_token_readable{true};
    bool second_frame_token_readable{true};
    bool frame_width_readable{true};
    bool frame_height_readable{true};
};

struct LegacyBattleActorFrameSnapshotResult {
    LegacyBattleActorFrameSnapshotStatus status{
        LegacyBattleActorFrameSnapshotStatus::completed
    };
    asset_runtime::LegacyActionRecord action_record{};
    asset_runtime::LegacyActionUpdateResult action_update{};
    rendering::LegacyFramePiece frame{};
    std::array<compat::u32, 4> output{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 frame_token{};
    compat::u32 overlapping_resource_dword{};
    compat::u32 overlapping_frame_dword{};
    compat::u32 actor_reads{};
    compat::u32 actor_writes{};
    compat::u32 local_reads{};
    compat::u32 frame_reads{};
    compat::u32 output_pointer_reads{};
    compat::u32 output_writes{};
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    bool frame_available{};
    bool returned_early{};
    bool mirrored{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x004784A0. The output block is addressed by its
// 32-bit legacy token so writes can alias actor + 0x254C without pre-reading or
// merging the four physical dword stores.
[[nodiscard]] LegacyBattleActorFrameSnapshotResult
query_legacy_battle_actor_frame_snapshot(
    const LegacyBattleActorFrameSnapshotView& actor,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleActorFrameSnapshotRequest& request = {}
);

}  // namespace openswd3::battle
