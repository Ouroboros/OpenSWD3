#include "test.hpp"

#include "openswd3/world_map/legacy_world_role_map_update.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::apply_legacy_world_role_map_update;
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldPartySlotCount;
using openswd3::world_map::LegacyMapsRoleSourceRecord;
using openswd3::world_map::LegacyMapsWorldDatabase;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldRoleMapUpdateContext;
using openswd3::world_map::LegacyWorldRoleMapUpdateRequest;
using openswd3::world_map::LegacyWorldRoleMapUpdateStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
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
        source.logical_map_id = 81U;
        source.guid = 9U;
        source.action_id = 100U;
        source.base_variant = 101U;
        source.variant_delta = 102U;
        source.talk_script_id = 103U;
        source.path_data_id = 104U;
        source.path_word_index = 7;
        source.flags = 0x0181U;
        database.role_sources.push_back(source);
        static_cast<void>(write_legacy_maps_role_source_record(
            payload, database.role_sources.front()
        ));

        roles.resize(4U);
        roles[1].guid = 9U;
        roles[1].world_x = 0x20U;
        roles[1].world_y = 0x20U;
        roles[1].map_cell_pointer_32 = 2U;
        roles[1].flags = 0x80U;
        roles[1].action.field_2c = 1U;
        roles[1].action.field_30 = 1U;

        party_indices = {10U, 11U, 1U, 13U, 14U, 15U, 16U, 17U};
        for (std::size_t index = 0U; index < party_objects.size(); ++index) {
            party_objects[index].bytes.fill(static_cast<u8>(index));
            write_u16(party_objects[index].bytes, 2U, 0x7FFFU);
        }
    }

    [[nodiscard]] LegacyWorldRoleMapUpdateContext context() {
        return {
            .controlled_role_index = 1U,
            .maps_payload = payload,
            .maps_database = &database,
            .roles = roles,
            .party_object_slots = party_objects,
            .party_role_indices = party_indices,
            .party_role_count = &party_count,
            .spatial_index = spatial_pointer,
            .role_surface = LegacyWorldRoleSurfaceContext{
                .map_width = 4U,
                .selected_guid = 1U,
                .surface_grid = surface,
            },
        };
    }

    [[nodiscard]] LegacyWorldRoleMapUpdateRequest request() const {
        return {
            .role_selector = 9U,
            .path_data_id = 0x8001U,
            .talk_script_id = 0x8002U,
            .action_id = 0x8003U,
            .base_variant = 0x8004U,
            .variant_delta = 0x8005U,
            .flags = 0xC000U,
        };
    }

    std::vector<u8> payload = std::vector<u8>(0x80U, 0U);
    LegacyMapsWorldDatabase database;
    LegacyMapsRoleSourceRecord source;
    std::vector<LegacyWorldRoleRecord> roles;
    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount> party_objects;
    std::array<u32, kLegacyWorldPartySlotCount> party_indices;
    u32 party_count{5U};
    std::vector<u8> surface = std::vector<u8>(16U * sizeof(u32), 0U);
    LegacyRoleSpatialIndex* spatial_pointer{};
};

void test_aligned_party_removal(openswd3::test::Context& test) {
    Fixture fixture;
    const auto result = apply_legacy_world_role_map_update(
        fixture.request(), fixture.context()
    );
    test.expect_equal(
        result.status,
        LegacyWorldRoleMapUpdateStatus::ready,
        "an aligned party role completes the D897 path"
    );
    test.expect_true(
        result.runtime_role_found && result.physical_party_index == 2U &&
            result.party_role_removed && !result.role_surface_cleared &&
            fixture.party_count == 4U,
        "the physical party hit and logical count decrement are observable"
    );
    const auto& role = fixture.roles[1];
    test.expect_true(
        role.path_data_id == 0x8001U && role.path_word_index == 0U &&
            role.talk_script_id == 0x8002U &&
            role.action.action_id == 0x8003U &&
            role.action.base_variant == 0x8004U &&
            role.action.variant_delta == 0x8005U && role.flags == 0xC000U,
        "all seven opcode operands reach their fields with zero extension"
    );
    test.expect_equal(
        fixture.party_indices,
        std::array<u32, 8U>{10U, 11U, 13U, 14U, 15U, 16U, 17U, 17U},
        "the fixed index memcpy shifts through slot six and leaves slot seven"
    );
    test.expect_true(
        fixture.party_objects[2].bytes[0] == 3U &&
            fixture.party_objects[6].bytes[0] == 7U &&
            fixture.party_objects[7].bytes[0] == 7U,
        "the fixed object memcpy also shifts all physical trailing slots"
    );
    const auto& source = fixture.database.role_sources.front();
    test.expect_true(
        source.action_id == 100U && source.base_variant == 101U &&
            source.variant_delta == 102U && source.talk_script_id == 0x8002U &&
            source.path_data_id == 0x8001U && source.path_word_index == 0 &&
            source.flags == 0x0101U,
        "MAPS keeps action operands but writes Talk/Path and clears bit seven"
    );
    test.expect_equal(
        read_u32(fixture.surface, 2U * sizeof(u32)),
        u32{0x30000000U},
        "the updated role is marked on its existing surface anchor"
    );
}

