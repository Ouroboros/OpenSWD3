#include "test.hpp"

#include "openswd3/world_map/legacy_world_player_motion.hpp"

#include <bit>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::world_map::advance_legacy_world_player_and_camera;
using openswd3::world_map::apply_legacy_world_player_motion_pre_encounter;
using openswd3::world_map::apply_legacy_world_player_motion_state;
using openswd3::world_map::finish_legacy_world_player_motion_frame;
using openswd3::world_map::prepare_legacy_world_player_motion_frame;
using openswd3::world_map::calculate_legacy_world_camera_rect;
using openswd3::world_map::compute_legacy_world_movement_bounds;
using openswd3::world_map::set_legacy_world_movement_step;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldDirectionInputResult;
using openswd3::world_map::LegacyWorldDirectionState;
using openswd3::world_map::LegacyWorldMovementBounds;
using openswd3::world_map::LegacyWorldMovementOptions;
using openswd3::world_map::LegacyWorldMovementRuntimeState;
using openswd3::world_map::LegacyWorldRoleRecord;

LegacyWorldDirectionInputResult make_input(
    const i32 x, const i32 y, const u32 direction, const u32 multiplicity = 1U
) {
    return LegacyWorldDirectionInputResult{
        .state = LegacyWorldDirectionState{direction, 0U},
        .delta_x = x,
        .delta_y = y,
        .multiplicity_bits = multiplicity,
    };
}

void test_movement_step_setter(openswd3::test::Context& test) {
    u32 movement_step = 3U;
    const u32 returned =
        set_legacy_world_movement_step(0xFFFFFFFFU, movement_step);
    test.expect_true(
        returned == 0xFFFFFFFFU && movement_step == 0xFFFFFFFFU,
        "sub_40DD10 stores and returns the complete unmodified dword"
    );
}

void test_movement_bounds(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.world_x = 404U;
    player.world_y = 340U;
    const auto centered = compute_legacy_world_movement_bounds(
        player, LegacyWorldCameraRect{100U, 100U, 740U, 580U}, 100U, 100U
    );
    test.expect_true(
        centered.camera_left && centered.camera_right && centered.camera_up &&
            centered.camera_down && centered.player_left &&
            centered.player_right && centered.player_up && centered.player_down,
        "camera and player thresholds include the exact 304/240 centers"
    );

    player.world_x = 0U;
    player.world_y = 0U;
    const auto origin = compute_legacy_world_movement_bounds(
        player, LegacyWorldCameraRect{}, 40U, 30U
    );
    test.expect_true(
        !origin.camera_left && !origin.camera_up && !origin.player_left &&
            !origin.player_up && origin.player_right && origin.player_down,
        "origin blocks negative movement but keeps positive screen movement"
    );

    player.world_x = 699U;
    player.world_y = 549U;
    const auto screen_edges = compute_legacy_world_movement_bounds(
        player, LegacyWorldCameraRect{100U, 100U, 740U, 580U}, 100U, 100U
    );
    test.expect_true(
        screen_edges.player_right && screen_edges.player_down,
        "player screen edges use strict less-than 600 and 450"
    );
    player.world_x = 700U;
    player.world_y = 550U;
    const auto outside = compute_legacy_world_movement_bounds(
        player, LegacyWorldCameraRect{100U, 100U, 740U, 580U}, 100U, 100U
    );
    test.expect_true(
        !outside.player_right && !outside.player_down,
        "player screen limits reject relative coordinates 600 and 450"
    );
}

void test_idle_state(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.action.base_variant = 8U;
    player.action.cached_base_variant = 0U;
    player.action.wait_remaining = 9U;
    LegacyWorldMovementRuntimeState state{
        .no_input_frame_count = 0U,
        .idle_phase = 1U,
        .idle_action_age = 129U,
        .world_frame_count = 0xFFFFFFFFU,
    };

    apply_legacy_world_player_motion_state(
        player, make_input(0, 0, 5U, 0U), {}, state, {}
    );
    test.expect_true(
        player.action.base_variant == 0x34U &&
            player.action.variant_delta == 5U &&
            player.action.wait_remaining == 0U &&
            state.no_input_frame_count == 1U && state.idle_phase == 0U &&
            state.idle_action_age == 130U && state.world_frame_count == 0U,
        "idle action threshold, phase decay, and counters match 0x00403DB6"
    );

    state.no_input_frame_count = 101U;
    state.idle_phase = 16U;
    state.idle_action_age = 0U;
    player.action.base_variant = 0U;
    player.action.cached_base_variant = 0U;
    apply_legacy_world_player_motion_state(
        player, make_input(0, 0, 5U, 0U), {}, state, {}
    );
    test.expect_true(
        state.no_input_frame_count == 102U && state.idle_phase == 16U,
        "idle phase increments after frame one hundred and clamps to sixteen"
    );
}

void test_encounter_split_order(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.action.base_variant = 8U;
    player.action.cached_base_variant = 8U;
    LegacyWorldMovementRuntimeState state{
        .idle_phase = 5U,
        .idle_action_age = 11U,
        .world_frame_count = 0xFFFFFFFFU,
    };
    prepare_legacy_world_player_motion_frame(state);
    test.expect_equal(
        state.idle_phase,
        u32{3U},
        "pre-control idle decay occurs before encounter and early-return gates"
    );
    apply_legacy_world_player_motion_pre_encounter(
        player,
        make_input(1, 0, 3U),
        LegacyWorldMovementBounds{.player_right = true},
        state,
        {}
    );
    test.expect_true(
        state.idle_phase == 3U && state.idle_action_age == 11U &&
            state.world_frame_count == 0xFFFFFFFFU,
        "pre-encounter motion does not repeat decay or tail counters"
    );
    finish_legacy_world_player_motion_frame(player, state);
    test.expect_true(
        state.idle_action_age == 0U && state.world_frame_count == 0U,
        "common tail runs after encounter and preserves 32-bit wrap"
    );
}

