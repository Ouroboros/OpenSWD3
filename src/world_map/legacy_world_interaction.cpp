#include "openswd3/world_map/legacy_world_interaction.hpp"

#include "openswd3/world_map/legacy_world_facing.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using asset_runtime::LegacyActionRecord;
using compat::i32;
using compat::u16;
using compat::u32;
using input_time_rng::LegacyInputRecord;

constexpr std::size_t kPrimaryInputIndex = 1U;
constexpr std::size_t kLeftInputIndex = 3U;
constexpr std::size_t kUpInputIndex = 4U;
constexpr std::size_t kRightInputIndex = 5U;
constexpr std::size_t kDownInputIndex = 6U;
constexpr std::size_t kMouseRightInputIndex = 14U;
constexpr std::size_t kMouseLeftInputIndex = 15U;
constexpr std::size_t kRequiredInputCount = kMouseLeftInputIndex + 1U;

constexpr u32 kHoverCandidateFlag = 0x00008000U;
constexpr u32 kHoverCursorSuppressedFlag = 0x00000020U;
constexpr u32 kMapEventSuppressedFlag = 0x00000040U;
constexpr u32 kMapEventLock = 0x00008000U;

[[nodiscard]] constexpr bool
player_state_allows_interaction(const LegacyWorldRoleRecord& player) noexcept {
    return player.action.base_variant == 0U ||
        player.action.base_variant == 0x34U;
}

[[nodiscard]] constexpr u32
opposite_legacy_direction(const u32 direction) noexcept {
    return (direction & 0xFFFFFFFEU) + ((direction - 1U) & 1U);
}

[[nodiscard]] constexpr u32
wrapping_scaled_add(const u32 value, const u32 scale) noexcept {
    return value + scale * 8U;
}

[[nodiscard]] constexpr bool strict_unsigned_hit(
    const u32 x,
    const u32 y,
    const u32 left,
    const u32 top,
    const u32 right,
    const u32 bottom
) noexcept {
    return x > left && x < right && y > top && y < bottom;
}

void clear_input(LegacyInputRecord& input) noexcept {
    input = {};
}

void update_action(
    LegacyWorldInteractionResult& result,
    LegacyActionRecord& action,
    LegacyWorldInteractionPorts& ports
) {
    ++result.action_update_count;
    if (ports.update_action(action) == 0U) {
        ++result.action_update_failure_count;
    }
}

[[nodiscard]] LegacyWorldFacingResult facing_from_player_to_target(
    const LegacyWorldRoleRecord& player, const u32 target_x, const u32 target_y
) noexcept {
    return measure_legacy_world_facing(
        wrapping_scaled_add(player.world_x, player.action.field_2c),
        wrapping_scaled_add(player.world_y, player.action.field_30),
        target_x,
        target_y
    );
}

[[nodiscard]] u32 find_hovered_role(
    LegacyWorldInteractionResult& result,
    const LegacyWorldInteractionRequest& request,
    const std::span<const LegacyWorldRoleRecord> roles,
    LegacyWorldTalkContext& talk_context,
    LegacyWorldInteractionState& state,
    LegacyWorldInteractionPorts& ports,
    bool& map_event_allowed
) {
    if (talk_context.source_guid != kLegacyWorldTalkIdleSource ||
        !player_state_allows_interaction(roles[request.player_index])) {
        return kLegacyWorldInteractionNoRole;
    }

    for (std::size_t index = 1U; index < roles.size(); ++index) {
        const LegacyWorldRoleRecord& role = roles[index];
        if ((role.flags & kHoverCandidateFlag) == 0U ||
            role.talk_script_id == 0U || role.interaction_gate != 0U) {
            continue;
        }

        u16 width = 0U;
        u16 height = 0U;
        ++result.role_frames_requested;
        if (!ports.load_role_frame_size(
                role.action.field_4a, role.action.field_4c, width, height
            )) {
            ++result.unavailable_role_frames;
            continue;
        }

        const u32 left =
            role.world_x - role.action.draw_offset_x - request.camera.left;
        const u32 top =
            role.world_y - role.action.draw_offset_y - request.camera.top;
        if (!strict_unsigned_hit(
                request.mouse_x,
                request.mouse_y,
                left,
                top,
                left + width,
                top + height
            )) {
            continue;
        }

        const u32 role_index = static_cast<u32>(index);
        result.hovered_role_index = role_index;
        if ((role.flags & kHoverCursorSuppressedFlag) != 0U) {
            return role_index;
        }

        state.cursor_variant = kLegacyWorldRoleCursorVariant;
        if ((role.flags & kMapEventSuppressedFlag) != 0U) {
            state.cursor_variant = kLegacyWorldBlockingRoleCursorVariant;
            map_event_allowed = false;
            result.map_event_suppressed = true;
        }
        return role_index;
    }
    return static_cast<u32>(roles.size());
}

