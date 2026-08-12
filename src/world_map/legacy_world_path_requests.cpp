#include "openswd3/world_map/legacy_world_path_requests.hpp"

#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>

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
constexpr std::size_t kPathStallOffset = 0x1AU;
constexpr std::size_t kPathFlagsOffset = 0x1BU;
constexpr std::size_t kPathBytesOffset = 0x1CU;

constexpr u16 kNoRole = 0xFFFFU;
constexpr u16 kPathCursorMask = 0x7FFFU;
constexpr u32 kPartyRoleFlag = 0x00000080U;
constexpr u32 kSpatialRoleFlag = 0x00008000U;
constexpr u32 kPartyCollisionMask = 0x40000000U;

constexpr std::array<i32, 8U> kSubCellStepX{4, 0, -4, -4, -4, 0, 4, 4};
constexpr std::array<i32, 8U> kSubCellStepY{4, 4, 4, 0, -4, -4, -4, 0};
constexpr std::array<u32, 8U> kDirectionCollisionBits{
    0x10U, 0x20U, 0x40U, 0x80U, 0x01U, 0x02U, 0x04U, 0x08U};
constexpr std::array<u32, 8U> kDirectionToVariant{5U, 1U, 6U, 2U,
                                                  4U, 0U, 7U, 3U};

[[nodiscard]] u16 read_u16_le(const std::span<const u8> bytes,
                              const std::size_t offset) noexcept {
  return static_cast<u16>(bytes[offset]) |
         static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u16 read_u16_le(const LegacyWorldObjectSlot &slot,
                              const std::size_t offset) noexcept {
  return static_cast<u16>(slot.bytes[offset]) |
         static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_u16_le(LegacyWorldObjectSlot &slot, const std::size_t offset,
                  const u16 value) noexcept {
  slot.bytes[offset] = static_cast<u8>(value);
  slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] constexpr u32 wrapping_add(const u32 left,
                                         const i32 right) noexcept {
  return left + std::bit_cast<u32>(right);
}

[[nodiscard]] bool
path_fits_slot(const LegacyWorldPathfindingResult &path) noexcept {
  return path.path.size() <= kLegacyWorldObjectSlotSize - kPathBytesOffset;
}

void copy_path_to_slot(const LegacyWorldPathfindingResult &path,
                       LegacyWorldObjectSlot &slot) noexcept {
  std::ranges::copy(path.path, slot.bytes.begin() + static_cast<std::ptrdiff_t>(
                                                        kPathBytesOffset));
}

[[nodiscard]] u32 clamped_view_start(const u32 value,
                                     const u32 margin) noexcept {
  const u32 candidate = value - margin;
  return std::bit_cast<i32>(candidate) < 0 ? 0U : candidate;
}

[[nodiscard]] bool
inside_follower_view(const LegacyWorldRoleRecord &role,
                     const LegacyWorldCameraRect &camera) noexcept {
  const u32 left = clamped_view_start(camera.left, 0x50U);
  const u32 top = clamped_view_start(camera.top, 0x50U);
  const u32 right = camera.right + 0x50U;
  const u32 bottom = camera.bottom + 0xA0U;
  return role.world_x > left && role.world_x < right && role.world_y > top &&
         role.world_y < bottom;
}

[[nodiscard]] bool initialize_party_slot(
    LegacyWorldPartyPathPreparationResult &result,
    LegacyWorldPathNodePool &node_pool, LegacyWorldRoleRecord &role,
    const u32 role_index, const u32 target_x, const u32 target_y,
    const LegacyWorldRoleSurfaceContext &surface_context, const u32 map_height,
    LegacyWorldPartyPathPorts &ports, LegacyWorldObjectSlot &slot) {
  slot.bytes.fill(0xFFU);
  LegacyWorldPathfinder pathfinder{node_pool};
  ++result.collision_service_queries;
  if (ports.query_collision_disabled()) {
    pathfinder.set_collision_mask(0U);
  }
  const auto path = pathfinder.find_path({
      .start_x = std::bit_cast<i32>(role.world_x),
      .start_y = std::bit_cast<i32>(role.world_y),
      .target_x = std::bit_cast<i32>(target_x),
      .target_y = std::bit_cast<i32>(target_y),
      .footprint_width = role.action.field_2c,
      .footprint_height = role.action.field_30,
      .map_width = surface_context.map_width,
      .map_height = map_height,
      .surface_grid = surface_context.surface_grid,
  });
  result.pathfinding_status = path.status;
  if (path.status != LegacyWorldPathfindingStatus::completed) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::pathfinding_boundary_failed;
    return false;
  }
  if (path.legacy_return_value == 0) {
    ++result.pathfinding_failures;
    // The original continues with cursor FFFF and reads far beyond the 21C
    // object. Valid game data is not expected to reach this branch; stop at
    // the exact modern ownership boundary instead of inventing bytes.
    result.status =
        LegacyWorldPartyPathPreparationStatus::path_cursor_out_of_range;
    return false;
  }
  if (!path_fits_slot(path)) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::path_does_not_fit_slot;
    return false;
  }

  copy_path_to_slot(path, slot);
  write_u16_le(slot, kRoleIndexOffset, static_cast<u16>(role_index));
  write_u16_le(slot, kPathCursorOffset, 0U);
  write_u16_le(slot, kDestinationXOffset, static_cast<u16>(target_x));
  write_u16_le(slot, kDestinationYOffset, static_cast<u16>(target_y));
  slot.bytes[kPathFlagsOffset] =
      static_cast<u8>((slot.bytes[kPathFlagsOffset] & 0xF1U) | 1U);
  slot.bytes[kPathStallOffset] = 0U;
  ++result.paths_generated;
  return true;
}

