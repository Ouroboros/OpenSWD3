#include "openswd3/world_map/legacy_world_path_script.hpp"

#include "openswd3/world_map/legacy_world_story_vm.hpp"
#include "openswd3/input_time_rng/legacy_crt_rng.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_role_lookup.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kPathDatabasePayloadOffset = 0x200U;
constexpr std::size_t kRoleIndexOffset = 0x00U;
constexpr std::size_t kPathCursorOffset = 0x02U;
constexpr std::size_t kStepXOffset = 0x16U;
constexpr std::size_t kStepYOffset = 0x18U;
constexpr std::size_t kPathStallOffset = 0x1AU;
constexpr std::size_t kPathFlagsOffset = 0x1BU;
constexpr std::size_t kPathBytesOffset = 0x1CU;

constexpr u16 kPathCursorMask = 0x7FFFU;
constexpr u16 kPathCursorFrameGate = 0x8000U;
constexpr u32 kPathRoleFlag = 0x00008000U;
constexpr u32 kPartyRoleFlag = 0x00000080U;
constexpr u32 kPathCompletionStepFlag = 0x04000000U;
constexpr u32 kInteractionSuspendedFlag = 0x80000000U;

constexpr std::array<u32, 8U> kDirectionCollisionBits{
    0x10U, 0x20U, 0x40U, 0x80U, 0x01U, 0x02U, 0x04U, 0x08U
};
constexpr std::array<i16, 8U> kSubCellStepX{4, 0, -4, -4, -4, 0, 4, 4};
constexpr std::array<i16, 8U> kSubCellStepY{4, 4, 4, 0, -4, -4, -4, 0};

struct ResolvedRoleSelector {
    u32 index{};
    bool found{};
};

[[nodiscard]] ResolvedRoleSelector resolve_role_selector(
    const std::span<const LegacyWorldRoleRecord> roles,
    const u16 selector,
    const u32 controlled_role_index
) noexcept {
    u32 index{};
    const bool found = resolve_legacy_world_role_selector(
        roles, selector, controlled_role_index, index
    );
    return {index, found};
}

[[nodiscard]] compat::i32 signed_field(const u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] long double
radial_distance(const LegacyWorldRoleRecord& role) noexcept {
    const u32 x_square = role.world_x * role.world_x;
    const u32 y_square = role.world_y * role.world_y;
    const u32 sum = x_square + y_square;
    return std::sqrt(static_cast<long double>(sum));
}

[[nodiscard]] u32 radial_distance_difference(
    const LegacyWorldRoleRecord& current, const LegacyWorldRoleRecord& target
) noexcept {
    const auto truncated_target =
        static_cast<std::int64_t>(radial_distance(target));
    const auto difference = static_cast<std::int64_t>(
        radial_distance(current) - static_cast<long double>(truncated_target)
    );
    return static_cast<u32>(difference < 0 ? -difference : difference);
}

