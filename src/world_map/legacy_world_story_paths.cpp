#include "openswd3/world_map/legacy_world_story_paths.hpp"

#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <new>

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
constexpr std::size_t kSavedRoleIndexOffset = 0x08U;
constexpr std::size_t kSavedDestinationXOffset = 0x0CU;
constexpr std::size_t kSavedDestinationYOffset = 0x0EU;
constexpr std::size_t kActionIdOffset = 0x10U;
constexpr std::size_t kBaseVariantOffset = 0x12U;
constexpr std::size_t kVariantDeltaOffset = 0x14U;
constexpr std::size_t kStepXOffset = 0x16U;
constexpr std::size_t kStepYOffset = 0x18U;
constexpr std::size_t kPathStallOffset = 0x1AU;
constexpr std::size_t kPathFlagsOffset = 0x1BU;
constexpr std::size_t kPathBytesOffset = 0x1CU;

constexpr u16 kNoRole = 0xFFFFU;
constexpr u16 kPathCursorMask = 0x7FFFU;
constexpr u16 kPathCursorFrameGate = 0x8000U;
constexpr u32 kCollisionBypassFlag = 0x00040000U;
constexpr u32 kPreparedMovementFlag = 0x40000000U;
constexpr u32 kPathOwnershipFlag = 0x80000000U;
constexpr u32 kArrivalClearMask = 0xBBFFFFFFU;
constexpr u32 kStoryCollisionMask = 0x40000000U;

constexpr std::array<i32, 8U> kSubCellStepX{4, 0, -4, -4, -4, 0, 4, 4};
constexpr std::array<i32, 8U> kSubCellStepY{4, 4, 4, 0, -4, -4, -4, 0};
constexpr i32 kOffscreenPathStepScale = 4;
constexpr std::array<u32, 8U> kDirectionCollisionBits{
    0x10U, 0x20U, 0x40U, 0x80U, 0x01U, 0x02U, 0x04U, 0x08U
};
constexpr std::array<u32, 8U> kDirectionToVariant{
    5U, 1U, 6U, 2U, 4U, 0U, 7U, 3U
};

[[nodiscard]] u16
read_u16(const LegacyWorldObjectSlot& slot, const std::size_t offset) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_u16(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] bool
runtime_is_available(const LegacyWorldStoryPathRuntime& runtime) noexcept {
    return runtime.spatial_index != nullptr && runtime.node_pool != nullptr &&
        runtime.movement != nullptr && runtime.camera != nullptr &&
        runtime.scene_render_flags != nullptr && runtime.map_height != 0U &&
        runtime.role_surface.map_width != 0U &&
        !runtime.role_surface.surface_grid.empty() &&
        runtime.active_object_slots.size() >= kLegacyWorldActiveObjectSlotCount;
}

[[nodiscard]] bool
path_fits_slot(const LegacyWorldPathfindingResult& path) noexcept {
    return path.path.size() <= kLegacyWorldObjectSlotSize - kPathBytesOffset;
}

void copy_path(
    const LegacyWorldPathfindingResult& path, LegacyWorldObjectSlot& slot
) noexcept {
    std::ranges::copy(
        path.path,
        slot.bytes.begin() + static_cast<std::ptrdiff_t>(kPathBytesOffset)
    );
}

[[nodiscard]] bool clear_surface(
    LegacyWorldStoryPathResult& result,
    const LegacyWorldRoleRecord& role,
    const LegacyWorldStoryPathRuntime& runtime
) noexcept {
    const auto cleared =
        clear_legacy_world_role_surface_occupancy(role, runtime.role_surface);
    result.surface_status = cleared.status;
    if (cleared.status == LegacyWorldRoleSurfaceStatus::ready) {
        return true;
    }
    result.status = LegacyWorldStoryPathStatus::surface_clear_failed;
    return false;
}