void test_transition_priority(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.action.cached_base_variant = 0U;
    player.action.cached_variant_delta = 0U;
    player.action.wait_remaining = 7U;
    LegacyWorldMovementRuntimeState state{};
    const LegacyWorldMovementBounds bounds{
        .camera_left = true,
        .camera_right = false,
        .camera_up = false,
        .camera_down = true,
        .player_left = true,
        .player_right = true,
        .player_up = true,
        .player_down = true,
    };

    apply_legacy_world_player_motion_state(
        player,
        make_input(-1, 1, 6U),
        bounds,
        state,
        LegacyWorldMovementOptions{3U, false, false}
    );
    test.expect_true(
        state.camera_x_transition == -1 && state.player_x_transition == 0 &&
            state.camera_y_transition == 1 && state.player_y_transition == 0 &&
            player.action.base_variant == 8U &&
            player.action.variant_delta == 6U &&
            player.action.wait_remaining == 0U && state.movement_step == 3U,
        "camera movement wins over player movement on each allowed axis"
    );
}

void test_speed_and_cached_action_quirks(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.action.base_variant = 0x10U;
    player.action.cached_base_variant = 0x10U;
    player.action.variant_delta = 3U;
    player.action.cached_variant_delta = 3U;
    player.action.wait_remaining = 4U;
    LegacyWorldMovementRuntimeState state{.movement_step = 9U};

    apply_legacy_world_player_motion_state(
        player,
        make_input(1, 0, 3U, 1U),
        LegacyWorldMovementBounds{.player_right = true},
        state,
        LegacyWorldMovementOptions{5U, false, false}
    );
    test.expect_true(
        player.action.base_variant == 0x10U &&
            player.action.wait_remaining == 4U && state.movement_step == 9U,
        "existing action sixteen persists and a cached action keeps its prior step"
    );

    player.action.cached_base_variant = 8U;
    apply_legacy_world_player_motion_state(
        player,
        make_input(1, 0, 3U, 2U),
        LegacyWorldMovementBounds{.player_right = true},
        state,
        LegacyWorldMovementOptions{0x80000001U, false, false}
    );
    test.expect_true(
        player.action.base_variant == 0x10U && state.movement_step == 2U,
        "double-speed step uses low-word-compatible unsigned multiplication wrap"
    );

    player.action.cached_base_variant = 8U;
    apply_legacy_world_player_motion_state(
        player,
        make_input(1, 0, 3U, 2U),
        LegacyWorldMovementBounds{.player_right = true},
        state,
        LegacyWorldMovementOptions{7U, false, true}
    );
    test.expect_equal(
        state.movement_step,
        u32{0x10U},
        "debug fixed speed overwrites the doubled base step last"
    );
}

void test_coordinate_advance(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.world_x = 100U;
    player.world_y = 200U;
    LegacyWorldCameraRect camera{10U, 20U, 30U, 40U};
    const LegacyWorldMovementRuntimeState state{
        .camera_x_transition = -1,
        .player_x_transition = 0,
        .camera_y_transition = 0,
        .player_y_transition = 1,
        .movement_step = 3U,
    };

    advance_legacy_world_player_and_camera(player, camera, state);
    test.expect_true(
        player.world_x == 97U && player.world_y == 203U && camera.left == 7U &&
            camera.right == 27U && camera.top == 20U && camera.bottom == 40U,
        "0x004120F9 applies combined player motion and camera-only scrolling"
    );

    player.world_x = 0U;
    player.world_y = 0U;
    LegacyWorldCameraRect wrapped{};
    const LegacyWorldMovementRuntimeState wrapping{
        .player_x_transition = -1,
        .player_y_transition = -1,
        .movement_step = 1U,
    };
    advance_legacy_world_player_and_camera(player, wrapped, wrapping);
    test.expect_true(
        player.world_x == std::bit_cast<u32>(i32{-1}) &&
            player.world_y == std::bit_cast<u32>(i32{-1}),
        "coordinate arithmetic preserves low 32-bit x86 wrap"
    );
}

void test_camera_recenter_helpers(openswd3::test::Context& test) {
    LegacyWorldRoleRecord role{};
    role.world_x = 0x200U;
    role.world_y = 0x180U;
    LegacyWorldCameraRect selected{};
    openswd3::world_map::recenter_legacy_world_camera(
        role, 100U, 80U, selected
    );
    const auto explicit_position = calculate_legacy_world_camera_rect(
        role.world_x, role.world_y, 100U, 80U
    );
    test.expect_true(
        selected.left == 0xD0U && selected.top == 0x90U &&
            selected.right == 0x350U && selected.bottom == 0x270U &&
            explicit_position.left == 0xC0U && explicit_position.top == 0x90U &&
            explicit_position.right == 0x340U &&
            explicit_position.bottom == 0x270U,
        "sub_40D0C0 and sub_40D160 preserve their distinct x offsets"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_movement_step_setter(test);
    test_movement_bounds(test);
    test_idle_state(test);
    test_encounter_split_order(test);
    test_transition_priority(test);
    test_speed_and_cached_action_quirks(test);
    test_coordinate_advance(test);
    test_camera_recenter_helpers(test);
    return test.exit_code();
}
