#include "test.hpp"

#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldActiveObjectSlotCount;
using openswd3::world_map::LegacyMapsRoleSourceRecord;
using openswd3::world_map::LegacyMapsWorldDatabase;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleTransferContext;
using openswd3::world_map::LegacyWorldRoleTransferState;
using openswd3::world_map::LegacyWorldRoleTransferStatus;
using openswd3::world_map::transfer_legacy_world_role;
using openswd3::world_map::write_legacy_maps_role_source_record;

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u32
read_u32(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

struct Fixture {
    Fixture() {
        source.payload_offset = 0x20U;
        source.logical_map_id = 5U;
        source.guid = 9U;
        source.flags = 0x0100U;
        database.role_sources.push_back(source);
        static_cast<void>(write_legacy_maps_role_source_record(
            payload, database.role_sources.front()
        ));

        roles.resize(3U);
        roles[1].guid = 9U;
        roles[1].flags = 0x00004080U;
        roles[1].talk_script_id = 0x33U;
    }

    [[nodiscard]] LegacyWorldRoleTransferStatus
    run(const LegacyWorldRoleTransferContext* context) {
        return transfer_legacy_world_role(
            payload, database, roles, 1U, context, state
        );
    }

    std::vector<u8> payload = std::vector<u8>(0x80U, 0U);
    LegacyMapsWorldDatabase database;
    LegacyMapsRoleSourceRecord source;
    std::vector<LegacyWorldRoleRecord> roles;
    LegacyWorldRoleTransferState state;
};

void test_common_and_aligned_object_paths(openswd3::test::Context& test) {
    Fixture without_path;
    without_path.roles[1].path_data_id = 0U;
    test.expect_equal(
        without_path.run(nullptr),
        LegacyWorldRoleTransferStatus::ready,
        "a role without Path data skips the 72-object scan"
    );
    test.expect_true(
        without_path.state.party_role_count == 2U &&
            without_path.state.party_role_indices[1] == 1U &&
            without_path.roles[1].talk_script_id == 0U &&
            without_path.roles[1].flags == 0x80U,
        "D741 appends the role and applies the exact common Talk/flag state"
    );

    Fixture no_matching_object;
    no_matching_object.roles[1].path_data_id = 4U;
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        empty_slots;
    const LegacyWorldRoleTransferContext no_match_context{
        .active_object_slots = empty_slots,
        .spatial_index = nullptr,
        .surface_grid = {},
        .map_width = 0U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        no_matching_object.run(&no_match_context),
        LegacyWorldRoleTransferStatus::ready,
        "an absent active object follows D64F directly to party append"
    );
    test.expect_true(
        no_matching_object.database.role_sources[0].flags == 0x0100U &&
            no_matching_object.state.active_object_slots_reset == 0U,
        "MAPS flags and object slots change only after a matching slot"
    );

    Fixture aligned;
    aligned.roles[1].path_data_id = 4U;
    aligned.roles[1].world_x = 0x20U;
    aligned.roles[1].world_y = 0x30U;
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        aligned_slots;
    aligned_slots[5].bytes.fill(0x55U);
    write_u16(aligned_slots[5].bytes, 0U, 1U);
    const LegacyWorldRoleTransferContext aligned_context{
        .active_object_slots = aligned_slots,
        .spatial_index = nullptr,
        .surface_grid = {},
        .map_width = 0U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        aligned.run(&aligned_context),
        LegacyWorldRoleTransferStatus::ready,
        "an aligned active object needs no surface or spatial runtime"
    );
    test.expect_true(
        aligned.database.role_sources[0].flags == 0x0180U &&
            aligned.payload[0x34U] == 0x80U &&
            aligned.payload[0x35U] == 0x01U &&
            std::ranges::all_of(
                aligned_slots[5].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            aligned.state.active_object_slots_reset == 1U &&
            aligned.state.aligned_roles == 0U,
        "D6FA patches MAPS and clears the full 0x21C-byte object slot"
    );
}

void test_nonaligned_surface_and_spatial_path(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].path_data_id = 4U;
    fixture.roles[1].world_x = 0x24U;
    fixture.roles[1].world_y = 0x24U;
    fixture.roles[1].map_cell_pointer_32 = 10U;
    fixture.roles[1].action.field_2c = 2U;
    fixture.roles[1].action.field_30 = 2U;

    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 5U;
    for (auto& group : spatial.row_heads) {
        group.resize(45U, 0U);
    }
    test.expect_true(
        insert_legacy_role_spatially(spatial, fixture.roles, 1U),
        "the moving role begins in spatial row two"
    );

    std::vector<u8> surface_grid(5U * 4U * sizeof(u32), 0xFFU);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_slots;
    active_slots[0].bytes.fill(0U);
    write_u16(active_slots[0].bytes, 0U, 1U);
    write_u16(active_slots[0].bytes, 2U, 0U);
    active_slots[0].bytes[0x1CU] = 4U;
    const LegacyWorldRoleTransferContext context{
        .active_object_slots = active_slots,
        .spatial_index = &spatial,
        .surface_grid = surface_grid,
        .map_width = 4U,
        .selected_guid = 9U,
    };

    test.expect_equal(
        fixture.run(&context),
        LegacyWorldRoleTransferStatus::ready,
        "the nonaligned D678 path completes"
    );
    test.expect_true(
        fixture.roles[1].world_x == 0x30U &&
            fixture.roles[1].world_y == 0x30U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding + 2U] == 0U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding + 3U] == 1U &&
            fixture.state.aligned_roles == 1U &&
            fixture.state.spatial_roles_relocated == 1U,
        "direction four adds four until aligned and moves the GUID chain"
    );

    constexpr u32 selected_mask = 0xCF7FFEFFU;
    test.expect_true(
        read_u32(surface_grid, 10U * 4U) == selected_mask &&
            read_u32(surface_grid, 11U * 4U) == selected_mask &&
            read_u32(surface_grid, 14U * 4U) == selected_mask &&
            read_u32(surface_grid, 15U * 4U) == selected_mask &&
            read_u32(surface_grid, 9U * 4U) == 0xFFFFFFFFU &&
            fixture.state.cleared_surface_cells == 5U,
        "AE20 clears the selected-role mask over the exact 2x2 footprint"
    );
}

