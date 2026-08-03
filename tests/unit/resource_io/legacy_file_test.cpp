#include "test.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <algorithm>
#include <array>
#include <chrono>
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
using openswd3::resource_io::LegacyFile;
using openswd3::resource_io::LegacyFileAccess;
using openswd3::resource_io::LegacyFileCreation;
using openswd3::resource_io::LegacyFileTime;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-file-" + std::to_string(unique_value));
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

void test_open_modes_and_exact_little_endian_io(
    openswd3::test::Context& test
) {
    const TestTree tree;
    LegacyFile file;

    test.expect_false(
        file.open(
            tree.path("missing.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        ),
        "OPEN_EXISTING rejects a missing file"
    );
    test.expect_true(
        !file.error_message().empty() && file.error_message().size() < 64U,
        "failure text stays inside the legacy 64-byte field"
    );

    test.expect_true(
        file.open(
            tree.path("created.bin"),
            LegacyFileCreation::open_always,
            LegacyFileAccess::read_write
        ),
        "OPEN_ALWAYS creates a missing file"
    );
    test.expect_true(file.write_u8(0x41U), "one-byte exact write succeeds");
    test.expect_true(
        file.write_u32(0x12345678U),
        "four-byte exact write succeeds"
    );
    test.expect_equal(file.size(), 5U, "size follows exact writes");
    test.expect_true(file.close(), "close succeeds");
    test.expect_true(file.close(), "close is idempotent");
    test.expect_true(
        file.error_message().empty(),
        "successful close clears the error field"
    );

    const std::vector<u8> bytes = tree.read("created.bin");
    constexpr std::array<u8, 5> kExpected{
        0x41U, 0x78U, 0x56U, 0x34U, 0x12U,
    };
    test.expect_true(
        std::ranges::equal(bytes, kExpected),
        "four-byte helper writes x86 little-endian stack bytes"
    );

    test.expect_true(
        file.open(
            tree.path("created.bin"),
            LegacyFileCreation::open_always,
            LegacyFileAccess::read_write
        ),
        "OPEN_ALWAYS reopens an existing file"
    );
    test.expect_equal(
        file.size(),
        5U,
        "OPEN_ALWAYS preserves existing contents"
    );
    test.expect_true(file.close(), "preserved file closes");

    test.expect_true(
        file.open(
            tree.path("created.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        ),
        "read-only open succeeds"
    );
    test.expect_false(file.write_u8(0xFFU), "read-only handle rejects write");
    test.expect_true(file.close(), "read-only handle closes");

    test.expect_true(
        file.open(
            tree.path("created.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::write
        ),
        "write-only open succeeds"
    );
    u8 value = 0xCCU;
    test.expect_false(file.read_u8(value), "write-only handle rejects read");
    test.expect_true(file.close(), "write-only handle closes");
}

void test_short_read_contract(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 2> kBytes{0x11U, 0x22U};
    tree.write("short.bin", kBytes);

    LegacyFile file;
    test.expect_true(
        file.open(
            tree.path("short.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        ),
        "short-read fixture opens"
    );

    std::array<u8, 4> buffer{};
    u32 requested = 4U;
    test.expect_true(file.read(buffer, requested), "EOF short read succeeds");
    test.expect_equal(requested, 2U, "read length is rewritten to actual bytes");

    test.expect_equal(
        file.seek_begin_one_based(0),
        1U,
        "rewind returns a one-based position"
    );
    u32 value = 0xAABBCCDDU;
    test.expect_true(file.read_u32(value), "four-byte helper accepts short read");
    test.expect_equal(
        value,
        0xAABB2211U,
        "short four-byte read only overwrites the bytes actually read"
    );
    test.expect_true(file.close(), "short-read fixture closes");
}

void test_seek_size_and_truncate(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 8> kBytes{0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    tree.write("seek.bin", kBytes);

    LegacyFile file;
    test.expect_true(
        file.open(
            tree.path("seek.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read_write
        ),
        "seek fixture opens"
    );
    test.expect_equal(file.seek_begin_one_based(0), 1U, "begin seek is one-based");
    test.expect_equal(
        file.seek_current_one_based(2),
        3U,
        "current seek is one-based"
    );
    test.expect_equal(file.seek_end_one_based(-1), 8U, "end seek is one-based");

    u32 position = 0xFFFFFFFFU;
    test.expect_true(file.current_position(position), "current position query succeeds");
    test.expect_equal(position, 7U, "position query remains zero-based");
    test.expect_equal(file.seek_begin_one_based(3), 4U, "truncate position is selected");
    test.expect_true(
        file.truncate_at_current_position(),
        "truncate uses the current position"
    );
    test.expect_equal(file.size(), 3U, "truncate changes the file size");
    test.expect_true(file.close(), "seek fixture closes");

    position = 0xA5A5A5A5U;
    test.expect_equal(
        file.seek_begin_one_based(0),
        1U,
        "invalid-handle raw zero still becomes one"
    );
    test.expect_false(
        file.current_position(position),
        "invalid-handle position query fails"
    );
    test.expect_equal(
        position,
        0xA5A5A5A5U,
        "failed position query leaves output untouched"
    );
    test.expect_equal(
        file.size(),
        0xFFFFFFFFU,
        "invalid-handle size keeps the legacy sentinel"
    );
}

void test_mapping_lifecycle(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 4> kBytes{0x10U, 0x20U, 0x30U, 0x40U};
    tree.write("mapped.bin", kBytes);

    LegacyFile file;
    test.expect_false(
        file.create_read_only_mapping(),
        "invalid file cannot create a mapping"
    );
    test.expect_true(
        file.open(
            tree.path("mapped.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        ),
        "mapping fixture opens"
    );
    test.expect_true(
        file.create_read_only_mapping(),
        "read-only mapping object is created"
    );

    const u8* view = file.map_view();
    test.expect_true(view != nullptr, "whole-file view is created");
    if (view != nullptr) {
        test.expect_true(
            std::ranges::equal(std::span{view, kBytes.size()}, kBytes),
            "mapped bytes match the file"
        );
        test.expect_false(
            file.close_view(view + 1),
            "mismatched view pointer is rejected"
        );
        test.expect_true(file.close_view(view), "matching view pointer closes");
    }

    view = file.map_view();
    test.expect_true(view != nullptr, "view can be established again");
    view = file.map_view();
    test.expect_true(view != nullptr, "remap replaces the previous view");
    test.expect_true(
        file.close_mapping(),
        "mapping close first releases its active view"
    );
    test.expect_true(file.close_mapping(), "mapping close is idempotent");
    test.expect_true(file.close(), "mapping fixture closes");
}

void test_last_write_time(openswd3::test::Context& test) {
    const TestTree tree;
    constexpr std::array<u8, 1> kByte{0x7FU};
    tree.write("timestamp.bin", kByte);

    LegacyFile file;
    test.expect_true(
        file.open(
            tree.path("timestamp.bin"),
            LegacyFileCreation::open_existing,
            LegacyFileAccess::read
        ),
        "timestamp fixture opens"
    );
    LegacyFileTime time{};
    test.expect_true(file.last_write_time(time), "last-write time is available");
    test.expect_true(time.low != 0U || time.high != 0U, "timestamp is populated");
    test.expect_true(file.close(), "timestamp fixture closes");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_open_modes_and_exact_little_endian_io(test);
    test_short_read_contract(test);
    test_seek_size_and_truncate(test);
    test_mapping_lifecycle(test);
    test_last_write_time(test);
    return test.exit_code();
}
