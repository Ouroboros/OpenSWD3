#include "test.hpp"

#include "openswd3/audio_video/legacy_media_acquisition.hpp"

#include <filesystem>
#include <vector>

namespace {

using openswd3::audio_video::LegacyMediaAcquisitionPorts;
using openswd3::audio_video::LegacyMediaLocationStatus;
using openswd3::audio_video::begin_legacy_media_wait;
using openswd3::audio_video::cancel_legacy_media_wait;
using openswd3::audio_video::complete_legacy_media_wait;
using openswd3::audio_video::legacy_optical_media_marker_path;
using openswd3::audio_video::resolve_configured_legacy_media;
using openswd3::compat::u32;

class RecordingPorts final : public LegacyMediaAcquisitionPorts {
public:
    void service_audio() override {
        ++service_count;
    }

    bool file_exists(const std::filesystem::path& path) override {
        probes.push_back(path);
        return path == available_path;
    }

    std::filesystem::path available_path;
    std::vector<std::filesystem::path> probes;
    std::size_t service_count{};
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
    const std::filesystem::path root{"/media/disc"};
    test.expect_equal(
        legacy_optical_media_marker_path(root),
        root / "swd3" / "swd3_dvd.dat",
        "original optical-media layout is root/swd3/swd3_dvd.dat"
    );

    {
        RecordingPorts ports;
        ports.available_path = root / "swd3_dvd.dat";
        u32 flags{};
        const auto result = resolve_configured_legacy_media(root, flags, ports);

        test.expect_equal(
            result.status,
            LegacyMediaLocationStatus::available,
            "configured game directory is accepted directly"
        );
        test.expect_equal(result.game_directory, root, "direct game root");
        test.expect_false(
            result.used_original_disc_layout,
            "direct directory is the modern compatibility path"
        );
        test.expect_equal(flags, 0U, "direct success clears wait state");
        test.expect_equal(ports.service_count, std::size_t{1U}, "audio served");
        test.expect_equal(
            ports.probes,
            std::vector<std::filesystem::path>{root / "swd3_dvd.dat"},
            "direct marker is checked first"
        );
    }

    {
        RecordingPorts ports;
        ports.available_path = root / "swd3" / "swd3_dvd.dat";
        u32 flags{0x80U};
        const auto result = resolve_configured_legacy_media(root, flags, ports);

        test.expect_equal(
            result.status,
            LegacyMediaLocationStatus::available,
            "original disc directory remains accepted"
        );
        test.expect_equal(
            result.game_directory,
            root / "swd3",
            "disc root resolves to its nested game directory"
        );
        test.expect_true(
            result.used_original_disc_layout,
            "result records original disc layout"
        );
        test.expect_equal(flags, 0x80U, "unrelated flags are preserved");
        test.expect_equal(
            ports.probes,
            std::vector<std::filesystem::path>{
                root / "swd3_dvd.dat",
                root / "swd3" / "swd3_dvd.dat",
            },
            "legacy nested marker follows the configured-directory probe"
        );
    }

    {
        RecordingPorts ports;
        u32 flags{0x40U};
        const auto result = resolve_configured_legacy_media(root, flags, ports);

        test.expect_equal(
            result.status,
            LegacyMediaLocationStatus::unavailable,
            "missing marker is explicit instead of entering an optical loop"
        );
        test.expect_equal(flags, 0x40U, "unavailable result clears wait bit");
        test.expect_equal(ports.probes.size(), std::size_t{2U}, "two layouts");
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_flag_transitions(test);
    test_media_path_resolution(test);
    return test.exit_code();
}