[[nodiscard]] bool range_available(
    const std::span<const u8> bytes,
    const std::size_t offset,
    const std::size_t size
) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] u16 read_u16_le(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_u32_le(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] u16 read_slot_u16(
    const LegacyWorldObjectSlot& slot, const std::size_t offset
) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_slot_u16(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

struct PathCommandView {
    LegacyWorldPathScriptStatus status{LegacyWorldPathScriptStatus::completed};
    std::size_t offset{};
};

[[nodiscard]] PathCommandView resolve_path_command(
    const std::span<const u8> path_database, const LegacyWorldRoleRecord& role
) noexcept {
    const std::size_t directory_offset = kPathDatabasePayloadOffset +
        static_cast<std::size_t>(role.path_data_id) * sizeof(u32);
    if (!range_available(path_database, directory_offset, sizeof(u32))) {
        return {
            LegacyWorldPathScriptStatus::path_directory_entry_out_of_range, 0U
        };
    }

    const u32 relative = read_u32_le(path_database, directory_offset);
    const std::int64_t command_offset =
        static_cast<std::int64_t>(kPathDatabasePayloadOffset) + relative +
        static_cast<std::int64_t>(
            std::bit_cast<compat::i32>(role.path_word_index)
        ) * static_cast<std::int64_t>(sizeof(u16));
    if (command_offset < 0 ||
        !range_available(
            path_database, static_cast<std::size_t>(command_offset), sizeof(u16)
        )) {
        return {LegacyWorldPathScriptStatus::path_command_out_of_range, 0U};
    }
    return {
        LegacyWorldPathScriptStatus::completed,
        static_cast<std::size_t>(command_offset)
    };
}

[[nodiscard]] bool transfer_path_cursor(
    const std::span<const u8> path_database,
    LegacyWorldRoleRecord& role,
    const u32 target_relative_offset
) noexcept {
    const std::size_t directory_offset = kPathDatabasePayloadOffset +
        static_cast<std::size_t>(role.path_data_id) * sizeof(u32);
    if (!range_available(path_database, directory_offset, sizeof(u32))) {
        return false;
    }

    const u32 script_relative_offset =
        read_u32_le(path_database, directory_offset);
    const u32 byte_difference = target_relative_offset - script_relative_offset;
    role.path_word_index =
        std::bit_cast<u32>(std::bit_cast<compat::i32>(byte_difference) >> 1U);
    const std::size_t target = kPathDatabasePayloadOffset +
        static_cast<std::size_t>(target_relative_offset);
    return range_available(path_database, target, sizeof(u16));
}

void advance_cursor(
    LegacyWorldRoleRecord& role,
    LegacyWorldPathScriptResult& result,
    const u32 word_count
) noexcept {
    role.path_word_index += word_count;
    result.cursor_words_advanced += word_count;
}

void record_action_update(
    LegacyWorldRoleRecord& role,
    LegacyWorldPathScriptResult& result,
    LegacyWorldPathScriptPorts& ports
) {
    ++result.action_updates;
    if (ports.update_action(role.action) == 0U) {
        ++result.action_update_failures;
    }
}

enum class PathMovementStatus : u8 {
    active,
    yielded,
    completed,
    no_slot,
    insufficient_slots,
    directional_probe_failed,
    direction_out_of_range,
};

struct PathMovementResult {
    PathMovementStatus status{PathMovementStatus::yielded};
    LegacyWorldDirectionProbeStatus directional_probe_status{
        LegacyWorldDirectionProbeStatus::completed
    };
};

[[nodiscard]] PathMovementResult prepare_role_path_movement(
    const u32 role_index,
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots
) noexcept {
    if (object_slots.size() < kLegacyWorldActiveObjectSlotCount) {
        return {PathMovementStatus::insufficient_slots};
    }

    for (LegacyWorldObjectSlot& slot :
         object_slots.first(kLegacyWorldActiveObjectSlotCount)) {
        if (read_slot_u16(slot, kRoleIndexOffset) !=
            static_cast<u16>(role_index)) {
            continue;
        }

        const u8 slot_kind =
            static_cast<u8>(slot.bytes[kPathFlagsOffset] & 0x0FU);
        if (slot_kind == 2U) {
            return {PathMovementStatus::yielded};
        }
        if (slot_kind != 1U) {
            continue;
        }
        if (role.interaction_gate == 1U) {
            write_slot_u16(
                slot,
                kPathCursorOffset,
                static_cast<u16>(
                    read_slot_u16(slot, kPathCursorOffset) |
                    kPathCursorFrameGate
                )
            );
            return {PathMovementStatus::yielded};
        }

        const u16 cursor = static_cast<u16>(
            read_slot_u16(slot, kPathCursorOffset) & kPathCursorMask
        );
        const std::size_t direction_offset = kPathBytesOffset + cursor;
        if (direction_offset >= slot.bytes.size()) {
            return {PathMovementStatus::direction_out_of_range};
        }
        const u8 direction = slot.bytes[direction_offset];
        if (direction == 0xFFU) {
            slot.bytes.fill(0xFFU);
            return {PathMovementStatus::completed};
        }
        if (direction >= kDirectionCollisionBits.size()) {
            return {PathMovementStatus::direction_out_of_range};
        }

        const u32 collision_mask =
            slot.bytes[kPathStallOffset] <= 8U ? 0x60000000U : 0x40000000U;
        const auto occupancy = compute_legacy_world_directional_occupancy_mask(
            surface_context.surface_grid,
            surface_context.map_width,
            map_height,
            role.map_cell_pointer_32,
            role.action.field_2c,
            role.action.field_30,
            collision_mask
        );
        if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
            return {
                PathMovementStatus::directional_probe_failed, occupancy.status
            };
        }

        write_slot_u16(
            slot,
            kPathCursorOffset,
            static_cast<u16>(cursor | kPathCursorFrameGate)
        );
        ++slot.bytes[kPathStallOffset];
        write_slot_u16(slot, kStepXOffset, 0U);
        write_slot_u16(slot, kStepYOffset, 0U);
        if ((occupancy.mask & kDirectionCollisionBits[direction]) != 0U) {
            return {PathMovementStatus::active};
        }

        write_slot_u16(slot, kPathCursorOffset, cursor);
        slot.bytes[kPathStallOffset] = 0U;
        write_slot_u16(
            slot, kStepXOffset, std::bit_cast<u16>(kSubCellStepX[direction])
        );
        write_slot_u16(
            slot, kStepYOffset, std::bit_cast<u16>(kSubCellStepY[direction])
        );
        return {PathMovementStatus::active};
    }
    return {PathMovementStatus::no_slot};
}

[[nodiscard]] LegacyWorldObjectSlot* find_role_slot(
    const u32 role_index, const std::span<LegacyWorldObjectSlot> object_slots
) noexcept {
    for (LegacyWorldObjectSlot& slot : object_slots.first(
             std::min(object_slots.size(), kLegacyWorldActiveObjectSlotCount)
         )) {
        if (read_slot_u16(slot, kRoleIndexOffset) ==
            static_cast<u16>(role_index)) {
            return &slot;
        }
    }
    return nullptr;
}

[[nodiscard]] bool initialize_random_walk_slot(
    const u32 role_index,
    LegacyWorldRoleRecord& role,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots,
    const LegacyWorldPathScriptRuntime& runtime,
    LegacyWorldPathScriptResult& result
) noexcept {
    if (object_slots.size() < kLegacyWorldActiveObjectSlotCount) {
        result.status = LegacyWorldPathScriptStatus::insufficient_object_slots;
        return false;
    }
    if (runtime.crt_rng == nullptr || runtime.secondary_rng == nullptr) {
        result.status = LegacyWorldPathScriptStatus::runtime_unavailable;
        return false;
    }

    static_cast<void>(runtime.crt_rng->next());
    const u32 random_value = runtime.secondary_rng->next_bounded(80U);
    LegacyWorldObjectSlot* selected = nullptr;
    for (LegacyWorldObjectSlot& slot :
         object_slots.first(kLegacyWorldActiveObjectSlotCount)) {
        if (read_slot_u16(slot, kRoleIndexOffset) !=
            static_cast<u16>(role_index)) {
            continue;
        }
        const u8 kind = static_cast<u8>(slot.bytes[kPathFlagsOffset] & 0x0FU);
        if (kind == 2U) {
            return true;
        }
        if (kind == 1U) {
            selected = &slot;
        }
    }

    if (selected == nullptr) {
        for (LegacyWorldObjectSlot& slot :
             object_slots.first(kLegacyWorldActiveObjectSlotCount)) {
            if (read_slot_u16(slot, kRoleIndexOffset) != 0xFFFFU) {
                continue;
            }
            selected = &slot;
            write_slot_u16(
                slot, kRoleIndexOffset, static_cast<u16>(role_index)
            );
            write_slot_u16(slot, kPathCursorOffset, 0U);
            write_slot_u16(slot, 0x04U, 0U);
            write_slot_u16(slot, 0x06U, 0U);
            slot.bytes[kPathFlagsOffset] =
                static_cast<u8>((slot.bytes[kPathFlagsOffset] & 0xF1U) | 1U);
            slot.bytes[kPathBytesOffset] = 1U;
            break;
        }
    }
    if (selected == nullptr) {
        result.status = LegacyWorldPathScriptStatus::insufficient_object_slots;
        return false;
    }

    LegacyWorldObjectSlot& slot = *selected;
    if (role.interaction_gate == 1U || random_value > 60U) {
        slot.bytes[3U] |= 0x80U;
        role.action.base_variant = 0U;
        return true;
    }

    role.action.base_variant = 8U;
    if (random_value < 8U) {
        slot.bytes[kPathBytesOffset] = static_cast<u8>(random_value);
    }
    const u8 direction = slot.bytes[kPathBytesOffset];
    if (direction == 0xFFU) {
        return true;
    }
    if (direction >= kDirectionCollisionBits.size()) {
        result.status = LegacyWorldPathScriptStatus::direction_out_of_range;
        return false;
    }

    const u32 collision_mask =
        slot.bytes[kPathStallOffset] <= 8U ? 0x60000000U : 0x40000000U;
    const auto occupancy = compute_legacy_world_directional_occupancy_mask(
        surface_context.surface_grid,
        surface_context.map_width,
        map_height,
        role.map_cell_pointer_32,
        role.action.field_2c,
        role.action.field_30,
        collision_mask
    );
    result.directional_probe_status = occupancy.status;
    if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
        result.status = LegacyWorldPathScriptStatus::directional_probe_failed;
        return false;
    }

    ++slot.bytes[kPathStallOffset];
    write_slot_u16(slot, kPathCursorOffset, kPathCursorFrameGate);
    write_slot_u16(slot, kStepXOffset, 0U);
    write_slot_u16(slot, kStepYOffset, 0U);
    if ((occupancy.mask & kDirectionCollisionBits[direction]) != 0U) {
        return true;
    }
    write_slot_u16(slot, kPathCursorOffset, 0U);
    slot.bytes[kPathStallOffset] = 0U;
    write_slot_u16(
        slot, kStepXOffset, std::bit_cast<u16>(kSubCellStepX[direction])
    );
    write_slot_u16(
        slot, kStepYOffset, std::bit_cast<u16>(kSubCellStepY[direction])
    );
    return true;
}

}  // namespace

