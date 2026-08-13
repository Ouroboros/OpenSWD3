#include "test.hpp"

#include "keyboard_snapshot_sdl3.hpp"

#include <SDL3/SDL_scancode.h>

#include <array>

namespace {

using openswd3::compat::u8;
using openswd3::input_time_rng::LegacyKeyboardSnapshot;

struct ExpectedMapping {
    SDL_Scancode scancode{};
    u8 dik{};
};

void test_default_game_bindings(openswd3::test::Context& test) {
    std::array<bool, SDL_SCANCODE_COUNT> state{};
    constexpr std::array<ExpectedMapping, 16> kMappings{
        ExpectedMapping{SDL_SCANCODE_ESCAPE, 0x01U},
        ExpectedMapping{SDL_SCANCODE_SPACE, 0x39U},
        ExpectedMapping{SDL_SCANCODE_RSHIFT, 0x36U},
        ExpectedMapping{SDL_SCANCODE_LEFT, 0xCBU},
        ExpectedMapping{SDL_SCANCODE_UP, 0xC8U},
        ExpectedMapping{SDL_SCANCODE_RIGHT, 0xCDU},
        ExpectedMapping{SDL_SCANCODE_DOWN, 0xD0U},
        ExpectedMapping{SDL_SCANCODE_PAGEUP, 0xC9U},
        ExpectedMapping{SDL_SCANCODE_PAGEDOWN, 0xD1U},
        ExpectedMapping{SDL_SCANCODE_RCTRL, 0x9DU},
        ExpectedMapping{SDL_SCANCODE_END, 0xCFU},
        ExpectedMapping{SDL_SCANCODE_RETURN, 0x1CU},
        ExpectedMapping{SDL_SCANCODE_R, 0x13U},
        ExpectedMapping{SDL_SCANCODE_A, 0x1EU},
        ExpectedMapping{SDL_SCANCODE_G, 0x22U},
        ExpectedMapping{SDL_SCANCODE_F1, 0x3BU},
    };
    for (const auto mapping : kMappings) {
        state[static_cast<std::size_t>(mapping.scancode)] = true;
    }

    LegacyKeyboardSnapshot snapshot{};
    openswd3::platform_sdl3::translate_sdl_keyboard_state(
        snapshot, state.data(), static_cast<int>(state.size())
    );
    for (const auto mapping : kMappings) {
        test.expect_equal(
            snapshot[mapping.dik],
            static_cast<u8>(0x80U),
            "default SDL scancode maps to its DirectInput DIK byte"
        );
    }
}

void test_extended_and_alias_mappings(openswd3::test::Context& test) {
    std::array<bool, SDL_SCANCODE_COUNT> state{};
    constexpr std::array<ExpectedMapping, 10> kMappings{
        ExpectedMapping{SDL_SCANCODE_LCTRL, 0x1DU},
        ExpectedMapping{SDL_SCANCODE_KP_ENTER, 0x9CU},
        ExpectedMapping{SDL_SCANCODE_PRINTSCREEN, 0xB7U},
        ExpectedMapping{SDL_SCANCODE_RALT, 0xB8U},
        ExpectedMapping{SDL_SCANCODE_LGUI, 0xDBU},
        ExpectedMapping{SDL_SCANCODE_APPLICATION, 0xDDU},
        ExpectedMapping{SDL_SCANCODE_MEDIA_SELECT, 0xEDU},
        ExpectedMapping{SDL_SCANCODE_INTERNATIONAL3, 0x7DU},
        ExpectedMapping{SDL_SCANCODE_BACKSLASH, 0x2BU},
        ExpectedMapping{SDL_SCANCODE_NONUSHASH, 0x2BU},
    };
    for (const auto mapping : kMappings) {
        state[static_cast<std::size_t>(mapping.scancode)] = true;
    }

    LegacyKeyboardSnapshot snapshot{};
    openswd3::platform_sdl3::translate_sdl_keyboard_state(
        snapshot, state.data(), static_cast<int>(state.size())
    );
    for (const auto mapping : kMappings) {
        test.expect_equal(
            snapshot[mapping.dik],
            static_cast<u8>(0x80U),
            "extended SDL scancode maps to the legacy DIK byte"
        );
    }
}

void test_translation_replaces_the_snapshot(openswd3::test::Context& test) {
    LegacyKeyboardSnapshot snapshot{};
    snapshot.fill(0xFFU);
    openswd3::platform_sdl3::translate_sdl_keyboard_state(snapshot, nullptr, 0);

    for (const u8 value : snapshot) {
        test.expect_equal(
            value,
            static_cast<u8>(0U),
            "each successful platform sample replaces all 256 bytes"
        );
    }
}

void test_short_press_is_latched_until_the_next_sample(
    openswd3::test::Context& test
) {
    std::array<bool, SDL_SCANCODE_COUNT> released_state{};
    LegacyKeyboardSnapshot pending{};
    LegacyKeyboardSnapshot snapshot{};

    test.expect_true(
        openswd3::platform_sdl3::latch_sdl_keyboard_press(
            pending, SDL_SCANCODE_RETURN
        ),
        "mapped SDL key-down event is accepted by the short-press latch"
    );
    test.expect_false(
        openswd3::platform_sdl3::latch_sdl_keyboard_press(
            pending, SDL_SCANCODE_UNKNOWN
        ),
        "unmapped SDL key-down event is not latched"
    );

    openswd3::platform_sdl3::translate_sdl_keyboard_state(
        snapshot, released_state.data(), static_cast<int>(released_state.size())
    );
    openswd3::platform_sdl3::merge_sdl_keyboard_press_latches(
        snapshot, pending
    );
    test.expect_equal(
        snapshot[0x1CU],
        static_cast<u8>(0x80U),
        "press and release between logical frames remains visible once"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_default_game_bindings(test);
    test_extended_and_alias_mappings(test);
    test_translation_replaces_the_snapshot(test);
    test_short_press_is_latched_until_the_next_sample(test);
    return test.exit_code();
}
