#include "openswd3/world_map/legacy_world_map_role_paths.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kRoleIndexOffset = 0x00U;
constexpr std::size_t kPathCursorOffset = 0x02U;
constexpr std::size_t kDestinationXOffset = 0x04U;
constexpr std::size_t kDestinationYOffset = 0x06U;
constexpr std::size_t kActionIdOffset = 0x10U;
constexpr std::size_t kBaseVariantOffset = 0x12U;
constexpr std::size_t kVariantDeltaOffset = 0x14U;
constexpr std::size_t kStepXOffset = 0x16U;
constexpr std::size_t kStepYOffset = 0x18U;
constexpr std::size_t kPathFlagsOffset = 0x1BU;
constexpr std::size_t kPathBytesOffset = 0x1CU;

constexpr u16 kNoRole = 0xFFFFU;
constexpr u16 kInactivePathCursor = 0x7FFFU;
constexpr u16 kPathCursorFrameGate = 0x8000U;
constexpr u16 kNoActionOverride = 0xFFFFU;

constexpr u32 kPathRoleFlag = 0x00008000U;
constexpr u32 kInteractionGateFlag = 0x00000800U;
constexpr u32 kDoublePathStepFlag = 0x04000000U;
constexpr u32 kArrivalFlagMask = 0xBBFFFFFFU;
constexpr u32 kArrivalFlag = 0x02000000U;
constexpr u32 kClearPathCompletionFlag = 0x7FFFFFFFU;
constexpr u32 kClearArrivalPendingFlag = 0xFFFBFFFFU;
constexpr u32 kAutomaticTalkFlag = 0x00002000U;
constexpr u32 kAutomaticTalkCellMask = 0x00000100U;
constexpr u8 kCameraRecenterSuppressedFlag = 0x02U;

constexpr std::array<i32, 8U> kCellStepX{
    1,
    0,
    -1,
    -1,
    -1,
    0,
    1,
    1,
};
constexpr std::array<i32, 8U> kCellStepY{
    1,
    1,
    1,
    0,
    -1,
    -1,
    -1,
    0,
};
constexpr std::array<u32, 8U> kDirectionToVariant{
    5U,
    1U,
    6U,
    2U,
    4U,
    0U,
    7U,
    3U,
};

