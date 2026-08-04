#include "mouse_sdl3.hpp"

#include <SDL3/SDL_mouse.h>

#include <bit>
#include <cstdint>

namespace openswd3::platform_sdl3 {

namespace {

[[nodiscard]] compat::i32 legacy_axis(const double value) noexcept {
    const auto truncated = static_cast<std::int64_t>(value);
    return std::bit_cast<compat::i32>(static_cast<compat::u32>(truncated));
}

}  // namespace

input_time_rng::LegacyMouseDeviceSample accumulate_sdl_mouse_sample(
    SdlMouseDeviceState& state,
    const double logical_delta_x,
    const double logical_delta_y,
    const compat::u32 sdl_button_mask
) noexcept {
    state.absolute_x += logical_delta_x;
    state.absolute_y += logical_delta_y;
    return {
        legacy_axis(state.absolute_x),
        legacy_axis(state.absolute_y),
        static_cast<compat::u8>(
            (sdl_button_mask & SDL_BUTTON_LMASK) != 0U ? 0x80U : 0U
        ),
        static_cast<compat::u8>(
            (sdl_button_mask & SDL_BUTTON_RMASK) != 0U ? 0x80U : 0U
        ),
    };
}

input_time_rng::LegacyMouseDeviceSample sample_sdl_mouse_state(
    SDL_Renderer& renderer,
    SdlMouseDeviceState& state
) noexcept {
    float window_delta_x{};
    float window_delta_y{};
    const compat::u32 buttons = SDL_GetRelativeMouseState(
        &window_delta_x,
        &window_delta_y
    );

    float origin_x{};
    float origin_y{};
    float endpoint_x{};
    float endpoint_y{};
    double logical_delta_x = window_delta_x;
    double logical_delta_y = window_delta_y;
    if (SDL_RenderCoordinatesFromWindow(
            &renderer,
            0.0F,
            0.0F,
            &origin_x,
            &origin_y
        ) &&
        SDL_RenderCoordinatesFromWindow(
            &renderer,
            window_delta_x,
            window_delta_y,
            &endpoint_x,
            &endpoint_y
        )) {
        logical_delta_x = static_cast<double>(endpoint_x - origin_x);
        logical_delta_y = static_cast<double>(endpoint_y - origin_y);
    }

    return accumulate_sdl_mouse_sample(
        state,
        logical_delta_x,
        logical_delta_y,
        buttons
    );
}

}  // namespace openswd3::platform_sdl3
