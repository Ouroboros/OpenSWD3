#include "test.hpp"

#include "openswd3/resource_io/legacy_environment.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::resource_io::LegacyEnvironmentCodecStatus;
using openswd3::resource_io::LegacyEnvironmentLayout;
using openswd3::resource_io::LegacyEnvironmentRecord;
using openswd3::resource_io::decode_legacy_environment;
using openswd3::resource_io::encode_legacy_environment;
using openswd3::resource_io::kLegacyEnvironmentWindowSize;
using openswd3::resource_io::migrate_legacy_environment;

constexpr std::array<u8, 63> kCurrentEnvironment{
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xC8U, 0xD0U, 0xCBU, 0xCDU,
    0x39U, 0x1CU, 0x9DU, 0x01U, 0xCFU, 0x36U, 0x13U, 0x1EU,
    0x22U, 0x3BU, 0xC9U, 0xD1U, 0x64U, 0x00U, 0x00U, 0x00U,
    0x06U, 0x06U, 0x3CU, 0x00U, 0x02U, 0x0AU, 0x00U, 0xF8U,
    0xE0U, 0x07U, 0x1FU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x45U, 0x3AU,
    0x5CU, 0x47U, 0x61U, 0x6DU, 0x65U, 0x5CU, 0x73U, 0x77U,
    0x64U, 0x33U, 0x5CU, 0x00U, 0x00U, 0x02U, 0x01U,
};

void test_current_asset_record(openswd3::test::Context& test) {
    const auto decoded = decode_legacy_environment(kCurrentEnvironment);
    test.expect_equal(
        decoded.status,
        LegacyEnvironmentCodecStatus::ok,
        "current Env.dat record decodes"
    );
    test.expect_equal(
        decoded.layout,
        LegacyEnvironmentLayout::current,
        "0xFFFFFFFF selects the current layout"
    );

    constexpr std::array<u8, 16> kBindings{
        0xC8U, 0xD0U, 0xCBU, 0xCDU,
        0x39U, 0x1CU, 0x9DU, 0x01U,
        0xCFU, 0x36U, 0x13U, 0x1EU,
        0x22U, 0x3BU, 0xC9U, 0xD1U,
    };
    constexpr std::array<u8, 6> kOptions{
        0x06U, 0x06U, 0x3CU, 0x00U, 0x02U, 0x0AU,
    };
    constexpr std::array<u8, 16> kPreserved{
        0x00U, 0xF8U, 0xE0U, 0x07U,
        0x1FU, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x00U,
    };
    test.expect_equal(
        decoded.record.binding_bytes,
        kBindings,
        "sixteen binding bytes keep file order"
    );
    test.expect_equal(
        decoded.record.integer_parameter,
        100U,
        "the 32-bit parameter is little-endian"
    );
    test.expect_equal(
        decoded.record.option_bytes,
        kOptions,
        "six option bytes keep physical order"
    );
    test.expect_equal(
        decoded.record.primary_directory,
        std::string{"E:\\Game\\swd3\\"},
        "the first raw ANSI directory is recovered"
    );
    test.expect_true(
        decoded.record.secondary_directory.empty(),
        "the second directory can be empty"
    );
    test.expect_equal(
        decoded.record.trailing_mode,
        u8{2U},
        "only the first byte after both strings is consumed"
    );
    test.expect_equal(
        decoded.record.preserved_bytes,
        kPreserved,
        "all sixteen preserved bytes retain their physical order"
    );

    const auto encoded = encode_legacy_environment(decoded.record);
    test.expect_equal(
        encoded.status,
        LegacyEnvironmentCodecStatus::ok,
        "decoded record re-encodes"
    );
    test.expect_equal(encoded.bytes.size(), 63U, "writer emits the exact prefix");
    test.expect_true(
        std::equal(
            encoded.bytes.begin(),
            encoded.bytes.end() - 1,
            kCurrentEnvironment.begin()
        ),
        "writer matches the current asset before its stale final byte"
    );
    test.expect_equal(
        encoded.bytes.back(),
        u8{0U},
        "writer stores zero after the consumed trailing mode"
    );
}

void test_legacy_layout_migration(openswd3::test::Context& test) {
    const std::vector<u8> legacy(
        kCurrentEnvironment.begin() + 4,
        kCurrentEnvironment.end()
    );
    const auto decoded = decode_legacy_environment(legacy);
    test.expect_equal(
        decoded.status,
        LegacyEnvironmentCodecStatus::ok,
        "markerless record decodes"
    );
    test.expect_equal(
        decoded.layout,
        LegacyEnvironmentLayout::legacy_without_marker,
        "a non-marker prefix selects the legacy layout"
    );
    test.expect_equal(
        decoded.record.primary_directory,
        std::string{"E:\\Game\\swd3\\"},
        "legacy strings begin four bytes earlier"
    );
    test.expect_equal(
        decoded.record.trailing_mode,
        u8{2U},
        "legacy trailing mode is captured before rewriting"
    );

    const LegacyEnvironmentRecord migrated = migrate_legacy_environment(
        decoded.record
    );
    constexpr std::array<u8, 16> kZeroBindings{};
    constexpr std::array<u8, 16> kZeroPreserved{};
    constexpr std::array<u8, 6> kMigrationOptions{
        6U, 6U, 0x3CU, 1U, 2U, 0x0AU,
    };
    test.expect_equal(
        migrated.binding_bytes,
        kZeroBindings,
        "loader clears all binding globals before the migration writer"
    );
    test.expect_equal(
        migrated.integer_parameter,
        100U,
        "migration uses the hard-coded integer parameter"
    );
    test.expect_equal(
        migrated.option_bytes,
        kMigrationOptions,
        "migration uses its six hard-coded option bytes"
    );
    test.expect_equal(
        migrated.preserved_bytes,
        kZeroPreserved,
        "old-layout migration clears the sixteen preserved bytes"
    );
    test.expect_equal(
        migrated.primary_directory,
        decoded.record.primary_directory,
        "migration retains the first legacy string"
    );
}

