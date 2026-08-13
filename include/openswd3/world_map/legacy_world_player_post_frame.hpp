#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldPlayerHistoryCount = 32U;

struct LegacyWorldPlayerPostFrameState {
    std::array<compat::u32, kLegacyWorldPlayerHistoryCount> world_x_history{};
    std::array<compat::u32, kLegacyWorldPlayerHistoryCount> world_y_history{};
    std::array<compat::u32, kLegacyWorldPlayerHistoryCount>
        action_variant_history{};
};

void initialize_legacy_world_player_position_history(
    LegacyWorldPlayerPostFrameState& state, const LegacyWorldRoleRecord& player
) noexcept;

enum class LegacyWorldPlayerPostFrameStatus : compat::u8 {
    completed,
    spatial_relocation_failed,
    surface_clear_failed,
    surface_mark_failed,
    cell_flag_refresh_failed,
};

struct LegacyWorldPlayerPostFrameResult {
    LegacyWorldPlayerPostFrameStatus status{
        LegacyWorldPlayerPostFrameStatus::completed
    };
    LegacyRoleSpatialRelocationStatus spatial_status{
        LegacyRoleSpatialRelocationStatus::ready
    };
    LegacyWorldRoleSurfaceStatus surface_status{
        LegacyWorldRoleSurfaceStatus::ready
    };
    bool aligned{};
    bool spatially_relocated{};
    bool old_occupancy_cleared{};
    bool new_occupancy_marked{};
    bool transitions_cleared{};
    bool history_shifted{};
    bool cell_flags_refreshed{};
    bool action_validation_requested{};
    bool action_update_failed{};
    compat::u32 map_cell_delta{};
    compat::u32 cleared_cells{};
    compat::u32 marked_cells{};
    compat::u32 action_update_count{};
};

// 0x00412719..0x0041287C: after presentation, update the aligned player's
// spatial/surface cell, clear movement transitions, shift three 32-entry
// histories, refresh map-derived flags, then perform the unconditional action
// validation gate. Action update failure is diagnostic-only in the original.
[[nodiscard]] LegacyWorldPlayerPostFrameResult
advance_legacy_world_player_post_frame(
    LegacyWorldRoleRecord& player,
    std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex& spatial_index,
    LegacyWorldMovementRuntimeState& movement,
    LegacyWorldPlayerPostFrameState& state,
    const LegacyWorldRoleSurfaceContext& surface_context,
    asset_runtime::LegacyActionDrawPorts& action_ports
);

}  // namespace openswd3::world_map
