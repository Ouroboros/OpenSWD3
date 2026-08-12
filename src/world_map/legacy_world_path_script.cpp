#include "openswd3/world_map/legacy_world_path_script.hpp"

#include <array>
#include <bit>
#include <cstddef>

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
    0x10U, 0x20U, 0x40U, 0x80U, 0x01U, 0x02U, 0x04U, 0x08U};
constexpr std::array<i16, 8U> kSubCellStepX{4, 0, -4, -4, -4, 0, 4, 4};
constexpr std::array<i16, 8U> kSubCellStepY{4, 4, 4, 0, -4, -4, -4, 0};

[[nodiscard]] bool range_available(const std::span<const u8> bytes,
                                   const std::size_t offset,
                                   const std::size_t size) noexcept {
  return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] u16 read_u16_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
  return static_cast<u16>(bytes[offset]) |
         static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_u32_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
  return static_cast<u32>(bytes[offset]) |
         (static_cast<u32>(bytes[offset + 1U]) << 8U) |
         (static_cast<u32>(bytes[offset + 2U]) << 16U) |
         (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] u16 read_slot_u16(const LegacyWorldObjectSlot &slot,
                                const std::size_t offset) noexcept {
  return static_cast<u16>(slot.bytes[offset]) |
         static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_slot_u16(LegacyWorldObjectSlot &slot, const std::size_t offset,
                    const u16 value) noexcept {
  slot.bytes[offset] = static_cast<u8>(value);
  slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

struct PathCommandView {
  LegacyWorldPathScriptStatus status{LegacyWorldPathScriptStatus::completed};
  std::size_t offset{};
};

[[nodiscard]] PathCommandView
resolve_path_command(const std::span<const u8> path_database,
                     const LegacyWorldRoleRecord &role) noexcept {
  const std::size_t directory_offset =
      kPathDatabasePayloadOffset +
      static_cast<std::size_t>(role.path_data_id) * sizeof(u32);
  if (!range_available(path_database, directory_offset, sizeof(u32))) {
    return {LegacyWorldPathScriptStatus::path_directory_entry_out_of_range, 0U};
  }

  const u32 relative = read_u32_le(path_database, directory_offset);
  const std::size_t command_offset =
      kPathDatabasePayloadOffset + static_cast<std::size_t>(relative) +
      static_cast<std::size_t>(role.path_word_index) * sizeof(u16);
  if (!range_available(path_database, command_offset, sizeof(u16))) {
    return {LegacyWorldPathScriptStatus::path_command_out_of_range, 0U};
  }
  return {LegacyWorldPathScriptStatus::completed, command_offset};
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
      LegacyWorldDirectionProbeStatus::completed};
};

[[nodiscard]] PathMovementResult prepare_role_path_movement(
    const u32 role_index, LegacyWorldRoleRecord &role,
    const LegacyWorldRoleSurfaceContext &surface_context, const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots) noexcept {
  if (object_slots.size() < kLegacyWorldActiveObjectSlotCount) {
    return {PathMovementStatus::insufficient_slots};
  }

  for (LegacyWorldObjectSlot &slot :
       object_slots.first(kLegacyWorldActiveObjectSlotCount)) {
    if (read_slot_u16(slot, kRoleIndexOffset) != static_cast<u16>(role_index)) {
      continue;
    }

    const u8 slot_kind = static_cast<u8>(slot.bytes[kPathFlagsOffset] & 0x0FU);
    if (slot_kind == 2U) {
      return {PathMovementStatus::yielded};
    }
    if (slot_kind != 1U) {
      continue;
    }
    if (role.interaction_gate == 1U) {
      write_slot_u16(slot, kPathCursorOffset,
                     static_cast<u16>(read_slot_u16(slot, kPathCursorOffset) |
                                      kPathCursorFrameGate));
      return {PathMovementStatus::yielded};
    }

    const u16 cursor = static_cast<u16>(read_slot_u16(slot, kPathCursorOffset) &
                                        kPathCursorMask);
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
        surface_context.surface_grid, surface_context.map_width, map_height,
        role.map_cell_pointer_32, role.action.field_2c, role.action.field_30,
        collision_mask);
    if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
      return {PathMovementStatus::directional_probe_failed, occupancy.status};
    }

    write_slot_u16(slot, kPathCursorOffset,
                   static_cast<u16>(cursor | kPathCursorFrameGate));
    ++slot.bytes[kPathStallOffset];
    write_slot_u16(slot, kStepXOffset, 0U);
    write_slot_u16(slot, kStepYOffset, 0U);
    if ((occupancy.mask & kDirectionCollisionBits[direction]) != 0U) {
      return {PathMovementStatus::active};
    }

    write_slot_u16(slot, kPathCursorOffset, cursor);
    slot.bytes[kPathStallOffset] = 0U;
    write_slot_u16(slot, kStepXOffset,
                   std::bit_cast<u16>(kSubCellStepX[direction]));
    write_slot_u16(slot, kStepYOffset,
                   std::bit_cast<u16>(kSubCellStepY[direction]));
    return {PathMovementStatus::active};
  }
  return {PathMovementStatus::no_slot};
}

} // namespace

