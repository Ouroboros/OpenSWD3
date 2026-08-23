#include "test.hpp"

#include "openswd3/world_map/legacy_world_frame_runtime.hpp"
#include "openswd3/world_map/legacy_world_interpolation.hpp"

#include <algorithm>
#include <bit>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::world_map::LegacyWorldFrameRuntimeState;
using openswd3::world_map::LegacyWorldInterpolationSnapshot;
using openswd3::world_map::LegacyWorldInterpolationStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldVisualMotionState;

[[nodiscard]] u32 bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] LegacyWorldInterpolationSnapshot make_snapshot(
    const i32 camera_left,
    const i32 camera_top,
    const i32 first_x,
    const i32 first_y,
    const i32 second_x,
    const i32 second_y
) {
    LegacyWorldInterpolationSnapshot snapshot;
    snapshot.valid = true;
    snapshot.map_id = 7U;
    snapshot.camera_left = camera_left;
    snapshot.camera_top = camera_top;
    snapshot.camera_right = camera_left + 640;
    snapshot.camera_bottom = camera_top + 480;
    snapshot.roles.resize(2U);
    snapshot.roles[0U].guid = 1U;
    snapshot.roles[0U].world_x = bits(first_x);
    snapshot.roles[0U].world_y = bits(first_y);
    snapshot.roles[1U].guid = 2U;
    snapshot.roles[1U].world_x = bits(second_x);
    snapshot.roles[1U].world_y = bits(second_y);
    return snapshot;
}

void test_snapshot_capture(openswd3::test::Context& test) {
    std::vector<i16> distances{3, 4};
    std::vector<i16> vertical_offsets{5, 6};
    LegacyWorldFrameRuntimeState state{
        .frame =
            {
                .camera_left = -20,
                .camera_top = 30,
                .camera_right = 620,
                .camera_bottom = 510,
                .talk_target = 9U,
            },
        .role_frame_counter = 17U,
        .flash_red_offset = 1,
        .flash_green_offset = 2,
        .flash_blue_offset = 3,
        .spatial_audio = {
            .controlled_role_index = 1U,
            .mix_level = 6,
            .distance_by_role = distances,
            .vertical_offset_by_role = vertical_offsets,
        },
    };
    std::vector<LegacyWorldRoleRecord> roles(2U);
    roles[0U].guid = 10U;
    roles[1U].guid = 11U;
    openswd3::world_map::LegacyRoleSpatialIndex spatial_index;
    spatial_index.map_height = 6U;
    spatial_index.row_heads[0U] = {1U, 2U};
    LegacyWorldInterpolationSnapshot snapshot;
    snapshot.map_id = 99U;

    test.expect_equal(
        openswd3::world_map::capture_legacy_world_interpolation_snapshot(
            snapshot, {}, spatial_index, roles, state
        ),
        LegacyWorldInterpolationStatus::ready,
        "composition entry can capture a bounded visual snapshot"
    );
    spatial_index.map_height = 12U;
    spatial_index.row_heads[0U][0U] = 9U;
    test.expect_true(
        snapshot.valid && snapshot.map_id == 99U &&
            snapshot.camera_left == -20 && snapshot.camera_top == 30 &&
            snapshot.spatial_index.map_height == 6U &&
            snapshot.spatial_index.row_heads[0U] == std::vector<u32>{1U, 2U} &&
            snapshot.role_frame_counter == 17U &&
            snapshot.controlled_role_index == 1U &&
            snapshot.distance_by_role == distances &&
            snapshot.vertical_offset_by_role == vertical_offsets &&
            snapshot.roles.size() == 2U && snapshot.roles[1U].guid == 11U,
        "capture copies visual roles, camera and spatial arrays without changing metadata"
    );
}

