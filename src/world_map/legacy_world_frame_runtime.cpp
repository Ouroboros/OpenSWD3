#include "openswd3/world_map/legacy_world_frame_runtime.hpp"

namespace openswd3::world_map {
namespace {

class RuntimeStagePorts final : public LegacyWorldFramePorts {
public:
  RuntimeStagePorts(const LegacyRoleSpatialIndex &spatial_index,
                    const std::span<LegacyWorldRoleRecord> roles,
                    const LegacyWorldFrameRuntimeState &state,
                    rendering::LegacyRleRowJitterState &jitter,
                    LegacyWorldFrameRuntimePorts ports,
                    LegacyWorldFrameRuntimeResult &result) noexcept
      : spatial_index_(spatial_index), roles_(roles), state_(state),
        jitter_(jitter), ports_(ports), result_(result) {}

  [[nodiscard]] bool
  query_service(const compat::u32 service_id) noexcept override {
    return ports_.remaining_stages.query_service(service_id);
  }

  [[nodiscard]] bool
  query_control(const compat::u32 control_index) noexcept override {
    return ports_.remaining_stages.query_control(control_index);
  }

  [[nodiscard]] bool
  execute_stage(const LegacyWorldFrameStage stage) noexcept override {
    try {
      switch (stage) {
      case LegacyWorldFrameStage::flagged_spatial_objects_00413ea0:
        return execute_flagged_roles(stage);
      case LegacyWorldFrameStage::world_spatial_objects_00413870:
        return execute_world_roles(stage);
      default:
        ++result_.delegated_stage_count;
        if (ports_.remaining_stages.execute_stage(stage)) {
          return true;
        }
        fail(LegacyWorldFrameRuntimeStatus::delegated_stage_failed, stage);
        return false;
      }
    } catch (...) {
      fail(LegacyWorldFrameRuntimeStatus::stage_exception, stage);
      return false;
    }
  }

  void draw_decorated_number(const compat::i32 right, const compat::i32 bottom,
                             const compat::u32 style,
                             const compat::u32 value) noexcept override {
    ports_.remaining_stages.draw_decorated_number(right, bottom, style, value);
  }

private:
  [[nodiscard]] bool execute_flagged_roles(const LegacyWorldFrameStage stage) {
    result_.flagged_stage_executed = true;
    result_.flagged_roles = draw_legacy_world_flagged_roles(
        spatial_index_, std::span<const LegacyWorldRoleRecord>{roles_},
        LegacyWorldRenderCamera{
            .left = state_.frame.camera_left,
            .top = state_.frame.camera_top,
        },
        ports_.flagged_roles);
    if (result_.flagged_roles.status ==
        LegacyWorldFlaggedRolesStatus::completed) {
      return true;
    }
    fail(LegacyWorldFrameRuntimeStatus::flagged_roles_failed, stage);
    return false;
  }

  [[nodiscard]] bool execute_world_roles(const LegacyWorldFrameStage stage) {
    result_.world_roles_stage_executed = true;
    result_.world_roles = draw_legacy_world_roles(
        spatial_index_, roles_,
        LegacyWorldRoleRenderState{
            .camera =
                LegacyWorldRenderCamera{
                    .left = state_.frame.camera_left,
                    .top = state_.frame.camera_top,
                },
            .frame_counter = state_.role_frame_counter,
            .flash_red_offset = state_.flash_red_offset,
            .flash_green_offset = state_.flash_green_offset,
            .flash_blue_offset = state_.flash_blue_offset,
            .talk_target = state_.frame.talk_target,
        },
        state_.spatial_audio, jitter_, ports_.world_roles,
        ports_.spatial_audio);
    if (result_.world_roles.status == LegacyWorldRolesStatus::completed) {
      return true;
    }
    fail(LegacyWorldFrameRuntimeStatus::world_roles_failed, stage);
    return false;
  }

  void fail(const LegacyWorldFrameRuntimeStatus status,
            const LegacyWorldFrameStage stage) noexcept {
    result_.status = status;
    result_.failed_stage_recorded = true;
    result_.failed_stage = stage;
  }

  const LegacyRoleSpatialIndex &spatial_index_;
  std::span<LegacyWorldRoleRecord> roles_;
  const LegacyWorldFrameRuntimeState &state_;
  rendering::LegacyRleRowJitterState &jitter_;
  LegacyWorldFrameRuntimePorts ports_;
  LegacyWorldFrameRuntimeResult &result_;
};

} // namespace

LegacyWorldFrameRuntimeResult compose_legacy_world_runtime_frame(
    rendering::LegacyFramebuffer &framebuffer,
    rendering::LegacyRasterGeometryState &raster,
    const LegacyWorldBackgroundSource &background_source,
    const LegacyRoleSpatialIndex &spatial_index,
    const std::span<LegacyWorldRoleRecord> roles,
    const LegacyWorldFrameRuntimeState &state,
    rendering::LegacyRleRowJitterState &jitter,
    const LegacyWorldFrameRuntimePorts ports) noexcept {
  LegacyWorldFrameRuntimeResult result;
  result.status = LegacyWorldFrameRuntimeStatus::completed;
  RuntimeStagePorts runtime_ports{spatial_index, roles, state,
                                  jitter,        ports, result};
  result.composition = compose_legacy_world_frame(
      framebuffer, raster, background_source, state.frame, runtime_ports);
  if (result.composition.status !=
          LegacyWorldFrameCompositionStatus::completed &&
      result.status == LegacyWorldFrameRuntimeStatus::completed) {
    result.status = LegacyWorldFrameRuntimeStatus::composition_failed;
  }
  return result;
}

} // namespace openswd3::world_map