[[nodiscard]] bool coordinate_choice_chain(
    LegacyWorldInteractionResult& result,
    const LegacyWorldInteractionRequest& request,
    const LegacyWorldRoleRecord& player,
    const u32 internal_flag_9,
    const std::span<LegacyInputRecord> input_records,
    LegacyWorldInteractionState& state
) noexcept {
    const u32 hotspot_count =
        count_legacy_world_choice_hotspots(request.choice_hotspots);
    if (internal_flag_9 != 0U || hotspot_count == 0U ||
        !player_state_allows_interaction(player)) {
        return false;
    }

    state.cursor_variant = kLegacyWorldDefaultCursorVariant;
    const LegacyWorldChoiceHotspotHit hit = find_legacy_world_choice_hotspot(
        request.choice_hotspots, request.mouse_x, request.mouse_y
    );

    // With a live choice chain the original function returns here even when
    // the cursor is outside every node or the press is not its first sample.
    if (hit.hotspot == nullptr) {
        return true;
    }

    state.selected_choice_index = hit.index;
    const LegacyInputRecord& click = input_records[kMouseLeftInputIndex];
    if (click.rapid_press_multiplicity == 0U || click.held_sample_count != 1U) {
        return true;
    }

    result.source = LegacyWorldInteractionSource::choice;
    result.choice_chain_clear_requested = true;
    clear_input(input_records[kMouseLeftInputIndex]);
    result.primary_input_cleared = true;
    return true;
}

[[nodiscard]] bool activate_hovered_role(
    LegacyWorldInteractionResult& result,
    const u32 hovered_role_index,
    const LegacyWorldInteractionRequest& request,
    const std::span<LegacyWorldRoleRecord> roles,
    const std::span<LegacyInputRecord> input_records,
    LegacyWorldTalkContext& talk_context,
    LegacyWorldInteractionPorts& ports
) {
    if (hovered_role_index == kLegacyWorldInteractionNoRole ||
        hovered_role_index >= roles.size() ||
        hovered_role_index == request.player_index ||
        talk_context.source_guid != kLegacyWorldTalkIdleSource) {
        return false;
    }

    LegacyWorldRoleRecord& target = roles[hovered_role_index];
    const LegacyInputRecord& click = input_records[kMouseLeftInputIndex];
    if (target.interaction_gate != 0U || click.rapid_press_multiplicity == 0U ||
        click.held_sample_count != 1U) {
        return false;
    }

    LegacyWorldRoleRecord& player = roles[request.player_index];
    const LegacyWorldFacingResult target_facing = facing_from_player_to_target(
        player,
        wrapping_scaled_add(target.world_x, target.action.field_2c),
        wrapping_scaled_add(target.world_y, target.action.field_30)
    );
    result.facing = target_facing.direction;

    if ((target.flags & kLegacyWorldTalkTurningRoleFlag) != 0U) {
        target.action.one_shot_base_variant = target.action.base_variant;
        target.action.one_shot_variant_delta = target.action.variant_delta;
        target.action.base_variant = 0U;
        target.action.wait_remaining = 0U;
        target.action.variant_delta = target_facing.direction;
        update_action(result, target.action, ports);
    }

    player.action.base_variant = 0U;
    player.action.variant_delta =
        opposite_legacy_direction(target_facing.direction);
    player.action.wait_remaining = 0U;
    update_action(result, player.action, ports);

    talk_context.talk_data_offset = target.talk_data_offset;
    talk_context.instruction_offset = target.talk_initial_offset;
    talk_context.talk_script_id = target.talk_script_id;
    talk_context.source_guid = target.guid;
    talk_context.source_flags = target.flags;
    talk_context.world_x = target.world_x;
    talk_context.world_y = target.world_y;
    clear_input(input_records[kMouseLeftInputIndex]);
    result.primary_input_cleared = true;
    result.source = LegacyWorldInteractionSource::role;
    return true;
}

