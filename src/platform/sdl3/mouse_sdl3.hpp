#pragma once

#include "openswd3/input_time_rng/legacy_input.hpp"

#include <SDL3/SDL_render.h>

namespace openswd3::platform_sdl3 {

struct SdlMouseDeviceState {
    double absolute_x{};
    double absolute_y{};
};

[[nodiscard]] input_time_rng::LegacyMouseDeviceSample
accumulate_sdl_mouse_sample(
    SdlMouseDeviceState& state,
    double logical_delta_x,
    double logical_delta_y,
    compat::u32 sdl_button_mask
) noexcept;

[[nodiscard]] input_time_rng::LegacyMouseDeviceSample sample_sdl_mouse_state(
    SDL_Renderer& renderer,
    SdlMouseDeviceState& state
) noexcept;

}  // namespace openswd3::platform_sdl3
