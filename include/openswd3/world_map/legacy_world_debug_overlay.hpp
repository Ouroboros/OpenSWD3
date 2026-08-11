#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_text_renderer.hpp"
#include "openswd3/world_map/legacy_world_background.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <span>

namespace openswd3::world_map {

class LegacyWorldDebugOverlayPorts {
public:
  virtual ~LegacyWorldDebugOverlayPorts() = default;

  virtual void configure_debug_text(compat::u16 background_color,
                                    compat::u16 secondary_color) noexcept = 0;
  [[nodiscard]] virtual rendering::LegacyTextDrawResult
  draw_debug_text(const rendering::LegacyTextDrawRequest &request) noexcept = 0;
  [[nodiscard]] virtual bool
  query_debug_flag(compat::u32 flag_index) noexcept = 0;
};

struct LegacyWorldDebugOverlayState {
  // These two switches are toggled by the hidden developer hotkeys in
  // 0x00403211..0x00403252. The outer developer-tools gate remains in the
  // ordinary-world frame coordinator.
  compat::u32 diagnostic_text_visible{};
  compat::u32 collision_grid_visible{};

  compat::u32 controlled_role_index{};
  compat::u32 mouse_screen_x{};
  compat::u32 mouse_screen_y{};
  compat::u32 frame_interval_milliseconds{1U};

  compat::u32 fps_fused{};
  compat::u32 fps_keyboard_repeat_delay{};
  compat::u32 fps_keyboard_repeat_period{};
  compat::u32 fps_ipa{};

  compat::u32 map_id{};
  compat::u32 map_cycle{};
  compat::u32 map_debug_value{};

  compat::u32 unlock_value{};
  compat::u32 ui_point_value{};
  compat::u32 game_time{};
  compat::u32 frame_state_counter{};
  compat::u32 battle_mode{};
  compat::u32 button_input{};

  compat::u32 talk_guid{};
  compat::u32 talk_id{};
  compat::u32 debug_value_1{};
  compat::u32 debug_value_2{};

  compat::u32 scene_position{};
  compat::u32 scene_limit{};
  compat::u32 story_position{};
  compat::u32 story_limit{};
  compat::u32 playing_value{};
  compat::u32 mode_flags{};

  compat::u16 text_color{0xFFFFU};
  compat::u16 role_text_color{0x80FFU};
};

enum class LegacyWorldDebugOverlayStatus : compat::u8 {
  completed,
  invalid_framebuffer,
  invalid_map_geometry,
  cell_grid_out_of_bounds,
  controlled_role_out_of_bounds,
  zero_frame_interval,
  text_format_overflow,
};

struct LegacyWorldDebugOverlayResult {
  LegacyWorldDebugOverlayStatus status{
      LegacyWorldDebugOverlayStatus::completed};
  compat::u32 marker_cells_visited{};
  compat::u32 marker_rectangles_drawn{};
  compat::u32 marker_pixel_writes{};
  compat::u32 text_draw_calls{};
  compat::u32 text_draw_failures{};
  compat::u32 nearby_roles{};
  compat::u32 event_id{};
  bool text_style_configured{};
  bool collision_grid_evaluated{};
  bool diagnostic_text_evaluated{};
  bool event_found{};
};

// Full sub_413FE0 owner. The original function has two parameters: the camera
// left/top values used by the collision-grid screen alignment. Its sole caller
// leaves one additional constant on the stack, but the callee never reads it.
[[nodiscard]] LegacyWorldDebugOverlayResult draw_legacy_world_debug_overlay(
    rendering::LegacyFramebuffer &framebuffer,
    const LegacyWorldBackgroundSource &background,
    std::span<const LegacyWorldMapEvent> events,
    std::span<const LegacyWorldRoleRecord> roles, compat::i32 camera_left,
    compat::i32 camera_top,
    const rendering::LegacyPixelConversionState &pixel_format,
    const LegacyWorldDebugOverlayState &state,
    LegacyWorldDebugOverlayPorts &ports) noexcept;

} // namespace openswd3::world_map