void test_surface_special_cases(openswd3::test::Context& test) {
    const auto run_case = [&](const u32 width,
                              const u32 height,
                              const std::size_t second_cell,
                              const char* const message) {
        Fixture fixture;
        fixture.roles[1].path_data_id = 4U;
        fixture.roles[1].world_x = 0x24U;
        fixture.roles[1].world_y = 0x20U;
        fixture.roles[1].map_cell_pointer_32 = 5U;
        fixture.roles[1].action.field_2c = width;
        fixture.roles[1].action.field_30 = height;

        LegacyRoleSpatialIndex spatial;
        spatial.map_height = 4U;
        for (auto& group : spatial.row_heads) {
            group.resize(44U, 0U);
        }
        static_cast<void>(
            insert_legacy_role_spatially(spatial, fixture.roles, 1U)
        );

        std::vector<u8> surface_grid(16U * sizeof(u32), 0xFFU);
        std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
            slots;
        slots[0].bytes.fill(0U);
        write_u16(slots[0].bytes, 0U, 1U);
        slots[0].bytes[0x1CU] = 7U;
        const LegacyWorldRoleTransferContext context{
            .active_object_slots = slots,
            .spatial_index = &spatial,
            .surface_grid = surface_grid,
            .map_width = 4U,
            .selected_guid = 1U,
        };

        test.expect_equal(
            fixture.run(&context), LegacyWorldRoleTransferStatus::ready, message
        );
        test.expect_true(
            read_u32(surface_grid, 5U * 4U) == 0xCF7FFFFFU &&
                read_u32(surface_grid, second_cell * 4U) == 0xCF7FFFFFU &&
                fixture.state.cleared_surface_cells == 2U,
            "AE20 special footprint clears exactly two cells"
        );
    };

    run_case(2U, 1U, 6U, "the 1x2 contiguous special case completes");
    run_case(1U, 2U, 9U, "the 2x1 row-stride special case completes");
}

