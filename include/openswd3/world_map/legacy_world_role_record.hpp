#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"

#include <cstddef>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldRoleRecordSize = 0xD8U;
inline constexpr std::size_t kLegacyWorldRoleCapacity = 256U;

struct LegacyWorldRoleRecord {
    compat::u32 spatial_next_link_32;
    compat::u32 world_x;
    compat::u32 world_y;
    compat::u32 map_cell_pointer_32;
    compat::u32 flags;
    compat::u32 talk_data_offset;
    compat::u32 path_word_index;
    compat::u16 path_data_id;
    compat::u16 talk_script_id;
    compat::u16 talk_initial_offset;
    compat::u16 path_wait_remaining;
    compat::u16 guid;
    compat::u16 interaction_gate;
    compat::u16 field_28;
    compat::u16 field_2a;
    compat::u32 field_2c;
    compat::u32 field_30;
    compat::u32 path_payload_relation;
    compat::u32 path_payload_pointer_32;
    compat::u32 field_3c;
    asset_runtime::LegacyActionRecord action;
};

static_assert(sizeof(LegacyWorldRoleRecord) == kLegacyWorldRoleRecordSize);
static_assert(offsetof(LegacyWorldRoleRecord, spatial_next_link_32) == 0x00U);
static_assert(offsetof(LegacyWorldRoleRecord, world_x) == 0x04U);
static_assert(offsetof(LegacyWorldRoleRecord, world_y) == 0x08U);
static_assert(offsetof(LegacyWorldRoleRecord, map_cell_pointer_32) == 0x0CU);
static_assert(offsetof(LegacyWorldRoleRecord, flags) == 0x10U);
static_assert(offsetof(LegacyWorldRoleRecord, talk_data_offset) == 0x14U);
static_assert(offsetof(LegacyWorldRoleRecord, path_word_index) == 0x18U);
static_assert(offsetof(LegacyWorldRoleRecord, path_data_id) == 0x1CU);
static_assert(offsetof(LegacyWorldRoleRecord, talk_script_id) == 0x1EU);
static_assert(offsetof(LegacyWorldRoleRecord, talk_initial_offset) == 0x20U);
static_assert(offsetof(LegacyWorldRoleRecord, path_wait_remaining) == 0x22U);
static_assert(offsetof(LegacyWorldRoleRecord, guid) == 0x24U);
static_assert(offsetof(LegacyWorldRoleRecord, interaction_gate) == 0x26U);
static_assert(offsetof(LegacyWorldRoleRecord, field_28) == 0x28U);
static_assert(offsetof(LegacyWorldRoleRecord, field_2a) == 0x2AU);
static_assert(offsetof(LegacyWorldRoleRecord, field_2c) == 0x2CU);
static_assert(offsetof(LegacyWorldRoleRecord, field_30) == 0x30U);
static_assert(offsetof(LegacyWorldRoleRecord, path_payload_relation) == 0x34U);
static_assert(
    offsetof(LegacyWorldRoleRecord, path_payload_pointer_32) == 0x38U
);
static_assert(offsetof(LegacyWorldRoleRecord, action) == 0x40U);

}  // namespace openswd3::world_map
