#pragma once

#include "openswd3/audio_video/legacy_audio_output.hpp"
#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/compat/types.hpp"

#include <SDL3/SDL_audio.h>

#include <array>
#include <cstddef>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openswd3::platform_sdl3 {

enum class LegacyPcmDecodeStatus {
    ready,
    invalid_riff,
    unsupported_format,
    conversion_failed,
};

struct LegacyDecodedPcm {
    LegacyPcmDecodeStatus status{LegacyPcmDecodeStatus::invalid_riff};
    std::vector<float> stereo_frames;
};

[[nodiscard]] LegacyDecodedPcm decode_legacy_riff_pcm(
    std::span<const compat::u8> bytes,
    compat::i32 target_sample_rate
);

class SdlLegacySampleBackend final
    : public audio_video::LegacyAudioOutputBackend,
      public audio_video::LegacySampleBackend {
public:
    SdlLegacySampleBackend();
    ~SdlLegacySampleBackend() override;

    SdlLegacySampleBackend(const SdlLegacySampleBackend&) = delete;
    SdlLegacySampleBackend& operator=(const SdlLegacySampleBackend&) = delete;
    SdlLegacySampleBackend(SdlLegacySampleBackend&&) = delete;
    SdlLegacySampleBackend& operator=(SdlLegacySampleBackend&&) = delete;

    void set_preference(compat::i32 index, compat::i32 value) override;
    [[nodiscard]] compat::i32 preference(compat::i32 index) override;
    [[nodiscard]] bool open_output(
        const audio_video::LegacyPcmOutputFormat& format
    ) override;
    [[nodiscard]] std::string_view output_configuration() override;
    [[nodiscard]] std::string_view last_error() const override;

    [[nodiscard]] compat::u32 driver_token() const override;
    [[nodiscard]] audio_video::LegacySampleHandle
    allocate_sample_handle() override;
    void initialize_sample(audio_video::LegacySampleHandle handle) override;
    void release_sample_handle(
        audio_video::LegacySampleHandle handle
    ) override;
    [[nodiscard]] bool set_sample_file(
        audio_video::LegacySampleHandle handle,
        std::span<const compat::u8> bytes
    ) override;
    [[nodiscard]] bool set_named_sample_file(
        audio_video::LegacySampleHandle handle,
        std::string_view extension,
        std::span<const compat::u8> bytes,
        compat::u32 auxiliary
    ) override;
    void set_sample_user_data(
        audio_video::LegacySampleHandle handle,
        compat::u32 slot,
        compat::u32 value
    ) override;
    [[nodiscard]] compat::u32 sample_user_data(
        audio_video::LegacySampleHandle handle,
        compat::u32 slot
    ) override;
    void set_sample_volume(
        audio_video::LegacySampleHandle handle,
        compat::i32 volume
    ) override;
    void set_sample_pan(
        audio_video::LegacySampleHandle handle,
        compat::i32 pan
    ) override;
    void set_sample_loop_count(
        audio_video::LegacySampleHandle handle,
        compat::i32 loop_count
    ) override;
    void start_sample(audio_video::LegacySampleHandle handle) override;
    void end_sample(audio_video::LegacySampleHandle handle) override;
    [[nodiscard]] compat::u32 sample_status(
        audio_video::LegacySampleHandle handle
    ) override;
    void close_output() override;

private:
    static constexpr std::size_t kMaximumSamples = 16U;
    static constexpr compat::u32 kStatusDone = 2U;
    static constexpr compat::u32 kStatusPlaying = 4U;

    struct Sample {
        std::vector<float> stereo_frames;
        std::size_t frame_cursor{};
        compat::u32 user_data{};
        compat::u32 status{kStatusDone};
        compat::i32 volume{127};
        compat::i32 pan{63};
        compat::i32 loop_count{1};
        compat::i32 loops_remaining{1};
        bool allocated{};
        bool configured{};
        bool playing{};
    };

    [[nodiscard]] Sample* sample(
        audio_video::LegacySampleHandle handle
    ) noexcept;
    [[nodiscard]] const Sample* sample(
        audio_video::LegacySampleHandle handle
    ) const noexcept;
    void reset_sample(Sample& sample, bool allocated);
    void set_sdl_error(std::string_view operation);
    static void SDLCALL audio_callback(
        void* userdata,
        SDL_AudioStream* stream,
        int additional_amount,
        int total_amount
    );
    void mix_and_queue(SDL_AudioStream& stream, int additional_amount);

    mutable std::mutex mutex_;
    std::array<compat::i32, 16U> preferences_{};
    std::array<Sample, kMaximumSamples> samples_{};
    SDL_AudioDeviceID device_{};
    SDL_AudioStream* mixer_stream_{};
    compat::i32 output_sample_rate_{};
    std::string configuration_;
    std::string last_error_;
};

}  // namespace openswd3::platform_sdl3