void test_checked_runtime_boundaries(openswd3::test::Context& test) {
    Fixture missing_slots;
    missing_slots.roles[1].path_data_id = 4U;
    test.expect_equal(
        missing_slots.run(nullptr),
        LegacyWorldRoleTransferStatus::active_object_slots_required,
        "Path roles require the physical 72-slot scan state"
    );

    Fixture bad_direction;
    bad_direction.roles[1].path_data_id = 4U;
    bad_direction.roles[1].world_x = 0x24U;
    bad_direction.roles[1].world_y = 0x20U;
    bad_direction.roles[1].map_cell_pointer_32 = 0U;
    std::vector<u8> surface_grid(4U * sizeof(u32), 0xFFU);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    slots[0].bytes.fill(0U);
    write_u16(slots[0].bytes, 0U, 1U);
    slots[0].bytes[0x1CU] = 8U;
    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 1U;
    for (auto& group : spatial.row_heads) {
        group.resize(41U, 0U);
    }
    const LegacyWorldRoleTransferContext context{
        .active_object_slots = slots,
        .spatial_index = &spatial,
        .surface_grid = surface_grid,
        .map_width = 2U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        bad_direction.run(&context),
        LegacyWorldRoleTransferStatus::direction_out_of_range,
        "direction bytes outside the eight-entry tables are explicit"
    );
    test.expect_true(
        read_u32(surface_grid, 0U) == 0xCF7FFFFFU &&
            bad_direction.state.party_role_count == 1U &&
            bad_direction.state.active_object_slots_reset == 0U,
        "AE20 precedes direction lookup while later transfer effects do not run"
    );

    Fixture impossible_alignment;
    impossible_alignment.roles[1].path_data_id = 4U;
    impossible_alignment.roles[1].world_x = 0x21U;
    impossible_alignment.roles[1].world_y = 0x20U;
    impossible_alignment.roles[1].map_cell_pointer_32 = 0U;
    std::vector<u8> alignment_grid(4U * sizeof(u32), 0xFFU);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        alignment_slots;
    alignment_slots[0].bytes.fill(0U);
    write_u16(alignment_slots[0].bytes, 0U, 1U);
    alignment_slots[0].bytes[0x1CU] = 7U;
    const LegacyWorldRoleTransferContext alignment_context{
        .active_object_slots = alignment_slots,
        .spatial_index = &spatial,
        .surface_grid = alignment_grid,
        .map_width = 2U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        impossible_alignment.run(&alignment_context),
        LegacyWorldRoleTransferStatus::direction_cannot_align,
        "non-quarter-pixel coordinates cannot terminate the original step loop"
    );

    Fixture bad_cursor;
    bad_cursor.roles[1].path_data_id = 4U;
    bad_cursor.roles[1].world_x = 0x24U;
    bad_cursor.roles[1].world_y = 0x20U;
    bad_cursor.roles[1].map_cell_pointer_32 = 0U;
    std::vector<u8> cursor_grid(4U * sizeof(u32), 0xFFU);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        cursor_slots;
    cursor_slots[0].bytes.fill(0U);
    write_u16(cursor_slots[0].bytes, 0U, 1U);
    write_u16(cursor_slots[0].bytes, 2U, 0x7FFFU);
    const LegacyWorldRoleTransferContext cursor_context{
        .active_object_slots = cursor_slots,
        .spatial_index = &spatial,
        .surface_grid = cursor_grid,
        .map_width = 2U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        bad_cursor.run(&cursor_context),
        LegacyWorldRoleTransferStatus::path_cursor_out_of_range,
        "a masked Path cursor cannot escape the 0x21C-byte object slot"
    );

    Fixture bad_surface;
    bad_surface.roles[1].path_data_id = 4U;
    bad_surface.roles[1].world_x = 0x24U;
    bad_surface.roles[1].world_y = 0x20U;
    bad_surface.roles[1].map_cell_pointer_32 = 4U;
    std::vector<u8> short_grid(4U * sizeof(u32), 0xFFU);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        surface_slots;
    surface_slots[0].bytes.fill(0U);
    write_u16(surface_slots[0].bytes, 0U, 1U);
    surface_slots[0].bytes[0x1CU] = 7U;
    const LegacyWorldRoleTransferContext surface_context{
        .active_object_slots = surface_slots,
        .spatial_index = &spatial,
        .surface_grid = short_grid,
        .map_width = 2U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        bad_surface.run(&surface_context),
        LegacyWorldRoleTransferStatus::surface_footprint_out_of_range,
        "the old surface anchor is checked before its first u32 access"
    );

    Fixture missing_spatial_role;
    missing_spatial_role.roles[1].path_data_id = 4U;
    missing_spatial_role.roles[1].world_x = 0x24U;
    missing_spatial_role.roles[1].world_y = 0x20U;
    missing_spatial_role.roles[1].map_cell_pointer_32 = 0U;
    std::vector<u8> missing_role_grid(4U * sizeof(u32), 0xFFU);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        missing_role_slots;
    missing_role_slots[0].bytes.fill(0U);
    write_u16(missing_role_slots[0].bytes, 0U, 1U);
    missing_role_slots[0].bytes[0x1CU] = 7U;
    const LegacyWorldRoleTransferContext missing_role_context{
        .active_object_slots = missing_role_slots,
        .spatial_index = &spatial,
        .surface_grid = missing_role_grid,
        .map_width = 2U,
        .selected_guid = 1U,
    };
    test.expect_equal(
        missing_spatial_role.run(&missing_role_context),
        LegacyWorldRoleTransferStatus::spatial_relocation_failed,
        "a GUID absent from the searched spatial rows is explicit"
    );

    Fixture full_party;
    full_party.state.party_role_count = 8U;
    test.expect_equal(
        full_party.run(nullptr),
        LegacyWorldRoleTransferStatus::party_capacity_exceeded,
        "the ninth physical party write is stopped at its exact boundary"
    );
    test.expect_true(
        full_party.roles[1].talk_script_id == 0x33U &&
            full_party.roles[1].flags == 0x00004080U,
        "capacity failure occurs immediately before common party side effects"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_common_and_aligned_object_paths(test);
    test_nonaligned_surface_and_spatial_path(test);
    test_surface_special_cases(test);
    test_checked_runtime_boundaries(test);
    return test.exit_code();
}