[[nodiscard]] bool mark_surface(
    LegacyWorldStoryPathResult& result,
    const LegacyWorldRoleRecord& role,
    const LegacyWorldStoryPathRuntime& runtime
) noexcept {
    const auto marked =
        mark_legacy_world_role_surface_occupancy(role, runtime.role_surface);
    result.surface_status = marked.status;
    if (marked.status == LegacyWorldRoleSurfaceStatus::ready) {
        return true;
    }
    result.status = LegacyWorldStoryPathStatus::surface_mark_failed;
    return false;
}

[[nodiscard]] bool relocate_role(
    LegacyWorldStoryPathResult& result,
    const LegacyWorldRoleRecord& role,
    LegacyWorldStoryPathRuntime& runtime,
    const i32 first_row = 0
) noexcept {
    const auto spatial_result = relocate_legacy_role_spatially_by_guid(
        *runtime.spatial_index,
        runtime.roles,
        role.guid,
        role.flags & 3U,
        first_row,
        true
    );
    result.spatial_status = spatial_result.status;
    if (result.spatial_status == LegacyRoleSpatialRelocationStatus::ready) {
        return true;
    }
    result.status = LegacyWorldStoryPathStatus::spatial_relocation_failed;
    return false;
}

[[nodiscard]] bool reset_selected_motion(
    LegacyWorldStoryPathResult& result,
    const u32 role_index,
    const LegacyWorldRoleRecord& role,
    LegacyWorldStoryPathRuntime& runtime,
    const bool recenter
) noexcept {
    if (role_index != runtime.selected_role_index) {
        return true;
    }
    if (runtime.selected_arrival_bytes.size() <
        kLegacyWorldGuidOneArrivalByteCount) {
        result.status = LegacyWorldStoryPathStatus::runtime_unavailable;
        return false;
    }
    runtime.movement->camera_x_transition = 0;
    runtime.movement->player_x_transition = 0;
    runtime.movement->camera_y_transition = 0;
    runtime.movement->player_y_transition = 0;
    std::ranges::fill(
        runtime.selected_arrival_bytes.first(
            kLegacyWorldGuidOneArrivalByteCount
        ),
        u8{}
    );
    if (recenter && (*runtime.scene_render_flags & 2U) == 0U) {
        recenter_legacy_world_camera(
            role,
            runtime.role_surface.map_width,
            runtime.map_height,
            *runtime.camera
        );
    }
    return true;
}

[[nodiscard]] std::size_t find_role_slot(
    const LegacyWorldStoryPathRuntime& runtime, const u32 role_index
) noexcept {
    for (std::size_t index = 0U; index < kLegacyWorldActiveObjectSlotCount;
         ++index) {
        if (read_u16(runtime.active_object_slots[index], kRoleIndexOffset) ==
            static_cast<u16>(role_index)) {
            return index;
        }
    }
    return kLegacyWorldActiveObjectSlotCount;
}

[[nodiscard]] std::size_t
find_free_slot(const LegacyWorldStoryPathRuntime& runtime) noexcept {
    for (std::size_t index = 0U; index < kLegacyWorldActiveObjectSlotCount;
         ++index) {
        if (read_u16(runtime.active_object_slots[index], kRoleIndexOffset) ==
            kNoRole) {
            return index;
        }
    }
    return kLegacyWorldActiveObjectSlotCount;
}

void update_map_cell(
    LegacyWorldRoleRecord& role, const LegacyWorldStoryPathRuntime& runtime
) noexcept {
    role.map_cell_pointer_32 =
        (role.world_y >> 4U) * runtime.role_surface.map_width +
        (role.world_x >> 4U);
}

