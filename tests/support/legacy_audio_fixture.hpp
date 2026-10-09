#pragma once

#include "openswd3/audio_video/legacy_audio_coordination.hpp"
#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/audio_video/legacy_sequence_manager.hpp"
#include "openswd3/audio_video/legacy_stream_manager.hpp"

#include <optional>
#include <string>
#include <vector>

namespace openswd3::test {

using audio_video::LegacyAudioQueuePorts;
using compat::i32;
using compat::u32;
using compat::u8;

struct QueueEvent {
    std::string call;
    std::string filename;
    i32 id{};
    i32 volume{};
    i32 loop_count{};

    bool operator==(const QueueEvent&) const = default;
};

class RecordingQueuePorts final : public LegacyAudioQueuePorts {
public:
    bool sequence_absent(const i32 sequence_id) override {
        events.push_back({"sequence_absent", {}, sequence_id});
        return sequence_is_absent;
    }

    bool stream_absent(const i32 stream_id) override {
        events.push_back({"stream_absent", {}, stream_id});
        if (maintenance_events) {
            maintenance_events->push_back("queue");
        }

        return streams ? streams->stream_absent(stream_id) : stream_is_absent;
    }

    void play_sequence(
        const std::string_view filename,
        const i32 sequence_id,
        const i32 volume,
        const i32 loop_count
    ) override {
        events.push_back({
            "play_sequence",
            std::string{filename},
            sequence_id,
            volume,
            loop_count,
        });
    }

    void play_stream(
        const std::string_view filename,
        const i32 stream_id,
        const i32 volume,
        const i32 loop_count
    ) override {
        events.push_back({
            "play_stream",
            std::string{filename},
            stream_id,
            volume,
            loop_count,
        });
    }

    void beep() override {
        events.push_back({"beep", {}, 0, 0, 0});
    }

    openswd3::audio_video::LegacyStreamManager* streams{};
    std::vector<std::string>* maintenance_events{};
    bool sequence_is_absent{true};
    bool stream_is_absent{true};
    std::vector<QueueEvent> events;
};

class MaintenanceBackend final
    : public openswd3::audio_video::LegacyStreamBackend,
      public openswd3::audio_video::LegacySequenceBackend,
      public openswd3::audio_video::LegacySampleBackend {
public:
    std::string_view last_error() const override {
        return {};
    }

    u32 open_stream(u32, std::string_view, i32) override {
        return 1U;
    }

    void close_stream(u32) override {}
    void set_stream_user_data(u32, u32, i32 value) override {
        stream_id = value;
    }

    i32 stream_user_data(u32, u32) override {
        return stream_id;
    }

    void set_stream_volume(u32, i32 value) override {
        volume = value;
    }

    i32 stream_volume(u32) override {
        return volume;
    }

    void set_stream_loop_count(u32, i32) override {}
    void start_stream(u32) override {}
    u32 stream_status(u32) override {
        if (process_flags) {
            observed_process_flags = *process_flags;
        }

        events.push_back("stream");
        return completed ? 2U : 4U;
    }

    void stream_ms_position(u32, i32& total, i32& current) override {
        total = 100;
        current = completed ? 100 : 0;
    }

    bool open_midi_output(i32, u32& driver) override {
        driver = 1U;
        return true;
    }

    void close_midi_output(u32) override {}
    u32 allocate_sequence_handle(u32) override {
        return 1U;
    }

    void release_sequence_handle(u32) override {}
    i32 initialize_sequence(u32, std::span<const u8> bytes, u32) override {
        return bytes.empty() ? 0 : 1;
    }

    void set_sequence_user_data(u32, u32, i32 value) override {
        sequence_id = value;
    }

    i32 sequence_user_data(u32, u32) override {
        return sequence_id;
    }

    void set_sequence_volume(u32, i32, i32) override {}
    void set_sequence_loop_count(u32, i32) override {}
    void start_sequence(u32) override {}
    u32 sequence_status(u32) override {
        events.push_back("sequence");
        return completed ? 2U : 4U;
    }

    void end_sequence(u32) override {}
    u32 driver_token() const override {
        return 1U;
    }

    u32 allocate_sample_handle() override {
        return 1U;
    }

    void initialize_sample(u32) override {}
    void release_sample_handle(u32) override {}
    bool set_sample_file(u32, std::span<const u8> bytes) override {
        return !bytes.empty();
    }

    bool set_named_sample_file(
        u32, std::string_view, std::span<const u8> bytes, u32
    ) override {
        return !bytes.empty();
    }

    void set_sample_user_data(u32, u32, u32 value) override {
        sample_id = value;
    }

    u32 sample_user_data(u32, u32) override {
        return sample_id;
    }

    void set_sample_volume(u32, i32) override {}
    void set_sample_pan(u32, i32) override {}
    void set_sample_loop_count(u32, i32) override {}
    void start_sample(u32) override {}
    void end_sample(u32) override {}
    u32 sample_status(u32) override {
        events.push_back("sample");
        return completed ? 2U : 4U;
    }

    void close_output() override {}

    std::vector<std::string> events;
    i32 stream_id{};
    i32 sequence_id{};
    u32 sample_id{};
    i32 volume{};
    bool completed{};
    const u32* process_flags{};
    std::optional<u32> observed_process_flags;
};

}
