#include "openswd3/world_map/legacy_moving_actions.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace openswd3::world_map {
namespace {

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
  return std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(left) -
                                    std::bit_cast<compat::u32>(right));
}

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::u32 right) noexcept {
  return std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(left) - right);
}

[[nodiscard]] compat::i32
truncate_x87_float_to_low_i32(const float value) noexcept {
  if (!std::isfinite(value)) {
    return 0;
  }
  const double truncated = std::trunc(static_cast<double>(value));
  constexpr double kMinimum =
      static_cast<double>(std::numeric_limits<std::int64_t>::min());
  constexpr double kMaximum =
      static_cast<double>(std::numeric_limits<std::int64_t>::max());
  if (truncated < kMinimum || truncated >= kMaximum) {
    return 0;
  }
  const auto integer = static_cast<std::int64_t>(truncated);
  return std::bit_cast<compat::i32>(static_cast<compat::u32>(integer));
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status) noexcept {
  return status == rendering::LegacyBlitExecutionStatus::completed ||
         status == rendering::LegacyBlitExecutionStatus::clipped_out ||
         status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

} // namespace

LegacyMovingActionResult update_draw_legacy_moving_actions(
    LegacyMovingActionList &nodes, const compat::i32 camera_left,
    const compat::i32 camera_top,
    asset_runtime::LegacyActionDrawPorts &action_ports) {
  LegacyMovingActionResult result;
  for (auto current = nodes.begin(); current != nodes.end();) {
    ++result.visited_count;
    const compat::i32 screen_x = wrapping_subtract(
        truncate_x87_float_to_low_i32(current->position_x), camera_left);
    const compat::i32 screen_y = wrapping_subtract(
        truncate_x87_float_to_low_i32(current->position_y), camera_top);

    if (action_ports.update_action_record(current->action) !=
        asset_runtime::LegacyActionUpdateStatus::completed) {
      ++result.action_update_failure_count;
    }

    if (screen_x > -72 && screen_x < 712 && screen_y > -72 && screen_y < 552) {
      rendering::LegacyFramePiece piece;
      ++result.frame_request_count;
      if (!action_ports.load_frame_piece(current->action.field_4a,
                                         current->action.field_4c, piece)) {
        ++result.frame_failure_count;
      } else {
        result.last_blit_status = action_ports.draw_frame_piece(
            piece, wrapping_subtract(screen_x, current->action.draw_offset_x),
            wrapping_subtract(screen_y, current->action.draw_offset_y),
            current->action.mode_flags,
            static_cast<compat::i32>(current->action.field_8a));
        ++result.draw_count;
        if (!accepted_blit_status(result.last_blit_status)) {
          ++result.blit_failure_count;
        }
      }
    }

    if (current->action.wait_remaining != 0U) {
      ++current;
      continue;
    }

    current->position_x = current->position_x + current->velocity_x;
    current->position_y = current->position_y + current->velocity_y;
    const compat::i32 position_x =
        truncate_x87_float_to_low_i32(current->position_x);
    const compat::i32 position_y =
        truncate_x87_float_to_low_i32(current->position_y);
    const compat::i32 target_x = static_cast<compat::i32>(current->target_x);
    const compat::i32 target_y = static_cast<compat::i32>(current->target_y);
    if (position_x > target_x - 32 && position_x < target_x + 32 &&
        position_y > target_y - 32 && position_y < target_y + 32) {
      current = nodes.erase(current);
      ++result.removed_count;
    } else {
      ++current;
    }
  }
  return result;
}

} // namespace openswd3::world_map
