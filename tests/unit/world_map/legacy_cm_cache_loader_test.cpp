#include "test.hpp"

#include "openswd3/resource_io/legacy_lzo1x.hpp"
#include "openswd3/world_map/legacy_cm_cache_loader.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::select_legacy_pixel_conversion;
using openswd3::resource_io::compress_legacy_lzo1x_15;
using openswd3::world_map::kLegacyCmCacheInvalidMap;
using openswd3::world_map::LegacyCmCacheLoadStatus;
using openswd3::world_map::LegacyCmCacheRequest;
using openswd3::world_map::load_legacy_cm_cache;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-cm-loader-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(
        const std::filesystem::path& relative_path,
        const std::span<const u8> bytes
    ) const {
        std::ofstream output{
            root_ / relative_path, std::ios::binary | std::ios::trunc
        };
        if (!bytes.empty()) {
            output.write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size())
            );
        }
    }

    [[nodiscard]] std::vector<u8>
    read(const std::filesystem::path& relative_path) const {
        std::ifstream input{root_ / relative_path, std::ios::binary};
        return std::vector<u8>{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };
    }

private:
    std::filesystem::path root_;
};

void append_u32(std::vector<u8>& bytes, const u32 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
    bytes.push_back(static_cast<u8>(value >> 16U));
    bytes.push_back(static_cast<u8>(value >> 24U));
}

void append_record(
    std::vector<u8>& bytes,
    const u32 map_id,
    const u32 byte_size,
    const u32 use_counter,
    const u32 stored_slot
) {
    append_u32(bytes, map_id);
    append_u32(bytes, byte_size);
    append_u32(bytes, use_counter);
    append_u32(bytes, stored_slot);
}

[[nodiscard]] u32
read_u32(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> make_archive(
    const std::span<const u8> raw, const u32 total_size, const u32 chunk_size
) {
    std::vector<u8> compressed(raw.size() * 2U + 128U);
    const auto compression = compress_legacy_lzo1x_15(raw, compressed);
    compressed.resize(compression.bytes_written);

    constexpr std::size_t header_offset = 0x60U;
    std::vector<u8> archive(header_offset + 0x1A8U + compressed.size());
    write_u32(archive, header_offset + 0x10U, total_size);
    write_u32(archive, header_offset + 0x14U, chunk_size);
    write_u32(
        archive, header_offset + 0x1CU, static_cast<u32>(compressed.size())
    );
    std::ranges::copy(
        compressed,
        archive.begin() + static_cast<std::ptrdiff_t>(header_offset + 0x1A8U)
    );
    return archive;
}

[[nodiscard]] LegacyPixelConversionState rgb565_conversion() {
    LegacyPixelConversionState state;
    select_legacy_pixel_conversion(state, {0xF800U, 0x07E0U, 0x001FU});
    return state;
}

[[nodiscard]] LegacyCmCacheRequest
request_for(const TestTree& tree, const u32 map_id) {
    return LegacyCmCacheRequest{
        .archive_path = tree.root() / "huge.lmf",
        .cache_directory = tree.root(),
        .map_id = map_id,
        .map_offset = 0x20U,
        .cm_relative_offset = 0x40U,
        .cache_limit_megabytes = 60U,
        .map_pixel_bits = 8U,
        .pixel_conversion = rgb565_conversion(),
    };
}

void test_empty_directory_initialization(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        1U,
        2U,
        3U,
        4U,
        5U,
        6U,
        7U,
        8U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));

    const auto result = load_legacy_cm_cache(request_for(tree, 24U));
    test.expect_true(
        result.status == LegacyCmCacheLoadStatus::ready_generated &&
            result.initialized_empty_directory && result.selected_slot == 0U &&
            result.index_persisted && result.index_truncated &&
            result.records.size() == 24U,
        "empty mcache follows the fixed 24-record initialization branch"
    );
    test.expect_equal(
        result.cache_bytes,
        std::vector<u8>{1U, 2U, 3U, 4U, 5U, 6U},
        "empty directory branch returns the generated slot zero bytes"
    );

    const std::vector<u8> index = tree.read("mcache.dat");
    test.expect_true(
        index.size() == 24U * 16U && read_u32(index, 0U) == 24U &&
            read_u32(index, 4U) == 6U && read_u32(index, 12U) == 0U &&
            read_u32(index, 16U) == kLegacyCmCacheInvalidMap &&
            read_u32(index, 28U) == 1U,
        "initialized directory stores map zero followed by indexed free slots"
    );
}

void test_hit_order_and_tail_preservation(openswd3::test::Context& test) {
    const TestTree tree;
    std::vector<u8> index;
    append_record(index, 24U, 4U, 7U, 3U);
    append_record(index, 81U, 2U, 9U, 1U);
    index.insert(index.end(), {0xAAU, 0xBBU, 0xCCU});
    tree.write("mcache.dat", index);
    constexpr std::array<u8, 4> cache{1U, 2U, 3U, 4U};
    tree.write("3.cm", cache);

    const auto result = load_legacy_cm_cache(request_for(tree, 24U));
    test.expect_true(
        result.status == LegacyCmCacheLoadStatus::ready_hit &&
            result.selected_slot == 3U && result.index_persisted &&
            !result.index_truncated,
        "hit loads the stored slot before rewriting the directory"
    );
    const std::vector<u8> rewritten = tree.read("mcache.dat");
    test.expect_true(
        rewritten.size() == index.size() && read_u32(rewritten, 8U) == 0U &&
            read_u32(rewritten, 0x18U) == 10U && rewritten[0x20U] == 0xAAU &&
            rewritten[0x22U] == 0xCCU,
        "hit persists ages without truncating a modulo-16 tail"
    );
}

