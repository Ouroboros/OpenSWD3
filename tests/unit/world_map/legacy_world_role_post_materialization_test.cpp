#include "test.hpp"

#include "openswd3/world_map/legacy_world_role_post_materialization.hpp"

#include <algorithm>
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
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::find_legacy_maps_map_descriptor;
using openswd3::world_map::kLegacyWorldActiveObjectSlotCount;
using openswd3::world_map::kLegacyWorldPartySlotCount;
using openswd3::world_map::LegacyMapsMapDescriptor;
using openswd3::world_map::LegacyMapsRoleSourceRecord;
using openswd3::world_map::LegacyMapsWorldDatabase;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyWorldLoadRequest;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldRolePostMaterializationContext;
using openswd3::world_map::LegacyWorldRolePostMaterializationState;
using openswd3::world_map::LegacyWorldRolePostMaterializationStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::post_materialize_legacy_world_role;
using openswd3::world_map::write_legacy_maps_role_source_record;

void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
    bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

void write_gate_row(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 guid,
    const std::array<u16, 8U>& states
) {
    write_u16(bytes, offset, guid);
    for (std::size_t index = 0U; index < states.size(); ++index) {
        write_u16(
            bytes,
            offset + sizeof(u16) + index * sizeof(u16),
            states[index]
        );
    }
}

struct Fixture {
    Fixture() {
        write_u32(payload, 0x64U, 0x100U);
        write_u16(payload, 0x100U, 0xFFFFU);

        source.payload_offset = 0x80U;
        source.logical_map_id = 5U;
        source.guid = 9U;
        source.flags = 0x0100U;
        database.role_sources.push_back(source);
        static_cast<void>(write_legacy_maps_role_source_record(
            payload,
            database.role_sources.front()
        ));

        descriptor.logical_map_id = 5U;
        descriptor.field_0c = 7U;
        request.logical_map_id = 5U;
        request.selected_guid = 1U;
        roles.resize(2U);
        roles[1].guid = 9U;
        roles[1].world_x = 0x120U;
        roles[1].world_y = 0x230U;
        roles[1].talk_script_id = 0x33U;
    }

    [[nodiscard]] LegacyWorldRolePostMaterializationStatus run(
        const LegacyWorldRolePostMaterializationContext* context = nullptr
    ) {
        return post_materialize_legacy_world_role(
            payload,
            database,
            descriptor,
            request,
            roles,
            1U,
            context,
            state
        );
    }

    std::vector<u8> payload = std::vector<u8>(0x180U, 0U);
    LegacyMapsWorldDatabase database;
    LegacyMapsRoleSourceRecord source;
    LegacyMapsMapDescriptor descriptor;
    LegacyWorldLoadRequest request;
    std::vector<LegacyWorldRoleRecord> roles;
    LegacyWorldRolePostMaterializationState state;
};

void test_guid_one_action_overrides(openswd3::test::Context& test) {
    Fixture previous_map;
    previous_map.roles[1].guid = 1U;
    previous_map.roles[1].action.action_id = 3U;
    previous_map.request.selected_guid = 1U;
    const LegacyWorldRolePostMaterializationContext previous_context{
        .previous_logical_map_id = 22U,
        .guid_one_action_override = 0x77U,
        .active_object_slots = {},
    };
    test.expect_equal(
        previous_map.run(&previous_context),
        LegacyWorldRolePostMaterializationStatus::ready,
        "GUID one previous-map override completes"
    );
    test.expect_true(
        previous_map.roles[1].action.action_id == 0x77U &&
            previous_map.state.guid_one_roles_overridden == 1U &&
            previous_map.state.party_role_indices[0] == 1U,
        "0x0040CADD applies the nonzero map-22 action and selects party zero"
    );

    Fixture descriptor_override;
    descriptor_override.roles[1].guid = 1U;
    descriptor_override.request.logical_map_id = 6U;
    descriptor_override.descriptor.field_0c = 0x8007U;
    const LegacyWorldRolePostMaterializationContext story_context{
        .has_story_state_0x0192 = true,
        .active_object_slots = {},
    };
    test.expect_equal(
        descriptor_override.run(&story_context),
        LegacyWorldRolePostMaterializationStatus::ready,
        "descriptor action override completes"
    );
    test.expect_equal(
        descriptor_override.roles[1].action.action_id,
        u32{0x5FU},
        "story node 0x0192 changes map 6 action 0x60 to 0x5F"
    );

    Fixture ordinary_story_map;
    ordinary_story_map.roles[1].guid = 1U;
    ordinary_story_map.request.logical_map_id = 7U;
    ordinary_story_map.descriptor.field_0c = 0x8007U;
    test.expect_equal(
        ordinary_story_map.run(&story_context),
        LegacyWorldRolePostMaterializationStatus::ready,
        "ordinary high-bit descriptor action override completes"
    );
    test.expect_equal(
        ordinary_story_map.roles[1].action.action_id,
        u32{0x60U},
        "only maps 6, 8, and 200 inspect story node 0x0192"
    );
}