[[nodiscard]] bool move_role_directly(
    LegacyWorldStoryPathResult& result,
    const u32 role_index,
    LegacyWorldRoleRecord& role,
    const u16 destination_x,
    const u16 destination_y,
    LegacyWorldStoryPathRuntime& runtime,
    const bool retain_path_ownership
) noexcept {
    if (!clear_surface(result, role, runtime)) {
        return false;
    }
    role.world_x = destination_x;
    role.world_y = destination_y;
    role.path_wait_remaining = 0U;
    update_map_cell(role, runtime);
    if (!mark_surface(result, role, runtime) ||
        !relocate_role(result, role, runtime) ||
        !reset_selected_motion(result, role_index, role, runtime, true)) {
        return false;
    }
    if (retain_path_ownership) {
        role.flags |= kPathOwnershipFlag;
    }
    result.direct_move = true;
    return true;
}

[[nodiscard]] bool align_selected_role(
    LegacyWorldStoryPathResult& result,
    const u32 role_index,
    LegacyWorldRoleRecord& role,
    LegacyWorldStoryPathRuntime& runtime
) noexcept {
    if (role_index != runtime.selected_role_index) {
        return true;
    }
    const i32 first_row = static_cast<i32>(role.world_y >> 4U) - 1;
    const u32 step_x =
        std::bit_cast<u32>(runtime.movement->camera_x_transition) +
        std::bit_cast<u32>(runtime.movement->player_x_transition);
    const u32 step_y =
        std::bit_cast<u32>(runtime.movement->camera_y_transition) +
        std::bit_cast<u32>(runtime.movement->player_y_transition);
    while ((role.world_x & 0x0FU) != 0U) {
        role.world_x -= step_x;
    }
    while ((role.world_y & 0x0FU) != 0U) {
        role.world_y -= step_y;
    }
    if (!reset_selected_motion(result, role_index, role, runtime, false)) {
        return false;
    }
    return relocate_role(result, role, runtime, first_row);
}

[[nodiscard]] bool align_nonselected_role_from_slot(
    LegacyWorldStoryPathResult& result,
    LegacyWorldRoleRecord& role,
    const LegacyWorldObjectSlot& slot,
    LegacyWorldStoryPathRuntime& runtime
) noexcept {
    if (!clear_surface(result, role, runtime)) {
        return false;
    }
    const u16 cursor =
        static_cast<u16>(read_u16(slot, kPathCursorOffset) & kPathCursorMask);
    const std::size_t direction_offset = kPathBytesOffset + cursor;
    if (direction_offset >= slot.bytes.size()) {
        result.status = LegacyWorldStoryPathStatus::path_cursor_out_of_range;
        return false;
    }
    const u8 direction = slot.bytes[direction_offset];
    if (direction >= kSubCellStepX.size()) {
        result.status = LegacyWorldStoryPathStatus::direction_out_of_range;
        return false;
    }
    const i32 first_row = static_cast<i32>(role.world_y >> 4U) - 1;
    while ((role.world_x & 0x0FU) != 0U) {
        role.world_x += std::bit_cast<u32>(-kSubCellStepX[direction]);
    }
    while ((role.world_y & 0x0FU) != 0U) {
        role.world_y += std::bit_cast<u32>(-kSubCellStepY[direction]);
    }
    return relocate_role(result, role, runtime, first_row);
}

[[nodiscard]] u32 clamped_subtract(const u32 value, const u32 amount) noexcept {
    return value < amount ? 0U : value - amount;
}

[[nodiscard]] bool inside_story_view(
    const LegacyWorldRoleRecord& role, const LegacyWorldCameraRect& camera
) noexcept {
    return role.world_x > clamped_subtract(camera.left, 0xA0U) &&
        role.world_x < camera.right + 0xA0U &&
        role.world_y > clamped_subtract(camera.top, 0x50U) &&
        role.world_y < camera.bottom + 0xA0U;
}

