#include "test.hpp"

#include "openswd3/resource_io/legacy_environment.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u8;
using openswd3::resource_io::LegacyEnvironmentCodecStatus;
using openswd3::resource_io::LegacyEnvironmentCacheSessionMarker;
using openswd3::resource_io::LegacyEnvironmentLayout;
using openswd3::resource_io::LegacyEnvironmentLoadStatus;
using openswd3::resource_io::LegacyEnvironmentRecord;
using openswd3::resource_io::decode_legacy_environment;
using openswd3::resource_io::encode_legacy_environment;
using openswd3::resource_io::initialize_legacy_environment;
using openswd3::resource_io::kLegacyEnvironmentWindowSize;
using openswd3::resource_io::load_legacy_environment;
using openswd3::resource_io::migrate_unmarked_environment;
using openswd3::resource_io::rewrite_legacy_environment;
using openswd3::resource_io::write_legacy_environment_binding_prefix;
using openswd3::resource_io::write_legacy_environment_cache_session_marker;

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

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-environment-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
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

    [[nodiscard]] std::vector<u8> read(const char* name) const {
        std::ifstream input{path(name), std::ios::binary};
        return std::vector<u8>{
            std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}
        };
    }

private:
    std::filesystem::path root_;
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
        LegacyEnvironmentLayout::marked,
        "0xFFFFFFFF selects the marked layout"
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

void test_unmarked_layout_migration(openswd3::test::Context& test) {
    const std::vector<u8> unmarked(
        kCurrentEnvironment.begin() + 4,
        kCurrentEnvironment.end()
    );
    const auto decoded = decode_legacy_environment(unmarked);
    test.expect_equal(
        decoded.status,
        LegacyEnvironmentCodecStatus::ok,
        "markerless record decodes"
    );
    test.expect_equal(
        decoded.layout,
        LegacyEnvironmentLayout::unmarked,
        "a non-marker prefix selects the unmarked layout"
    );
    test.expect_equal(
        decoded.record.primary_directory,
        std::string{"E:\\Game\\swd3\\"},
        "unmarked strings begin four bytes earlier"
    );
    test.expect_equal(
        decoded.record.trailing_mode,
        u8{2U},
        "unmarked trailing mode is captured before rewriting"
    );

    const LegacyEnvironmentRecord migrated = migrate_unmarked_environment(
        decoded.record
    );
    constexpr std::array<u8, 16> kDefaultBindings{
        0xC8U, 0xD0U, 0xCBU, 0xCDU,
        0x39U, 0x1CU, 0x9DU, 0x01U,
        0xCFU, 0x36U, 0x13U, 0x1EU,
        0x22U, 0x3BU, 0xC9U, 0xD1U,
    };
    constexpr std::array<u8, 16> kZeroPreserved{};
    constexpr std::array<u8, 6> kMigrationOptions{
        6U, 6U, 0x3CU, 1U, 2U, 0x0AU,
    };
    test.expect_equal(
        migrated.binding_bytes,
        kDefaultBindings,
        "0x00424390 restores the sixteen default bindings before writing"
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
        "unmarked-layout migration clears the sixteen preserved bytes"
    );
    test.expect_equal(
        migrated.primary_directory,
        decoded.record.primary_directory,
        "migration retains the first unmarked string"
    );
}