void test_gate_transfer_decisions(openswd3::test::Context& test) {
    Fixture no_guid_row;
    no_guid_row.roles[1].flags = 0x00004080U;
    write_gate_row(
        no_guid_row.payload,
        0x100U,
        8U,
        {7U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}
    );
    write_u16(no_guid_row.payload, 0x112U, 0xFFFFU);
    test.expect_equal(
        no_guid_row.run(),
        LegacyWorldRolePostMaterializationStatus::ready,
        "missing GUID row follows decision one"
    );
    test.expect_true(
        no_guid_row.state.roles_transferred == 1U &&
            no_guid_row.state.party_role_count == 2U &&
            no_guid_row.state.party_role_indices[1] == 1U &&
            no_guid_row.roles[1].talk_script_id == 0U &&
            no_guid_row.roles[1].flags == 0x00008080U,
        "sub_40D610 common state and caller bit 15 match the ordinary map path"
    );

    Fixture state_absent;
    state_absent.roles[1].flags = 0x00000080U;
    write_gate_row(
        state_absent.payload,
        0x100U,
        9U,
        {6U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}
    );
    write_u16(state_absent.payload, 0x112U, 0xFFFFU);
    test.expect_equal(
        state_absent.run(),
        LegacyWorldRolePostMaterializationStatus::ready,
        "matching GUID with absent map state completes"
    );
    test.expect_true(
        state_absent.state.roles_suppressed == 1U &&
            state_absent.state.roles_transferred == 0U &&
            state_absent.state.party_role_count == 1U &&
            state_absent.roles[1].flags == 0x00000080U,
        "decision zero suppresses the transfer without changing caller flags"
    );

    Fixture state_present;
    state_present.roles[1].flags = 0x00000080U;
    write_gate_row(
        state_present.payload,
        0x100U,
        9U,
        {7U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}
    );
    write_u16(state_present.payload, 0x112U, 0xFFFFU);
    state_present.request.logical_map_id = 22U;
    test.expect_equal(
        state_present.run(),
        LegacyWorldRolePostMaterializationStatus::ready,
        "matching GUID and map state transfers"
    );
    test.expect_equal(
        state_present.roles[1].flags,
        u32{0x00000080U},
        "target map 22 clears caller bit 15 after transfer"
    );
}

