#include "openswd3/world_map/legacy_world_player_post_frame.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u32;

constexpr u32 kRoleReadsMapCellFlag = 0x00000100U;
constexpr u32 kRoleActionValidationFlag = 0x00001000U;
constexpr u32 kRoleActionValidationSuppressedFlag = 0x40000000U;

[[nodiscard]] u32 transition_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

void clear_transitions(LegacyWorldMovementRuntimeState& movement) noexcept {
    movement.camera_x_transition = 0;
    movement.player_x_transition = 0;
    movement.camera_y_transition = 0;
    movement.player_y_transition = 0;
}

void shift_history(
    LegacyWorldPlayerPostFrameState& state, const LegacyWorldRoleRecord& player
) noexcept {
    for (std::size_t index = kLegacyWorldPlayerHistoryCount - 1U; index != 0U;
         --index) {
        state.world_x_history[index] = state.world_x_history[index - 1U];
        state.world_y_history[index] = state.world_y_history[index - 1U];
        state.action_variant_history[index] =
            state.action_variant_history[index - 1U];
    }
    state.world_x_history[0] = player.world_x;
    state.world_y_history[0] = player.world_y;
    state.action_variant_history[0] = player.action.variant_delta;
}

}  // namespace

void initialize_legacy_world_player_position_history(
    LegacyWorldPlayerPostFrameState& state, const LegacyWorldRoleRecord& player
) noexcept {
    state.world_x_history.fill(player.world_x);
    state.world_y_history.fill(player.world_y);
}

LegacyWorldPlayerPostFrameResult advance_legacy_world_player_post_frame(
    LegacyWorldRoleRecord& player,
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex& spatial_index,
    LegacyWorldMovementRuntimeState& movement,
    LegacyWorldPlayerPostFrameState& state,
    const LegacyWorldRoleSurfaceContext& surface_context,
    asset_runtime::LegacyActionDrawPorts& action_ports
) {
    LegacyWorldPlayerPostFrameResult result;
    result.aligned = ((player.world_x | player.world_y) & 0x0FU) == 0U;
    if (result.aligned) {
        const u32 first_row_bits = (player.world_y >> 4U) - 1U;
        const auto spatial_result = relocate_legacy_role_spatially_by_guid(
            spatial_index,
            roles,
            player.guid,
            player.flags & 3U,
            std::bit_cast<i32>(first_row_bits),
            true
        );
        result.spatial_status = spatial_result.status;
        if (result.spatial_status != LegacyRoleSpatialRelocationStatus::ready) {
            result.status =
                LegacyWorldPlayerPostFrameStatus::spatial_relocation_failed;
            return result;
        }
        result.spatially_relocated = true;

        const auto cleared =
            clear_legacy_world_role_surface_occupancy(player, surface_context);
        result.surface_status = cleared.status;
        result.cleared_cells = cleared.touched_cells;
        if (cleared.status != LegacyWorldRoleSurfaceStatus::ready) {
            result.status =
                LegacyWorldPlayerPostFrameStatus::surface_clear_failed;
            return result;
        }
        result.old_occupancy_cleared = true;

        const u32 vertical = transition_bits(movement.camera_y_transition) +
            transition_bits(movement.player_y_transition);
        const u32 horizontal = transition_bits(movement.player_x_transition) +
            transition_bits(movement.camera_x_transition);
        result.map_cell_delta =
            vertical * surface_context.map_width + horizontal;
        player.map_cell_pointer_32 += result.map_cell_delta;

        const auto marked =
            mark_legacy_world_role_surface_occupancy(player, surface_context);
        result.surface_status = marked.status;
        result.marked_cells = marked.touched_cells;
        if (marked.status != LegacyWorldRoleSurfaceStatus::ready) {
            result.status =
                LegacyWorldPlayerPostFrameStatus::surface_mark_failed;
            return result;
        }
        result.new_occupancy_marked = true;

        clear_transitions(movement);
        result.transitions_cleared = true;

        if (state.world_x_history[0] != player.world_x ||
            state.world_y_history[0] != player.world_y) {
            shift_history(state, player);
            result.history_shifted = true;
        }

        if ((player.flags & kRoleReadsMapCellFlag) != 0U) {
            if (refresh_legacy_world_role_cell_flags(
                    player, surface_context.surface_grid
                ) != LegacyWorldRoleCellFlagRefreshStatus::ready) {
                result.status =
                    LegacyWorldPlayerPostFrameStatus::cell_flag_refresh_failed;
                return result;
            }
            result.cell_flags_refreshed = true;
        }
    }

    if ((player.flags & kRoleActionValidationSuppressedFlag) == 0U &&
        (player.flags & kRoleActionValidationFlag) != 0U) {
        result.action_validation_requested = true;
        ++result.action_update_count;
        result.action_update_failed =
            action_ports.update_action_record(player.action) !=
            asset_runtime::LegacyActionUpdateStatus::completed;
    }
    return result;
}

}  // namespace openswd3::world_map