void test_moving_role_alignment_and_spatial_removal(
    openswd3::test::Context& test
) {
    Fixture fixture;
    fixture.roles[1].world_x = 0x20U;
    fixture.roles[1].world_y = 0x24U;
    fixture.surface.assign(fixture.surface.size(), 0xFFU);
    write_u16(fixture.party_objects[2].bytes, 2U, 0U);
    fixture.party_objects[2].bytes[0x1CU] = 1U;

    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 5U;
    for (auto& group : spatial.row_heads) {
        group.resize(45U, 0U);
    }
    test.expect_true(
        insert_legacy_role_spatially(
            spatial, fixture.roles, 1U, fixture.roles[1U].flags & 3U
        ),
        "the role starts in the spatial index"
    );
    fixture.spatial_pointer = &spatial;

    const auto result = apply_legacy_world_role_map_update(
        fixture.request(), fixture.context()
    );
    test.expect_true(
        result.status == LegacyWorldRoleMapUpdateStatus::ready &&
            result.role_surface_cleared && result.coordinates_aligned &&
            result.spatial_role_relocated,
        "nonaligned coordinates take clear, direction and spatial stages"
    );
    test.expect_true(
        fixture.roles[1].world_x == 0x20U &&
            fixture.roles[1].world_y == 0x30U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding + 2U] == 0U &&
            spatial.row_heads[0U][kLegacySpatialRowPadding + 3U] == 1U,
        "direction one aligns Y upward and relocates to spatial row three"
    );
}

void test_ff_direction_still_removes_spatial_role(
    openswd3::test::Context& test
) {
    Fixture fixture;
    fixture.roles[1].world_x = 0x24U;
    fixture.roles[1].world_y = 0x20U;
    fixture.surface.assign(fixture.surface.size(), 0xFFU);
    write_u16(fixture.party_objects[2].bytes, 2U, 0U);
    fixture.party_objects[2].bytes[0x1CU] = 0xFFU;

    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 5U;
    for (auto& group : spatial.row_heads) {
        group.resize(45U, 0U);
    }
    static_cast<void>(insert_legacy_role_spatially(
        spatial, fixture.roles, 1U, fixture.roles[1U].flags & 3U
    ));
    fixture.spatial_pointer = &spatial;

    const auto result = apply_legacy_world_role_map_update(
        fixture.request(), fixture.context()
    );
    test.expect_true(
        result.status == LegacyWorldRoleMapUpdateStatus::ready &&
            !result.coordinates_aligned && result.spatial_role_relocated &&
            fixture.roles[1].world_x == 0x24U,
        "direction FF skips coordinate loops but not sub_411530"
    );
}

