#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_activity.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::asset_runtime::kLegacyAniFramebufferBytes;
using openswd3::asset_runtime::LegacyAniActivity;
using openswd3::asset_runtime::LegacyAniActivityBlockers;
using openswd3::asset_runtime::LegacyAniActivityPath;
using openswd3::asset_runtime::LegacyAniActivityPorts;
using openswd3::asset_runtime::LegacyAniActivityStartStatus;
using openswd3::asset_runtime::LegacyAniActivityState;
using openswd3::asset_runtime::LegacyAniActivityStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u16
read_u16(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

class TestTree {
public:
    TestTree() {
        const auto value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-ani-activity-" + std::to_string(value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path path(const char* name) const {
        return root_ / name;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream output{path(name), std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

[[nodiscard]] std::vector<u8> make_one_frame_ani() {
    constexpr std::size_t kHeaderSize = 0x24U;
    constexpr std::size_t kPaletteSize = 0x300U;
    constexpr u32 kTargetOffset = 100U * 1280U + 20U;

    std::array<u8, 9> commands{};
    write_u32(commands, 0U, 9U);
    write_u32(commands, 4U, kTargetOffset);
    commands[8U] = 10U;

    std::vector<u8> compressed(commands.size() + 68U);
    const auto compression =
        openswd3::resource_io::compress_legacy_lzo1x_14(commands, compressed);
    compressed.resize(compression.bytes_written);

    const u32 record_size = static_cast<u32>(compressed.size() + 12U);
    const std::size_t palette_offset = kHeaderSize + compressed.size();
    std::vector<u8> bytes(palette_offset + kPaletteSize, 0U);
    bytes[0U] = 'A';
    bytes[1U] = 'N';
    bytes[2U] = 'I';
    write_u32(bytes, 0x04U, 1U);
    write_u16(bytes, 0x08U, 8U);
    write_u16(bytes, 0x0AU, 640U);
    write_u16(bytes, 0x0CU, 480U);
    write_u16(bytes, 0x0EU, 640U);
    write_u16(bytes, 0x10U, 480U);
    write_u16(bytes, 0x12U, 0x0101U);
    write_u32(bytes, 0x18U, static_cast<u32>(commands.size() + 12U));
    write_u32(bytes, 0x1CU, 1U);
    write_u32(bytes, 0x20U, record_size);
    std::ranges::copy(
        compressed, bytes.begin() + static_cast<std::ptrdiff_t>(kHeaderSize)
    );
    bytes[palette_offset + 30U] = 0xFFU;
    return bytes;
}

class RecordingPorts final : public LegacyAniActivityPorts {
public:
    void redraw_scene_without_ani() override {
        ++redraw_count;
        if (observed_state != nullptr) {
            redraw_active_extent = observed_state->active_extent;
            redraw_scene_flags = observed_state->scene_flags;
        }
    }

    void apply_ending_color_adjustment(
        const std::span<u8> framebuffer,
        const u32 pixel_count,
        const i32 first,
        const i32 second,
        const i32 third
    ) override {
        effect_framebuffer_size = framebuffer.size();
        effect_pixel_counts.push_back(pixel_count);
        effect_values.push_back({first, second, third});
    }

    void finalize_service(const u32 service_id) override {
        service_ids.push_back(service_id);
        if (observed_state != nullptr) {
            finalize_active_extent = observed_state->active_extent;
            finalize_phase = observed_state->phase;
            finalize_process_flags = observed_state->process_flags;
        }
    }

    const LegacyAniActivityState* observed_state{};
    u32 redraw_count{};
    u32 redraw_active_extent{};
    u32 redraw_scene_flags{};
    std::size_t effect_framebuffer_size{};
    std::vector<u32> effect_pixel_counts;
    std::vector<std::array<i32, 3>> effect_values;
    std::vector<u32> service_ids;
    u32 finalize_active_extent{};
    i32 finalize_phase{};
    u32 finalize_process_flags{};
};

void test_reveal_snapshot_playback_and_default_end(
    openswd3::test::Context& test, const std::filesystem::path& ani_path
) {
    LegacyAniActivity activity;
    const auto started = activity.start(ani_path, 0U, 0x40U, 0x08U);
    test.expect_equal(
        started.status,
        LegacyAniActivityStartStatus::ready,
        "ANI activity starts"
    );
    test.expect_equal(
        activity.state().active_extent,
        640U,
        "header +0x0e is the activity sentinel"
    );
    test.expect_equal(
        activity.state().phase,
        i32{-13},
        "normal activity begins at negative thirteen"
    );
    test.expect_equal(
        activity.state().process_flags, 0x42U, "start sets process bit one"
    );

    RecordingPorts ports;
    ports.observed_state = &activity.state();
    std::vector<u8> framebuffer(kLegacyAniFramebufferBytes, 0xA5U);
    LegacyAniActivityBlockers blockers;

    auto result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::reveal_frame,
        "negative phase renders reveal frame"
    );
    test.expect_equal(
        result.phase_after, i32{-12}, "reveal phase increments once"
    );
    test.expect_equal(
        read_u16(framebuffer, 100U * 1280U + 20U),
        u16{0x7C00U},
        "reveal applies current ANI span"
    );

    std::ranges::fill(framebuffer, 0xA5U);
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.phase_after, i32{-11}, "second reveal phase increments once"
    );
    test.expect_true(
        std::ranges::all_of(
            std::span<const u8>{framebuffer}.first(4U * 1280U),
            [](const u8 value) { return value == 0U; }
        ),
        "phase minus twelve clears four top rows"
    );
    test.expect_true(
        std::ranges::all_of(
            std::span<const u8>{framebuffer}.last(4U * 1280U),
            [](const u8 value) { return value == 0U; }
        ),
        "phase minus twelve clears four bottom rows"
    );

    blockers.first = 1U;
    const std::vector<u8> snapshot = framebuffer;
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::snapshot_saved,
        "first blocked call saves framebuffer"
    );
    std::ranges::fill(framebuffer, 0x11U);
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::snapshot_restored_while_blocked,
        "later blocked call restores framebuffer"
    );
    test.expect_true(framebuffer == snapshot, "blocked restore is byte exact");
    blockers.first = 0U;
    std::ranges::fill(framebuffer, 0x22U);
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::snapshot_restored_on_resume,
        "first resumed call only restores framebuffer"
    );
    test.expect_equal(
        activity.state().phase,
        i32{-11},
        "resume restore does not advance phase"
    );
    test.expect_true(framebuffer == snapshot, "resume restore is byte exact");

    activity.state().phase = 1;
    std::ranges::fill(framebuffer, 0U);
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::playback_frame,
        "positive phase reloads one-based frame"
    );
    test.expect_equal(
        result.phase_after, i32{2}, "playback increments requested frame"
    );
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::playback_exhausted,
        "frame past end enters phase ten thousand"
    );
    test.expect_equal(
        result.legacy_return_value,
        0U,
        "past-end transition itself returns zero"
    );

    std::ranges::fill(framebuffer, 0x7AU);
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::ending_frame,
        "default ending begins after exhausted transition"
    );
    test.expect_equal(
        result.legacy_return_value, 1U, "ending phase returns one"
    );
    test.expect_equal(
        ports.redraw_count, 1U, "default ending redraws scene once per call"
    );
    test.expect_equal(
        ports.redraw_active_extent, 0U, "redraw observes inactive ANI sentinel"
    );
    test.expect_equal(
        ports.redraw_scene_flags & 1U,
        0U,
        "redraw observes scene bit zero cleared"
    );
    test.expect_true(
        std::ranges::all_of(
            std::span<const u8>{framebuffer}.first(60U * 1280U),
            [](const u8 value) { return value == 0U; }
        ),
        "phase ten thousand clears sixty top rows"
    );
    test.expect_true(
        std::ranges::all_of(
            std::span<const u8>{framebuffer}.last(60U * 1280U),
            [](const u8 value) { return value == 0U; }
        ),
        "phase ten thousand clears sixty bottom rows"
    );

    activity.state().phase = 10030;
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::finalized,
        "default ending finalizes after phase 10030"
    );
    test.expect_true(result.finalized, "final result marks cleanup");
    test.expect_false(activity.is_active(), "cleanup clears active sentinel");
    test.expect_equal(
        ports.service_ids, std::vector<u32>{0x23U}, "cleanup calls service 0x23"
    );
    test.expect_equal(
        ports.finalize_active_extent,
        0U,
        "service observes cleared active sentinel"
    );
    test.expect_equal(
        ports.finalize_phase, i32{0}, "service observes cleared phase"
    );
    test.expect_equal(
        ports.finalize_process_flags,
        0x40U,
        "service observes process bit one cleared"
    );
}

