#include "test.hpp"

#include "mouse_sdl3.hpp"

#include <SDL3/SDL_mouse.h>

namespace {

void test_virtual_absolute_axes_and_buttons(
    openswd3::test::Context& test
) {
    openswd3::platform_sdl3::SdlMouseDeviceState state{};
    auto sample = openswd3::platform_sdl3::accumulate_sdl_mouse_sample(
        state,
        0.5,
        0.25,
        SDL_BUTTON_LMASK
    );
    test.expect_equal(sample.absolute_x, 0, "fractional x remains accumulated");
    test.expect_equal(sample.absolute_y, 0, "fractional y remains accumulated");
    test.expect_equal(
        sample.button_0,
        static_cast<openswd3::compat::u8>(0x80U),
        "SDL left button maps to legacy button zero"
    );
    test.expect_equal(
        sample.button_1,
        static_cast<openswd3::compat::u8>(0U),
        "released SDL right button stays clear"
    );

    sample = openswd3::platform_sdl3::accumulate_sdl_mouse_sample(
        state,
        0.5,
        0.75,
        SDL_BUTTON_RMASK
    );
    test.expect_equal(sample.absolute_x, 1, "fractional x carries to next sample");
    test.expect_equal(sample.absolute_y, 1, "fractional y carries to next sample");
    test.expect_equal(
        sample.button_0,
        static_cast<openswd3::compat::u8>(0U),
        "released SDL left button stays clear"
    );
    test.expect_equal(
        sample.button_1,
        static_cast<openswd3::compat::u8>(0x80U),
        "SDL right button maps to legacy button one"
    );

    sample = openswd3::platform_sdl3::accumulate_sdl_mouse_sample(
        state,
        -2.25,
        -3.5,
        SDL_BUTTON_LMASK | SDL_BUTTON_RMASK | SDL_BUTTON_MMASK
    );
    test.expect_equal(sample.absolute_x, -1, "negative x truncates toward zero");
    test.expect_equal(sample.absolute_y, -2, "negative y truncates toward zero");
    test.expect_equal(
        sample.button_0,
        static_cast<openswd3::compat::u8>(0x80U),
        "left remains set when extra SDL buttons are present"
    );
    test.expect_equal(
        sample.button_1,
        static_cast<openswd3::compat::u8>(0x80U),
        "right remains set when extra SDL buttons are present"
    );
}

void test_short_click_latches(openswd3::test::Context& test) {
    openswd3::input_time_rng::LegacyMouseDeviceSample sample{};
    openswd3::platform_sdl3::merge_sdl_mouse_press_latches(sample, 3U);
    test.expect_true(
        sample.button_0 == 0x80U && sample.button_1 == 0x80U,
        "left and right clicks survive release before the next logical frame"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_virtual_absolute_axes_and_buttons(test);
    test_short_click_latches(test);
    return test.exit_code();
}
