#include "openswd3/story_scene/legacy_dialog_geometry.hpp"

#include <bit>

namespace openswd3::story_scene {
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

[[nodiscard]] constexpr i32 wrapping_multiply(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] constexpr i32 zero_extend(const u16 value) noexcept {
  return static_cast<i32>(value);
}

[[nodiscard]] constexpr i32 signed_quarter(const i32 value) noexcept {
  return value / 4;
}

[[nodiscard]] constexpr i32 unsigned_quarter(const u16 value) noexcept {
  return static_cast<i32>(static_cast<u32>(value) >> 2U);
}

void advance_four_step_transition(LegacyDialogRecord32 &record,
                                  bool &transition_in_progress) noexcept {
  record.transition_step = static_cast<u16>(record.transition_step + 1U);
  if (record.transition_step > 4U) {
    record.transition_step = 4U;
    transition_in_progress = false;
  }
}

} // namespace

LegacyDialogGeometryResult prepare_legacy_dialog_geometry(
    LegacyDialogRecord32 &record,
    const LegacyDialogAnchorInput &input) noexcept {
  LegacyDialogGeometryResult result;
  const i32 left = zero_extend(record.left);
  const i32 top = zero_extend(record.top);
  const i32 width = zero_extend(record.width);
  const i32 height = zero_extend(record.height);
  const i32 twice_scale = wrapping_add(input.scale, input.scale);

  i32 panel_left{};
  i32 panel_top{};
  i32 panel_right{};
  i32 panel_bottom{};

  if ((record.flags & kLegacyDialogFlagDirectRectangle) != 0U) {
    result.transition_in_progress = false;
    if ((record.flags & kLegacyDialogFlagAlternateDirectTransition) != 0U) {
      if (record.display_counter < 8U) {
        result.opacity_step = wrapping_multiply(
            zero_extend(record.display_counter), 2);
      }
    } else {
      advance_four_step_transition(record, result.transition_in_progress);
      result.opacity_step = wrapping_multiply(
          zero_extend(record.transition_step), 4);
    }

    panel_left = left;
    panel_top = top;
    panel_right = wrapping_add(wrapping_add(left, width), twice_scale);
    panel_bottom = wrapping_add(top, height);
  } else {
    advance_four_step_transition(record, result.transition_in_progress);
    const i32 step = zero_extend(record.transition_step);

    if ((record.flags & kLegacyDialogFlagExplicitAnchor) != 0U) {
      const i32 anchor_left = zero_extend(record.anchor_left);
      const i32 anchor_top = zero_extend(record.anchor_top);
      panel_left = wrapping_add(
          anchor_left,
          wrapping_multiply(
              step, signed_quarter(wrapping_subtract(left, anchor_left))));
      panel_top = wrapping_add(
          wrapping_add(
              anchor_top,
              wrapping_multiply(
                  step,
                  signed_quarter(wrapping_subtract(top, anchor_top)))),
          -4);
    } else {
      i32 anchor_left{};
      i32 anchor_top{};
      if (record.role_index == 0xFFFDU) {
        anchor_left = zero_extend(record.anchor_left);
        anchor_top = zero_extend(record.anchor_top);
      } else {
        if (!input.role_anchor_available) {
          result.status = LegacyDialogGeometryStatus::role_anchor_unavailable;
          return result;
        }
        anchor_left = input.role_world_x;
        anchor_top = input.role_world_y;
      }

      panel_left = wrapping_subtract(
          wrapping_add(
              anchor_left,
              wrapping_multiply(
                  step,
                  signed_quarter(wrapping_subtract(
                      wrapping_add(input.camera_left, left),
                      anchor_left)))),
          input.camera_left);
      panel_top = wrapping_add(
          wrapping_subtract(
              wrapping_add(
                  anchor_top,
                  wrapping_multiply(
                      step,
                      signed_quarter(wrapping_subtract(
                          wrapping_add(input.camera_top, top),
                          anchor_top)))),
              input.camera_top),
          -4);
    }

    panel_right = wrapping_add(
        panel_left,
        wrapping_multiply(
            step,
            signed_quarter(wrapping_add(width, twice_scale))));
    panel_bottom = wrapping_add(
        wrapping_add(panel_top, 8),
        wrapping_multiply(step, unsigned_quarter(record.height)));
  }

  result.opacity_step &= 0x0F;
  result.panel = LegacyDialogRectangle{
      .left = panel_left,
      .top = panel_top,
      .right = panel_right,
      .bottom = panel_bottom,
  };
  result.panel_draw_requested =
      (record.flags & kLegacyDialogFlagSuppressPanel) == 0U;
  if (!result.panel_draw_requested) {
    result.transition_in_progress = false;
  }

  // The original calls sub_416FF0(left, top, left + right, top + bottom)
  // after the optional panel draw. This unusual double-origin expansion is
  // preserved verbatim rather than normalized to the panel rectangle.
  result.text_clip = LegacyDialogRectangle{
      .left = panel_left,
      .top = panel_top,
      .right = wrapping_add(panel_left, panel_right),
      .bottom = wrapping_add(panel_top, panel_bottom),
  };
  return result;
}

} // namespace openswd3::story_scene