[[nodiscard]] bool clear_role_surface(
    LegacyWorldPartyPathPreparationResult &result,
    const LegacyWorldRoleRecord &role,
    const LegacyWorldRoleSurfaceContext &surface_context) noexcept {
  const auto cleared =
      clear_legacy_world_role_surface_occupancy(role, surface_context);
  result.surface_status = cleared.status;
  if (cleared.status == LegacyWorldRoleSurfaceStatus::ready) {
    return true;
  }
  result.status = LegacyWorldPartyPathPreparationStatus::surface_clear_failed;
  return false;
}

[[nodiscard]] bool mark_role_surface(
    LegacyWorldPartyPathPreparationResult &result,
    const LegacyWorldRoleRecord &role,
    const LegacyWorldRoleSurfaceContext &surface_context) noexcept {
  const auto marked =
      mark_legacy_world_role_surface_occupancy(role, surface_context);
  result.surface_status = marked.status;
  if (marked.status == LegacyWorldRoleSurfaceStatus::ready) {
    return true;
  }
  result.status = LegacyWorldPartyPathPreparationStatus::surface_mark_failed;
  return false;
}

[[nodiscard]] bool
preadvance_party_role(LegacyWorldPartyPathPreparationResult &result,
                      LegacyWorldRoleRecord &role, LegacyWorldObjectSlot &slot,
                      const LegacyWorldCameraRect &camera,
                      const LegacyWorldRoleSurfaceContext &surface_context,
                      LegacyRoleSpatialIndex &spatial_index,
                      const std::span<LegacyWorldRoleRecord> roles) noexcept {
  if (!clear_role_surface(result, role, surface_context)) {
    return false;
  }

  u8 direction{};
  while (true) {
    const u16 cursor = read_u16_le(slot, kPathCursorOffset);
    const std::size_t direction_offset =
        kPathBytesOffset + static_cast<std::size_t>(cursor);
    if (direction_offset >= slot.bytes.size()) {
      result.status =
          LegacyWorldPartyPathPreparationStatus::path_cursor_out_of_range;
      return false;
    }
    direction = slot.bytes[direction_offset];
    if (inside_follower_view(role, camera) || direction == 0xFFU) {
      break;
    }
    if (direction >= kSubCellStepX.size()) {
      result.status =
          LegacyWorldPartyPathPreparationStatus::direction_out_of_range;
      return false;
    }
    write_u16_le(slot, kPathCursorOffset, static_cast<u16>(cursor + 1U));
    role.world_x = wrapping_add(role.world_x, kSubCellStepX[direction]);
    role.world_y = wrapping_add(role.world_y, kSubCellStepY[direction]);
    ++result.preadvanced_steps;
  }
  if (direction != 0xFFU) {
    if (direction >= kDirectionToVariant.size()) {
      result.status =
          LegacyWorldPartyPathPreparationStatus::direction_out_of_range;
      return false;
    }
    role.action.variant_delta = kDirectionToVariant[direction];
  }

  const u32 tile_x = role.world_x >> 4U;
  const u32 tile_y = role.world_y >> 4U;
  role.map_cell_pointer_32 = tile_y * surface_context.map_width + tile_x;
  if (!mark_role_surface(result, role, surface_context)) {
    return false;
  }
  result.spatial_removal_status = relocate_legacy_role_spatially_by_guid(
      spatial_index, roles, role.guid, role.flags & 3U, 0, false);
  if (result.spatial_removal_status !=
      LegacyRoleSpatialRelocationStatus::ready) {
    ++result.spatial_removal_failures;
  }
  return true;
}