void test_negative_direction_alignment(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].world_x = 0x2CU;
    fixture.roles[1].world_y = 0x20U;
    fixture.surface.assign(fixture.surface.size(), 0xFFU);
    write_u16(fixture.party_objects[2].bytes, 2U, 0U);
    fixture.party_objects[2].bytes[0x1CU] = 3U;

    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 5U;
    for (auto& group : spatial.row_heads) {
        group.resize(45U, 0U);
    }
    static_cast<void>(insert_legacy_role_spatially(
        spatial, fixture.roles, 1U, fixture.roles[1U].flags & 3U
    ));
    fixture.spatial_pointer = &spatial;

    const auto result = apply_legacy_world_role_map_update(
        fixture.request(), fixture.context()
    );
    test.expect_true(
        result.status == LegacyWorldRoleMapUpdateStatus::ready &&
            fixture.roles[1].world_x == 0x20U &&
            fixture.roles[1].world_y == 0x20U,
        "direction three adds negative four until X reaches the lower tile"
    );
}

void test_missing_and_stale_physical_paths(openswd3::test::Context& test) {
    Fixture missing;
    missing.roles[1].guid = 8U;
    const auto missing_result = apply_legacy_world_role_map_update(
        missing.request(), missing.context()
    );
    test.expect_true(
        missing_result.status == LegacyWorldRoleMapUpdateStatus::ready &&
            !missing_result.runtime_role_found &&
            missing.database.role_sources[0].flags == 0x0101U &&
            missing.database.role_sources[0].path_data_id == 104U,
        "a missing runtime role only clears source bit seven"
    );

    Fixture not_party;
    not_party.party_indices.fill(99U);
    const auto not_party_result = apply_legacy_world_role_map_update(
        not_party.request(), not_party.context()
    );
    test.expect_equal(
        not_party_result.status,
        LegacyWorldRoleMapUpdateStatus::active_role_not_in_physical_party,
        "a runtime role absent from all eight physical slots only diagnoses"
    );

    Fixture stale;
    stale.party_count = 2U;
    stale.party_indices.fill(99U);
    stale.party_indices[6] = 1U;
    const auto stale_result =
        apply_legacy_world_role_map_update(stale.request(), stale.context());
    test.expect_true(
        stale_result.status == LegacyWorldRoleMapUpdateStatus::ready &&
            stale_result.physical_party_index == 6U &&
            stale.party_count == 1U && stale.party_indices[6] == 99U,
        "the fixed eight-slot scan preserves a stale hit beyond logical count"
    );

    Fixture missing_source;
    missing_source.database.role_sources.clear();
    const auto missing_source_result = apply_legacy_world_role_map_update(
        missing_source.request(), missing_source.context()
    );
    test.expect_true(
        missing_source_result.status ==
                LegacyWorldRoleMapUpdateStatus::maps_patch_failed &&
            missing_source_result.party_role_removed &&
            missing_source.party_count == 4U &&
            missing_source.roles[1].path_data_id == 0x8001U,
        "ignored sub_40D460 failure does not skip later runtime side effects"
    );
}

void test_checked_invalid_direction(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].world_x = 0x24U;
    fixture.surface.assign(fixture.surface.size(), 0xFFU);
    write_u16(fixture.party_objects[2].bytes, 2U, 0U);
    fixture.party_objects[2].bytes[0x1CU] = 8U;
    const auto result = apply_legacy_world_role_map_update(
        fixture.request(), fixture.context()
    );
    test.expect_true(
        result.status ==
                LegacyWorldRoleMapUpdateStatus::path_direction_out_of_range &&
            result.role_surface_cleared && fixture.party_count == 5U,
        "direction table overflow is isolated after the original clear point"
    );

    Fixture cannot_align;
    cannot_align.roles[1].world_x = 0x20U;
    cannot_align.roles[1].world_y = 0x24U;
    cannot_align.surface.assign(cannot_align.surface.size(), 0xFFU);
    write_u16(cannot_align.party_objects[2].bytes, 2U, 0U);
    cannot_align.party_objects[2].bytes[0x1CU] = 7U;
    const auto cannot_align_result = apply_legacy_world_role_map_update(
        cannot_align.request(), cannot_align.context()
    );
    test.expect_true(
        cannot_align_result.status ==
                LegacyWorldRoleMapUpdateStatus::path_direction_cannot_align &&
            cannot_align_result.role_surface_cleared &&
            cannot_align.party_count == 5U,
        "a nonaligned axis with zero step isolates the original infinite loop"
    );
}

