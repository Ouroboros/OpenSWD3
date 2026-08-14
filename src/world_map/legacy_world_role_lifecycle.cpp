#include "openswd3/world_map/legacy_world_role_lifecycle.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <cstddef>

namespace openswd3::world_map {

LegacyWorldRoleTableResetResult reset_legacy_world_role_table(
    const std::span<LegacyWorldRoleRecord> roles,
    std::array<std::vector<compat::u8>, kLegacyWorldRoleCapacity>&
        role_label_payloads,
    const compat::i32 highest_role_index
) noexcept {
    LegacyWorldRoleTableResetResult result;
    if (roles.size() > kLegacyWorldRoleCapacity) {
        result.status =
            LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity;
        return result;
    }
    if (highest_role_index >=
        static_cast<compat::i32>(kLegacyWorldRoleCapacity)) {
        result.status =
            LegacyWorldRoleTableResetStatus::highest_role_index_out_of_range;
        return result;
    }
    if (highest_role_index >= 0 &&
        static_cast<std::size_t>(highest_role_index) >= roles.size()) {
        result.status =
            LegacyWorldRoleTableResetStatus::active_role_range_out_of_bounds;
        return result;
    }

    for (compat::i32 index = 0; index <= highest_role_index; ++index) {
        ++result.payload_slots_scanned;
        LegacyWorldRoleRecord& role = roles[static_cast<std::size_t>(index)];
        if (role.path_payload_pointer_32 != 0U) {
            std::vector<compat::u8>{}.swap(
                role_label_payloads[static_cast<std::size_t>(index)]
            );
            ++result.payload_owners_released;
        }
    }

    for (LegacyWorldRoleRecord& role : roles) {
        role = {};
        ++result.roles_zeroed;
    }
    for (LegacyWorldRoleRecord& role : roles) {
        asset_runtime::initialize_legacy_action_record(role.action);
        ++result.action_records_initialized;
    }

    result.status = LegacyWorldRoleTableResetStatus::ready;
    return result;
}

}  // namespace openswd3::world_map