void test_motion_interpolation(openswd3::test::Context& test) {
    const auto previous = make_snapshot(100, -40, 120, 50, -20, 70);
    const auto current = make_snapshot(108, -32, 136, 58, -12, 86);
    LegacyWorldInterpolationSnapshot output;

    test.expect_equal(
        openswd3::world_map::interpolate_legacy_world_visual_state(
            previous, current, 5U, 10U, output
        ),
        LegacyWorldInterpolationStatus::ready,
        "compatible world snapshots interpolate"
    );
    test.expect_true(
        output.camera_left == 104 && output.camera_top == -36 &&
            std::bit_cast<i32>(output.roles[0U].world_x) == 128 &&
            std::bit_cast<i32>(output.roles[0U].world_y) == 54 &&
            std::bit_cast<i32>(output.roles[1U].world_x) == -16 &&
            std::bit_cast<i32>(output.roles[1U].world_y) == 78,
        "camera and every role use the same render-only interpolation fraction"
    );

    test.expect_equal(
        openswd3::world_map::interpolate_legacy_world_visual_state(
            previous, current, 20U, 10U, output
        ),
        LegacyWorldInterpolationStatus::ready,
        "late display frames clamp to the current logical state"
    );
    test.expect_true(
        output.camera_left == current.camera_left &&
            output.roles[0U].world_x == current.roles[0U].world_x,
        "clamped interpolation never extrapolates game movement"
    );
}

void test_low_latency_stable_velocity_projection(
    openswd3::test::Context& test
) {
    const auto older = make_snapshot(100, -40, 120, 50, -20, 70);
    const auto previous = make_snapshot(104, -36, 124, 54, -16, 74);
    const auto current = make_snapshot(108, -32, 128, 58, -12, 78);
    LegacyWorldInterpolationSnapshot output;

    test.expect_equal(
        openswd3::world_map::project_legacy_world_visual_state(
            older, previous, current, 0U, 10U, output
        ),
        LegacyWorldInterpolationStatus::ready,
        "responsive projection accepts three compatible snapshots"
    );
    test.expect_true(
        output.camera_left == current.camera_left &&
            output.camera_top == current.camera_top &&
            output.roles[0U].world_x == current.roles[0U].world_x &&
            output.roles[1U].world_y == previous.roles[1U].world_y,
        "responsive camera and controlled role start current while NPC motion starts previous"
    );

    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        older, previous, current, 5U, 10U, output
    ));
    test.expect_true(
        output.camera_left == 110 && output.camera_top == -30 &&
            std::bit_cast<i32>(output.roles[0U].world_x) == 130 &&
            std::bit_cast<i32>(output.roles[0U].world_y) == 60 &&
            std::bit_cast<i32>(output.roles[1U].world_x) == -14 &&
            std::bit_cast<i32>(output.roles[1U].world_y) == 76,
        "controlled motion projects while NPC motion interpolates every observed delta"
    );

    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        older, previous, current, 20U, 10U, output
    ));
    test.expect_true(
        output.camera_left == 112 && output.camera_top == -28 &&
            std::bit_cast<i32>(output.roles[0U].world_x) == 132 &&
            std::bit_cast<i32>(output.roles[1U].world_y) == 78,
        "responsive projection clamps while NPC interpolation reaches current"
    );

    auto scripted_current = current;
    scripted_current.roles[0U].flags |= 0x00008000U;
    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        older, previous, scripted_current, 5U, 10U, output
    ));
    test.expect_true(
        std::bit_cast<i32>(output.roles[0U].world_x) == 126 &&
            std::bit_cast<i32>(output.roles[0U].world_y) == 56,
        "scripted path motion interpolates even when it owns the controlled role"
    );

    LegacyWorldInterpolationSnapshot missing_older;
    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        missing_older, previous, current, 5U, 10U, output
    ));
    test.expect_true(
        output.camera_left == current.camera_left &&
            output.roles[0U].world_x == current.roles[0U].world_x &&
            std::bit_cast<i32>(output.roles[1U].world_x) == -14,
        "NPC interpolation does not wait for stable three-frame history"
    );
}

