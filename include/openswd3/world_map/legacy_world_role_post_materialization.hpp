#pragma once

#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldObjectSlotSize = 0x21CU;
inline constexpr std::size_t kLegacyWorldActiveObjectSlotCount = 72U;
inline constexpr std::size_t kLegacyWorldPartySlotCount = 8U;
inline constexpr std::size_t kLegacyWorldFlaggedRoleRecordCount = 4U;

struct LegacyWorldObjectSlot {
    std::array<compat::u8, kLegacyWorldObjectSlotSize> bytes = [] {
        std::array<compat::u8, kLegacyWorldObjectSlotSize> value{};
        value.fill(0xFFU);
        return value;
    }();
};

static_assert(sizeof(LegacyWorldObjectSlot) == kLegacyWorldObjectSlotSize);

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
};

struct LegacyWorldRolePostMaterializationState {
    std::array<compat::u32, kLegacyWorldPartySlotCount>
        party_role_indices = [] {
            std::array<compat::u32, kLegacyWorldPartySlotCount> value{};
            value.fill(0xFFFFFFFFU);
            return value;
        }();
    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount>
        party_object_slots;
    std::array<
        LegacyWorldFlaggedRoleRecord,
        kLegacyWorldFlaggedRoleRecordCount
    > flagged_role_records;
    compat::u32 party_role_count{1U};
    compat::u32 flagged_role_record_count{};
    compat::u32 flagged_role_overflow_count{};
    compat::u32 guid_one_roles_overridden{};
    compat::u32 gated_roles_scanned{};
    compat::u32 roles_transferred{};
    compat::u32 roles_suppressed{};
    compat::u32 active_object_slots_reset{};
};

enum class LegacyWorldRolePostMaterializationStatus {
    ready,
    role_index_out_of_range,
    gate_offset_field_out_of_range,
    gate_directory_offset_out_of_range,
    gate_record_truncated,
    gate_directory_unterminated,
    active_object_slots_required,
    materialized_role_not_tile_aligned,
    role_source_patch_failed,
    party_capacity_exceeded,
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
