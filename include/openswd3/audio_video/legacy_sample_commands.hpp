#pragma once

#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::audio_video {

struct LegacySpatialSampleState {
    compat::i32 listener_x{};
    compat::i32 listener_y{};
    compat::i32 mix_level{};
};

[[nodiscard]] compat::i32 play_legacy_sample(
    LegacySampleManager& manager, compat::u32 raw_sound_id, compat::i32 level
);

[[nodiscard]] compat::i32 set_legacy_sample_pan(
    LegacySampleManager& manager, compat::u32 raw_sound_id, compat::i32 pan
);

[[nodiscard]] compat::i32 play_legacy_sample_u16_level(
    LegacySampleManager& manager,
    compat::u32 raw_sound_id,
    compat::u32 raw_level
);

[[nodiscard]] compat::i32
stop_legacy_sample(LegacySampleManager& manager, compat::u32 raw_sound_id);

[[nodiscard]] compat::i32 stop_all_legacy_samples(LegacySampleManager& manager);

[[nodiscard]] compat::i32 play_legacy_spatial_sample(
    LegacySampleManager& manager,
    compat::u32 sound_id,
    compat::i32 target_x,
    compat::i32 target_y,
    const LegacySpatialSampleState& state
);

}  // namespace openswd3::audio_video
