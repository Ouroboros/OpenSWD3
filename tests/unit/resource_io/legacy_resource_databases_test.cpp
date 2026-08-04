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

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::u8;
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

    void write(
        const char* name,
        const std::span<const u8> bytes
    ) const {
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
        databases.path_bytes().empty(),
        "null path view exports no bytes"
    );
}

void test_real_database_set(
    openswd3::test::Context& test,
    const std::filesystem::path& root
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
        databases.maps_file().size(),
        162929U,
        "real MAPS.DAT size"
    );
    test.expect_equal(
        databases.path_bytes().size(),
        std::size_t{23114U},
        "real PATH.DAT size"
    );
    test.expect_equal(
        databases.talk_file().size(),
        371450U,
        "real TALK1.DAT size"
    );

    const auto path_bytes = databases.path_bytes();
    test.expect_true(
        path_bytes.size() >= 4U &&
            path_bytes[0] == 0x4DU && path_bytes[1] == 0x5AU &&
            path_bytes[2] == 0x4AU && path_bytes[3] == 0x00U,
        "real PATH.DAT mapped prefix"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_failure_order(test);
    test_complete_database_set(test);
    test_mapping_failure_is_not_an_open_failure(test);
    if (argument_count == 2) {
        test_real_database_set(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