[[nodiscard]] bool coordinate_map_event(
    LegacyWorldInteractionResult& result,
    const LegacyWorldInteractionRequest& request,
    const std::span<LegacyWorldRoleRecord> roles,
    const std::span<const LegacyWorldMapEvent> map_events,
    const std::span<const compat::u8> surface_grid,
    const std::span<LegacyInputRecord> input_records,
    LegacyWorldTalkContext& talk_context,
    LegacyWorldInteractionState& state,
    LegacyWorldInteractionPorts& ports,
    const bool map_event_allowed
) {
    LegacyWorldRoleRecord& player = roles[request.player_index];
    if (talk_context.source_guid != kLegacyWorldTalkIdleSource ||
        !map_event_allowed || !player_state_allows_interaction(player)) {
        return false;
    }

    const u32 tile_y = (request.mouse_y + request.camera.top) >> 4U;
    const u32 tile_x = (request.mouse_x + request.camera.left) >> 4U;
    const u32 cell_index = tile_y * request.map_width + tile_x;
    const std::size_t byte_offset = static_cast<std::size_t>(cell_index * 4U);
    if (byte_offset > surface_grid.size() ||
        surface_grid.size() - byte_offset < 4U) {
        result.status = LegacyWorldInteractionStatus::invalid_surface_grid;
        return true;
    }

    const u32 event_code = surface_grid[byte_offset];
    result.map_event_code = event_code;
    if (event_code == 0U) {
        return false;
    }

    const LegacyWorldMapEvent* event =
        find_legacy_world_map_event(map_events, event_code);
    if (event == nullptr) {
        result.status = LegacyWorldInteractionStatus::missing_map_event;
        return true;
    }

    ++result.internal_flag_queries;
    if (ports.query_internal_flag(event->field_0c & 0xFFFFU) == 0U) {
        return false;
    }

    state.cursor_variant = kLegacyWorldMapEventCursorVariant;
    const LegacyInputRecord& click = input_records[kMouseLeftInputIndex];
    if (click.rapid_press_multiplicity == 0U || click.held_sample_count != 1U) {
        return true;
    }

    talk_context.talk_data_offset = 0U;
    talk_context.instruction_offset = 0U;
    talk_context.talk_script_id = static_cast<u16>(event->field_08 & 0x7FFFU);
    talk_context.source_guid = kLegacyWorldTalkMapEventSource;
    talk_context.source_flags = 0U;
    talk_context.world_x =
        (request.mouse_x & 0xFFFFFFF0U) + request.camera.left;
    talk_context.world_y = (request.mouse_y & 0xFFFFFFF0U) + request.camera.top;

    const LegacyWorldFacingResult target_facing = facing_from_player_to_target(
        player,
        request.mouse_x + request.camera.left,
        request.mouse_y + request.camera.top
    );
    result.facing = target_facing.direction;
    player.action.one_shot_base_variant = player.action.base_variant;
    player.action.one_shot_variant_delta = player.action.variant_delta;
    player.action.base_variant = 0U;
    player.action.variant_delta =
        opposite_legacy_direction(target_facing.direction);
    player.action.wait_remaining = 0U;
    update_action(result, player.action, ports);

    state.global_lock = kMapEventLock;
    result.source = LegacyWorldInteractionSource::map_event;
    return true;
}

void synthesize_mouse_direction(
    const u32 direction,
    const u32 multiplicity,
    const std::span<LegacyInputRecord> input_records
) noexcept {
    if (direction == 0U || direction == 4U || direction == 7U) {
        input_records[kUpInputIndex].rapid_press_multiplicity = multiplicity;
    }
    if (direction == 1U || direction == 5U || direction == 6U) {
        input_records[kDownInputIndex].rapid_press_multiplicity = multiplicity;
    }
    if (direction == 2U || direction == 4U || direction == 6U) {
        input_records[kLeftInputIndex].rapid_press_multiplicity = multiplicity;
    }
    if (direction == 3U || direction == 5U || direction == 7U) {
        input_records[kRightInputIndex].rapid_press_multiplicity = multiplicity;
    }
}

