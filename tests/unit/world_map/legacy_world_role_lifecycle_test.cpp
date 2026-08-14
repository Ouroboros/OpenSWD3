#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/world_map/legacy_world_role_lifecycle.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::world_map::kLegacyWorldRoleCapacity;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleTableResetStatus;
using openswd3::world_map::reset_legacy_world_role_table;

void fill_role_bytes(
    const std::span<LegacyWorldRoleRecord> roles, const std::byte value
) {
    std::ranges::fill(std::as_writable_bytes(roles), value);
}

[[nodiscard]] bool
is_initialized_empty_role(const LegacyWorldRoleRecord& role) {
    LegacyWorldRoleRecord expected{};
    openswd3::asset_runtime::initialize_legacy_action_record(expected.action);
    return std::ranges::equal(
        std::as_bytes(std::span{&role, 1U}),
        std::as_bytes(std::span{&expected, 1U})
    );
}

void test_full_physical_table_reset(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, kLegacyWorldRoleCapacity> roles{};
    fill_role_bytes(roles, std::byte{0xA5U});
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    for (std::size_t index = 0U; index < payloads.size(); ++index) {
        payloads[index] = {static_cast<u8>(index), 0U};
    }
    roles[0U].path_payload_pointer_32 = 1U;
    roles[1U].path_payload_pointer_32 = 0U;
    roles[2U].path_payload_pointer_32 = 3U;

    const auto result = reset_legacy_world_role_table(roles, payloads, 2);
    test.expect_true(
        result.status == LegacyWorldRoleTableResetStatus::ready &&
            result.payload_slots_scanned == 3U &&
            result.payload_owners_released == 2U &&
            result.roles_zeroed == kLegacyWorldRoleCapacity &&
            result.action_records_initialized == kLegacyWorldRoleCapacity,
        "sub_40F3B0 scans the inclusive live range before both table passes"
    );
    test.expect_true(
        payloads[0U].empty() && payloads[0U].capacity() == 0U &&
            !payloads[1U].empty() && payloads[2U].empty() &&
            payloads[2U].capacity() == 0U && !payloads[3U].empty(),
        "only non-null +38 owners inside the inclusive range are freed"
    );
    test.expect_true(
        std::ranges::all_of(roles, is_initialized_empty_role),
        "all 256 role records are zeroed before sub_40DC00 initialization"
    );
}

void test_negative_highest_skips_only_release(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    fill_role_bytes(roles, std::byte{0x5AU});
    roles[0U].path_payload_pointer_32 = 1U;
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    payloads[0U] = {1U, 2U, 3U};

    const auto result =
        reset_legacy_world_role_table(roles, payloads, i32{-17});
    test.expect_true(
        result.status == LegacyWorldRoleTableResetStatus::ready &&
            result.payload_slots_scanned == 0U &&
            result.payload_owners_released == 0U && result.roles_zeroed == 2U &&
            result.action_records_initialized == 2U && !payloads[0U].empty() &&
            std::ranges::all_of(roles, is_initialized_empty_role),
        "every negative highest index skips release but still resets roles"
    );
}

void test_modern_bounds_are_transactional(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[0U].guid = 0x1234U;
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    payloads[0U] = {1U};

    const auto missing_role = reset_legacy_world_role_table(roles, payloads, 2);
    test.expect_true(
        missing_role.status ==
                LegacyWorldRoleTableResetStatus::
                    active_role_range_out_of_bounds &&
            roles[0U].guid == 0x1234U && !payloads[0U].empty(),
        "a truncated modern owner is rejected before release or reset"
    );

    const auto excessive_highest = reset_legacy_world_role_table(
        roles, payloads, static_cast<i32>(kLegacyWorldRoleCapacity)
    );
    test.expect_true(
        excessive_highest.status ==
                LegacyWorldRoleTableResetStatus::
                    highest_role_index_out_of_range &&
            roles[0U].guid == 0x1234U && !payloads[0U].empty(),
        "a highest index beyond the physical 256-role table is isolated"
    );

    std::array<LegacyWorldRoleRecord, kLegacyWorldRoleCapacity + 1U>
        oversized{};
    const auto excessive_span =
        reset_legacy_world_role_table(oversized, payloads, -1);
    test.expect_equal(
        excessive_span.status,
        LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity,
        "a modern role span cannot exceed the physical table"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_full_physical_table_reset(test);
    test_negative_highest_skips_only_release(test);
    test_modern_bounds_are_transactional(test);
    return test.exit_code();
}
