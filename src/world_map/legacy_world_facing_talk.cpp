#include "openswd3/world_map/legacy_world_facing_talk.hpp"

#include "openswd3/world_map/legacy_world_facing.hpp"

namespace openswd3::world_map {
namespace {

using asset_runtime::LegacyActionRecord;
using compat::u32;

[[nodiscard]] constexpr u32
wrapping_scaled_add(const u32 value, const u32 scale) noexcept {
    return value + scale * 8U;
}

[[nodiscard]] constexpr u32
opposite_legacy_direction(const u32 direction) noexcept {
    return (direction & 0xFFFFFFFEU) + ((direction - 1U) & 1U);
}

void update_action(
    LegacyWorldFacingTalkResult& result,
    LegacyActionRecord& action,
    LegacyWorldFacingTalkPorts& ports
) noexcept {
    ++result.action_update_count;
    if (ports.update_action(action) == 0U) {
        ++result.action_update_failure_count;
    }
}

}  // namespace

LegacyWorldFacingTalkResult coordinate_legacy_world_facing_talk(
    const LegacyWorldFacingTalkRequest& request,
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyWorldTalkContext& talk_context,
    u32& one_shot_interaction_state,
    LegacyWorldFacingTalkPorts& ports
) noexcept {
    LegacyWorldFacingTalkResult result;
    if (request.player_index >= roles.size()) {
        result.status = LegacyWorldFacingTalkStatus::invalid_player_index;
        return result;
    }
    if (talk_context.source_guid != kLegacyWorldTalkIdleSource) {
        return result;
    }

    const LegacyWorldFacingRoleQueryResult query =
        find_legacy_world_facing_role(
            roles,
            static_cast<u32>(roles.size()),
            request.player_index,
            request.delta_x,
            request.delta_y,
            request.map_width,
            request.map_height
        );
    result.role_index = query.role_index;
    result.tile_query_count = query.tile_query_count;
    if (query.status != LegacyWorldFacingRoleQueryStatus::completed) {
        result.status = LegacyWorldFacingTalkStatus::invalid_player_index;
        return result;
    }
    if (result.role_index == kLegacyWorldFacingRoleNotFound) {
        return result;
    }

    LegacyWorldRoleRecord& target = roles[result.role_index];
    if (target.interaction_gate != 0U) {
        return result;
    }
    LegacyWorldRoleRecord& player = roles[request.player_index];
    result.facing =
        measure_legacy_world_facing(
            wrapping_scaled_add(player.world_x, player.action.field_2c),
            wrapping_scaled_add(player.world_y, player.action.field_30),
            wrapping_scaled_add(target.world_x, target.action.field_2c),
            wrapping_scaled_add(target.world_y, target.action.field_30)
        )
            .direction;

    if ((target.flags & kLegacyWorldTalkTurningRoleFlag) != 0U) {
        target.action.one_shot_base_variant = target.action.base_variant;
        target.action.one_shot_variant_delta = target.action.variant_delta;
        target.action.base_variant = 0U;
        target.action.variant_delta = result.facing;
        target.action.wait_remaining = 0U;
        update_action(result, target.action, ports);
    }

    player.action.base_variant = 0U;
    player.action.variant_delta = opposite_legacy_direction(result.facing);
    player.action.wait_remaining = 0U;
    update_action(result, player.action, ports);

    talk_context.talk_data_offset = target.talk_data_offset;
    talk_context.instruction_offset = target.talk_initial_offset;
    talk_context.talk_script_id = target.talk_script_id;
    talk_context.source_guid = target.guid;
    talk_context.source_flags = target.flags;
    one_shot_interaction_state = 0U;
    result.talk_created = true;
    return result;
}

}  // namespace openswd3::world_map