void coordinate_direction_tail(
    LegacyWorldInteractionResult& result,
    const LegacyWorldInteractionRequest& request,
    const LegacyWorldRoleRecord& player,
    const std::span<LegacyInputRecord> input_records,
    LegacyWorldInteractionState& state
) noexcept {
    const u32 player_x =
        wrapping_scaled_add(player.world_x, player.action.field_2c) -
        request.camera.left;
    const u32 player_y =
        wrapping_scaled_add(player.world_y, player.action.field_30) -
        request.camera.top;
    const LegacyWorldFacingResult facing = measure_legacy_world_facing(
        request.mouse_x, request.mouse_y, player_x, player_y
    );
    result.facing = facing.direction;
    result.distance = facing.distance;
    if (state.cursor_variant == kLegacyWorldDefaultCursorVariant) {
        state.cursor_variant = facing.direction;
    }

    const u32 multiplicity =
        input_records[kMouseRightInputIndex].rapid_press_multiplicity;
    if (((player.world_x | player.world_y) & 0x0FU) == 0U &&
        state.global_lock == 0U && multiplicity != 0U &&
        std::bit_cast<i32>(facing.distance) >= 16) {
        synthesize_mouse_direction(
            facing.direction, multiplicity, input_records
        );
    }
}

}  // namespace

u32 count_legacy_world_choice_hotspots(
    const std::span<const LegacyWorldInteractionHotspot> hotspots
) noexcept {
    return static_cast<u32>(hotspots.size());
}

LegacyWorldChoiceHotspotHit find_legacy_world_choice_hotspot(
    const std::span<const LegacyWorldInteractionHotspot> hotspots,
    const u32 mouse_x,
    const u32 mouse_y
) noexcept {
    u32 index = 0U;
    for (const LegacyWorldInteractionHotspot& hotspot : hotspots) {
        if (strict_unsigned_hit(
                mouse_x,
                mouse_y,
                hotspot.left,
                hotspot.top,
                hotspot.right,
                hotspot.bottom
            )) {
            return {index, &hotspot};
        }
        ++index;
    }
    return {index, nullptr};
}

LegacyWorldInteractionResult coordinate_legacy_world_interaction(
    const LegacyWorldInteractionRequest& request,
    const std::span<LegacyWorldRoleRecord> roles,
    const std::span<const LegacyWorldMapEvent> map_events,
    const std::span<const compat::u8> surface_grid,
    const std::span<LegacyInputRecord> input_records,
    LegacyWorldTalkContext& talk_context,
    LegacyWorldInteractionState& state,
    LegacyWorldInteractionPorts& ports
) {
    LegacyWorldInteractionResult result;
    if (request.player_index >= roles.size()) {
        result.status = LegacyWorldInteractionStatus::invalid_player_index;
        return result;
    }
    if (input_records.size() < kRequiredInputCount) {
        result.status = LegacyWorldInteractionStatus::missing_input_records;
        return result;
    }

    bool map_event_allowed = true;
    const u32 hovered_role_index = find_hovered_role(
        result, request, roles, talk_context, state, ports, map_event_allowed
    );

    ++result.internal_flag_queries;
    const u32 internal_flag_9 = ports.query_internal_flag(9U);
    if (coordinate_choice_chain(
            result,
            request,
            roles[request.player_index],
            internal_flag_9,
            input_records,
            state
        )) {
        return result;
    }

    if (activate_hovered_role(
            result,
            hovered_role_index,
            request,
            roles,
            input_records,
            talk_context,
            ports
        )) {
        return result;
    }

    if (coordinate_map_event(
            result,
            request,
            roles,
            map_events,
            surface_grid,
            input_records,
            talk_context,
            state,
            ports,
            map_event_allowed
        )) {
        return result;
    }

    coordinate_direction_tail(
        result, request, roles[request.player_index], input_records, state
    );

    if (request.dialog_chain_active &&
        input_records[kPrimaryInputIndex].rapid_press_multiplicity == 0U) {
        input_records[kPrimaryInputIndex] = input_records[kMouseLeftInputIndex];
        result.delayed_primary_input_copied = true;
    }
    return result;
}

}  // namespace openswd3::world_map
