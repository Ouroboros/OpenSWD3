#include "test.hpp"

#include "openswd3/asset_runtime/legacy_act_runtime.hpp"

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

using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyActRuntimeStatus;
using openswd3::asset_runtime::LegacyActVariantStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kIndexRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kBlockOffset =
    kIndexOffset + kIndexRecordSize * kSlotCount;

constexpr std::array<const char*, 6> kArchiveNames{
    "all_char.act",
    "all_item.act",
    "all_magic.act",
    "all_sys.act",
    "all_map1.act",
    "all_map2.act",
};

constexpr std::array<u8, 4> kVariant0{0x00U, 0x00U, 0x4EU, 0x54U};
constexpr std::array<u8, 4> kVariant2{0x46U, 0x52U, 0x71U, 0x01U};

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
            ("legacy-act-runtime-" + std::to_string(unique_value));
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

[[nodiscard]] std::vector<u8> synthetic_archive() {
    constexpr std::size_t kTableSize = 2U + 3U * 4U;
    std::vector<u8> bytes(
        kBlockOffset + kTableSize + kVariant0.size() + kVariant2.size(), 0U
    );
    const u32 block_size = static_cast<u32>(bytes.size() - kBlockOffset);
    for (std::size_t record = 0U; record < 30U; ++record) {
        const std::size_t index = kIndexOffset + record * kIndexRecordSize;
        write_u32(bytes, index + 0x14U, block_size);
        write_u32(bytes, index + 0x18U, static_cast<u32>(kBlockOffset));
        write_u32(bytes, index + 0x1CU, static_cast<u32>(record + 1U));
    }

    write_u16(bytes, kBlockOffset, 3U);
    write_u32(bytes, kBlockOffset + 2U, static_cast<u32>(kTableSize));
    write_u32(bytes, kBlockOffset + 6U, 0U);
    write_u32(
        bytes,
        kBlockOffset + 10U,
        static_cast<u32>(kTableSize + kVariant0.size())
    );
    std::ranges::copy(
        kVariant0,
        bytes.begin() + static_cast<std::ptrdiff_t>(kBlockOffset + kTableSize)
    );
    std::ranges::copy(
        kVariant2,
        bytes.begin() +
            static_cast<std::ptrdiff_t>(
                kBlockOffset + kTableSize + kVariant0.size()
            )
    );
    return bytes;
}

void write_six_archives(const TestTree& tree) {
    const std::vector<u8> bytes = synthetic_archive();
    for (const char* const name : kArchiveNames) {
        tree.write(name, bytes);
    }
}

void test_lazy_open_full_keys_and_two_caches(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyActRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00080000U);
    test.expect_false(runtime.is_initialized(), "ACT files open lazily");
    test.expect_equal(
        runtime.find_cached(1U, 0U).status,
        LegacyActRuntimeStatus::cache_miss,
        "cache-only lookup does not initialize"
    );

    const auto first = runtime.query_cached(1U, 0U);
    test.expect_equal(
        first.status,
        LegacyActRuntimeStatus::ready,
        "first query loads selected stream"
    );
    test.expect_false(first.cache_hit, "first query misses cache");
    test.expect_true(
        std::ranges::equal(first.stream, kVariant0),
        "runtime returns exact selected bytes"
    );
    test.expect_equal(
        runtime.cached_stream_bytes(), 4U, "cache counts stream bytes only"
    );
    test.expect_equal(
        runtime.index_cache_entry_count(),
        std::size_t{1U},
        "first block creates one index node"
    );

    const u8* const first_pointer = first.stream.data();
    const auto repeated = runtime.query_cached(1U, 0U);
    test.expect_true(repeated.cache_hit, "repeated full key hits cache");
    test.expect_true(
        repeated.stream.data() == first_pointer,
        "cached query returns a borrowed stable view"
    );

    const auto same_block_direct = runtime.load_direct(1U, 2U);
    test.expect_equal(
        same_block_direct.status,
        LegacyActRuntimeStatus::ready,
        "direct query loads another variant"
    );
    test.expect_true(
        std::ranges::equal(same_block_direct.stream, kVariant2),
        "direct query owns exact stream bytes"
    );
    test.expect_equal(
        runtime.index_cache_entry_count(),
        std::size_t{1U},
        "direct query reuses index metadata cache"
    );
    test.expect_equal(
        runtime.stream_cache_entry_count(),
        std::size_t{1U},
        "direct query does not enter stream cache"
    );

    const auto second_archive = runtime.load_direct(3001U, 0U);
    test.expect_equal(
        second_archive.status,
        LegacyActRuntimeStatus::ready,
        "quotient selects the second ACT archive"
    );
    test.expect_equal(
        runtime.index_cache_entry_count(),
        std::size_t{2U},
        "index key includes archive group"
    );

    const auto high_action = runtime.query_cached(0x00010001U, 0U);
    test.expect_false(
        high_action.cache_hit, "action ID cache key keeps all 32 bits"
    );
    test.expect_equal(
        high_action.status,
        LegacyActRuntimeStatus::action_id_out_of_range,
        "high action ID does not alias action one"
    );
    const auto high_variant = runtime.query_cached(1U, 0x00010000U);
    test.expect_false(
        high_variant.cache_hit, "variant cache key keeps all 32 bits"
    );
    test.expect_equal(
        high_variant.physical_status,
        LegacyActVariantStatus::variant_out_of_range,
        "high variant does not alias variant zero"
    );

    runtime.close();
    test.expect_false(runtime.is_initialized(), "close resets lazy state");
    test.expect_equal(
        runtime.stream_cache_entry_count(),
        std::size_t{0U},
        "close clears selected streams"
    );
    test.expect_equal(
        runtime.index_cache_entry_count(),
        std::size_t{0U},
        "close clears index metadata"
    );
}