void test_low_latency_transition_snap(openswd3::test::Context& test) {
    const auto stationary = make_snapshot(100, 0, 100, 0, 10, 10);
    const auto moving = make_snapshot(104, 0, 104, 0, 14, 10);
    LegacyWorldInterpolationSnapshot output;

    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        stationary, stationary, moving, 1U, 2U, output
    ));
    test.expect_true(
        output.camera_left == moving.camera_left &&
            output.roles[0U].world_x == moving.roles[0U].world_x,
        "motion start snaps to current instead of guessing a new velocity"
    );

    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        stationary, moving, moving, 1U, 2U, output
    ));
    test.expect_true(
        output.camera_left == moving.camera_left &&
            output.roles[0U].world_x == moving.roles[0U].world_x,
        "motion stop snaps to current without visual coasting"
    );

    auto turned = make_snapshot(102, 0, 102, 0, 12, 10);
    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        stationary, moving, turned, 1U, 2U, output
    ));
    test.expect_true(
        output.camera_left == turned.camera_left &&
            output.roles[0U].world_x == turned.roles[0U].world_x,
        "direction or speed changes snap to current without stale prediction"
    );

    auto teleported_previous = make_snapshot(500, 0, 500, 0, 10, 10);
    auto teleported_current = make_snapshot(1000, 0, 1000, 0, 10, 10);
    static_cast<void>(openswd3::world_map::project_legacy_world_visual_state(
        stationary, teleported_previous, teleported_current, 1U, 2U, output
    ));
    test.expect_true(
        output.camera_left == teleported_current.camera_left &&
            output.roles[0U].world_x == teleported_current.roles[0U].world_x,
        "large deltas remain teleport snaps in responsive mode"
    );

    turned.map_id = 8U;
    test.expect_equal(
        openswd3::world_map::project_legacy_world_visual_state(
            stationary, moving, turned, 1U, 2U, output
        ),
        LegacyWorldInterpolationStatus::incompatible_snapshots,
        "responsive projection rejects cross-map history"
    );
}

void test_incompatible_and_teleport_snap(openswd3::test::Context& test) {
    const auto previous = make_snapshot(0, 0, 0, 0, 10, 10);
    auto current = make_snapshot(500, 0, 500, 0, 20, 20);
    LegacyWorldInterpolationSnapshot output;

    test.expect_equal(
        openswd3::world_map::interpolate_legacy_world_visual_state(
            previous, current, 1U, 2U, output
        ),
        LegacyWorldInterpolationStatus::ready,
        "large teleport deltas remain valid snapshots"
    );
    test.expect_true(
        output.camera_left == 500 &&
            std::bit_cast<i32>(output.roles[0U].world_x) == 500 &&
            std::bit_cast<i32>(output.roles[1U].world_x) == 15,
        "teleports snap while ordinary role motion still interpolates"
    );

    current.roles[1U].guid = 3U;
    test.expect_equal(
        openswd3::world_map::interpolate_legacy_world_visual_state(
            previous, current, 1U, 2U, output
        ),
        LegacyWorldInterpolationStatus::incompatible_snapshots,
        "role identity changes reject cross-scene interpolation"
    );
}

void test_scripted_motion_spans_action_wait(openswd3::test::Context& test) {
    auto initial = make_snapshot(0, 0, 0, 0, 0, 0);
    LegacyWorldVisualMotionState state;
    test.expect_equal(
        openswd3::world_map::update_legacy_world_visual_motion(
            state, initial, 0U, 10U
        ),
        LegacyWorldInterpolationStatus::ready,
        "visual motion initializes from the first accepted world snapshot"
    );

    auto moved = initial;
    moved.roles[1U].world_x = bits(4);
    moved.roles[1U].action.wait_remaining = 3U;
    static_cast<void>(openswd3::world_map::update_legacy_world_visual_motion(
        state, moved, 10U, 10U
    ));

    LegacyWorldInterpolationSnapshot display = moved;
    static_cast<void>(openswd3::world_map::apply_legacy_world_visual_motion(
        state, 10U, display
    ));
    test.expect_equal(
        std::bit_cast<i32>(display.roles[1U].world_x),
        i32{0},
        "scripted display motion starts from the prior visual position"
    );

    auto held = moved;
    held.roles[1U].action.wait_remaining = 2U;
    static_cast<void>(openswd3::world_map::update_legacy_world_visual_motion(
        state, held, 20U, 10U
    ));
    display = held;
    static_cast<void>(openswd3::world_map::apply_legacy_world_visual_motion(
        state, 20U, display
    ));
    test.expect_equal(
        std::bit_cast<i32>(display.roles[1U].world_x),
        i32{1},
        "unchanged logical snapshots keep advancing the active visual segment"
    );

    display = held;
    static_cast<void>(openswd3::world_map::apply_legacy_world_visual_motion(
        state, 30U, display
    ));
    test.expect_equal(
        std::bit_cast<i32>(display.roles[1U].world_x),
        i32{2},
        "path motion reaches its midpoint across the action wait cycle"
    );

    display = held;
    static_cast<void>(openswd3::world_map::apply_legacy_world_visual_motion(
        state, 50U, display
    ));
    test.expect_equal(
        std::bit_cast<i32>(display.roles[1U].world_x),
        i32{4},
        "path motion reaches the logical target when the action wait expires"
    );
}

