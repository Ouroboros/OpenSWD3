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

using openswd3::compat::i32;
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

    constexpr std::size_t kHeaderOffset = 0x60U;
    std::vector<u8> archive(kHeaderOffset + 0x1A8U + compressed.size());
    const std::span<u8> bytes{archive};
    write_u32(bytes, kHeaderOffset + 0x10U, total_size);
    write_u32(bytes, kHeaderOffset + 0x14U, chunk_size);
    write_u32(
        bytes, kHeaderOffset + 0x1CU, static_cast<u32>(compressed.size())
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
        0x1FU,
        0x00U,
        0xE0U,
        0x03U,
        0x00U,
        0x7CU,
        0x34U,
        0x12U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));
    tree.write("0.cm", std::span<const u8>{});

    std::vector<std::string> service_order;
    const auto result = generate_legacy_cm_cache_unit(
        tree.root() / "huge.lmf",
        0x20U,
        0x40U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion(),
        [&](const i32 progress) {
            service_order.push_back("progress:" + std::to_string(progress));
        },
        [&] { service_order.emplace_back("audio"); }
    );
    test.expect_true(
        result.status == LegacyCmCacheGenerationStatus::ready &&
            result.declared_output_size == 6U &&
            result.chunk_output_size == 8U && result.chunk_count == 1U &&
            result.completed_chunks == 1U && result.cache_bytes_written == 6U &&
            result.decompressed_bytes_discarded == 2U,
        "single chunk retains the declared write prefix and discarded tail"
    );
    test.expect_equal(
        service_order,
        std::vector<std::string>{
            "audio",
            "audio",
            "audio",
            "audio",
            "audio",
            "audio",
            "audio",
            "progress:15",
        },
        "single chunk preserves seven direct services before post-write progress"
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
        0x1FU,
        0x00U,
        0xE0U,
        0x03U,
        0x00U,
        0x7CU,
        0x34U,
        0x12U,
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
    test.expect_equal(
        result.status,
        LegacyCmCacheGenerationStatus::ready,
        "non-16-bit map still writes the CM prefix"
    );
    test.expect_equal(
        tree.read("0.cm"),
        std::vector<u8>{0x1FU, 0x00U, 0xE0U, 0x03U, 0x00U, 0x7CU},
        "non-16-bit map bypasses the forward pixel converter"
    );
}

void test_exact_multiple_keeps_extra_chunk(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> first_raw{
        1U,
        2U,
        3U,
        4U,
        5U,
        6U,
        7U,
        8U,
    };
    constexpr std::array<u8, 8> second_raw{
        9U,
        10U,
        11U,
        12U,
        13U,
        14U,
        15U,
        16U,
    };
    std::vector<u8> first_compressed(first_raw.size() * 2U + 128U);
    std::vector<u8> second_compressed(second_raw.size() * 2U + 128U);
    const auto first_result =
        compress_legacy_lzo1x_15(first_raw, first_compressed);
    const auto second_result =
        compress_legacy_lzo1x_15(second_raw, second_compressed);
    first_compressed.resize(first_result.bytes_written);
    second_compressed.resize(second_result.bytes_written);

    constexpr std::size_t kHeaderOffset = 0x60U;
    std::vector<u8> archive(
        kHeaderOffset + 0x1A8U + first_compressed.size() +
        second_compressed.size()
    );
    const std::span<u8> archive_bytes{archive};
    write_u32(archive_bytes, kHeaderOffset + 0x10U, 8U);
    write_u32(archive_bytes, kHeaderOffset + 0x14U, 8U);
    write_u32(
        archive_bytes,
        kHeaderOffset + 0x1CU,
        static_cast<u32>(first_compressed.size())
    );
    write_u32(
        archive_bytes,
        kHeaderOffset + 0x24U,
        static_cast<u32>(second_compressed.size())
    );
    auto payload =
        archive.begin() + static_cast<std::ptrdiff_t>(kHeaderOffset + 0x1A8U);
    payload = std::ranges::copy(first_compressed, payload).out;
    std::ranges::copy(second_compressed, payload);
    tree.write("huge.lmf", archive);

    std::vector<i32> progress_stages;
    u32 audio_calls{};
    const auto result = generate_legacy_cm_cache_unit(
        tree.root() / "huge.lmf",
        0x20U,
        0x40U,
        tree.root() / "0.cm",
        8U,
        rgb565_conversion(),
        [&](const i32 progress) { progress_stages.push_back(progress); },
        [&] { ++audio_calls; }
    );
    test.expect_true(
        result.status == LegacyCmCacheGenerationStatus::ready &&
            result.chunk_count == 2U && result.completed_chunks == 2U &&
            result.cache_bytes_written == 8U &&
            result.decompressed_bytes_discarded == 8U && audio_calls == 10U &&
            progress_stages == std::vector<i32>{15, 37},
        "exact multiple preserves the original extra decompressed zero-write chunk"
    );
    test.expect_equal(
        tree.read("0.cm"),
        std::vector<u8>{1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U},
        "extra exact-multiple chunk does not append its discarded bytes"
    );
}

