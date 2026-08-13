#include "test.hpp"

#include "openswd3/world_map/legacy_world_role_preload.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::kLegacyWorldObjectSlotCount;
using openswd3::world_map::LegacyMapsRoleSourceRecord;
using openswd3::world_map::LegacyMapsWorldDatabase;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyWorldLoadRequest;
using openswd3::world_map::LegacyWorldObjectSlotPrefix;
using openswd3::world_map::LegacyWorldRolePreloadContext;
using openswd3::world_map::LegacyWorldRolePreloadStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::preload_legacy_world_roles_before_load;
using openswd3::world_map::write_legacy_maps_role_source_record;

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

struct MapsFixture {
    std::vector<u8> payload;
    LegacyMapsWorldDatabase database;
};

MapsFixture make_maps_fixture(const std::span<const u16> guids) {
    MapsFixture fixture;
    fixture.payload.resize(
        guids.size() * openswd3::world_map::kLegacyMapsRoleSourceRecordSize
    );
    fixture.database.role_sources.reserve(guids.size());
    for (std::size_t index = 0U; index < guids.size(); ++index) {
        LegacyMapsRoleSourceRecord role{
            .payload_offset = static_cast<u32>(
                index * openswd3::world_map::kLegacyMapsRoleSourceRecordSize
            ),
            .logical_map_id = 1U,
            .guid = guids[index],
            .action_id = static_cast<u16>(10U + index),
            .base_variant = 2U,
            .variant_delta = 3U,
            .tile_x = 4U,
            .tile_y = 5U,
            .talk_script_id = 6U,
            .path_data_id = 7U,
            .path_word_index = -8,
            .flags = 0xA100U,
        };
        fixture.database.role_sources.push_back(role);
        static_cast<void>(write_legacy_maps_role_source_record(
            fixture.payload, fixture.database.role_sources.back()
        ));
    }

    return fixture;
}

LegacyWorldRoleRecord
make_role(const u16 guid, const u32 world_x, const u32 world_y) {
    LegacyWorldRoleRecord role{};
    role.guid = guid;
    role.world_x = world_x;
    role.world_y = world_y;
    role.action.action_id = 0x11112222U + guid;
    role.action.base_variant = 0x33334444U + guid;
    role.action.variant_delta = 0x55556666U + guid;
    role.talk_script_id = static_cast<u16>(0x7000U + guid);
    role.flags = 0xA100U;
    return role;
}

const LegacyMapsRoleSourceRecord&
source_by_guid(const LegacyMapsWorldDatabase& database, const u16 guid) {
    const auto found = std::ranges::find(
        database.role_sources, guid, &LegacyMapsRoleSourceRecord::guid
    );
    return *found;
}

std::vector<u8> make_path_database() {
    std::vector<u8> path(0x300U, 0U);
    write_u32(path, 0x204U, 0x80U);
    write_u32(path, 0x208U, 0x90U);
    write_u32(path, 0x20CU, 0x82U);
    write_u16(path, 0x280U, 8U);
    write_u16(path, 0x284U, 8U);
    write_u16(path, 0x290U, 5U);
    return path;
}

