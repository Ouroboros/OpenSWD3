#include "openswd3/world_map/legacy_world_flagged_roles.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
  return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
  return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(const i32 left,
                                         const i32 right) noexcept {
  return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_subtract(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 field_as_i32(const u32 value) noexcept {
  return from_bits(value);
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status) noexcept {
  return status == rendering::LegacyBlitExecutionStatus::completed ||
         status == rendering::LegacyBlitExecutionStatus::clipped_out ||
         status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] constexpr i32 truncating_tile_row(const i32 value) noexcept {
  // cdq; and edx, 0Fh; add eax, edx; sar eax, 4
  return value / 16;
}

} // namespace

LegacyWorldFlaggedRoleDrawResult
draw_legacy_world_flagged_role(const LegacyWorldRoleRecord &role,
                               const LegacyWorldRenderCamera camera,
                               asset_runtime::LegacyActionDrawPorts &ports) {
  LegacyWorldFlaggedRoleDrawResult result;
  result.drawable = (role.flags & kLegacyWorldDrawableRoleBits) ==
                    kLegacyWorldDrawableRoleValue;
  if (!result.drawable) {
    return result;
  }

  const i32 horizontal_distance =
      wrapping_subtract(field_as_i32(role.world_x), camera.left);
  result.horizontally_visible =
      horizontal_distance > -320 && horizontal_distance < 960;
  if (!result.horizontally_visible) {
    return result;
  }

  result.resource_id =
      role.action.action_id == 0U ? u16{0xFFFFU} : role.action.field_4a;
  result.frame_index = role.action.field_4c;
  result.frame_requested = true;
  rendering::LegacyFramePiece piece;
  if (!ports.load_frame_piece(result.resource_id, result.frame_index, piece)) {
    result.status = LegacyWorldFlaggedRoleDrawStatus::frame_load_failed;
    return result;
  }

  result.destination_x = wrapping_add(
      wrapping_subtract(
          wrapping_subtract(static_cast<i32>(role.field_28),
                            field_as_i32(role.action.draw_offset_x)),
          camera.left),
      field_as_i32(role.world_x));
  result.destination_y = wrapping_add(
      wrapping_add(
          wrapping_subtract(static_cast<i32>(role.field_2a), camera.top),
          field_as_i32(role.world_y)),
      8);
  result.flags = (role.action.mode_flags & 0x80000017U) | 0x00000016U;
  result.opacity_step = 4;
  result.last_blit_status =
      ports.draw_frame_piece(piece, result.destination_x, result.destination_y,
                             result.flags, result.opacity_step);
  result.drawn = true;
  if (!accepted_blit_status(result.last_blit_status)) {
    result.blit_failure_count = 1U;
  }
  return result;
}

LegacyWorldFlaggedRolesResult draw_legacy_world_flagged_roles(
    const LegacyRoleSpatialIndex &spatial_index,
    const std::span<const LegacyWorldRoleRecord> roles,
    const LegacyWorldRenderCamera camera,
    asset_runtime::LegacyActionDrawPorts &ports) {
  LegacyWorldFlaggedRolesResult result;
  constexpr std::size_t group = 0U;
  const auto &row_heads = spatial_index.row_heads[group];
  const std::size_t expected_rows =
      static_cast<std::size_t>(spatial_index.map_height) +
      2U * static_cast<std::size_t>(kLegacySpatialRowPadding);
  if (row_heads.size() < expected_rows || roles.empty()) {
    return result;
  }

  result.status = LegacyWorldFlaggedRolesStatus::completed;
  i32 row = truncating_tile_row(camera.top) - 5;
  for (i32 scan = -10; scan < 30; ++scan, ++row) {
    if (row >= static_cast<i32>(spatial_index.map_height)) {
      break;
    }
    ++result.visited_rows;
    if (row < 0) {
      continue;
    }

    const i32 padded_row = row + static_cast<i32>(kLegacySpatialRowPadding);
    if (padded_row < 0 ||
        static_cast<std::size_t>(padded_row) >= row_heads.size()) {
      result.status = LegacyWorldFlaggedRolesStatus::invalid_spatial_index;
      return result;
    }

    u32 role_index = row_heads[static_cast<std::size_t>(padded_row)];
    u32 link_count = 0U;
    while (role_index != kLegacySpatialNoRole) {
      if (role_index >= roles.size() || ++link_count >= roles.size()) {
        result.status = LegacyWorldFlaggedRolesStatus::invalid_role_link;
        return result;
      }

      const LegacyWorldRoleRecord &role = roles[role_index];
      ++result.visited_roles;
      if ((role.flags & kLegacyWorldFlaggedRoleBit) != 0U) {
        ++result.flagged_roles;
        const LegacyWorldFlaggedRoleDrawResult draw =
            draw_legacy_world_flagged_role(role, camera, ports);
        result.frame_requests += draw.frame_requested ? 1U : 0U;
        result.draw_count += draw.drawn ? 1U : 0U;
        result.blit_failure_count += draw.blit_failure_count;
        if (draw.status ==
            LegacyWorldFlaggedRoleDrawStatus::frame_load_failed) {
          result.status = LegacyWorldFlaggedRolesStatus::frame_load_failed;
          return result;
        }
      }
      role_index = role.spatial_next_link_32;
    }
  }
  return result;
}

} // namespace openswd3::world_map