[[nodiscard]] u16 read_u16_le(
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_u16_le(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] i32 read_i16_as_i32(
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<i32>(std::bit_cast<i16>(read_u16_le(slot, offset)));
}

[[nodiscard]] u32 sign_extend_u16_bits(const u16 value) noexcept {
    return static_cast<u32>(static_cast<i32>(std::bit_cast<i16>(value)));
}

[[nodiscard]] u32 scaled_step_bits(
    const i32 value, const bool double_once, const bool double_twice
) noexcept {
    u32 bits = std::bit_cast<u32>(value);
    if (double_once) {
        bits <<= 1U;
    }
    if (double_twice) {
        bits <<= 1U;
    }
    return bits;
}

[[nodiscard]] bool relocate_aligned_role(
    LegacyWorldMapRolePathResult& result,
    LegacyWorldRoleRecord& role,
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex& spatial_index
) noexcept {
    const u32 first_row_bits = (role.world_y >> 4U) - 1U;
    result.spatial_status = relocate_legacy_role_spatially_by_guid(
        spatial_index,
        roles,
        role.guid,
        role.flags & 3U,
        std::bit_cast<i32>(first_row_bits),
        true
    );
    if (result.spatial_status == LegacyRoleSpatialRelocationStatus::ready) {
        return true;
    }
    result.status = LegacyWorldMapRolePathStatus::spatial_relocation_failed;
    return false;
}

[[nodiscard]] bool clear_and_move_surface_cell(
    LegacyWorldMapRolePathResult& result,
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u8 direction
) noexcept {
    const auto cleared =
        clear_legacy_world_role_surface_occupancy(role, surface_context);
    result.surface_status = cleared.status;
    if (cleared.status != LegacyWorldRoleSurfaceStatus::ready) {
        result.status = LegacyWorldMapRolePathStatus::surface_clear_failed;
        return false;
    }

    if (direction != 0xFFU) {
        const u32 vertical = std::bit_cast<u32>(kCellStepY[direction]);
        const u32 horizontal = std::bit_cast<u32>(kCellStepX[direction]);
        role.map_cell_pointer_32 +=
            vertical * surface_context.map_width + horizontal;
    }

    const auto marked =
        mark_legacy_world_role_surface_occupancy(role, surface_context);
    result.surface_status = marked.status;
    if (marked.status != LegacyWorldRoleSurfaceStatus::ready) {
        result.status = LegacyWorldMapRolePathStatus::surface_mark_failed;
        return false;
    }
    return true;
}

[[nodiscard]] bool refresh_cell_flags(
    LegacyWorldMapRolePathResult& result,
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& surface_context
) noexcept {
    if ((role.flags & 0x00000100U) == 0U) {
        return true;
    }
    if (refresh_legacy_world_role_cell_flags(
            role, surface_context.surface_grid
        ) == LegacyWorldRoleCellFlagRefreshStatus::ready) {
        return true;
    }
    result.status = LegacyWorldMapRolePathStatus::cell_flag_refresh_failed;
    return false;
}

void recenter_camera(
    LegacyWorldMapRolePathResult& result,
    const LegacyWorldRoleRecord& role,
    const LegacyRoleSpatialIndex& spatial_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    LegacyWorldCameraRect& camera
) noexcept {
    recenter_legacy_world_camera(
        role, surface_context.map_width, spatial_index.map_height, camera
    );
    ++result.camera_recenter_count;
}

[[nodiscard]] bool create_automatic_talk_context(
    LegacyWorldMapRolePathResult& result,
    const LegacyWorldRoleRecord& role,
    const LegacyRoleSpatialIndex& spatial_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    LegacyWorldTalkContext& talk_context
) noexcept {
    if (talk_context.source_guid != kLegacyWorldTalkIdleSource ||
        (role.flags & kAutomaticTalkFlag) == 0U) {
        return true;
    }

    const auto occupancy = compute_legacy_world_directional_occupancy_mask(
        surface_context.surface_grid,
        surface_context.map_width,
        spatial_index.map_height,
        role.map_cell_pointer_32,
        role.action.field_2c,
        role.action.field_30,
        kAutomaticTalkCellMask
    );
    result.directional_probe_status = occupancy.status;
    if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
        result.status = LegacyWorldMapRolePathStatus::directional_probe_failed;
        return false;
    }
    if (occupancy.mask == 0U) {
        return true;
    }

    talk_context.talk_data_offset = role.talk_data_offset;
    talk_context.instruction_offset = role.talk_initial_offset;
    talk_context.talk_script_id = role.talk_script_id;
    talk_context.source_guid = role.guid;
    talk_context.source_flags = role.flags;
    result.talk_context_created = true;
    return true;
}

[[nodiscard]] bool handle_arrival(
    LegacyWorldMapRolePathResult& result,
    const u32 role_index,
    LegacyWorldRoleRecord& role,
    LegacyWorldObjectSlot& slot,
    const u8 runtime_flags,
    LegacyWorldMovementRuntimeState& movement,
    LegacyWorldCameraRect& camera,
    LegacyWorldMapRolePathState& state,
    const LegacyRoleSpatialIndex& spatial_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    LegacyWorldMapRolePathPorts& path_ports
) noexcept {
    if (role.world_x != read_u16_le(slot, kDestinationXOffset) ||
        role.world_y != read_u16_le(slot, kDestinationYOffset)) {
        return true;
    }

    ++result.arrivals;
    role.path_wait_remaining = 0U;
    role.flags = (role.flags & kArrivalFlagMask) | kArrivalFlag;

    const u16 action_id = read_u16_le(slot, kActionIdOffset);
    if (action_id != kNoActionOverride) {
        role.action.action_id = sign_extend_u16_bits(action_id);
    }
    const u16 base_variant = read_u16_le(slot, kBaseVariantOffset);
    if (base_variant != kNoActionOverride) {
        role.action.base_variant = sign_extend_u16_bits(base_variant);
    }
    const u16 variant_delta = read_u16_le(slot, kVariantDeltaOffset);
    if (variant_delta != kNoActionOverride) {
        role.action.variant_delta = sign_extend_u16_bits(variant_delta);
    }

    if (slot.bytes[kPathFlagsOffset] < 0x80U) {
        ++result.path_completion_calls;
        if (!path_ports.complete_role_path(role_index)) {
            result.status =
                LegacyWorldMapRolePathStatus::path_completion_port_failed;
            return false;
        }
        role.action.wait_remaining = 0U;
        role.flags &= kClearPathCompletionFlag;
    }

    if (role.guid == 1U) {
        movement.camera_x_transition = 0;
        movement.player_x_transition = 0;
        movement.camera_y_transition = 0;
        movement.player_y_transition = 0;
        std::ranges::fill(state.guid_one_arrival_bytes, u8{});
        if ((runtime_flags & kCameraRecenterSuppressedFlag) == 0U) {
            recenter_camera(
                result, role, spatial_index, surface_context, camera
            );
        }
    }
    role.flags &= kClearArrivalPendingFlag;
    return true;
}

}  // namespace

