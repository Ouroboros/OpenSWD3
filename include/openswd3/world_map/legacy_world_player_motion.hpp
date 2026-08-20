#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_direction_input.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

namespace openswd3::world_map {

struct LegacyWorldMovementBounds {
    bool camera_left{};
    bool camera_right{};
    bool camera_up{};
    bool camera_down{};
    bool player_left{};
    bool player_right{};
    bool player_up{};
    bool player_down{};
};

struct LegacyWorldMovementRuntimeState {
    compat::i32 camera_x_transition{};
    compat::i32 player_x_transition{};
    compat::i32 camera_y_transition{};
    compat::i32 player_y_transition{};
    compat::u32 no_input_frame_count{};
    compat::u32 idle_phase{};
    compat::u32 idle_action_age{};
    compat::u32 world_frame_count{};
    compat::u32 movement_step{};
};

struct LegacyWorldMovementOptions {
    compat::u32 base_movement_step{};
    bool speed_override{};
    bool fixed_debug_speed{};
};

// sub_40DD10: store the exact 32-bit movement step and return the same value.
[[nodiscard]] compat::u32 set_legacy_world_movement_step(
    compat::u32 value, compat::u32& movement_step
) noexcept;

// sub_402F80 0x0040352E..0x0040354C: decay the idle phase after the
// transition test but before every later normal-control gate and early return.
void prepare_legacy_world_player_motion_frame(
    LegacyWorldMovementRuntimeState& state
) noexcept;

// sub_402F80 0x00403DB6..0x00403F42: update idle/movement action and
// transition state before the random-encounter tail.
void apply_legacy_world_player_motion_pre_encounter(
    LegacyWorldRoleRecord& player,
    const LegacyWorldDirectionInputResult& input,
    const LegacyWorldMovementBounds& bounds,
    LegacyWorldMovementRuntimeState& state,
    const LegacyWorldMovementOptions& options
) noexcept;

// sub_402F80 0x00404075..0x00404099: common frame counters reached after
// every encounter outcome, including immediate battle entry.
void finish_legacy_world_player_motion_frame(
    const LegacyWorldRoleRecord& player, LegacyWorldMovementRuntimeState& state
) noexcept;

// Compatibility wrapper for consumers that do not insert encounter work
// between the two physical sub_402F80 regions.
void apply_legacy_world_player_motion_state(
    LegacyWorldRoleRecord& player,
    const LegacyWorldDirectionInputResult& input,
    const LegacyWorldMovementBounds& bounds,
    LegacyWorldMovementRuntimeState& state,
    const LegacyWorldMovementOptions& options
) noexcept;

struct LegacyWorldCameraRect {
    compat::u32 left{};
    compat::u32 top{};
    compat::u32 right{};
    compat::u32 bottom{};
};

[[nodiscard]] LegacyWorldMovementBounds compute_legacy_world_movement_bounds(
    const LegacyWorldRoleRecord& player,
    const LegacyWorldCameraRect& camera,
    compat::u32 map_width_tiles,
    compat::u32 map_height_tiles
) noexcept;

void advance_legacy_world_player_and_camera(
    LegacyWorldRoleRecord& player,
    LegacyWorldCameraRect& camera,
    const LegacyWorldMovementRuntimeState& state
) noexcept;

// sub_40D0C0: center the 640x480 world viewport on a role and retain the
// original signed clamp/order behavior at both map edges.
void recenter_legacy_world_camera(
    const LegacyWorldRoleRecord& role,
    compat::u32 map_width_tiles,
    compat::u32 map_height_tiles,
    LegacyWorldCameraRect& camera
) noexcept;

// sub_40D160: calculate a 640x480 viewport for an explicit world position.
// Unlike sub_40D0C0, the horizontal leading offset is 0x140.
[[nodiscard]] LegacyWorldCameraRect calculate_legacy_world_camera_rect(
    compat::u32 world_x,
    compat::u32 world_y,
    compat::u32 map_width_tiles,
    compat::u32 map_height_tiles
) noexcept;

}  // namespace openswd3::world_map
