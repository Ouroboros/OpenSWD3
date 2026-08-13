#include "test.hpp"

#include "openswd3/world_map/legacy_world_role_lookup.hpp"

#include <array>

namespace {

using openswd3::compat::u32;
using openswd3::world_map::find_legacy_world_role_by_guid;
using openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
using openswd3::world_map::kLegacyWorldRoleNotFound;
using openswd3::world_map::legacy_world_role_guid_at;
using openswd3::world_map::LegacyWorldRoleGuidStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::lookup_legacy_world_role_by_guid;
using openswd3::world_map::resolve_legacy_world_role_selector;

void test_guid_scan_and_wrapper(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 3U> roles{};
    roles[0].guid = 9U;
    roles[0].flags = kLegacyWorldGuidLookupSkipBit;
    roles[1].guid = 9U;
    roles[2].guid = 9U;

    test.expect_equal(
        find_legacy_world_role_by_guid(roles, 9U),
        u32{1U},
        "0x0040C020 skips bit-28 roles and returns the first clear match"
    );
    test.expect_equal(
        find_legacy_world_role_by_guid(roles, 8U),
        kLegacyWorldRoleNotFound,
        "0x0040C020 returns FFFFFFFF for a missing GUID"
    );

    u32 role_index = 0x12345678U;
    test.expect_true(
        lookup_legacy_world_role_by_guid(roles, 9U, role_index) &&
            role_index == 1U,
        "0x0040C100 stores a successful lookup result"
    );
    test.expect_true(
        !lookup_legacy_world_role_by_guid(roles, 8U, role_index) &&
            role_index == kLegacyWorldRoleNotFound,
        "0x0040C100 overwrites the output with FFFFFFFF on failure"
    );
}

void test_index_to_guid_boundary(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0xBEEFU;

    const auto valid = legacy_world_role_guid_at(roles, 1U);
    test.expect_true(
        valid.status == LegacyWorldRoleGuidStatus::ready &&
            valid.guid == 0xBEEFU,
        "0x0040C060 returns the low-word GUID for a valid index"
    );

    const auto invalid = legacy_world_role_guid_at(roles, 2U);
    test.expect_equal(
        invalid.status,
        LegacyWorldRoleGuidStatus::invalid_role_index,
        "0x0040C060 unsafe diagnostic read is isolated at the modern boundary"
    );
}

void test_controlled_selector_and_failure_output(
    openswd3::test::Context& test
) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 7U;

    u32 role_index = 0x12345678U;
    test.expect_true(
        resolve_legacy_world_role_selector(roles, 0xFFFEU, 9U, role_index) &&
            role_index == 9U,
        "0x0040C0D0 accepts FFFE without validating the index"
    );
    test.expect_true(
        resolve_legacy_world_role_selector(roles, 7U, 0U, role_index) &&
            role_index == 1U,
        "0x0040C0D0 delegates ordinary selectors to 0x0040C100"
    );
    test.expect_true(
        !resolve_legacy_world_role_selector(roles, 8U, 0U, role_index) &&
            role_index == kLegacyWorldRoleNotFound,
        "failed ordinary selector keeps 0x0040C100 FFFFFFFF output"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_guid_scan_and_wrapper(test);
    test_index_to_guid_boundary(test);
    test_controlled_selector_and_failure_output(test);
    return test.exit_code();
}
