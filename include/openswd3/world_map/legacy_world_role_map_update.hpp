#pragma once

#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_role_lookup.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <span>

namespace openswd3::world_map {

struct LegacyWorldRoleMapUpdateRequest {
    compat::u16 role_selector{};
    compat::u16 path_data_id{};
    compat::u16 talk_script_id{};
    compat::u16 action_id{};
    compat::u16 base_variant{};
    compat::u16 variant_delta{};
    compat::u16 flags{};
};

struct LegacyWorldRoleMapUpdateContext {
    compat::u32 controlled_role_index{};
    std::span<compat::u8> maps_payload;
    LegacyMapsWorldDatabase* maps_database{};
    std::span<LegacyWorldRoleRecord> roles;
    std::span<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount>
        party_object_slots;
    std::span<compat::u32, kLegacyWorldPartySlotCount> party_role_indices;
    compat::u32* party_role_count{};
    LegacyRoleSpatialIndex* spatial_index{};
    LegacyWorldRoleSurfaceContext role_surface;
};

enum class LegacyWorldRoleMapUpdateStatus : compat::u8 {
    ready,
    maps_runtime_required,
    controlled_role_out_of_range,
    party_count_out_of_range,
    active_role_not_in_physical_party,
    role_surface_failed,
    path_direction_out_of_range,
    path_direction_cannot_align,
    role_spatial_relocation_failed,
    maps_patch_failed,
};

struct LegacyWorldRoleMapUpdateResult {
    LegacyWorldRoleMapUpdateStatus status{
        LegacyWorldRoleMapUpdateStatus::ready
    };
    compat::u32 resolved_role_index{kLegacyWorldRoleNotFound};
    compat::u32 physical_party_index{kLegacyWorldRoleNotFound};
    bool runtime_role_found{};
    bool coordinates_aligned{};
    bool spatial_role_relocated{};
    bool role_surface_cleared{};
    bool party_role_removed{};
    bool maps_source_patched{};
};

// sub_40D790: apply opcode 66's role/map update. The eight physical party
// indices are scanned even past the logical count, and the vacated tail is
// deliberately not cleared, matching the original fixed-array behavior.
[[nodiscard]] LegacyWorldRoleMapUpdateResult apply_legacy_world_role_map_update(
    const LegacyWorldRoleMapUpdateRequest& request,
    const LegacyWorldRoleMapUpdateContext& context
) noexcept;

}  // namespace openswd3::world_map
