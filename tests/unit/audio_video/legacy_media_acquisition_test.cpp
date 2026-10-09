#include "test.hpp"
#include "legacy_audio_fixture.hpp"

#include "openswd3/audio_video/legacy_media_acquisition.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace {

using openswd3::audio_video::LegacyMediaLocationStatus;
using openswd3::audio_video::begin_legacy_media_wait;
using openswd3::audio_video::cancel_legacy_media_wait;
using openswd3::audio_video::complete_legacy_media_wait;
using openswd3::audio_video::legacy_optical_media_marker_path;
using openswd3::audio_video::resolve_configured_legacy_media;
using openswd3::compat::u32;

class MediaFixture {
public:
    MediaFixture() {
        root = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("media-acquisition-" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count()
             ));
        std::filesystem::create_directories(root / "swd3");
        static_cast<void>(streams.initialize_pool(1U));
        static_cast<void>(streams.play("completed.mp3", 100, 64, 1));
        backend.completed = true;
    }

    ~MediaFixture() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

    openswd3::audio_video::LegacyAudioMaintenanceBindings audio() {
        return {queue, streams, sequences, samples};
    }

    std::filesystem::path root;
    openswd3::test::MaintenanceBackend backend;
    openswd3::audio_video::LegacySndArchive archive;
    openswd3::audio_video::LegacyStreamManager streams{backend};
    openswd3::audio_video::LegacySequenceManager sequences{backend};
    openswd3::audio_video::LegacySampleManager samples{backend, archive};
    openswd3::test::RecordingQueuePorts queue_ports;
    openswd3::audio_video::LegacyAudioQueueCoordinator queue{queue_ports};
};

void test_flag_transitions(openswd3::test::Context& test) {
    u32 flags = 0x20U;
    begin_legacy_media_wait(flags);
    test.expect_equal(flags, 0x30U, "0x0041190B sets media wait bit");
    complete_legacy_media_wait(flags);
    test.expect_equal(flags, 0x20U, "successful exit clears media wait bit");

    begin_legacy_media_wait(flags);
    cancel_legacy_media_wait(flags);
    test.expect_equal(
        flags,
        0x34U,
        "cancel preserves original stale wait bit and sets close request"
    );
}

void test_media_path_resolution(openswd3::test::Context& test) {
    for (const bool direct : {false, true}) {
        for (const bool nested : {false, true}) {
            MediaFixture fixture;
            const auto direct_marker = fixture.root / "swd3_dvd.dat";
            const auto nested_marker = fixture.root / "swd3" / "swd3_dvd.dat";
            if (direct) {
                std::ofstream{direct_marker, std::ios::binary}.put('\0');
            }

            if (nested) {
                std::ofstream{nested_marker, std::ios::binary}.put('\0');
            }

            test.expect_equal(
                legacy_optical_media_marker_path(fixture.root),
                nested_marker,
                "original optical-media layout is root/swd3/swd3_dvd.dat"
            );
            u32 flags{0x84U};
            fixture.backend.process_flags = &flags;
            const auto result = resolve_configured_legacy_media(
                fixture.root, flags, fixture.audio()
            );
            test.expect_equal(
                fixture.backend.observed_process_flags,
                std::optional<u32>{0x84U},
                "actual audio maintenance precedes setting the wait bit"
            );
            test.expect_true(
                fixture.streams.active_stream_count() == 0U &&
                    fixture.streams.free_stream_count() == 2U &&
                    fixture.queue.state().pending_mode == 0,
                "media lookup services the actual queue and reclaims completed streams"
            );
            test.expect_equal(
                flags,
                0x84U,
                "every result clears wait and preserves unrelated flags"
            );
            if (direct || nested) {
                test.expect_true(
                    result.status == LegacyMediaLocationStatus::available &&
                        result.game_directory ==
                            (direct ? fixture.root : fixture.root / "swd3") &&
                        result.marker_path ==
                            (direct ? direct_marker : nested_marker) &&
                        result.used_original_disc_layout == !direct,
                    "actual marker files resolve the configured directory before the nested layout"
                );
            } else {
                test.expect_true(
                    result.status == LegacyMediaLocationStatus::unavailable &&
                        result.game_directory.empty() &&
                        result.marker_path.empty() &&
                        !std::filesystem::exists(direct_marker) &&
                        !std::filesystem::exists(nested_marker),
                    "failed probes report unavailable without creating marker files"
                );
            }
        }
    }

    {
        MediaFixture fixture;
        std::filesystem::create_directory(fixture.root / "swd3_dvd.dat");
        const auto marker = fixture.root / "swd3" / "swd3_dvd.dat";
        std::ofstream{marker, std::ios::binary}.put('\0');
        u32 flags{};
        const auto result = resolve_configured_legacy_media(
            fixture.root, flags, fixture.audio()
        );
        test.expect_true(
            result.status == LegacyMediaLocationStatus::available &&
                result.used_original_disc_layout &&
                result.marker_path == marker && flags == 0U,
            "a directory cannot masquerade as the marker or prevent nested-file fallback"
        );
    }

    MediaFixture fixture;
    const auto regular_file = fixture.root / "not-a-directory";
    std::ofstream{regular_file, std::ios::binary}.put('\0');
    u32 flags{0x40U};
    const auto result =
        resolve_configured_legacy_media(regular_file, flags, fixture.audio());
    test.expect_true(
        result.status == LegacyMediaLocationStatus::unavailable &&
            flags == 0x40U,
        "an invalid parent path follows the normal unavailable path without throwing"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_flag_transitions(test);
    test_media_path_resolution(test);
    return test.exit_code();
}
