#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::audio_video {

[[nodiscard]] compat::i32 legacy_audio_volume_parameter(
    compat::i32 value
) noexcept;

[[nodiscard]] compat::i32 legacy_audio_pan_parameter(
    compat::i32 value
) noexcept;

}  // namespace openswd3::audio_video