void test_full_preload_behavior(openswd3::test::Context& test) {
    constexpr std::array<u16, 7U> guids{11U, 13U, 14U, 15U, 16U, 17U, 18U};
    MapsFixture maps = make_maps_fixture(guids);
    std::vector<LegacyWorldRoleRecord> roles(10U);
    roles[1U] = make_role(11U, 0x111U, 0x222U);
    roles[1U].flags = 0x08000000U;
    roles[2U] = make_role(0xFFFFU, 0x111U, 0x222U);
    roles[3U] = make_role(13U, 0x111U, 0x222U);
    roles[4U] = make_role(14U, 0x111U, 0x222U);
    roles[4U].flags = 0xA180U;
    roles[4U].path_data_id = 0x1234U;
    roles[4U].path_word_index = 9U;
    roles[5U] = make_role(15U, 0xFFFFFFF0U, 0x000100F0U);
    roles[5U].path_word_index = 0xFFFF8001U;
    roles[6U] = make_role(16U, 0x123U, 0x237U);
    roles[6U].path_data_id = 1U;
    roles[6U].path_word_index = 2U;
    roles[7U] = make_role(17U, 0x12FU, 0x23FU);
    roles[7U].path_data_id = 2U;
    roles[7U].path_word_index = 0U;
    roles[8U] = make_role(99U, 0x333U, 0x444U);
    roles[9U] = make_role(18U, 0x12FU, 0x23FU);
    roles[9U].path_data_id = 3U;
    roles[9U].path_word_index = 0xFFFFFFFFU;

    std::array<LegacyWorldObjectSlotPrefix, kLegacyWorldObjectSlotCount> slots;
    slots[7U] = {
        .role_index = 6U,
        .field_02 = 0U,
        .world_x = 0x0350U,
        .world_y = 0x0460U,
    };
    const std::vector<u8> path = make_path_database();
    const auto result = preload_legacy_world_roles_before_load(
        maps.payload,
        maps.database,
        LegacyWorldLoadRequest{
            .logical_map_id = 81U,
            .tile_x = 31U,
            .tile_y = 32U,
        },
        LegacyWorldRolePreloadContext{
            .path_database = path,
            .roles = roles,
            .object_slots = slots,
            .controlled_role_index = 3U,
            .current_map_width = 10U,
            .current_map_height = 10U,
        }
    );
    test.expect_true(
        result.status == LegacyWorldRolePreloadStatus::ready &&
            result.roles_visited == 9U && result.roles_skipped == 3U &&
            result.flagged_roles_patched == 1U &&
            result.ordinary_roles_synchronized == 4U &&
            result.path_type_eight_roles == 2U &&
            result.object_coordinate_overrides == 1U &&
            result.out_of_bounds_coordinates == 1U &&
            result.missing_role_sources == 1U,
        "sub_40D200 keeps every skip, patch, type-eight and missing counter"
    );

    const auto& flagged = source_by_guid(maps.database, 14U);
    test.expect_true(
        flagged.logical_map_id == 81U && flagged.tile_x == 31U &&
            flagged.tile_y == 32U && flagged.path_data_id == 0x1234U &&
            flagged.path_word_index == 0 && flagged.flags == 0xA180U &&
            flagged.action_id == static_cast<u16>(roles[4U].action.action_id) &&
            flagged.base_variant ==
                static_cast<u16>(roles[4U].action.base_variant) &&
            flagged.variant_delta ==
                static_cast<u16>(roles[4U].action.variant_delta),
        "bit-seven roles use target coordinates and exact low-word state"
    );

    const auto& ordinary = source_by_guid(maps.database, 15U);
    test.expect_true(
        ordinary.tile_x == 0x0FFFU && ordinary.tile_y == 0x000FU &&
            ordinary.path_word_index == std::bit_cast<i16>(u16{0x8001U}) &&
            ordinary.action_id == static_cast<u16>(roles[5U].action.action_id),
        "ordinary writeback truncates coordinates before the 16-bit shift"
    );

    const auto& overridden = source_by_guid(maps.database, 16U);
    const auto& non_eight = source_by_guid(maps.database, 17U);
    const auto& wrapped = source_by_guid(maps.database, 18U);
    test.expect_true(
        overridden.tile_x == 0x35U && overridden.tile_y == 0x46U &&
            overridden.path_word_index == 3 && non_eight.tile_x == 0x12U &&
            non_eight.tile_y == 0x23U && non_eight.path_word_index == 0 &&
            wrapped.tile_x == 0x12U && wrapped.tile_y == 0x23U &&
            wrapped.path_word_index == 0,
        "type eight overrides or aligns then wraps while command five does not"
    );
    test.expect_true(
        source_by_guid(maps.database, 11U).action_id == 10U &&
            source_by_guid(maps.database, 13U).action_id == 11U,
        "skipped and controlled roles leave MAPS untouched"
    );
}

