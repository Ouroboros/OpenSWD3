#include "test.hpp"

#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"
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

using openswd3::asset_runtime::LegacyTswDirectResult;
using openswd3::asset_runtime::LegacyTswFrameStatus;
using openswd3::asset_runtime::LegacyTswQueryResult;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::asset_runtime::LegacyTswRuntimeFrame;
using openswd3::asset_runtime::LegacyTswRuntimeStatus;
using openswd3::asset_runtime::LegacyTswSpecialFrameLoader;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kBlockOffset = kIndexOffset + kRecordSize * kSlotCount;
constexpr std::size_t kPaletteSize = 512U;

constexpr std::array<const char*, 6> kArchiveNames{
    "all_char.tsw",
    "all_item.tsw",
    "all_magic.tsw",
    "all_sys.tsw",
    "all_map1.tsw",
    "all_map2.tsw",
};

constexpr std::array<u8, 38> kStream8{
    0xFFU, 0xFFU, 0x06U, 0x00U, 0x02U, 0x00U, 0x08U, 0x00U, 0x0CU, 0x80U,
    0x02U, 0x80U, 0x02U, 0x00U, 0x02U, 0x04U, 0x02U, 0xC0U, 0x00U, 0x00U,
    0x10U, 0x80U, 0x01U, 0x00U, 0x05U, 0x01U, 0x80U, 0x01U, 0xC0U, 0x03U,
    0x00U, 0x06U, 0x07U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U,
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
            ("legacy-tsw-runtime-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream output{root_ / name, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

[[nodiscard]] std::array<u16, 256> synthetic_palette_words() noexcept {
    std::array<u16, 256> palette{};
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        palette[index] = static_cast<u16>((index * 97U) & 0x7FFFU);
    }
    return palette;
}

[[nodiscard]] std::vector<u8> synthetic_archive() {
    std::vector<u8> compressed(kStream8.size() + kStream8.size() / 16U + 67U);
    const auto compression =
        openswd3::resource_io::compress_legacy_lzo1x_14(kStream8, compressed);
    compressed.resize(compression.bytes_written);

    constexpr std::size_t descriptor_offset = kBlockOffset + 12U + kPaletteSize;
    constexpr std::size_t payload_offset = descriptor_offset + 36U;
    std::vector<u8> bytes(payload_offset + compressed.size(), 0U);
    const u32 block_size = static_cast<u32>(bytes.size() - kBlockOffset);

    for (std::size_t record = 0U; record < 10U; ++record) {
        const std::size_t offset = kIndexOffset + record * kRecordSize;
        write_u32(bytes, offset + 0x14U, block_size);
        write_u32(bytes, offset + 0x18U, static_cast<u32>(kBlockOffset));
        write_u32(bytes, offset + 0x1CU, static_cast<u32>(record + 1U));
    }

    write_u32(bytes, kBlockOffset, 0x12345678U);
    write_u16(bytes, kBlockOffset + 4U, 0xABCDU);
    write_u16(bytes, kBlockOffset + 6U, 1U);
    write_u16(bytes, kBlockOffset + 8U, 8U);
    write_u16(bytes, kBlockOffset + 10U, 12U);
    const std::array<u16, 256> palette = synthetic_palette_words();
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        write_u16(bytes, kBlockOffset + 12U + index * 2U, palette[index]);
    }

    write_u32(
        bytes,
        descriptor_offset + 0x00U,
        static_cast<u32>(payload_offset - kBlockOffset)
    );
    write_u32(
        bytes, descriptor_offset + 0x04U, static_cast<u32>(compressed.size())
    );
    write_u32(
        bytes, descriptor_offset + 0x08U, static_cast<u32>(kStream8.size())
    );
    write_u16(bytes, descriptor_offset + 0x20U, 47U);
    write_u16(bytes, descriptor_offset + 0x22U, 95U);
    std::ranges::copy(
        compressed, bytes.begin() + static_cast<std::ptrdiff_t>(payload_offset)
    );
    return bytes;
}

void write_six_archives(const TestTree& tree) {
    const std::vector<u8> bytes = synthetic_archive();
    for (const char* const name : kArchiveNames) {
        tree.write(name, bytes);
    }
}

class FakeSpecialLoader final : public LegacyTswSpecialFrameLoader {
public:
    [[nodiscard]] bool load_special_frame(
        const u16 variant_index, LegacyTswRuntimeFrame& frame
    ) override {
        ++calls;
        if (fail) {
            return false;
        }
        frame.primary_stream = {
            static_cast<u8>(variant_index),
            static_cast<u8>(variant_index >> 8U),
            0xAAU,
            0x55U,
        };
        frame.width = static_cast<u16>(variant_index + 1U);
        frame.height = 2U;
        return true;
    }

