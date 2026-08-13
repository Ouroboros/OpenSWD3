#pragma once

#include "openswd3/audio_video/legacy_stream_manager.hpp"
#include "openswd3/compat/types.hpp"

#include <string_view>

namespace openswd3::audio_video {

struct LegacyStreamCommandState {
    compat::i32 transition_mode{};
    compat::i32 current_fade_divisor{};
    compat::i32 pending_fade_divisor{};
    compat::i32 mix_level{};
};

[[nodiscard]] compat::i32 play_legacy_stream(
    LegacyStreamManager& manager,
    std::string_view filename,
    compat::i32 stream_gate,
    compat::i32 mix_level
);

[[nodiscard]] compat::i32 stop_legacy_stream(LegacyStreamManager& manager);

[[nodiscard]] compat::i32 legacy_stream_absent(LegacyStreamManager& manager);

[[nodiscard]] compat::i32
set_legacy_stream_volume(LegacyStreamManager& manager, compat::i32 level);

[[nodiscard]] compat::i32 apply_legacy_stream_transition(
    LegacyStreamManager& manager, LegacyStreamCommandState& state
);

[[nodiscard]] compat::i32 poll_legacy_stream_transition(
    LegacyStreamManager& manager, LegacyStreamCommandState& state
);

[[nodiscard]] compat::i32 configure_legacy_stream_transition(
    LegacyStreamCommandState& state, compat::i32 mode, compat::i32 value
) noexcept;

}  // namespace openswd3::audio_video
