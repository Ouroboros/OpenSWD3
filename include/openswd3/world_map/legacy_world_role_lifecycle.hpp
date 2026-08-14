#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::world_map {

enum class LegacyWorldRoleTableResetStatus {
    ready,
    role_span_exceeds_capacity,
    highest_role_index_out_of_range,
    active_role_range_out_of_bounds,
};

struct LegacyWorldRoleTableResetResult {
    LegacyWorldRoleTableResetStatus status{
        LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity
    };
    compat::u32 payload_slots_scanned{};
    compat::u32 payload_owners_released{};
    compat::u32 roles_zeroed{};
    compat::u32 action_records_initialized{};
};

// sub_40F3B0 (0x0040F3B0..0x0040F40D): release +0x38 for inclusive
// indices 0..highest, zero the complete physical role table, then apply
// sub_40DC00 to every +0x40 action record. A negative highest index skips
// only the release loop.
[[nodiscard]] LegacyWorldRoleTableResetResult reset_legacy_world_role_table(
    std::span<LegacyWorldRoleRecord> roles,
    std::array<std::vector<compat::u8>, kLegacyWorldRoleCapacity>&
        role_label_payloads,
    compat::i32 highest_role_index
) noexcept;

}  // namespace openswd3::world_map
