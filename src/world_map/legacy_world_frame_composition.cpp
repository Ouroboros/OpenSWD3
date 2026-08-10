#include "openswd3/world_map/legacy_world_frame_composition.hpp"

#include <algorithm>
#include <bit>

namespace openswd3::world_map {
namespace {

using compat::i32;
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

[[nodiscard]] bool
valid_framebuffer(const rendering::LegacyFramebuffer &framebuffer,
                  const rendering::LegacyRasterGeometryState &raster) noexcept {
  const auto &surface = framebuffer.geometry().surface;
  return surface.width == rendering::kLegacyFramebufferWidth &&
         surface.height == rendering::kLegacyFramebufferHeight &&
         surface.pitch_bytes >= rendering::kLegacyFramebufferPitchBytes &&
         raster.surface.width == surface.width &&
         raster.surface.height == surface.height &&
         raster.surface.pitch_bytes == surface.pitch_bytes &&
         framebuffer.physical_pixels().size() >=
             rendering::kLegacyFixedCanvasPixels;
}

void set_full_clip(rendering::LegacyRasterGeometryState &raster,
                   LegacyWorldFrameCompositionResult &result) noexcept {
  rendering::set_legacy_clip_rectangle(raster, 0, 0,
                                       rendering::kLegacyFramebufferWidth,
                                       rendering::kLegacyFramebufferHeight);
  ++result.clip_update_count;
}

void set_partial_clip(rendering::LegacyRasterGeometryState &raster,
                      const LegacyWorldFrameState &state,
                      LegacyWorldFrameCompositionResult &result) noexcept {
  rendering::set_legacy_clip_rectangle(
      raster, wrapping_subtract(state.partial_focus_x, 0xC0),
      wrapping_subtract(state.partial_focus_y, 0xC0),
      wrapping_add(state.partial_focus_x, 0xC0),
      wrapping_add(state.partial_focus_y, 0xC0));
  ++result.clip_update_count;
}

void clear_fixed_canvas(rendering::LegacyFramebuffer &framebuffer,
                        LegacyWorldFrameCompositionResult &result) noexcept {
  std::ranges::fill(
      framebuffer.physical_pixels().first(rendering::kLegacyFixedCanvasPixels),
      compat::u16{});
  ++result.clear_pass_count;
}

[[nodiscard]] bool query_service(LegacyWorldFramePorts &ports,
                                 LegacyWorldFrameCompositionResult &result,
                                 const u32 service_id) noexcept {
  ++result.service_query_count;
  return ports.query_service(service_id);
}

[[nodiscard]] bool query_control(LegacyWorldFramePorts &ports,
                                 LegacyWorldFrameCompositionResult &result,
                                 const u32 control_index) noexcept {
  ++result.control_query_count;
  return ports.query_control(control_index);
}

[[nodiscard]] bool execute_stage(LegacyWorldFramePorts &ports,
                                 LegacyWorldFrameCompositionResult &result,
                                 const LegacyWorldFrameStage stage) noexcept {
  ++result.stage_call_count;
  if (ports.execute_stage(stage)) {
    return true;
  }
  result.status = LegacyWorldFrameCompositionStatus::stage_failed;
  result.stage_failure_recorded = true;
  result.failed_stage = stage;
  return false;
}

[[nodiscard]] bool
execute_common_tail(const LegacyWorldFrameState &state,
                    LegacyWorldFramePorts &ports,
                    LegacyWorldFrameCompositionResult &result) noexcept {
  if (!execute_stage(ports, result,
                     LegacyWorldFrameStage::packed_row_effects_00414e50) ||
      !execute_stage(ports, result,
                     LegacyWorldFrameStage::timed_ui_update_0042ed40) ||
      !execute_stage(ports, result,
                     LegacyWorldFrameStage::role_head_sprites_00414ce0)) {
    return false;
  }

  if ((state.talk_target == kLegacyWorldNoTalkTarget ||
       state.talk_phase < 8U) &&
      !query_service(ports, result, 0x51U)) {
    ports.draw_decorated_number(0x27C, 0x1CC, 0U, state.decorated_value);
    result.decorated_number_drawn = true;
  }

  bool indicator_requires_control = false;
  if (query_service(ports, result, 0x0AU)) {
    indicator_requires_control = true;
  } else if (query_service(ports, result, 0x09U)) {
    indicator_requires_control = true;
  } else if (query_service(ports, result, 0x51U)) {
    indicator_requires_control = true;
  }

  if (!indicator_requires_control || query_control(ports, result, 0x2EU)) {
    if (!execute_stage(ports, result,
                       LegacyWorldFrameStage::world_indicator_004149b0)) {
      return false;
    }
    result.world_indicator_updated = true;
  }

  return execute_stage(ports, result,
                       LegacyWorldFrameStage::frame_color_update_004146f0) &&
         execute_stage(ports, result,
                       LegacyWorldFrameStage::timed_messages_004153d0);
}

} // namespace

LegacyWorldFrameCompositionResult
compose_legacy_world_frame(rendering::LegacyFramebuffer &framebuffer,
                           rendering::LegacyRasterGeometryState &raster,
                           const LegacyWorldBackgroundSource &background_source,
                           const LegacyWorldFrameState &state,
                           LegacyWorldFramePorts &ports) noexcept {
  LegacyWorldFrameCompositionResult result;
  if (!valid_framebuffer(framebuffer, raster)) {
    return result;
  }

  set_full_clip(raster, result);
  if ((state.runtime_flags & kLegacyWorldFramePartialRefresh) != 0U) {
    set_partial_clip(raster, state, result);
  }

  if (state.ani_activity_active) {
    result.path = LegacyWorldFramePath::ani_activity;
    if (!execute_stage(ports, result,
                       LegacyWorldFrameStage::ani_activity_004154a0)) {
      set_full_clip(raster, result);
      return result;
    }
  } else {
    const bool clear_for_service = query_service(ports, result, 0x0FU) ||
                                   query_service(ports, result, 0x13U);
    if (clear_for_service) {
      clear_fixed_canvas(framebuffer, result);
    }

    if ((state.runtime_flags & kLegacyWorldFrameClearOnly) != 0U) {
      result.path = LegacyWorldFramePath::clear_only;
      clear_fixed_canvas(framebuffer, result);
      if (!execute_stage(
              ports, result,
              LegacyWorldFrameStage::secondary_picture_actions_004147e0)) {
        set_full_clip(raster, result);
        return result;
      }
    } else {
      if (!execute_stage(
              ports, result,
              LegacyWorldFrameStage::pre_background_records_004151f0)) {
        set_full_clip(raster, result);
        return result;
      }

      if (!query_service(ports, result, 0x48U) &&
          query_service(ports, result, 0x13U)) {
        set_partial_clip(raster, state, result);
      }

      const bool partial_background_refresh =
          !query_service(ports, result, 0x48U) &&
          query_service(ports, result, 0x13U);
      result.background_attempted = true;
      result.background = render_legacy_world_background(
          framebuffer, background_source,
          LegacyWorldBackgroundView{
              .camera_left = state.camera_left,
              .camera_top = state.camera_top,
              .partial_refresh = partial_background_refresh,
              .partial_focus_x = state.partial_focus_x,
              .partial_focus_y = state.partial_focus_y,
          });
      if (result.background.status !=
          LegacyWorldBackgroundRenderStatus::completed) {
        set_full_clip(raster, result);
        result.status = LegacyWorldFrameCompositionStatus::background_failed;
        return result;
      }

      if (!query_service(ports, result, 0x0BU)) {
        if (!execute_stage(
                ports, result,
                LegacyWorldFrameStage::flagged_spatial_objects_00413ea0)) {
          set_full_clip(raster, result);
          return result;
        }
      }
      if (!execute_stage(
              ports, result,
              LegacyWorldFrameStage::world_spatial_objects_00413870) ||
          !execute_stage(
              ports, result,
              LegacyWorldFrameStage::primary_picture_actions_004147e0) ||
          !execute_stage(
              ports, result,
              LegacyWorldFrameStage::moving_action_sprites_00414b60)) {
        set_full_clip(raster, result);
        return result;
      }

      if (!query_service(ports, result, 0x48U) &&
          (state.runtime_flags & kLegacyWorldFrameClearOnly) == 0U) {
        if (!execute_stage(ports, result,
                           LegacyWorldFrameStage::ani_drift_004161c0) ||
            !execute_stage(ports, result,
                           LegacyWorldFrameStage::ani_streak_00416590) ||
            !execute_stage(ports, result,
                           LegacyWorldFrameStage::ani_spark_004167b0) ||
            !execute_stage(ports, result,
                           LegacyWorldFrameStage::ani_directional_00415b70) ||
            !execute_stage(ports, result,
                           LegacyWorldFrameStage::ani_row_copy_004163c0) ||
            !execute_stage(
                ports, result,
                LegacyWorldFrameStage::framebuffer_deformation_00416cc0)) {
          set_full_clip(raster, result);
          return result;
        }
      }

      if (!query_service(ports, result, 0x48U)) {
        if (!execute_stage(ports, result,
                           LegacyWorldFrameStage::ani_follower_00416b30)) {
          set_full_clip(raster, result);
          return result;
        }
      }
      if (!execute_stage(
              ports, result,
              LegacyWorldFrameStage::secondary_picture_actions_004147e0)) {
        set_full_clip(raster, result);
        return result;
      }
    }
  }

  set_full_clip(raster, result);
  if (!execute_common_tail(state, ports, result)) {
    set_full_clip(raster, result);
    return result;
  }
  result.status = LegacyWorldFrameCompositionStatus::completed;
  return result;
}

} // namespace openswd3::world_map
