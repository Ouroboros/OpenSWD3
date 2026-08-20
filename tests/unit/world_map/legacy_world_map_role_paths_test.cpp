#include "test.hpp"

#include "openswd3/world_map/legacy_world_map_role_paths.hpp"
#include "openswd3/world_map/legacy_world_story_paths.hpp"

#include <algorithm>
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
using openswd3::world_map::advance_legacy_world_map_role_paths;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldMapRolePathPorts;
using openswd3::world_map::LegacyWorldMapRolePathResult;
using openswd3::world_map::LegacyWorldMapRolePathState;
using openswd3::world_map::LegacyWorldMapRolePathStatus;
using openswd3::world_map::LegacyWorldMovementRuntimeState;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::LegacyWorldStoryPathRequest;
using openswd3::world_map::LegacyWorldStoryPathRuntime;
using openswd3::world_map::LegacyWorldStoryPathStatus;

constexpr std::size_t kPathCursorOffset = 0x02U;
constexpr std::size_t kDestinationXOffset = 0x04U;
constexpr std::size_t kDestinationYOffset = 0x06U;
constexpr std::size_t kSavedRoleIndexOffset = 0x08U;
constexpr std::size_t kSavedDestinationXOffset = 0x0CU;
constexpr std::size_t kSavedDestinationYOffset = 0x0EU;
constexpr std::size_t kActionIdOffset = 0x10U;
constexpr std::size_t kBaseVariantOffset = 0x12U;
constexpr std::size_t kVariantDeltaOffset = 0x14U;
constexpr std::size_t kStepXOffset = 0x16U;
constexpr std::size_t kStepYOffset = 0x18U;
constexpr std::size_t kPathFlagsOffset = 0x1BU;
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

class RecordingPathPorts final : public LegacyWorldMapRolePathPorts {
public:
    [[nodiscard]] bool
    complete_role_path(const u32 role_index) noexcept override {
        ++calls;
        last_role_index = role_index;
        if (mutated_slot != nullptr) {
            write_u16(*mutated_slot, kPathCursorOffset, mutated_cursor);
        }
        return succeeds;
    }

    bool succeeds{true};
    LegacyWorldObjectSlot* mutated_slot{};
    u16 mutated_cursor{};
    u32 calls{};
    u32 last_role_index{0xFFFFFFFFU};
};

struct Fixture {
    static constexpr u32 kMapWidth = 50U;
    static constexpr u32 kMapHeight = 40U;
    static constexpr u32 kOldCell = 19U * kMapWidth + 24U;

    std::array<LegacyWorldRoleRecord, 2U> roles{};
    LegacyRoleSpatialIndex spatial;
    std::vector<u8> surface = std::vector<u8>(kMapWidth * kMapHeight * 4U, 0U);
    LegacyWorldMapRolePathState state;
    LegacyWorldMovementRuntimeState movement{
        .camera_x_transition = 3,
        .player_x_transition = 4,
        .camera_y_transition = 5,
        .player_y_transition = 6,
    };
    LegacyWorldCameraRect camera{};
    openswd3::world_map::LegacyWorldPathNodePool node_pool;
    u8 scene_render_flags{};
    RecordingActionPorts actions;
    RecordingPathPorts paths;

    Fixture() {
        LegacyWorldRoleRecord& role = roles[1];
        role.world_x = 384U;
        role.world_y = 304U;
        role.map_cell_pointer_32 = kOldCell;
        role.flags = 0x00008000U;
        role.guid = 7U;
        role.action.field_2c = 1U;
        role.action.field_30 = 1U;
        role.action.field_94 = 1U;

        spatial.map_height = kMapHeight;
        const std::size_t row_count = static_cast<std::size_t>(kMapHeight) +
            2U * kLegacySpatialRowPadding;
        for (auto& rows : spatial.row_heads) {
            rows.assign(row_count, kLegacySpatialNoRole);
        }
        static_cast<void>(insert_legacy_role_spatially(
            spatial, roles, 1U, roles[1U].flags & 3U
        ));
        write_cell(surface, kOldCell, 0x10000100U);

        LegacyWorldObjectSlot& slot = state.active_object_slots[0];
        write_u16(slot, 0x00U, 1U);
        write_u16(slot, kPathCursorOffset, 0U);
    }