[[nodiscard]] bool finish_scheduled_path(
    LegacyWorldStoryPathResult& result,
    const u32 role_index,
    LegacyWorldRoleRecord& role,
    LegacyWorldObjectSlot& slot,
    LegacyWorldStoryPathRuntime& runtime
) noexcept {
    if (!clear_surface(result, role, runtime)) {
        return false;
    }

    u16 cursor = read_u16(slot, kPathCursorOffset);
    u8 direction{};
    while (true) {
        const std::size_t direction_offset = kPathBytesOffset + cursor;
        if (direction_offset >= slot.bytes.size()) {
            result.status =
                LegacyWorldStoryPathStatus::path_cursor_out_of_range;
            return false;
        }
        direction = slot.bytes[direction_offset];
        if (inside_story_view(role, *runtime.camera) || direction == 0xFFU) {
            break;
        }
        if (direction >= kSubCellStepX.size()) {
            result.status = LegacyWorldStoryPathStatus::direction_out_of_range;
            return false;
        }
        ++cursor;
        role.world_x += std::bit_cast<u32>(
            kSubCellStepX[direction] * kOffscreenPathStepScale
        );
        role.world_y += std::bit_cast<u32>(
            kSubCellStepY[direction] * kOffscreenPathStepScale
        );
        ++result.preadvanced_steps;
    }

    if (direction == 0xFFU) {
        const u16 action_id = read_u16(slot, kActionIdOffset);
        const u16 base_variant = read_u16(slot, kBaseVariantOffset);
        const u16 variant_delta = read_u16(slot, kVariantDeltaOffset);
        if (action_id != 0xFFFFU) {
            role.action.action_id = static_cast<u32>(
                static_cast<i32>(std::bit_cast<i16>(action_id))
            );
        }
        if (base_variant != 0xFFFFU) {
            role.action.base_variant = static_cast<u32>(
                static_cast<i32>(std::bit_cast<i16>(base_variant))
            );
        }
        if (variant_delta != 0xFFFFU) {
            role.action.variant_delta = static_cast<u32>(
                static_cast<i32>(std::bit_cast<i16>(variant_delta))
            );
        }
    } else {
        if (direction >= kDirectionToVariant.size()) {
            result.status = LegacyWorldStoryPathStatus::direction_out_of_range;
            return false;
        }
        role.action.variant_delta = kDirectionToVariant[direction];
    }

    write_u16(
        slot, kPathCursorOffset, static_cast<u16>(cursor | kPathCursorFrameGate)
    );
    update_map_cell(role, runtime);
    if (!mark_surface(result, role, runtime) ||
        !relocate_role(result, role, runtime) ||
        !reset_selected_motion(result, role_index, role, runtime, true)) {
        return false;
    }
    role.flags |= kPathOwnershipFlag;
    return true;
}

}  // namespace

