#include "test.hpp"

#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <array>
#include <bit>
#include <limits>
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
using openswd3::world_map::find_legacy_world_map_event;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyRoleSpatialRelocationStatus;
using openswd3::world_map::LegacyWorldMapBusinessStatus;
using openswd3::world_map::LegacyWorldMapEvent;
using openswd3::world_map::LegacyWorldRoleCellBindingStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::relocate_legacy_role_spatially_by_guid;

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

void test_map_event_lookup(openswd3::test::Context& test) {
    const std::array<LegacyWorldMapEvent, 3U> events{
        LegacyWorldMapEvent{
            .field_04 = 7U,
            .field_08 = 10U,
            .name_bytes_with_terminator = {},
        },
        LegacyWorldMapEvent{
            .field_04 = 7U,
            .field_08 = 20U,
            .name_bytes_with_terminator = {},
        },
        LegacyWorldMapEvent{
            .field_04 = 9U,
            .field_08 = 30U,
            .name_bytes_with_terminator = {},
        },
    };

    test.expect_true(
        find_legacy_world_map_event(events, 7U) == &events[0] &&
            find_legacy_world_map_event(events, 9U) == &events[2],
        "sub_40DC30 returns the first head-to-tail id match"
    );
    test.expect_true(
        find_legacy_world_map_event(events, 8U) == nullptr &&
            find_legacy_world_map_event({}, 7U) == nullptr,
        "sub_40DC30 returns null for a miss or an empty head"
    );
}

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
            0U,
            1U,
            i16{0},
            0U,
            0U,
            i16{-20},
            0U,
            std::vector<u8>{0U},
        },
        LegacyLmfOffset14Record{
            0U,
            1U,
            i16{0},
            0U,
            0U,
            i16{30},
            0U,
            std::vector<u8>{0U},
        },
    };
    source.offset1c.records = {
        LegacyLmfOffset1cRecord{
            0U,
            0x1234U,
            5U,
            3U,
            9U,
            0xB201U,
            std::vector<u8>{0U},
        },
        LegacyLmfOffset1cRecord{
            0U,
            9U,
            8U,
            1U,
            10U,
            0U,
            std::vector<u8>{0U},
        },
    };

    const auto result = build_legacy_world_map_business_state(
        source.header, source.post_surface, source.offset14, source.offset1c
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
            first.action.action_id == 0U && first.action.draw_offset_x == 0U &&
            first.action.draw_offset_y == 0xFFFFFFD0U &&
            first.action.field_4c == 0xFFFFU,
        "offset14 action and packed subfields preserve word arithmetic"
    );

    const auto& second = result.state.roles[2];
    test.expect_true(
        second.world_x == 48U && second.world_y == 144U &&
            second.flags == 0x00009001U && second.action.action_id == 0x1234U &&
            second.action.base_variant == 5U && second.field_28 == 0xBU &&
            second.field_2a == 2U,
        "offset1c zero-extended coordinates and action key match 0x004266B8"
    );
    test.expect_equal(
        result.state.spatial_index.row_heads[2U][kLegacySpatialRowPadding + 2U],
        u32{1U},
        "offset14 role is inserted in its packed group and truncated row"
    );
    test.expect_equal(
        result.state.spatial_index.row_heads[1U][kLegacySpatialRowPadding + 9U],
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

    test.expect_true(
        insert_legacy_role_spatially(spatial, roles, 1U, 0U),
        "first role becomes row head"
    );
    test.expect_true(
        insert_legacy_role_spatially(spatial, roles, 2U, 0U),
        "larger equal-row GUID enters before a single node"
    );
    test.expect_true(
        insert_legacy_role_spatially(spatial, roles, 3U, 0U),
        "middle GUID follows the multi-node insertion branch"
    );
    test.expect_true(
        insert_legacy_role_spatially(spatial, roles, 4U, 0U),
        "later sub-tile Y follows the exact walk-to-tail branch"
    );

    const u32 head = spatial.row_heads[0U][kLegacySpatialRowPadding + 1U];
    test.expect_true(
        head == 2U && roles[2].spatial_next_link_32 == 3U &&
            roles[3].spatial_next_link_32 == 1U &&
            roles[1].spatial_next_link_32 == 4U &&
            roles[4].spatial_next_link_32 == 0U,
        "0x00411490 pointer chain is represented by role indices"
    );

    test.expect_true(
        insert_legacy_role_spatially(spatial, roles, 5U, 1U),
        "signed division truncates negative sub-tile Y to zero"
    );
    test.expect_equal(
        spatial.row_heads[1U][kLegacySpatialRowPadding],
        u32{5U},
        "negative one uses logical row zero after x86 truncation"
    );

    roles[5].world_y = std::bit_cast<u32>(i32{-336});
    test.expect_false(
        insert_legacy_role_spatially(spatial, roles, 5U, 1U),
        "row below the twenty-row prefix is rejected by the modern boundary"
    );

    roles[5].world_y = 0U;
    test.expect_false(
        insert_legacy_role_spatially(spatial, roles, 5U, 3U),
        "group three is outside the three pointers allocated at 0x00411620"
    );

    std::array<LegacyWorldRoleRecord, 4U> branch_roles{};
    LegacyRoleSpatialIndex branch_spatial;
    branch_spatial.map_height = 4U;
    for (auto& group : branch_spatial.row_heads) {
        group.resize(44U, 0U);
    }
    branch_roles[1U].world_y = 16U;
    branch_roles[1U].guid = 20U;
    branch_roles[2U].world_y = 16U;
    branch_roles[2U].guid = 10U;
    branch_roles[3U].world_y = 16U;
    branch_roles[3U].guid = 30U;
    test.expect_true(
        insert_legacy_role_spatially(branch_spatial, branch_roles, 1U, 0U) &&
            insert_legacy_role_spatially(
                branch_spatial, branch_roles, 2U, 0U
            ) &&
            branch_roles[1U].spatial_next_link_32 == 2U,
        "single-node fallback appends the lower GUID"
    );
    test.expect_true(
        insert_legacy_role_spatially(branch_spatial, branch_roles, 3U, 0U) &&
            branch_spatial.row_heads[0U][kLegacySpatialRowPadding + 1U] == 3U &&
            branch_roles[3U].spatial_next_link_32 == 1U,
        "multi-node head branch inserts the greater GUID before the head"
    );

    std::array<LegacyWorldRoleRecord, 3U> single_y_roles{};
    single_y_roles[1U].world_y = 31U;
    single_y_roles[1U].guid = 100U;
    single_y_roles[2U].world_y = 16U;
    single_y_roles[2U].guid = 1U;
    test.expect_true(
        insert_legacy_role_spatially(branch_spatial, single_y_roles, 1U, 1U) &&
            insert_legacy_role_spatially(
                branch_spatial, single_y_roles, 2U, 1U
            ) &&
            branch_spatial.row_heads[1U][kLegacySpatialRowPadding + 1U] == 2U &&
            single_y_roles[2U].spatial_next_link_32 == 1U,
        "single-node Y comparison inserts the earlier sub-tile position first"
    );

    std::array<LegacyWorldRoleRecord, 2U> explicit_group_roles{};
    explicit_group_roles[1U].world_y = 0U;
    explicit_group_roles[1U].flags = 0U;
    test.expect_true(
        insert_legacy_role_spatially(
            branch_spatial, explicit_group_roles, 1U, 2U
        ) && branch_spatial.row_heads[2U][kLegacySpatialRowPadding] == 1U &&
            branch_spatial.row_heads[0U][kLegacySpatialRowPadding] == 0U,
        "physical group argument selects the row-head table independently"
    );
}