void test_size_probe_and_existing_tail(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> raw{
        0x1FU,
        0x00U,
        0xE0U,
        0x03U,
        0x00U,
        0x7CU,
        0x34U,
        0x12U,
    };
    tree.write("huge.lmf", make_archive(raw, 6U, 8U));

    u32 size_audio_calls{};
    const auto size = read_legacy_cm_cache_declared_size(
        tree.root() / "huge.lmf", 0x20U, 0x40U, [&] { ++size_audio_calls; }
    );
    test.expect_true(
        size.status == LegacyCmCacheSizeStatus::ready &&
            size.declared_output_size == 6U && size_audio_calls == 1U,
        "0x004270F0 reads only the declared CM output size"
    );

    tree.write(
        "0.cm",
        std::array<u8, 10>{
            0xAAU,
            0xAAU,
            0xAAU,
            0xAAU,
            0xAAU,
            0xAAU,
            0xEEU,
            0xEEU,
            0xEEU,
            0xEEU,
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
            0x1FU,
            0x00U,
            0xE0U,
            0x03U,
            0x00U,
            0x7CU,
            0xEEU,
            0xEEU,
            0xEEU,
            0xEEU,
        },
        "generator overwrites from offset zero without truncating the old tail"
    );
}

void test_size_probe_failures_preserve_service_boundary(
    openswd3::test::Context& test
) {
    const TestTree tree;

    u32 zero_offset_audio{};
    const auto zero_offset = read_legacy_cm_cache_declared_size(
        tree.root() / "missing.lmf", 0U, 0U, [&] { ++zero_offset_audio; }
    );
    test.expect_true(
        zero_offset.status == LegacyCmCacheSizeStatus::cm_offset_zero &&
            zero_offset_audio == 0U,
        "zero size-probe offset exits before its direct service"
    );

    u32 missing_archive_audio{};
    const auto missing_archive = read_legacy_cm_cache_declared_size(
        tree.root() / "missing.lmf", 0U, 1U, [&] { ++missing_archive_audio; }
    );
    test.expect_true(
        missing_archive.status ==
                LegacyCmCacheSizeStatus::archive_open_failed &&
            missing_archive_audio == 1U,
        "nonzero size probe services audio before platform archive open"
    );

    constexpr std::array<u8, 1> marker{0xAAU};
    tree.write("tiny.lmf", marker);
    u32 seek_failure_audio{};
    const auto seek_failure = read_legacy_cm_cache_declared_size(
        tree.root() / "tiny.lmf", 0x80000000U, 1U, [&] { ++seek_failure_audio; }
    );
    test.expect_true(
        seek_failure.status == LegacyCmCacheSizeStatus::size_seek_failed &&
            seek_failure_audio == 1U,
        "checked negative seek stops after the original direct service"
    );

    constexpr std::array<u8, 19> short_size_source{};
    tree.write("short-size.lmf", short_size_source);
    u32 read_failure_audio{};
    const auto read_failure = read_legacy_cm_cache_declared_size(
        tree.root() / "short-size.lmf", 0U, 1U, [&] { ++read_failure_audio; }
    );
    test.expect_true(
        read_failure.status == LegacyCmCacheSizeStatus::size_read_failed &&
            read_failure_audio == 1U,
        "short u32 read stops after the same pre-seek direct service"
    );
}

void test_early_failures_preserve_original_order(
    openswd3::test::Context& test
) {
    const TestTree tree;
    constexpr std::array<u8, 3> original{1U, 2U, 3U};
    tree.write("0.cm", original);
    u32 zero_offset_audio{};
    std::vector<i32> zero_offset_progress;
    const auto zero_offset = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        0U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion(),
        [&](const i32 progress) { zero_offset_progress.push_back(progress); },
        [&] { ++zero_offset_audio; }
    );
    test.expect_true(
        zero_offset.status == LegacyCmCacheGenerationStatus::cm_offset_zero &&
            tree.read("0.cm") == std::vector<u8>{1U, 2U, 3U} &&
            zero_offset_audio == 0U && zero_offset_progress.empty(),
        "zero CM offset exits before opening and truncating the cache unit"
    );

    u32 missing_cache_audio{};
    const auto missing_cache = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        1U,
        tree.root() / "missing.cm",
        16U,
        rgb565_conversion(),
        {},
        [&] { ++missing_cache_audio; }
    );
    test.expect_true(
        missing_cache.status ==
                LegacyCmCacheGenerationStatus::archive_open_failed &&
            std::filesystem::is_regular_file(tree.root() / "missing.cm") &&
            tree.read("missing.cm").empty() && missing_cache_audio == 2U,
        "OPEN_ALWAYS creates the cache slot before the source open fails"
    );

    u32 missing_archive_audio{};
    const auto missing_archive = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        1U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion(),
        {},
        [&] { ++missing_archive_audio; }
    );
    test.expect_true(
        missing_archive.status ==
                LegacyCmCacheGenerationStatus::archive_open_failed &&
            tree.read("0.cm") == std::vector<u8>{1U, 2U, 3U} &&
            missing_archive_audio == 2U,
        "opening an existing cache slot does not truncate it"
    );

    u32 missing_parent_audio{};
    const auto missing_parent = generate_legacy_cm_cache_unit(
        tree.root() / "missing.lmf",
        0U,
        1U,
        tree.root() / "missing-parent" / "0.cm",
        16U,
        rgb565_conversion(),
        {},
        [&] { ++missing_parent_audio; }
    );
    test.expect_equal(
        missing_parent.status,
        LegacyCmCacheGenerationStatus::cache_file_open_failed,
        "OPEN_ALWAYS still fails when the cache directory is absent"
    );
    test.expect_equal(
        missing_parent_audio,
        u32{1U},
        "cache-open failure stops after the first direct service"
    );
}