void test_temporal_subpixel_quantization(openswd3::test::Context& test) {
    auto initial = make_snapshot(0, 0, 0, 0, 0, 0);
    LegacyWorldVisualMotionState state;
    static_cast<void>(openswd3::world_map::update_legacy_world_visual_motion(
        state, initial, 0U, 10U
    ));

    auto moved = initial;
    moved.roles[1U].world_x = bits(1);
    moved.roles[1U].world_y = bits(1);
    moved.roles[1U].action.wait_remaining = 3U;
    static_cast<void>(openswd3::world_map::update_legacy_world_visual_motion(
        state, moved, 10U, 10U
    ));

    u32 horizontal_upper_samples = 0U;
    u32 vertical_upper_samples = 0U;
    for (u32 sample = 0U; sample < 256U; ++sample) {
        LegacyWorldInterpolationSnapshot display = moved;
        static_cast<void>(openswd3::world_map::apply_legacy_world_visual_motion(
            state, 30U, display
        ));
        if (std::bit_cast<i32>(display.roles[1U].world_x) == 1) {
            ++horizontal_upper_samples;
        }
        if (std::bit_cast<i32>(display.roles[1U].world_y) == 1) {
            ++vertical_upper_samples;
        }
    }
    test.expect_equal(
        horizontal_upper_samples,
        u32{256U},
        "horizontal half-pixels use monotonic nearest sampling without backtracking"
    );
    test.expect_equal(
        vertical_upper_samples,
        u32{128U},
        "240 Hz low-discrepancy vertical sampling represents a half-pixel without changing logic"
    );

    auto teleported = moved;
    teleported.roles[1U].world_x = bits(500);
    static_cast<void>(openswd3::world_map::update_legacy_world_visual_motion(
        state, teleported, 60U, 10U
    ));
    LegacyWorldInterpolationSnapshot display = teleported;
    static_cast<void>(openswd3::world_map::apply_legacy_world_visual_motion(
        state, 60U, display
    ));
    test.expect_equal(
        std::bit_cast<i32>(display.roles[1U].world_x),
        i32{500},
        "large visual deltas remain immediate teleport snaps"
    );
}

void test_residual_overlay(openswd3::test::Context& test) {
    LegacyFramebuffer output;
    LegacyFramebuffer current_base;
    LegacyFramebuffer current_final;
    std::ranges::fill(output.physical_pixels(), 20U);
    std::ranges::fill(current_base.physical_pixels(), 10U);
    std::ranges::fill(current_final.physical_pixels(), 10U);
    current_final.physical_pixels()[1U] = 99U;
    current_final.physical_pixels()[100U] = 77U;

    test.expect_equal(
        openswd3::world_map::apply_legacy_world_frame_residual(
            output, current_base, current_final
        ),
        LegacyWorldInterpolationStatus::ready,
        "current non-base pixels can be restored over an interpolated scene"
    );
    test.expect_true(
        output.physical_pixels()[0U] == 20U &&
            output.physical_pixels()[1U] == 99U &&
            output.physical_pixels()[100U] == 77U,
        "residual overlay preserves interpolated base pixels and current UI/effects"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_snapshot_capture(test);
    test_motion_interpolation(test);
    test_low_latency_stable_velocity_projection(test);
    test_low_latency_transition_snap(test);
    test_incompatible_and_teleport_snap(test);
    test_scripted_motion_spans_action_wait(test);
    test_temporal_subpixel_quantization(test);
    test_residual_overlay(test);
    return test.exit_code();
}