LegacyWorldStoryPathResult suspend_legacy_world_story_role(
    LegacyWorldStoryPathRuntime& runtime, const u32 role_index
) noexcept {
    LegacyWorldStoryPathResult result;
    if (runtime.spatial_index == nullptr || runtime.movement == nullptr ||
        runtime.role_surface.map_width == 0U ||
        runtime.role_surface.surface_grid.empty() ||
        runtime.active_object_slots.size() <
            kLegacyWorldActiveObjectSlotCount) {
        result.status = LegacyWorldStoryPathStatus::runtime_unavailable;
        return result;
    }
    if (role_index >= runtime.roles.size()) {
        result.status = LegacyWorldStoryPathStatus::invalid_role_index;
        return result;
    }
    if (runtime.selected_role_index >= runtime.roles.size()) {
        result.status = LegacyWorldStoryPathStatus::invalid_selected_role_index;
        return result;
    }

    auto& role = runtime.roles[role_index];
    if (role_index == runtime.selected_role_index) {
        const i32 first_row = static_cast<i32>(role.world_y >> 4U) - 1;
        const u32 step_x =
            std::bit_cast<u32>(runtime.movement->camera_x_transition) +
            std::bit_cast<u32>(runtime.movement->player_x_transition);
        const u32 step_y =
            std::bit_cast<u32>(runtime.movement->camera_y_transition) +
            std::bit_cast<u32>(runtime.movement->player_y_transition);
        while ((role.world_x & 0x0FU) != 0U) {
            role.world_x -= step_x;
        }
        while ((role.world_y & 0x0FU) != 0U) {
            role.world_y -= step_y;
        }
        runtime.movement->camera_x_transition = 0;
        runtime.movement->player_x_transition = 0;
        runtime.movement->camera_y_transition = 0;
        runtime.movement->player_y_transition = 0;
        if (!relocate_role(result, role, runtime, first_row)) {
            return result;
        }
    }

    const std::size_t slot_index = find_role_slot(runtime, role_index);
    result.slot_index = static_cast<u32>(slot_index);
    result.existing_slot_found =
        slot_index != kLegacyWorldActiveObjectSlotCount;
    if (result.existing_slot_found) {
        auto& slot = runtime.active_object_slots[slot_index];
        if ((slot.bytes[kPathFlagsOffset] & 0x0FU) == 1U) {
            write_u16(
                slot,
                kPathCursorOffset,
                static_cast<u16>(
                    read_u16(slot, kPathCursorOffset) | kPathCursorFrameGate
                )
            );
            write_u16(
                slot, kSavedRoleIndexOffset, read_u16(slot, kRoleIndexOffset)
            );
            write_u16(
                slot,
                kSavedDestinationXOffset,
                read_u16(slot, kDestinationXOffset)
            );
            write_u16(
                slot,
                kSavedDestinationYOffset,
                read_u16(slot, kDestinationYOffset)
            );
        }
        if (((role.world_x | role.world_y) & 0x0FU) != 0U &&
            role_index != runtime.selected_role_index &&
            !align_nonselected_role_from_slot(result, role, slot, runtime)) {
            return result;
        }
    }

    role.flags |= kPathOwnershipFlag;
    result.legacy_return_value = 1;
    return result;
}