void test_zero_padding_and_string_bounds(openswd3::test::Context& test) {
    const std::array<u8, 4> empty_legacy{};
    const auto padded = decode_legacy_environment(empty_legacy);
    test.expect_equal(
        padded.status,
        LegacyEnvironmentCodecStatus::ok,
        "short reads observe the pre-zeroed 4 KiB window"
    );
    test.expect_equal(
        padded.layout,
        LegacyEnvironmentLayout::legacy_without_marker,
        "a zero prefix is markerless"
    );
    test.expect_true(
        padded.record.primary_directory.empty() &&
            padded.record.secondary_directory.empty(),
        "zero padding terminates both strings"
    );
    test.expect_equal(
        padded.record.trailing_mode,
        u8{0U},
        "zero padding supplies the trailing mode"
    );

    std::vector<u8> oversized(kLegacyEnvironmentWindowSize + 1U);
    test.expect_equal(
        decode_legacy_environment(oversized).status,
        LegacyEnvironmentCodecStatus::input_too_large,
        "modern boundary rejects the original 4 KiB read overflow"
    );

    std::vector<u8> no_primary(kLegacyEnvironmentWindowSize, 0x41U);
    std::fill_n(no_primary.begin(), 4, 0xFFU);
    test.expect_equal(
        decode_legacy_environment(no_primary).status,
        LegacyEnvironmentCodecStatus::unterminated_primary_directory,
        "modern boundary stops an unterminated first string"
    );

    std::vector<u8> no_secondary(kLegacyEnvironmentWindowSize, 0x41U);
    std::fill_n(no_secondary.begin(), 4, 0xFFU);
    no_secondary[0x2EU] = 0U;
    test.expect_equal(
        decode_legacy_environment(no_secondary).status,
        LegacyEnvironmentCodecStatus::unterminated_secondary_directory,
        "modern boundary stops an unterminated second string"
    );

    std::vector<u8> no_trailing(kLegacyEnvironmentWindowSize, 0x41U);
    std::fill_n(no_trailing.begin(), 4, 0xFFU);
    no_trailing[0x2EU] = 0U;
    no_trailing.back() = 0U;
    test.expect_equal(
        decode_legacy_environment(no_trailing).status,
        LegacyEnvironmentCodecStatus::missing_trailing_mode,
        "modern boundary stops the original trailing-byte overread"
    );
}

void test_encoder_bounds_and_lstrlen_prefix(openswd3::test::Context& test) {
    LegacyEnvironmentRecord record;
    record.primary_directory = std::string{"abc\0ignored", 11U};
    record.secondary_directory = "xy";
    record.trailing_mode = 7U;
    const auto encoded = encode_legacy_environment(record);
    test.expect_equal(
        encoded.status,
        LegacyEnvironmentCodecStatus::ok,
        "embedded NUL input encodes"
    );
    test.expect_equal(
        encoded.bytes.size(),
        0x32U + 3U + 2U,
        "encoder follows lstrlenA prefixes"
    );
    test.expect_equal(
        encoded.bytes[0x2EU],
        u8{'a'},
        "first string starts at +0x2E"
    );
    test.expect_equal(
        encoded.bytes[0x31U],
        u8{0U},
        "first prefix is terminated"
    );
    test.expect_equal(
        encoded.bytes[0x35U],
        u8{7U},
        "trailing mode follows both strings"
    );
    test.expect_equal(
        encoded.bytes[0x36U],
        u8{0U},
        "writer appends a final zero"
    );

    const auto round_trip = decode_legacy_environment(encoded.bytes);
    test.expect_equal(
        round_trip.record.primary_directory,
        std::string{"abc"},
        "decoder sees the lstrlenA-limited first string"
    );
    test.expect_equal(
        round_trip.record.secondary_directory,
        std::string{"xy"},
        "decoder sees a non-empty second string"
    );

    record.primary_directory.assign(kLegacyEnvironmentWindowSize, 'A');
    record.secondary_directory.clear();
    test.expect_equal(
        encode_legacy_environment(record).status,
        LegacyEnvironmentCodecStatus::output_too_large,
        "modern boundary rejects the original string-copy overflow"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_current_asset_record(test);
    test_legacy_layout_migration(test);
    test_zero_padding_and_string_bounds(test);
    test_encoder_bounds_and_lstrlen_prefix(test);
    return test.exit_code();
}