    std::size_t calls{};
    bool fail{};
};

void test_lazy_open_route_and_conversion(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyTswRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00400000U);
    test.expect_false(runtime.is_initialized(), "six TSW files open lazily");
    test.expect_equal(
        runtime.find_cached(1U, 0U).status,
        LegacyTswRuntimeStatus::cache_miss,
        "cache-only lookup does not initialize or load"
    );
    test.expect_false(
        runtime.is_initialized(), "cache-only miss preserves lazy state"
    );

    const LegacyTswQueryResult first = runtime.query_cached(1U, 0U);
    test.expect_equal(
        first.status, LegacyTswRuntimeStatus::ready, "first cached query loads"
    );
    test.expect_true(
        runtime.is_initialized(), "first query opens all archives"
    );
    test.expect_false(first.cache_hit, "first query is a miss");
    test.expect_true(
        first.frame.auxiliary_stream.empty(),
        "physical TSW auxiliary pointer stays null"
    );
    test.expect_true(
        first.frame.palette.empty(), "sub_401C70 releases the external palette"
    );
    test.expect_equal(first.frame.width, u16{47U}, "descriptor width is kept");
    test.expect_equal(
        first.frame.height, u16{95U}, "descriptor height is kept"
    );

    const auto expected =
        openswd3::rendering::convert_legacy_image_command_stream(
            kStream8,
            synthetic_palette_words(),
            openswd3::rendering::LegacyPixelConversionState{}
        );
    test.expect_equal(
        expected.status,
        openswd3::rendering::LegacyImageCommandStreamStatus::completed,
        "fixture conversion succeeds"
    );
    test.expect_true(
        std::ranges::equal(first.frame.primary_stream, expected.bytes),
        "cached frame is the exact converted command stream"
    );
    test.expect_equal(
        runtime.cached_primary_bytes(),
        static_cast<u32>(expected.bytes.size()),
        "cache counts converted primary bytes only"
    );

    const u8* const first_pointer = first.frame.primary_stream.data();
    const LegacyTswQueryResult repeated = runtime.query_cached(1U, 0U);
    test.expect_equal(
        repeated.status, LegacyTswRuntimeStatus::ready, "repeat query succeeds"
    );
    test.expect_true(repeated.cache_hit, "repeat query hits cache");
    test.expect_true(
        repeated.frame.primary_stream.data() == first_pointer,
        "cache returns a borrowed stable view"
    );

    const LegacyTswQueryResult truncated =
        runtime.query_cached(0x00010001U, 0x00010000U);
    test.expect_true(
        truncated.cache_hit, "both four-byte ABI slots truncate to low 16 bits"
    );
    test.expect_true(
        runtime.find_cached(0x00010001U, 0x00010000U).cache_hit,
        "sub_431A20-style cache-only lookup returns the view"
    );

    const LegacyTswDirectResult second_archive = runtime.load_direct(3001U, 0U);
    test.expect_equal(
        second_archive.status,
        LegacyTswRuntimeStatus::ready,
        "resource quotient selects the second archive"
    );
    test.expect_equal(
        runtime.cache_entry_count(),
        std::size_t{1U},
        "direct load does not enter the cache"
    );

    const LegacyTswDirectResult record_zero = runtime.load_direct(3000U, 0U);
    test.expect_equal(
        record_zero.status,
        LegacyTswRuntimeStatus::physical_frame_failed,
        "physical remainder zero is safely isolated"
    );
    test.expect_equal(
        record_zero.physical_status,
        LegacyTswFrameStatus::physical_record_out_of_range,
        "remainder zero retains the physical failure reason"
    );
    test.expect_equal(
        runtime.load_direct(18001U, 0U).status,
        LegacyTswRuntimeStatus::resource_group_out_of_range,
        "group beyond the six-handle table is isolated"
    );

    runtime.close();
    test.expect_false(runtime.is_initialized(), "close resets lazy state");
    test.expect_equal(
        runtime.cache_entry_count(),
        std::size_t{0U},
        "close clears cached ownership"
    );
}

