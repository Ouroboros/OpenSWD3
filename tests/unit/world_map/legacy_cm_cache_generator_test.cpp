#include "test.hpp"

#include "openswd3/resource_io/legacy_lzo1x.hpp"
#include "openswd3/world_map/legacy_cm_cache_generator.hpp"

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
using openswd3::world_map::generate_legacy_cm_cache_unit;
using openswd3::world_map::LegacyCmCacheGenerationStatus;
using openswd3::world_map::LegacyCmCacheSizeStatus;
using openswd3::world_map::read_legacy_cm_cache_declared_size;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-cm-generator-" + std::to_string(unique_value));
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
            root_ / relative_path,
            std::ios::binary | std::ios::trunc
        };
        if (!bytes.empty()) {
            output.write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size())
            );
        }
    }

    [[nodiscard]] std::vector<u8> read(
        const std::filesystem::path& relative_path
    ) const {
        std::ifstream input{root_ / relative_path, std::ios::binary};
        return std::vector<u8>{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };
    }

private:
    std::filesystem::path root_;
};

void write_u32(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::vector<u8> make_archive(
    const std::span<const u8> raw,
    const u32 total_size,
    const u32 chunk_size
) {
    std::vector<u8> compressed(raw.size() * 2U + 128U);
    const auto compression = compress_legacy_lzo1x_15(raw, compressed);
    compressed.resize(compression.bytes_written);

    constexpr std::size_t kHeaderOffset = 0x60U;
    std::vector<u8> archive(kHeaderOffset + 0x1A8U + compressed.size());
    const std::span<u8> bytes{archive};
    write_u32(bytes, kHeaderOffset + 0x10U, total_size);
    write_u32(bytes, kHeaderOffset + 0x14U, chunk_size);
    write_u32(
        bytes,
        kHeaderOffset + 0x1CU,
        static_cast<u32>(compressed.size())
    );
    std::ranges::copy(
        compressed,
        archive.begin() + static_cast<std::ptrdiff_t>(kHeaderOffset + 0x1A8U)
    );
    return archive;
}

[[nodiscard]] LegacyPixelConversionState rgb565_conversion() {
    LegacyPixelConversionState state;
    select_legacy_pixel_conversion(state, {0xF800U, 0x07E0U, 0x001FU});
    return state;
}

void test_decompress_convert_and_discard(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        0x1FU, 0x00U,
        0xE0U, 0x03U,
        0x00U, 0x7CU,
        0x34U, 0x12U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));
    tree.write("0.cm", std::span<const u8>{});

    const auto result = generate_legacy_cm_cache_unit(
        tree.root() / "huge.lmf",
        0x20U,
        0x40U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion()
    );
    test.expect_true(
        result.status == LegacyCmCacheGenerationStatus::ready &&
            result.declared_output_size == 6U &&
            result.chunk_output_size == 8U &&
            result.chunk_count == 1U && result.completed_chunks == 1U &&
            result.cache_bytes_written == 6U &&
            result.decompressed_bytes_discarded == 2U,
        "single chunk retains the declared write prefix and discarded tail"
    );
    test.expect_equal(
        tree.read("0.cm"),
        std::vector<u8>{0x1FU, 0x00U, 0xC0U, 0x07U, 0x00U, 0xF8U},
        "16-bit maps convert only the caller-written RGB555 prefix"
    );
}

void test_non_16_bit_map_skips_conversion(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        0x1FU, 0x00U, 0xE0U, 0x03U, 0x00U, 0x7CU, 0x34U, 0x12U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));
    tree.write("0.cm", std::span<const u8>{});

    const auto result = generate_legacy_cm_cache_unit(
        tree.root() / "huge.lmf",
        0x20U,
        0x40U,
        tree.root() / "0.cm",
        8U,
        rgb565_conversion()
    );
    test.expect_equal(result.status, LegacyCmCacheGenerationStatus::ready,
                      "non-16-bit map still writes the CM prefix");
    test.expect_equal(
        tree.read("0.cm"),
        std::vector<u8>{0x1FU, 0x00U, 0xE0U, 0x03U, 0x00U, 0x7CU},
        "non-16-bit map bypasses the forward pixel converter"
    );
}