void test_block_failures_stop_at_original_service_boundary(
    openswd3::test::Context& test
) {
    const TestTree tree;

    constexpr std::array<u8, 2> short_archive{0xAAU, 0xBBU};
    tree.write("short.lmf", short_archive);
    u32 header_failure_audio{};
    const auto header_failure = generate_legacy_cm_cache_unit(
        tree.root() / "short.lmf",
        0U,
        1U,
        tree.root() / "header.cm",
        16U,
        rgb565_conversion(),
        {},
        [&] { ++header_failure_audio; }
    );
    test.expect_true(
        header_failure.status ==
                LegacyCmCacheGenerationStatus::header_read_failed &&
            header_failure_audio == 3U,
        "header-read failure follows the post-seek direct service"
    );

    std::vector<u8> truncated(0x60U + 0x1A8U);
    write_u32(truncated, 0x60U + 0x10U, 6U);
    write_u32(truncated, 0x60U + 0x14U, 8U);
    write_u32(truncated, 0x60U + 0x1CU, 4U);
    tree.write("truncated.lmf", truncated);
    u32 block_read_audio{};
    const auto block_read_failure = generate_legacy_cm_cache_unit(
        tree.root() / "truncated.lmf",
        0x20U,
        0x40U,
        tree.root() / "block-read.cm",
        16U,
        rgb565_conversion(),
        {},
        [&] { ++block_read_audio; }
    );
    test.expect_true(
        block_read_failure.status ==
                LegacyCmCacheGenerationStatus::compressed_read_failed &&
            block_read_audio == 5U,
        "compressed-read failure stops before the post-read service"
    );

    truncated.push_back(0xFFU);
    write_u32(truncated, 0x60U + 0x1CU, 1U);
    tree.write("invalid.lmf", truncated);
    u32 decompression_audio{};
    const auto decompression_failure = generate_legacy_cm_cache_unit(
        tree.root() / "invalid.lmf",
        0x20U,
        0x40U,
        tree.root() / "decompression.cm",
        16U,
        rgb565_conversion(),
        {},
        [&] { ++decompression_audio; }
    );
    test.expect_true(
        decompression_failure.status ==
                LegacyCmCacheGenerationStatus::decompression_failed &&
            decompression_audio == 6U,
        "decompression failure stops before conversion and pre-write service"
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
    tree.write("0.cm", std::span<const u8>{});
    u32 size_audio_calls{};
    const auto size = read_legacy_cm_cache_declared_size(
        archive_path, 0x026698A3U, 0x00049193U, [&] { ++size_audio_calls; }
    );
    test.expect_true(
        size.status == LegacyCmCacheSizeStatus::ready &&
            size.declared_output_size == 3'706'880U && size_audio_calls == 1U,
        "map 24 size probe preserves its one direct service"
    );

    std::vector<i32> progress_stages;
    u32 audio_calls{};
    const auto result = generate_legacy_cm_cache_unit(
        archive_path,
        0x026698A3U,
        0x00049193U,
        tree.root() / "0.cm",
        16U,
        rgb565_conversion(),
        [&](const i32 progress) { progress_stages.push_back(progress); },
        [&] { ++audio_calls; }
    );
    const std::vector<u8> generated = tree.read("0.cm");
    test.expect_true(
        result.status == LegacyCmCacheGenerationStatus::ready &&
            result.chunk_output_size == 1'024'768U &&
            result.chunk_count == 4U && result.completed_chunks == 4U &&
            result.cache_bytes_written == 3'706'880U && audio_calls == 16U &&
            progress_stages == std::vector<i32>{15, 26, 37, 48},
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
    test_exact_multiple_keeps_extra_chunk(test);
    test_size_probe_and_existing_tail(test);
    test_size_probe_failures_preserve_service_boundary(test);
    test_early_failures_preserve_original_order(test);
    test_block_failures_stop_at_original_service_boundary(test);

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
