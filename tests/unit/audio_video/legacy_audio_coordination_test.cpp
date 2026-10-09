#include "test.hpp"
#include "legacy_audio_fixture.hpp"

#include "openswd3/audio_video/legacy_audio_coordination.hpp"
#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/audio_video/legacy_sequence_manager.hpp"
#include "openswd3/audio_video/legacy_stream_manager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::LegacyAudioMaintenanceBindings;
using openswd3::audio_video::LegacyAudioQueueCoordinator;
using openswd3::audio_video::LegacyAudioQueuePorts;
using openswd3::audio_video::LegacyAudioQueueState;
using openswd3::audio_video::LegacyQueuedAudioCommand;
using openswd3::audio_video::kLegacySequencePlaybackType;
using openswd3::audio_video::kLegacyStreamPlaybackType;
using openswd3::audio_video::maintain_legacy_audio;
using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::compat::u8;

using openswd3::test::MaintenanceBackend;
using openswd3::test::QueueEvent;
using openswd3::test::RecordingQueuePorts;

void test_defaults_and_sequence_queue(openswd3::test::Context& test) {
    RecordingQueuePorts ports;
    LegacyAudioQueueCoordinator coordinator(ports);
    LegacyAudioQueueState& state = coordinator.state();
    test.expect_equal(state.default_transition_ticks, 30, "default ticks");
    test.expect_equal(state.current_mode, 3, "constructor current mode");
    test.expect_equal(state.pending_mode, 3, "constructor pending mode");
    test.expect_equal(state.volume, 127, "constructor volume");
    test.expect_equal(coordinator.service(), 0, "idle service returns zero");
    test.expect_true(
        ports.events.empty(), "default queue has no backend calls"
    );
    test.expect_equal(
        state.current_mode, 0, "pending mode three clears current"
    );
    test.expect_equal(
        state.pending_mode, 0, "pending mode three consumes itself"
    );

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

    test.expect_equal(
        coordinator.service(), 0, "sequence dispatch returns zero"
    );
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
    test.expect_equal(
        state.sequence_index, 0, "repeat one wraps after slot two"
    );
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
        coordinator.clear_commands(99), 0, "invalid clear selector returns zero"
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

class MaintenanceFiles {
public:
    MaintenanceFiles() {
        root = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("audio-maintenance-" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count()
             ));
        std::filesystem::create_directories(root);
        std::ofstream sequence{root / "sequence.xmi", std::ios::binary};
        sequence.put('X');
        std::ofstream archive{root / "samples.snd", std::ios::binary};
        const std::string index(0x1CU + 3000U * 0x2CU, '\0');
        archive.write(index.data(), static_cast<std::streamsize>(index.size()));
    }

    ~MaintenanceFiles() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

    std::filesystem::path root;
};

void test_maintenance_order(openswd3::test::Context& test) {
    for (const bool stream_enabled : {true, false}) {
        MaintenanceFiles files;
        MaintenanceBackend backend;
        openswd3::audio_video::LegacySndArchive archive;
        openswd3::audio_video::LegacyStreamManager streams{backend};
        openswd3::audio_video::LegacySequenceManager sequences{backend};
        openswd3::audio_video::LegacySampleManager samples{backend, archive};
        RecordingQueuePorts queue_ports;
        queue_ports.streams = &streams;
        queue_ports.maintenance_events = &backend.events;
        LegacyAudioQueueCoordinator queue{queue_ports};
        LegacyAudioMaintenanceBindings audio{
            queue, streams, sequences, samples
        };
        static_cast<void>(archive.open(files.root / "samples.snd"));
        static_cast<void>(streams.initialize_pool(1U));
        static_cast<void>(sequences.initialize_output(1U));
        static_cast<void>(samples.initialize_pool(1));
        static_cast<void>(streams.play("stream.mp3", 100, 64, 1));
        static_cast<void>(
            sequences.play((files.root / "sequence.xmi").string(), 42, 64, 1)
        );
        static_cast<void>(samples.play({
            .existing_buffer = std::vector<u8>{1U, 2U},
            .sound_id = 1U,
            .volume = 64,
            .loop_count = 1,
        }));
        test.expect_true(
            streams.active_stream_count() == 1U &&
                sequences.active_sequence_count() == 1U &&
                samples.active_sample_count() == 1U &&
                samples.live_buffer_count() == 1U,
            "maintenance starts with actual active streams, sequences and owned sample bytes"
        );
        queue.state().current_playback_type = kLegacyStreamPlaybackType;
        queue.state().current_playback_id = 100;
        backend.events.clear();
        maintain_legacy_audio(audio);
        test.expect_equal(
            backend.events,
            std::vector<std::string>{"queue", "stream", "sequence", "sample"},
            "active media is serviced in LST order"
        );
        test.expect_true(
            streams.active_stream_count() == 1U &&
                sequences.active_sequence_count() == 1U &&
                samples.active_sample_count() == 1U,
            "active media remains owned after maintenance"
        );
        backend.completed = true;
        streams.set_stream_enabled(stream_enabled);
        backend.events.clear();
        maintain_legacy_audio(audio);
        test.expect_equal(
            backend.events,
            std::vector<std::string>{"queue", "stream", "sequence", "sample"},
            "disabling new stream playback does not skip maintenance or subsequent cleanup"
        );
        test.expect_true(
            sequences.active_sequence_count() == 0U &&
                sequences.free_sequence_count() == 1U &&
                samples.active_sample_count() == 0U &&
                samples.free_sample_count() == 1U &&
                samples.live_buffer_count() == 0U &&
                archive.entry(1U)->reference_count == 0U,
            "completed media returns nodes and releases the sample buffer and archive reference"
        );
        test.expect_true(
            streams.active_stream_count() == 0U &&
                streams.free_stream_count() == 2U &&
                queue.state().pending_mode == 3,
            "queue observes the stream before the same pass releases it"
        );
        maintain_legacy_audio(audio);
        test.expect_equal(
            queue.state().pending_mode, 0, "queue advances on the next pass"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_defaults_and_sequence_queue(test);
    test_stream_queue_and_clear(test);
    test_maintenance_order(test);
    return test.exit_code();
}
