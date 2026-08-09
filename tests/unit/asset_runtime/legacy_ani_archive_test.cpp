#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_archive.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
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

using openswd3::asset_runtime::apply_legacy_ani_spans;
using openswd3::asset_runtime::kLegacyAniHeaderSize;
using openswd3::asset_runtime::kLegacyAniPaletteBytes;
using openswd3::asset_runtime::kLegacyAniScratchCapacity;
using openswd3::asset_runtime::LegacyAniArchive;
using openswd3::asset_runtime::LegacyAniFrameLoadMode;
using openswd3::asset_runtime::LegacyAniFrameLoadStatus;
using openswd3::asset_runtime::LegacyAniOpenStatus;
using openswd3::asset_runtime::LegacyAniSpanStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::resource_io::LegacyLzo1xStatus;

void write_u16(const std::span<u8> bytes, const std::size_t offset,
               const u16 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(const std::span<u8> bytes, const std::size_t offset,
               const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u16 read_u16(const std::span<const u8> bytes,
                           const std::size_t offset) {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U));
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) noexcept {
    std::uint64_t value = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        value ^= byte;
        value *= 0x100000001B3ULL;
    }
    return value;
}

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-ani-archive-" + std::to_string(unique_value));
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

[[nodiscard]] std::vector<u8> compress(const std::span<const u8> source) {
    std::vector<u8> output(source.size() + source.size() / 16U + 67U);
    const auto compressed =
        openswd3::resource_io::compress_legacy_lzo1x_14(source, output);
    if (compressed.status != LegacyLzo1xStatus::success) {
        return {};
    }
    output.resize(compressed.bytes_written);
    return output;
}

struct SyntheticAni {
    std::vector<u8> bytes;
    std::vector<u8> frame_one_commands;
    std::vector<u8> frame_three_commands;
};

[[nodiscard]] SyntheticAni make_synthetic_ani() {
    SyntheticAni ani;
    ani.frame_one_commands.resize(11U);
    write_u32(ani.frame_one_commands, 0U, 11U);
    write_u32(ani.frame_one_commands, 4U, 4U);
    ani.frame_one_commands[8U] = 1U;
    ani.frame_one_commands[9U] = 2U;
    ani.frame_one_commands[10U] = 3U;

    ani.frame_three_commands.resize(10U);
    write_u32(ani.frame_three_commands, 0U, 10U);
    write_u32(ani.frame_three_commands, 4U, 0U);
    ani.frame_three_commands[8U] = 3U;
    ani.frame_three_commands[9U] = 1U;

    const std::vector<u8> compressed_one = compress(ani.frame_one_commands);
    const std::vector<u8> compressed_three = compress(ani.frame_three_commands);
    const u32 record_one_size = static_cast<u32>(compressed_one.size() + 12U);
    constexpr u32 kRecordTwoSize = 12U;
    const u32 record_three_size =
        static_cast<u32>(compressed_three.size() + 12U);

    const std::size_t frame_one_offset = kLegacyAniHeaderSize;
    const std::size_t frame_two_offset = frame_one_offset + record_one_size;
    const std::size_t frame_three_offset = frame_two_offset + kRecordTwoSize;
    const std::size_t palette_offset =
        frame_three_offset + compressed_three.size();
    ani.bytes.resize(palette_offset + kLegacyAniPaletteBytes, 0U);

    ani.bytes[0U] = 'A';
    ani.bytes[1U] = 'N';
    ani.bytes[2U] = 'I';
    write_u32(ani.bytes, 0x04U, 3U);
    write_u16(ani.bytes, 0x08U, 8U);
    write_u16(ani.bytes, 0x0AU, 640U);
    write_u16(ani.bytes, 0x0CU, 480U);
    write_u16(ani.bytes, 0x0EU, 640U);
    write_u16(ani.bytes, 0x10U, 480U);
    write_u16(ani.bytes, 0x12U, 0x0101U);
    write_u16(ani.bytes, 0x14U, 0U);
    write_u16(ani.bytes, 0x16U, 0x0400U);
    write_u32(ani.bytes, 0x18U,
              static_cast<u32>(ani.frame_one_commands.size() + 12U));
    write_u32(ani.bytes, 0x1CU, 1U);
    write_u32(ani.bytes, 0x20U, record_one_size);

    std::ranges::copy(compressed_one,
                      ani.bytes.begin() +
                          static_cast<std::ptrdiff_t>(frame_one_offset));
    const std::size_t trailer_one = frame_one_offset + compressed_one.size();
    write_u32(ani.bytes, trailer_one, 12U);
    write_u32(ani.bytes, trailer_one + 4U, 0U);
    write_u32(ani.bytes, trailer_one + 8U, kRecordTwoSize);

    write_u32(ani.bytes, frame_two_offset,
              static_cast<u32>(ani.frame_three_commands.size() + 12U));
    write_u32(ani.bytes, frame_two_offset + 4U, 1U);
    write_u32(ani.bytes, frame_two_offset + 8U, record_three_size);
    std::ranges::copy(compressed_three,
                      ani.bytes.begin() +
                          static_cast<std::ptrdiff_t>(frame_three_offset));

    ani.bytes[palette_offset + 3U] = 0xFFU;
    ani.bytes[palette_offset + 7U] = 0xFFU;
    ani.bytes[palette_offset + 11U] = 0xFFU;
    return ani;
}

