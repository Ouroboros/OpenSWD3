#include "event_translation.hpp"

#include <SDL3/SDL.h>

namespace openswd3::platform_sdl3 {

namespace {

constexpr compat::u32 kLegacySizeRestored = 0U;
constexpr compat::u32 kLegacySizeMinimized = 1U;
constexpr compat::u32 kLegacySizeMaximized = 2U;

constexpr compat::u32 kLegacyKeyEscape = 0x1BU;
constexpr compat::u32 kLegacyKeyScreenshot = 0x50U;
constexpr compat::u32 kLegacyKeyPause = 0x77U;
constexpr compat::u32 kLegacyKeyEnter = 0x0DU;

constexpr compat::u32 kLegacyMouseLeft = 0x01U;
constexpr compat::u32 kLegacyMouseRight = 0x02U;
constexpr compat::u32 kLegacyMouseShift = 0x04U;
constexpr compat::u32 kLegacyMouseControl = 0x08U;
constexpr compat::u32 kLegacyMouseMiddle = 0x10U;
constexpr compat::u32 kLegacyMouseX1 = 0x20U;
constexpr compat::u32 kLegacyMouseX2 = 0x40U;

[[nodiscard]] compat::u32 current_legacy_mouse_button_state() {
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
    const SDL_Keymod modifiers = SDL_GetModState();
    compat::u32 result = 0U;

    if ((buttons & SDL_BUTTON_LMASK) != 0U) {
        result |= kLegacyMouseLeft;
    }
    if ((buttons & SDL_BUTTON_RMASK) != 0U) {
        result |= kLegacyMouseRight;
    }
    if ((modifiers & SDL_KMOD_SHIFT) != 0U) {
        result |= kLegacyMouseShift;
    }
    if ((modifiers & SDL_KMOD_CTRL) != 0U) {
        result |= kLegacyMouseControl;
    }
    if ((buttons & SDL_BUTTON_MMASK) != 0U) {
        result |= kLegacyMouseMiddle;
    }
    if ((buttons & SDL_BUTTON_X1MASK) != 0U) {
        result |= kLegacyMouseX1;
    }
    if ((buttons & SDL_BUTTON_X2MASK) != 0U) {
        result |= kLegacyMouseX2;
    }
    return result;
}

[[nodiscard]] std::optional<app::HostWindowEvent> translate_key_release(
    const SDL_Keycode key
) {
    switch (key) {
    case SDLK_ESCAPE:
        return app::HostWindowEvent{
            app::HostWindowEventKind::key_release,
            kLegacyKeyEscape,
        };
    case SDLK_F8:
        return app::HostWindowEvent{
            app::HostWindowEventKind::key_release,
            kLegacyKeyPause,
        };
    case SDLK_P:
        return app::HostWindowEvent{
            app::HostWindowEventKind::key_release,
            kLegacyKeyScreenshot,
        };
    default:
        return std::nullopt;
    }
}

}  // namespace

std::optional<app::HostWindowEvent> translate_sdl_event(
    const SDL_Event& event
) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return app::HostWindowEvent{
            app::HostWindowEventKind::request_close,
            0U,
        };
    case SDL_EVENT_WINDOW_MINIMIZED:
        return app::HostWindowEvent{
            app::HostWindowEventKind::size,
            kLegacySizeMinimized,
        };
    case SDL_EVENT_WINDOW_MAXIMIZED:
        return app::HostWindowEvent{
            app::HostWindowEventKind::size,
            kLegacySizeMaximized,
        };
    case SDL_EVENT_WINDOW_RESTORED:
        return app::HostWindowEvent{
            app::HostWindowEventKind::size,
            kLegacySizeRestored,
        };
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        return app::HostWindowEvent{
            app::HostWindowEventKind::activation,
            0U,
        };
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        return app::HostWindowEvent{
            app::HostWindowEventKind::activation,
            1U,
        };
    case SDL_EVENT_KEY_UP:
        return translate_key_release(event.key.key);
    case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_RETURN &&
            (event.key.mod & SDL_KMOD_ALT) != 0U) {
            return app::HostWindowEvent{
                app::HostWindowEventKind::system_key_down,
                kLegacyKeyEnter,
            };
        }
        return std::nullopt;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
            return app::HostWindowEvent{
                app::HostWindowEventKind::left_button_down,
                current_legacy_mouse_button_state(),
            };
        }
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

}  // namespace openswd3::platform_sdl3
