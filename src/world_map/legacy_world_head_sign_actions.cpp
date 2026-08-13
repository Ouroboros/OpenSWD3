#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"

namespace openswd3::world_map {

LegacyWorldHeadSignActionsState::LegacyWorldHeadSignActionsState() noexcept {
    for (asset_runtime::LegacyActionRecord& record : records) {
        asset_runtime::initialize_legacy_action_record(record);
    }

    for (std::size_t index = 0U; index < 4U; ++index) {
        records[index].action_id = kLegacyWorldHeadSignActionId;
        records[index].base_variant = static_cast<compat::u32>(index);
    }
}

const asset_runtime::LegacyActionRecord* resolve_legacy_world_head_sign_action(
    const LegacyWorldHeadSignActionsState& state,
    const compat::u32 token) noexcept {
    if (token < kLegacyWorldHeadSignActionBaseAddress) {
        return nullptr;
    }
    const compat::u32 byte_offset =
        token - kLegacyWorldHeadSignActionBaseAddress;
    if (byte_offset % asset_runtime::kLegacyActionRecordSize != 0U) {
        return nullptr;
    }
    const std::size_t slot =
        byte_offset / asset_runtime::kLegacyActionRecordSize;
    if (slot >= state.records.size()) {
        return nullptr;
    }
    return &state.records[slot];
}

LegacyWorldHeadSignActionsResult advance_legacy_world_head_sign_actions(
    LegacyWorldHeadSignActionsState& state,
    asset_runtime::LegacyActionDrawPorts& action_ports) {
    LegacyWorldHeadSignActionsResult result;
    for (std::size_t remaining = state.records.size(); remaining != 0U;
         --remaining) {
        ++result.visited_count;
        asset_runtime::LegacyActionRecord& record =
            state.records[remaining - 1U];
        if (record.action_id == 0U) {
            continue;
        }

        ++result.active_count;
        ++result.update_count;
        if (action_ports.update_action_record(record) !=
            asset_runtime::LegacyActionUpdateStatus::completed) {
            ++result.update_failure_count;
        }
    }

    if (result.update_failure_count != 0U) {
        result.status =
            LegacyWorldHeadSignActionsStatus::completed_with_update_failures;
    }
    return result;
}

}  // namespace openswd3::world_map
