#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::resource_io::LegacyLzo1xResult;
using openswd3::resource_io::LegacyLzo1xStatus;

struct DecodeOutput {
    LegacyLzo1xResult result{};
    std::vector<u8> bytes;
};

[[nodiscard]] DecodeOutput
decode(const std::span<const u8> source, const std::size_t destination_size) {
    std::vector<u8> destination(destination_size);
    const LegacyLzo1xResult result =
        openswd3::resource_io::decompress_legacy_lzo1x(source, destination);
    destination.resize(result.bytes_written);
    return DecodeOutput{result, std::move(destination)};
}

void expect_decode(
    openswd3::test::Context& test,
    const std::span<const u8> source,
    const std::span<const u8> expected
) {
    const DecodeOutput output = decode(source, expected.size());
    test.expect_equal(
        output.result.status, LegacyLzo1xStatus::success, "valid stream status"
    );
    test.expect_equal(
        output.result.bytes_written,
        static_cast<openswd3::compat::u32>(expected.size()),
        "valid stream output size"
    );
    test.expect_true(
        std::ranges::equal(output.bytes, expected), "valid stream output bytes"
    );
}

void test_valid_branch_vectors(openswd3::test::Context& test) {
    constexpr std::array<u8, 5> kInitialLiteralOne{
        0x12U,
        0x41U,
        0x11U,
        0x00U,
        0x00U,
    };
    constexpr std::array<u8, 1> kOneA{0x41U};
    expect_decode(test, kInitialLiteralOne, kOneA);

    constexpr std::array<u8, 8> kInitialLiteralFour{
        0x15U,
        0x41U,
        0x42U,
        0x43U,
        0x44U,
        0x11U,
        0x00U,
        0x00U,
    };
    constexpr std::array<u8, 4> kAbcd{0x41U, 0x42U, 0x43U, 0x44U};
    expect_decode(test, kInitialLiteralFour, kAbcd);

    constexpr std::array<u8, 7> kShortMatchAfterMatch{
        0x12U,
        0x41U,
        0x00U,
        0x00U,
        0x11U,
        0x00U,
        0x00U,
    };
    constexpr std::array<u8, 3> kThreeA{0x41U, 0x41U, 0x41U};
    expect_decode(test, kShortMatchAfterMatch, kThreeA);

    std::vector<u8> long_literals(2050U);
    for (std::size_t index = 0U; index < long_literals.size(); ++index) {
        long_literals[index] = static_cast<u8>((index * 37U) & 0xFFU);
    }

    std::vector<u8> short_match_after_literal{
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0xF7U,
    };
    short_match_after_literal.insert(
        short_match_after_literal.end(),
        long_literals.begin(),
        long_literals.end()
    );
    short_match_after_literal.insert(
        short_match_after_literal.end(), {0x00U, 0x00U, 0x11U, 0x00U, 0x00U}
    );

    std::vector<u8> long_expected = long_literals;
    long_expected.insert(
        long_expected.end(),
        {long_literals[1], long_literals[2], long_literals[3]}
    );
    expect_decode(test, short_match_after_literal, long_expected);
}