[[nodiscard]] bool
finish_party_slot(LegacyWorldPartyPathPreparationResult &result,
                  LegacyWorldRoleRecord &role, LegacyWorldObjectSlot &slot,
                  const LegacyWorldRoleSurfaceContext &surface_context,
                  const u32 map_height, LegacyWorldPartyPathPorts &ports,
                  const LegacyWorldRoleRecord &selected_role) noexcept {
  const u16 cursor =
      static_cast<u16>(read_u16_le(slot, kPathCursorOffset) & kPathCursorMask);
  const std::size_t direction_offset = kPathBytesOffset + cursor;
  if (direction_offset >= slot.bytes.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::path_cursor_out_of_range;
    return false;
  }
  const u8 direction = slot.bytes[direction_offset];

  ++result.collision_service_queries;
  const bool collision_disabled = ports.query_collision_disabled();
  const auto occupancy = compute_legacy_world_directional_occupancy_mask(
      surface_context.surface_grid, surface_context.map_width, map_height,
      role.map_cell_pointer_32, role.action.field_2c, role.action.field_30,
      collision_disabled ? 0U : kPartyCollisionMask);
  result.directional_probe_status = occupancy.status;
  if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::directional_probe_failed;
    return false;
  }

  slot.bytes[3U] |= 0x80U;
  write_u16_le(slot, kStepXOffset, 0U);
  write_u16_le(slot, kStepYOffset, 0U);
  if (direction == 0xFFU) {
    role.action.base_variant = 0U;
    role.action.variant_delta = selected_role.action.variant_delta;
    ++result.terminal_paths;
    return true;
  }
  if (direction >= kDirectionCollisionBits.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::direction_out_of_range;
    return false;
  }
  if ((occupancy.mask & kDirectionCollisionBits[direction]) != 0U) {
    role.action.base_variant = 0U;
    ++result.movement_slots_blocked;
    return true;
  }

  write_u16_le(slot, kPathCursorOffset, cursor);
  ++slot.bytes[kPathStallOffset];
  role.action.base_variant = 8U;
  const i16 step_x = static_cast<i16>(kSubCellStepX[direction] * 2);
  const i16 step_y = static_cast<i16>(kSubCellStepY[direction] * 2);
  write_u16_le(slot, kStepXOffset, std::bit_cast<u16>(step_x));
  write_u16_le(slot, kStepYOffset, std::bit_cast<u16>(step_y));
  ++result.movement_slots_enabled;
  return true;
}