void test_spatial_miss_keeps_later_side_effects(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].world_x = 0x24U;
    fixture.roles[1].world_y = 0x20U;
    fixture.surface.assign(fixture.surface.size(), 0xFFU);
    write_u16(fixture.party_objects[2].bytes, 2U, 0U);
    fixture.party_objects[2].bytes[0x1CU] = 7U;

    LegacyRoleSpatialIndex spatial;
    spatial.map_height = 5U;
    for (auto& group : spatial.row_heads) {
        group.resize(45U, 0U);
    }
    fixture.spatial_pointer = &spatial;

    const auto result = apply_legacy_world_role_map_update(
        fixture.request(), fixture.context()
    );
    test.expect_true(
        result.status ==
                LegacyWorldRoleMapUpdateStatus::
                    role_spatial_relocation_failed &&
            result.party_role_removed && result.maps_source_patched &&
            fixture.party_count == 4U &&
            fixture.roles[1].path_data_id == 0x8001U,
        "ignored sub_411530 miss preserves every later D897 side effect"
    );
}

void test_real_maps_missing_runtime_role(
    openswd3::test::Context& test, const std::filesystem::path& path
) {
    std::ifstream input(path, std::ios::binary);
    const std::vector<u8> file_bytes{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };
    test.expect_true(
        input.is_open() && file_bytes.size() > 0x200U,
        "current MAPS.DAT exposes its payload"
    );
    if (!input.is_open() || file_bytes.size() <= 0x200U) {
        return;
    }

    std::vector<u8> payload(file_bytes.begin() + 0x200, file_bytes.end());
    auto decoded = decode_legacy_maps_world_database(payload);
    test.expect_true(
        decoded.status == LegacyMapsWorldDatabaseStatus::ready &&
            !decoded.database.role_sources.empty(),
        "current MAPS role directory is available"
    );
    if (decoded.status != LegacyMapsWorldDatabaseStatus::ready ||
        decoded.database.role_sources.empty()) {
        return;
    }

    auto& source = decoded.database.role_sources.front();
    const auto original = source;
    source.flags |= 0x0080U;
    test.expect_true(
        write_legacy_maps_role_source_record(payload, source),
        "the real-derived source vector can expose a set bit seven"
    );

    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount> objects{};
    std::array<u32, kLegacyWorldPartySlotCount> indices{};
    std::span<LegacyWorldRoleRecord> no_roles;
    u32 party_count = 1U;
    const LegacyWorldRoleMapUpdateContext context{
        .controlled_role_index = 0U,
        .maps_payload = payload,
        .maps_database = &decoded.database,
        .roles = no_roles,
        .party_object_slots = objects,
        .party_role_indices = indices,
        .party_role_count = &party_count,
        .role_surface = {},
    };
    const auto result = apply_legacy_world_role_map_update(
        LegacyWorldRoleMapUpdateRequest{.role_selector = source.guid}, context
    );
    test.expect_true(
        result.status == LegacyWorldRoleMapUpdateStatus::ready &&
            !result.runtime_role_found && result.maps_source_patched,
        "a real source absent from runtime takes the D98A branch"
    );
    test.expect_true(
        source.logical_map_id == original.logical_map_id &&
            source.guid == original.guid &&
            source.action_id == original.action_id &&
            source.base_variant == original.base_variant &&
            source.variant_delta == original.variant_delta &&
            source.tile_x == original.tile_x &&
            source.tile_y == original.tile_y &&
            source.talk_script_id == original.talk_script_id &&
            source.path_data_id == original.path_data_id &&
            source.path_word_index == original.path_word_index &&
            source.flags == static_cast<u16>(original.flags & 0xFF7FU),
        "the real-derived missing-role path changes only source bit seven"
    );
}

}  // namespace

int main(const int argc, const char* const argv[]) {
    openswd3::test::Context test;
    test_aligned_party_removal(test);
    test_moving_role_alignment_and_spatial_removal(test);
    test_ff_direction_still_removes_spatial_role(test);
    test_negative_direction_alignment(test);
    test_missing_and_stale_physical_paths(test);
    test_checked_invalid_direction(test);
    test_spatial_miss_keeps_later_side_effects(test);
    if (argc == 2) {
        test_real_maps_missing_runtime_role(test, argv[1]);
    }
    return test.exit_code();
}
