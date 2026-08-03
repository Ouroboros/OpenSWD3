#include "test.hpp"

#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyLmfMapFormat;
using openswd3::resource_io::LegacyLmfMapHeaderStatus;
using openswd3::resource_io::LegacyLmfMapLookupStatus;
using openswd3::resource_io::legacy_lmf_lookup_map;
using openswd3::resource_io::legacy_lmf_read_map_header;

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

void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

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

[[nodiscard]] std::vector<u8> make_map_header(const u32 signature) {
    std::vector<u8> bytes(0x2000U);
    write_u32(bytes, 0x00U, signature);
    write_u32(bytes, 0x04U, 0x84U);
    write_u32(bytes, 0x14U, 0x11111111U);
    write_u32(bytes, 0x18U, 0x22222222U);
    write_u32(bytes, 0x1CU, 0x33333333U);
    write_u32(bytes, 0x20U, 0x44444444U);
    write_u16(bytes, 0x84U, 120U);
    write_u16(bytes, 0x86U, 90U);
    write_u16(bytes, 0x88U, 16U);
    write_u16(bytes, 0x8AU, 10247U);
    write_u16(bytes, 0x8CU, 2U);
    constexpr std::array<u8, 4> kName{'m', 'a', 'p', 0U};
    std::copy(kName.begin(), kName.end(), bytes.begin() + 0x96);
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

void test_map_header_fields_and_formats(openswd3::test::Context& test) {
    const TestTree tree;
    for (const auto [signature, format] : {
             std::pair{0x7046534DU, LegacyLmfMapFormat::msfp},
             std::pair{0x3246534DU, LegacyLmfMapFormat::msf2},
         }) {
        const std::vector<u8> bytes = make_map_header(signature);
        tree.write("header.lmf", bytes);
        const auto header = legacy_lmf_read_map_header(
            tree.path("header.lmf"),
            0U
        );

        test.expect_equal(
            header.status,
            LegacyLmfMapHeaderStatus::ready,
            "both original map signatures are accepted"
        );
        test.expect_equal(header.format, format, "signature selects map format");
        test.expect_true(
            header.offset_04 == 0x84U &&
                header.offset_14 == 0x11111111U &&
                header.offset_18 == 0x22222222U &&
                header.offset_1c == 0x33333333U &&
                header.offset_20 == 0x44444444U,
            "five directly consumed dword offsets retain their physical values"
        );
        test.expect_true(
            header.width == 120U &&
                header.height == 90U &&
                header.field_88 == 16U &&
                header.field_8a == 10247U &&
                header.layers == 2U,
            "five words are zero-extended into runtime dwords"
        );
        test.expect_equal(
            header.name_bytes_with_terminator,
            std::vector<u8>{'m', 'a', 'p', 0U},
            "map name copy includes its NUL terminator"
        );
        test.expect_equal(
            header.raw_table_offset,
            0x9AU,
            "raw table starts immediately after the copied name"
        );
    }
}

void test_map_header_failures(openswd3::test::Context& test) {
    const TestTree tree;
    std::vector<u8> bytes = make_map_header(0x3246534DU);

    tree.write("short-header.lmf", std::span<const u8>{bytes}.first(0x1000U));
    test.expect_equal(
        legacy_lmf_read_map_header(tree.path("short-header.lmf"), 0U).status,
        LegacyLmfMapHeaderStatus::header_read_failed,
        "short fixed header is isolated"
    );

    write_u32(bytes, 0x00U, 0x12345678U);
    tree.write("signature.lmf", bytes);
    test.expect_equal(
        legacy_lmf_read_map_header(tree.path("signature.lmf"), 0U).status,
        LegacyLmfMapHeaderStatus::unsupported_signature,
        "unknown signature follows the original failure path"
    );

    bytes = make_map_header(0x3246534DU);
    write_u32(bytes, 0x04U, 0x80000000U);
    tree.write("data-seek.lmf", bytes);
    test.expect_equal(
        legacy_lmf_read_map_header(tree.path("data-seek.lmf"), 0U).status,
        LegacyLmfMapHeaderStatus::data_seek_failed,
        "signed legacy seek failure is preserved"
    );

    bytes = make_map_header(0x3246534DU);
    write_u16(bytes, 0x8EU, 1U);
    tree.write("tile-high.lmf", bytes);
    test.expect_equal(
        legacy_lmf_read_map_header(tree.path("tile-high.lmf"), 0U).status,
        LegacyLmfMapHeaderStatus::tile_count_high_word_nonzero,
        "nonzero word at +0x8E rejects the map"
    );

    bytes = make_map_header(0x3246534DU);
    std::fill(bytes.begin() + 0x96, bytes.end(), 0x41U);
    tree.write("name.lmf", bytes);
    test.expect_equal(
        legacy_lmf_read_map_header(tree.path("name.lmf"), 0U).status,
        LegacyLmfMapHeaderStatus::unterminated_name,
        "unterminated name is isolated at the fixed read boundary"
    );
}

[[nodiscard]] u32 read_stream_u32(std::ifstream& input) {
    std::array<u8, 4> bytes{};
    input.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
    return static_cast<u32>(bytes[0]) |
        (static_cast<u32>(bytes[1]) << 8U) |
        (static_cast<u32>(bytes[2]) << 16U) |
        (static_cast<u32>(bytes[3]) << 24U);
}

void test_all_current_map_headers(
    openswd3::test::Context& test,
    const std::filesystem::path& archive_path
) {
    struct IndexedMap {
        u32 id{};
        u32 offset{};
    };

    std::ifstream input{archive_path, std::ios::binary};
    const u32 index_offset = read_stream_u32(input);
    input.seekg(0, std::ios::end);
    const auto file_size = static_cast<u32>(input.tellg());
    const u32 record_count = (file_size - index_offset - 1U) / 16U;
    input.seekg(index_offset, std::ios::beg);

    std::vector<IndexedMap> maps;
    maps.reserve(record_count);
    for (u32 record = 0U; record < record_count; ++record) {
        const u32 map_offset = read_stream_u32(input);
        static_cast<void>(read_stream_u32(input));
        const u32 map_id = read_stream_u32(input);
        static_cast<void>(read_stream_u32(input));
        maps.push_back({map_id, map_offset});
    }
    input.close();

    bool all_lookups_match = true;
    bool all_headers_match = true;
    for (const IndexedMap& map : maps) {
        const auto lookup = legacy_lmf_lookup_map(archive_path, map.id);
        all_lookups_match = all_lookups_match &&
            lookup.status == LegacyLmfMapLookupStatus::ready &&
            lookup.map_offset == map.offset;

        const auto header = legacy_lmf_read_map_header(
            archive_path,
            map.offset
        );
        all_headers_match = all_headers_match &&
            header.status == LegacyLmfMapHeaderStatus::ready &&
            header.format == LegacyLmfMapFormat::msf2 &&
            !header.name_bytes_with_terminator.empty() &&
            header.name_bytes_with_terminator.back() == 0U &&
            header.raw_table_offset >= 0x98U &&
            header.raw_table_offset <= 0xA5U;
    }

    test.expect_equal(record_count, 309U, "current archive has 309 searchable maps");
    test.expect_true(all_lookups_match, "all current map ids resolve to their indexed offsets");
    test.expect_true(all_headers_match, "all current map headers satisfy the recovered contract");
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

    const auto first_header = legacy_lmf_read_map_header(archive_path, 4U);
    test.expect_true(
        first_header.width == 120U &&
            first_header.height == 90U &&
            first_header.field_88 == 16U &&
            first_header.field_8a == 10247U &&
            first_header.layers == 2U,
        "first current map runtime words match inventory"
    );

    test_all_current_map_headers(test, archive_path);
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_open_and_header_failures(test);
    test_lookup_uses_full_map_id_and_first_match(test);
    test_last_physical_record_is_excluded(test);
    test_map_header_fields_and_formats(test);
    test_map_header_failures(test);

    test.expect_true(
        argument_count == 1 || argument_count == 2,
        "the optional argument names the current huge.lmf"
    );
    if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
        test_current_archive(test, arguments[1]);
    }

    return test.exit_code();
}