void test_zero_padding_and_string_bounds(openswd3::test::Context& test) {
    const std::array<u8, 4> empty_unmarked{};
    const auto padded = decode_legacy_environment(empty_unmarked);
    test.expect_equal(
        padded.status,
        LegacyEnvironmentCodecStatus::ok,
        "short reads observe the pre-zeroed 4 KiB window"
    );
    test.expect_equal(
        padded.layout,
        LegacyEnvironmentLayout::unmarked,
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

void test_file_loader_current_and_missing(openswd3::test::Context& test) {
    const TestTree tree;
    tree.write("Env.dat", kCurrentEnvironment);

    LegacyEnvironmentRecord state;
    state.primary_directory = "unchanged";
    int resolver_calls = 0;
    const auto resolver = [&resolver_calls](const std::string_view) {
        ++resolver_calls;
        return std::filesystem::path{};
    };
    const auto loaded = load_legacy_environment(
        tree.path("Env.dat"),
        resolver,
        state
    );
    test.expect_equal(
        loaded.status,
        LegacyEnvironmentLoadStatus::marked_layout_loaded,
        "current file follows the direct load path"
    );
    test.expect_true(
        loaded.original_return_value,
        "marked initial layout reproduces EAX one"
    );
    test.expect_equal(
        resolver_calls,
        0,
        "marked layout never resolves the stored directory"
    );
    test.expect_equal(
        state.primary_directory,
        std::string{"E:\\Game\\swd3\\"},
        "current file replaces the output state"
    );

    LegacyEnvironmentRecord unchanged;
    unchanged.integer_parameter = 0xAABBCCDDU;
    unchanged.primary_directory = "sentinel";
    const LegacyEnvironmentRecord expected = unchanged;
    const auto missing = load_legacy_environment(
        tree.path("missing.dat"),
        resolver,
        unchanged
    );
    test.expect_equal(
        missing.status,
        LegacyEnvironmentLoadStatus::initial_open_failed,
        "missing initial file follows the zero-return path"
    );
    test.expect_false(
        missing.original_return_value,
        "missing initial file reproduces EAX zero"
    );
    test.expect_equal(
        unchanged,
        expected,
        "failed initial open leaves all output state untouched"
    );
}

void test_file_loader_migration_and_no_truncate(
    openswd3::test::Context& test
) {
    const TestTree tree;
    std::vector<u8> unmarked(
        kCurrentEnvironment.begin() + 4,
        kCurrentEnvironment.end()
    );
    constexpr std::array<u8, 8> kStaleTail{
        0xA0U, 0xA1U, 0xA2U, 0xA3U,
        0xA4U, 0xA5U, 0xA6U, 0xA7U,
    };
    unmarked.insert(unmarked.end(), kStaleTail.begin(), kStaleTail.end());
    tree.write("Env.dat", unmarked);

    std::string resolved_raw_path;
    const auto resolver = [&tree, &resolved_raw_path](
        const std::string_view raw_path
    ) {
        resolved_raw_path.assign(raw_path);
        return tree.root();
    };
    LegacyEnvironmentRecord state;
    const auto loaded = load_legacy_environment(
        tree.path("Env.dat"),
        resolver,
        state
    );
    test.expect_equal(
        loaded.status,
        LegacyEnvironmentLoadStatus::unmarked_layout_migrated,
        "markerless file follows the migration path"
    );
    test.expect_false(
        loaded.original_return_value,
        "successful migration still reproduces EAX zero"
    );
    test.expect_true(
        loaded.migration_write_succeeded,
        "migration writes its marked-layout prefix"
    );
    test.expect_true(
        loaded.migrated_reopen_succeeded,
        "migration reopens Env.dat through the stored directory"
    );
    test.expect_equal(
        resolved_raw_path,
        std::string{"E:\\Game\\swd3\\"},
        "resolver receives unmodified stored ANSI path bytes"
    );

    const auto decoded_unmarked = decode_legacy_environment(unmarked);
    const LegacyEnvironmentRecord expected = migrate_unmarked_environment(
        decoded_unmarked.record
    );
    test.expect_equal(
        state,
        expected,
        "reopened migrated file yields the hard-coded migration state"
    );

    const auto expected_prefix = encode_legacy_environment(expected);
    const std::vector<u8> file_bytes = tree.read("Env.dat");
    test.expect_equal(
        file_bytes.size(),
        unmarked.size(),
        "migration writer does not truncate stale file tail bytes"
    );
    test.expect_true(
        std::equal(
            expected_prefix.bytes.begin(),
            expected_prefix.bytes.end(),
            file_bytes.begin()
        ),
        "migration writes the exact marked-layout prefix"
    );
    test.expect_true(
        std::equal(
            kStaleTail.end() - 4,
            kStaleTail.end(),
            file_bytes.end() - 4
        ),
        "bytes beyond the rewritten prefix remain physically stale"
    );
}

void test_failed_migrated_reopen_keeps_old_window(
    openswd3::test::Context& test
) {
    const TestTree tree;
    LegacyEnvironmentRecord old_record;
    old_record.binding_bytes.fill(0x11U);
    old_record.integer_parameter = 0x12345678U;
    old_record.option_bytes.fill(0x22U);
    old_record.primary_directory = "Z:\\missing\\";
    old_record.secondary_directory.clear();
    old_record.trailing_mode = 5U;
    auto current = encode_legacy_environment(old_record).bytes;
    std::vector<u8> unmarked(current.begin() + 4, current.end());
    tree.write("Env.dat", unmarked);

    const auto resolver = [&tree](const std::string_view) {
        return tree.path("absent-directory");
    };
    LegacyEnvironmentRecord state;
    const auto loaded = load_legacy_environment(
        tree.path("Env.dat"),
        resolver,
        state
    );
    test.expect_equal(
        loaded.status,
        LegacyEnvironmentLoadStatus::unmarked_layout_migrated,
        "failed second open does not turn migration into a new error return"
    );
    test.expect_false(
        loaded.migrated_reopen_succeeded,
        "missing stored directory is recorded by the platform port"
    );
    test.expect_equal(
        state.primary_directory,
        std::string{"issing\\"},
        "old buffer is forcibly decoded at the current +0x2E offset"
    );
    test.expect_equal(
        state.trailing_mode,
        u8{5U},
        "failed reopen retains the old window trailing byte"
    );
}

void test_cache_session_marker_writers(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 4> kInitialBytes{
        0x10U, 0x20U, 0x02U, 0x7FU,
    };
    tree.write("Env.dat", kInitialBytes);

    test.expect_true(
        write_legacy_environment_cache_session_marker(
            tree.path("Env.dat"),
            LegacyEnvironmentCacheSessionMarker::active
        ),
        "0x004259B0 writes the active-session marker"
    );
    test.expect_equal(
        tree.read("Env.dat"),
        std::vector<u8>{0x10U, 0x20U, 0x02U, 0x01U},
        "active marker replaces only the physical final byte"
    );

    test.expect_true(
        write_legacy_environment_cache_session_marker(
            tree.path("Env.dat"),
            LegacyEnvironmentCacheSessionMarker::clean
        ),
        "0x004258E0 writes the clean-shutdown marker"
    );
    test.expect_equal(
        tree.read("Env.dat"),
        std::vector<u8>{0x10U, 0x20U, 0x02U, 0x00U},
        "clean marker preserves the prefix and file length"
    );

    test.expect_false(
        write_legacy_environment_cache_session_marker(
            tree.path("missing.dat"),
            LegacyEnvironmentCacheSessionMarker::active
        ),
        "OPEN_EXISTING does not create a missing environment file"
    );

    constexpr std::array<u8, 0> kEmpty{};
    tree.write("empty.dat", kEmpty);
    test.expect_true(
        write_legacy_environment_cache_session_marker(
            tree.path("empty.dat"),
            LegacyEnvironmentCacheSessionMarker::active
        ),
        "ignored negative end seek still writes an existing empty file"
    );
    test.expect_equal(
        tree.read("empty.dat"),
        std::vector<u8>{0x01U},
        "empty file receives one marker byte at its unchanged position"
    );
}

void test_environment_rewrite_preservation(openswd3::test::Context& test) {
    const TestTree tree;
    tree.write("Env.dat", kCurrentEnvironment);

    LegacyEnvironmentRecord replacement;
    replacement.binding_bytes.fill(0x31U);
    replacement.integer_parameter = 0x12345678U;
    replacement.option_bytes.fill(0x42U);
    replacement.preserved_bytes.fill(0xAAU);
    replacement.primary_directory = "primary";
    replacement.secondary_directory = "secondary";
    replacement.trailing_mode = 9U;

    test.expect_true(
        rewrite_legacy_environment(tree.path("Env.dat"), replacement),
        "0x00423AF0 rewrites an existing marked record"
    );
    const std::vector<u8> rewritten_bytes = tree.read("Env.dat");
    const auto rewritten = decode_legacy_environment(rewritten_bytes);
    test.expect_equal(
        rewritten.status,
        LegacyEnvironmentCodecStatus::ok,
        "rewritten marked record decodes"
    );
    test.expect_equal(
        rewritten.record.preserved_bytes,
        decode_legacy_environment(kCurrentEnvironment).record.preserved_bytes,
        "marked rewrite preserves the file's existing sixteen-byte region"
    );
    test.expect_equal(
        rewritten.record.integer_parameter,
        replacement.integer_parameter,
        "marked rewrite replaces caller-owned fields"
    );
    test.expect_equal(
        rewritten.record.primary_directory,
        replacement.primary_directory,
        "marked rewrite replaces the first string"
    );

    const auto marked = encode_legacy_environment(replacement).bytes;
    const std::vector<u8> unmarked(marked.begin() + 4, marked.end());
    tree.write("Env.dat", unmarked);
    test.expect_true(
        rewrite_legacy_environment(tree.path("Env.dat"), replacement),
        "0x00423AF0 rewrites an existing unmarked record"
    );
    const std::vector<u8> migrated_bytes = tree.read("Env.dat");
    const auto migrated = decode_legacy_environment(migrated_bytes);
    constexpr std::array<u8, 16> kZeroPreserved{};
    test.expect_equal(
        migrated.record.preserved_bytes,
        kZeroPreserved,
        "unmarked rewrite clears the sixteen-byte migration region"
    );

    test.expect_false(
        rewrite_legacy_environment(tree.path("missing.dat"), replacement),
        "0x00423AF0 OPEN_EXISTING does not create a file"
    );
}

void test_environment_initialize_and_binding_prefix(
    openswd3::test::Context& test
) {
    const TestTree tree;
    LegacyEnvironmentRecord record;
    record.binding_bytes.fill(0x11U);
    record.integer_parameter = 100U;
    record.option_bytes = {6U, 6U, 0x3CU, 0U, 2U, 0x0AU};
    record.preserved_bytes.fill(0xBBU);
    record.primary_directory = "root";
    record.secondary_directory.clear();
    record.trailing_mode = 2U;

    test.expect_true(
        initialize_legacy_environment(tree.path("Env.dat"), record),
        "0x00423A10 creates a missing Env.dat before rewriting it"
    );
    const std::vector<u8> initialized_bytes = tree.read("Env.dat");
    const auto initialized = decode_legacy_environment(initialized_bytes);
    constexpr std::array<u8, 16> kZeroPreserved{};
    test.expect_equal(
        initialized.record.preserved_bytes,
        kZeroPreserved,
        "newly created environment follows the unmarked clear path"
    );

    constexpr std::array<u8, 16> kBindings{
        0x00U, 0x01U, 0x02U, 0x03U,
        0x04U, 0x05U, 0x06U, 0x07U,
        0x08U, 0x09U, 0x0AU, 0x0BU,
        0x0CU, 0x0DU, 0x0EU, 0x0FU,
    };
    const std::vector<u8> before_prefix = tree.read("Env.dat");
    test.expect_true(
        write_legacy_environment_binding_prefix(
            tree.path("Env.dat"),
            kBindings
        ),
        "0x00423E00 writes the sixteen binding bytes at file offset zero"
    );
    const std::vector<u8> after_prefix = tree.read("Env.dat");
    test.expect_true(
        std::equal(kBindings.begin(), kBindings.end(), after_prefix.begin()),
        "binding prefix replaces the marker and following twelve bytes"
    );
    test.expect_true(
        std::equal(
            before_prefix.begin() + 16,
            before_prefix.end(),
            after_prefix.begin() + 16
        ),
        "binding prefix preserves all bytes after offset sixteen"
    );
    test.expect_false(
        write_legacy_environment_binding_prefix(
            tree.path("missing.dat"),
            kBindings
        ),
        "binding prefix OPEN_EXISTING does not create a missing file"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_current_asset_record(test);
    test_unmarked_layout_migration(test);
    test_zero_padding_and_string_bounds(test);
    test_encoder_bounds_and_lstrlen_prefix(test);
    test_file_loader_current_and_missing(test);
    test_file_loader_migration_and_no_truncate(test);
    test_failed_migrated_reopen_keeps_old_window(test);
    test_cache_session_marker_writers(test);
    test_environment_rewrite_preservation(test);
    test_environment_initialize_and_binding_prefix(test);
    return test.exit_code();
}