void test_special_resource_and_failures(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyTswRuntime unavailable{tree.root()};
    test.expect_equal(
        unavailable.query_cached(0xFFFFU, 13U).status,
        LegacyTswRuntimeStatus::special_loader_unavailable,
        "resource FFFF reaches the dedicated loader port"
    );
    unavailable.close();

    FakeSpecialLoader loader;
    LegacyTswRuntime runtime{tree.root(), {}, &loader};
    runtime.set_cache_limit(0x1000U);
    const LegacyTswQueryResult first = runtime.query_cached(0xFFFFU, 13U);
    test.expect_equal(
        first.status,
        LegacyTswRuntimeStatus::ready,
        "special frame loads through port"
    );
    test.expect_equal(loader.calls, std::size_t{1U}, "special loader called");
    test.expect_equal(
        runtime.bucket_entry_count(3U),
        std::size_t{1U},
        "FFFF cache bucket is variant modulo ten"
    );
    test.expect_equal(first.frame.width, u16{14U}, "special frame is borrowed");

    const LegacyTswQueryResult repeated =
        runtime.query_cached(0x1FFFFU, 0x1000DU);
    test.expect_true(
        repeated.cache_hit, "special key also truncates both slots"
    );
    test.expect_equal(
        loader.calls, std::size_t{1U}, "special cache hit skips loader"
    );

    loader.fail = true;
    test.expect_equal(
        runtime.load_direct(0xFFFFU, 14U).status,
        LegacyTswRuntimeStatus::special_frame_load_failed,
        "special loader failure remains explicit"
    );
    runtime.close();

    LegacyTswRuntime missing{tree.root() / "missing"};
    test.expect_equal(
        missing.query_cached(1U, 0U).status,
        LegacyTswRuntimeStatus::archive_open_failed,
        "missing six-file set fails initialization"
    );
    test.expect_false(
        missing.is_initialized(), "failed initialization closes partial state"
    );
}

void populate_eviction_shape(
    LegacyTswRuntime& runtime,
    FakeSpecialLoader& loader,
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 7> kVariants{0U, 10U, 20U, 1U, 11U, 2U, 12U};
    for (const u16 variant : kVariants) {
        const auto loaded = runtime.query_cached(0xFFFFU, variant);
        test.expect_equal(
            loaded.status,
            LegacyTswRuntimeStatus::ready,
            "eviction fixture frame loads"
        );
    }
    test.expect_equal(
        loader.calls, std::size_t{7U}, "fixture has seven distinct keys"
    );
    test.expect_equal(
        runtime.cached_primary_bytes(),
        28U,
        "fixture counts four bytes per special frame"
    );
    test.expect_equal(
        runtime.bucket_entry_count(0U),
        std::size_t{3U},
        "bucket zero starts longest"
    );
    test.expect_equal(
        runtime.bucket_entry_count(1U), std::size_t{2U}, "bucket one count"
    );
    test.expect_equal(
        runtime.bucket_entry_count(2U), std::size_t{2U}, "bucket two count"
    );
}

void test_original_bucket_eviction(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    FakeSpecialLoader first_loader;
    LegacyTswRuntime before_hit{tree.root(), {}, &first_loader};
    before_hit.set_cache_limit(0x1000U);
    populate_eviction_shape(before_hit, first_loader, test);
    before_hit.set_cache_limit(28U);
    const auto reloaded_tail = before_hit.query_cached(0xFFFFU, 0U);
    test.expect_false(
        reloaded_tail.cache_hit,
        "eviction runs before lookup and removes the LRU tail"
    );
    test.expect_equal(
        first_loader.calls,
        std::size_t{8U},
        "evicted requested frame is loaded again"
    );
    test.expect_equal(
        before_hit.cached_primary_bytes(),
        28U,
        "reload returns total to the limit"
    );
    before_hit.close();

    FakeSpecialLoader second_loader;
    LegacyTswRuntime one_bucket{tree.root(), {}, &second_loader};
    one_bucket.set_cache_limit(0x1000U);
    populate_eviction_shape(one_bucket, second_loader, test);
    one_bucket.set_cache_limit(12U);
    const auto surviving_hit = one_bucket.query_cached(0xFFFFU, 1U);
    test.expect_true(
        surviving_hit.cache_hit,
        "query hits a non-selected bucket after eviction"
    );
    test.expect_equal(
        one_bucket.bucket_entry_count(0U),
        std::size_t{0U},
        "selected longest bucket is drained continuously"
    );
    test.expect_equal(
        one_bucket.bucket_entry_count(1U),
        std::size_t{2U},
        "eviction does not recompute a new longest bucket"
    );
    test.expect_equal(
        one_bucket.bucket_entry_count(2U),
        std::size_t{2U},
        "second non-selected bucket also survives"
    );
    test.expect_equal(
        one_bucket.cached_primary_bytes(),
        16U,
        "empty selected bucket can leave total above limit"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_lazy_open_route_and_conversion(test);
    test_special_resource_and_failures(test);
    test_original_bucket_eviction(test);
    return test.exit_code();
}