LegacyWorldPathScriptResult run_legacy_world_path_script(
    const u32 role_index, const std::span<const u8> path_database,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context, const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool) {
  LegacyWorldPathScriptResult result;
  if (role_index >= roles.size()) {
    result.status = LegacyWorldPathScriptStatus::invalid_role_index;
    return result;
  }

  LegacyWorldRoleRecord &role = roles[role_index];
  for (;;) {
    const PathCommandView command = resolve_path_command(path_database, role);
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

    case 4U:
      if (!range_available(path_database, command.offset, 2U * sizeof(u16))) {
        result.status = LegacyWorldPathScriptStatus::path_command_truncated;
        return result;
      }
      role.path_wait_remaining =
          read_u16_le(path_database, command.offset + sizeof(u16));
      role.path_word_index += 2U;
      result.cursor_words_advanced += 2U;
      ++result.waits_set;
      break;

    case 5U:
      if (role.path_wait_remaining > 0U) {
        --role.path_wait_remaining;
        ++result.waits_decremented;
      } else {
        ++role.path_word_index;
        ++result.cursor_words_advanced;
      }
      return result;

    case 7U: {
      if (!range_available(path_database, command.offset, 3U * sizeof(u16))) {
        result.status = LegacyWorldPathScriptStatus::path_command_truncated;
        return result;
      }
      const auto request = request_legacy_world_role_path(
          role_index, path_database.subspan(command.offset, 3U * sizeof(u16)),
          roles, surface_context, map_height, object_slots, node_pool);
      ++result.path_requests;
      result.path_request_status = request.status;
      result.pathfinding_status = request.pathfinding_status;
      if (request.status != LegacyWorldRolePathRequestStatus::completed) {
        result.status = LegacyWorldPathScriptStatus::path_request_failed;
        return result;
      }
      role.path_word_index += 3U;
      result.cursor_words_advanced += 3U;
      break;
    }

    case 8U: {
      if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
        return result;
      }
      const auto movement = prepare_role_path_movement(
          role_index, role, surface_context, map_height, object_slots);
      result.directional_probe_status = movement.directional_probe_status;
      if (movement.status == PathMovementStatus::insufficient_slots) {
        result.status = LegacyWorldPathScriptStatus::insufficient_object_slots;
        return result;
      }
      if (movement.status == PathMovementStatus::directional_probe_failed) {
        result.status = LegacyWorldPathScriptStatus::directional_probe_failed;
        return result;
      }
      if (movement.status == PathMovementStatus::direction_out_of_range) {
        result.status = LegacyWorldPathScriptStatus::direction_out_of_range;
        return result;
      }
      if (movement.status == PathMovementStatus::active) {
        ++result.movement_slots_advanced;
        return result;
      }
      if (movement.status == PathMovementStatus::completed ||
          movement.status == PathMovementStatus::no_slot) {
        ++result.movement_slots_completed;
        ++role.path_word_index;
        ++result.cursor_words_advanced;
        role.flags &= ~kPathCompletionStepFlag;
        break;
      }
      return result;
    }

    default:
      result.status = LegacyWorldPathScriptStatus::unsupported_opcode;
      return result;
    }
  }
}

LegacyWorldPathScriptScanResult run_legacy_world_path_scripts(
    const std::span<const u8> path_database,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context, const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool) {
  LegacyWorldPathScriptScanResult result;
  for (u32 role_index = 1U; role_index < roles.size(); ++role_index) {
    ++result.roles_scanned;
    const LegacyWorldRoleRecord &role = roles[role_index];
    if ((role.flags & kPathRoleFlag) == 0U ||
        (role.flags & kPartyRoleFlag) != 0U ||
        (role.flags & kInteractionSuspendedFlag) != 0U ||
        role.interaction_gate == 1U || role.path_data_id == 0U) {
      continue;
    }

    ++result.eligible_roles;
    result.last_role_result = run_legacy_world_path_script(
        role_index, path_database, roles, surface_context, map_height,
        object_slots, node_pool);
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

} // namespace openswd3::world_map