void test_hit_creates_missing_slot_before_empty_failure(
    openswd3::test::Context& test
) {
    const TestTree tree;
    std::vector<u8> index;
    append_record(index, 24U, 4U, 7U, 3U);
    tree.write("mcache.dat", index);

    const auto result = load_legacy_cm_cache(request_for(tree, 24U));
    test.expect_true(
        result.status == LegacyCmCacheLoadStatus::cache_file_empty &&
            result.index_persisted &&
            std::filesystem::is_regular_file(tree.root() / "3.cm") &&
            std::filesystem::file_size(tree.root() / "3.cm") == 0U,
        "cache hit uses OPEN_ALWAYS before an empty mapping fails"
    );
    test.expect_equal(
        read_u32(tree.read("mcache.dat"), 8U),
        u32{0U},
        "empty hit still persists the reset use counter"
    );
}

void test_miss_inserts_and_truncates_index(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        1U,
        2U,
        3U,
        4U,
        5U,
        6U,
        7U,
        8U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));
    std::vector<u8> index;
    append_record(index, 10U, 3U, 4U, 0U);
    append_record(index, kLegacyCmCacheInvalidMap, 0U, 8U, 1U);
    index.insert(index.end(), {0xAAU, 0xBBU});
    tree.write("mcache.dat", index);

    const auto result = load_legacy_cm_cache(request_for(tree, 24U));
    test.expect_true(
        result.status == LegacyCmCacheLoadStatus::ready_generated &&
            result.selected_slot == 1U && result.evictions.empty() &&
            result.index_persisted && result.index_truncated,
        "miss uses the first free record before generation"
    );
    const std::vector<u8> rewritten = tree.read("mcache.dat");
    test.expect_true(
        rewritten.size() == 32U && read_u32(rewritten, 8U) == 5U &&
            read_u32(rewritten, 16U) == 24U && read_u32(rewritten, 20U) == 6U &&
            read_u32(rewritten, 28U) == 1U,
        "miss removes the old index tail and persists the inserted record"
    );
}

void test_eviction_truncates_slot_to_sixteen_bytes(
    openswd3::test::Context& test
) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        1U,
        2U,
        3U,
        4U,
        5U,
        6U,
        7U,
        8U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));
    std::vector<u8> index;
    append_record(index, 10U, 600'000U, 0U, 0U);
    append_record(index, 11U, 600'000U, 0U, 1U);
    tree.write("mcache.dat", index);
    constexpr std::array<u8, 20> old_slot{
        0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU,
        0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU,
    };
    tree.write("0.cm", old_slot);

    LegacyCmCacheRequest request = request_for(tree, 24U);
    request.cache_limit_megabytes = 1U;
    const auto result = load_legacy_cm_cache(request);
    test.expect_true(
        result.status == LegacyCmCacheLoadStatus::ready_generated &&
            result.evictions.size() == 1U &&
            result.evictions[0].record_index == 0U &&
            result.selected_slot == 0U,
        "capacity miss evicts the first record older than limit/4"
    );
    test.expect_true(
        result.cache_bytes.size() == 16U &&
            std::ranges::equal(
                std::span<const u8>{result.cache_bytes}.first(6U),
                std::array<u8, 6>{1U, 2U, 3U, 4U, 5U, 6U}
            ) &&
            result.cache_bytes[6U] == 0xAAU && result.cache_bytes[15U] == 0xAAU,
        "eviction truncates at 0x10 and generation leaves its unwritten tail"
    );
}

void test_failed_size_probe_leaves_index_unwritten(
    openswd3::test::Context& test
) {
    const TestTree tree;
    std::vector<u8> index;
    append_record(index, 10U, 3U, 4U, 0U);
    tree.write("mcache.dat", index);

    LegacyCmCacheRequest request = request_for(tree, 24U);
    request.cm_relative_offset = 0U;
    const auto result = load_legacy_cm_cache(request);
    test.expect_true(
        result.status == LegacyCmCacheLoadStatus::declared_size_failed &&
            !result.index_persisted,
        "zero CM offset exits the size helper before the miss is committed"
    );
    test.expect_equal(
        read_u32(tree.read("mcache.dat"), 8U),
        u32{4U},
        "uncommitted miss does not persist its in-memory age increment"
    );
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        hash ^= byte;
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

void test_current_map_24(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
) {
    const TestTree tree;
    LegacyCmCacheRequest request{
        .archive_path = archive_path,
        .cache_directory = tree.root(),
        .map_id = 24U,
        .map_offset = 0x026698A3U,
        .cm_relative_offset = 0x00049193U,
        .cache_limit_megabytes = 60U,
        .map_pixel_bits = 16U,
        .pixel_conversion = rgb565_conversion(),
    };
    const auto generated = load_legacy_cm_cache(request);
    test.expect_true(
        generated.status == LegacyCmCacheLoadStatus::ready_generated &&
            generated.cache_bytes.size() == 3'706'880U &&
            fnv1a64(generated.cache_bytes) == 0x9923E29AAAA434EEULL,
        "map 24 completes empty-directory generation and mapped-byte load"
    );

    const auto hit = load_legacy_cm_cache(request);
    test.expect_true(
        hit.status == LegacyCmCacheLoadStatus::ready_hit &&
            hit.selected_slot == 0U && hit.cache_bytes == generated.cache_bytes,
        "second map 24 request follows the persisted hit path"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_empty_directory_initialization(test);
    test_hit_order_and_tail_preservation(test);
    test_hit_creates_missing_slot_before_empty_failure(test);
    test_miss_inserts_and_truncates_index(test);
    test_eviction_truncates_slot_to_sixteen_bytes(test);
    test_failed_size_probe_leaves_index_unwritten(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr &&
        arguments[1] != nullptr) {
        test_current_map_24(test, arguments[1]);
    }
    return test.exit_code();
}
