#include "test.hpp"

#include "openswd3/app/display_refresh.hpp"

#include <cstdint>

namespace {

void test_legacy_coupled_mode(openswd3::test::Context& test) {
    openswd3::app::DisplayRefreshClockState state;
    openswd3::app::configure_display_refresh_clock(state, 0U, 123U);

    test.expect_true(
        !openswd3::app::independent_display_refresh_enabled(state) &&
            state.previous_refresh_nanoseconds == 123U &&
            state.refresh_interval_nanoseconds == 0U &&
            !openswd3::app::try_accept_display_refresh(state, 1'000'000'000U),
        "zero display FPS keeps presentation coupled to accepted game frames"
    );
}

void test_independent_sixty_fps_clock(openswd3::test::Context& test) {
    constexpr std::uint64_t start = 1'000U;
    constexpr std::uint64_t interval = 16'666'666U;
    openswd3::app::DisplayRefreshClockState state;
    openswd3::app::configure_display_refresh_clock(state, 60U, start);

    test.expect_true(
        openswd3::app::independent_display_refresh_enabled(state) &&
            state.refresh_interval_nanoseconds == interval &&
            !openswd3::app::try_accept_display_refresh(
                state, start + interval - 1U
            ) &&
            state.previous_refresh_nanoseconds == start,
        "independent display refresh does not accept before its own deadline"
    );

    test.expect_true(
        openswd3::app::try_accept_display_refresh(state, start + interval) &&
            state.previous_refresh_nanoseconds == start + interval,
        "independent display refresh accepts exactly at its own deadline"
    );
}

void test_independent_two_hundred_forty_fps_clock(
    openswd3::test::Context& test
) {
    constexpr std::uint64_t start = 500U;
    constexpr std::uint64_t interval = 4'166'666U;
    openswd3::app::DisplayRefreshClockState state;
    openswd3::app::configure_display_refresh_clock(state, 240U, start);

    test.expect_true(
        state.refresh_interval_nanoseconds == interval &&
            !openswd3::app::try_accept_display_refresh(
                state, start + interval - 1U
            ) &&
            openswd3::app::try_accept_display_refresh(state, start + interval),
        "240 FPS uses its exact integer nanosecond display deadline"
    );
}

void test_refresh_does_not_catch_up(openswd3::test::Context& test) {
    constexpr std::uint64_t interval = 8'333'333U;
    openswd3::app::DisplayRefreshClockState state;
    openswd3::app::configure_display_refresh_clock(state, 120U, 100U);

    const std::uint64_t late = 100U + interval * 5U + 17U;
    test.expect_true(
        openswd3::app::try_accept_display_refresh(state, late) &&
            state.previous_refresh_nanoseconds == late &&
            !openswd3::app::try_accept_display_refresh(
                state, late + interval - 1U
            ),
        "a late display refresh emits once and never runs catch-up presentations"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_legacy_coupled_mode(test);
    test_independent_sixty_fps_clock(test);
    test_independent_two_hundred_forty_fps_clock(test);
    test_refresh_does_not_catch_up(test);
    return test.exit_code();
}
