#include "test.hpp"

#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <array>
#include <bit>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::resource_io::LegacyLmfMapHeader;
using openswd3::resource_io::LegacyLmfMapHeaderStatus;
using openswd3::resource_io::LegacyLmfOffset14Directory;
using openswd3::resource_io::LegacyLmfOffset14DirectoryStatus;
using openswd3::resource_io::LegacyLmfOffset14Record;
using openswd3::resource_io::LegacyLmfOffset1cDirectory;
using openswd3::resource_io::LegacyLmfOffset1cDirectoryStatus;
using openswd3::resource_io::LegacyLmfOffset1cRecord;
using openswd3::resource_io::LegacyLmfPostSurfaceRecord;
using openswd3::resource_io::LegacyLmfPostSurfaceRecords;
using openswd3::resource_io::LegacyLmfPostSurfaceRecordsStatus;
using openswd3::world_map::bind_legacy_world_role_cells;
using openswd3::world_map::build_legacy_world_map_business_state;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldMapBusinessStatus;
using openswd3::world_map::LegacyWorldRoleCellBindingStatus;
using openswd3::world_map::LegacyWorldRoleRecord;

struct ReadyPhysicalState {
    ReadyPhysicalState() {
        header.status = LegacyLmfMapHeaderStatus::ready;
        header.width = 8U;
        header.height = 10U;
        post_surface.status = LegacyLmfPostSurfaceRecordsStatus::ready;
        offset14.status = LegacyLmfOffset14DirectoryStatus::ready;
        offset1c.status = LegacyLmfOffset1cDirectoryStatus::ready;
    }

    LegacyLmfMapHeader header;
    LegacyLmfPostSurfaceRecords post_surface;
    LegacyLmfOffset14Directory offset14;
    LegacyLmfOffset1cDirectory offset1c;
};

void test_business_conversion(openswd3::test::Context& test) {
    ReadyPhysicalState source;
    source.post_surface.records = {
        LegacyLmfPostSurfaceRecord{
            1U, 2U, 3U, 4U, 5U, std::vector<u8>{'a', 0U}
        },
        LegacyLmfPostSurfaceRecord{
            6U, 7U, 8U, 9U, 10U, std::vector<u8>{'b', 0U}
        },
    };
    source.offset14.records = {
        LegacyLmfOffset14Record{
            0U,
            0U,
            i16{-1},
            5U,
            0U,
            i16{2},
            0xA302U,
            std::vector<u8>{0U},
        },
        LegacyLmfOffset14Record{
            0U, 1U, i16{0}, 0U, 0U, i16{-20}, 0U,
            std::vector<u8>{0U},
        },
        LegacyLmfOffset14Record{
            0U, 1U, i16{0}, 0U, 0U, i16{30}, 0U,
            std::vector<u8>{0U},
        },
    };
    source.offset1c.records = {
        LegacyLmfOffset1cRecord{
            0U, 0x1234U, 5U, 3U, 9U, 0xB201U,
            std::vector<u8>{0U},
        },
        LegacyLmfOffset1cRecord{
            0U, 9U, 8U, 1U, 10U, 0U, std::vector<u8>{0U},
        },
    };

    const auto result = build_legacy_world_map_business_state(
        source.header,
        source.post_surface,
        source.offset14,
        source.offset1c
    );
    test.expect_equal(
        result.status,
        LegacyWorldMapBusinessStatus::ready,
        "ready physical directories build business state"
    );
    test.expect_true(
        result.state.events.size() == 2U &&
            result.state.events[0].field_04 == 7U &&
            result.state.events[0].name_bytes_with_terminator[0] == 'b' &&
            result.state.events[1].field_04 == 2U,
        "post-surface nodes preserve the original prepend order"
    );
    test.expect_true(
        result.state.roles.size() == 3U &&
            result.state.offset14_role_count == 1U &&
            result.state.offset1c_role_count == 1U,
        "role zero is retained and rejected map records do not raise count"
    );

    const auto& first = result.state.roles[1];
    test.expect_true(
        first.world_x == 0xFFFFFFF0U && first.world_y == 32U &&
            first.flags == 0x00009002U && first.guid == 0xFFFFU,
        "offset14 signed coordinates and packed flags match 0x00426324"
    );
    test.expect_true(
        first.field_28 == 0xAU && first.field_2a == 3U &&
            first.action.action_id == 0U &&
            first.action.draw_offset_x == 0U &&
            first.action.draw_offset_y == 0xFFFFFFD0U &&
            first.action.field_4c == 0xFFFFU,
        "offset14 action and packed subfields preserve word arithmetic"
    );

    const auto& second = result.state.roles[2];
    test.expect_true(
        second.world_x == 48U && second.world_y == 144U &&
            second.flags == 0x00009001U &&
            second.action.action_id == 0x1234U &&
            second.action.base_variant == 5U &&
            second.field_28 == 0xBU && second.field_2a == 2U,
        "offset1c zero-extended coordinates and action key match 0x004266B8"
    );
    test.expect_equal(
        result.state.spatial_index.row_heads[2U][
            kLegacySpatialRowPadding + 2U
        ],
        u32{1U},
        "offset14 role is inserted in its packed group and truncated row"
    );
    test.expect_equal(
        result.state.spatial_index.row_heads[1U][
            kLegacySpatialRowPadding + 9U
        ],
        u32{2U},
        "offset1c role is inserted in its packed group and row"
    );
}

