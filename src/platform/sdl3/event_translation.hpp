#pragma once

#include "openswd3/app/host_window_event.hpp"

#include <SDL3/SDL_events.h>

#include <optional>

namespace openswd3::platform_sdl3 {

[[nodiscard]] std::optional<app::HostWindowEvent>
translate_sdl_event(const SDL_Event& event);

}  // namespace openswd3::platform_sdl3
