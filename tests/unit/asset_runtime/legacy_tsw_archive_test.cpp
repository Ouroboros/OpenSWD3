#include "test.hpp"

#include "openswd3/asset_runtime/legacy_tsw_archive.hpp"
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

using openswd3::asset_runtime::LegacyTswArchive;
using openswd3::asset_runtime::LegacyTswFrameStatus;
using openswd3::asset_runtime::LegacyTswOpenStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::resource_io::LegacyLzo1xStatus;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kBlockOffset = kIndexOffset + kRecordSize * kSlotCount;
constexpr std::size_t kBlockHeaderSize = 12U;
constexpr std::size_t kPaletteSize = 512U;
constexpr std::size_t kDescriptorSize = 36U;

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

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-tsw-archive-" + std::to_string(unique_value));
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

struct SyntheticArchive {
    std::vector<u8> bytes;
    std::vector<u8> command_stream;
    std::size_t descriptor_offset{};
    std::size_t payload_offset{};
};

[[nodiscard]] SyntheticArchive make_synthetic_archive() {
    SyntheticArchive archive;
    archive.command_stream = {
        0xFFU, 0xFFU, 0x06U, 0x00U, 0x02U, 0x00U, 0x08U, 0x00U, 0x0CU, 0x80U,
        0x02U, 0x80U, 0x02U, 0x00U, 0x02U, 0x04U, 0x02U, 0xC0U, 0x00U, 0x00U,
        0x10U, 0x80U, 0x01U, 0x00U, 0x05U, 0x01U, 0x80U, 0x01U, 0xC0U, 0x03U,
        0x00U, 0x06U, 0x07U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U,
    };

    std::vector<u8> compressed(
        archive.command_stream.size() + archive.command_stream.size() / 16U +
        67U
    );
    const auto compression = openswd3::resource_io::compress_legacy_lzo1x_14(
        archive.command_stream, compressed
    );
    if (compression.status != LegacyLzo1xStatus::success) {
        compressed.clear();
    } else {
        compressed.resize(compression.bytes_written);
    }

    archive.descriptor_offset = kBlockOffset + kBlockHeaderSize + kPaletteSize;
    archive.payload_offset = archive.descriptor_offset + kDescriptorSize;
    archive.bytes.resize(archive.payload_offset + compressed.size(), 0U);

    constexpr std::array<u8, 9> kName{
        'S',
        'Y',
        'N',
        'T',
        'H',
        'E',
        'T',
        'I',
        'C',
    };
    std::ranges::copy(
        kName, archive.bytes.begin() + static_cast<std::ptrdiff_t>(kIndexOffset)
    );
    const u32 block_size =
        static_cast<u32>(archive.bytes.size() - kBlockOffset);
    write_u32(archive.bytes, kIndexOffset + 0x14U, block_size);
    write_u32(
        archive.bytes, kIndexOffset + 0x18U, static_cast<u32>(kBlockOffset)
    );
    write_u32(archive.bytes, kIndexOffset + 0x1CU, 1U);
    write_u32(archive.bytes, kIndexOffset + 0x20U, 0x10203040U);
    write_u32(archive.bytes, kIndexOffset + 0x24U, 0x50607080U);
    write_u32(archive.bytes, kIndexOffset + 0x28U, 0x90A0B0C0U);

    write_u32(archive.bytes, kBlockOffset, 0x12345678U);
    write_u16(archive.bytes, kBlockOffset + 4U, 0xABCDU);
    write_u16(archive.bytes, kBlockOffset + 6U, 1U);
    write_u16(archive.bytes, kBlockOffset + 8U, 8U);
    write_u16(archive.bytes, kBlockOffset + 10U, 12U);
    for (std::size_t index = 0U; index < kPaletteSize; ++index) {
        archive.bytes[kBlockOffset + kBlockHeaderSize + index] =
            static_cast<u8>((index * 3U) & 0xFFU);
    }

    const u32 payload_relative =
        static_cast<u32>(archive.payload_offset - kBlockOffset);
    write_u32(
        archive.bytes, archive.descriptor_offset + 0x00U, payload_relative
    );
    write_u32(
        archive.bytes,
        archive.descriptor_offset + 0x04U,
        static_cast<u32>(compressed.size())
    );
    write_u32(
        archive.bytes,
        archive.descriptor_offset + 0x08U,
        static_cast<u32>(archive.command_stream.size())
    );
    write_u32(archive.bytes, archive.descriptor_offset + 0x0CU, 0x11111111U);
    write_u32(archive.bytes, archive.descriptor_offset + 0x10U, 0x22222222U);
    write_u32(archive.bytes, archive.descriptor_offset + 0x14U, 0x33333333U);
    write_u32(archive.bytes, archive.descriptor_offset + 0x18U, 2U);
    write_u32(archive.bytes, archive.descriptor_offset + 0x1CU, 0x44444444U);
    write_u16(archive.bytes, archive.descriptor_offset + 0x20U, 47U);
    write_u16(archive.bytes, archive.descriptor_offset + 0x22U, 95U);
    std::ranges::copy(
        compressed,
        archive.bytes.begin() +
            static_cast<std::ptrdiff_t>(archive.payload_offset)
    );
    return archive;
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) noexcept {
    std::uint64_t value = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        value ^= byte;
        value *= 0x100000001B3ULL;
    }
    return value;
}