void test_spatial_insertion_order(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 6U> roles{};
    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 4U;
    for (auto& group : spatial.row_heads) {
        group.resize(44U, 0U);
    }

    roles[1].world_y = 16U;
    roles[1].guid = 10U;
    roles[2].world_y = 16U;
    roles[2].guid = 20U;
    roles[3].world_y = 16U;
    roles[3].guid = 15U;
    roles[4].world_y = 31U;
    roles[4].guid = 1U;
    roles[5].world_y = 0xFFFFFFFFU;
    roles[5].guid = 4U;
    roles[5].flags = 1U;

    test.expect_true(insert_legacy_role_spatially(spatial, roles, 1U),
                     "first role becomes row head");
    test.expect_true(insert_legacy_role_spatially(spatial, roles, 2U),
                     "larger equal-row GUID enters before a single node");
    test.expect_true(insert_legacy_role_spatially(spatial, roles, 3U),
                     "middle GUID follows the multi-node insertion branch");
    test.expect_true(insert_legacy_role_spatially(spatial, roles, 4U),
                     "later sub-tile Y follows the exact walk-to-tail branch");

    const u32 head = spatial.row_heads[0U][kLegacySpatialRowPadding + 1U];
    test.expect_true(
        head == 2U && roles[2].spatial_next_link_32 == 3U &&
            roles[3].spatial_next_link_32 == 1U &&
            roles[1].spatial_next_link_32 == 4U &&
            roles[4].spatial_next_link_32 == 0U,
        "0x00411490 pointer chain is represented by role indices"
    );

    test.expect_true(insert_legacy_role_spatially(spatial, roles, 5U),
                     "signed division truncates negative sub-tile Y to zero");
    test.expect_equal(
        spatial.row_heads[1U][kLegacySpatialRowPadding],
        u32{5U},
        "negative one uses logical row zero after x86 truncation"
    );

    roles[5].world_y = std::bit_cast<u32>(i32{-336});
    test.expect_false(
        insert_legacy_role_spatially(spatial, roles, 5U),
        "row below the twenty-row prefix is rejected by the modern boundary"
    );

    roles[5].world_y = 0U;
    roles[5].flags = 3U;
    test.expect_false(
        insert_legacy_role_spatially(spatial, roles, 5U),
        "packed group three is outside the three pointers allocated at 0x00411620"
    );
}

void test_cell_binding(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 3U> roles{};
    roles[1].world_x = 32U;
    roles[1].world_y = 16U;
    roles[1].flags = 0xFFFFFFFFU;
    roles[1].action.mode_flags = 7U;
    roles[2].world_x = 0xFFFFFFF0U;
    roles[2].world_y = 0U;
    roles[2].flags = 0x00009000U;

    std::vector<u8> grid(4U * 3U * 4U, 0U);
    constexpr std::size_t cell_six = 6U * 4U;
    grid[cell_six + 1U] = 0xA8U;

    const auto result = bind_legacy_world_role_cells(
        roles,
        1U,
        3U,
        4U,
        grid
    );
    test.expect_equal(
        result.status,
        LegacyWorldRoleCellBindingStatus::ready,
        "unflagged off-map records retain an index without reading it"
    );
    test.expect_true(
        result.roles_bound == 2U && result.out_of_bounds_indices == 1U &&
            roles[1].map_cell_pointer_32 == 6U &&
            roles[1].flags == 0xFFAFFFFFU &&
            roles[1].action.mode_flags == 0U,
        "0x0040F2C1 cell index and flag projection preserve exact masks"
    );

    roles[2].flags |= 0x00000100U;
    const auto flagged = bind_legacy_world_role_cells(
        roles,
        2U,
        3U,
        4U,
        grid
    );
    test.expect_equal(
        flagged.status,
        LegacyWorldRoleCellBindingStatus::flagged_cell_out_of_bounds,
        "a cell that the original would dereference is checked explicitly"
    );
}

void test_invalid_and_capacity_statuses(openswd3::test::Context& test) {
    ReadyPhysicalState source;
    source.header.status = LegacyLmfMapHeaderStatus::unsupported_signature;
    auto result = build_legacy_world_map_business_state(
        source.header,
        source.post_surface,
        source.offset14,
        source.offset1c
    );
    test.expect_equal(
        result.status,
        LegacyWorldMapBusinessStatus::invalid_physical_state,
        "business conversion does not accept incomplete physical state"
    );

    source.header.status = LegacyLmfMapHeaderStatus::ready;
    source.offset14.records.resize(256U);
    result = build_legacy_world_map_business_state(
        source.header,
        source.post_surface,
        source.offset14,
        source.offset1c
    );
    test.expect_equal(
        result.status,
        LegacyWorldMapBusinessStatus::role_capacity_exceeded,
        "modern boundary stops the original fixed role-array overflow"
    );

    source.offset14.records = {LegacyLmfOffset14Record{
        0U, 1U, i16{0}, 0U, 0U, i16{30}, 3U, std::vector<u8>{0U}
    }};
    result = build_legacy_world_map_business_state(
        source.header,
        source.post_surface,
        source.offset14,
        source.offset1c
    );
    test.expect_true(
        result.status == LegacyWorldMapBusinessStatus::ready &&
            result.state.roles.size() == 1U,
        "a rejected group-three record never reaches the original spatial insertion"
    );

    source.offset14.records.front().field_08 = i16{0};
    result = build_legacy_world_map_business_state(
        source.header,
        source.post_surface,
        source.offset14,
        source.offset1c
    );
    test.expect_equal(
        result.status,
        LegacyWorldMapBusinessStatus::unsupported_spatial_group,
        "current assets never select the adjacent unallocated group-three pointer"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_business_conversion(test);
    test_spatial_insertion_order(test);
    test_cell_binding(test);
    test_invalid_and_capacity_statuses(test);
    return test.exit_code();
}
