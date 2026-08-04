#include "keyboard_snapshot_sdl3.hpp"

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>

#include <optional>

namespace openswd3::platform_sdl3 {

namespace {

[[nodiscard]] std::optional<compat::u8> map_sdl_scancode_to_dik(
    const SDL_Scancode scancode
) noexcept {
    switch (scancode) {
        case SDL_SCANCODE_ESCAPE: return 0x01U;
        case SDL_SCANCODE_1: return 0x02U;
        case SDL_SCANCODE_2: return 0x03U;
        case SDL_SCANCODE_3: return 0x04U;
        case SDL_SCANCODE_4: return 0x05U;
        case SDL_SCANCODE_5: return 0x06U;
        case SDL_SCANCODE_6: return 0x07U;
        case SDL_SCANCODE_7: return 0x08U;
        case SDL_SCANCODE_8: return 0x09U;
        case SDL_SCANCODE_9: return 0x0AU;
        case SDL_SCANCODE_0: return 0x0BU;
        case SDL_SCANCODE_MINUS: return 0x0CU;
        case SDL_SCANCODE_EQUALS: return 0x0DU;
        case SDL_SCANCODE_BACKSPACE: return 0x0EU;
        case SDL_SCANCODE_TAB: return 0x0FU;
        case SDL_SCANCODE_Q: return 0x10U;
        case SDL_SCANCODE_W: return 0x11U;
        case SDL_SCANCODE_E: return 0x12U;
        case SDL_SCANCODE_R: return 0x13U;
        case SDL_SCANCODE_T: return 0x14U;
        case SDL_SCANCODE_Y: return 0x15U;
        case SDL_SCANCODE_U: return 0x16U;
        case SDL_SCANCODE_I: return 0x17U;
        case SDL_SCANCODE_O: return 0x18U;
        case SDL_SCANCODE_P: return 0x19U;
        case SDL_SCANCODE_LEFTBRACKET: return 0x1AU;
        case SDL_SCANCODE_RIGHTBRACKET: return 0x1BU;
        case SDL_SCANCODE_RETURN: return 0x1CU;
        case SDL_SCANCODE_LCTRL: return 0x1DU;
        case SDL_SCANCODE_A: return 0x1EU;
        case SDL_SCANCODE_S: return 0x1FU;
        case SDL_SCANCODE_D: return 0x20U;
        case SDL_SCANCODE_F: return 0x21U;
        case SDL_SCANCODE_G: return 0x22U;
        case SDL_SCANCODE_H: return 0x23U;
        case SDL_SCANCODE_J: return 0x24U;
        case SDL_SCANCODE_K: return 0x25U;
        case SDL_SCANCODE_L: return 0x26U;
        case SDL_SCANCODE_SEMICOLON: return 0x27U;
        case SDL_SCANCODE_APOSTROPHE: return 0x28U;
        case SDL_SCANCODE_GRAVE: return 0x29U;
        case SDL_SCANCODE_LSHIFT: return 0x2AU;
        case SDL_SCANCODE_BACKSLASH:
        case SDL_SCANCODE_NONUSHASH:
            return 0x2BU;
        case SDL_SCANCODE_Z: return 0x2CU;
        case SDL_SCANCODE_X: return 0x2DU;
        case SDL_SCANCODE_C: return 0x2EU;
        case SDL_SCANCODE_V: return 0x2FU;
        case SDL_SCANCODE_B: return 0x30U;
        case SDL_SCANCODE_N: return 0x31U;
        case SDL_SCANCODE_M: return 0x32U;
        case SDL_SCANCODE_COMMA: return 0x33U;
        case SDL_SCANCODE_PERIOD: return 0x34U;
        case SDL_SCANCODE_SLASH: return 0x35U;
        case SDL_SCANCODE_RSHIFT: return 0x36U;
        case SDL_SCANCODE_KP_MULTIPLY: return 0x37U;
        case SDL_SCANCODE_LALT: return 0x38U;
        case SDL_SCANCODE_SPACE: return 0x39U;
        case SDL_SCANCODE_CAPSLOCK: return 0x3AU;
        case SDL_SCANCODE_F1: return 0x3BU;
        case SDL_SCANCODE_F2: return 0x3CU;
        case SDL_SCANCODE_F3: return 0x3DU;
        case SDL_SCANCODE_F4: return 0x3EU;
        case SDL_SCANCODE_F5: return 0x3FU;
        case SDL_SCANCODE_F6: return 0x40U;
        case SDL_SCANCODE_F7: return 0x41U;
        case SDL_SCANCODE_F8: return 0x42U;
        case SDL_SCANCODE_F9: return 0x43U;
        case SDL_SCANCODE_F10: return 0x44U;
        case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45U;
        case SDL_SCANCODE_SCROLLLOCK: return 0x46U;
        case SDL_SCANCODE_KP_7: return 0x47U;
        case SDL_SCANCODE_KP_8: return 0x48U;
        case SDL_SCANCODE_KP_9: return 0x49U;
        case SDL_SCANCODE_KP_MINUS: return 0x4AU;
        case SDL_SCANCODE_KP_4: return 0x4BU;
        case SDL_SCANCODE_KP_5: return 0x4CU;
        case SDL_SCANCODE_KP_6: return 0x4DU;
        case SDL_SCANCODE_KP_PLUS: return 0x4EU;
        case SDL_SCANCODE_KP_1: return 0x4FU;
        case SDL_SCANCODE_KP_2: return 0x50U;
        case SDL_SCANCODE_KP_3: return 0x51U;
        case SDL_SCANCODE_KP_0: return 0x52U;
        case SDL_SCANCODE_KP_PERIOD: return 0x53U;
        case SDL_SCANCODE_NONUSBACKSLASH: return 0x56U;
        case SDL_SCANCODE_F11: return 0x57U;
        case SDL_SCANCODE_F12: return 0x58U;
        case SDL_SCANCODE_F13: return 0x64U;
        case SDL_SCANCODE_F14: return 0x65U;
        case SDL_SCANCODE_F15: return 0x66U;
        case SDL_SCANCODE_INTERNATIONAL2:
        case SDL_SCANCODE_LANG3:
            return 0x70U;
        case SDL_SCANCODE_INTERNATIONAL1: return 0x73U;
        case SDL_SCANCODE_INTERNATIONAL4: return 0x79U;
        case SDL_SCANCODE_INTERNATIONAL5: return 0x7BU;
        case SDL_SCANCODE_INTERNATIONAL3: return 0x7DU;
        case SDL_SCANCODE_KP_EQUALS:
        case SDL_SCANCODE_KP_EQUALSAS400:
            return 0x8DU;
        case SDL_SCANCODE_MEDIA_PREVIOUS_TRACK: return 0x90U;
        case SDL_SCANCODE_LANG5: return 0x94U;
        case SDL_SCANCODE_STOP: return 0x95U;
        case SDL_SCANCODE_MEDIA_NEXT_TRACK: return 0x99U;
        case SDL_SCANCODE_KP_ENTER: return 0x9CU;
        case SDL_SCANCODE_RCTRL: return 0x9DU;
        case SDL_SCANCODE_MUTE: return 0xA0U;
        case SDL_SCANCODE_MEDIA_PLAY_PAUSE: return 0xA2U;
        case SDL_SCANCODE_MEDIA_STOP: return 0xA4U;
        case SDL_SCANCODE_VOLUMEDOWN: return 0xAEU;
        case SDL_SCANCODE_VOLUMEUP: return 0xB0U;
        case SDL_SCANCODE_AC_HOME: return 0xB2U;
        case SDL_SCANCODE_KP_COMMA:
        case SDL_SCANCODE_INTERNATIONAL6:
            return 0xB3U;
        case SDL_SCANCODE_KP_DIVIDE: return 0xB5U;
        case SDL_SCANCODE_PRINTSCREEN:
        case SDL_SCANCODE_SYSREQ:
            return 0xB7U;
        case SDL_SCANCODE_RALT: return 0xB8U;
        case SDL_SCANCODE_PAUSE: return 0xC5U;
        case SDL_SCANCODE_HOME: return 0xC7U;
        case SDL_SCANCODE_UP: return 0xC8U;
        case SDL_SCANCODE_PAGEUP: return 0xC9U;
        case SDL_SCANCODE_LEFT: return 0xCBU;
        case SDL_SCANCODE_RIGHT: return 0xCDU;
        case SDL_SCANCODE_END: return 0xCFU;
        case SDL_SCANCODE_DOWN: return 0xD0U;
        case SDL_SCANCODE_PAGEDOWN: return 0xD1U;
        case SDL_SCANCODE_INSERT: return 0xD2U;
        case SDL_SCANCODE_DELETE: return 0xD3U;
        case SDL_SCANCODE_LGUI: return 0xDBU;
        case SDL_SCANCODE_RGUI: return 0xDCU;
        case SDL_SCANCODE_APPLICATION: return 0xDDU;
        case SDL_SCANCODE_POWER: return 0xDEU;
        case SDL_SCANCODE_SLEEP: return 0xDFU;
        case SDL_SCANCODE_WAKE: return 0xE3U;
        case SDL_SCANCODE_AC_SEARCH: return 0xE5U;
        case SDL_SCANCODE_AC_BOOKMARKS: return 0xE6U;
        case SDL_SCANCODE_AC_REFRESH: return 0xE7U;
        case SDL_SCANCODE_AC_STOP: return 0xE8U;
        case SDL_SCANCODE_AC_FORWARD: return 0xE9U;
        case SDL_SCANCODE_AC_BACK: return 0xEAU;
        case SDL_SCANCODE_MEDIA_SELECT: return 0xEDU;
        default: return std::nullopt;
    }
}

}  // namespace

void translate_sdl_keyboard_state(
    input_time_rng::LegacyKeyboardSnapshot& destination,
    const bool* const sdl_state,
    const int scancode_count
) noexcept {
    destination.fill(0U);
    if (sdl_state == nullptr || scancode_count <= 0) {
        return;
    }

    for (int index = 0; index < scancode_count; ++index) {
        if (!sdl_state[index]) {
            continue;
        }

        const auto dik = map_sdl_scancode_to_dik(
            static_cast<SDL_Scancode>(index)
        );
        if (dik.has_value()) {
            destination[*dik] |= 0x80U;
        }
    }
}

compat::i32 sample_sdl_keyboard_state(
    input_time_rng::LegacyKeyboardSnapshot& destination
) noexcept {
    int scancode_count{};
    const bool* const state = SDL_GetKeyboardState(&scancode_count);
    translate_sdl_keyboard_state(destination, state, scancode_count);
    return 1;
}

}  // namespace openswd3::platform_sdl3
