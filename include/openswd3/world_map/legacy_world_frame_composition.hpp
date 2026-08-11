#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/world_map/legacy_world_background.hpp"

namespace openswd3::world_map {

inline constexpr compat::u8 kLegacyWorldFrameClearOnly = 0x01U;
inline constexpr compat::u8 kLegacyWorldFramePartialRefresh = 0x04U;
inline constexpr compat::u16 kLegacyWorldNoTalkTarget = 0xFFFFU;

enum class LegacyWorldFrameStage : compat::u8 {
  ani_activity_004154a0,
  pre_background_records_004151f0,
  flagged_spatial_objects_00413ea0,
  world_spatial_objects_00413870,
  primary_picture_actions_004147e0,
  moving_action_sprites_00414b60,
  ani_drift_004161c0,
  ani_streak_00416590,
  ani_spark_004167b0,
  ani_directional_00415b70,
  ani_row_copy_004163c0,
  framebuffer_deformation_00416cc0,
  ani_follower_00416b30,
  secondary_picture_actions_004147e0,
  packed_row_effects_00414e50,
  timed_ui_update_0042ed40,
  role_head_sprites_00414ce0,
  world_indicator_004149b0,
  frame_color_update_004146f0,
  timed_messages_004153d0,
};

struct LegacyWorldFrameState {
  compat::u8 runtime_flags{};
  bool ani_activity_active{};
  compat::i32 camera_left{};
  compat::i32 camera_top{};
  compat::i32 camera_right{};
  compat::i32 camera_bottom{};
  compat::i32 partial_focus_x{};
  compat::i32 partial_focus_y{};
  compat::u16 talk_target{kLegacyWorldNoTalkTarget};
  compat::u16 talk_phase{};
  compat::u32 decorated_value{};
};

class LegacyWorldFramePorts {
public:
  virtual ~LegacyWorldFramePorts() = default;

  [[nodiscard]] virtual bool query_service(compat::u32 service_id) noexcept = 0;
  [[nodiscard]] virtual bool
  query_control(compat::u32 control_index) noexcept = 0;
  [[nodiscard]] virtual bool
  execute_stage(LegacyWorldFrameStage stage) noexcept = 0;
  virtual void draw_decorated_number(compat::i32 right, compat::i32 bottom,
                                     compat::u32 style,
                                     compat::u32 value) noexcept = 0;
};

enum class LegacyWorldFramePath : compat::u8 {
  normal,
  clear_only,
  ani_activity,
};

enum class LegacyWorldFrameCompositionStatus : compat::u8 {
  completed,
  invalid_framebuffer,
  background_failed,
  stage_failed,
};

struct LegacyWorldFrameCompositionResult {
  LegacyWorldFrameCompositionStatus status{
      LegacyWorldFrameCompositionStatus::invalid_framebuffer};
  LegacyWorldFramePath path{LegacyWorldFramePath::normal};
  LegacyWorldBackgroundRenderResult background{};
  compat::u32 clear_pass_count{};
  compat::u32 clip_update_count{};
  compat::u32 service_query_count{};
  compat::u32 control_query_count{};
  compat::u32 stage_call_count{};
  bool background_attempted{};
  bool decorated_number_drawn{};
  bool world_indicator_updated{};
  bool stage_failure_recorded{};
  LegacyWorldFrameStage failed_stage{
      LegacyWorldFrameStage::ani_activity_004154a0};
};

// Ordinary-world software-frame composition at 0x00412930. Presentation is
// deliberately outside this boundary: the only ordinary-world present call
// remains in 0x004120B0 after this coordinator returns.
[[nodiscard]] LegacyWorldFrameCompositionResult
compose_legacy_world_frame(rendering::LegacyFramebuffer &framebuffer,
                           rendering::LegacyRasterGeometryState &raster,
                           const LegacyWorldBackgroundSource &background_source,
                           const LegacyWorldFrameState &state,
                           LegacyWorldFramePorts &ports) noexcept;

} // namespace openswd3::world_map
