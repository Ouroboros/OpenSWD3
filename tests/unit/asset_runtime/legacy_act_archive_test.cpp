#include "test.hpp"

#include "openswd3/asset_runtime/legacy_act_archive.hpp"

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

using openswd3::asset_runtime::LegacyActArchive;
using openswd3::asset_runtime::LegacyActOpenStatus;
using openswd3::asset_runtime::LegacyActVariantStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kIndexRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kBlockOffset =
    kIndexOffset + kIndexRecordSize * kSlotCount;

constexpr std::array<u8, 6> kVariant0{
    0x00U,
    0x00U,
    0x4EU,
    0x54U,
    0x01U,
    0x00U,
};
constexpr std::array<u8, 4> kVariant2{
    0x46U,
    0x52U,
    0x71U,
    0x01U,
};
constexpr std::array<u8, 6> kVariant4{
    0x44U,
    0x53U,
    0x00U,
    0x00U,
    0x4FU,
    0x4EU,
};

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
            ("legacy-act-archive-" + std::to_string(unique_value));
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

[[nodiscard]] std::vector<u8> synthetic_archive() {
    constexpr std::size_t kVariantTableSize = 2U + 5U * 4U;
    std::vector<u8> bytes(
        kBlockOffset + kVariantTableSize + kVariant0.size() + kVariant2.size() +
            kVariant4.size(),
        0U
    );

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
        kName, bytes.begin() + static_cast<std::ptrdiff_t>(kIndexOffset)
    );
    const u32 block_size = static_cast<u32>(bytes.size() - kBlockOffset);
    write_u32(bytes, kIndexOffset + 0x14U, block_size);
    write_u32(bytes, kIndexOffset + 0x18U, static_cast<u32>(kBlockOffset));
    write_u32(bytes, kIndexOffset + 0x1CU, 0x10203040U);
    write_u32(bytes, kIndexOffset + 0x20U, 0x50607080U);
    write_u32(bytes, kIndexOffset + 0x24U, 0x90A0B0C0U);
    write_u32(bytes, kIndexOffset + 0x28U, 0xD0E0F000U);

    write_u16(bytes, kBlockOffset, 5U);
    constexpr u32 kVariant0Offset = static_cast<u32>(kVariantTableSize);
    constexpr u32 kVariant2Offset = kVariant0Offset + kVariant0.size();
    constexpr u32 kVariant4Offset = kVariant2Offset + kVariant2.size();
    write_u32(bytes, kBlockOffset + 2U, kVariant0Offset);
    write_u32(bytes, kBlockOffset + 6U, 0U);
    write_u32(bytes, kBlockOffset + 10U, kVariant2Offset);
    write_u32(bytes, kBlockOffset + 14U, 0U);
    write_u32(bytes, kBlockOffset + 18U, kVariant4Offset);

    std::size_t cursor = kBlockOffset + kVariantTableSize;
    std::ranges::copy(
        kVariant0, bytes.begin() + static_cast<std::ptrdiff_t>(cursor)
    );
    cursor += kVariant0.size();
    std::ranges::copy(
        kVariant2, bytes.begin() + static_cast<std::ptrdiff_t>(cursor)
    );
    cursor += kVariant2.size();
    std::ranges::copy(
        kVariant4, bytes.begin() + static_cast<std::ptrdiff_t>(cursor)
    );
    return bytes;
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) noexcept {
    std::uint64_t value = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        value ^= byte;
        value *= 0x100000001B3ULL;
    }
    return value;
}

void test_synthetic_variant_selection(openswd3::test::Context& test) {
    const TestTree tree;
    const std::vector<u8> bytes = synthetic_archive();
    tree.write("synthetic.act", bytes);

    LegacyActArchive archive;
    test.expect_equal(
        archive.open(tree.path("synthetic.act")),
        LegacyActOpenStatus::ready,
        "synthetic ACT opens"
    );
    const auto index = archive.read_index(1U);
    test.expect_equal(
        index.status, LegacyActVariantStatus::ready, "44-byte index reads"
    );
    test.expect_equal(
        index.index.block_size,
        static_cast<u32>(bytes.size() - kBlockOffset),
        "index +14 is block size"
    );
    test.expect_equal(
        index.index.block_offset,
        static_cast<u32>(kBlockOffset),
        "index +18 is block offset"
    );
    test.expect_equal(
        index.index.metadata_id, 0x10203040U, "unused metadata is preserved"
    );
    test.expect_equal(
        index.index.field_28, 0xD0E0F000U, "last unused field is preserved"
    );

    const auto variant0 = archive.read_variant(index.index, 0U);
    test.expect_equal(
        variant0.status, LegacyActVariantStatus::ready, "variant zero loads"
    );
    test.expect_true(
        std::ranges::equal(variant0.variant.stream, kVariant0),
        "zero hole is skipped to variant two"
    );
    test.expect_equal(
        variant0.variant.variant_count,
        u16{5U},
        "variant count is little endian"
    );

    const auto variant2 = archive.read_variant(1U, 2U);
    test.expect_equal(
        variant2.status, LegacyActVariantStatus::ready, "middle variant loads"
    );
    test.expect_true(
        std::ranges::equal(variant2.variant.stream, kVariant2),
        "second zero hole is skipped"
    );

    const auto variant4 = archive.read_variant(1U, 4U);
    test.expect_equal(
        variant4.status, LegacyActVariantStatus::ready, "last variant loads"
    );
    test.expect_true(
        std::ranges::equal(variant4.variant.stream, kVariant4),
        "last variant ends at block size"
    );

    test.expect_equal(
        archive.read_variant(1U, 1U).status,
        LegacyActVariantStatus::variant_absent,
        "zero offset is an absent variant"
    );
    test.expect_equal(
        archive.read_variant(1U, 5U).status,
        LegacyActVariantStatus::variant_out_of_range,
        "variant count boundary is isolated"
    );
    test.expect_equal(
        archive.read_variant(1U, 0xFFFFFFFFU).status,
        LegacyActVariantStatus::variant_out_of_range,
        "negative signed variant is isolated safely"
    );
    archive.close();
}

