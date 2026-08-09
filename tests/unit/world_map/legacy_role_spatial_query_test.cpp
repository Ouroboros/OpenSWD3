#include "test.hpp"

#include "openswd3/world_map/legacy_role_spatial_query.hpp"

#include <array>
#include <limits>

namespace {

using openswd3::compat::u32;
using openswd3::world_map::find_legacy_collision_role_at_tile;
using openswd3::world_map::find_legacy_role_at_tile;
using openswd3::world_map::kLegacyRoleCollisionFlag;
using openswd3::world_map::kLegacyRoleNotFound;
using openswd3::world_map::kLegacyRoleSpatiallyActiveFlag;
using openswd3::world_map::LegacyWorldRoleRecord;

void make_queryable(
    LegacyWorldRoleRecord& role,
    const u32 tile_x,
    const u32 tile_y,
    const u32 width,
    const u32 height
) {
    role.world_x = tile_x << 4U;
    role.world_y = tile_y << 4U;
    role.flags = kLegacyRoleSpatiallyActiveFlag;
    role.talk_script_id = 1U;
    role.action.field_2c = width;
    role.action.field_30 = height;
}

void test_index_zero_and_short_counts(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2> roles{};
    make_queryable(roles[0], 2U, 3U, 1U, 1U);

    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, 2U, 3U),
        kLegacyRoleNotFound,
        "role zero is never scanned"
    );
    test.expect_equal(
        find_legacy_role_at_tile(roles, 1U, 2U, 3U),
        kLegacyRoleNotFound,
        "a count of one exits before scanning"
    );
    test.expect_equal(
        find_legacy_role_at_tile(roles, 0U, 2U, 3U),
        kLegacyRoleNotFound,
        "a count of zero follows the unsigned early exit"
    );
}

void test_activity_gates_and_order(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 5> roles{};
    make_queryable(roles[1], 4U, 6U, 2U, 2U);
    roles[1].talk_script_id = 0U;
    make_queryable(roles[2], 4U, 6U, 2U, 2U);
    roles[2].flags = 0xFFFFFFFFU & ~kLegacyRoleSpatiallyActiveFlag;
    make_queryable(roles[3], 4U, 6U, 2U, 2U);
    make_queryable(roles[4], 4U, 6U, 2U, 2U);

    test.expect_equal(
        find_legacy_role_at_tile(roles, 5U, 4U, 6U),
        u32{3U},
        "the first active matching role wins"
    );
}

void test_half_open_tile_bounds(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2> roles{};
    make_queryable(roles[1], 10U, 20U, 3U, 2U);
    roles[1].world_x += 15U;
    roles[1].world_y += 15U;

    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, 10U, 20U),
        u32{1U},
        "world coordinates are floored to 16-pixel tiles"
    );
    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, 12U, 21U),
        u32{1U},
        "last included tile is found"
    );
    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, 13U, 21U),
        kLegacyRoleNotFound,
        "right edge is excluded"
    );
    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, 12U, 22U),
        kLegacyRoleNotFound,
        "bottom edge is excluded"
    );
    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, 9U, 20U),
        kLegacyRoleNotFound,
        "tile before the left edge is excluded"
    );
}

void test_extent_addition_wraps(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2> roles{};
    const u32 start_x = 0x0FFFFFFFU;
    make_queryable(roles[1], start_x, 1U, 1U, 1U);
    roles[1].world_x = 0xFFFFFFF0U;
    roles[1].action.field_2c = 0xF0000002U;

    test.expect_equal(
        find_legacy_role_at_tile(roles, 2U, start_x, 1U),
        kLegacyRoleNotFound,
        "x86 dword extent addition wraps instead of saturating"
    );
}

void test_collision_flag_filter(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2> roles{};
    make_queryable(roles[1], 7U, 8U, 1U, 1U);

    test.expect_equal(
        find_legacy_collision_role_at_tile(roles, 2U, 7U, 8U),
        kLegacyRoleNotFound,
        "spatial match without flag bit 13 is rejected"
    );

    roles[1].flags |= kLegacyRoleCollisionFlag;
    test.expect_equal(
        find_legacy_collision_role_at_tile(roles, 2U, 7U, 8U),
        u32{1U},
        "flag bit 13 admits the spatial match"
    );

    roles[1].flags &= ~kLegacyRoleCollisionFlag;
    roles[1].flags |= 0x00200000U;
    test.expect_equal(
        find_legacy_collision_role_at_tile(roles, 2U, 7U, 8U),
        kLegacyRoleNotFound,
        "bit 21 does not satisfy test ch,20h"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_index_zero_and_short_counts(test);
    test_activity_gates_and_order(test);
    test_half_open_tile_bounds(test);
    test_extent_addition_wraps(test);
    test_collision_flag_filter(test);
    return test.exit_code();
}
