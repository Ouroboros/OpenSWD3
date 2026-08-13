#include "test.hpp"

#include "openswd3/resource_io/legacy_resource_databases.hpp"

#include <algorithm>
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
using openswd3::resource_io::LegacyMapsPayloadStatus;
using openswd3::resource_io::LegacyResourceDatabaseStatus;
using openswd3::resource_io::LegacyResourceDatabases;

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-resource-databases-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream output{root_ / name, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

constexpr std::array<u8, 3> kMaps{0x10U, 0x20U, 0x30U};
constexpr std::array<u8, 4> kPath{0x40U, 0x50U, 0x60U, 0x70U};
constexpr std::array<u8, 2> kTalk{0x80U, 0x90U};

std::vector<u8> talk_file_with_entry(
    const std::size_t entry_index,
    const openswd3::compat::u32 data_offset,
    const std::span<const u8> payload
) {
    const std::size_t table = 0x200U + entry_index * 4U;
    const std::size_t data = 0x200U + data_offset;
    std::vector<u8> bytes(std::max(table + 4U, data + payload.size()), 0U);
    bytes[table] = static_cast<u8>(data_offset);
    bytes[table + 1U] = static_cast<u8>(data_offset >> 8U);
    bytes[table + 2U] = static_cast<u8>(data_offset >> 16U);
    bytes[table + 3U] = static_cast<u8>(data_offset >> 24U);
    std::ranges::copy(
        payload,
        bytes.begin() + static_cast<std::vector<u8>::difference_type>(data)
    );
    return bytes;
}

void test_failure_order(openswd3::test::Context& test) {
    {
        const TestTree tree;
        LegacyResourceDatabases databases;
        const auto result = databases.initialize(tree.root());
        test.expect_equal(
            result.status,
            LegacyResourceDatabaseStatus::maps_open_failed,
            "missing MAPS.DAT stops before later files"
        );
    }

    {
        const TestTree tree;
        tree.write("MAPS.DAT", kMaps);
        LegacyResourceDatabases databases;
        const auto result = databases.initialize(tree.root());
        test.expect_equal(
            result.status,
            LegacyResourceDatabaseStatus::path_open_failed,
            "missing PATH.DAT follows successful MAPS.DAT"
        );
        test.expect_equal(
            databases.maps_file().size(),
            3U,
            "MAPS.DAT remains open after PATH.DAT failure"
        );
    }

    {
        const TestTree tree;
        tree.write("MAPS.DAT", kMaps);
        tree.write("PATH.DAT", kPath);
        LegacyResourceDatabases databases;
        const auto result = databases.initialize(tree.root());
        test.expect_equal(
            result.status,
            LegacyResourceDatabaseStatus::talk_open_failed,
            "missing TALK1.DAT follows PATH.DAT mapping"
        );
        test.expect_true(
            result.path_mapping_created && result.path_view_created,
            "PATH.DAT remains mapped after TALK1.DAT failure"
        );
    }
}

void test_complete_database_set(openswd3::test::Context& test) {
    const TestTree tree;
    tree.write("MAPS.DAT", kMaps);
    tree.write("PATH.DAT", kPath);
    tree.write("TALK1.DAT", kTalk);

    LegacyResourceDatabases databases;
    const auto result = databases.initialize(tree.root());
    test.expect_equal(
        result.status,
        LegacyResourceDatabaseStatus::ready,
        "all three legacy databases open"
    );
    test.expect_true(
        result.path_mapping_created && result.path_view_created,
        "PATH.DAT whole-file mapping is established"
    );
    test.expect_equal(
        databases.path_bytes().size(),
        kPath.size(),
        "PATH.DAT view keeps the complete file size"
    );
    test.expect_true(
        std::ranges::equal(databases.path_bytes(), kPath),
        "PATH.DAT view keeps exact bytes"
    );
    test.expect_equal(
        databases.talk_file().size(),
        2U,
        "TALK1.DAT remains an ordinary file object"
    );
}

void test_mapping_failure_is_not_an_open_failure(
    openswd3::test::Context& test
) {
    const TestTree tree;
    tree.write("MAPS.DAT", kMaps);
    tree.write("PATH.DAT", std::span<const u8>{});
    tree.write("TALK1.DAT", kTalk);

    LegacyResourceDatabases databases;
    const auto result = databases.initialize(tree.root());
    test.expect_equal(
        result.status,
        LegacyResourceDatabaseStatus::ready,
        "ignored PATH.DAT mapping failure does not stop TALK1.DAT open"
    );
    test.expect_false(
        result.path_mapping_created,
        "empty PATH.DAT cannot create the legacy mapping"
    );
    test.expect_false(
        result.path_view_created,
        "failed mapping leaves the legacy path view null"
    );
    test.expect_true(
        databases.path_bytes().empty(), "null path view exports no bytes"
    );
}

void test_maps_payload_reload(openswd3::test::Context& test) {
    const TestTree tree;
    std::vector<u8> maps(0x206U, 0xA5U);
    maps[0x200U] = 0x10U;
    maps[0x201U] = 0x20U;
    maps[0x202U] = 0x30U;
    maps[0x203U] = 0x40U;
    maps[0x204U] = 0x50U;
    maps[0x205U] = 0x60U;
    tree.write("MAPS.DAT", maps);
    tree.write("PATH.DAT", kPath);
    tree.write("TALK1.DAT", kTalk);

    LegacyResourceDatabases databases;
    test.expect_equal(
        databases.initialize(tree.root()).status,
        LegacyResourceDatabaseStatus::ready,
        "payload source databases open"
    );
    const auto loaded = databases.reload_maps_payload();
    test.expect_true(
        loaded.status == LegacyMapsPayloadStatus::ready &&
            loaded.requested_size == 6U && loaded.actual_size == 6U,
        "MAPS reload skips exactly the original 0x200-byte prefix"
    );
    constexpr std::array<u8, 6U> kExpected{
        0x10U,
        0x20U,
        0x30U,
        0x40U,
        0x50U,
        0x60U,
    };
    test.expect_true(
        std::ranges::equal(databases.maps_payload_bytes(), kExpected),
        "MAPS payload owner retains exact post-prefix bytes"
    );
    databases.mutable_maps_payload_bytes()[0] = 0x77U;
    test.expect_equal(
        databases.maps_payload_bytes()[0],
        u8{0x77U},
        "MAPS payload remains mutable like dword_4C9A10"
    );

    const TestTree short_tree;
    short_tree.write("MAPS.DAT", kMaps);
    short_tree.write("PATH.DAT", kPath);
    short_tree.write("TALK1.DAT", kTalk);
    test.expect_equal(
        databases.initialize(short_tree.root()).status,
        LegacyResourceDatabaseStatus::ready,
        "short replacement database reopens"
    );
    test.expect_true(
        databases.maps_payload_bytes().empty(),
        "database reinitialization releases the preceding payload"
    );
    test.expect_equal(
        databases.reload_maps_payload().status,
        LegacyMapsPayloadStatus::file_smaller_than_prefix,
        "a file shorter than the physical prefix is rejected"
    );
}

void test_talk_window_loading(openswd3::test::Context& test) {
    const TestTree tree;
    tree.write("MAPS.DAT", kMaps);
    tree.write("PATH.DAT", kPath);
    const std::array<u8, 4U> first_payload{0x02U, 0x04U, 0xFFU, 0xFFU};
    const std::array<u8, 3U> second_payload{0xA1U, 0x00U, 0x2AU};
    const auto talk1 = talk_file_with_entry(248U, 0x1000U, first_payload);
    const auto talk2 = talk_file_with_entry(42U, 0x1100U, second_payload);
    tree.write("TALK1.DAT", talk1);
    tree.write("TALK2.DAT", talk2);

    LegacyResourceDatabases databases;
    test.expect_equal(
        databases.initialize(tree.root()).status,
        LegacyResourceDatabaseStatus::ready,
        "TALK window fixture databases open"
    );
    std::array<u8, openswd3::resource_io::kLegacyTalkWindowSize> window{};
    const auto first = databases.load_talk_story_window(248, window, true);
    test.expect_true(
        first.status == openswd3::resource_io::LegacyTalkWindowStatus::ready &&
            first.file_number == 1U && first.entry_index == 248U &&
            first.data_offset == 0x1000U && first.actual_size == 4U &&
            std::ranges::equal(
                first_payload,
                std::span<const u8>{window}.first(first_payload.size())
            ) &&
            window[4] == 0x0CU,
        "TALK1 entry table selects and pre-fills the exact 0x8000-byte window"
    );

    const auto second = databases.load_talk_story_window(2042, window, false);
    test.expect_true(
        second.status == openswd3::resource_io::LegacyTalkWindowStatus::ready &&
            second.file_number == 2U && second.entry_index == 42U &&
            second.data_offset == 0x1100U && second.actual_size == 3U &&
            std::ranges::equal(
                second_payload,
                std::span<const u8>{window}.first(second_payload.size())
            ) &&
            window[3] == first_payload[3],
        "TALK2 transfer retains the unread tail like sub_42E480"
    );

    const auto direct =
        databases.load_talk_data_window(2U, 0x1100U, window, false);
    test.expect_true(
        direct.status == openswd3::resource_io::LegacyTalkWindowStatus::ready &&
            direct.data_offset == 0x1100U && direct.actual_size == 3U,
        "same-file branch reloads a direct TALK data offset"
    );
    test.expect_equal(
        databases.load_talk_story_window(-1, window, true).status,
        openswd3::resource_io::LegacyTalkWindowStatus::invalid_story_id,
        "negative story ids cannot select a TALK file"
    );
}

void test_real_database_set(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    LegacyResourceDatabases databases;
    const auto result = databases.initialize(root);
    test.expect_equal(
        result.status,
        LegacyResourceDatabaseStatus::ready,
        "real MAPS/PATH/TALK1 database set opens"
    );
    test.expect_true(
        result.path_mapping_created && result.path_view_created,
        "real PATH.DAT whole-file view is established"
    );
    test.expect_equal(
        databases.maps_file().size(), 162929U, "real MAPS.DAT size"
    );
    test.expect_equal(
        databases.path_bytes().size(), std::size_t{23114U}, "real PATH.DAT size"
    );
    test.expect_equal(
        databases.talk_file().size(), 371450U, "real TALK1.DAT size"
    );

    const auto maps_payload = databases.reload_maps_payload();
    test.expect_true(
        maps_payload.status == LegacyMapsPayloadStatus::ready &&
            maps_payload.requested_size == 162417U &&
            maps_payload.actual_size == 162417U &&
            databases.maps_payload_bytes().size() == 162417U,
        "real MAPS.DAT reload owns the complete post-0x200 payload"
    );
    const auto payload = databases.maps_payload_bytes();
    test.expect_true(
        payload.size() >= 0x14U && payload[0] == 0x65U && payload[4] == 0x28U &&
            payload[5] == 0x36U && payload[0x10U] == 0x38U &&
            payload[0x11U] == 0x23U,
        "real MAPS.DAT payload header matches the locked game-data bytes"
    );

    const auto path_bytes = databases.path_bytes();
    test.expect_true(
        path_bytes.size() >= 4U && path_bytes[0] == 0x4DU &&
            path_bytes[1] == 0x5AU && path_bytes[2] == 0x4AU &&
            path_bytes[3] == 0x00U,
        "real PATH.DAT mapped prefix"
    );

    std::array<u8, openswd3::resource_io::kLegacyTalkWindowSize> window{};
    const auto talk1 = databases.load_talk_story_window(248, window, true);
    test.expect_true(
        talk1.status == openswd3::resource_io::LegacyTalkWindowStatus::ready &&
            talk1.file_number == 1U && talk1.entry_index == 248U &&
            talk1.data_offset == 34307U && window[0] == 0x02U &&
            window[1] == 0x04U && window[2] == 0x5BU && window[3] == 0x00U,
        "real TALK1 story 248 resolves to its exact table offset and bytes"
    );
    const auto talk2 = databases.load_talk_story_window(2042, window, false);
    test.expect_true(
        talk2.status == openswd3::resource_io::LegacyTalkWindowStatus::ready &&
            talk2.file_number == 2U && talk2.entry_index == 42U &&
            talk2.data_offset == 29414U && window[0] == 0x02U &&
            window[1] == 0x04U && window[2] == 0x4CU && window[3] == 0x00U,
        "real TALK2 story 2042 resolves to its exact table offset and bytes"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_failure_order(test);
    test_complete_database_set(test);
    test_mapping_failure_is_not_an_open_failure(test);
    test_maps_payload_reload(test);
    test_talk_window_loading(test);
    if (argument_count == 2) {
        test_real_database_set(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
