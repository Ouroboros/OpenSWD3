#include "test.hpp"

#include "openswd3/world_map/legacy_world_camera_pan.hpp"

#include <limits>

namespace {

using openswd3::world_map::advance_legacy_world_camera_pan;
using openswd3::world_map::LegacyWorldCameraPanState;
using openswd3::world_map::LegacyWorldCameraRect;

void test_inactive_state_returns_without_mutation(
    openswd3::test::Context& test
) {
    LegacyWorldCameraRect camera{10U, 20U, 650U, 500U};
    LegacyWorldCameraPanState state{
        .remaining_x = 0,
        .remaining_y = 0,
        .step_x = 7,
        .step_y = -9,
    };

    test.expect_true(
        !advance_legacy_world_camera_pan(camera, state),
        "two zero remaining axes skip sub_414570"
    );
    test.expect_true(
        camera.left == 10U && camera.top == 20U && camera.right == 650U &&
            camera.bottom == 500U && state.step_x == 7 && state.step_y == -9,
        "the early return does not sanitize dormant step fields"
    );
}

void test_signed_steps_and_axis_completion(openswd3::test::Context& test) {
    LegacyWorldCameraRect camera{1U, 2U, 641U, 482U};
    LegacyWorldCameraPanState state{
        .remaining_x = 8,
        .remaining_y = -3,
        .step_x = 4,
        .step_y = -3,
    };

    test.expect_true(
        advance_legacy_world_camera_pan(camera, state),
        "an active axis advances the full camera rectangle"
    );
    test.expect_true(
        camera.left == 5U && camera.right == 645U &&
            camera.top == 0xFFFFFFFFU && camera.bottom == 479U &&
            state.remaining_x == 4 && state.remaining_y == 0 &&
            state.step_x == 4 && state.step_y == 0,
        "signed steps use 32-bit wrap and clear only a completed axis"
    );

    test.expect_true(
        advance_legacy_world_camera_pan(camera, state),
        "the unfinished x axis advances on the following frame"
    );
    test.expect_true(
        camera.left == 9U && camera.right == 649U &&
            camera.top == 0xFFFFFFFFU && camera.bottom == 479U &&
            state.remaining_x == 0 && state.remaining_y == 0 &&
            state.step_x == 0 && state.step_y == 0,
        "the final exact subtraction clears the remaining x step"
    );
}

void test_active_peer_preserves_noncanonical_axis(
    openswd3::test::Context& test
) {
    LegacyWorldCameraRect camera{};
    LegacyWorldCameraPanState state{
        .remaining_x = 0,
        .remaining_y = 1,
        .step_x = 2,
        .step_y = 1,
    };

    test.expect_true(
        advance_legacy_world_camera_pan(camera, state),
        "one active axis enters the shared update body"
    );
    test.expect_true(
        camera.left == 2U && camera.right == 2U && camera.top == 1U &&
            camera.bottom == 1U && state.remaining_x == -2 &&
            state.remaining_y == 0 && state.step_x == 2 && state.step_y == 0,
        "the shared body does not independently gate a zero axis"
    );
}

void test_overshoot_does_not_clamp(openswd3::test::Context& test) {
    LegacyWorldCameraRect camera{};
    LegacyWorldCameraPanState state{
        .remaining_x = 1,
        .remaining_y = -1,
        .step_x = 2,
        .step_y = -2,
    };

    test.expect_true(
        advance_legacy_world_camera_pan(camera, state),
        "non-divisible remaining distances still enter the shared update"
    );
    test.expect_true(
        camera.left == 2U && camera.right == 2U && camera.top == 0xFFFFFFFEU &&
            camera.bottom == 0xFFFFFFFEU && state.remaining_x == -1 &&
            state.remaining_y == 1 && state.step_x == 2 && state.step_y == -2,
        "overshoot wraps past zero without clamping or clearing either step"
    );
}

void test_x86_integer_wrap(openswd3::test::Context& test) {
    LegacyWorldCameraRect camera{0xFFFFFFFEU, 0xFFFFFFFFU, 0U, 1U};
    LegacyWorldCameraPanState state{
        .remaining_x = std::numeric_limits<openswd3::compat::i32>::min(),
        .remaining_y = 0,
        .step_x = 1,
        .step_y = 0,
    };

    test.expect_true(
        advance_legacy_world_camera_pan(camera, state),
        "x86 subtraction overflow remains defined in the port"
    );
    test.expect_true(
        camera.left == 0xFFFFFFFFU && camera.right == 1U &&
            camera.top == 0xFFFFFFFFU && camera.bottom == 1U &&
            state.remaining_x ==
                std::numeric_limits<openswd3::compat::i32>::max(),
        "camera addition and remaining subtraction wrap modulo 2^32"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_inactive_state_returns_without_mutation(test);
    test_signed_steps_and_axis_completion(test);
    test_active_peer_preserves_noncanonical_axis(test);
    test_overshoot_does_not_clamp(test);
    test_x86_integer_wrap(test);
    return test.exit_code();
}