[[nodiscard]] bool prepare_one_party_path(
    LegacyWorldPartyPathPreparationResult &result,
    LegacyWorldPathNodePool &node_pool, const u32 role_index,
    const u32 party_role_count, const std::span<const u32> party_role_indices,
    const std::span<LegacyWorldObjectSlot> party_object_slots,
    const LegacyWorldPlayerPostFrameState &player_history,
    const u32 selected_role_index, const LegacyWorldCameraRect &camera,
    LegacyWorldPartyPathPorts &ports,
    const LegacyWorldRoleSurfaceContext &surface_context,
    LegacyRoleSpatialIndex &spatial_index,
    const std::span<LegacyWorldRoleRecord> roles) {
  LegacyWorldRoleRecord &role = roles[role_index];
  if (((role.world_x | role.world_y) & 0x0FU) != 0U) {
    ++result.unaligned_roles;
    return true;
  }

  u32 party_index = 1U;
  while (party_index < party_role_count &&
         party_role_indices[party_index] != role_index) {
    ++party_index;
  }
  if (party_index == party_role_count) {
    ++result.missing_party_roles;
    return true;
  }
  const std::size_t history_index = static_cast<std::size_t>(party_index) * 2U;
  if (history_index >= player_history.world_x_history.size() ||
      history_index >= player_history.world_y_history.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::history_index_out_of_range;
    return false;
  }
  LegacyWorldObjectSlot &slot = party_object_slots[party_index];
  const u16 existing_role_index = read_u16_le(slot, kRoleIndexOffset);
  const u16 existing_cursor =
      static_cast<u16>(read_u16_le(slot, kPathCursorOffset) & kPathCursorMask);
  const std::size_t existing_direction_offset =
      kPathBytesOffset + existing_cursor;
  bool reuse = false;
  if (existing_role_index != kNoRole && slot.bytes[kPathStallOffset] == 0U) {
    if (existing_direction_offset >= slot.bytes.size()) {
      result.status =
          LegacyWorldPartyPathPreparationStatus::path_cursor_out_of_range;
      return false;
    }
    reuse = slot.bytes[existing_direction_offset] != 0xFFU;
  }
  if (reuse) {
    ++result.paths_reused;
  } else {
    const u32 target_x = player_history.world_x_history[history_index];
    const u32 target_y = player_history.world_y_history[history_index];
    if (!initialize_party_slot(result, node_pool, role, role_index, target_x,
                               target_y, surface_context,
                               spatial_index.map_height, ports, slot) ||
        !preadvance_party_role(result, role, slot, camera, surface_context,
                               spatial_index, roles)) {
      return false;
    }
  }

  return finish_party_slot(result, role, slot, surface_context,
                           spatial_index.map_height, ports,
                           roles[selected_role_index]);
}

} // namespace

LegacyWorldRolePathRequestResult request_legacy_world_role_path(
    const u32 role_index, const std::span<const u8> path_command,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldRoleSurfaceContext &surface_context, const u32 map_height,
    const std::span<LegacyWorldObjectSlot> object_slots,
    LegacyWorldPathNodePool &node_pool) {
  LegacyWorldRolePathRequestResult result;
  if (role_index >= roles.size()) {
    result.status = LegacyWorldRolePathRequestStatus::invalid_role_index;
    return result;
  }
  if (path_command.size() < 6U) {
    result.status = LegacyWorldRolePathRequestStatus::command_truncated;
    return result;
  }
  if (object_slots.size() < kLegacyWorldActiveObjectSlotCount) {
    result.status = LegacyWorldRolePathRequestStatus::insufficient_object_slots;
    return result;
  }

  u32 slot_index = 0U;
  while (slot_index < kLegacyWorldActiveObjectSlotCount &&
         read_u16_le(object_slots[slot_index], kRoleIndexOffset) != kNoRole) {
    ++slot_index;
  }
  result.slot_index = slot_index;
  result.legacy_return_value = slot_index != 0x20U ? 1 : 0;
  if (slot_index == kLegacyWorldActiveObjectSlotCount) {
    return result;
  }
  result.free_slot_found = true;

  LegacyWorldPathfinder pathfinder{node_pool};

  const u32 target_x = static_cast<u32>(read_u16_le(path_command, 2U)) << 4U;
  const u32 target_y = static_cast<u32>(read_u16_le(path_command, 4U)) << 4U;
  const u32 maximum_x = surface_context.map_width << 4U;
  const u32 maximum_y = map_height << 4U;
  if (target_x > maximum_x || target_y > maximum_y) {
    result.legacy_return_value = 0;
    return result;
  }
  result.target_in_legacy_bounds = true;

  LegacyWorldRoleRecord &role = roles[role_index];
  const auto path = pathfinder.find_path({
      .start_x = std::bit_cast<i32>(role.world_x),
      .start_y = std::bit_cast<i32>(role.world_y),
      .target_x = std::bit_cast<i32>(target_x),
      .target_y = std::bit_cast<i32>(target_y),
      .footprint_width = role.action.field_2c,
      .footprint_height = role.action.field_30,
      .map_width = surface_context.map_width,
      .map_height = map_height,
      .surface_grid = surface_context.surface_grid,
  });
  result.pathfinding_status = path.status;
  if (path.status != LegacyWorldPathfindingStatus::completed) {
    result.status =
        LegacyWorldRolePathRequestStatus::pathfinding_boundary_failed;
    return result;
  }

  if (path.legacy_return_value == 1) {
    if (!path_fits_slot(path)) {
      result.status = LegacyWorldRolePathRequestStatus::path_does_not_fit_slot;
      return result;
    }
    LegacyWorldObjectSlot &slot = object_slots[slot_index];
    copy_path_to_slot(path, slot);
    write_u16_le(slot, kActionIdOffset, 0xFFFFU);
    write_u16_le(slot, kBaseVariantOffset, 0xFFFFU);
    write_u16_le(slot, kVariantDeltaOffset, 0xFFFFU);
    write_u16_le(slot, kRoleIndexOffset, static_cast<u16>(role_index));
    write_u16_le(slot, kPathCursorOffset, 0U);
    write_u16_le(slot, kDestinationXOffset, static_cast<u16>(target_x));
    write_u16_le(slot, kDestinationYOffset, static_cast<u16>(target_y));
    slot.bytes[kPathFlagsOffset] =
        static_cast<u8>((slot.bytes[kPathFlagsOffset] & 0xF1U) | 1U);
    result.path_found = true;
    return result;
  }

  role.world_x = target_x;
  role.world_y = target_y;
  result.role_relocated_after_path_failure = true;
  return result;
}