void test_object_slot_and_maps_patch(openswd3::test::Context& test) {
    Fixture missing_slots;
    missing_slots.roles[1].flags = 0x80U;
    missing_slots.roles[1].path_data_id = 4U;
    test.expect_equal(
        missing_slots.run(),
        LegacyWorldRolePostMaterializationStatus::
            active_object_slots_required,
        "path-backed transfers require the exact 72-slot state"
    );

    Fixture matched_slot;
    matched_slot.roles[1].flags = 0x80U;
    matched_slot.roles[1].path_data_id = 4U;
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_slots;
    active_slots[5].bytes.fill(0x55U);
    write_u16(active_slots[5].bytes, 0U, 1U);
    const LegacyWorldRolePostMaterializationContext context{
        .active_object_slots = active_slots,
    };
    test.expect_equal(
        matched_slot.run(&context),
        LegacyWorldRolePostMaterializationStatus::ready,
        "aligned path-backed role transfers"
    );
    test.expect_true(
        matched_slot.database.role_sources[0].flags == 0x0180U &&
            matched_slot.payload[0x94U] == 0x80U &&
            matched_slot.payload[0x95U] == 0x01U &&
            std::ranges::all_of(
                active_slots[5].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            matched_slot.state.active_object_slots_reset == 1U,
        "matched object reset and MAPS flags OR 0x80 preserve D610 order"
    );

    Fixture unaligned;
    unaligned.roles[1].flags = 0x80U;
    unaligned.roles[1].path_data_id = 4U;
    unaligned.roles[1].world_x |= 1U;
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        unaligned_slots;
    write_u16(unaligned_slots[0].bytes, 0U, 1U);
    const LegacyWorldRolePostMaterializationContext unaligned_context{
        .active_object_slots = unaligned_slots,
    };
    test.expect_equal(
        unaligned.run(&unaligned_context),
        LegacyWorldRolePostMaterializationStatus::
            materialized_role_not_tile_aligned,
        "the post-materialization owner rejects a broken tile-shift invariant"
    );
}

void test_flagged_role_records(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles.resize(6U);
    fixture.state.flagged_role_records[0].trailing_bytes.fill(0xA5U);
    for (u32 index = 1U; index < fixture.roles.size(); ++index) {
        auto& role = fixture.roles[index];
        role.world_x = 0x10000U + index;
        role.world_y = 0x20000U + index;
        role.guid = static_cast<u16>(20U + index);
        role.flags = 0x200U;
        test.expect_equal(
            post_materialize_legacy_world_role(
                fixture.payload,
                fixture.database,
                fixture.descriptor,
                fixture.request,
                fixture.roles,
                index,
                nullptr,
                fixture.state
            ),
            LegacyWorldRolePostMaterializationStatus::ready,
            "bit-nine record append completes"
        );
    }
    test.expect_true(
        fixture.state.flagged_role_record_count == 4U &&
            fixture.state.flagged_role_overflow_count == 1U &&
            fixture.state.flagged_role_records[0].world_x == 1U &&
            fixture.state.flagged_role_records[0].world_y == 1U &&
            fixture.state.flagged_role_records[0].guid == 21U &&
            fixture.state.flagged_role_records[0].field_04 == 0U &&
            fixture.state.flagged_role_records[0].field_0a == 0U &&
            fixture.state.flagged_role_records[0].trailing_bytes[0] == 0xA5U,
        "four 16-byte records write only offsets 0..0x0B and diagnose the fifth"
    );
}

void test_checked_boundaries(openswd3::test::Context& test) {
    Fixture short_header;
    short_header.roles[1].flags = 0x80U;
    test.expect_equal(
        post_materialize_legacy_world_role(
            std::span<u8>{short_header.payload}.first(0x67U),
            short_header.database,
            short_header.descriptor,
            short_header.request,
            short_header.roles,
            1U,
            nullptr,
            short_header.state
        ),
        LegacyWorldRolePostMaterializationStatus::
            gate_offset_field_out_of_range,
        "gate pointer field is checked"
    );

    Fixture bad_offset;
    bad_offset.roles[1].flags = 0x80U;
    write_u32(bad_offset.payload, 0x64U, 0x180U);
    test.expect_equal(
        bad_offset.run(),
        LegacyWorldRolePostMaterializationStatus::
            gate_directory_offset_out_of_range,
        "gate directory pointer is checked"
    );

    Fixture truncated;
    truncated.roles[1].flags = 0x80U;
    write_u32(truncated.payload, 0x64U, 0x170U);
    write_u16(truncated.payload, 0x170U, 9U);
    test.expect_equal(
        truncated.run(),
        LegacyWorldRolePostMaterializationStatus::gate_record_truncated,
        "partial 18-byte gate row is checked"
    );

    Fixture unterminated;
    unterminated.roles[1].flags = 0x80U;
    write_u32(unterminated.payload, 0x64U, 0x16EU);
    write_gate_row(
        unterminated.payload,
        0x16EU,
        8U,
        {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}
    );
    test.expect_equal(
        unterminated.run(),
        LegacyWorldRolePostMaterializationStatus::
            gate_directory_unterminated,
        "gate directory without FFFF terminator is checked"
    );

    Fixture party_full;
    party_full.roles[1].flags = 0x80U;
    party_full.state.party_role_count = kLegacyWorldPartySlotCount;
    test.expect_equal(
        party_full.run(),
        LegacyWorldRolePostMaterializationStatus::party_capacity_exceeded,
        "the original eight-entry party overflow is an explicit boundary"
    );
}

void test_real_gate_directory(
    openswd3::test::Context& test,
    const std::filesystem::path& maps_path
) {
    std::ifstream input(maps_path, std::ios::binary);
    std::vector<u8> file_bytes{
        std::istreambuf_iterator<char>{input},
        std::istreambuf_iterator<char>{},
    };
    test.expect_true(
        input.is_open() && file_bytes.size() > 0x200U,
        "current game MAPS payload is readable"
    );
    if (!input.is_open() || file_bytes.size() <= 0x200U) {
        return;
    }

    std::vector<u8> payload(file_bytes.begin() + 0x200, file_bytes.end());
    auto decoded = decode_legacy_maps_world_database(payload);
    test.expect_equal(
        decoded.status,
        LegacyMapsWorldDatabaseStatus::ready,
        "current game MAPS database decodes"
    );
    if (decoded.status != LegacyMapsWorldDatabaseStatus::ready) {
        return;
    }

    const auto* const descriptor = find_legacy_maps_map_descriptor(
        decoded.database,
        decoded.database.initial_load.logical_map_id
    );
    test.expect_true(
        descriptor != nullptr,
        "current initial map descriptor exists"
    );
    if (descriptor == nullptr) {
        return;
    }

    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0x7FFEU;
    roles[1].flags = 0x80U;
    LegacyWorldRolePostMaterializationState state;
    test.expect_equal(
        post_materialize_legacy_world_role(
            payload,
            decoded.database,
            *descriptor,
            decoded.database.initial_load,
            roles,
            1U,
            nullptr,
            state
        ),
        LegacyWorldRolePostMaterializationStatus::ready,
        "current +0x64 gate directory reaches its FFFF terminator"
    );
    test.expect_true(
        state.gated_roles_scanned == 1U &&
            state.roles_transferred == 1U,
        "an absent synthetic GUID preserves the original decision-one path"
    );
}

}  // namespace

int main(const int argc, char** argv) {
    openswd3::test::Context test;
    test_guid_one_action_overrides(test);
    test_gate_transfer_decisions(test);
    test_object_slot_and_maps_patch(test);
    test_flagged_role_records(test);
    test_checked_boundaries(test);
    if (argc == 2) {
        test_real_gate_directory(test, argv[1]);
    }

    return test.exit_code();
}
