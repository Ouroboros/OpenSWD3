#include "test.hpp"

#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include <array>
#include <chrono>
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

using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyLmfMapLookupStatus;
using openswd3::resource_io::legacy_lmf_lookup_map;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-lmf-archive-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path path(
        const std::string& name
    ) const {
        return root_ / name;
    }

    void write(
        const std::string& name,
        const std::span<const u8> bytes
    ) const {
        std::ofstream output{
            root_ / name,
            std::ios::binary | std::ios::trunc
        };
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
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
    const u32 map_offset,
    const u32 map_span,
    const u32 map_id,
    const u32 reserved = 0U
) {
    append_u32(bytes, map_offset);
    append_u32(bytes, map_span);
    append_u32(bytes, map_id);
    append_u32(bytes, reserved);
}

[[nodiscard]] std::vector<u8> make_archive_with_index() {
    std::vector<u8> bytes;
    append_u32(bytes, 4U);
    append_record(bytes, 0x100U, 0x20U, 7U);
    append_record(bytes, 0x200U, 0x30U, 0xFFFF0001U);
    append_record(bytes, 0U, 0U, 0U);
    return bytes;
}

void test_open_and_header_failures(openswd3::test::Context& test) {
    const TestTree tree;
    test.expect_equal(
        legacy_lmf_lookup_map(tree.path("missing.lmf"), 7U).status,
        LegacyLmfMapLookupStatus::file_open_failed,
        "missing archive fails at open"
    );

    constexpr std::array<u8, 3> kShortHeader{0x04U, 0x00U, 0x00U};
    tree.write("short.lmf", kShortHeader);
    test.expect_equal(
        legacy_lmf_lookup_map(tree.path("short.lmf"), 7U).status,
        LegacyLmfMapLookupStatus::header_read_failed,
        "short header is isolated before legacy uninitialized-byte behavior"
    );

    constexpr std::array<u8, 4> kPastEndOffset{0x05U, 0x00U, 0x00U, 0x00U};
    tree.write("past-end.lmf", kPastEndOffset);
    test.expect_equal(
        legacy_lmf_lookup_map(tree.path("past-end.lmf"), 7U).status,
        LegacyLmfMapLookupStatus::index_offset_out_of_range,
        "tail offset beyond EOF is isolated"
    );
}

void test_lookup_uses_full_map_id_and_first_match(
    openswd3::test::Context& test
) {
    const TestTree tree;
    std::vector<u8> archive = make_archive_with_index();
    tree.write("index.lmf", archive);

    const auto first = legacy_lmf_lookup_map(tree.path("index.lmf"), 7U);
    test.expect_equal(
        first.status,
        LegacyLmfMapLookupStatus::ready,
        "first searchable map is found"
    );
    test.expect_equal(first.map_offset, 0x100U, "map offset comes from record +0");

    const auto full_id = legacy_lmf_lookup_map(
        tree.path("index.lmf"),
        0xFFFF0001U
    );
    test.expect_equal(
        full_id.status,
        LegacyLmfMapLookupStatus::ready,
        "map id comparison uses the full dword"
    );
    test.expect_equal(full_id.map_offset, 0x200U, "second map offset is returned");

    archive.insert(archive.end() - 16, 16U, 0U);
    archive[archive.size() - 32U] = 0x00U;
    archive[archive.size() - 31U] = 0x03U;
    archive[archive.size() - 24U] = 7U;
    tree.write("duplicate.lmf", archive);
    test.expect_equal(
        legacy_lmf_lookup_map(tree.path("duplicate.lmf"), 7U).map_offset,
        0x100U,
        "linear search returns the first matching map"
    );
}

void test_last_physical_record_is_excluded(openswd3::test::Context& test) {
    const TestTree tree;
    std::vector<u8> archive = make_archive_with_index();
    tree.write("sentinel.lmf", archive);

    test.expect_equal(
        legacy_lmf_lookup_map(tree.path("sentinel.lmf"), 0U).status,
        LegacyLmfMapLookupStatus::map_not_found,
        "(tail size - 1) / 16 excludes the final sentinel record"
    );

    archive.push_back(0xA5U);
    tree.write("extra-tail-byte.lmf", archive);
    const auto included = legacy_lmf_lookup_map(
        tree.path("extra-tail-byte.lmf"),
        0U
    );
    test.expect_equal(
        included.status,
        LegacyLmfMapLookupStatus::ready,
        "one trailing byte makes the former sentinel searchable"
    );
    test.expect_equal(included.map_offset, 0U, "included sentinel offset is zero");
}

void test_current_archive(
    openswd3::test::Context& test,
    const std::filesystem::path& archive_path
) {
    const auto first = legacy_lmf_lookup_map(archive_path, 22U);
    test.expect_equal(
        first.status,
        LegacyLmfMapLookupStatus::ready,
        "current archive contains first map id 22"
    );
    test.expect_equal(first.map_offset, 4U, "first current map starts at offset four");

    const auto cached = legacy_lmf_lookup_map(archive_path, 24U);
    test.expect_equal(cached.map_offset, 0x026698A3U, "map 24 offset matches inventory");

    const auto last = legacy_lmf_lookup_map(archive_path, 500U);
    test.expect_equal(last.map_offset, 0x1C16E962U, "last current map is searchable");

    test.expect_equal(
        legacy_lmf_lookup_map(archive_path, 0U).status,
        LegacyLmfMapLookupStatus::map_not_found,
        "current zero sentinel is excluded from search"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_open_and_header_failures(test);
    test_lookup_uses_full_map_id_and_first_match(test);
    test_last_physical_record_is_excluded(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "the optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
        test_current_archive(test, arguments[1]);
    }

    return test.exit_code();
}
