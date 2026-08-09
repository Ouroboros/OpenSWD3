#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace openswd3::audio_video {

inline constexpr compat::i32 kLegacyStreamPlaybackType = 1;
inline constexpr compat::i32 kLegacySequencePlaybackType = 2;

struct LegacyQueuedAudioCommand {
    std::optional<std::string> filename;
    std::array<compat::u32, 4U> opaque_fields{};
};

struct LegacyAudioQueueState {
    compat::i32 default_transition_ticks{30};
    compat::i32 current_mode{3};
    compat::i32 pending_mode{3};
    compat::i32 sequence_index{};
    compat::i32 sequence_repeat{};
    compat::i32 stream_index{};
    compat::i32 stream_repeat{};
    compat::i32 volume{127};
    LegacyQueuedAudioCommand current_command;
    compat::i32 current_playback_id{};
    compat::i32 current_playback_type{};
    std::array<LegacyQueuedAudioCommand, 2U> sequence_commands;
    std::array<LegacyQueuedAudioCommand, 2U> stream_commands;
};

class LegacyAudioQueuePorts {
public:
    virtual ~LegacyAudioQueuePorts() = default;

    [[nodiscard]] virtual bool sequence_absent(compat::i32 sequence_id) = 0;
    [[nodiscard]] virtual bool stream_absent(compat::i32 stream_id) = 0;
    virtual void play_sequence(
        std::string_view filename,
        compat::i32 sequence_id,
        compat::i32 volume,
        compat::i32 loop_count
    ) = 0;
    virtual void play_stream(
        std::string_view filename,
        compat::i32 stream_id,
        compat::i32 volume,
        compat::i32 loop_count
    ) = 0;
    virtual void beep() = 0;
};

class LegacyAudioQueueCoordinator final {
public:
    explicit LegacyAudioQueueCoordinator(
        LegacyAudioQueuePorts& ports
    ) noexcept;
    ~LegacyAudioQueueCoordinator();

    LegacyAudioQueueCoordinator(const LegacyAudioQueueCoordinator&) = delete;
    LegacyAudioQueueCoordinator& operator=(
        const LegacyAudioQueueCoordinator&
    ) = delete;
    LegacyAudioQueueCoordinator(LegacyAudioQueueCoordinator&&) = delete;
    LegacyAudioQueueCoordinator& operator=(LegacyAudioQueueCoordinator&&) =
        delete;

    [[nodiscard]] LegacyAudioQueueState& state() noexcept;
    [[nodiscard]] const LegacyAudioQueueState& state() const noexcept;
    [[nodiscard]] compat::i32 clear_commands(compat::i32 playback_type);
    [[nodiscard]] bool shutdown();
    [[nodiscard]] compat::i32 service();

private:
    LegacyAudioQueuePorts& ports_;
    LegacyAudioQueueState state_;
};

class LegacyAudioMaintenancePorts {
public:
    virtual ~LegacyAudioMaintenancePorts() = default;

    virtual void service_queue() = 0;
    virtual void service_streams() = 0;
    virtual void service_sequences() = 0;
    virtual void service_samples() = 0;
};

[[nodiscard]] bool maintain_legacy_audio(
    LegacyAudioMaintenancePorts& ports
);

}  // namespace openswd3::audio_video
