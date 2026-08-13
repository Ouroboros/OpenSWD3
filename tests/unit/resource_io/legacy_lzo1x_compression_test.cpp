#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyLzo1xResult;
using openswd3::resource_io::LegacyLzo1xStatus;
using openswd3::resource_io::compress_legacy_save_block;

using Compressor =
    LegacyLzo1xResult (*)(std::span<const u8>, std::span<u8>) noexcept;

constexpr std::array<Compressor, 2> kCompressors{
    openswd3::resource_io::compress_legacy_lzo1x_14,
    openswd3::resource_io::compress_legacy_lzo1x_15,
};

struct CompressionOutput {
    LegacyLzo1xResult result{};
    std::vector<u8> bytes;
};

[[nodiscard]] std::size_t
compression_capacity(const std::size_t source_size) noexcept {
    return source_size + source_size / 16U + 67U;
}

[[nodiscard]] CompressionOutput compress(
    const std::span<const u8> source,
    const Compressor compressor,
    const std::size_t destination_size
) {
    std::vector<u8> destination(destination_size);
    const LegacyLzo1xResult result = compressor(source, destination);
    destination.resize(
        std::min<std::size_t>(destination.size(), result.bytes_written)
    );
    return CompressionOutput{result, std::move(destination)};
}

[[nodiscard]] CompressionOutput
compress(const std::span<const u8> source, const Compressor compressor) {
    return compress(source, compressor, compression_capacity(source.size()));
}

void expect_round_trip(
    openswd3::test::Context& test,
    const std::span<const u8> source,
    const CompressionOutput& compressed
) {
    std::vector<u8> restored(source.size());
    const LegacyLzo1xResult result =
        openswd3::resource_io::decompress_legacy_lzo1x(
            compressed.bytes, restored
        );
    test.expect_equal(
        result.status,
        LegacyLzo1xStatus::success,
        "compressed stream round-trip status"
    );
    test.expect_equal(
        result.bytes_written,
        static_cast<u32>(source.size()),
        "compressed stream round-trip size"
    );
    test.expect_true(
        std::ranges::equal(restored, source),
        "compressed stream round-trip bytes"
    );
}

void expect_exact(
    openswd3::test::Context& test,
    const std::span<const u8> source,
    const std::span<const u8> expected
) {
    for (const Compressor compressor : kCompressors) {
        const CompressionOutput output = compress(source, compressor);
        test.expect_equal(
            output.result.status,
            LegacyLzo1xStatus::success,
            "exact vector compression status"
        );
        test.expect_equal(
            output.result.bytes_written,
            static_cast<u32>(expected.size()),
            "exact vector compression size"
        );
        test.expect_true(
            std::ranges::equal(output.bytes, expected),
            "exact vector compression bytes"
        );
        expect_round_trip(test, source, output);
    }
}

[[nodiscard]] std::uint64_t fnv1a64(const std::span<const u8> bytes) noexcept {
    std::uint64_t value = 0xCBF29CE484222325ULL;
    for (const u8 byte : bytes) {
        value ^= byte;
        value *= 0x100000001B3ULL;
    }
    return value;
}

[[nodiscard]] bool contains_bytes(
    const std::span<const u8> bytes, const std::span<const u8> expected
) {
    return std::search(
               bytes.begin(), bytes.end(), expected.begin(), expected.end()
           ) != bytes.end();
}

void test_literal_vectors(openswd3::test::Context& test) {
    constexpr std::array<u8, 0> kEmpty{};
    constexpr std::array<u8, 3> kEmptyExpected{0x11U, 0x00U, 0x00U};
    expect_exact(test, kEmpty, kEmptyExpected);

    constexpr std::array<u8, 1> kOne{0x41U};
    constexpr std::array<u8, 5> kOneExpected{
        0x12U,
        0x41U,
        0x11U,
        0x00U,
        0x00U,
    };
    expect_exact(test, kOne, kOneExpected);

    constexpr std::array<u8, 13> kThirteen{
        0x00U,
        0x01U,
        0x02U,
        0x03U,
        0x04U,
        0x05U,
        0x06U,
        0x07U,
        0x08U,
        0x09U,
        0x0AU,
        0x0BU,
        0x0CU,
    };
    constexpr std::array<u8, 17> kThirteenExpected{
        0x1EU,
        0x00U,
        0x01U,
        0x02U,
        0x03U,
        0x04U,
        0x05U,
        0x06U,
        0x07U,
        0x08U,
        0x09U,
        0x0AU,
        0x0BU,
        0x0CU,
        0x11U,
        0x00U,
        0x00U,
    };
    expect_exact(test, kThirteen, kThirteenExpected);

    constexpr std::array<u8, 14> kFourteen{
        0x00U,
        0x01U,
        0x02U,
        0x03U,
        0x04U,
        0x05U,
        0x06U,
        0x07U,
        0x08U,
        0x09U,
        0x0AU,
        0x0BU,
        0x0CU,
        0x0DU,
    };
    constexpr std::array<u8, 18> kFourteenExpected{
        0x1FU,
        0x00U,
        0x01U,
        0x02U,
        0x03U,
        0x04U,
        0x05U,
        0x06U,
        0x07U,
        0x08U,
        0x09U,
        0x0AU,
        0x0BU,
        0x0CU,
        0x0DU,
        0x11U,
        0x00U,
        0x00U,
    };
    expect_exact(test, kFourteen, kFourteenExpected);

    std::vector<u8> long_literal;
    long_literal.reserve(300U);
    for (u32 index = 0U; index < 100U; ++index) {
        long_literal.push_back(static_cast<u8>(index));
        long_literal.push_back(0x00U);
        long_literal.push_back(0xA5U);
    }

    std::vector<u8> long_expected{0x00U, 0x00U, 0x1BU};
    long_expected.insert(
        long_expected.end(), long_literal.begin(), long_literal.end()
    );
    long_expected.insert(long_expected.end(), {0x11U, 0x00U, 0x00U});
    expect_exact(test, long_literal, long_expected);
}

