#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <span>

namespace openswd3::world_map {

struct LegacyWorldRoleSurfaceContext {
    compat::u32 map_width{};
    compat::u32 selected_guid{};
    std::span<compat::u8> surface_grid;
};

enum class LegacyWorldRoleSurfaceStatus : compat::u8 {
    ready,
    invalid_surface_grid,
    footprint_out_of_range,
};

struct LegacyWorldRoleSurfaceResult {
    LegacyWorldRoleSurfaceStatus status{
        LegacyWorldRoleSurfaceStatus::invalid_surface_grid};
    compat::u32 touched_cells{};
    compat::u32 mask{};
};

// sub_40AE20: clear this role's footprint bits from its current map cell.
[[nodiscard]] LegacyWorldRoleSurfaceResult
clear_legacy_world_role_surface_occupancy(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& context) noexcept;

// sub_40AEC0: derive the footprint bits from role flags/selection and OR them
// into this role's current map cell.
[[nodiscard]] LegacyWorldRoleSurfaceResult
mark_legacy_world_role_surface_occupancy(
    const LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& context) noexcept;

enum class LegacyWorldRoleCellFlagRefreshStatus : compat::u8 {
    ready,
    cell_out_of_range,
};

// Shared body used by 0x00412807..0x00412839 and initial role binding.
[[nodiscard]] LegacyWorldRoleCellFlagRefreshStatus
refresh_legacy_world_role_cell_flags(
    LegacyWorldRoleRecord& role,
    std::span<const compat::u8> surface_grid) noexcept;

}  // namespace openswd3::world_map
