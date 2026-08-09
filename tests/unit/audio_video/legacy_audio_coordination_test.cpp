#include "test.hpp"

#include "openswd3/audio_video/legacy_audio_coordination.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::LegacyAudioMaintenancePorts;
using openswd3::audio_video::LegacyAudioQueueCoordinator;
using openswd3::audio_video::LegacyAudioQueuePorts;
using openswd3::audio_video::LegacyAudioQueueState;
using openswd3::audio_video::LegacyQueuedAudioCommand;
using openswd3::audio_video::kLegacySequencePlaybackType;
using openswd3::audio_video::kLegacyStreamPlaybackType;
using openswd3::audio_video::maintain_legacy_audio;
using openswd3::compat::i32;

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
        return stream_is_absent;
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

    bool sequence_is_absent{true};
    bool stream_is_absent{true};
    std::vector<QueueEvent> events;
};

void test_defaults_and_sequence_queue(openswd3::test::Context& test) {
    RecordingQueuePorts ports;
    LegacyAudioQueueCoordinator coordinator(ports);
    LegacyAudioQueueState& state = coordinator.state();
    test.expect_equal(state.default_transition_ticks, 30, "default ticks");
    test.expect_equal(state.current_mode, 3, "constructor current mode");
    test.expect_equal(state.pending_mode, 3, "constructor pending mode");
    test.expect_equal(state.volume, 127, "constructor volume");
    test.expect_equal(coordinator.service(), 0, "idle service returns zero");
    test.expect_true(ports.events.empty(), "default queue has no backend calls");
    test.expect_equal(state.current_mode, 0, "pending mode three clears current");
    test.expect_equal(state.pending_mode, 0, "pending mode three consumes itself");

    state.current_playback_type = kLegacySequencePlaybackType;
    state.current_playback_id = 42;
    state.volume = 88;
    state.pending_mode = kLegacySequencePlaybackType;
    state.sequence_repeat = 1;
    state.sequence_commands[0] = LegacyQueuedAudioCommand{
        std::string{"Music\\first.xmi"},
        {1U, 2U, 3U, 4U},
    };
    state.sequence_commands[1] = LegacyQueuedAudioCommand{
        std::string{"Music\\second.xmi"},
        {5U, 6U, 7U, 8U},
    };

    test.expect_equal(coordinator.service(), 0, "sequence dispatch returns zero");
    test.expect_equal(
        ports.events,
        std::vector<QueueEvent>{
            {"sequence_absent", {}, 42},
            {"play_sequence", "Music\\first.xmi", 42, 88, 1},
        },
        "sequence absence precedes first queued play"
    );
    test.expect_equal(state.sequence_index, 1, "first slot advances index");
    test.expect_equal(
        state.current_command.opaque_fields,
        std::array<openswd3::compat::u32, 4U>{1U, 2U, 3U, 4U},
        "all five legacy record fields are copied"
    );

    ports.events.clear();
    ports.sequence_is_absent = false;
    test.expect_equal(coordinator.service(), 0, "busy sequence returns zero");
    test.expect_equal(
        ports.events,
        std::vector<QueueEvent>{{"sequence_absent", {}, 42}},
        "busy sequence blocks queue advancement"
    );
    test.expect_equal(state.sequence_index, 1, "busy sequence keeps index");

    ports.events.clear();
    ports.sequence_is_absent = true;
    static_cast<void>(coordinator.service());
    test.expect_equal(
        ports.events,
        std::vector<QueueEvent>{
            {"sequence_absent", {}, 42},
            {"play_sequence", "Music\\second.xmi", 42, 88, 1},
        },
        "second slot dispatches after completion"
    );
    test.expect_equal(state.sequence_index, 0, "repeat one wraps after slot two");
}

void test_stream_queue_and_clear(openswd3::test::Context& test) {
    RecordingQueuePorts ports;
    LegacyAudioQueueCoordinator coordinator(ports);
    LegacyAudioQueueState& state = coordinator.state();
    state.pending_mode = kLegacyStreamPlaybackType;
    state.current_playback_type = kLegacyStreamPlaybackType;
    state.current_playback_id = 100;
    state.volume = 64;
    state.stream_commands[0].filename = std::string{"Music\\queue.mp3"};

    static_cast<void>(coordinator.service());
    test.expect_equal(
        ports.events,
        std::vector<QueueEvent>{
            {"stream_absent", {}, 100},
            {"beep", {}, 0, 0, 0},
            {"play_stream", "Music\\queue.mp3", 100, 64, 1},
        },
        "stream queue preserves beep before play"
    );

    test.expect_equal(
        coordinator.clear_commands(99),
        0,
        "invalid clear selector returns zero"
    );
    test.expect_true(
        state.stream_commands[0].filename.has_value(),
        "invalid clear leaves commands intact"
    );
    static_cast<void>(coordinator.clear_commands(kLegacyStreamPlaybackType));
    test.expect_false(
        state.stream_commands[0].filename.has_value(),
        "stream clear resets both records"
    );
}

class RecordingMaintenancePorts final : public LegacyAudioMaintenancePorts {
public:
    void service_queue() override { calls.push_back("queue"); }
    void service_streams() override { calls.push_back("stream"); }
    void service_sequences() override { calls.push_back("sequence"); }
    void service_samples() override { calls.push_back("sample"); }

    std::vector<std::string> calls;
};

void test_maintenance_order(openswd3::test::Context& test) {
    RecordingMaintenancePorts ports;
    test.expect_true(maintain_legacy_audio(ports), "maintenance returns one");
    test.expect_equal(
        ports.calls,
        std::vector<std::string>{"queue", "stream", "sequence", "sample"},
        "0x0040CF10 order is exact"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_defaults_and_sequence_queue(test);
    test_stream_queue_and_clear(test);
    test_maintenance_order(test);
    return test.exit_code();
}