void test_short_match_and_tail_literals(openswd3::test::Context& test) {
    constexpr std::array<u8, 30> kM2Source{
        'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'X', 'q',
        'A', 'B', 'C', 'D', 'Y', '6', '7', '8', '9', '0',
        '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    };
    constexpr std::array<u8, 33> kM2Expected{
        0x07U, 'w',   'x',   'y', 'z', 'A', 'B', 'C', 'D',   'X',   'q',
        0x74U, 0x00U, 0x0DU, 'Y', '6', '7', '8', '9', '0',   '1',   '2',
        '3',   '4',   '5',   '6', '7', '8', '9', '0', 0x11U, 0x00U, 0x00U,
    };
    expect_exact(test, kM2Source, kM2Expected);

    constexpr std::array<u8, 3> kTailBytes{'B', 'C', 'D'};
    for (std::size_t tail_size = 0U; tail_size <= kTailBytes.size();
         ++tail_size) {
        std::vector<u8> source(30U, static_cast<u8>('A'));
        source.insert(
            source.end(),
            kTailBytes.begin(),
            kTailBytes.begin() + static_cast<std::ptrdiff_t>(tail_size)
        );

        std::vector<u8> expected{
            0x02U,
            'A',
            'A',
            'A',
            'A',
            'A',
            0x37U,
            static_cast<u8>(tail_size),
            0x00U,
        };
        expected.insert(
            expected.end(),
            kTailBytes.begin(),
            kTailBytes.begin() + static_cast<std::ptrdiff_t>(tail_size)
        );
        expected.insert(expected.end(), {0x11U, 0x00U, 0x00U});
        expect_exact(test, source, expected);
    }

    const std::vector<u8> long_match_source(600U, static_cast<u8>('A'));
    constexpr std::array<u8, 15> kLongMatchExpected{
        0x02U,
        'A',
        'A',
        'A',
        'A',
        'A',
        0x20U,
        0x00U,
        0x00U,
        0x34U,
        0x00U,
        0x00U,
        0x11U,
        0x00U,
        0x00U,
    };
    expect_exact(test, long_match_source, kLongMatchExpected);
}

[[nodiscard]] std::vector<u8> make_distance_source(const std::size_t distance) {
    std::vector<u8> source{
        'w',
        'x',
        'y',
        'z',
        'A',
        'B',
        'C',
        'D',
        'X',
    };
    source.insert(source.end(), distance - 5U, static_cast<u8>('Z'));
    source.insert(source.end(), {'A', 'B', 'C', 'D', 'Y', '1', '2', '3', '4',
                                 '5', '6', '7', '8', '9', '0', '1', '2', '3',
                                 '4', '5', '6', '7', '8', '9', '0'});
    return source;
}

void expect_distance_marker(
    openswd3::test::Context& test,
    const std::size_t distance,
    const std::span<const u8> marker
) {
    const std::vector<u8> source = make_distance_source(distance);
    for (const Compressor compressor : kCompressors) {
        const CompressionOutput output = compress(source, compressor);
        test.expect_equal(
            output.result.status,
            LegacyLzo1xStatus::success,
            "distance boundary compression status"
        );
        test.expect_true(
            contains_bytes(output.bytes, marker),
            "distance boundary marker bytes"
        );
        expect_round_trip(test, source, output);
    }
}

void test_distance_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kM2Maximum{
        0x7CU,
        0xFFU,
        0x00U,
        0x03U,
        'Y',
    };
    expect_distance_marker(test, 0x0800U, kM2Maximum);

    constexpr std::array<u8, 6> kM3Minimum{
        0x22U,
        0x00U,
        0x20U,
        0x00U,
        0x03U,
        'Y',
    };
    expect_distance_marker(test, 0x0801U, kM3Minimum);

    constexpr std::array<u8, 6> kM3Maximum{
        0x22U,
        0xFCU,
        0xFFU,
        0x00U,
        0x03U,
        'Y',
    };
    expect_distance_marker(test, 0x4000U, kM3Maximum);

    constexpr std::array<u8, 6> kM4Minimum{
        0x12U,
        0x04U,
        0x00U,
        0x00U,
        0x03U,
        'Y',
    };
    expect_distance_marker(test, 0x4001U, kM4Minimum);

    constexpr std::array<u8, 6> kM4Maximum{
        0x1AU,
        0xFCU,
        0xFFU,
        0x00U,
        0x03U,
        'Y',
    };
    expect_distance_marker(test, 0xBFFFU, kM4Maximum);
}

