#include "test.hpp"

#include "openswd3/input_time_rng/legacy_frame_clock.hpp"

namespace {

using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyFrameClockState;

void test_interval_helpers(openswd3::test::Context& test) {
    u32 interval = 70U;
    test.expect_equal(
        openswd3::input_time_rng::clear_frame_interval(interval),
        1,
        "0x0040DD30 returns one"
    );
    test.expect_equal(interval, 0U, "0x0040DD30 clears the interval");

    test.expect_equal(
        openswd3::input_time_rng::set_frame_interval(interval, 35U),
        1,
        "0x0040DD20 returns one"
    );
    test.expect_equal(interval, 35U, "0x0040DD20 stores the argument");
}

void test_gate_boundaries(openswd3::test::Context& test) {
    LegacyFrameClockState state{};
    state.previous_accepted_frame_milliseconds = 90U;
    state.frame_interval_milliseconds = 11U;

    test.expect_true(
        !openswd3::input_time_rng::try_accept_frame_milliseconds(state, 100U),
        "elapsed below the interval rejects"
    );
    test.expect_equal(
        state.sampled_milliseconds,
        100U,
        "rejected attempts still update the shared sample"
    );
    test.expect_equal(
        state.previous_accepted_frame_milliseconds,
        90U,
        "rejected attempts preserve the accepted baseline"
    );

    test.expect_true(
        openswd3::input_time_rng::try_accept_frame_milliseconds(state, 101U),
        "elapsed equal to the interval accepts"
    );
    test.expect_equal(
        state.previous_accepted_frame_milliseconds,
        101U,
        "accepted attempts store now instead of catching up"
    );
}

void test_wrap_and_zero_interval(openswd3::test::Context& test) {
    LegacyFrameClockState state{};
    state.previous_accepted_frame_milliseconds = 0xFFFFFFF0U;
    state.frame_interval_milliseconds = 0x20U;

    test.expect_true(
        openswd3::input_time_rng::try_accept_frame_milliseconds(state, 0x10U),
        "unsigned millisecond subtraction survives one wrap"
    );

    state.frame_interval_milliseconds = 0U;
    test.expect_true(
        openswd3::input_time_rng::try_accept_frame_milliseconds(state, 0x10U),
        "zero interval accepts the same millisecond"
    );
}

void test_accepted_frame_snapshot(openswd3::test::Context& test) {
    LegacyFrameClockState state{};
    state.sampled_milliseconds = 100U;
    state.previous_input_milliseconds = 40U;

    openswd3::input_time_rng::finish_accepted_frame_time(state);
    test.expect_equal(
        state.current_frame_milliseconds,
        100U,
        "accepted frame stores the shared millisecond sample"
    );
    test.expect_equal(
        state.frame_delta_milliseconds,
        60U,
        "accepted frame delta uses independent previous input time"
    );
    test.expect_equal(
        state.previous_input_milliseconds,
        100U,
        "accepted frame advances previous input time"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_interval_helpers(test);
    test_gate_boundaries(test);
    test_wrap_and_zero_interval(test);
    test_accepted_frame_snapshot(test);
    return test.exit_code();
}