void test_synthetic_frame(openswd3::test::Context& test) {
    const TestTree tree;
    const SyntheticArchive synthetic = make_synthetic_archive();
    tree.write("synthetic.tsw", synthetic.bytes);

    LegacyTswArchive archive;
    test.expect_equal(
        archive.open(tree.path("synthetic.tsw")),
        LegacyTswOpenStatus::ready,
        "synthetic TSW opens"
    );
    test.expect_true(archive.is_open(), "open state is recorded");

    LegacyTswArchive shared_reader;
    test.expect_equal(
        shared_reader.open(tree.path("synthetic.tsw")),
        LegacyTswOpenStatus::ready,
        "TSW physical files allow concurrent read handles"
    );
    shared_reader.close();

    const auto loaded = archive.read_frame(1U, 0U);
    test.expect_equal(
        loaded.status, LegacyTswFrameStatus::ready, "synthetic frame loads"
    );
    test.expect_equal(
        loaded.frame.index.block_offset,
        static_cast<u32>(kBlockOffset),
        "index block offset"
    );
    test.expect_equal(loaded.frame.index.metadata_id, 1U, "index metadata ID");
    test.expect_equal(
        loaded.frame.index.field_20, 0x10203040U, "index field 20 is preserved"
    );
    test.expect_equal(
        loaded.frame.block_value, 0x12345678U, "block field 00 is preserved"
    );
    test.expect_equal(loaded.frame.frame_count, u16{1U}, "frame count");
    test.expect_equal(loaded.frame.storage_bpp, u16{8U}, "storage bpp");
    test.expect_equal(loaded.frame.header_size, u16{12U}, "stored header size");
    test.expect_true(loaded.frame.has_palette, "8-bit block has palette");
    test.expect_equal(loaded.frame.palette[0U], u8{0U}, "palette byte zero");
    test.expect_equal(loaded.frame.palette[511U], u8{253U}, "palette byte 511");
    test.expect_equal(
        loaded.frame.descriptor.auxiliary_presence,
        0x11111111U,
        "descriptor +0c is preserved"
    );
    test.expect_equal(
        loaded.frame.descriptor.auxiliary_trailing_span,
        0x22222222U,
        "descriptor +10 is preserved"
    );
    test.expect_equal(
        loaded.frame.descriptor.auxiliary_pixel_count,
        0x33333333U,
        "descriptor +14 is preserved"
    );
    test.expect_equal(
        loaded.frame.descriptor.auxiliary_type,
        2U,
        "descriptor +18 is preserved"
    );
    test.expect_equal(
        loaded.frame.descriptor.field_1c,
        0x44444444U,
        "descriptor +1c is preserved"
    );
    test.expect_equal(loaded.frame.descriptor.width, u16{47U}, "frame width");
    test.expect_equal(loaded.frame.descriptor.height, u16{95U}, "frame height");
    test.expect_true(
        std::ranges::equal(
            loaded.frame.command_stream, synthetic.command_stream
        ),
        "decompressed command stream is exact"
    );

    archive.close();
    test.expect_false(archive.is_open(), "close clears open state");
    test.expect_equal(
        archive.read_frame(1U, 0U).status,
        LegacyTswFrameStatus::archive_not_open,
        "closed archive rejects reads"
    );
}