LegacyWorldStoryPathResult schedule_legacy_world_story_path(
    LegacyWorldStoryPathRuntime& runtime,
    const LegacyWorldStoryPathRequest& request
) noexcept {
    LegacyWorldStoryPathResult result;
    if (!runtime_is_available(runtime)) {
        result.status = LegacyWorldStoryPathStatus::runtime_unavailable;
        return result;
    }
    if (request.role_index >= runtime.roles.size()) {
        result.status = LegacyWorldStoryPathStatus::invalid_role_index;
        return result;
    }
    if (runtime.selected_role_index >= runtime.roles.size()) {
        result.status = LegacyWorldStoryPathStatus::invalid_selected_role_index;
        return result;
    }

    try {
        LegacyWorldRoleRecord& role = runtime.roles[request.role_index];
        role.flags &= 0xFDFFFFFFU;

        std::size_t slot_index = find_role_slot(runtime, request.role_index);
        result.existing_slot_found =
            slot_index != kLegacyWorldActiveObjectSlotCount;
        if (result.existing_slot_found) {
            LegacyWorldObjectSlot& slot =
                runtime.active_object_slots[slot_index];
            if ((slot.bytes[kPathFlagsOffset] & 0x0FU) == 1U) {
                write_u16(
                    slot, kActionIdOffset, static_cast<u16>(request.action_id)
                );
                write_u16(
                    slot,
                    kBaseVariantOffset,
                    static_cast<u16>(request.base_variant)
                );
                write_u16(
                    slot,
                    kVariantDeltaOffset,
                    static_cast<u16>(request.variant_delta)
                );
                write_u16(
                    slot,
                    kSavedRoleIndexOffset,
                    read_u16(slot, kRoleIndexOffset)
                );
                write_u16(
                    slot,
                    kSavedDestinationXOffset,
                    read_u16(slot, kDestinationXOffset)
                );
                write_u16(
                    slot,
                    kSavedDestinationYOffset,
                    read_u16(slot, kDestinationYOffset)
                );
                if ((request.flags & 0x8000U) != 0U) {
                    slot.bytes[kPathFlagsOffset] &= 0x7FU;
                }
            }
        }

        if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
            if (request.role_index == runtime.selected_role_index) {
                if (!align_selected_role(
                        result, request.role_index, role, runtime
                    )) {
                    return result;
                }
            } else if (
                result.existing_slot_found &&
                !align_nonselected_role_from_slot(
                    result,
                    role,
                    runtime.active_object_slots[slot_index],
                    runtime
                )
            ) {
                return result;
            }
        }

        const u32 path_mode = request.flags & 0x0FU;
        if (path_mode == 0U && !result.existing_slot_found) {
            slot_index = find_free_slot(runtime);
            result.free_slot_allocated =
                slot_index != kLegacyWorldActiveObjectSlotCount;
        }
        result.slot_index = static_cast<u32>(slot_index);

        if (slot_index == kLegacyWorldActiveObjectSlotCount) {
            if (!move_role_directly(
                    result,
                    request.role_index,
                    role,
                    request.destination_x,
                    request.destination_y,
                    runtime,
                    false
                )) {
                return result;
            }
            result.legacy_return_value = 0;
            return result;
        }

        LegacyWorldObjectSlot& slot = runtime.active_object_slots[slot_index];
        write_u16(slot, kActionIdOffset, static_cast<u16>(request.action_id));
        write_u16(
            slot, kBaseVariantOffset, static_cast<u16>(request.base_variant)
        );
        write_u16(
            slot, kVariantDeltaOffset, static_cast<u16>(request.variant_delta)
        );
        write_u16(slot, kRoleIndexOffset, static_cast<u16>(request.role_index));
        write_u16(slot, kPathCursorOffset, 0U);
        write_u16(slot, kDestinationXOffset, request.destination_x);
        write_u16(slot, kDestinationYOffset, request.destination_y);
        slot.bytes[kPathFlagsOffset] =
            static_cast<u8>((slot.bytes[kPathFlagsOffset] & 0xF2U) | 2U);
        if ((request.flags & 0x8000U) != 0U) {
            slot.bytes[kPathFlagsOffset] &= 0x7FU;
        }

        const u32 maximum_x = runtime.role_surface.map_width << 4U;
        const u32 maximum_y = runtime.map_height << 4U;
        if (request.destination_x > maximum_x ||
            request.destination_y > maximum_y || path_mode != 0U) {
            write_u16(slot, kStepXOffset, 0U);
            write_u16(slot, kStepYOffset, 0U);
            if (!move_role_directly(
                    result,
                    request.role_index,
                    role,
                    request.destination_x,
                    request.destination_y,
                    runtime,
                    true
                )) {
                return result;
            }
            result.legacy_return_value = 1;
            return result;
        }

        LegacyWorldPathfinder pathfinder{*runtime.node_pool};
        if ((role.flags & kCollisionBypassFlag) != 0U) {
            pathfinder.set_collision_mask(0U);
        }
        const auto path = pathfinder.find_path({
            .start_x = std::bit_cast<i32>(role.world_x),
            .start_y = std::bit_cast<i32>(role.world_y),
            .target_x = static_cast<i32>(request.destination_x),
            .target_y = static_cast<i32>(request.destination_y),
            .footprint_width = role.action.field_2c,
            .footprint_height = role.action.field_30,
            .map_width = runtime.role_surface.map_width,
            .map_height = runtime.map_height,
            .surface_grid = runtime.role_surface.surface_grid,
        });
        result.pathfinding_status = path.status;
        if (path.status != LegacyWorldPathfindingStatus::completed) {
            result.status = LegacyWorldStoryPathStatus::pathfinding_failed;
            return result;
        }
        if (path.legacy_return_value != 1) {
            write_u16(slot, kStepXOffset, 0U);
            write_u16(slot, kStepYOffset, 0U);
            if (!move_role_directly(
                    result,
                    request.role_index,
                    role,
                    request.destination_x,
                    request.destination_y,
                    runtime,
                    true
                )) {
                return result;
            }
            result.legacy_return_value = 1;
            return result;
        }
        if (!path_fits_slot(path)) {
            result.status = LegacyWorldStoryPathStatus::path_does_not_fit_slot;
            return result;
        }
        copy_path(path, slot);
        result.path_found = true;
        if (!finish_scheduled_path(
                result, request.role_index, role, slot, runtime
            )) {
            return result;
        }
        result.legacy_return_value = 1;
        return result;
    } catch (const std::bad_alloc&) {
        result.status = LegacyWorldStoryPathStatus::allocation_failed;
        return result;
    }
}