void test_corrupt_boundaries(openswd3::test::Context& test) {
    const TestTree tree;
    LegacyActArchive archive;
    test.expect_equal(
        archive.open(tree.path("missing.act")),
        LegacyActOpenStatus::file_open_failed,
        "missing ACT fails open"
    );

    std::vector<u8> bytes = synthetic_archive();
    tree.write("base.act", bytes);
    static_cast<void>(archive.open(tree.path("base.act")));
    test.expect_equal(
        archive.read_index(0U).status,
        LegacyActVariantStatus::physical_record_out_of_range,
        "record zero underflow is isolated"
    );
    test.expect_equal(
        archive.read_index(3001U).status,
        LegacyActVariantStatus::physical_record_out_of_range,
        "record beyond 3000 is isolated"
    );
    test.expect_equal(
        archive.read_variant(2U, 0U).status,
        LegacyActVariantStatus::empty_index_record,
        "empty physical slot is explicit"
    );
    archive.close();

    write_u16(bytes, kBlockOffset, 0xFFFFU);
    tree.write("bad-table.act", bytes);
    static_cast<void>(archive.open(tree.path("bad-table.act")));
    test.expect_equal(
        archive.read_variant(1U, 0U).status,
        LegacyActVariantStatus::invalid_variant_table,
        "variant table must fit the declared block"
    );
    archive.close();

    bytes = synthetic_archive();
    write_u32(bytes, kBlockOffset + 18U, 0U);
    tree.write("no-following-offset.act", bytes);
    static_cast<void>(archive.open(tree.path("no-following-offset.act")));
    test.expect_equal(
        archive.read_variant(1U, 2U).status,
        LegacyActVariantStatus::following_variant_offset_not_found,
        "unsafe original scan is bounded only for corrupt input"
    );
    archive.close();

    bytes = synthetic_archive();
    write_u32(bytes, kBlockOffset + 2U, 1U);
    tree.write("bad-slice.act", bytes);
    static_cast<void>(archive.open(tree.path("bad-slice.act")));
    test.expect_equal(
        archive.read_variant(1U, 0U).status,
        LegacyActVariantStatus::variant_slice_out_of_block_range,
        "slice cannot point into its offset table"
    );
}

struct RealVariantExpectation {
    const char* archive_name;
    u32 variant_index;
    u32 stream_size;
    std::uint64_t fnv1a;
};

constexpr std::array<RealVariantExpectation, 6> kRealVariants{{
    {"all_char.act", 0U, 34U, 0xBB9BBD44F3828A30ULL},
    {"all_item.act", 68U, 22U, 0x755674BCC46200FFULL},
    {"all_magic.act", 0U, 290U, 0x5A0004140F826C9AULL},
    {"all_sys.act", 0U, 128U, 0xC33DE1B7B0689453ULL},
    {"all_map1.act", 0U, 60U, 0x23BC43E251C05AABULL},
    {"all_map2.act", 0U, 22U, 0xCAF93AB8E20E6F53ULL},
}};

void test_real_archives(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    for (const RealVariantExpectation& expected : kRealVariants) {
        LegacyActArchive archive;
        test.expect_equal(
            archive.open(root / expected.archive_name),
            LegacyActOpenStatus::ready,
            "real ACT archive opens"
        );
        if (!archive.is_open()) {
            continue;
        }
        const auto loaded = archive.read_variant(1U, expected.variant_index);
        test.expect_equal(
            loaded.status,
            LegacyActVariantStatus::ready,
            "real ACT variant loads"
        );
        if (loaded.status != LegacyActVariantStatus::ready) {
            continue;
        }
        test.expect_equal(
            loaded.variant.stream.size(),
            static_cast<std::size_t>(expected.stream_size),
            "real selected stream size"
        );
        test.expect_equal(
            fnv1a64(loaded.variant.stream),
            expected.fnv1a,
            "real selected bytes match evidence"
        );
        test.expect_true(
            (loaded.variant.stream.size() % 2U) == 0U,
            "action stream is a 16-bit word sequence"
        );
        archive.close();
    }
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_synthetic_variant_selection(test);
    test_corrupt_boundaries(test);
    if (argument_count == 2) {
        test_real_archives(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