void test_checked_input_boundaries(openswd3::test::Context& test) {
    constexpr std::array<u16, 1U> guids{20U};
    MapsFixture maps = make_maps_fixture(guids);
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1U] = make_role(20U, 0x123U, 0x234U);
    roles[1U].path_data_id = 1U;
    roles[1U].path_word_index = 0U;

    auto run = [&](const std::span<const u8> path,
                   const std::span<const LegacyWorldObjectSlotPrefix> slots) {
        MapsFixture local = maps;
        return preload_legacy_world_roles_before_load(
            local.payload,
            local.database,
            LegacyWorldLoadRequest{},
            LegacyWorldRolePreloadContext{
                .path_database = path,
                .roles = roles,
                .object_slots = slots,
            }
        );
    };

    test.expect_equal(
        run({}, {}).status,
        LegacyWorldRolePreloadStatus::path_directory_entry_out_of_range,
        "a missing PATH directory entry is explicit"
    );

    std::vector<u8> bad_command(0x220U, 0U);
    write_u32(bad_command, 0x204U, 0x1000U);
    test.expect_equal(
        run(bad_command, {}).status,
        LegacyWorldRolePreloadStatus::path_command_out_of_range,
        "an out-of-range PATH command is explicit"
    );

    std::vector<u8> type_eight(0x240U, 0U);
    write_u32(type_eight, 0x204U, 0x20U);
    write_u16(type_eight, 0x220U, 8U);
    test.expect_equal(
        run(type_eight, {}).status,
        LegacyWorldRolePreloadStatus::object_slots_required,
        "type eight requires the original fixed 72 coordinate slots"
    );

    roles[1U].path_word_index = 0xFFFFFFFFU;
    std::vector<u8> signed_cursor(0x240U, 0U);
    write_u32(signed_cursor, 0x204U, 0x22U);
    write_u16(signed_cursor, 0x220U, 5U);
    test.expect_equal(
        run(signed_cursor, {}).status,
        LegacyWorldRolePreloadStatus::ready,
        "a negative PATH word cursor is resolved with signed 32-bit scaling"
    );
}

void test_real_path_commands(
    openswd3::test::Context& test,
    const std::filesystem::path& maps_path,
    const std::filesystem::path& path_path
) {
    std::ifstream maps_input(maps_path, std::ios::binary);
    std::vector<u8> maps_file{
        std::istreambuf_iterator<char>{maps_input},
        std::istreambuf_iterator<char>{},
    };
    std::ifstream path_input(path_path, std::ios::binary);
    const std::vector<u8> path{
        std::istreambuf_iterator<char>{path_input},
        std::istreambuf_iterator<char>{},
    };
    test.expect_true(
        maps_input.is_open() && maps_file.size() > 0x200U &&
            path_input.is_open() && path.size() > 0x200U,
        "current game MAPS and PATH data are readable"
    );
    if (!maps_input.is_open() || maps_file.size() <= 0x200U ||
        !path_input.is_open() || path.size() <= 0x200U) {
        return;
    }

    std::vector<u8> payload(maps_file.begin() + 0x200, maps_file.end());
    auto decoded = decode_legacy_maps_world_database(payload);
    test.expect_equal(
        decoded.status,
        LegacyMapsWorldDatabaseStatus::ready,
        "current game MAPS roles decode before preload verification"
    );
    if (decoded.status != LegacyMapsWorldDatabaseStatus::ready) {
        return;
    }

    std::vector<LegacyWorldRoleRecord> roles(1U);
    for (const auto& source : decoded.database.role_sources) {
        if (source.path_data_id == 0U) {
            continue;
        }

        LegacyWorldRoleRecord role{};
        role.guid = source.guid;
        role.world_x = static_cast<u32>(source.tile_x) << 4U;
        role.world_y = static_cast<u32>(source.tile_y) << 4U;
        role.path_data_id = source.path_data_id;
        role.path_word_index = static_cast<u32>(source.path_word_index);
        role.flags = source.flags;
        roles.push_back(role);
    }

    const auto result = preload_legacy_world_roles_before_load(
        payload,
        decoded.database,
        LegacyWorldLoadRequest{},
        LegacyWorldRolePreloadContext{
            .path_database = path,
            .roles = roles,
            .object_slots = {},
        }
    );
    test.expect_true(
        result.status == LegacyWorldRolePreloadStatus::ready &&
            roles.size() == 137U &&
            result.ordinary_roles_synchronized == 136U &&
            result.path_type_eight_roles == 0U,
        "all 136 current initial Path cursors resolve to non-eight commands"
    );
}

}  // namespace

int main(const int argc, char** argv) {
    openswd3::test::Context test;
    test_full_preload_behavior(test);
    test_checked_input_boundaries(test);
    if (argc == 3) {
        test_real_path_commands(test, argv[1], argv[2]);
    }

    return test.exit_code();
}