LegacyWorldStoryPathResult query_legacy_world_story_path(
    LegacyWorldStoryPathRuntime& runtime, const u32 role_index
) noexcept {
    LegacyWorldStoryPathResult result;
    if (!runtime_is_available(runtime)) {
        result.status = LegacyWorldStoryPathStatus::runtime_unavailable;
        return result;
    }
    if (role_index >= runtime.roles.size()) {
        result.status = LegacyWorldStoryPathStatus::invalid_role_index;
        return result;
    }

    std::size_t slot_index = 0U;
    for (; slot_index < kLegacyWorldActiveObjectSlotCount; ++slot_index) {
        const auto& slot = runtime.active_object_slots[slot_index];
        if (read_u16(slot, kRoleIndexOffset) == static_cast<u16>(role_index) &&
            (slot.bytes[kPathFlagsOffset] & 0x0FU) == 2U) {
            break;
        }
    }
    result.slot_index = static_cast<u32>(slot_index);
    if (slot_index == kLegacyWorldActiveObjectSlotCount) {
        result.legacy_return_value = 0;
        return result;
    }

    LegacyWorldObjectSlot& slot = runtime.active_object_slots[slot_index];
    LegacyWorldRoleRecord& role = runtime.roles[role_index];
    const u16 cursor =
        static_cast<u16>(read_u16(slot, kPathCursorOffset) & kPathCursorMask);
    const std::size_t direction_offset = kPathBytesOffset + cursor;
    if (direction_offset >= slot.bytes.size()) {
        result.status = LegacyWorldStoryPathStatus::path_cursor_out_of_range;
        return result;
    }
    const u8 direction = slot.bytes[direction_offset];
    if (direction == 0xFFU) {
        role.path_wait_remaining = 0U;
        role.flags &= kArrivalClearMask;
        if (!reset_selected_motion(result, role_index, role, runtime, true)) {
            return result;
        }
        result.legacy_return_value = 2;
        return result;
    }
    if (direction >= kSubCellStepX.size()) {
        result.status = LegacyWorldStoryPathStatus::direction_out_of_range;
        return result;
    }

    u8 occupancy_mask{};
    if ((role.flags & kCollisionBypassFlag) == 0U) {
        const auto occupancy = compute_legacy_world_directional_occupancy_mask(
            runtime.role_surface.surface_grid,
            runtime.role_surface.map_width,
            runtime.map_height,
            role.map_cell_pointer_32,
            role.action.field_2c,
            role.action.field_30,
            kStoryCollisionMask
        );
        result.directional_probe_status = occupancy.status;
        if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
            result.status =
                LegacyWorldStoryPathStatus::directional_probe_failed;
            return result;
        }
        occupancy_mask = occupancy.mask;
    }

    write_u16(
        slot, kPathCursorOffset, static_cast<u16>(cursor | kPathCursorFrameGate)
    );
    ++slot.bytes[kPathStallOffset];
    if ((occupancy_mask & kDirectionCollisionBits[direction]) == 0U) {
        write_u16(slot, kPathCursorOffset, cursor);
        slot.bytes[kPathStallOffset] = 0U;
        write_u16(
            slot,
            kStepXOffset,
            std::bit_cast<u16>(static_cast<i16>(kSubCellStepX[direction]))
        );
        write_u16(
            slot,
            kStepYOffset,
            std::bit_cast<u16>(static_cast<i16>(kSubCellStepY[direction]))
        );
    }
    role.flags |= kPreparedMovementFlag;
    result.legacy_return_value = 1;
    return result;
}