void test_size_probe_and_existing_tail(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        0x1FU, 0x00U, 0xE0U, 0x03U, 0x00U, 0x7CU, 0x34U, 0x12U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));

    const auto size = read_legacy_cm_cache_declared_size(
        tree.root() / "huge.lmf",
        0x20U,
        0x40U
    );
    test.expect_true(
        size.status == LegacyCmCacheSizeStatus::ready &&
            size.declared_output_size == 6U,
        "0x004270F0 reads only the declared CM output size"
    );

    tree.write(
        "0.cm",
        std::array<u8, 10>{
            0xAAU, 0xAAU, 0xAAU, 0xAAU, 0xAAU,
            0xAAU, 0xEEU, 0xEEU, 0xEEU, 0xEEU,
        }
    );
    const auto generated = generate_legacy_cm_cache_unit(
        tree.root() / "huge.lmf",
        0x20U,
        0x40U,
        tree.root() / "0.cm",
        8U,
        rgb565_conversion()
    );
    test.expect_equal(
        generated.status,
        LegacyCmCacheGenerationStatus::ready,
        "existing cache slot accepts overwrite generation"
    );
    test.expect_equal(
        tree.read("0.cm"),
        std::vector<u8>{
            0x1FU, 0x00U, 0xE0U, 0x03U, 0x00U,
            0x7CU, 0xEEU, 0xEEU, 0xEEU, 0xEEU,
        },
        "generator overwrites from offset zero without truncating the old tail"
    );
}

void test_early_failures_preserve_original_order(
    openswd3::test::Context& test
) {
    const TestTree tree;
    constexpr std::array<u8, 3> original{1U, 2U, 3U};
    tree.write("0.cm", original);
    const auto zero_offset = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        0U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion()
    );
    test.expect_true(
        zero_offset.status == LegacyCmCacheGenerationStatus::cm_offset_zero &&
            tree.read("0.cm") == std::vector<u8>{1U, 2U, 3U},
        "zero CM offset exits before opening and truncating the cache unit"
    );

    const auto missing_cache = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        1U,
        tree.root() / "missing.cm",
        16U,
        rgb565_conversion()
    );
    test.expect_true(
        missing_cache.status ==
            LegacyCmCacheGenerationStatus::archive_open_failed &&
            std::filesystem::is_regular_file(tree.root() / "missing.cm") &&
            tree.read("missing.cm").empty(),
        "OPEN_ALWAYS creates the cache slot before the source open fails"
    );

    const auto missing_archive = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        1U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion()
    );
    test.expect_true(
        missing_archive.status ==
            LegacyCmCacheGenerationStatus::archive_open_failed &&
            tree.read("0.cm") == std::vector<u8>{1U, 2U, 3U},
        "opening an existing cache slot does not truncate it"
    );

    const auto missing_parent = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        1U,
        tree.root() / "missing-parent" / "0.cm",
        16U,
        rgb565_conversion()
    );
    test.expect_equal(
        missing_parent.status,
        LegacyCmCacheGenerationStatus::cache_file_open_failed,
        "OPEN_ALWAYS still fails when the cache directory is absent"
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
    openswd3::test::Context& test,
    const std::filesystem::path& archive_path
) {
    const TestTree tree;
    tree.write("0.cm", std::span<const u8>{});
    const auto result = generate_legacy_cm_cache_unit(
        archive_path,
        0x026698A3U,
        0x00049193U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion()
    );
    const std::vector<u8> generated = tree.read("0.cm");
    test.expect_true(
        result.status == LegacyCmCacheGenerationStatus::ready &&
            result.chunk_output_size == 1'024'768U &&
            result.chunk_count == 4U && result.completed_chunks == 4U &&
            result.cache_bytes_written == 3'706'880U,
        "map 24 regenerates the complete current CM cache"
    );
    test.expect_equal(
        fnv1a64(generated),
        std::uint64_t{0x9923E29AAAA434EEULL},
        "map 24 RGB565 cache bytes match the recovered full output"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_decompress_convert_and_discard(test);
    test_non_16_bit_map_skips_conversion(test);
    test_size_probe_and_existing_tail(test);
    test_early_failures_preserve_original_order(test);

    test.expect_true(argument_count == 1 || argument_count == 2,
                     "optional argument names the current huge.lmf");
    if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
        test_current_map_24(test, arguments[1]);
    }
    return test.exit_code();
}
