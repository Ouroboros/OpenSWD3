#pragma once

#include "openswd3/input_time_rng/legacy_input.hpp"

namespace openswd3::platform_sdl3 {

void translate_sdl_keyboard_state(
    input_time_rng::LegacyKeyboardSnapshot& destination,
    const bool* sdl_state,
    int scancode_count
) noexcept;

[[nodiscard]] compat::i32 sample_sdl_keyboard_state(
    input_time_rng::LegacyKeyboardSnapshot& destination
) noexcept;

}  // namespace openswd3::platform_sdl3
