#include "test.hpp"

#include "openswd3/audio_video/legacy_sample_commands.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::audio_video::LegacySampleBackend;
using openswd3::audio_video::LegacySampleHandle;
using openswd3::audio_video::LegacySampleManager;
using openswd3::audio_video::LegacySampleManagerInitializeStatus;
using openswd3::audio_video::LegacySndArchive;
using openswd3::audio_video::LegacySndOpenStatus;
using openswd3::audio_video::LegacySpatialSampleState;
using openswd3::audio_video::play_legacy_sample;
using openswd3::audio_video::play_legacy_sample_u16_level;
using openswd3::audio_video::play_legacy_spatial_sample;
using openswd3::audio_video::set_legacy_sample_pan;
using openswd3::audio_video::stop_all_legacy_samples;
using openswd3::audio_video::stop_legacy_sample;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u32;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kDiskRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kPayloadOffset =
    kIndexOffset + kSlotCount * kDiskRecordSize;
constexpr std::size_t kPayloadSize = 48U;
constexpr u32 kSyntheticSoundCount = 8U;

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> synthetic_archive() {
    std::vector<u8> bytes(
        kPayloadOffset + kSyntheticSoundCount * kPayloadSize, 0U
    );
    for (u32 sound_id = 1U; sound_id <= kSyntheticSoundCount; ++sound_id) {
        const std::size_t record =
            kIndexOffset + (sound_id - 1U) * kDiskRecordSize;
        const u32 payload =
            static_cast<u32>(kPayloadOffset + (sound_id - 1U) * kPayloadSize);
        write_u32(bytes, record + 0x14U, static_cast<u32>(kPayloadSize));
        write_u32(bytes, record + 0x18U, payload);
        write_u32(bytes, record + 0x20U, 0U);
        for (std::size_t index = 0U; index < kPayloadSize; ++index) {
            bytes[payload + index] = static_cast<u8>(sound_id + index);
        }
    }
    return bytes;
}

class TestTree {
public:
    TestTree() {
        const auto unique =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-sample-commands-" + std::to_string(unique));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path write_archive() const {
        const std::filesystem::path path = root_ / "all.snd";
        const std::vector<u8> bytes = synthetic_archive();
        std::ofstream output{path, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
        return path;
    }

private:
    std::filesystem::path root_;
};

class RecordingBackend final : public LegacySampleBackend {
public:
    [[nodiscard]] u32 driver_token() const override {
        return 1U;
    }

    [[nodiscard]] LegacySampleHandle allocate_sample_handle() override {
        return ++allocated_count;
    }

    void initialize_sample(LegacySampleHandle) override {}
    void release_sample_handle(LegacySampleHandle) override {}

    [[nodiscard]] bool
    set_sample_file(LegacySampleHandle, std::span<const u8>) override {
        return true;
    }

    [[nodiscard]] bool set_named_sample_file(
        LegacySampleHandle, std::string_view, std::span<const u8>, u32
    ) override {
        return true;
    }

    void set_sample_user_data(
        const LegacySampleHandle handle, u32, const u32 value
    ) override {
        user_data[handle] = value;
        last_sound_id = value;
    }

    [[nodiscard]] u32
    sample_user_data(const LegacySampleHandle handle, u32) override {
        return user_data[handle];
    }

    void set_sample_volume(LegacySampleHandle, const i32 value) override {
        volumes.push_back(value);
    }

    void set_sample_pan(LegacySampleHandle, const i32 value) override {
        pans.push_back(value);
    }

    void set_sample_loop_count(LegacySampleHandle, const i32 value) override {
        last_loop_count = value;
    }

    void start_sample(LegacySampleHandle) override {
        ++start_count;
    }
    void end_sample(LegacySampleHandle) override {
        ++end_count;
    }

    [[nodiscard]] u32 sample_status(LegacySampleHandle) override {
        return 4U;
    }

    void close_output() override {}

    u32 allocated_count{};
    std::array<u32, 17U> user_data{};
    u32 last_sound_id{};
    i32 last_loop_count{};
    u32 start_count{};
    u32 end_count{};
    std::vector<i32> volumes;
    std::vector<i32> pans;
};

struct Fixture {
    Fixture() : manager(backend, archive) {
        const std::filesystem::path path = tree.write_archive();
        open_status = archive.open(path);
        initialize_status = manager.initialize_pool(8);
    }

