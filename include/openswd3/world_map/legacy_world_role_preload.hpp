#pragma once

#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldObjectSlotCount = 72U;

struct LegacyWorldObjectSlotPrefix {
    compat::u16 role_index{0xFFFFU};
    compat::u16 field_02{0xFFFFU};
    compat::u16 world_x{0xFFFFU};
    compat::u16 world_y{0xFFFFU};
};

static_assert(sizeof(LegacyWorldObjectSlotPrefix) == 0x08U);
static_assert(offsetof(LegacyWorldObjectSlotPrefix, role_index) == 0x00U);
static_assert(offsetof(LegacyWorldObjectSlotPrefix, world_x) == 0x04U);
static_assert(offsetof(LegacyWorldObjectSlotPrefix, world_y) == 0x06U);

struct LegacyWorldRolePreloadContext {
    std::span<const compat::u8> path_database;
    std::span<const LegacyWorldRoleRecord> roles;
    std::span<const LegacyWorldObjectSlotPrefix> object_slots;
    compat::u32 controlled_role_index{};
    compat::u32 current_map_width{};
    compat::u32 current_map_height{};
};

enum class LegacyWorldRolePreloadStatus {
    ready,
    path_directory_entry_out_of_range,
    path_command_out_of_range,
    object_slots_required,
    role_source_write_failed,
};

struct LegacyWorldRolePreloadResult {
    LegacyWorldRolePreloadStatus status{LegacyWorldRolePreloadStatus::ready};
    compat::u32 roles_visited{};
    compat::u32 roles_skipped{};
    compat::u32 flagged_roles_patched{};
    compat::u32 ordinary_roles_synchronized{};
    compat::u32 path_type_eight_roles{};
    compat::u32 object_coordinate_overrides{};
    compat::u32 out_of_bounds_coordinates{};
    compat::u32 missing_role_sources{};
};

[[nodiscard]] LegacyWorldRolePreloadResult
preload_legacy_world_roles_before_load(
    std::span<compat::u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    const LegacyWorldLoadRequest& target,
    const LegacyWorldRolePreloadContext& context
) noexcept;

}  // namespace openswd3::world_map
