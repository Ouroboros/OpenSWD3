#include "test.hpp"

#include "openswd3/world_map/legacy_world_frame_tail.hpp"
#include "openswd3/world_map/legacy_world_selection_scroll.hpp"

#include <array>
#include <bit>
#include <limits>

namespace {

using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::world_map::advance_legacy_world_selection_scroll;
using openswd3::world_map::kLegacyWorldSelectionSentinel;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldSelectionScrollState;
using openswd3::world_map::LegacyWorldSelectionScrollStatus;
using openswd3::world_map::LegacyWorldViewportRestoreState;
using openswd3::world_map::restore_legacy_world_viewport_after_selection_scroll;

constexpr i16 sentinel() noexcept {
    return std::bit_cast<i16>(kLegacyWorldSelectionSentinel);
}

void test_entry_gates(openswd3::test::Context& test) {
    LegacyWorldCameraRect camera{10U, 20U, 650U, 500U};
    LegacyWorldSelectionScrollState state{
        .cursor_word_index = 6U,
        .frames_remaining = 9,
        .frame_interval = 3,
        .saved_left = 77U,
        .saved_top = 88U,
    };

    const std::array<i16, 2> inactive{sentinel(), 5};
    test.expect_equal(
        advance_legacy_world_selection_scroll(inactive, 24U, camera, state),
        LegacyWorldSelectionScrollStatus::selection_inactive,
        "the first CFCF word returns before reading the cursor"
    );
    test.expect_true(
        camera.left == 10U && camera.top == 20U &&
            state.cursor_word_index == 6U && state.frames_remaining == 9 &&
            state.saved_left == 77U,
        "inactive selection leaves camera and state untouched"
    );

    const std::array<i16, 2> active{1, 2};
    test.expect_equal(
        advance_legacy_world_selection_scroll(active, 0x16U, camera, state),
        LegacyWorldSelectionScrollStatus::map_excluded,
        "map 22 returns before reading the cursor"
    );
    test.expect_true(
        camera.left == 10U && camera.top == 20U &&
            state.cursor_word_index == 6U && state.frames_remaining == 9,
        "map exclusion leaves camera and state untouched"
    );
}

void test_cursor_rewind_and_signed_scroll(openswd3::test::Context& test) {
    const std::array<i16, 6> selection{-3, 4, 9, 10, sentinel(), sentinel()};
    LegacyWorldCameraRect camera{1U, 2U, 641U, 482U};
    LegacyWorldSelectionScrollState state{
        .cursor_word_index = 4U,
        .frames_remaining = 2,
        .frame_interval = 7,
    };

    test.expect_equal(
        advance_legacy_world_selection_scroll(selection, 24U, camera, state),
        LegacyWorldSelectionScrollStatus::completed,
        "a later sentinel rewinds and consumes the first pair"
    );
    test.expect_true(
        state.cursor_word_index == 0U && state.frames_remaining == 1 &&
            state.saved_left == 1U && state.saved_top == 2U &&
            camera.left == 0xFFFFFFFEU && camera.right == 638U &&
            camera.top == 6U && camera.bottom == 486U,
        "rewound signed deltas scroll the full rectangle with u32 wrap"
    );
}

void test_interval_reload_and_cursor_advance(openswd3::test::Context& test) {
    const std::array<i16, 5> selection{2, -3, 5, 6, sentinel()};
    LegacyWorldCameraRect camera{100U, 200U, 740U, 680U};
    LegacyWorldSelectionScrollState state{
        .cursor_word_index = 0U,
        .frames_remaining = 1,
        .frame_interval = 4,
    };

    test.expect_equal(
        advance_legacy_world_selection_scroll(selection, 24U, camera, state),
        LegacyWorldSelectionScrollStatus::completed,
        "an expired pair still applies before the next cursor is selected"
    );
    test.expect_true(
        state.cursor_word_index == 2U && state.frames_remaining == 4 &&
            state.saved_left == 100U && state.saved_top == 200U &&
            camera.left == 102U && camera.right == 742U && camera.top == 197U &&
            camera.bottom == 677U,
        "zero countdown reloads the interval and advances by two words"
    );
}

void test_countdown_wrap(openswd3::test::Context& test) {
    const std::array<i16, 2> selection{0, 0};
    LegacyWorldCameraRect camera{};
    LegacyWorldSelectionScrollState state{
        .cursor_word_index = 0U,
        .frames_remaining = std::numeric_limits<i32>::min(),
        .frame_interval = 8,
    };

    test.expect_equal(
        advance_legacy_world_selection_scroll(selection, 24U, camera, state),
        LegacyWorldSelectionScrollStatus::completed,
        "INT32_MIN decrement wraps to a positive x86 value"
    );
    test.expect_true(
        state.frames_remaining == std::numeric_limits<i32>::max() &&
            state.cursor_word_index == 0U,
        "wrapped positive countdown skips interval reload and cursor advance"
    );
}

void test_invalid_windows(openswd3::test::Context& test) {
    LegacyWorldCameraRect camera{10U, 20U, 650U, 500U};
    LegacyWorldSelectionScrollState state{
        .cursor_word_index = 2U,
        .frames_remaining = 3,
        .frame_interval = 4,
        .saved_left = 5U,
        .saved_top = 6U,
    };

    const std::array<i16, 2> selection{1, 2};
    test.expect_equal(
        advance_legacy_world_selection_scroll(selection, 24U, camera, state),
        LegacyWorldSelectionScrollStatus::invalid_selection_window,
        "a cursor outside the supplied modern span is isolated"
    );
    test.expect_true(
        camera.left == 10U && camera.top == 20U &&
            state.frames_remaining == 3 && state.saved_left == 5U,
        "invalid cursor returns before legacy state mutation"
    );

    const std::array<i16, 1> missing_y{1};
    state.cursor_word_index = 0U;
    test.expect_equal(
        advance_legacy_world_selection_scroll(missing_y, 24U, camera, state),
        LegacyWorldSelectionScrollStatus::invalid_selection_window,
        "a pair missing its y word is isolated"
    );
}

void test_scroll_and_frame_tail_restore(openswd3::test::Context& test) {
    const std::array<i16, 3> selection{7, -9, sentinel()};
    const LegacyWorldCameraRect original{100U, 200U, 740U, 680U};
    LegacyWorldCameraRect camera = original;
    LegacyWorldSelectionScrollState scroll{
        .frames_remaining = 2,
        .frame_interval = 2,
    };

    test.expect_equal(
        advance_legacy_world_selection_scroll(selection, 24U, camera, scroll),
        LegacyWorldSelectionScrollStatus::completed,
        "frame-start selection scroll completes"
    );
    test.expect_true(
        camera.left == 107U && camera.top == 191U && camera.right == 747U &&
            camera.bottom == 671U,
        "the viewport remains temporarily scrolled for composition"
    );

    restore_legacy_world_viewport_after_selection_scroll(
        camera,
        LegacyWorldViewportRestoreState{
            .first_selection_word = kLegacyWorldSelectionSentinel,
            .map_id = 24U,
            .saved_left = scroll.saved_left,
            .saved_top = scroll.saved_top,
        }
    );
    test.expect_true(
        camera.left == 107U && camera.top == 191U,
        "an inactive frame-tail gate does not restore"
    );

    restore_legacy_world_viewport_after_selection_scroll(
        camera,
        LegacyWorldViewportRestoreState{
            .first_selection_word = 7U,
            .map_id = 24U,
            .saved_left = scroll.saved_left,
            .saved_top = scroll.saved_top,
        }
    );
    test.expect_true(
        camera.left == original.left && camera.top == original.top &&
            camera.right == original.right && camera.bottom == original.bottom,
        "active selection restores the exact pre-scroll 640x480 viewport"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_entry_gates(test);
    test_cursor_rewind_and_signed_scroll(test);
    test_interval_reload_and_cursor_advance(test);
    test_countdown_wrap(test);
    test_invalid_windows(test);
    test_scroll_and_frame_tail_restore(test);
    return test.exit_code();
}