    TestTree tree;
    RecordingBackend backend;
    LegacySndArchive archive;
    LegacySampleManager manager;
    LegacySndOpenStatus open_status{};
    LegacySampleManagerInitializeStatus initialize_status{};
};

void expect_ready(openswd3::test::Context& test, const Fixture& fixture) {
    test.expect_equal(
        fixture.open_status,
        LegacySndOpenStatus::ready,
        "synthetic SND archive opens"
    );
    test.expect_equal(
        fixture.initialize_status,
        LegacySampleManagerInitializeStatus::ready,
        "sample manager initializes"
    );
}

void test_play_and_pan_wrappers(openswd3::test::Context& test) {
    Fixture fixture;
    expect_ready(test, fixture);

    test.expect_equal(
        play_legacy_sample(fixture.manager, 0x00010001U, 5),
        0,
        "0x00485610 returns the manager play result"
    );
    test.expect_equal(fixture.backend.last_sound_id, 1U, "sound ID is u16");
    test.expect_equal(fixture.backend.volumes.back(), 58, "5*128/11");
    test.expect_equal(fixture.backend.pans.back(), 63, "play starts centered");
    test.expect_equal(fixture.backend.last_loop_count, 1, "play loop count");

    test.expect_equal(
        set_legacy_sample_pan(fixture.manager, 0xFFFF0001U, -16),
        47,
        "0x00485650 returns converted pan"
    );
    test.expect_equal(fixture.backend.pans.back(), 47, "pan reaches backend");

    test.expect_equal(
        play_legacy_sample_u16_level(fixture.manager, 0xABCD0002U, 0x0001000BU),
        1,
        "0x00485670 returns one"
    );
    test.expect_equal(fixture.backend.last_sound_id, 2U, "second sound ID u16");
    test.expect_equal(
        fixture.backend.volumes.back(),
        127,
        "u16 level eleven scales to 128 then manager clamps"
    );
}

void test_stop_wrappers(openswd3::test::Context& test) {
    Fixture fixture;
    expect_ready(test, fixture);
    static_cast<void>(play_legacy_sample(fixture.manager, 1U, 5));
    static_cast<void>(play_legacy_sample(fixture.manager, 2U, 5));

    test.expect_equal(
        stop_legacy_sample(fixture.manager, 0x12340002U),
        1,
        "0x00485720 returns one"
    );
    test.expect_equal(fixture.manager.active_sample_count(), 1U, "one stopped");
    test.expect_equal(
        stop_all_legacy_samples(fixture.manager), 1, "0x00485740 returns one"
    );
    test.expect_equal(fixture.manager.active_sample_count(), 0U, "all stopped");

    fixture.manager.set_sample_enabled(false);
    test.expect_equal(
        stop_all_legacy_samples(fixture.manager),
        1,
        "wrapper still returns one when manager rejects stop-all"
    );
}

void test_spatial_wrapper(openswd3::test::Context& test) {
    Fixture fixture;
    expect_ready(test, fixture);
    const LegacySpatialSampleState state{
        .listener_x = 0,
        .listener_y = 0,
        .mix_level = 11,
    };

    test.expect_equal(
        play_legacy_spatial_sample(fixture.manager, 1U, 512, 0, state),
        512,
        "distance 512 is returned without playback"
    );
    test.expect_equal(fixture.backend.start_count, 0U, "far sample skipped");

    test.expect_equal(
        play_legacy_spatial_sample(fixture.manager, 1U, 511, 0, state),
        126,
        "near-right sample returns converted pan"
    );
    test.expect_equal(fixture.backend.start_count, 1U, "near sample starts");
    test.expect_equal(
        fixture.backend.volumes.back(), 1, "distance attenuation"
    );
    test.expect_equal(fixture.backend.pans.back(), 126, "right pan");

    test.expect_equal(
        play_legacy_spatial_sample(fixture.manager, 2U, 3, 4, state),
        63,
        "3-4-5 distance keeps centered integer pan"
    );
    test.expect_equal(
        fixture.backend.volumes.back(), 127, "distance five volume"
    );

    const u32 starts_before_unmasked_id = fixture.backend.start_count;
    test.expect_equal(
        play_legacy_spatial_sample(fixture.manager, 0x00010001U, 0, 0, state),
        63,
        "spatial wrapper returns manager pan conversion"
    );
    test.expect_equal(
        fixture.backend.start_count,
        starts_before_unmasked_id,
        "0x00485750 does not truncate its sound ID"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_play_and_pan_wrappers(test);
    test_stop_wrappers(test);
    test_spatial_wrapper(test);
    return test.exit_code();
}