void test_synthetic_chain_and_spans(openswd3::test::Context& test) {
    const TestTree tree;
    const SyntheticAni synthetic = make_synthetic_ani();
    tree.write("synthetic.Ani", synthetic.bytes);

    LegacyAniArchive archive;
    test.expect_equal(archive.open(tree.path("synthetic.Ani")),
                      LegacyAniOpenStatus::ready, "synthetic ANI opens");
    test.expect_equal(archive.header().frame_count, 3U,
                      "frame count is parsed");
    test.expect_equal(archive.header().initial_span_count, 1U,
                      "first span count is parsed");

    const auto first = archive.load_frame(1U);
    test.expect_equal(first.status, LegacyAniFrameLoadStatus::ready,
                      "first frame loads");
    test.expect_equal(first.mode, LegacyAniFrameLoadMode::first_load,
                      "first frame excludes its trailer");
    test.expect_equal(first.node.record_seek_base, 0x18U,
                      "legacy node stores physical offset minus twelve");
    test.expect_equal(first.node.one_based_frame, 1U,
                      "frame numbering is one based");
    test.expect_true(
        std::ranges::equal(first.command_stream, synthetic.frame_one_commands),
        "first decompressed stream is exact");
    test.expect_equal(first.palette[1U], u16{0x7C00U},
                      "RGB palette red becomes RGB555");
    test.expect_equal(first.palette[2U], u16{0x03E0U},
                      "RGB palette green becomes RGB555");
    test.expect_equal(first.palette[3U], u16{0x001FU},
                      "RGB palette blue becomes RGB555");

    std::vector<u8> framebuffer(640U * 480U * 2U, 0U);
    const auto rendered = apply_legacy_ani_spans(
        first.command_stream, first.node.span_count, first.palette, framebuffer,
        1280U, archive.header().display_height);
    test.expect_equal(rendered.status, LegacyAniSpanStatus::completed,
                      "indexed span renders");
    test.expect_equal(rendered.spans_written, 1U, "one span is written");
    test.expect_equal(rendered.source_pixels, 3U, "three indexed pixels");
    test.expect_equal(read_u16(framebuffer, 4U), u16{0x7C00U},
                      "first palette pixel");
    test.expect_equal(read_u16(framebuffer, 6U), u16{0x03E0U},
                      "second palette pixel");
    test.expect_equal(read_u16(framebuffer, 8U), u16{0x001FU},
                      "third palette pixel");

    const auto cached_first = archive.load_frame(1U);
    test.expect_equal(cached_first.status, LegacyAniFrameLoadStatus::ready,
                      "cached frame reloads");
    test.expect_equal(cached_first.mode, LegacyAniFrameLoadMode::cached_reload,
                      "cached path includes its trailer");
    test.expect_equal(cached_first.decompression_status,
                      LegacyLzo1xStatus::input_not_consumed,
                      "cached trailer produces original input-not-consumed");
    test.expect_true(std::ranges::equal(cached_first.command_stream,
                                        synthetic.frame_one_commands),
                     "cached output still matches first load");

    const auto second = archive.load_frame(2U);
    test.expect_equal(second.status, LegacyAniFrameLoadStatus::ready,
                      "zero-payload frame loads");
    test.expect_equal(second.mode,
                      LegacyAniFrameLoadMode::first_load_zero_payload,
                      "first zero-payload path skips decompression");
    test.expect_true(second.command_stream.empty(),
                     "zero-payload frame has no span stream");

    const auto cached_second = archive.load_frame(2U);
    test.expect_equal(cached_second.status, LegacyAniFrameLoadStatus::ready,
                      "cached zero-payload frame remains visible no-op");
    test.expect_equal(
        cached_second.mode,
        LegacyAniFrameLoadMode::cached_zero_payload_isolated,
        "unsafe stale-scratch decompression is isolated at platform boundary");

    const auto third = archive.load_frame(3U);
    test.expect_equal(third.status, LegacyAniFrameLoadStatus::ready,
                      "last frame overlaps palette trailer");
    test.expect_true(std::ranges::equal(third.command_stream,
                                        synthetic.frame_three_commands),
                     "last frame excludes overlapping palette bytes");
    test.expect_equal(archive.cached_frame_count(), std::size_t{3U},
                      "all frame nodes remain cached");
    const auto past_end = archive.load_frame(4U);
    test.expect_equal(past_end.status, LegacyAniFrameLoadStatus::frame_past_end,
                      "frame after declared count ends playback");
    test.expect_equal(past_end.legacy_return_value, 1U,
                      "past-end path keeps original true return");
}

