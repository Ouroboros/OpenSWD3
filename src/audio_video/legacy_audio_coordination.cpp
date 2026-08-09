#include "openswd3/audio_video/legacy_audio_coordination.hpp"

namespace openswd3::audio_video {

LegacyAudioQueueCoordinator::LegacyAudioQueueCoordinator(
    LegacyAudioQueuePorts& ports
) noexcept : ports_(ports) {}

LegacyAudioQueueCoordinator::~LegacyAudioQueueCoordinator() {
    static_cast<void>(shutdown());
}

LegacyAudioQueueState& LegacyAudioQueueCoordinator::state() noexcept {
    return state_;
}

const LegacyAudioQueueState& LegacyAudioQueueCoordinator::state() const
    noexcept {
    return state_;
}

compat::i32 LegacyAudioQueueCoordinator::clear_commands(
    const compat::i32 playback_type
) {
    auto* commands = playback_type == kLegacySequencePlaybackType
        ? &state_.sequence_commands
        : playback_type == kLegacyStreamPlaybackType
            ? &state_.stream_commands
            : nullptr;
    if (commands == nullptr) {
        return 0;
    }
    for (LegacyQueuedAudioCommand& command : *commands) {
        command = LegacyQueuedAudioCommand{};
    }
    return 0;
}

bool LegacyAudioQueueCoordinator::shutdown() {
    static_cast<void>(clear_commands(kLegacySequencePlaybackType));
    static_cast<void>(clear_commands(kLegacyStreamPlaybackType));
    return true;
}

compat::i32 LegacyAudioQueueCoordinator::service() {
    if (state_.current_playback_type == kLegacySequencePlaybackType) {
        if (!ports_.sequence_absent(state_.current_playback_id)) {
            return 0;
        }
    } else if (state_.current_playback_type == kLegacyStreamPlaybackType) {
        if (!ports_.stream_absent(state_.current_playback_id)) {
            return 0;
        }
    }

    state_.current_command = LegacyQueuedAudioCommand{};
    if (state_.pending_mode > 0) {
        if (state_.pending_mode <= kLegacySequencePlaybackType) {
            state_.current_mode = state_.pending_mode;
            state_.pending_mode = 0;
            if (state_.current_mode == kLegacyStreamPlaybackType) {
                state_.stream_index = 0;
            } else if (
                state_.current_mode == kLegacySequencePlaybackType
            ) {
                state_.sequence_index = 0;
            }
        } else if (state_.pending_mode == 3) {
            state_.current_mode = 0;
            state_.pending_mode = 0;
        }
    }

    std::array<LegacyQueuedAudioCommand, 2U>* commands{};
    compat::i32* index{};
    compat::i32 repeat{};
    if (state_.current_mode == kLegacySequencePlaybackType) {
        commands = &state_.sequence_commands;
        index = &state_.sequence_index;
        repeat = state_.sequence_repeat;
    } else if (state_.current_mode == kLegacyStreamPlaybackType) {
        commands = &state_.stream_commands;
        index = &state_.stream_index;
        repeat = state_.stream_repeat;
    } else {
        return 0;
    }

    if (*index >= static_cast<compat::i32>(commands->size())) {
        return 0;
    }
    const LegacyQueuedAudioCommand& source = (*commands)[
        static_cast<std::size_t>(*index)
    ];
    state_.current_command = source;

    if (state_.current_command.filename.has_value()) {
        const std::string_view filename = *state_.current_command.filename;
        if (state_.current_playback_type ==
            kLegacySequencePlaybackType) {
            ports_.play_sequence(
                filename,
                state_.current_playback_id,
                state_.volume,
                1
            );
        } else if (
            state_.current_playback_type == kLegacyStreamPlaybackType
        ) {
            ports_.beep();
            ports_.play_stream(
                filename,
                state_.current_playback_id,
                state_.volume,
                1
            );
        }
    }

    ++(*index);
    if (repeat == 1 && *index >= 2) {
        *index = 0;
    }
    return 0;
}

bool maintain_legacy_audio(LegacyAudioMaintenancePorts& ports) {
    ports.service_queue();
    ports.service_streams();
    ports.service_sequences();
    ports.service_samples();
    return true;
}

}  // namespace openswd3::audio_video
