#include "openswd3/audio_video/legacy_audio_output.hpp"

#include <algorithm>

namespace openswd3::audio_video {
namespace {

constexpr compat::i32 kInitialSampleRate = 44'100;
constexpr compat::i32 kMinimumSampleRate = 11'025;
constexpr compat::i32 kInitialBitsPerSample = 16;
constexpr compat::i32 kFallbackBitsPerSample = 8;
constexpr compat::i32 kChannelCount = 2;
constexpr compat::i32 kMaximumSampleHandleCount = 16;

[[nodiscard]] LegacyPcmOutputFormat make_format(
    const compat::i32 sample_rate,
    const compat::i32 bits_per_sample
) noexcept {
    const compat::i32 bytes_per_sample = (bits_per_sample + 7) / 8;
    const compat::i32 block_align = bytes_per_sample * kChannelCount;
    return LegacyPcmOutputFormat{
        1U,
        static_cast<compat::u16>(kChannelCount),
        static_cast<compat::u32>(sample_rate),
        static_cast<compat::u32>(sample_rate * block_align),
        static_cast<compat::u16>(block_align),
        static_cast<compat::u16>(bits_per_sample),
    };
}

[[nodiscard]] bool contains_emulated(
    const std::string_view configuration
) noexcept {
    return configuration.find("Emulated") != std::string_view::npos;
}

}  // namespace

LegacyAudioOutputResult initialize_legacy_audio_output(
    LegacyAudioOutputBackend& backend
) {
    LegacyAudioOutputResult result;
    compat::i32 sample_rate = kInitialSampleRate;
    compat::i32 bits_per_sample = kInitialBitsPerSample;
    backend.set_preference(kLegacyWavePreference, 0);

    while (sample_rate >= kMinimumSampleRate) {
        const LegacyPcmOutputFormat format = make_format(
            sample_rate,
            bits_per_sample
        );
        if (backend.open_output(format)) {
            const std::string_view configuration =
                backend.output_configuration();
            if (backend.preference(kLegacyWavePreference) != 0 ||
                !contains_emulated(configuration)) {
                result.status = LegacyAudioOutputStatus::ready;
                result.selected_format = format;
                result.sample_handle_count = std::min(
                    backend.preference(kLegacySampleCountPreference) - 8,
                    kMaximumSampleHandleCount
                );
                return result;
            }

            backend.close_output();
            backend.set_preference(kLegacyWavePreference, 1);
            backend.set_preference(kLegacyWavePreference, 1);
            continue;
        }

        result.last_error = backend.last_error();
        if (backend.preference(kLegacyWavePreference) == 0) {
            backend.set_preference(kLegacyWavePreference, 1);
            continue;
        }

        sample_rate /= 2;
        if (sample_rate < kMinimumSampleRate &&
            bits_per_sample == kInitialBitsPerSample) {
            sample_rate = kInitialSampleRate;
            bits_per_sample = kFallbackBitsPerSample;
        }
    }

    return result;
}

}  // namespace openswd3::audio_video
