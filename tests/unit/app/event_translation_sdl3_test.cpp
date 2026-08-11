#include "test.hpp"

#include "event_translation.hpp"

#include <SDL3/SDL_events.h>

namespace {

void test_focus_changes_remain_live_in_background(
    openswd3::test::Context& test
) {
    SDL_Event event{};
    event.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    test.expect_false(
        openswd3::platform_sdl3::translate_sdl_event(event).has_value(),
        "focus loss does not enter the legacy minimize path"
    );

    event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    test.expect_false(
        openswd3::platform_sdl3::translate_sdl_event(event).has_value(),
        "focus gain needs no display recovery when background execution stayed live"
    );
}

void test_explicit_window_actions_are_preserved(
    openswd3::test::Context& test
) {
    SDL_Event event{};
    event.type = SDL_EVENT_WINDOW_MINIMIZED;
    const auto minimized =
        openswd3::platform_sdl3::translate_sdl_event(event);
    test.expect_true(
        minimized.has_value() &&
            minimized->kind == openswd3::app::HostWindowEventKind::size &&
            minimized->value == 1U,
        "explicit minimization still reaches the legacy display lifecycle"
    );

    event.type = SDL_EVENT_WINDOW_MAXIMIZED;
    test.expect_false(
        openswd3::platform_sdl3::translate_sdl_event(event).has_value(),
        "maximization remains under SDL ownership"
    );

    event.type = SDL_EVENT_WINDOW_RESTORED;
    const auto restored =
        openswd3::platform_sdl3::translate_sdl_event(event);
    test.expect_true(
        restored.has_value() &&
            restored->kind == openswd3::app::HostWindowEventKind::size &&
            restored->value == 0U,
        "restore after explicit minimization reaches the legacy display lifecycle"
    );

    event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
    const auto closed = openswd3::platform_sdl3::translate_sdl_event(event);
    test.expect_true(
        closed.has_value() &&
            closed->kind == openswd3::app::HostWindowEventKind::request_close,
        "background execution does not suppress explicit close requests"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_focus_changes_remain_live_in_background(test);
    test_explicit_window_actions_are_preserved(test);
    return test.exit_code();
}