void test_raw_span_conversion_and_boundaries(openswd3::test::Context& test) {
    std::array<u8, 12> command{};
    write_u32(command, 0U, 12U);
    write_u32(command, 4U, 2U);
    write_u16(command, 8U, 0x7C00U);
    write_u16(command, 10U, 0x03E0U);
    std::array<u8, 16> destination{};
    openswd3::rendering::LegacyPixelConversionState conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        conversion, {0xF800U, 0x07E0U, 0x001FU});
    const auto result = apply_legacy_ani_spans(command, 1U, {}, destination,
                                               16U, 480U, conversion);
    test.expect_equal(result.status, LegacyAniSpanStatus::completed,
                      "raw 16-bit span renders");
    test.expect_equal(read_u16(destination, 2U), u16{0xF800U},
                      "raw red is converted to RGB565");
    test.expect_equal(read_u16(destination, 4U), u16{0x07C0U},
                      "legacy conversion preserves five-bit green");

    test.expect_equal(
        apply_legacy_ani_spans(command, 1U, {}, destination, 0U, 480U).status,
        LegacyAniSpanStatus::invalid_pitch, "zero pitch is isolated");
    test.expect_equal(
        apply_legacy_ani_spans(command, 1U, {}, destination, 16U, 481U).status,
        LegacyAniSpanStatus::invalid_display_height,
        "display height above legacy viewport is isolated");
    test.expect_equal(
        apply_legacy_ani_spans(command, 1U, {},
                               std::span<u8>{destination}.first(4U), 16U, 480U)
            .status,
        LegacyAniSpanStatus::destination_out_of_bounds,
        "span destination overflow is isolated");
}

void test_open_and_frame_safety(openswd3::test::Context& test) {
    const TestTree tree;
    LegacyAniArchive archive;
    test.expect_equal(archive.open(tree.path("missing.Ani")),
                      LegacyAniOpenStatus::file_open_failed,
                      "missing ANI fails open");

    SyntheticAni synthetic = make_synthetic_ani();
    synthetic.bytes[0U] = 'X';
    tree.write("bad-magic.Ani", synthetic.bytes);
    test.expect_equal(archive.open(tree.path("bad-magic.Ani")),
                      LegacyAniOpenStatus::invalid_magic,
                      "invalid ANI magic is isolated");

    synthetic = make_synthetic_ani();
    write_u32(synthetic.bytes, 0x20U,
              static_cast<u32>(kLegacyAniScratchCapacity + 1U));
    tree.write("large-record.Ani", synthetic.bytes);
    static_cast<void>(archive.open(tree.path("large-record.Ani")));
    test.expect_equal(archive.load_frame(1U).status,
                      LegacyAniFrameLoadStatus::scratch_capacity_exceeded,
                      "record larger than original scratch is isolated");
    archive.close();

    synthetic = make_synthetic_ani();
    tree.write("base.Ani", synthetic.bytes);
    static_cast<void>(archive.open(tree.path("base.Ani")));
    test.expect_equal(archive.load_frame(0U).status,
                      LegacyAniFrameLoadStatus::frame_index_zero,
                      "zero frame is outside one-based range");
    archive.close();
}

struct RealAniExpectation {
    const char* name;
    u32 frame_count;
    std::uint64_t first_frame_fnv1a;
};

