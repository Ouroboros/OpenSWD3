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
    test_incompatible_and_teleport_snap(test);
    test_residual_overlay(test);
    return test.exit_code();
}
