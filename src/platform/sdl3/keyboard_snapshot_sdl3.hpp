#pragma once

#include "openswd3/input_time_rng/legacy_input.hpp"

#include <SDL3/SDL_scancode.h>

namespace openswd3::platform_sdl3 {

void translate_sdl_keyboard_state(
    input_time_rng::LegacyKeyboardSnapshot& destination,
    const bool* sdl_state,
    int scancode_count
) noexcept;

[[nodiscard]] bool latch_sdl_keyboard_press(
    input_time_rng::LegacyKeyboardSnapshot& pending_presses,
    SDL_Scancode scancode
) noexcept;

void merge_sdl_keyboard_press_latches(
    input_time_rng::LegacyKeyboardSnapshot& destination,
    const input_time_rng::LegacyKeyboardSnapshot& pending_presses
) noexcept;

[[nodiscard]] compat::i32 sample_sdl_keyboard_state(
    input_time_rng::LegacyKeyboardSnapshot& destination
) noexcept;

}  // namespace openswd3::platform_sdl3
