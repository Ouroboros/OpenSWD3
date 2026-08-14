#include "test.hpp"

#include "openswd3/world_map/legacy_world_party_role_actions.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::world_map::advance_legacy_world_party_role_actions;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldPartySlotCount;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldPartyRoleActionsResult;
using openswd3::world_map::LegacyWorldPartyRoleActionsStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;

constexpr std::size_t kRoleIndexOffset = 0x00U;
constexpr std::size_t kPathCursorOffset = 0x02U;
constexpr std::size_t kStepXOffset = 0x16U;
constexpr std::size_t kStepYOffset = 0x18U;
constexpr std::size_t kPathBytesOffset = 0x1CU;

void write_u16(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u16
read_u16(const LegacyWorldObjectSlot& slot, const std::size_t offset) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_cell(
    std::span<u8> surface, const u32 cell_index, const u32 value
) noexcept {
    const std::size_t offset = static_cast<std::size_t>(cell_index) * 4U;
    surface[offset] = static_cast<u8>(value);
    surface[offset + 1U] = static_cast<u8>(value >> 8U);
    surface[offset + 2U] = static_cast<u8>(value >> 16U);
    surface[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u32
read_cell(std::span<const u8> surface, const u32 cell_index) noexcept {
    const std::size_t offset = static_cast<std::size_t>(cell_index) * 4U;
    return static_cast<u32>(surface[offset]) |
        (static_cast<u32>(surface[offset + 1U]) << 8U) |
        (static_cast<u32>(surface[offset + 2U]) << 16U) |
        (static_cast<u32>(surface[offset + 3U]) << 24U);
}

class RecordingActionPorts final : public LegacyActionDrawPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        updates.push_back(record);
        return update_status;
    }

    [[nodiscard]] bool
    load_frame_piece(const u16, const u16, LegacyFramePiece&) override {
        return false;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&, const i32, const i32, const u32, const i32
    ) noexcept override {
        return LegacyBlitExecutionStatus::completed;
    }

    LegacyActionUpdateStatus update_status{LegacyActionUpdateStatus::completed};
    std::vector<LegacyActionRecord> updates;
};

struct Fixture {
    static constexpr u32 kMapWidth = 50U;
    static constexpr u32 kMapHeight = 40U;
    static constexpr u32 kOldCell = 19U * kMapWidth + 24U;

    std::array<LegacyWorldRoleRecord, 3U> roles{};
    LegacyRoleSpatialIndex spatial;
    std::vector<u8> surface = std::vector<u8>(kMapWidth * kMapHeight * 4U, 0U);
    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount> slots;
    RecordingActionPorts actions;

    Fixture() {
        LegacyWorldRoleRecord& role = roles[2];
        role.world_x = 384U;
        role.world_y = 304U;
        role.map_cell_pointer_32 = kOldCell;
        role.guid = 7U;
        role.action.field_2c = 1U;
        role.action.field_30 = 1U;

        spatial.map_height = kMapHeight;
        const std::size_t row_count = static_cast<std::size_t>(kMapHeight) +
            2U * kLegacySpatialRowPadding;
        for (auto& rows : spatial.row_heads) {
            rows.assign(row_count, kLegacySpatialNoRole);
        }
        static_cast<void>(insert_legacy_role_spatially(
            spatial, roles, 2U, roles[2U].flags & 3U
        ));
        write_cell(surface, kOldCell, 0x10000000U);

        write_u16(slots[1], kRoleIndexOffset, 2U);
        write_u16(slots[1], kPathCursorOffset, 0U);
    }

    [[nodiscard]] LegacyWorldPartyRoleActionsResult
    run(const u32 party_count = 2U) {
        return advance_legacy_world_party_role_actions(
            roles,
            spatial,
            LegacyWorldRoleSurfaceContext{
                .map_width = kMapWidth,
                .selected_guid = 1U,
                .surface_grid = surface,
            },
            party_count,
            slots,
            actions
        );
    }
};

void test_aligned_path_removes_spatial_role_and_moves_surface(
    openswd3::test::Context& test
) {
    Fixture fixture;
    LegacyWorldRoleRecord& role = fixture.roles[2];
    LegacyWorldObjectSlot& slot = fixture.slots[1];
    role.flags = 0x00000100U;
    write_u16(slot, kStepXOffset, 16U);
    write_u16(slot, kStepYOffset, 0U);
    slot.bytes[kPathBytesOffset] = 7U;
    write_cell(fixture.surface, Fixture::kOldCell + 1U, 0x0000A800U);
    fixture.actions.update_status =
        LegacyActionUpdateStatus::stream_load_failed;

    const auto result = fixture.run();
    test.expect_true(
        result.status == LegacyWorldPartyRoleActionsStatus::completed &&
            result.slots_scanned == 1U && result.populated_slots == 1U &&
            result.active_path_slots == 1U && result.roles_moved == 1U &&
            result.aligned_updates == 1U && result.cursor_advances == 1U &&
            result.action_update_count == 1U &&
            result.action_update_failure_count == 1U,
        "party slots 1..count-1 retain the active path and diagnostic slots"
    );
    test.expect_true(
        role.world_x == 400U && role.world_y == 304U &&
            role.map_cell_pointer_32 == Fixture::kOldCell + 1U &&
            role.action.variant_delta == 3U && role.flags == 0x20A00100U &&
            read_u16(slot, kPathCursorOffset) == 0x8001U,
        "aligned follower motion updates direction, cell cursor and " "projected flags"
    );
    test.expect_true(
        fixture.spatial.row_heads[0][19U + kLegacySpatialRowPadding] ==
                kLegacySpatialNoRole &&
            read_cell(fixture.surface, Fixture::kOldCell) == 0U &&
            read_cell(fixture.surface, Fixture::kOldCell + 1U) == 0x1000A800U,
        "aligned party movement removes without spatial reinsertion and " "transfers occupancy"
    );
}

void test_inactive_and_waiting_slots_still_update_action(
    openswd3::test::Context& test
) {
    {
        Fixture fixture;
        write_u16(fixture.slots[1], kPathCursorOffset, 0x7FFFU);
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldPartyRoleActionsStatus::completed &&
                result.active_path_slots == 0U && result.roles_moved == 0U &&
                result.action_update_count == 1U &&
                fixture.actions.updates.size() == 1U,
            "an inactive cursor skips path bytes but still refreshes the party " "action"
        );
    }

    {
        Fixture fixture;
        fixture.roles[2].action.wait_remaining = 1U;
        fixture.slots[1].bytes[kPathBytesOffset] = 0xFFU;
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldPartyRoleActionsStatus::completed &&
                result.active_path_slots == 1U && result.roles_moved == 0U &&
                result.action_update_count == 1U &&
                fixture.roles[2].world_x == 384U,
            "the path byte is fetched before the wait gate but not indexed while " "waiting"
        );
    }
}

