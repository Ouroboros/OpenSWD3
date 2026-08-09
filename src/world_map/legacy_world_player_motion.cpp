#include "openswd3/world_map/legacy_world_player_motion.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] compat::u32 wrapping_multiply(
    const compat::i32 left,
    const compat::u32 right
) noexcept {
    return std::bit_cast<compat::u32>(left) * right;
}

void update_idle_state(
    LegacyWorldRoleRecord& player,
    LegacyWorldMovementRuntimeState& state
) noexcept {
    if (player.action.base_variant != player.action.cached_base_variant) {
        player.action.wait_remaining = 0U;
    }
    player.action.base_variant = 0U;
    if (state.idle_action_age > 128U) {
        player.action.base_variant = 0x34U;
    }

    ++state.no_input_frame_count;
    if (state.no_input_frame_count <= 100U &&
        (state.idle_phase & 0x0FU) == 0U) {
        return;
    }
    ++state.idle_phase;
    if (std::bit_cast<compat::i32>(state.idle_phase) > 16) {
        state.idle_phase = 16U;
    }
}

void select_axis_transitions(
    const LegacyWorldDirectionInputResult& input,
    const LegacyWorldMovementBounds& bounds,
    LegacyWorldMovementRuntimeState& state
) noexcept {
    if (input.delta_x == -1) {
        if (bounds.camera_left) {
            state.camera_x_transition = -1;
        } else if (bounds.player_left) {
            state.player_x_transition = -1;
        }
    } else if (input.delta_x != 0) {
        if (bounds.camera_right) {
            state.camera_x_transition = 1;
        } else if (bounds.player_right) {
            state.player_x_transition = 1;
        }
    }

    if (input.delta_y == -1) {
        if (bounds.camera_up) {
            state.camera_y_transition = -1;
        } else if (bounds.player_up) {
            state.player_y_transition = -1;
        }
    } else if (input.delta_y != 0) {
        if (bounds.camera_down) {
            state.camera_y_transition = 1;
        } else if (bounds.player_down) {
            state.player_y_transition = 1;
        }
    }
}

void update_movement_action(
    LegacyWorldRoleRecord& player,
    const LegacyWorldDirectionInputResult& input,
    LegacyWorldMovementRuntimeState& state,
    const LegacyWorldMovementOptions& options
) noexcept {
    if (player.action.base_variant != 0x10U) {
        player.action.base_variant = 8U;
    }
    if ((input.multiplicity_bits & 2U) != 0U || options.speed_override) {
        player.action.base_variant = 0x10U;
    }

    if (player.action.variant_delta !=
        player.action.cached_variant_delta) {
        player.action.wait_remaining = 0U;
    }
    if (player.action.base_variant !=
        player.action.cached_base_variant) {
        player.action.wait_remaining = 0U;
        compat::u32 step = options.base_movement_step;
        if (options.speed_override ||
            (input.multiplicity_bits & 2U) != 0U) {
            step *= 2U;
        }
        if (options.fixed_debug_speed) {
            step = 0x10U;
        }
        state.movement_step = step;
    }
}

}  // namespace

LegacyWorldMovementBounds compute_legacy_world_movement_bounds(
    const LegacyWorldRoleRecord& player,
    const LegacyWorldCameraRect& camera,
    const compat::u32 map_width_tiles,
    const compat::u32 map_height_tiles
) noexcept {
    const compat::u32 relative_x = player.world_x - camera.left;
    const compat::u32 relative_y = player.world_y - camera.top;
    const compat::u32 map_width_pixels = map_width_tiles << 4U;
    const compat::u32 map_height_pixels = map_height_tiles << 4U;

    return LegacyWorldMovementBounds{
        .camera_left = relative_x <= 304U &&
            std::bit_cast<compat::i32>(camera.left) > 0,
        .camera_right =
            std::bit_cast<compat::i32>(camera.right) <
                std::bit_cast<compat::i32>(map_width_pixels) &&
            relative_x >= 304U,
        .camera_up = relative_y <= 240U &&
            std::bit_cast<compat::i32>(camera.top) > 0,
        .camera_down =
            std::bit_cast<compat::i32>(camera.bottom) <
                std::bit_cast<compat::i32>(map_height_pixels) &&
            relative_y >= 240U,
        .player_left = player.world_x > 0U,
        .player_right = relative_x < 600U,
        .player_up = player.world_y > 0U,
        .player_down = relative_y < 450U,
    };
}

void apply_legacy_world_player_motion_state(
    LegacyWorldRoleRecord& player,
    const LegacyWorldDirectionInputResult& input,
    const LegacyWorldMovementBounds& bounds,
    LegacyWorldMovementRuntimeState& state,
    const LegacyWorldMovementOptions& options
) noexcept {
    if (state.no_input_frame_count == 0U) {
        state.idle_phase -= 2U;
        if (std::bit_cast<compat::i32>(state.idle_phase) < 0) {
            state.idle_phase = 0U;
        }
    }

    player.action.variant_delta = input.state.direction;
    if (input.delta_x == 0 && input.delta_y == 0) {
        update_idle_state(player, state);
    } else {
        state.no_input_frame_count = 0U;
        select_axis_transitions(input, bounds, state);
        update_movement_action(player, input, state, options);
    }

    ++state.idle_action_age;
    if (player.action.base_variant != 0U &&
        player.action.base_variant != 0x34U) {
        state.idle_action_age = 0U;
    }
    ++state.world_frame_count;
}

void advance_legacy_world_player_and_camera(
    LegacyWorldRoleRecord& player,
    LegacyWorldCameraRect& camera,
    const LegacyWorldMovementRuntimeState& state
) noexcept {
    const compat::i32 player_x = state.camera_x_transition +
        state.player_x_transition;
    const compat::i32 player_y = state.camera_y_transition +
        state.player_y_transition;
    player.world_x += wrapping_multiply(player_x, state.movement_step);
    player.world_y += wrapping_multiply(player_y, state.movement_step);

    const compat::u32 camera_x = wrapping_multiply(
        state.camera_x_transition,
        state.movement_step
    );
    const compat::u32 camera_y = wrapping_multiply(
        state.camera_y_transition,
        state.movement_step
    );
    camera.left += camera_x;
    camera.right += camera_x;
    camera.top += camera_y;
    camera.bottom += camera_y;
}

}  // namespace openswd3::world_map