    [[nodiscard]] LegacyWorldMapRolePathResult
    run(const u8 runtime_flags = 0U) {
        return advance_legacy_world_map_role_paths(
            roles,
            spatial,
            LegacyWorldRoleSurfaceContext{
                .map_width = kMapWidth,
                .selected_guid = roles[1].guid,
                .surface_grid = surface,
            },
            1U,
            runtime_flags,
            movement,
            camera,
            state,
            actions,
            paths
        );
    }

    [[nodiscard]] LegacyWorldStoryPathRuntime story_runtime() {
        return {
            .roles = roles,
            .active_object_slots = state.active_object_slots,
            .spatial_index = &spatial,
            .role_surface =
                {
                    .map_width = kMapWidth,
                    .selected_guid = roles[1].guid,
                    .surface_grid = surface,
                },
            .node_pool = &node_pool,
            .movement = &movement,
            .camera = &camera,
            .selected_arrival_bytes = state.guid_one_arrival_bytes,
            .selected_role_index = 1U,
            .map_height = kMapHeight,
            .scene_render_flags = &scene_render_flags,
        };
    }
};

void test_story_path_schedule_query_and_completion(
    openswd3::test::Context& test
) {
    Fixture fixture;
    LegacyWorldStoryPathRuntime released_runtime{};
    released_runtime.roles = fixture.roles;
    const auto already_released =
        openswd3::world_map::complete_legacy_world_story_path(
            released_runtime, 1U
        );
    test.expect_true(
        already_released.status == LegacyWorldStoryPathStatus::completed &&
            already_released.legacy_return_value == 1,
        "sub_42D920 returns before touching slots when bit31 is clear"
    );

    for (auto& slot : fixture.state.active_object_slots) {
        slot.bytes.fill(0xFFU);
    }
    fixture.camera.right = 640U;
    fixture.camera.bottom = 480U;
    auto runtime = fixture.story_runtime();

    const auto scheduled =
        openswd3::world_map::schedule_legacy_world_story_path(
            runtime,
            LegacyWorldStoryPathRequest{
                .role_index = 1U,
                .destination_x = 400U,
                .destination_y = 304U,
            }
        );
    auto& slot = fixture.state.active_object_slots[0];
    test.expect_true(
        scheduled.status == LegacyWorldStoryPathStatus::completed &&
            scheduled.legacy_return_value == 1 &&
            scheduled.free_slot_allocated && scheduled.path_found &&
            scheduled.slot_index == 0U && read_u16(slot, 0U) == 1U &&
            read_u16(slot, kPathCursorOffset) == 0x8000U &&
            read_u16(slot, kDestinationXOffset) == 400U &&
            read_u16(slot, kDestinationYOffset) == 304U &&
            (slot.bytes[kPathFlagsOffset] & 0x0FU) == 2U &&
            (fixture.roles[1].flags & 0x80000000U) != 0U,
        "sub_42DAF0 schedules a type-two path in the first free ordinary slot"
    );

    const auto prepared =
        openswd3::world_map::query_legacy_world_story_path(runtime, 1U);
    test.expect_true(
        prepared.status == LegacyWorldStoryPathStatus::completed &&
            prepared.legacy_return_value == 1 &&
            read_u16(slot, kPathCursorOffset) == 0U &&
            read_u16(slot, kStepXOffset) == 4U &&
            read_u16(slot, kStepYOffset) == 0U &&
            (fixture.roles[1].flags & 0x40000000U) != 0U,
        "sub_42E280 opens the cursor gate and arms one four-pixel step"
    );

    const auto completed =
        openswd3::world_map::complete_legacy_world_story_path(runtime, 1U);
    test.expect_true(
        completed.status == LegacyWorldStoryPathStatus::completed &&
            completed.legacy_return_value == 1 && completed.slot_cleared &&
            std::ranges::all_of(
                slot.bytes, [](const u8 value) { return value == 0xFFU; }
            ),
        "sub_42D920 clears an unchained completed type-two slot"
    );
}