constexpr std::array<RealAniExpectation, 19> kRealAniFiles{{
    {"Bd2Dh2.Ani", 329U, 0xBC730F72D7DBBAD7ULL},
    {"BigArmy.Ani", 176U, 0xE1FF3E8C4AAD6C52ULL},
    {"ChaosWar.Ani", 31U, 0xA2FF7FC5C5D854DBULL},
    {"GetSword.Ani", 301U, 0x12DDA7E5433E295DULL},
    {"LiliaDie.Ani", 403U, 0xF7EB28238C0B4760ULL},
    {"MonkDie.Ani", 205U, 0x5EC8B523B1B29EB2ULL},
    {"Withdraw.Ani", 109U, 0xB3CA428B765EB5FAULL},
    {"attack-1.Ani", 89U, 0x29ACA6E48938B942ULL},
    {"bd2dh.Ani", 233U, 0xB6157C2E25EEAA25ULL},
    {"combat01.Ani", 154U, 0x35FBFCDECECCA0BCULL},
    {"dh2ch3.Ani", 280U, 0x491276CECE22379AULL},
    {"dm2bd.Ani", 170U, 0x984B2AC8D74404AEULL},
    {"expv.Ani", 127U, 0x35E810BFE3A60DFDULL},
    {"fogg.Ani", 77U, 0xBEB5D1CE251BCB21ULL},
    {"kungfu.Ani", 846U, 0xF44991E0B7341659ULL},
    {"memory.Ani", 507U, 0x267652083A92EB9AULL},
    {"monk.Ani", 330U, 0xAFCC9AD60C85ADE1ULL},
    {"nicole.Ani", 790U, 0xAC4C7DF349D4B25DULL},
    {"ve2dm.Ani", 155U, 0x0DC73CB911E74A2FULL},
}};

void test_real_ani_files(openswd3::test::Context& test,
                         const std::filesystem::path& video_root) {
    std::uint64_t total_output_bytes{};
    std::uint64_t total_spans{};
    std::uint64_t total_frames{};
    std::uint64_t nonempty_frames{};
    std::uint64_t zero_frames{};
    std::uint64_t total_indexed_pixels{};
    for (const RealAniExpectation expected : kRealAniFiles) {
        LegacyAniArchive archive;
        test.expect_equal(archive.open(video_root / expected.name),
                          LegacyAniOpenStatus::ready, "real ANI opens");
        test.expect_equal(archive.header().frame_count, expected.frame_count,
                          "real frame count matches inventory");
        std::vector<u8> framebuffer(640U * 480U * 2U, 0U);
        for (u32 frame = 1U; frame <= expected.frame_count; ++frame) {
            const auto loaded = archive.load_frame(frame);
            test.expect_equal(loaded.status, LegacyAniFrameLoadStatus::ready,
                              "real ANI frame loads");
            total_output_bytes += loaded.command_stream.size();
            total_spans += loaded.node.span_count;
            ++total_frames;
            if (frame == 1U) {
                test.expect_equal(fnv1a64(loaded.command_stream),
                                  expected.first_frame_fnv1a,
                                  "real first-frame command hash");
            }
            const auto rendered = apply_legacy_ani_spans(
                loaded.command_stream, loaded.node.span_count, loaded.palette,
                framebuffer, 1280U, archive.header().display_height);
            test.expect_equal(rendered.status, LegacyAniSpanStatus::completed,
                              "real ANI spans stay in the framebuffer");
            test.expect_equal(rendered.spans_written, loaded.node.span_count,
                              "real declared span count is consumed exactly");
            total_indexed_pixels += rendered.source_pixels;
            if (loaded.mode ==
                LegacyAniFrameLoadMode::first_load_zero_payload) {
                ++zero_frames;
            } else {
                ++nonempty_frames;
            }
        }
        test.expect_equal(archive.cached_frame_count(),
                          static_cast<std::size_t>(expected.frame_count),
                          "real frame chain is complete");
        const auto cached_first = archive.load_frame(1U);
        test.expect_equal(cached_first.status, LegacyAniFrameLoadStatus::ready,
                          "real first frame cached reload succeeds");
        test.expect_equal(cached_first.mode,
                          LegacyAniFrameLoadMode::cached_reload,
                          "real cached frame includes trailer");
    }

    test.expect_equal(total_frames, std::uint64_t{5312U},
                      "all real frames are traversed");
    test.expect_equal(nonempty_frames, std::uint64_t{4623U},
                      "all compressed frames are decompressed");
    test.expect_equal(zero_frames, std::uint64_t{689U},
                      "all zero-payload hold frames are preserved");
    test.expect_equal(total_output_bytes, std::uint64_t{226892398U},
                      "all decompressed command bytes match inventory");
    test.expect_equal(total_spans, std::uint64_t{6057767U},
                      "all declared spans match inventory");
    test.expect_equal(total_indexed_pixels, std::uint64_t{178426082U},
                      "all indexed pixels match inventory");
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_synthetic_chain_and_spans(test);
    test_raw_span_conversion_and_boundaries(test);
    test_open_and_frame_safety(test);
    if (argument_count == 2) {
        test_real_ani_files(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
