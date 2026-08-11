#include "openswd3/world_map/legacy_world_debug_overlay.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdio>
#include <limits>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::array<u32, 5U> kMarkerMasks{
    0x40000000U, 0x10000000U, 0x000000FFU, 0x20000000U, 0x00800000U,
};
constexpr std::array<i32, 5U> kMarkerInsets{0, 2, 4, 6, 8};
constexpr i32 kMarkerColumns = 38;
constexpr i32 kMarkerRows = 28;

constexpr char kRoleOverlap[] = "\xA8\xA4\xA6\xE2\xAD\xAB\xC5\x7C";
constexpr char kRoleSummaryFormat[] =
    "\xA8\xA4\xA6\xE2 [GUID %d] [act %d] [qq %d] [dir %d]";
constexpr char kEventDetailFormat[] =
    "\xBC\x40\xB1\xA1\xB8\xB9\xBD\x58[%d] [%d,%s] [%d,%s]";
constexpr char kEventErrorFormat[] =
    "\xBF\xF9\xBB\x7E\xAA\xBA\xA6\x61\xB9\xCF\xBC\x40\xB1\xA1"
    "\xB8\xB9\xBD\x58 (%d)";
constexpr char kSelectable[] = "\xA5\x69\xC2\x49\xBF\xEF";
constexpr char kNotSelectable[] = "\xA4\xA3\xA5\x69\xC2\x49\xBF\xEF";
constexpr char kPassable[] = "\xA5\x69\xB3\x71\xB9\x4C";
constexpr char kNotPassable[] = "\xA4\xA3\xA5\x69\xB3\x71\xB9\x4C";
constexpr char kScene[] = "\xB3\xF5\xB4\xBA";
constexpr char kStory[] = "\xBC\x40\xB1\xA1";
constexpr char kNil[] = "Nil";
constexpr char kCycle[] = "Cyc2";
constexpr char kRepeat[] = "Rep1";

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
  return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
  return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(const i32 left,
                                         const i32 right) noexcept {
  return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr u32 wrapping_add(const u32 left,
                                         const u32 right) noexcept {
  return left + right;
}

[[nodiscard]] constexpr i32 arithmetic_shift_right_4(const i32 value) noexcept {
  u32 shifted = to_bits(value) >> 4U;
  if ((to_bits(value) & 0x80000000U) != 0U) {
    shifted |= 0xF0000000U;
  }
  return from_bits(shifted);
}

[[nodiscard]] constexpr i32 floor_grid_coordinate(const i32 value) noexcept {
  return arithmetic_shift_right_4(wrapping_add(value, 16));
}

[[nodiscard]] bool read_cell_flags(const std::span<const u8> bytes,
                                   const std::size_t cell_index,
                                   u32 &value) noexcept {
  if (cell_index > std::numeric_limits<std::size_t>::max() / 4U) {
    return false;
  }
  const std::size_t offset = cell_index * 4U;
  if (offset > bytes.size() || bytes.size() - offset < 4U) {
    return false;
  }
  value = static_cast<u32>(bytes[offset]) |
          (static_cast<u32>(bytes[offset + 1U]) << 8U) |
          (static_cast<u32>(bytes[offset + 2U]) << 16U) |
          (static_cast<u32>(bytes[offset + 3U]) << 24U);
  return true;
}

[[nodiscard]] bool valid_marker_framebuffer(
    const rendering::LegacyFramebuffer &framebuffer) noexcept {
  const auto &surface = framebuffer.geometry().surface;
  return surface.width >= rendering::kLegacyFramebufferWidth &&
         surface.height >= rendering::kLegacyFramebufferHeight &&
         surface.pitch_bytes >= rendering::kLegacyFramebufferPitchBytes;
}

[[nodiscard]] bool
draw_marker_rectangle(rendering::LegacyFramebuffer &framebuffer, const i32 x,
                      const i32 y, const i32 extent, const u16 color,
                      LegacyWorldDebugOverlayResult &result) noexcept {
  const i32 horizontal_pairs = extent / 2;
  const i32 right = x + horizontal_pairs * 2;
  const i32 bottom = y + extent;
  const auto &surface = framebuffer.geometry().surface;
  if (x < 0 || y < 0 || right >= surface.width || bottom >= surface.height) {
    return false;
  }

  auto top_row = framebuffer.row_pixels(static_cast<u32>(y));
  auto bottom_row = framebuffer.row_pixels(static_cast<u32>(bottom));
  for (i32 pair = 0; pair < horizontal_pairs; ++pair) {
    const auto offset = static_cast<std::size_t>(x + pair * 2);
    top_row[offset] = color;
    top_row[offset + 1U] = color;
    bottom_row[offset] = color;
    bottom_row[offset + 1U] = color;
    result.marker_pixel_writes += 4U;
  }
  for (i32 row_offset = 0; row_offset < extent; ++row_offset) {
    auto row = framebuffer.row_pixels(static_cast<u32>(y + row_offset));
    row[static_cast<std::size_t>(x)] = color;
    row[static_cast<std::size_t>(right)] = color;
    result.marker_pixel_writes += 2U;
  }
  ++result.marker_rectangles_drawn;
  return true;
}

[[nodiscard]] LegacyWorldDebugOverlayStatus
draw_collision_grid(rendering::LegacyFramebuffer &framebuffer,
                    const LegacyWorldBackgroundSource &background,
                    const i32 camera_left, const i32 camera_top,
                    const rendering::LegacyPixelConversionState &pixel_format,
                    LegacyWorldDebugOverlayResult &result) noexcept {
  if (!valid_marker_framebuffer(framebuffer)) {
    return LegacyWorldDebugOverlayStatus::invalid_framebuffer;
  }
  if (background.map_width == 0U || background.map_height == 0U) {
    return LegacyWorldDebugOverlayStatus::invalid_map_geometry;
  }
  const auto &masks = pixel_format.effective_masks;
  const std::array<u16, 5U> colors{
      static_cast<u16>(masks.red + masks.green + masks.blue),
      static_cast<u16>(masks.green + masks.blue),
      static_cast<u16>(masks.red + masks.green),
      static_cast<u16>(masks.red),
      static_cast<u16>(masks.red + masks.blue),
  };
  const i32 start_x = floor_grid_coordinate(camera_left);
  const i32 start_y = floor_grid_coordinate(camera_top);
  const i32 screen_x_origin = -(from_bits(to_bits(camera_left) & 0x0FU));
  const i32 screen_y_origin = -(from_bits(to_bits(camera_top) & 0x0FU));

  for (std::size_t pass = 0U; pass < kMarkerMasks.size(); ++pass) {
    for (i32 row = 0; row < kMarkerRows; ++row) {
      const i32 cell_y = start_y + row;
      for (i32 column = 0; column < kMarkerColumns; ++column) {
        const i32 cell_x = start_x + column;
        if (cell_x < 0 || cell_y < 0 ||
            static_cast<u32>(cell_x) >= background.map_width ||
            static_cast<u32>(cell_y) >= background.map_height) {
          return LegacyWorldDebugOverlayStatus::cell_grid_out_of_bounds;
        }
        const std::size_t cell_index =
            static_cast<std::size_t>(cell_y) * background.map_width +
            static_cast<u32>(cell_x);
        u32 flags{};
        if (!read_cell_flags(background.cell_flags, cell_index, flags)) {
          return LegacyWorldDebugOverlayStatus::cell_grid_out_of_bounds;
        }
        ++result.marker_cells_visited;
        if ((flags & kMarkerMasks[pass]) == 0U) {
          continue;
        }
        const i32 inset = kMarkerInsets[pass];
        const i32 x = screen_x_origin + 16 + column * 16 + inset;
        const i32 y = screen_y_origin + 16 + row * 16 + inset;
        if (!draw_marker_rectangle(framebuffer, x, y, 15 - inset, colors[pass],
                                   result)) {
          return LegacyWorldDebugOverlayStatus::invalid_framebuffer;
        }
      }
    }
  }
  return LegacyWorldDebugOverlayStatus::completed;
}

void record_text_result(const rendering::LegacyTextDrawResult draw,
                        LegacyWorldDebugOverlayResult &result) noexcept {
  ++result.text_draw_calls;
  if (draw.status != rendering::LegacyTextDrawStatus::completed) {
    ++result.text_draw_failures;
  }
}

void draw_text(LegacyWorldDebugOverlayPorts &ports,
               const std::span<const u8> bytes, const i32 y, const u16 color,
               LegacyWorldDebugOverlayResult &result) noexcept {
  record_text_result(ports.draw_debug_text(rendering::LegacyTextDrawRequest{
                         .destination_x = 8,
                         .destination_y = y,
                         .nul_terminated_text = bytes,
                         .foreground_color = color,
                         .flags = 0x10U,
                     }),
                     result);
}

template <typename... Arguments>
[[nodiscard]] bool
format_and_draw(LegacyWorldDebugOverlayPorts &ports, const i32 y,
                const u16 color, LegacyWorldDebugOverlayResult &result,
                const char *format, Arguments... arguments) noexcept {
  std::array<char, 256U> buffer{};
  const int written =
      std::snprintf(buffer.data(), buffer.size(), format, arguments...);
  if (written < 0 || static_cast<std::size_t>(written) >= buffer.size()) {
    if (result.status == LegacyWorldDebugOverlayStatus::completed) {
      result.status = LegacyWorldDebugOverlayStatus::text_format_overflow;
    }
    return false;
  }
  draw_text(ports,
            std::span<const u8>{reinterpret_cast<const u8 *>(buffer.data()),
                                static_cast<std::size_t>(written) + 1U},
            y, color, result);
  return true;
}

[[nodiscard]] const char *cycle_mode(const u32 flags, const u32 enable_mask,
                                     const u32 repeat_mask) noexcept {
  if ((flags & enable_mask) == 0U) {
    return kNil;
  }
  return (flags & repeat_mask) != 0U ? kRepeat : kCycle;
}

[[nodiscard]] const LegacyWorldMapEvent *
find_event(const std::span<const LegacyWorldMapEvent> events,
           const u32 event_id) noexcept {
  for (const auto &event : events) {
    if (event.field_04 == event_id) {
      return &event;
    }
  }
  return nullptr;
}

[[nodiscard]] bool
in_role_debug_rectangle(const u32 mouse_x, const u32 mouse_y,
                        const LegacyWorldRoleRecord &role) noexcept {
  return mouse_x > role.world_x - 0x10U && mouse_x < role.world_x + 0x30U &&
         mouse_y > role.world_y - 0x40U && mouse_y < role.world_y + 0x10U;
}

void draw_event_debug(const LegacyWorldBackgroundSource &background,
                      const std::span<const LegacyWorldMapEvent> events,
                      const i32 camera_left, const i32 camera_top,
                      const LegacyWorldDebugOverlayState &state,
                      LegacyWorldDebugOverlayPorts &ports,
                      LegacyWorldDebugOverlayResult &result) noexcept {
  const u32 world_x = wrapping_add(to_bits(camera_left), state.mouse_screen_x);
  const u32 world_y = wrapping_add(to_bits(camera_top), state.mouse_screen_y);
  const u32 cell_x = world_x >> 4U;
  const u32 cell_y = world_y >> 4U;
  if (cell_x >= background.map_width || cell_y >= background.map_height) {
    if (result.status == LegacyWorldDebugOverlayStatus::completed) {
      result.status = LegacyWorldDebugOverlayStatus::cell_grid_out_of_bounds;
    }
    return;
  }
  const std::size_t cell_index =
      static_cast<std::size_t>(cell_y) * background.map_width + cell_x;
  u32 flags{};
  if (!read_cell_flags(background.cell_flags, cell_index, flags)) {
    if (result.status == LegacyWorldDebugOverlayStatus::completed) {
      result.status = LegacyWorldDebugOverlayStatus::cell_grid_out_of_bounds;
    }
    return;
  }
  result.event_id = flags & 0xFFU;
  if (result.event_id == 0U) {
    return;
  }

  const LegacyWorldMapEvent *const event = find_event(events, result.event_id);
  if (event == nullptr) {
    static_cast<void>(format_and_draw(ports, 0x1B0, state.text_color, result,
                                      kEventErrorFormat,
                                      from_bits(result.event_id)));
    return;
  }
  result.event_found = true;
  if (!event->name_bytes_with_terminator.empty()) {
    draw_text(ports, event->name_bytes_with_terminator, 0x1C0, state.text_color,
              result);
  }
  const u32 high_flag = event->field_0c >> 16U;
  const char *const high_text =
      ports.query_debug_flag(high_flag) ? kNotPassable : kPassable;
  const u32 low_flag = event->field_0c & 0xFFFFU;
  const char *const low_text =
      ports.query_debug_flag(low_flag) ? kSelectable : kNotSelectable;
  static_cast<void>(format_and_draw(
      ports, 0x1D0, state.text_color, result, kEventDetailFormat,
      from_bits(event->field_08 & 0x7FFFU), from_bits(low_flag), low_text,
      from_bits(high_flag), high_text));
}

void draw_nearby_role_debug(const std::span<const LegacyWorldRoleRecord> roles,
                            const i32 camera_left, const i32 camera_top,
                            const LegacyWorldDebugOverlayState &state,
                            LegacyWorldDebugOverlayPorts &ports,
                            LegacyWorldDebugOverlayResult &result) noexcept {
  const u32 mouse_world_x =
      wrapping_add(state.mouse_screen_x, to_bits(camera_left));
  const u32 mouse_world_y =
      wrapping_add(state.mouse_screen_y, to_bits(camera_top));
  for (std::size_t index = 1U; index < roles.size(); ++index) {
    const LegacyWorldRoleRecord &role = roles[index];
    if (role.action.action_id == 0U ||
        !in_role_debug_rectangle(mouse_world_x, mouse_world_y, role)) {
      continue;
    }
    ++result.nearby_roles;
    if (result.nearby_roles >= 2U) {
      draw_text(ports,
                std::span<const u8>{reinterpret_cast<const u8 *>(kRoleOverlap),
                                    sizeof(kRoleOverlap)},
                0x190, state.text_color, result);
    }
    static_cast<void>(format_and_draw(
        ports, 0x1A0, state.role_text_color, result, kRoleSummaryFormat,
        static_cast<i32>(role.guid), from_bits(role.action.action_id),
        from_bits(role.action.base_variant),
        from_bits(role.action.variant_delta)));
    static_cast<void>(format_and_draw(
        ports, 0x1B0, state.text_color, result,
        "[GUID %d] [Talk %d] [Path %d] [Argu %4x] [ArrIdx %d]",
        static_cast<i32>(role.guid), static_cast<i32>(role.talk_script_id),
        static_cast<i32>(role.path_data_id), role.flags,
        static_cast<i32>(index)));
  }
}

void draw_diagnostic_text(const LegacyWorldBackgroundSource &background,
                          const std::span<const LegacyWorldMapEvent> events,
                          const std::span<const LegacyWorldRoleRecord> roles,
                          const i32 camera_left, const i32 camera_top,
                          const LegacyWorldDebugOverlayState &state,
                          LegacyWorldDebugOverlayPorts &ports,
                          LegacyWorldDebugOverlayResult &result) noexcept {
  if (state.controlled_role_index >= roles.size()) {
    result.status =
        LegacyWorldDebugOverlayStatus::controlled_role_out_of_bounds;
    return;
  }
  if (state.frame_interval_milliseconds == 0U) {
    result.status = LegacyWorldDebugOverlayStatus::zero_frame_interval;
    return;
  }

  const auto &controlled = roles[state.controlled_role_index];
  static_cast<void>(format_and_draw(
      ports, 0, state.text_color, result, "MAct[%4.1f/%4.1f] LTCor[%4d/%4d]",
      static_cast<double>(from_bits(controlled.world_x)) * 0.0625,
      static_cast<double>(from_bits(controlled.world_y)) * 0.0625,
      arithmetic_shift_right_4(camera_left),
      arithmetic_shift_right_4(camera_top)));

  const i32 map_mouse_x = wrapping_add(arithmetic_shift_right_4(camera_left),
                                       from_bits(state.mouse_screen_x >> 4U));
  const i32 map_mouse_y = wrapping_add(arithmetic_shift_right_4(camera_top),
                                       from_bits(state.mouse_screen_y >> 4U));
  static_cast<void>(format_and_draw(
      ports, 0x10, state.text_color, result,
      "Mouse :OnMap[%4d/%4d] OnScr[%4d/%4d]", map_mouse_x, map_mouse_y,
      from_bits(state.mouse_screen_x), from_bits(state.mouse_screen_y)));

  static_cast<void>(format_and_draw(
      ports, 0x20, state.text_color, result,
      "FPS[%2d] FUsed(%d),kr.d[%1d] ,kr.p[%4d] _IPA[%4d]",
      from_bits(1000U / state.frame_interval_milliseconds),
      from_bits(state.fps_fused), from_bits(state.fps_keyboard_repeat_delay),
      from_bits(state.fps_keyboard_repeat_period), from_bits(state.fps_ipa)));
  static_cast<void>(format_and_draw(
      ports, 0x30, state.text_color, result, "MapID[%d] MapCyc[%d] %d",
      from_bits(state.map_id), from_bits(state.map_cycle),
      from_bits(state.map_debug_value)));
  static_cast<void>(format_and_draw(
      ports, 0x40, state.text_color, result,
      "ULck[%x] UIpt[%x] gTm[%d] Fsc[%d] ,BtlM[%d] ,BtnIp[%d]",
      state.unlock_value, state.ui_point_value, from_bits(state.game_time),
      from_bits(state.frame_state_counter & 0x7FFFFFFFU),
      from_bits(state.battle_mode), from_bits(state.button_input)));
  static_cast<void>(format_and_draw(
      ports, 0x50, state.text_color, result,
      "T.Gd[%d] T.Tid[%d] dbg1[%d] dbg2[%d]",
      from_bits(state.talk_guid & 0xFFFFU), from_bits(state.talk_id & 0xFFFFU),
      from_bits(state.debug_value_1), from_bits(state.debug_value_2)));

  const char *const scene_mode =
      cycle_mode(state.mode_flags, 0x00080000U, 0x00040000U);
  const char *const story_mode =
      cycle_mode(state.mode_flags, 0x00020000U, 0x00010000U);
  const char *const active_mode =
      (state.mode_flags & 0x00800000U) != 0U ? kStory : kScene;
  static_cast<void>(format_and_draw(
      ports, 0x60, state.text_color, result,
      "[%d/%d][%d/%d][playing/%d][mode/%s],"
      "\xB3\xF5\xB4\xBA%s,\xBC\x40\xB1\xA1%s",
      from_bits(state.scene_position), from_bits(state.scene_limit),
      from_bits(state.story_position), from_bits(state.story_limit),
      from_bits(state.playing_value & 0xFFU), active_mode, scene_mode,
      story_mode));

  draw_event_debug(background, events, camera_left, camera_top, state, ports,
                   result);
  draw_nearby_role_debug(roles, camera_left, camera_top, state, ports, result);
}

} // namespace

LegacyWorldDebugOverlayResult draw_legacy_world_debug_overlay(
    rendering::LegacyFramebuffer &framebuffer,
    const LegacyWorldBackgroundSource &background,
    const std::span<const LegacyWorldMapEvent> events,
    const std::span<const LegacyWorldRoleRecord> roles, const i32 camera_left,
    const i32 camera_top,
    const rendering::LegacyPixelConversionState &pixel_format,
    const LegacyWorldDebugOverlayState &state,
    LegacyWorldDebugOverlayPorts &ports) noexcept {
  LegacyWorldDebugOverlayResult result;
  ports.configure_debug_text(0xFFFEU, 0U);
  result.text_style_configured = true;

  if (state.collision_grid_visible == 1U) {
    result.collision_grid_evaluated = true;
    result.status = draw_collision_grid(framebuffer, background, camera_left,
                                        camera_top, pixel_format, result);
    if (result.status != LegacyWorldDebugOverlayStatus::completed) {
      return result;
    }
  }
  if (state.diagnostic_text_visible == 1U) {
    result.diagnostic_text_evaluated = true;
    draw_diagnostic_text(background, events, roles, camera_left, camera_top,
                         state, ports, result);
  }
  return result;
}

} // namespace openswd3::world_map