void test_story_path_offscreen_preadvance(openswd3::test::Context& test) {
    Fixture fixture;
    for (auto& slot : fixture.state.active_object_slots) {
        slot.bytes.fill(0xFFU);
    }
    fixture.camera.left = 1000U;
    fixture.camera.right = 1640U;
    fixture.camera.bottom = 480U;
    auto runtime = fixture.story_runtime();

    const auto scheduled =
        openswd3::world_map::schedule_legacy_world_story_path(
            runtime,
            LegacyWorldStoryPathRequest{
                .role_index = 1U,
                .destination_x = 400U,
                .destination_y = 304U,
            }
        );
    const auto& slot = fixture.state.active_object_slots[0];
    test.expect_true(
        scheduled.status == LegacyWorldStoryPathStatus::completed &&
            scheduled.preadvanced_steps == 1U &&
            fixture.roles[1].world_x == 400U &&
            fixture.roles[1].world_y == 304U &&
            read_u16(slot, kPathCursorOffset) == 0x8001U,
        "sub_42DAF0 advances one full 16-pixel path tile while offscreen"
    );
}

void test_story_role_suspension_preserves_selected_path(
    openswd3::test::Context& test
) {
    Fixture fixture;
    auto& role = fixture.roles[1];
    auto& slot = fixture.state.active_object_slots[0];
    role.world_x = 388U;
    role.world_y = 308U;
    fixture.movement.camera_x_transition = 0;
    fixture.movement.player_x_transition = 4;
    fixture.movement.camera_y_transition = 0;
    fixture.movement.player_y_transition = 4;
    fixture.state.guid_one_arrival_bytes.fill(0xCCU);
    write_u16(slot, kDestinationXOffset, 432U);
    write_u16(slot, kDestinationYOffset, 336U);
    slot.bytes[kPathFlagsOffset] = 1U;
    auto runtime = fixture.story_runtime();

    const auto result =
        openswd3::world_map::suspend_legacy_world_story_role(runtime, 1U);
    test.expect_true(
        result.status == LegacyWorldStoryPathStatus::completed &&
            result.legacy_return_value == 1 && result.existing_slot_found &&
            result.slot_index == 0U && role.world_x == 384U &&
            role.world_y == 304U && (role.flags & 0x80000000U) != 0U &&
            fixture.movement.camera_x_transition == 0 &&
            fixture.movement.player_x_transition == 0 &&
            fixture.movement.camera_y_transition == 0 &&
            fixture.movement.player_y_transition == 0,
        "sub_42E5A0 aligns and suspends the selected story role"
    );
    test.expect_true(
        read_u16(slot, kPathCursorOffset) == 0x8000U &&
            read_u16(slot, kSavedRoleIndexOffset) == 1U &&
            read_u16(slot, kSavedDestinationXOffset) == 432U &&
            read_u16(slot, kSavedDestinationYOffset) == 336U &&
            std::ranges::all_of(
                fixture.state.guid_one_arrival_bytes,
                [](const u8 value) { return value == 0xCCU; }
            ) &&
            read_cell(fixture.surface, Fixture::kOldCell) == 0x10000100U,
        "sub_42E5A0 preserves the ordinary path without clearing arrival or " "surface state for the selected role"
    );
}