[[nodiscard]] std::vector<u8> make_collision_source() {
    std::vector<u8> source;
    source.reserve(512U);
    u32 state = 0x12345678U;
    for (std::size_t index = 0U; index < 512U; ++index) {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        source.push_back(static_cast<u8>(state & 0x0FU));
    }
    return source;
}

void test_dictionary_widths(openswd3::test::Context& test) {
    const std::vector<u8> source = make_collision_source();
    const CompressionOutput output_14 =
        compress(source, openswd3::resource_io::compress_legacy_lzo1x_14);
    const CompressionOutput output_15 =
        compress(source, openswd3::resource_io::compress_legacy_lzo1x_15);

    test.expect_equal(
        output_14.result.status,
        LegacyLzo1xStatus::success,
        "14-bit dictionary status"
    );
    test.expect_equal(
        output_14.result.bytes_written, 518U, "14-bit output size"
    );
    test.expect_equal(
        fnv1a64(output_14.bytes), 0x28C584A5307464A9ULL, "14-bit output bytes"
    );

    test.expect_equal(
        output_15.result.status,
        LegacyLzo1xStatus::success,
        "15-bit dictionary status"
    );
    test.expect_equal(
        output_15.result.bytes_written, 517U, "15-bit output size"
    );
    test.expect_equal(
        fnv1a64(output_15.bytes), 0x4C542BBD551818C9ULL, "15-bit output bytes"
    );
    test.expect_false(
        std::ranges::equal(output_14.bytes, output_15.bytes),
        "dictionary widths produce distinct fixed streams"
    );
    expect_round_trip(test, source, output_14);
    expect_round_trip(test, source, output_15);
}

void test_destination_boundary(openswd3::test::Context& test) {
    constexpr std::array<u8, 19> kSource{
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
    };
    constexpr std::array<u8, 12> kExpected{
        0x02U,
        'A',
        'A',
        'A',
        'A',
        'A',
        0x2CU,
        0x00U,
        0x00U,
        0x11U,
        0x00U,
        0x00U,
    };

    for (const Compressor compressor : kCompressors) {
        const CompressionOutput exact =
            compress(kSource, compressor, kExpected.size());
        test.expect_equal(
            exact.result.status,
            LegacyLzo1xStatus::success,
            "exact destination capacity status"
        );
        test.expect_true(
            std::ranges::equal(exact.bytes, kExpected),
            "exact destination capacity bytes"
        );

        const CompressionOutput short_output =
            compress(kSource, compressor, kExpected.size() - 1U);
        test.expect_equal(
            short_output.result.status,
            LegacyLzo1xStatus::destination_exhausted,
            "short destination safety status"
        );
    }
}

void test_save_allocation_wrapper(openswd3::test::Context& test) {
    constexpr std::array<u8, 19> kSource{
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
    };
    constexpr std::array<u8, 12> kExpected{
        0x02U,
        'A',
        'A',
        'A',
        'A',
        'A',
        0x2CU,
        0x00U,
        0x00U,
        0x11U,
        0x00U,
        0x00U,
    };

    const auto block = compress_legacy_save_block(kSource);
    test.expect_equal(
        block.status,
        LegacyLzo1xStatus::success,
        "0x004267E0 save wrapper compression status"
    );
    test.expect_equal(
        block.storage.size(),
        kSource.size() + 0x20U,
        "save wrapper allocates source length plus 0x20 bytes"
    );
    test.expect_equal(
        block.bytes_written,
        static_cast<u32>(kExpected.size()),
        "save wrapper writes the compressed length output"
    );
    test.expect_true(
        std::equal(kExpected.begin(), kExpected.end(), block.storage.begin()),
        "save wrapper returns the exact 14-bit compressed prefix"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_literal_vectors(test);
    test_short_match_and_tail_literals(test);
    test_distance_boundaries(test);
    test_dictionary_widths(test);
    test_destination_boundary(test);
    test_save_allocation_wrapper(test);
    return test.exit_code();
}
