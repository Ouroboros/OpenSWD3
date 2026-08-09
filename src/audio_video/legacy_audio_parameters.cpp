#include "openswd3/audio_video/legacy_audio_parameters.hpp"

#include <bit>

namespace openswd3::audio_video {

compat::i32 legacy_audio_volume_parameter(const compat::i32 value) noexcept {
    if (value > 127) {
        return 127;
    }
    if (value < 0) {
        return 0;
    }
    return value;
}

compat::i32 legacy_audio_pan_parameter(const compat::i32 value) noexcept {
    const compat::u32 shifted_bits =
        static_cast<compat::u32>(value) + 63U;
    const compat::i32 shifted = std::bit_cast<compat::i32>(shifted_bits);
    return legacy_audio_volume_parameter(shifted);
}

}  // namespace openswd3::audio_video