LegacyWorldMapRolePathResult advance_legacy_world_map_role_paths(
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex& spatial_index,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u32 selected_role_index,
    const u8 runtime_flags,
    LegacyWorldMovementRuntimeState& movement,
    LegacyWorldCameraRect& camera,
    LegacyWorldMapRolePathState& state,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    LegacyWorldMapRolePathPorts& path_ports
) {
    LegacyWorldMapRolePathResult result;
    if (selected_role_index >= roles.size()) {
        result.status =
            LegacyWorldMapRolePathStatus::invalid_selected_role_index;
        return result;
    }

    for (LegacyWorldObjectSlot& slot : state.active_object_slots) {
        ++result.slots_scanned;
        const u16 role_index_word = read_u16_le(slot, kRoleIndexOffset);
        if (role_index_word == kNoRole) {
            continue;
        }
        const u16 path_cursor = read_u16_le(slot, kPathCursorOffset);
        if (path_cursor >= kInactivePathCursor) {
            continue;
        }
        const u32 role_index = role_index_word;
        if (role_index >= roles.size()) {
            result.status = LegacyWorldMapRolePathStatus::invalid_role_index;
            return result;
        }

        LegacyWorldRoleRecord& role = roles[role_index];
        if ((role.flags & kPathRoleFlag) == 0U) {
            continue;
        }
        ++result.active_slots;
        if (role.guid == state.talk_context.source_guid &&
            (slot.bytes[kPathFlagsOffset] & 0x0FU) == 1U) {
            continue;
        }
        if ((role.flags & kInteractionGateFlag) != 0U &&
            role.interaction_gate == 1U) {
            continue;
        }

        if (role.action.wait_remaining == 0U) {
            const std::size_t direction_offset = kPathBytesOffset + path_cursor;
            if (direction_offset >= slot.bytes.size()) {
                result.status =
                    LegacyWorldMapRolePathStatus::path_byte_out_of_range;
                return result;
            }
            const u8 direction = slot.bytes[direction_offset];
            if (direction != 0xFFU && direction >= kCellStepX.size()) {
                result.status =
                    LegacyWorldMapRolePathStatus::direction_out_of_range;
                return result;
            }

            const bool base_double = role.action.field_94 == 0U;
            const bool flag_double = (role.flags & kDoublePathStepFlag) != 0U;
            const u32 step_x = scaled_step_bits(
                read_i16_as_i32(slot, kStepXOffset), base_double, flag_double
            );
            const u32 step_y = scaled_step_bits(
                read_i16_as_i32(slot, kStepYOffset), base_double, flag_double
            );
            if (direction != 0xFFU) {
                role.action.variant_delta = kDirectionToVariant[direction];
            }
            role.world_x += step_x;
            role.world_y += step_y;
            ++result.roles_moved;

            if (((role.world_x | role.world_y) & 0x0FU) == 0U) {
                ++result.aligned_updates;
                if (!relocate_aligned_role(
                        result, role, roles, spatial_index
                    ) ||
                    !handle_arrival(
                        result,
                        role_index,
                        role,
                        slot,
                        runtime_flags,
                        movement,
                        camera,
                        state,
                        spatial_index,
                        surface_context,
                        path_ports
                    ) ||
                    !clear_and_move_surface_cell(
                        result, role, surface_context, direction
                    )) {
                    return result;
                }

                const u16 current_cursor = read_u16_le(slot, kPathCursorOffset);
                write_u16_le(
                    slot,
                    kPathCursorOffset,
                    static_cast<u16>(current_cursor + 1U) | kPathCursorFrameGate
                );
                ++result.cursor_advances;
                if (!refresh_cell_flags(result, role, surface_context)) {
                    return result;
                }
            }

            if (!create_automatic_talk_context(
                    result,
                    role,
                    spatial_index,
                    surface_context,
                    state.talk_context
                )) {
                return result;
            }
        }

        ++result.action_update_count;
        if (action_ports.update_action_record(role.action) !=
            asset_runtime::LegacyActionUpdateStatus::completed) {
            ++result.action_update_failure_count;
        }
        if (role_index == selected_role_index &&
            (runtime_flags & kCameraRecenterSuppressedFlag) == 0U) {
            recenter_camera(
                result, role, spatial_index, surface_context, camera
            );
        }
    }
    return result;
}

}  // namespace openswd3::world_map
