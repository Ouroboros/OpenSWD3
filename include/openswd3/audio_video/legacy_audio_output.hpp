#pragma once

#include "openswd3/compat/types.hpp"

#include <string>
#include <string_view>

namespace openswd3::audio_video {

inline constexpr compat::i32 kLegacyWavePreference = 15;
inline constexpr compat::i32 kLegacySampleCountPreference = 1;

struct LegacyPcmOutputFormat {
    compat::u16 format_tag{1U};
    compat::u16 channels{};
    compat::u32 sample_rate{};
    compat::u32 average_bytes_per_second{};
    compat::u16 block_align{};
    compat::u16 bits_per_sample{};

    friend bool operator==(
        const LegacyPcmOutputFormat&,
        const LegacyPcmOutputFormat&
    ) = default;
};

static_assert(sizeof(LegacyPcmOutputFormat) == 16U);

class LegacyAudioOutputBackend {
public:
    virtual ~LegacyAudioOutputBackend() = default;

    virtual void set_preference(compat::i32 index, compat::i32 value) = 0;
    [[nodiscard]] virtual compat::i32 preference(compat::i32 index) = 0;
    [[nodiscard]] virtual bool open_output(
        const LegacyPcmOutputFormat& format
    ) = 0;
    [[nodiscard]] virtual std::string_view output_configuration() = 0;
    [[nodiscard]] virtual std::string_view last_error() const = 0;
    virtual void close_output() = 0;
};

enum class LegacyAudioOutputStatus {
    ready,
    output_open_failed,
};

struct LegacyAudioOutputResult {
    LegacyAudioOutputStatus status{LegacyAudioOutputStatus::output_open_failed};
    LegacyPcmOutputFormat selected_format{};
    compat::i32 sample_handle_count{};
    std::string last_error;
};

[[nodiscard]] LegacyAudioOutputResult initialize_legacy_audio_output(
    LegacyAudioOutputBackend& backend
);

}  // namespace openswd3::audio_video
