#include "openswd3/world_map/legacy_world_collision_talk.hpp"

#include "openswd3/world_map/legacy_world_facing.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using asset_runtime::LegacyActionRecord;
using compat::i32;
using compat::u32;

constexpr std::array<i32, 8> kLegacyTalkDirectionX{
    4,
    0,
    -4,
    -4,
    -4,
    0,
    4,
    4,
};
constexpr std::array<i32, 8> kLegacyTalkDirectionY{
    4,
    4,
    4,
    0,
    -4,
    -4,
    -4,
    0,
};

[[nodiscard]] constexpr bool
has_motion(const i32 delta_x, const i32 delta_y) noexcept {
    return delta_x != 0 || delta_y != 0;
}

[[nodiscard]] constexpr u32
wrapping_scaled_add(const u32 value, const u32 scale) noexcept {
    return value + scale * 8U;
}

[[nodiscard]] bool run_collision_query(
    LegacyWorldCollisionTalkResult& result,
    const u32 player_index,
    const i32 delta_x,
    const i32 delta_y,
    LegacyWorldCollisionTalkPorts& ports
) {
    const LegacyMovementCollisionResult collision =
        ports.query_collision(player_index, delta_x, delta_y);
    ++result.collision_query_count;
    result.event_code = collision.event_code;
    result.hit_role_index = collision.hit_role_index;
    if (collision.status != LegacyMovementCollisionStatus::completed) {
        result.status = LegacyWorldCollisionTalkStatus::collision_query_failed;
        return false;
    }
    return true;
}

[[nodiscard]] constexpr u32
opposite_legacy_direction(const u32 direction) noexcept {
    return (direction & 0xFFFFFFFEU) + ((direction - 1U) & 1U);
}

}  // namespace

LegacyWorldCollisionTalkResult coordinate_legacy_world_collision_talk(
    const LegacyWorldCollisionTalkRequest request,
    const std::span<LegacyWorldRoleRecord> roles,
    const std::span<const LegacyWorldMapEvent> map_events,
    LegacyWorldTalkContext& talk_context,
    u32& one_shot_interaction_state,
    LegacyWorldCollisionTalkPorts& ports
) {
    LegacyWorldCollisionTalkResult result{
        .delta_x = request.adjusted_delta_x,
        .delta_y = request.adjusted_delta_y,
    };
    if (request.player_index >= roles.size()) {
        result.status = LegacyWorldCollisionTalkStatus::invalid_player_index;
        return result;
    }

    if (has_motion(result.delta_x, result.delta_y)) {
        if (!run_collision_query(
                result,
                request.player_index,
                result.delta_x,
                result.delta_y,
                ports
            )) {
            return result;
        }
    }
    if (result.event_code == 0U &&
        has_motion(request.original_delta_x, request.original_delta_y)) {
        if (!run_collision_query(
                result,
                request.player_index,
                request.original_delta_x,
                request.original_delta_y,
                ports
            )) {
            return result;
        }
    }

    if (result.hit_role_index != kLegacyMovementCollisionNoRole) {
        if (result.hit_role_index >= roles.size()) {
            result.status =
                LegacyWorldCollisionTalkStatus::invalid_hit_role_index;
            return result;
        }
        result.event_code = roles[result.hit_role_index].talk_script_id;
    }
    if (result.event_code == 0U) {
        return result;
    }

    const LegacyWorldMapEvent* map_event = nullptr;
    if (result.hit_role_index == kLegacyMovementCollisionNoRole) {
        map_event = find_legacy_world_map_event(map_events, result.event_code);
        if (map_event == nullptr) {
            result.status = LegacyWorldCollisionTalkStatus::missing_map_event;
            return result;
        }
        if (ports.query_internal_flag(map_event->field_0c >> 16U) == 1U) {
            result.delta_x = 0;
            result.delta_y = 0;
            result.map_event_stopped_motion = true;
        }
    }

    if (talk_context.source_guid != kLegacyWorldTalkIdleSource) {
        return result;
    }

    LegacyWorldRoleRecord& player = roles[request.player_index];
    if (map_event != nullptr) {
        const u32 direction = player.action.variant_delta;
        if (direction >= kLegacyTalkDirectionX.size()) {
            result.status =
                LegacyWorldCollisionTalkStatus::invalid_player_direction;
            return result;
        }
        talk_context.talk_data_offset = 0U;
        talk_context.instruction_offset = 0U;
        talk_context.talk_script_id =
            static_cast<compat::u16>(map_event->field_08);
        talk_context.source_guid = kLegacyWorldTalkMapEventSource;
        talk_context.source_flags = 0U;
        talk_context.world_x = player.world_x -
            (std::bit_cast<u32>(kLegacyTalkDirectionX[direction]) << 4U);
        talk_context.world_y = player.world_y -
            (std::bit_cast<u32>(kLegacyTalkDirectionY[direction]) << 4U);
        result.talk_source = LegacyWorldTalkSource::map_event;
        return result;
    }

    LegacyWorldRoleRecord& target = roles[result.hit_role_index];
    const u32 target_center_x =
        wrapping_scaled_add(target.world_x, target.action.field_2c);
    const u32 target_center_y =
        wrapping_scaled_add(target.world_y, target.action.field_30);
    const u32 facing = measure_legacy_world_controlled_role_direction(
        player, target_center_x, target_center_y
    );

    if ((target.flags & kLegacyWorldTalkTurningRoleFlag) != 0U) {
        LegacyActionRecord& action = target.action;
        action.one_shot_base_variant = action.base_variant;
        action.one_shot_variant_delta = action.variant_delta;
        action.base_variant = 0U;
        action.variant_delta = facing;
        action.wait_remaining = 0U;
        result.target_action_update_failed = ports.update_action(action) == 0U;
    }

    player.action.base_variant = 0U;
    player.action.variant_delta = opposite_legacy_direction(facing);
    player.action.wait_remaining = 0U;
    result.post_player_turn_target_update_failed =
        ports.update_action(target.action) == 0U;

    talk_context.talk_data_offset = target.talk_data_offset;
    talk_context.instruction_offset = target.talk_initial_offset;
    talk_context.source_guid = target.guid;
    talk_context.source_flags = target.flags;
    talk_context.talk_script_id = target.talk_script_id;
    one_shot_interaction_state = 0U;
    result.talk_source = LegacyWorldTalkSource::role;
    return result;
}

}  // namespace openswd3::world_map