void test_spatial_relocation_by_guid(openswd3::test::Context& test) {
    const auto make_spatial = [] {
        LegacyRoleSpatialIndex spatial;
        spatial.map_height = 4U;
        for (auto& group : spatial.row_heads) {
            group.resize(44U, 0U);
        }
        return spatial;
    };

    std::array<LegacyWorldRoleRecord, 5U> roles{};
    roles[1U].guid = 10U;
    roles[2U].guid = 20U;
    roles[3U].guid = 30U;
    roles[4U].guid = 40U;

    auto spatial = make_spatial();
    spatial.row_heads[0U][kLegacySpatialRowPadding] = 1U;
    roles[1U].spatial_next_link_32 = 2U;
    const auto removed_head = relocate_legacy_role_spatially_by_guid(
        spatial, roles, 10U, 0U, 0, false
    );
    test.expect_true(
        removed_head.status == LegacyRoleSpatialRelocationStatus::ready &&
            removed_head.legacy_return_role_index == 1U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding] == 2U &&
            roles[1U].spatial_next_link_32 == 0U,
        "remove-only mode returns the removed head while advancing the row"
    );
    const auto removed_single = relocate_legacy_role_spatially_by_guid(
        spatial, roles, 20U, 0U, 0, false
    );
    test.expect_true(
        removed_single.status == LegacyRoleSpatialRelocationStatus::ready &&
            removed_single.legacy_return_role_index == 2U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding] == 0U &&
            roles[2U].spatial_next_link_32 == 0U,
        "single-node removal clears both row head and role next"
    );

    spatial = make_spatial();
    spatial.row_heads[0U][kLegacySpatialRowPadding] = 1U;
    roles[1U].spatial_next_link_32 = 2U;
    roles[2U].spatial_next_link_32 = 3U;
    roles[3U].spatial_next_link_32 = 0U;
    test.expect_true(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 20U, 0U, 0, false
        )
                    .status == LegacyRoleSpatialRelocationStatus::ready &&
            roles[1U].spatial_next_link_32 == 3U &&
            roles[2U].spatial_next_link_32 == 0U,
        "second-node removal uses the head predecessor branch"
    );

    roles[1U].spatial_next_link_32 = 2U;
    roles[2U].spatial_next_link_32 = 3U;
    roles[3U].spatial_next_link_32 = 4U;
    roles[4U].spatial_next_link_32 = 0U;
    test.expect_true(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 40U, 0U, 0, false
        )
                    .status == LegacyRoleSpatialRelocationStatus::ready &&
            roles[3U].spatial_next_link_32 == 0U &&
            roles[4U].spatial_next_link_32 == 0U,
        "deep-node removal rewrites the immediate predecessor link"
    );

    spatial = make_spatial();
    roles[1U].world_y = 32U;
    roles[1U].flags = 1U;
    roles[1U].spatial_next_link_32 = 0U;
    spatial.row_heads[0U][kLegacySpatialRowPadding] = 1U;
    const auto reinserted = relocate_legacy_role_spatially_by_guid(
        spatial, roles, 10U, 0U, 0, true
    );
    test.expect_true(
        reinserted.status == LegacyRoleSpatialRelocationStatus::ready &&
            reinserted.legacy_return_role_index == 0U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding] == 0U &&
            spatial.row_heads[1U][kLegacySpatialRowPadding + 2U] == 1U,
        "physical zero final argument removes then reinserts by role flags and Y"
    );

    spatial = make_spatial();
    roles[1U].world_y = 0U;
    roles[1U].flags = 0U;
    roles[1U].spatial_next_link_32 = 0U;
    spatial.row_heads[0U][kLegacySpatialRowPadding] = 1U;
    const auto high_guid_miss = relocate_legacy_role_spatially_by_guid(
        spatial, roles, 0x0001000AU, 0U, 0, false
    );
    test.expect_true(
        high_guid_miss.status ==
                LegacyRoleSpatialRelocationStatus::role_not_found &&
            high_guid_miss.legacy_return_role_index == 0U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding] == 1U,
        "full 32-bit GUID argument cannot alias a matching low word"
    );

    spatial.row_heads[0U][kLegacySpatialRowPadding + 1U] = 2U;
    roles[2U].spatial_next_link_32 = 0U;
    test.expect_true(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 20U, 0U, 0, false
        )
                    .status == LegacyRoleSpatialRelocationStatus::ready &&
            spatial.row_heads[0U][kLegacySpatialRowPadding + 1U] == 0U,
        "row scan advances from the requested row until map height"
    );
    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 10U, 0U, 1, false
        )
            .status,
        LegacyRoleSpatialRelocationStatus::role_not_found,
        "first row is an inclusive signed lower bound"
    );
    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 10U, 0U, 4, false
        )
            .status,
        LegacyRoleSpatialRelocationStatus::role_not_found,
        "first row equal to map height follows the original empty diagnostic path"
    );
    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 10U, 0U, std::numeric_limits<i32>::max(), false
        )
            .status,
        LegacyRoleSpatialRelocationStatus::role_not_found,
        "a first row above map height exits before any row dereference"
    );

    roles[1U].flags = 3U;
    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(spatial, roles, 10U, 0U, 0, true)
            .status,
        LegacyRoleSpatialRelocationStatus::reinsertion_failed,
        "invalid destination group is isolated only after the original unlink point"
    );
    test.expect_true(
        spatial.row_heads[0U][kLegacySpatialRowPadding] == 0U &&
            roles[1U].spatial_next_link_32 == 0U,
        "reinsertion failure preserves the completed unlink side effect"
    );

    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 10U, 3U, 0, false
        )
            .status,
        LegacyRoleSpatialRelocationStatus::invalid_group,
        "invalid search group is rejected at the modern boundary"
    );
    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 10U, 0U, -21, false
        )
            .status,
        LegacyRoleSpatialRelocationStatus::first_row_out_of_range,
        "row before the twenty-row prefix is rejected at the modern boundary"
    );
    spatial.row_heads[0U][kLegacySpatialRowPadding] =
        static_cast<u32>(roles.size());
    test.expect_equal(
        relocate_legacy_role_spatially_by_guid(
            spatial, roles, 10U, 0U, 0, false
        )
            .status,
        LegacyRoleSpatialRelocationStatus::broken_link,
        "invalid role link is isolated before a host dereference"
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

    const auto result = bind_legacy_world_role_cells(roles, 1U, 3U, 4U, grid);
    test.expect_equal(
        result.status,
        LegacyWorldRoleCellBindingStatus::ready,
        "unflagged off-map records retain an index without reading it"
    );
    test.expect_true(
        result.roles_bound == 2U && result.out_of_bounds_indices == 1U &&
            roles[1].map_cell_pointer_32 == 6U &&
            roles[1].flags == 0xFFAFFFFFU && roles[1].action.mode_flags == 7U,
        "0x0040F2C1 cell index and flag projection preserve exact masks without repeating the pre-update mode reset"
    );

    roles[2].flags |= 0x00000100U;
    const auto flagged = bind_legacy_world_role_cells(roles, 2U, 3U, 4U, grid);
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
        source.header, source.post_surface, source.offset14, source.offset1c
    );
    test.expect_equal(
        result.status,
        LegacyWorldMapBusinessStatus::invalid_physical_state,
        "business conversion does not accept incomplete physical state"
    );

    source.header.status = LegacyLmfMapHeaderStatus::ready;
    source.offset14.records.resize(256U);
    result = build_legacy_world_map_business_state(
        source.header, source.post_surface, source.offset14, source.offset1c
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
        source.header, source.post_surface, source.offset14, source.offset1c
    );
    test.expect_true(
        result.status == LegacyWorldMapBusinessStatus::ready &&
            result.state.roles.size() == 1U,
        "a rejected group-three record never reaches the original spatial insertion"
    );

    source.offset14.records.front().field_08 = i16{0};
    result = build_legacy_world_map_business_state(
        source.header, source.post_surface, source.offset14, source.offset1c
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
    test_map_event_lookup(test);
    test_business_conversion(test);
    test_spatial_insertion_order(test);
    test_spatial_relocation_by_guid(test);
    test_cell_binding(test);
    test_invalid_and_capacity_statuses(test);
    return test.exit_code();
}