void test_end_and_safety_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u8, 3> kExactEnd{0x11U, 0x00U, 0x00U};
    const DecodeOutput exact = decode(kExactEnd, 16U);
    test.expect_equal(
        exact.result.status, LegacyLzo1xStatus::success, "exact end status"
    );
    test.expect_equal(exact.result.bytes_written, 0U, "exact end output");

    constexpr std::array<u8, 4> kTrailingInput{
        0x11U,
        0x00U,
        0x00U,
        0x58U,
    };
    const DecodeOutput trailing = decode(kTrailingInput, 16U);
    test.expect_equal(
        trailing.result.status,
        LegacyLzo1xStatus::input_not_consumed,
        "trailing input status"
    );

    constexpr std::array<u8, 2> kTruncatedEnd{0x11U, 0x00U};
    const DecodeOutput truncated = decode(kTruncatedEnd, 16U);
    test.expect_equal(
        truncated.result.status,
        LegacyLzo1xStatus::source_exhausted,
        "truncated input safety status"
    );

    constexpr std::array<u8, 8> kFourLiterals{
        0x01U,
        0x41U,
        0x42U,
        0x43U,
        0x44U,
        0x11U,
        0x00U,
        0x00U,
    };
    const DecodeOutput short_destination = decode(kFourLiterals, 3U);
    test.expect_equal(
        short_destination.result.status,
        LegacyLzo1xStatus::destination_exhausted,
        "destination safety status"
    );

    constexpr std::array<u8, 10> kInvalidLookbehind{
        0x01U,
        0x41U,
        0x42U,
        0x43U,
        0x44U,
        0x40U,
        0xFFU,
        0x11U,
        0x00U,
        0x00U,
    };
    const DecodeOutput invalid_lookbehind = decode(kInvalidLookbehind, 64U);
    test.expect_equal(
        invalid_lookbehind.result.status,
        LegacyLzo1xStatus::invalid_lookbehind,
        "lookbehind safety status"
    );
    test.expect_equal(
        invalid_lookbehind.result.bytes_written,
        4U,
        "lookbehind preserves written byte count"
    );
}

void test_shared_wrapper_contract(openswd3::test::Context& test) {
    constexpr std::array<u8, 6> kTrailingInput{
        0x12U,
        0x41U,
        0x11U,
        0x00U,
        0x00U,
        0x58U,
    };
    std::array<u8, 8> destination{};
    openswd3::compat::u32 actual_output_size = 0xFFFFFFFFU;
    const LegacyLzo1xStatus trailing_status =
        openswd3::resource_io::decompress_legacy_resource_block(
            kTrailingInput, destination, actual_output_size
        );

    test.expect_equal(
        trailing_status,
        LegacyLzo1xStatus::input_not_consumed,
        "wrapper propagates the decoder status"
    );
    test.expect_equal(
        actual_output_size, 1U, "wrapper writes output size after an end marker"
    );
    test.expect_equal(
        destination[0],
        u8{0x41U},
        "ignored trailing-input status still leaves the decoded output"
    );

    constexpr std::array<u8, 2> kTruncatedEnd{0x11U, 0x00U};
    actual_output_size = 0xFFFFFFFFU;
    const LegacyLzo1xStatus safety_status =
        openswd3::resource_io::decompress_legacy_resource_block(
            kTruncatedEnd, destination, actual_output_size
        );
    test.expect_equal(
        safety_status,
        LegacyLzo1xStatus::source_exhausted,
        "wrapper exposes the modern source boundary"
    );
    test.expect_equal(
        actual_output_size,
        0U,
        "early safety return preserves the legacy entry-time zero"
    );
}

void test_real_tsw_frame(openswd3::test::Context& test) {
    constexpr std::array<u8, 23> kCompressed{
        0x0AU, 0xFFU, 0xFFU, 0x20U, 0x00U, 0x20U, 0x00U, 0x08U,
        0x00U, 0x06U, 0x00U, 0x20U, 0xC0U, 0x00U, 0x20U, 0x9AU,
        0x16U, 0x00U, 0x00U, 0x00U, 0x11U, 0x00U, 0x00U,
    };

    std::vector<u8> expected{
        0xFFU,
        0xFFU,
        0x20U,
        0x00U,
        0x20U,
        0x00U,
        0x08U,
        0x00U,
        0x06U,
        0x00U,
        0x20U,
        0xC0U,
        0x00U,
    };
    constexpr std::array<u8, 6> kRepeated{
        0x00U,
        0x06U,
        0x00U,
        0x20U,
        0xC0U,
        0x00U,
    };
    for (std::size_t index = 0U; index < 187U; ++index) {
        expected.push_back(kRepeated[index % kRepeated.size()]);
    }

    expected.insert(expected.end(), {0x00U, 0x00U});
    expect_decode(test, kCompressed, expected);
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_valid_branch_vectors(test);
    test_end_and_safety_boundaries(test);
    test_shared_wrapper_contract(test);
    test_real_tsw_frame(test);
    return test.exit_code();
}