void test_arrival_replays_exact_state_order(openswd3::test::Context& test) {
    Fixture fixture;
    LegacyWorldRoleRecord& role = fixture.roles[1];
    LegacyWorldObjectSlot& slot = fixture.state.active_object_slots[0];
    role.flags = 0xC4008100U;
    write_u16(slot, kDestinationXOffset, 400U);
    write_u16(slot, kDestinationYOffset, 320U);
    write_u16(slot, kActionIdOffset, 0x8001U);
    write_u16(slot, kBaseVariantOffset, 2U);
    write_u16(slot, kVariantDeltaOffset, 3U);
    write_u16(slot, kStepXOffset, 8U);
    write_u16(slot, kStepYOffset, 8U);
    slot.bytes[kPathFlagsOffset] = 0U;
    slot.bytes[kPathBytesOffset] = 0U;
    fixture.actions.update_status =
        LegacyActionUpdateStatus::stream_load_failed;
    fixture.paths.mutated_slot = &slot;
    fixture.paths.mutated_cursor = 7U;

    const auto result = fixture.run();
    const u32 new_cell = Fixture::kOldCell + Fixture::kMapWidth + 1U;
    test.expect_true(
        result.status == LegacyWorldMapRolePathStatus::completed &&
            result.slots_scanned == 72U && result.active_slots == 1U &&
            result.roles_moved == 1U && result.aligned_updates == 1U &&
            result.arrivals == 1U && result.path_completion_calls == 1U &&
            result.cursor_advances == 1U && result.action_update_count == 1U &&
            result.action_update_failure_count == 1U &&
            result.camera_recenter_count == 1U,
        "the 72-slot loop preserves movement, arrival, diagnostic and camera " "order"
    );
    test.expect_true(
        role.world_x == 400U && role.world_y == 320U &&
            role.map_cell_pointer_32 == new_cell && role.flags == 0x02008100U &&
            role.path_wait_remaining == 0U &&
            role.action.wait_remaining == 0U &&
            role.action.action_id == 0xFFFF8001U &&
            role.action.base_variant == 2U && role.action.variant_delta == 3U,
        "arrival applies signed action overrides and the exact flag masks"
    );
    test.expect_true(
        fixture.paths.calls == 1U && fixture.paths.last_role_index == 1U &&
            read_u16(slot, kPathCursorOffset) == 0x8008U,
        "the cursor is re-read after the story/path completion callback"
    );
    test.expect_true(
        read_cell(fixture.surface, Fixture::kOldCell) == 0U &&
            read_cell(fixture.surface, new_cell) == 0x10000100U &&
            fixture.spatial.row_heads[0][19U + kLegacySpatialRowPadding] ==
                kLegacySpatialNoRole &&
            fixture.spatial.row_heads[0][20U + kLegacySpatialRowPadding] == 1U,
        "aligned motion transfers both spatial-list and surface ownership"
    );
    test.expect_true(
        fixture.actions.updates.size() == 1U &&
            fixture.actions.updates[0].variant_delta == 3U &&
            fixture.camera.left == 96U && fixture.camera.top == 80U &&
            fixture.camera.right == 736U && fixture.camera.bottom == 560U,
        "action validation sees arrival overrides before selected-role " "recentering"
    );
}

void test_wait_and_skip_gates_keep_their_slots(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.roles[1].action.wait_remaining = 2U;
        write_u16(fixture.state.active_object_slots[0], kStepXOffset, 16U);
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldMapRolePathStatus::completed &&
                result.roles_moved == 0U && result.aligned_updates == 0U &&
                result.action_update_count == 1U &&
                result.camera_recenter_count == 1U &&
                fixture.roles[1].world_x == 384U &&
                read_u16(
                    fixture.state.active_object_slots[0], kPathCursorOffset
                ) == 0U,
            "a live action wait bypasses motion but retains update " "and camera slots"
        );
    }

    {
        Fixture fixture;
        fixture.state.talk_context.source_guid = fixture.roles[1].guid;
        fixture.state.active_object_slots[0].bytes[kPathFlagsOffset] = 1U;
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldMapRolePathStatus::completed &&
                result.active_slots == 1U && result.roles_moved == 0U &&
                result.action_update_count == 0U &&
                result.camera_recenter_count == 0U &&
                fixture.actions.updates.empty(),
            "matching Talk source plus low-nibble one skips the entire role body"
        );
    }

    {
        Fixture fixture;
        fixture.roles[1].flags |= 0x00000800U;
        fixture.roles[1].interaction_gate = 1U;
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldMapRolePathStatus::completed &&
                result.roles_moved == 0U && result.action_update_count == 0U,
            "bit-eleven interaction state skips at the second assembly gate"
        );
    }
}

void test_unaligned_motion_defers_cell_bookkeeping(
    openswd3::test::Context& test
) {
    Fixture fixture;
    LegacyWorldObjectSlot& slot = fixture.state.active_object_slots[0];
    write_u16(slot, kStepXOffset, 2U);
    write_u16(slot, kStepYOffset, 0U);
    slot.bytes[kPathBytesOffset] = 7U;

    const auto result = fixture.run();
    test.expect_true(
        result.status == LegacyWorldMapRolePathStatus::completed &&
            result.roles_moved == 1U && result.aligned_updates == 0U &&
            result.cursor_advances == 0U && result.action_update_count == 1U &&
            fixture.roles[1].world_x == 386U &&
            fixture.roles[1].map_cell_pointer_32 == Fixture::kOldCell &&
            fixture.roles[1].action.variant_delta == 3U &&
            read_u16(slot, kPathCursorOffset) == 0U &&
            read_cell(fixture.surface, Fixture::kOldCell) == 0x10000100U,
        "sub-cell movement changes coordinates/action only until 16-pixel " "alignment"
    );
}

