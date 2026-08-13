#include "openswd3/audio_video/legacy_stream_commands.hpp"

#include <bit>

namespace openswd3::audio_video {
namespace {

constexpr compat::i32 kMusicStreamId = 100;

[[nodiscard]] constexpr compat::i32
from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(
        static_cast<compat::u32>(left) - static_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32
scale_level_by_128_over_11(const compat::i32 level) noexcept {
    return from_bits(static_cast<compat::u32>(level) << 7U) / 11;
}

}  // namespace

compat::i32 play_legacy_stream(
    LegacyStreamManager& manager,
    const std::string_view filename,
    const compat::i32 stream_gate,
    const compat::i32 mix_level
) {
    if (stream_gate == 0) {
        return 0;
    }

    static_cast<void>(manager.play(
        filename, kMusicStreamId, scale_level_by_128_over_11(mix_level), 1
    ));
    return 1;
}

compat::i32 stop_legacy_stream(LegacyStreamManager& manager) {
    return manager.begin_fade(kMusicStreamId, 1);
}

compat::i32 legacy_stream_absent(LegacyStreamManager& manager) {
    return manager.stream_absent(kMusicStreamId) ? 1 : 0;
}

compat::i32 set_legacy_stream_volume(
    LegacyStreamManager& manager, const compat::i32 level
) {
    return manager.set_volume(
        kMusicStreamId, scale_level_by_128_over_11(level)
    );
}

compat::i32 apply_legacy_stream_transition(
    LegacyStreamManager& manager, LegacyStreamCommandState& state
) {
    compat::i32 result = wrapping_subtract(state.transition_mode, 1);
    if (result == 0) {
        static_cast<void>(manager.begin_fade(kMusicStreamId, 1));
        state.transition_mode = 0;
        state.current_fade_divisor = 0;
        return 0;
    }

    result = wrapping_subtract(result, 1);
    if (result != 0) {
        return result;
    }

    result = manager.begin_fade(kMusicStreamId, state.pending_fade_divisor);
    state.current_fade_divisor = state.pending_fade_divisor;
    return result;
}

compat::i32 poll_legacy_stream_transition(
    LegacyStreamManager& manager, LegacyStreamCommandState& state
) {
    compat::i32 result = state.transition_mode;
    if (result == 0) {
        return 0;
    }

    result = wrapping_subtract(result, 2);
    if (result != 0) {
        return result;
    }

    result = manager.stream_absent(kMusicStreamId) ? 1 : 0;
    if (result == 1) {
        state.transition_mode = 0;
        state.current_fade_divisor = 0;
    }
    return result;
}

compat::i32 configure_legacy_stream_transition(
    LegacyStreamCommandState& state,
    const compat::i32 mode,
    const compat::i32 value
) noexcept {
    state.transition_mode = mode;
    if (mode == 2) {
        state.pending_fade_divisor = value;
        return value;
    }
    return mode;
}

}  // namespace openswd3::audio_video