void test_absent_stream_and_special_bucket(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyActRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00080000U);
    const auto absent = runtime.query_cached(1U, 1U);
    test.expect_equal(
        absent.status,
        LegacyActRuntimeStatus::physical_variant_failed,
        "zero offset returns no selected stream"
    );
    test.expect_equal(
        absent.physical_status,
        LegacyActVariantStatus::variant_absent,
        "zero offset reason is retained"
    );
    test.expect_false(absent.cache_hit, "first absent query is a miss");
    const auto repeated = runtime.query_cached(1U, 1U);
    test.expect_true(
        repeated.cache_hit, "original leaves failed selected-stream node cached"
    );
    test.expect_equal(
        runtime.cached_stream_bytes(),
        0U,
        "failed node contributes no stream bytes"
    );

    const auto special = runtime.query_cached(0xFFFFU, 3U);
    test.expect_equal(
        special.status,
        LegacyActRuntimeStatus::action_id_out_of_range,
        "ACT FFFF has no separate physical loader"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(3U),
        std::size_t{1U},
        "FFFF uses variant modulo ten for selected cache"
    );
    runtime.close();
}

void populate_eviction_shape(
    LegacyActRuntime& runtime, openswd3::test::Context& test
) {
    constexpr std::array<u32, 7> kActions{10U, 20U, 30U, 1U, 11U, 2U, 12U};
    for (const u32 action : kActions) {
        const auto loaded = runtime.query_cached(action, 2U);
        test.expect_equal(
            loaded.status,
            LegacyActRuntimeStatus::ready,
            "eviction fixture stream loads"
        );
    }
    test.expect_equal(
        runtime.cached_stream_bytes(),
        28U,
        "fixture counts seven four-byte streams"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(0U),
        std::size_t{3U},
        "bucket zero starts longest"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(1U),
        std::size_t{2U},
        "bucket one count"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(2U),
        std::size_t{2U},
        "bucket two count"
    );
}

void test_original_eviction_order(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyActRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00080000U);
    populate_eviction_shape(runtime, test);
    runtime.set_cache_limit(12U);
    const auto surviving = runtime.query_cached(1U, 2U);
    test.expect_true(
        surviving.cache_hit, "non-selected bucket is searched after eviction"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(0U),
        std::size_t{0U},
        "selected longest bucket is drained continuously"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(1U),
        std::size_t{2U},
        "eviction does not recompute the longest bucket"
    );
    test.expect_equal(
        runtime.stream_bucket_entry_count(2U),
        std::size_t{2U},
        "other tied bucket also survives"
    );
    test.expect_equal(
        runtime.cached_stream_bytes(),
        16U,
        "empty chosen bucket can leave total above limit"
    );
    runtime.close();

    LegacyActRuntime mru{tree.root()};
    mru.set_cache_limit(0x00080000U);
    static_cast<void>(mru.query_cached(1U, 2U));
    static_cast<void>(mru.query_cached(11U, 2U));
    test.expect_true(
        mru.query_cached(11U, 2U).cache_hit,
        "hit moves appended node to bucket head"
    );
    mru.set_cache_limit(8U);
    test.expect_true(
        mru.query_cached(11U, 2U).cache_hit,
        "tail eviction preserves the MRU head"
    );
    test.expect_equal(
        mru.find_cached(1U, 2U).status,
        LegacyActRuntimeStatus::cache_miss,
        "tail entry was removed"
    );
    mru.close();
}

void test_initialization_failure(openswd3::test::Context& test) {
    const TestTree tree;
    LegacyActRuntime runtime{tree.root() / "missing"};
    test.expect_equal(
        runtime.query_cached(1U, 0U).status,
        LegacyActRuntimeStatus::archive_open_failed,
        "missing six-file set fails initialization"
    );
    test.expect_false(
        runtime.is_initialized(), "failed initialization closes partial files"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_lazy_open_full_keys_and_two_caches(test);
    test_absent_stream_and_special_bucket(test);
    test_original_eviction_order(test);
    test_initialization_failure(test);
    return test.exit_code();
}
