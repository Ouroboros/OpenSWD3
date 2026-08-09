#include "openswd3/audio_video/legacy_sample_commands.hpp"

#include <bit>
#include <cmath>
#include <optional>

namespace openswd3::audio_video {
namespace {

[[nodiscard]] constexpr compat::i32 from_bits(
    const compat::u32 value
) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::i32 wrapping_subtract(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return from_bits(
        static_cast<compat::u32>(left) - static_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32 wrapping_multiply(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return from_bits(
        static_cast<compat::u32>(left) * static_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32 wrapping_shift_left(
    const compat::i32 value,
    const compat::u32 count
) noexcept {
    return from_bits(static_cast<compat::u32>(value) << count);
}

[[nodiscard]] constexpr compat::u32 low_word(
    const compat::u32 value
) noexcept {
    return value & 0xFFFFU;
}

// 0x00485610/0x00485670 use the signed high half of a multiply by
// 0x2E8BA2E9 followed by one arithmetic shift. That sequence is signed
// truncating division by 11 after the 32-bit left shift has wrapped.
[[nodiscard]] constexpr compat::i32 scale_level_by_128_over_11(
    const compat::i32 level
) noexcept {
    return wrapping_shift_left(level, 7U) / 11;
}

[[nodiscard]] compat::i32 spatial_distance(
    const compat::i32 listener_x,
    const compat::i32 listener_y,
    const compat::i32 target_x,
    const compat::i32 target_y
) noexcept {
    const compat::i32 delta_y = wrapping_subtract(listener_y, target_y);
    const compat::i32 delta_x = wrapping_subtract(listener_x, target_x);
    const compat::i32 squared_distance = from_bits(
        static_cast<compat::u32>(wrapping_multiply(delta_y, delta_y)) +
        static_cast<compat::u32>(wrapping_multiply(delta_x, delta_x))
    );

    // A negative wrapped sum feeds fsqrt a negative integer in the original.
    // The masked x87 invalid operation reaches __ftol as NaN and leaves zero
    // in EAX (the low half of the 64-bit integer-indefinite result).
    if (squared_distance < 0) {
        return 0;
    }
    return static_cast<compat::i32>(
        std::sqrt(static_cast<double>(squared_distance))
    );
}

[[nodiscard]] LegacySamplePlayRequest make_play_request(
    const compat::u32 sound_id,
    const compat::i32 volume
) {
    return LegacySamplePlayRequest{
        .existing_buffer = std::nullopt,
        .sound_id = sound_id,
        .volume = volume,
        .pan = 0,
        .loop_count = 1,
        .named_file_auxiliary = 0U,
    };
}

}  // namespace

compat::i32 play_legacy_sample(
    LegacySampleManager& manager,
    const compat::u32 raw_sound_id,
    const compat::i32 level
) {
    return manager.play(make_play_request(
        low_word(raw_sound_id),
        scale_level_by_128_over_11(level)
    ));
}

compat::i32 set_legacy_sample_pan(
    LegacySampleManager& manager,
    const compat::u32 raw_sound_id,
    const compat::i32 pan
) {
    return manager.set_pan(low_word(raw_sound_id), pan);
}

compat::i32 play_legacy_sample_u16_level(
    LegacySampleManager& manager,
    const compat::u32 raw_sound_id,
    const compat::u32 raw_level
) {
    static_cast<void>(manager.play(make_play_request(
        low_word(raw_sound_id),
        scale_level_by_128_over_11(
            static_cast<compat::i32>(low_word(raw_level))
        )
    )));
    return 1;
}

compat::i32 stop_legacy_sample(
    LegacySampleManager& manager,
    const compat::u32 raw_sound_id
) {
    static_cast<void>(manager.stop(low_word(raw_sound_id)));
    return 1;
}

compat::i32 stop_all_legacy_samples(LegacySampleManager& manager) {
    static_cast<void>(manager.stop_all());
    return 1;
}

compat::i32 play_legacy_spatial_sample(
    LegacySampleManager& manager,
    const compat::u32 sound_id,
    const compat::i32 target_x,
    const compat::i32 target_y,
    const LegacySpatialSampleState& state
) {
    const compat::i32 distance = spatial_distance(
        state.listener_x,
        state.listener_y,
        target_x,
        target_y
    );
    if (distance >= 0x200) {
        return distance;
    }

    static_cast<void>(manager.play(make_play_request(sound_id, 0)));

    const compat::i32 distance_attenuation =
        wrapping_shift_left(distance, 7U) / 0x200;
    const compat::i32 remaining_level = wrapping_subtract(
        0x80,
        distance_attenuation
    );
    const compat::i32 scaled_volume =
        wrapping_multiply(remaining_level, state.mix_level) / 11;
    static_cast<void>(manager.set_volume(sound_id, scaled_volume));

    const compat::i32 horizontal_delta = wrapping_subtract(
        target_x,
        state.listener_x
    );
    const compat::i32 pan =
        wrapping_shift_left(horizontal_delta, 6U) / 0x200;
    return manager.set_pan(sound_id, pan);
}

}  // namespace openswd3::audio_video