LegacyWorldStoryPathResult complete_legacy_world_story_path(
    LegacyWorldStoryPathRuntime& runtime, const u32 role_index
) noexcept {
    LegacyWorldStoryPathResult result;
    if (role_index >= runtime.roles.size()) {
        result.status = LegacyWorldStoryPathStatus::invalid_role_index;
        return result;
    }
    LegacyWorldRoleRecord& role = runtime.roles[role_index];
    if ((role.flags & kPathOwnershipFlag) == 0U) {
        result.legacy_return_value = 1;
        return result;
    }
    if (runtime.active_object_slots.size() <
        kLegacyWorldActiveObjectSlotCount) {
        result.status = LegacyWorldStoryPathStatus::runtime_unavailable;
        return result;
    }

    std::size_t slot_index = 0U;
    for (; slot_index < kLegacyWorldActiveObjectSlotCount; ++slot_index) {
        const auto& slot = runtime.active_object_slots[slot_index];
        if (read_u16(slot, kRoleIndexOffset) == static_cast<u16>(role_index) &&
            (slot.bytes[kPathFlagsOffset] & 0x0FU) > 1U) {
            break;
        }
    }
    result.slot_index = static_cast<u32>(slot_index);
    if (slot_index == kLegacyWorldActiveObjectSlotCount) {
        result.legacy_return_value = 0;
        return result;
    }

    LegacyWorldObjectSlot& slot = runtime.active_object_slots[slot_index];
    const u16 saved_role_index = read_u16(slot, kSavedRoleIndexOffset);
    if (saved_role_index == kNoRole) {
        if ((slot.bytes[kPathFlagsOffset] & 0x0FU) != 1U) {
            static_cast<void>(reset_legacy_world_object_slot(slot));
            result.slot_cleared = true;
        }
        result.legacy_return_value = 1;
        return result;
    }

    if (runtime.node_pool == nullptr || runtime.map_height == 0U ||
        runtime.role_surface.map_width == 0U ||
        runtime.role_surface.surface_grid.empty()) {
        result.status = LegacyWorldStoryPathStatus::runtime_unavailable;
        return result;
    }

    try {
        const u16 destination_x = read_u16(slot, kSavedDestinationXOffset);
        const u16 destination_y = read_u16(slot, kSavedDestinationYOffset);
        write_u16(slot, kRoleIndexOffset, saved_role_index);
        write_u16(slot, kPathCursorOffset, 0U);
        write_u16(slot, kDestinationXOffset, destination_x);
        write_u16(slot, kDestinationYOffset, destination_y);
        slot.bytes[kPathFlagsOffset] =
            static_cast<u8>((slot.bytes[kPathFlagsOffset] & 0xF1U) | 1U);

        LegacyWorldPathfinder pathfinder{*runtime.node_pool};
        const auto path = pathfinder.find_path({
            .start_x = std::bit_cast<i32>(role.world_x),
            .start_y = std::bit_cast<i32>(role.world_y),
            .target_x = destination_x,
            .target_y = destination_y,
            .footprint_width = role.action.field_2c,
            .footprint_height = role.action.field_30,
            .map_width = runtime.role_surface.map_width,
            .map_height = runtime.map_height,
            .surface_grid = runtime.role_surface.surface_grid,
        });
        result.pathfinding_status = path.status;
        if (path.status != LegacyWorldPathfindingStatus::completed) {
            result.status = LegacyWorldStoryPathStatus::pathfinding_failed;
            return result;
        }
        if (path.legacy_return_value == 1) {
            if (!path_fits_slot(path)) {
                result.status =
                    LegacyWorldStoryPathStatus::path_does_not_fit_slot;
                return result;
            }
            copy_path(path, slot);
            result.path_found = true;
        }
        result.legacy_return_value = 1;
        return result;
    } catch (const std::bad_alloc&) {
        result.status = LegacyWorldStoryPathStatus::allocation_failed;
        return result;
    }
}

}  // namespace openswd3::world_map