void test_effect_end_and_boundaries(
    openswd3::test::Context& test, const std::filesystem::path& ani_path
) {
    LegacyAniActivity activity;
    auto started = activity.start(ani_path, 0x03U, 0U, 0U);
    test.expect_equal(
        started.status,
        LegacyAniActivityStartStatus::ready,
        "effect activity starts"
    );
    test.expect_equal(
        activity.state().phase,
        i32{1},
        "flag bit one skips negative reveal phase"
    );

    RecordingPorts ports;
    ports.observed_state = &activity.state();
    std::vector<u8> framebuffer(kLegacyAniFramebufferBytes, 0U);
    const LegacyAniActivityBlockers blockers;
    activity.state().phase = 10000;
    auto result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::ending_frame,
        "effect ending remains active at phase 10000"
    );
    test.expect_equal(
        result.ending_adjustment, i32{0}, "first effect amount is zero"
    );
    test.expect_equal(
        ports.effect_pixel_counts,
        std::vector<u32>{0x4B000U},
        "effect receives all legacy pixels"
    );
    test.expect_equal(
        ports.effect_framebuffer_size,
        kLegacyAniFramebufferBytes,
        "effect receives fixed framebuffer byte span"
    );

    activity.state().phase = 10015;
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::finalized,
        "effect ending finalizes after phase 10015"
    );
    test.expect_equal(
        result.ending_adjustment,
        i32{-30},
        "last effect amount preserves signed minus thirty"
    );
    test.expect_equal(
        ports.redraw_count, 0U, "effect ending does not use scene redraw branch"
    );
    test.expect_equal(
        ports.effect_values.back(),
        std::array<i32, 3>{-30, -30, -30},
        "all three color adjustments are identical"
    );

    started = activity.start(ani_path, 0U, 0U, 0U);
    test.expect_equal(
        started.status,
        LegacyAniActivityStartStatus::ready,
        "activity restarts for boundary checks"
    );
    const i32 phase = activity.state().phase;
    result = activity.update(
        std::span<u8>{framebuffer}.first(100U), 1280U, blockers, ports
    );
    test.expect_equal(
        result.status,
        LegacyAniActivityStatus::framebuffer_too_small,
        "short framebuffer is isolated"
    );
    test.expect_equal(
        activity.state().phase, phase, "boundary failure does not advance phase"
    );
    result = activity.update(framebuffer, 1279U, blockers, ports);
    test.expect_equal(
        result.status,
        LegacyAniActivityStatus::invalid_pitch,
        "odd pitch is isolated"
    );

    activity.state().flags |= 0x10U;
    result = activity.update(framebuffer, 1280U, blockers, ports);
    test.expect_equal(
        result.path,
        LegacyAniActivityPath::snapshot_saved,
        "flag bit four enters snapshot branch"
    );
}

void test_start_failure(
    openswd3::test::Context& test, const std::filesystem::path& missing
) {
    LegacyAniActivity activity;
    const auto result = activity.start(missing, 0U, 0U, 0U);
    test.expect_equal(
        result.status,
        LegacyAniActivityStartStatus::archive_open_failed,
        "missing ANI does not start activity"
    );
    test.expect_false(
        activity.is_active(), "failed start leaves activity inactive"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    const TestTree tree;
    const std::vector<u8> ani = make_one_frame_ani();
    tree.write("one-frame.Ani", ani);
    test_reveal_snapshot_playback_and_default_end(
        test, tree.path("one-frame.Ani")
    );
    test_effect_end_and_boundaries(test, tree.path("one-frame.Ani"));
    test_start_failure(test, tree.path("missing.Ani"));
    return test.exit_code();
}