LegacyWorldPartyPathPreparationResult prepare_legacy_world_party_paths(
    const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex &spatial_index,
    const LegacyWorldRoleSurfaceContext &surface_context,
    const u32 selected_role_index, const u32 party_role_count,
    const std::span<const u32> party_role_indices,
    const std::span<LegacyWorldObjectSlot> party_object_slots,
    const LegacyWorldPlayerPostFrameState &player_history,
    const LegacyWorldCameraRect &camera, LegacyWorldPathNodePool &node_pool,
    LegacyWorldPartyPathPorts &ports) {
  LegacyWorldPartyPathPreparationResult result;
  if (selected_role_index >= roles.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::invalid_selected_role_index;
    return result;
  }
  if (party_role_count > party_role_indices.size() ||
      party_role_count > party_object_slots.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::invalid_party_role_count;
    return result;
  }

  for (u32 role_index = 1U; role_index < roles.size(); ++role_index) {
    ++result.roles_scanned;
    const u32 flags = roles[role_index].flags;
    if ((flags & kSpatialRoleFlag) == 0U || (flags & kPartyRoleFlag) == 0U) {
      continue;
    }
    ++result.eligible_roles;
    if (!prepare_one_party_path(result, node_pool, role_index, party_role_count,
                                party_role_indices, party_object_slots,
                                player_history, selected_role_index, camera,
                                ports, surface_context, spatial_index, roles)) {
      return result;
    }
  }
  return result;
}

LegacyWorldPartyPathPreparationResult prepare_legacy_world_party_path(
    const u32 role_index, const std::span<LegacyWorldRoleRecord> roles,
    LegacyRoleSpatialIndex &spatial_index,
    const LegacyWorldRoleSurfaceContext &surface_context,
    const u32 selected_role_index, const u32 party_role_count,
    const std::span<const u32> party_role_indices,
    const std::span<LegacyWorldObjectSlot> party_object_slots,
    const LegacyWorldPlayerPostFrameState &player_history,
    const LegacyWorldCameraRect &camera, LegacyWorldPathNodePool &node_pool,
    LegacyWorldPartyPathPorts &ports) {
  LegacyWorldPartyPathPreparationResult result;
  if (selected_role_index >= roles.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::invalid_selected_role_index;
    return result;
  }
  if (role_index >= roles.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::invalid_party_role_index;
    return result;
  }
  if (party_role_count > party_role_indices.size() ||
      party_role_count > party_object_slots.size()) {
    result.status =
        LegacyWorldPartyPathPreparationStatus::invalid_party_role_count;
    return result;
  }

  result.roles_scanned = 1U;
  result.eligible_roles = 1U;
  static_cast<void>(prepare_one_party_path(
      result, node_pool, role_index, party_role_count, party_role_indices,
      party_object_slots, player_history, selected_role_index, camera, ports,
      surface_context, spatial_index, roles));
  return result;
}

} // namespace openswd3::world_map
