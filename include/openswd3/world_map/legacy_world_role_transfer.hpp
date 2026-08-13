#pragma once

#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldObjectSlotSize = 0x21CU;
inline constexpr std::size_t kLegacyWorldActiveObjectSlotCount = 72U;
inline constexpr std::size_t kLegacyWorldPartySlotCount = 8U;

struct LegacyWorldObjectSlot {
    std::array<compat::u8, kLegacyWorldObjectSlotSize> bytes = [] {
        std::array<compat::u8, kLegacyWorldObjectSlotSize> value{};
        value.fill(0xFFU);
        return value;
    }();
};

static_assert(sizeof(LegacyWorldObjectSlot) == kLegacyWorldObjectSlotSize);

struct LegacyWorldRoleTransferContext {
    std::span<LegacyWorldObjectSlot> active_object_slots;
    LegacyRoleSpatialIndex* spatial_index{};
    std::span<compat::u8> surface_grid;
    compat::u32 map_width{};
    compat::u16 selected_guid{};
};

struct LegacyWorldRoleTransferState {
    std::array<compat::u32, kLegacyWorldPartySlotCount> party_role_indices =
        [] {
            std::array<compat::u32, kLegacyWorldPartySlotCount> value{};
            value.fill(0xFFFFFFFFU);
            return value;
        }();
    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount>
        party_object_slots;
    compat::u32 party_role_count{1U};
    compat::u32 roles_transferred{};
    compat::u32 active_object_slots_reset{};
    compat::u32 aligned_roles{};
    compat::u32 cleared_surface_cells{};
    compat::u32 spatial_roles_relocated{};
};

enum class LegacyWorldRoleTransferStatus {
    ready,
    role_index_out_of_range,
    party_capacity_exceeded,
    active_object_slots_required,
    path_cursor_out_of_range,
    direction_out_of_range,
    direction_cannot_align,
    surface_grid_required,
    surface_grid_invalid,
    surface_footprint_out_of_range,
    spatial_index_required,
    spatial_relocation_failed,
    role_source_patch_failed,
};

[[nodiscard]] LegacyWorldRoleTransferStatus transfer_legacy_world_role(
    std::span<compat::u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    std::span<LegacyWorldRoleRecord> roles,
    compat::u32 role_index,
    const LegacyWorldRoleTransferContext* context,
    LegacyWorldRoleTransferState& state
) noexcept;

}  // namespace openswd3::world_map