void test_unaligned_steps_are_not_scaled(openswd3::test::Context& test) {
    Fixture fixture;
    LegacyWorldRoleRecord& role = fixture.roles[2];
    role.flags = 0x04000000U;
    role.action.field_94 = 0U;
    write_u16(fixture.slots[1], kStepXOffset, 2U);
    write_u16(fixture.slots[1], kStepYOffset, 0U);
    fixture.slots[1].bytes[kPathBytesOffset] = 0U;

    const auto result = fixture.run();
    test.expect_true(
        result.status == LegacyWorldPartyRoleActionsStatus::completed &&
            result.roles_moved == 1U && result.aligned_updates == 0U &&
            role.world_x == 386U && role.action.variant_delta == 5U &&
            role.map_cell_pointer_32 == Fixture::kOldCell &&
            read_u16(fixture.slots[1], kPathCursorOffset) == 0U,
        "party step words are added once and ignore map-role doubling flags"
    );
}

void test_modern_ownership_guards(openswd3::test::Context& test) {
    {
        Fixture fixture;
        const auto result = fixture.run(9U);
        test.expect_equal(
            result.status,
            LegacyWorldPartyRoleActionsStatus::invalid_party_role_count,
            "party count cannot address beyond the eight owned slots"
        );
    }

    {
        Fixture fixture;
        write_u16(fixture.slots[1], kPathCursorOffset, 0x300U);
        fixture.roles[2].action.wait_remaining = 1U;
        const auto result = fixture.run();
        test.expect_equal(
            result.status,
            LegacyWorldPartyRoleActionsStatus::path_byte_out_of_range,
            "the original pre-wait path-byte read remains an " "explicit ownership boundary"
        );
    }

    {
        Fixture fixture;
        fixture.slots[1].bytes[kPathBytesOffset] = 8U;
        const auto result = fixture.run();
        test.expect_equal(
            result.status,
            LegacyWorldPartyRoleActionsStatus::direction_out_of_range,
            "direction table access is checked before the modern array boundary"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_aligned_path_removes_spatial_role_and_moves_surface(test);
    test_inactive_and_waiting_slots_still_update_action(test);
    test_unaligned_steps_are_not_scaled(test);
    test_modern_ownership_guards(test);
    return test.exit_code();
}