LegacyWorldPathScriptResult run_legacy_world_path_script(
    const u32 role_index,
    const std::span<const u8> path_database,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool& node_pool,
    LegacyWorldPathScriptState& state,
    const LegacyWorldPathScriptRuntime& runtime,
    LegacyWorldPathScriptPorts& ports
) {
    LegacyWorldPathScriptResult result;
    if (role_index >= roles.size()) {
        result.status = LegacyWorldPathScriptStatus::invalid_role_index;
        return result;
    }

    LegacyWorldRoleRecord& role = roles[role_index];
    compat::i32 legacy_transformed_x{};
    compat::i32 legacy_transformed_y{};
    bool legacy_transformed_offset_initialized{};
    for (;;) {
        const PathCommandView command =
            resolve_path_command(path_database, role);
        if (command.status != LegacyWorldPathScriptStatus::completed) {
            result.status = command.status;
            return result;
        }

        const u16 opcode = read_u16_le(path_database, command.offset);
        result.last_opcode = opcode;
        ++result.opcodes_dispatched;
        switch (opcode) {
        case 0U:
            role.path_word_index = 0U;
            return result;

        case 1U:
            role.path_data_id = 0U;
            role.path_word_index = 0U;
            return result;

        case 2U:
        case 3U:
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (opcode == 2U) {
                role.action.base_variant =
                    read_u16_le(path_database, command.offset + sizeof(u16));
            } else {
                role.action.variant_delta =
                    read_u16_le(path_database, command.offset + sizeof(u16));
            }
            advance_cursor(role, result, 2U);
            record_action_update(role, result, ports);
            return result;

        case 4U:
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            role.path_wait_remaining =
                read_u16_le(path_database, command.offset + sizeof(u16));
            advance_cursor(role, result, 2U);
            ++result.waits_set;
            break;

        case 5U:
            if (role.path_wait_remaining > 0U) {
                --role.path_wait_remaining;
                ++result.waits_decremented;
            } else {
                advance_cursor(role, result, 1U);
            }
            return result;

        case 6U:
            if (role.path_wait_remaining > 0U) {
                --role.path_wait_remaining;
                ++result.waits_decremented;
            } else {
                advance_cursor(role, result, 1U);
            }
            record_action_update(role, result, ports);
            return result;

        case 7U: {
            if (!range_available(
                    path_database, command.offset, 3U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const auto request = request_legacy_world_role_path(
                role_index,
                path_database.subspan(command.offset, 3U * sizeof(u16)),
                roles,
                surface_context,
                map_height,
                object_slots,
                node_pool
            );
            ++result.path_requests;
            result.path_request_status = request.status;
            result.pathfinding_status = request.pathfinding_status;
            advance_cursor(role, result, 3U);
            break;
        }

        case 8U: {
            if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
                return result;
            }
            const auto movement = prepare_role_path_movement(
                role_index, role, surface_context, map_height, object_slots
            );
            result.directional_probe_status = movement.directional_probe_status;
            if (movement.status == PathMovementStatus::insufficient_slots) {
                result.status =
                    LegacyWorldPathScriptStatus::insufficient_object_slots;
                return result;
            }
            if (movement.status ==
                PathMovementStatus::directional_probe_failed) {
                result.status =
                    LegacyWorldPathScriptStatus::directional_probe_failed;
                return result;
            }
            if (movement.status == PathMovementStatus::direction_out_of_range) {
                result.status =
                    LegacyWorldPathScriptStatus::direction_out_of_range;
                return result;
            }
            if (movement.status == PathMovementStatus::active) {
                ++result.movement_slots_advanced;
                return result;
            }
            if (movement.status == PathMovementStatus::completed ||
                movement.status == PathMovementStatus::no_slot) {
                ++result.movement_slots_completed;
                advance_cursor(role, result, 1U);
                role.flags &= ~kPathCompletionStepFlag;
                break;
            }
            return result;
        }

        case 9U: {
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            const bool elapsed =
                read_u16_le(path_database, command.offset + sizeof(u16)) <=
                runtime.shared_script_state->script_clock;
            if (elapsed) {
                advance_cursor(role, result, 2U);
            }
            if ((role.flags & 0x00001000U) != 0U) {
                record_action_update(role, result, ports);
            }
            if (!elapsed) {
                return result;
            }
            break;
        }

        case 10U: {
            if (!range_available(
                    path_database, command.offset, 4U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            const u16 threshold =
                read_u16_le(path_database, command.offset + sizeof(u16));
            const u16 clock =
                static_cast<u16>(runtime.shared_script_state->script_clock);
            if (threshold <= clock) {
                if (!transfer_path_cursor(
                        path_database,
                        role,
                        read_u32_le(
                            path_database, command.offset + 2U * sizeof(u16)
                        )
                    )) {
                    result.status =
                        LegacyWorldPathScriptStatus::path_command_out_of_range;
                    return result;
                }
                ++result.conditional_transfers;
            } else {
                advance_cursor(role, result, 4U);
            }
            break;
        }

        case 11U:
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            runtime.shared_script_state->script_clock =
                read_u16_le(path_database, command.offset + sizeof(u16));
            if (runtime.shared_script_state->script_clock > 1000U) {
                runtime.shared_script_state->script_clock = 0U;
            }
            // The original advances by one word, deliberately reinterpreting the
            // operand word as the next opcode.
            advance_cursor(role, result, 1U);
            break;

        case 12U:
            if (!range_available(
                    path_database, command.offset, 3U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (!transfer_path_cursor(
                    path_database,
                    role,
                    read_u32_le(path_database, command.offset + sizeof(u16))
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_out_of_range;
                return result;
            }
            ++result.conditional_transfers;
            break;

        case 13U:
        case 14U: {
            if (!range_available(
                    path_database, command.offset, 4U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            const bool flag = query_legacy_world_story_flag(
                *runtime.shared_script_state,
                read_u16_le(path_database, command.offset + sizeof(u16))
            );
            const bool take = opcode == 13U ? flag : !flag;
            if (take) {
                if (!transfer_path_cursor(
                        path_database,
                        role,
                        read_u32_le(
                            path_database, command.offset + 2U * sizeof(u16)
                        )
                    )) {
                    result.status =
                        LegacyWorldPathScriptStatus::path_command_out_of_range;
                    return result;
                }
                ++result.conditional_transfers;
            } else {
                advance_cursor(role, result, 4U);
            }
            break;
        }

        case 15U:
        case 16U: {
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            std::size_t flag_offset = command.offset + sizeof(u16);
            u32 flag_count = 0U;
            u32 set_count = 0U;
            while (true) {
                if (!range_available(path_database, flag_offset, sizeof(u16))) {
                    result.status =
                        LegacyWorldPathScriptStatus::path_command_truncated;
                    return result;
                }
                const u16 flag = read_u16_le(path_database, flag_offset);
                if (flag == 0xFFFFU) {
                    break;
                }
                if (query_legacy_world_story_flag(
                        *runtime.shared_script_state, flag
                    )) {
                    ++set_count;
                }
                ++flag_count;
                flag_offset += sizeof(u16);
            }
            if (!range_available(
                    path_database, flag_offset + sizeof(u16), sizeof(u32)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const bool take =
                opcode == 15U ? set_count == flag_count : set_count >= 1U;
            if (take) {
                if (!transfer_path_cursor(
                        path_database,
                        role,
                        read_u32_le(path_database, flag_offset + sizeof(u16))
                    )) {
                    result.status =
                        LegacyWorldPathScriptStatus::path_command_out_of_range;
                    return result;
                }
                ++result.conditional_transfers;
            } else {
                advance_cursor(role, result, flag_count + 4U);
            }
            break;
        }

        case 17U:
        case 18U:
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            if (opcode == 17U) {
                set_legacy_world_story_flag(
                    *runtime.shared_script_state,
                    read_u16_le(path_database, command.offset + sizeof(u16))
                );
            } else {
                clear_legacy_world_story_flag(
                    *runtime.shared_script_state,
                    read_u16_le(path_database, command.offset + sizeof(u16))
                );
            }
            advance_cursor(role, result, 2U);
            break;

        case 19U:
        case 20U:
        case 21U:
        case 22U:
        case 23U: {
            if (!range_available(
                    path_database, command.offset, 3U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            const u16 index =
                read_u16_le(path_database, command.offset + sizeof(u16));
            const u16 value =
                read_u16_le(path_database, command.offset + 2U * sizeof(u16));
            if (index >= runtime.shared_script_state->script_variables.size()) {
                ++result.invalid_variable_indices;
                return result;
            }
            u32& variable =
                runtime.shared_script_state->script_variables[index];
            if (opcode == 19U) {
                variable = value;
                advance_cursor(role, result, 3U);
                break;
            }
            if (opcode == 20U) {
                variable += value;
                if (variable > 1000U) {
                    variable = 1000U;
                }
                advance_cursor(role, result, 3U);
                break;
            }
            if (opcode == 21U) {
                variable -= value;
                if ((variable & 0x80000000U) != 0U) {
                    variable = 0U;
                }
                advance_cursor(role, result, 3U);
                break;
            }
            if (!range_available(
                    path_database, command.offset, 5U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const bool take =
                opcode == 22U ? variable >= value : variable <= value;
            if (take) {
                if (!transfer_path_cursor(
                        path_database,
                        role,
                        read_u32_le(
                            path_database, command.offset + 3U * sizeof(u16)
                        )
                    )) {
                    result.status =
                        LegacyWorldPathScriptStatus::path_command_out_of_range;
                    return result;
                }
                ++result.conditional_transfers;
            } else {
                advance_cursor(role, result, 5U);
            }
            break;
        }

        case 24U: {
            if (!range_available(
                    path_database, command.offset, 3U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const ResolvedRoleSelector target_selector = resolve_role_selector(
                roles,
                read_u16_le(path_database, command.offset + sizeof(u16)),
                runtime.controlled_role_index
            );
            const u32 target_index = target_selector.index;
            if (target_index >= roles.size()) {
                result.status = LegacyWorldPathScriptStatus::invalid_role_index;
                return result;
            }
            LegacyWorldRoleRecord& target = roles[target_index];
            if (LegacyWorldObjectSlot* slot =
                    find_role_slot(target_index, object_slots);
                slot != nullptr) {
                const u8 kind =
                    static_cast<u8>(slot->bytes[kPathFlagsOffset] & 0x0FU);
                if (kind == 2U) {
                    for (std::size_t offset = 0x08U; offset <= 0x0EU;
                         offset += sizeof(u16)) {
                        write_slot_u16(*slot, offset, 0xFFFFU);
                    }
                } else if (kind == 1U) {
                    if (((target.world_x | target.world_y) & 0x0FU) != 0U) {
                        const compat::i32 original_row =
                            static_cast<compat::i32>(target.world_y >> 4U);
                        static_cast<void>(
                            clear_legacy_world_role_surface_occupancy(
                                target, surface_context
                            )
                        );
                        const u16 cursor = static_cast<u16>(
                            read_slot_u16(*slot, kPathCursorOffset) &
                            kPathCursorMask
                        );
                        const std::size_t direction_offset =
                            kPathBytesOffset + cursor;
                        if (direction_offset >= slot->bytes.size()) {
                            result.status = LegacyWorldPathScriptStatus::
                                direction_out_of_range;
                            return result;
                        }
                        const u8 direction = slot->bytes[direction_offset];
                        if (direction >= kSubCellStepX.size()) {
                            result.status = LegacyWorldPathScriptStatus::
                                direction_out_of_range;
                            return result;
                        }
                        while ((target.world_x & 0x0FU) != 0U) {
                            target.world_x -=
                                std::bit_cast<u32>(static_cast<compat::i32>(
                                    kSubCellStepX[direction]
                                ));
                        }
                        while ((target.world_y & 0x0FU) != 0U) {
                            target.world_y -=
                                std::bit_cast<u32>(static_cast<compat::i32>(
                                    kSubCellStepY[direction]
                                ));
                        }
                        if (runtime.spatial_index != nullptr) {
                            static_cast<void>(
                                relocate_legacy_role_spatially_by_guid(
                                    *runtime.spatial_index,
                                    roles,
                                    target.guid,
                                    target.flags & 3U,
                                    original_row,
                                    true
                                )
                            );
                        }
                    }
                    slot->bytes.fill(0xFFU);
                }
            }

            advance_cursor(role, result, 3U);
            target.path_data_id =
                read_u16_le(path_database, command.offset + 2U * sizeof(u16));
            target.path_word_index = 0U;
            if (target_index == role_index) {
                // The next loop iteration resolves the newly selected script.
                continue;
            }
            break;
        }

        case 25U:
            if (!range_available(
                    path_database, command.offset, 4U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            if (runtime.shared_script_state->script_clock >
                runtime.shared_script_state->script_clock_origin +
                    read_u16_le(path_database, command.offset + sizeof(u16))) {
                if (!transfer_path_cursor(
                        path_database,
                        role,
                        read_u32_le(
                            path_database, command.offset + 2U * sizeof(u16)
                        )
                    )) {
                    result.status =
                        LegacyWorldPathScriptStatus::path_command_out_of_range;
                    return result;
                }
                ++result.conditional_transfers;
            } else {
                advance_cursor(role, result, 4U);
            }
            break;

        case 26U:
            if (runtime.shared_script_state == nullptr) {
                result.status =
                    LegacyWorldPathScriptStatus::runtime_unavailable;
                return result;
            }
            runtime.shared_script_state->script_clock_origin =
                runtime.shared_script_state->script_clock;
            advance_cursor(role, result, 1U);
            break;

        case 27U:
        case 28U: {
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const ResolvedRoleSelector target_selector = resolve_role_selector(
                roles,
                read_u16_le(path_database, command.offset + sizeof(u16)),
                runtime.controlled_role_index
            );
            const u32 target_index = target_selector.index;
            if (target_index >= roles.size()) {
                result.status = LegacyWorldPathScriptStatus::invalid_role_index;
                return result;
            }
            LegacyWorldRoleRecord& target = roles[target_index];
            if (opcode == 27U) {
                target.flags &= 0xFFFF3FFFU;
                static_cast<void>(clear_legacy_world_role_surface_occupancy(
                    target, surface_context
                ));
            } else {
                target.flags |= 0x0000C000U;
            }
            advance_cursor(role, result, 2U);
            break;
        }

        case 29U: {
            if (!range_available(
                    path_database, command.offset, 3U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            static_cast<void>(
                clear_legacy_world_role_surface_occupancy(role, surface_context)
            );
            role.world_x =
                static_cast<u32>(
                    read_u16_le(path_database, command.offset + sizeof(u16))
                )
                << 4U;
            role.world_y = static_cast<u32>(read_u16_le(
                               path_database, command.offset + 2U * sizeof(u16)
                           ))
                << 4U;
            role.map_cell_pointer_32 =
                (role.world_y >> 4U) * surface_context.map_width +
                (role.world_x >> 4U);
            record_action_update(role, result, ports);
            static_cast<void>(
                mark_legacy_world_role_surface_occupancy(role, surface_context)
            );
            advance_cursor(role, result, 3U);
            if (runtime.spatial_index != nullptr) {
                static_cast<void>(relocate_legacy_role_spatially_by_guid(
                    *runtime.spatial_index,
                    roles,
                    role.guid,
                    role.flags & 3U,
                    0,
                    true
                ));
            }
            return result;
        }

        case 30U:
            if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
                return result;
            }
            if (!initialize_random_walk_slot(
                    role_index,
                    role,
                    surface_context,
                    map_height,
                    object_slots,
                    runtime,
                    result
                )) {
                return result;
            }
            return result;

        case 31U:
            if (!range_available(
                    path_database, command.offset, 4U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            if (const u16 value =
                    read_u16_le(path_database, command.offset + sizeof(u16));
                value != 0xFFFFU) {
                role.action.action_id = value;
            }
            if (const u16 value = read_u16_le(
                    path_database, command.offset + 2U * sizeof(u16)
                );
                value != 0xFFFFU) {
                role.action.base_variant = value;
            }
            if (const u16 value = read_u16_le(
                    path_database, command.offset + 3U * sizeof(u16)
                );
                value != 0xFFFFU) {
                role.action.variant_delta = value;
            }
            advance_cursor(role, result, 4U);
            record_action_update(role, result, ports);
            return result;

        case 32U:
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            ports.play_positional_sample(
                read_u16_le(path_database, command.offset + sizeof(u16)),
                signed_field(role.world_x),
                signed_field(role.world_y)
            );
            advance_cursor(role, result, 2U);
            return result;

        case 33U:
        case 36U: {
            const u32 word_count = opcode == 33U ? 2U : 4U;
            // The missing-role exit at 0x004061B7 reads only the selector and always
            // advances two words, even for opcode 36. Its two offset operands are
            // read only after a role has been resolved.
            if (!range_available(
                    path_database, command.offset, 2U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const ResolvedRoleSelector target_selector = resolve_role_selector(
                roles,
                read_u16_le(path_database, command.offset + sizeof(u16)),
                runtime.controlled_role_index
            );
            const u32 target_index = target_selector.index;
            if (!target_selector.found || target_index >= roles.size()) {
                advance_cursor(role, result, 2U);
                return result;
            }
            if (!range_available(
                    path_database, command.offset, word_count * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            const LegacyWorldRoleRecord& target = roles[target_index];
            if (((target.world_x | target.world_y) & 0x0FU) != 0U) {
                return result;
            }

            const u16 fallback_x = static_cast<u16>(target.world_x >> 4U);
            const u16 fallback_y = static_cast<u16>(target.world_y >> 4U);
            state.camera_target_x = fallback_x;
            state.camera_target_y = fallback_y;
            if (opcode == 36U) {
                const compat::i32 local_x = static_cast<i16>(read_u16_le(
                    path_database, command.offset + 2U * sizeof(u16)
                ));
                const compat::i32 local_y = static_cast<i16>(read_u16_le(
                    path_database, command.offset + 3U * sizeof(u16)
                ));
                bool transformed_offset_written = true;
                switch (target.action.variant_delta) {
                case 0U:
                    legacy_transformed_x = local_x;
                    legacy_transformed_y = -local_y;
                    break;

                case 1U:
                    transformed_offset_written = false;
                    break;

                case 2U:
                    legacy_transformed_x = -local_y;
                    legacy_transformed_y = local_x;
                    break;

                case 3U:
                    legacy_transformed_x = local_y;
                    legacy_transformed_y = -local_x;
                    break;

                case 4U:
                    legacy_transformed_x = local_x + local_y;
                    legacy_transformed_y = local_y - local_x;
                    break;

                case 5U:
                    legacy_transformed_x = -(local_x + local_y);
                    legacy_transformed_y = local_x - local_y;
                    break;

                case 6U:
                    legacy_transformed_x = local_y - local_x;
                    legacy_transformed_y = -(local_x + local_y);
                    break;

                case 7U:
                    legacy_transformed_x = local_x - local_y;
                    legacy_transformed_y = local_x + local_y;
                    break;

                default:
                    transformed_offset_written = false;
                    break;
                }
                if (transformed_offset_written) {
                    legacy_transformed_offset_initialized = true;
                } else if (!legacy_transformed_offset_initialized) {
                    // The original case-1/default branch reads two stack locals that
                    // have not yet been written in this invocation. Valid game data
                    // does not take that branch; expose it instead of inventing values.
                    result.status = LegacyWorldPathScriptStatus::
                        indeterminate_legacy_stack_state;
                    return result;
                }
                const u16 candidate_x =
                    static_cast<u16>(fallback_x + legacy_transformed_x);
                const u16 candidate_y =
                    static_cast<u16>(fallback_y + legacy_transformed_y);
                const std::size_t cell = static_cast<std::size_t>(candidate_y) *
                        surface_context.map_width +
                    candidate_x;
                const std::size_t byte_offset = cell * sizeof(u32);
                if (range_available(
                        surface_context.surface_grid,
                        byte_offset,
                        2U * sizeof(u32)
                    ) &&
                    (read_u32_le(surface_context.surface_grid, byte_offset) &
                     0x40000000U) == 0U &&
                    (read_u32_le(
                         surface_context.surface_grid, byte_offset + sizeof(u32)
                     ) &
                     0x40000000U) == 0U) {
                    state.camera_target_x = candidate_x;
                    state.camera_target_y = candidate_y;
                }
            }

            std::array<u8, 6U> path_command{};
            path_command[2U] = static_cast<u8>(state.camera_target_x);
            path_command[3U] = static_cast<u8>(state.camera_target_x >> 8U);
            path_command[4U] = static_cast<u8>(state.camera_target_y);
            path_command[5U] = static_cast<u8>(state.camera_target_y >> 8U);
            const auto request = request_legacy_world_role_path(
                role_index,
                path_command,
                roles,
                surface_context,
                map_height,
                object_slots,
                node_pool
            );
            ++result.path_requests;
            result.path_request_status = request.status;
            result.pathfinding_status = request.pathfinding_status;
            advance_cursor(role, result, word_count);
            if (radial_distance_difference(role, target) > 128U) {
                role.flags |= kPathCompletionStepFlag;
            }
            break;
        }

        case 34U: {
            if (!range_available(
                    path_database, command.offset, 3U * sizeof(u16)
                )) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            std::size_t terminator = command.offset + 2U * sizeof(u16);
            while (range_available(path_database, terminator, sizeof(u16)) &&
                   read_u16_le(path_database, terminator) != 0x5125U) {
                ++terminator;
            }
            if (!range_available(path_database, terminator, sizeof(u16))) {
                result.status =
                    LegacyWorldPathScriptStatus::path_command_truncated;
                return result;
            }
            role.path_payload_relation =
                read_u16_le(path_database, command.offset + sizeof(u16));
            if (role.path_payload_relation > 10U) {
                role.path_payload_relation = 0U;
            }
            const std::size_t payload_start = command.offset + 2U * sizeof(u16);
            std::vector<u8>& payload = state.role_label_payloads[role_index];
            payload.assign(
                path_database.begin() +
                    static_cast<std::ptrdiff_t>(payload_start),
                path_database.begin() + static_cast<std::ptrdiff_t>(terminator)
            );
            payload.push_back(0U);
            role.path_payload_pointer_32 = role_index + 1U;
            const u32 words =
                static_cast<u32>((terminator - command.offset + 2U) / 2U);
            advance_cursor(role, result, words);
            return result;
        }

        case 35U:
            state.role_label_payloads[role_index].clear();
            if (role.path_payload_pointer_32 != 0U) {
                role.path_payload_pointer_32 = 0U;
                role.path_payload_relation = 0U;
            }
            advance_cursor(role, result, 1U);
            break;

        default:
            result.status = LegacyWorldPathScriptStatus::unsupported_opcode;
            return result;
        }
    }
}

LegacyWorldPathRoleFrameAction select_legacy_world_path_role_frame_action(
    const LegacyWorldRoleRecord& role,
    const u32 role_index,
    const u32 controlled_role_index
) noexcept {
    if ((role.flags & kPathRoleFlag) == 0U) {
        return LegacyWorldPathRoleFrameAction::skip;
    }
    if ((role.flags & kPartyRoleFlag) != 0U) {
        return LegacyWorldPathRoleFrameAction::run_party_path;
    }
    if ((role.flags & kInteractionSuspendedFlag) != 0U ||
        role.interaction_gate == 1U) {
        return role.action.action_id != 0U
            ? LegacyWorldPathRoleFrameAction::mark_surface
            : LegacyWorldPathRoleFrameAction::skip;
    }
    if (role.path_data_id != 0U) {
        return LegacyWorldPathRoleFrameAction::run_path_script;
    }
    if ((role.flags & 0x00001000U) != 0U &&
        role_index != controlled_role_index) {
        return LegacyWorldPathRoleFrameAction::update_action_then_mark_surface;
    }
    return LegacyWorldPathRoleFrameAction::skip;
}

LegacyWorldPathScriptScanResult run_legacy_world_path_scripts(
    const std::span<const u8> path_database,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext& surface_context,
    const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool& node_pool,
    LegacyWorldPathScriptState& state,
    const LegacyWorldPathScriptRuntime& runtime,
    LegacyWorldPathScriptPorts& ports
) {
    LegacyWorldPathScriptScanResult result;
    for (u32 role_index = 1U; role_index < roles.size(); ++role_index) {
        ++result.roles_scanned;
        LegacyWorldRoleRecord& role = roles[role_index];
        const auto action = select_legacy_world_path_role_frame_action(
            role, role_index, runtime.controlled_role_index
        );
        if (action == LegacyWorldPathRoleFrameAction::mark_surface) {
            static_cast<void>(
                mark_legacy_world_role_surface_occupancy(role, surface_context)
            );
            continue;
        }
        if (action ==
            LegacyWorldPathRoleFrameAction::update_action_then_mark_surface) {
            static_cast<void>(ports.update_action(role.action));
            if (role.action.action_id != 0U) {
                static_cast<void>(mark_legacy_world_role_surface_occupancy(
                    role, surface_context
                ));
            }
            continue;
        }
        if (action != LegacyWorldPathRoleFrameAction::run_path_script) {
            continue;
        }

        ++result.eligible_roles;
        result.last_role_result = run_legacy_world_path_script(
            role_index,
            path_database,
            roles,
            surface_context,
            map_height,
            object_slots,
            node_pool,
            state,
            runtime,
            ports
        );
        if (result.last_role_result.status ==
            LegacyWorldPathScriptStatus::completed) {
            ++result.scripts_completed;
            continue;
        }
        if (result.last_role_result.status ==
            LegacyWorldPathScriptStatus::unsupported_opcode) {
            ++result.unsupported_scripts;
            continue;
        }
        result.status = result.last_role_result.status;
        return result;
    }
    return result;
}

std::span<const u8> resolve_legacy_world_path_label(
    const LegacyWorldPathScriptState& state, const u32 token
) noexcept {
    if (token == 0U || token > state.role_label_payloads.size()) {
        return {};
    }
    return state.role_label_payloads[token - 1U];
}

}  // namespace openswd3::world_map
