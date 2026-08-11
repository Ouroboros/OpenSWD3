#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_lmf_archive.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacySpatialNoRole = 0U;
inline constexpr compat::u32 kLegacySpatialRowPadding = 20U;
inline constexpr compat::u32 kLegacySpatialGroupCount = 3U;

struct LegacyWorldMapEvent {
    compat::u32 field_04{};
    compat::u32 field_08{};
    compat::u32 field_0c{};
    compat::u32 field_10{};
    std::vector<compat::u8> name_bytes_with_terminator;
};

struct LegacyRoleSpatialIndex {
    compat::u32 map_height{};
    std::array<
        std::vector<compat::u32>,
        static_cast<std::size_t>(kLegacySpatialGroupCount)
    > row_heads;
};

enum class LegacyWorldMapBusinessStatus {
    ready,
    invalid_physical_state,
    spatial_index_size_overflow,
    allocation_failed,
    role_capacity_exceeded,
    unsupported_spatial_group,
    spatial_row_out_of_range,
};

struct LegacyWorldMapBusinessState {
    std::vector<LegacyWorldMapEvent> events;
    std::vector<LegacyWorldRoleRecord> roles;
    LegacyRoleSpatialIndex spatial_index;
    compat::u32 offset14_role_count{};
    compat::u32 offset1c_role_count{};
};

struct LegacyWorldMapBusinessResult {
    LegacyWorldMapBusinessStatus status{
        LegacyWorldMapBusinessStatus::invalid_physical_state
    };
    LegacyWorldMapBusinessState state;
};

[[nodiscard]] bool insert_legacy_role_spatially(
    LegacyRoleSpatialIndex& spatial_index,
    std::span<LegacyWorldRoleRecord> roles,
    compat::u32 role_index
) noexcept;

enum class LegacyRoleSpatialRelocationStatus {
    ready,
    invalid_group,
    first_row_out_of_range,
    broken_link,
    role_not_found,
    reinsertion_failed,
};

[[nodiscard]] LegacyRoleSpatialRelocationStatus
relocate_legacy_role_spatially_by_guid(
    LegacyRoleSpatialIndex& spatial_index,
    std::span<LegacyWorldRoleRecord> roles,
    compat::u16 guid,
    compat::u32 group,
    compat::i32 first_row,
    bool reinsert
) noexcept;

[[nodiscard]] LegacyWorldMapBusinessResult
build_legacy_world_map_business_state(
    const resource_io::LegacyLmfMapHeader& header,
    const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records,
    const resource_io::LegacyLmfOffset14Directory& offset14_directory,
    const resource_io::LegacyLmfOffset1cDirectory& offset1c_directory
);

enum class LegacyWorldRoleCellBindingStatus {
    ready,
    role_range_out_of_bounds,
    flagged_cell_out_of_bounds,
};

struct LegacyWorldRoleCellBindingResult {
    LegacyWorldRoleCellBindingStatus status{
        LegacyWorldRoleCellBindingStatus::role_range_out_of_bounds
    };
    compat::u32 roles_bound{};
    compat::u32 out_of_bounds_indices{};
};

[[nodiscard]] LegacyWorldRoleCellBindingResult
bind_legacy_world_role_cells(
    std::span<LegacyWorldRoleRecord> roles,
    compat::u32 first_role_index,
    compat::u32 role_count,
    compat::u32 map_width,
    std::span<const compat::u8> surface_grid
) noexcept;

}  // namespace openswd3::world_map