void test_guid_one_arrival_resets_motion_and_recenters(
    openswd3::test::Context& test
) {
    for (const u8 runtime_flags : std::array<u8, 2U>{0U, 2U}) {
        Fixture fixture;
        LegacyWorldRoleRecord& role = fixture.roles[1];
        LegacyWorldObjectSlot& slot = fixture.state.active_object_slots[0];
        role.guid = 1U;
        write_u16(slot, kDestinationXOffset, 400U);
        write_u16(slot, kDestinationYOffset, 304U);
        write_u16(slot, kStepXOffset, 16U);
        write_u16(slot, kStepYOffset, 0U);
        slot.bytes[kPathFlagsOffset] = 0x80U;
        slot.bytes[kPathBytesOffset] = 7U;
        fixture.state.guid_one_arrival_bytes.fill(0xCCU);

        const auto result = fixture.run(runtime_flags);
        test.expect_true(
            result.status == LegacyWorldMapRolePathStatus::completed &&
                result.arrivals == 1U && result.path_completion_calls == 0U &&
                fixture.movement.camera_x_transition == 0 &&
                fixture.movement.player_x_transition == 0 &&
                fixture.movement.camera_y_transition == 0 &&
                fixture.movement.player_y_transition == 0 &&
                std::ranges::all_of(
                    fixture.state.guid_one_arrival_bytes,
                    [](const u8 value) { return value == 0U; }
                ) &&
                result.camera_recenter_count == (runtime_flags == 0U ? 2U : 0U),
            "GUID one arrival clears four transitions and 0x200 bytes before " "camera gates"
        );
    }
}

void test_automatic_talk_and_failure_boundary(openswd3::test::Context& test) {
    {
        Fixture fixture;
        LegacyWorldRoleRecord& role = fixture.roles[1];
        LegacyWorldObjectSlot& slot = fixture.state.active_object_slots[0];
        role.flags |= 0x00002000U;
        role.talk_data_offset = 0x12345678U;
        role.talk_script_id = 0x3456U;
        role.talk_initial_offset = 0x789AU;
        write_u16(slot, kStepXOffset, 0U);
        write_u16(slot, kStepYOffset, 0U);
        slot.bytes[kPathBytesOffset] = 0xFFU;
        write_cell(
            fixture.surface, Fixture::kOldCell - Fixture::kMapWidth - 1U, 0x100U
        );

        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldMapRolePathStatus::completed &&
                result.talk_context_created &&
                fixture.state.talk_context.source_guid == role.guid &&
                fixture.state.talk_context.source_flags == role.flags &&
                fixture.state.talk_context.talk_data_offset == 0x12345678U &&
                fixture.state.talk_context.talk_script_id == 0x3456U &&
                fixture.state.talk_context.instruction_offset == 0x789AU,
            "idle Talk context is populated from the first flagged nearby role"
        );
    }

    {
        Fixture fixture;
        LegacyWorldObjectSlot& slot = fixture.state.active_object_slots[0];
        write_u16(slot, kDestinationXOffset, 400U);
        write_u16(slot, kDestinationYOffset, 304U);
        write_u16(slot, kStepXOffset, 16U);
        write_u16(slot, kStepYOffset, 0U);
        slot.bytes[kPathFlagsOffset] = 0U;
        slot.bytes[kPathBytesOffset] = 7U;
        fixture.paths.succeeds = false;

        const auto result = fixture.run();
        test.expect_true(
            result.status ==
                    LegacyWorldMapRolePathStatus::path_completion_port_failed &&
                result.path_completion_calls == 1U &&
                result.cursor_advances == 0U &&
                result.action_update_count == 0U &&
                fixture.roles[1].world_x == 400U &&
                fixture.roles[1].map_cell_pointer_32 == Fixture::kOldCell &&
                read_cell(fixture.surface, Fixture::kOldCell) == 0x10000100U,
            "an unconnected story/path owner stops at sub_42D920 without faking " "later state"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_story_path_schedule_query_and_completion(test);
    test_story_path_offscreen_preadvance(test);
    test_story_role_suspension_preserves_selected_path(test);
    test_arrival_replays_exact_state_order(test);
    test_wait_and_skip_gates_keep_their_slots(test);
    test_unaligned_motion_defers_cell_bookkeeping(test);
    test_guid_one_arrival_resets_motion_and_recenters(test);
    test_automatic_talk_and_failure_boundary(test);
    return test.exit_code();
}