void test_safety_boundaries(openswd3::test::Context& test) {
    const TestTree tree;
    const SyntheticArchive synthetic = make_synthetic_archive();
    tree.write("base.tsw", synthetic.bytes);

    LegacyTswArchive archive;
    static_cast<void>(archive.open(tree.path("base.tsw")));
    test.expect_equal(
        archive.read_frame(0U, 0U).status,
        LegacyTswFrameStatus::physical_record_out_of_range,
        "record zero underflow is isolated"
    );
    test.expect_equal(
        archive.read_frame(3001U, 0U).status,
        LegacyTswFrameStatus::physical_record_out_of_range,
        "record above physical table is isolated"
    );
    test.expect_equal(
        archive.read_frame(2U, 0U).status,
        LegacyTswFrameStatus::empty_index_record,
        "zero index slot remains empty"
    );
    test.expect_equal(
        archive.read_frame(1U, 1U).status,
        LegacyTswFrameStatus::variant_out_of_range,
        "variant outside valid caller range is isolated"
    );
    archive.close();

    std::vector<u8> bytes = synthetic.bytes;
    write_u16(bytes, kBlockOffset + 4U, 0U);
    tree.write("bad-magic.tsw", bytes);
    static_cast<void>(archive.open(tree.path("bad-magic.tsw")));
    test.expect_equal(
        archive.read_frame(1U, 0U).status,
        LegacyTswFrameStatus::invalid_block_magic,
        "block magic is exact"
    );
    archive.close();

    bytes = synthetic.bytes;
    write_u32(bytes, kIndexOffset + 0x14U, 12U);
    tree.write("short-palette.tsw", bytes);
    static_cast<void>(archive.open(tree.path("short-palette.tsw")));
    test.expect_equal(
        archive.read_frame(1U, 0U).status,
        LegacyTswFrameStatus::palette_read_failed,
        "8-bit palette must fit in the block"
    );
    archive.close();

    bytes = synthetic.bytes;
    write_u32(
        bytes,
        synthetic.descriptor_offset + 0x08U,
        static_cast<u32>(synthetic.command_stream.size() + 1U)
    );
    tree.write("size-mismatch.tsw", bytes);
    static_cast<void>(archive.open(tree.path("size-mismatch.tsw")));
    test.expect_equal(
        archive.read_frame(1U, 0U).status,
        LegacyTswFrameStatus::decompressed_size_mismatch,
        "declared output size must match exactly"
    );
    archive.close();

    bytes = synthetic.bytes;
    write_u32(bytes, synthetic.descriptor_offset + 0x04U, 2U);
    tree.write("truncated-stream.tsw", bytes);
    static_cast<void>(archive.open(tree.path("truncated-stream.tsw")));
    test.expect_equal(
        archive.read_frame(1U, 0U).status,
        LegacyTswFrameStatus::decompression_failed,
        "truncated compressed stream is rejected"
    );
    archive.close();

    constexpr std::array<u8, 16> kTinyFile{};
    tree.write("tiny.tsw", kTinyFile);
    static_cast<void>(archive.open(tree.path("tiny.tsw")));
    test.expect_equal(
        archive.read_frame(1U, 0U).status,
        LegacyTswFrameStatus::index_out_of_file_range,
        "short physical index is isolated"
    );
}

struct RealFrameExpectation {
    const char* archive_name;
    u16 storage_bpp;
    u16 width;
    u16 height;
    u32 decompressed_size;
    std::uint64_t fnv1a;
};

constexpr std::array<RealFrameExpectation, 6> kRealFrames{{
    {"all_char.tsw", 8U, 47U, 95U, 2235U, 0x332B7CEEA6A4FCA1ULL},
    {"all_item.tsw", 16U, 120U, 220U, 54130U, 0x920D424365A642F9ULL},
    {"all_magic.tsw", 16U, 132U, 132U, 19660U, 0xD293F7B1271ED2FBULL},
    {"all_sys.tsw", 8U, 32U, 32U, 561U, 0x760701EB31B82503ULL},
    {"all_map1.tsw", 16U, 144U, 176U, 23780U, 0xAE307258CC1B01DAULL},
    {"all_map2.tsw", 16U, 640U, 400U, 514410U, 0xF1B883B0F8A6EDA6ULL},
}};

void test_real_archives(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    for (const RealFrameExpectation& expected : kRealFrames) {
        LegacyTswArchive archive;
        test.expect_equal(
            archive.open(root / expected.archive_name),
            LegacyTswOpenStatus::ready,
            "real TSW archive opens"
        );
        if (!archive.is_open()) {
            continue;
        }

        const auto loaded = archive.read_frame(1U, 0U);
        test.expect_equal(
            loaded.status, LegacyTswFrameStatus::ready, "real first frame loads"
        );
        if (loaded.status != LegacyTswFrameStatus::ready) {
            continue;
        }
        test.expect_equal(
            loaded.frame.index.metadata_id, 1U, "real first slot metadata ID"
        );
        test.expect_equal(
            loaded.frame.storage_bpp, expected.storage_bpp, "real storage bpp"
        );
        test.expect_equal(
            loaded.frame.has_palette,
            expected.storage_bpp == 8U,
            "palette presence follows storage bpp"
        );
        test.expect_equal(
            loaded.frame.descriptor.width, expected.width, "real frame width"
        );
        test.expect_equal(
            loaded.frame.descriptor.height, expected.height, "real frame height"
        );
        test.expect_equal(
            loaded.frame.command_stream.size(),
            static_cast<std::size_t>(expected.decompressed_size),
            "real decompressed size"
        );
        test.expect_true(
            loaded.frame.command_stream.size() >= 2U,
            "real command stream has family word"
        );
        if (loaded.frame.command_stream.size() >= 2U) {
            test.expect_equal(
                loaded.frame.command_stream[0U],
                u8{0xFFU},
                "real stream family low byte"
            );
            test.expect_equal(
                loaded.frame.command_stream[1U],
                u8{0xFFU},
                "real stream family high byte"
            );
        }
        test.expect_equal(
            fnv1a64(loaded.frame.command_stream),
            expected.fnv1a,
            "real decompressed bytes match evidence"
        );
    }
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_synthetic_frame(test);
    test_safety_boundaries(test);
    if (argument_count == 2) {
        test_real_archives(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
