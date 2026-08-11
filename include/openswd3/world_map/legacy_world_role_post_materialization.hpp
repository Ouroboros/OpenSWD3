#pragma once

#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldFlaggedRoleRecordCount = 4U;

struct LegacyWorldFlaggedRoleRecord {
    compat::u16 world_x{};
    compat::u16 world_y{};
    compat::u16 field_04{};
    compat::u16 field_06{};
    compat::u16 guid{};
    compat::u16 field_0a{};
    std::array<compat::u8, 4U> trailing_bytes{};
};

static_assert(sizeof(LegacyWorldFlaggedRoleRecord) == 0x10U);
static_assert(offsetof(LegacyWorldFlaggedRoleRecord, world_x) == 0x00U);
static_assert(offsetof(LegacyWorldFlaggedRoleRecord, world_y) == 0x02U);
static_assert(offsetof(LegacyWorldFlaggedRoleRecord, guid) == 0x08U);
static_assert(
    offsetof(LegacyWorldFlaggedRoleRecord, trailing_bytes) == 0x0CU
);

struct LegacyWorldRolePostMaterializationContext {
    compat::u32 previous_logical_map_id{};
    compat::u32 guid_one_action_override{};
    bool has_story_state_0x0192{};
    std::span<LegacyWorldObjectSlot> active_object_slots;
    LegacyRoleSpatialIndex* spatial_index{};
    std::span<compat::u8> surface_grid;
    compat::u32 map_width{};
};

struct LegacyWorldRolePostMaterializationState
    : LegacyWorldRoleTransferState {
    std::array<
        LegacyWorldFlaggedRoleRecord,
        kLegacyWorldFlaggedRoleRecordCount
    > flagged_role_records;
    compat::u32 flagged_role_record_count{};
    compat::u32 flagged_role_overflow_count{};
    compat::u32 guid_one_roles_overridden{};
    compat::u32 gated_roles_scanned{};
    compat::u32 roles_suppressed{};
    LegacyWorldRoleTransferStatus last_transfer_status{
        LegacyWorldRoleTransferStatus::ready
    };
};

enum class LegacyWorldRolePostMaterializationStatus {
    ready,
    role_index_out_of_range,
    gate_offset_field_out_of_range,
    gate_directory_offset_out_of_range,
    gate_record_truncated,
    gate_directory_unterminated,
    active_object_slots_required,
    role_source_patch_failed,
    party_capacity_exceeded,
    role_transfer_failed,
};

[[nodiscard]] LegacyWorldRolePostMaterializationStatus
post_materialize_legacy_world_role(
    std::span<compat::u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    const LegacyMapsMapDescriptor& map_descriptor,
    const LegacyWorldLoadRequest& request,
    std::span<LegacyWorldRoleRecord> roles,
    compat::u32 role_index,
    const LegacyWorldRolePostMaterializationContext* context,
    LegacyWorldRolePostMaterializationState& state
) noexcept;

}  // namespace openswd3::world_map
