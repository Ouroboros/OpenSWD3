#include "test.hpp"

#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <vector>

namespace {

using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::clear_legacy_world_role_surface_occupancy;
using openswd3::world_map::LegacyWorldRoleCellFlagRefreshStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::LegacyWorldRoleSurfaceStatus;
using openswd3::world_map::mark_legacy_world_role_surface_occupancy;
using openswd3::world_map::refresh_legacy_world_role_cell_flags;

void write_cell(
    std::vector<u8>& bytes, const std::size_t index, const u32 value
) {
    const std::size_t offset = index * 4U;
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u32
read_cell(const std::vector<u8>& bytes, const std::size_t index) {
    const std::size_t offset = index * 4U;
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void test_exact_masks_and_special_footprints(openswd3::test::Context& test) {
    std::vector<u8> surface(4U * 16U, 0xFFU);
    LegacyWorldRoleRecord role{};
    role.map_cell_pointer_32 = 5U;
    role.guid = 7U;
    role.action.field_2c = 2U;
    role.action.field_30 = 1U;
    const LegacyWorldRoleSurfaceContext context{
        .map_width = 4U,
        .selected_guid = 7U,
        .surface_grid = surface,
    };

    const auto cleared =
        clear_legacy_world_role_surface_occupancy(role, context);
    test.expect_true(
        cleared.status == LegacyWorldRoleSurfaceStatus::ready &&
            cleared.mask == 0xCF7FFEFFU && cleared.touched_cells == 2U &&
            read_cell(surface, 5U) == 0xCF7FFEFFU &&
            read_cell(surface, 6U) == 0xCF7FFEFFU,
        "selected 1x2 footprint uses the exact sub_40AE20 mask"
    );

    role.flags = 0x00004010U;
    role.action.field_2c = 1U;
    role.action.field_30 = 2U;
    write_cell(surface, 5U, 0U);
    write_cell(surface, 9U, 0U);
    const auto marked = mark_legacy_world_role_surface_occupancy(role, context);
    test.expect_true(
        marked.status == LegacyWorldRoleSurfaceStatus::ready &&
            marked.mask == 0x30800100U && marked.touched_cells == 2U &&
            read_cell(surface, 5U) == 0x30800100U &&
            read_cell(surface, 9U) == 0x30800100U,
        "selected blocking 2x1 footprint derives every sub_40AEC0 bit"
    );
}

void test_generic_path_revisits_anchor(openswd3::test::Context& test) {
    std::vector<u8> surface(4U * 16U, 0U);
    LegacyWorldRoleRecord role{};
    role.map_cell_pointer_32 = 1U;
    role.action.field_2c = 2U;
    role.action.field_30 = 2U;
    const auto result = mark_legacy_world_role_surface_occupancy(
        role, LegacyWorldRoleSurfaceContext{4U, 0xFFFFU, surface}
    );

    test.expect_true(
        result.status == LegacyWorldRoleSurfaceStatus::ready &&
            result.touched_cells == 5U &&
            read_cell(surface, 1U) == 0x10000000U &&
            read_cell(surface, 2U) == 0x10000000U &&
            read_cell(surface, 5U) == 0x10000000U &&
            read_cell(surface, 6U) == 0x10000000U,
        "generic footprint keeps the original anchor write before both rows"
    );
}

void test_cell_flag_refresh(openswd3::test::Context& test) {
    std::vector<u8> surface(8U, 0U);
    write_cell(surface, 1U, 0x0000B800U);
    LegacyWorldRoleRecord role{};
    role.map_cell_pointer_32 = 1U;
    role.flags = 0xFFFFFFFFU;

    const auto status = refresh_legacy_world_role_cell_flags(role, surface);
    test.expect_true(
        status == LegacyWorldRoleCellFlagRefreshStatus::ready &&
            role.flags == 0xFFBFFFFFU,
        "cell bit eleven and nibble B replace the exact mapped flag fields"
    );

    role.map_cell_pointer_32 = 2U;
    test.expect_equal(
        refresh_legacy_world_role_cell_flags(role, surface),
        LegacyWorldRoleCellFlagRefreshStatus::cell_out_of_range,
        "refresh rejects a modern span overrun before reading the cell"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_masks_and_special_footprints(test);
    test_generic_path_revisits_anchor(test);
    test_cell_flag_refresh(test);
    return test.exit_code();
}
