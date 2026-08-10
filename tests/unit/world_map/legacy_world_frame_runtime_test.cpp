#include "test.hpp"

#include "openswd3/asset_runtime/legacy_act_runtime.hpp"
#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/world_map/legacy_world_frame_runtime.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionDrawRuntimePorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::world_map::compose_legacy_world_runtime_frame;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldFlaggedRoleBit;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldBackgroundPixelLayout;
using openswd3::world_map::LegacyWorldBackgroundSource;
using openswd3::world_map::LegacyWorldFrameCompositionStatus;
using openswd3::world_map::LegacyWorldFramePorts;
using openswd3::world_map::LegacyWorldFrameRuntimePorts;
using openswd3::world_map::LegacyWorldFrameRuntimeState;
using openswd3::world_map::LegacyWorldFrameRuntimeStatus;
using openswd3::world_map::LegacyWorldFrameStage;
using openswd3::world_map::LegacyWorldRoleBlitRequest;
using openswd3::world_map::LegacyWorldRoleExternalPorts;
using openswd3::world_map::LegacyWorldRoleFrame;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleRenderPorts;
using openswd3::world_map::LegacyWorldRoleRenderRuntimePorts;
using openswd3::world_map::LegacyWorldRolesStatus;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;

struct BackgroundFixture {
  u32 width{45U};
  u32 height{40U};
  std::vector<u16> tiles =
      std::vector<u16>(static_cast<std::size_t>(width) * height, 0U);
  std::vector<u8> flags =
      std::vector<u8>(static_cast<std::size_t>(width) * height * 4U, 0U);
  std::vector<u8> pixels = [] {
    std::vector<u8> bytes(0x200U);
    for (std::size_t offset = 0U; offset < bytes.size(); offset += 2U) {
      bytes[offset] = 0x34U;
      bytes[offset + 1U] = 0x12U;
    }
    return bytes;
  }();

  [[nodiscard]] LegacyWorldBackgroundSource source() const noexcept {
    return {
        .map_width = width,
        .map_height = height,
        .tile_indices = tiles,
        .cell_flags = flags,
        .tile_bytes = pixels,
        .pixel_layout = LegacyWorldBackgroundPixelLayout::direct_16,
    };
  }
};

[[nodiscard]] LegacyRoleSpatialIndex make_spatial_index(const u32 height) {
  LegacyRoleSpatialIndex index;
  index.map_height = height;
  const std::size_t rows =
      static_cast<std::size_t>(height) + 2U * kLegacySpatialRowPadding;
  for (auto &group : index.row_heads) {
    group.assign(rows, kLegacySpatialNoRole);
  }
  return index;
}

[[nodiscard]] LegacyWorldRoleRecord make_drawable_role(const u32 x,
                                                       const u32 y) {
  LegacyWorldRoleRecord role{};
  initialize_legacy_action_record(role.action);
  role.flags = 0x00008000U;
  role.world_x = x;
  role.world_y = y;
  role.action.action_id = 1U;
  role.action.field_4a = 1U;
  role.action.field_4c = 0U;
  return role;
}

class RemainingPorts final : public LegacyWorldFramePorts {
public:
  [[nodiscard]] bool query_service(const u32 service_id) noexcept override {
    service_queries.push_back(service_id);
    return service_id < services.size() && services[service_id];
  }

  [[nodiscard]] bool query_control(const u32 control_index) noexcept override {
    control_queries.push_back(control_index);
    return control_index < controls.size() && controls[control_index];
  }

  [[nodiscard]] bool
  execute_stage(const LegacyWorldFrameStage stage) noexcept override {
    stages.push_back(stage);
    return !fail_stage || stage != failed_stage;
  }

  void draw_decorated_number(const i32, const i32, const u32,
                             const u32) noexcept override {
    ++decorated_calls;
  }

  std::array<bool, 256U> services{};
  std::array<bool, 256U> controls{};
  std::vector<u32> service_queries;
  std::vector<u32> control_queries;
  std::vector<LegacyWorldFrameStage> stages;
  LegacyWorldFrameStage failed_stage{
      LegacyWorldFrameStage::pre_background_records_004151f0};
  bool fail_stage{};
  u32 decorated_calls{};
};

class RecordingFlaggedPorts final : public LegacyActionDrawPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &) override {
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    loads.emplace_back(resource_id, frame_index);
    piece.width = 16U;
    piece.height = 16U;
    return true;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32, const i32, const u32,
                   const i32) noexcept override {
    ++draws;
    return LegacyBlitExecutionStatus::completed;
  }

  std::vector<std::pair<u16, u16>> loads;
  u32 draws{};
};

class RecordingRolePorts final : public LegacyWorldRoleRenderPorts {
public:
  [[nodiscard]] bool query_service(const u32) noexcept override {
    return false;
  }

  void play_positional_sample(const u16, const i32,
                              const i32) noexcept override {}

  [[nodiscard]] bool load_frame(const u16 resource_id, const u16 frame_index,
                                LegacyWorldRoleFrame &frame) override {
    loads.emplace_back(resource_id, frame_index);
    frame.width = 16U;
    frame.height = 16U;
    return true;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame(const LegacyWorldRoleFrame &, const LegacyWorldRoleBlitRequest &,
             LegacyRleRowJitterState &jitter) noexcept override {
    ++draws;
    ++jitter.group;
    jitter.phase_bytes += 4U;
    return LegacyBlitExecutionStatus::completed;
  }

  [[nodiscard]] const LegacyActionRecord *
  resolve_overlay_action(const u32) noexcept override {
    return nullptr;
  }

  void emit_role_particles(const i32, const i32, const u16) noexcept override {}

  [[nodiscard]] std::span<const u8>
  resolve_label_bytes(const u32) noexcept override {
    return {};
  }

  [[nodiscard]] u16 label_color(const u32) noexcept override { return 0U; }

  void draw_label(std::span<const u8>, const i32, const i32, const u16,
                  const u32) noexcept override {}

  std::vector<std::pair<u16, u16>> loads;
  u32 draws{};
};

class RecordingAudioPorts final : public LegacyWorldSpatialAudioPorts {
public:
  void play_sample(const u16, const i32, const i32,
                   const i32) noexcept override {}
  void stop_sample(const u16) noexcept override {}
  void set_sample_volume(const u16, const i32) noexcept override {}
  void set_sample_pan(const u16, const i32) noexcept override {}
};

[[nodiscard]] LegacyWorldFrameRuntimeState
make_runtime_state(std::vector<openswd3::compat::i16> &distances,
                   std::vector<openswd3::compat::i16> &vertical_offsets) {
  return {
      .frame = {},
      .spatial_audio =
          LegacyWorldSpatialAudioState{
              .controlled_role_index = 0U,
              .mix_level = 11,
              .distance_by_role = distances,
              .vertical_offset_by_role = vertical_offsets,
          },
  };
}

void test_spatial_stages_execute_in_frame_order(openswd3::test::Context &test) {
  BackgroundFixture background;
  LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
  std::array<LegacyWorldRoleRecord, 3U> roles{};
  roles[1] = make_drawable_role(320U, 240U);
  roles[1].flags |= kLegacyWorldFlaggedRoleBit;
  roles[2] = make_drawable_role(360U, 240U);
  spatial.row_heads[0U][kLegacySpatialRowPadding + 15U] = 1U;
  spatial.row_heads[2U][kLegacySpatialRowPadding + 15U] = 2U;

  std::vector<openswd3::compat::i16> distances(roles.size());
  std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
  const LegacyWorldFrameRuntimeState state =
      make_runtime_state(distances, vertical_offsets);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster = framebuffer.geometry();
  LegacyRleRowJitterState jitter;
  RemainingPorts remaining;
  RecordingFlaggedPorts flagged;
  RecordingRolePorts ordinary;
  RecordingAudioPorts audio;

  const auto result = compose_legacy_world_runtime_frame(
      framebuffer, raster, background.source(), spatial, roles, state, jitter,
      LegacyWorldFrameRuntimePorts{
          .remaining_stages = remaining,
          .flagged_roles = flagged,
          .world_roles = ordinary,
          .spatial_audio = audio,
      });

  test.expect_true(
      result.status == LegacyWorldFrameRuntimeStatus::completed &&
          result.composition.status ==
              LegacyWorldFrameCompositionStatus::completed &&
          result.flagged_stage_executed && result.world_roles_stage_executed &&
          result.flagged_roles.draw_count == 1U &&
          result.world_roles.status == LegacyWorldRolesStatus::completed &&
          result.world_roles.visited_roles == 2U &&
          result.world_roles.draw_count == 2U,
      "0x00412930 executes both recovered spatial stages at their real slots");
  test.expect_true(
      result.composition.stage_call_count == 19U &&
          result.delegated_stage_count == 17U && flagged.draws == 1U &&
          ordinary.draws == 2U &&
          std::ranges::find(
              remaining.stages,
              LegacyWorldFrameStage::flagged_spatial_objects_00413ea0) ==
              remaining.stages.end() &&
          std::ranges::find(
              remaining.stages,
              LegacyWorldFrameStage::world_spatial_objects_00413870) ==
              remaining.stages.end(),
      "runtime adapter delegates only the still-unwired frame stages");
}

void test_spatial_failure_stops_at_original_stage(
    openswd3::test::Context &test) {
  BackgroundFixture background;
  LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
  spatial.row_heads[0U].clear();
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  std::vector<openswd3::compat::i16> distances(roles.size());
  std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
  const LegacyWorldFrameRuntimeState state =
      make_runtime_state(distances, vertical_offsets);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster = framebuffer.geometry();
  LegacyRleRowJitterState jitter;
  RemainingPorts remaining;
  RecordingFlaggedPorts flagged;
  RecordingRolePorts ordinary;
  RecordingAudioPorts audio;

  const auto result = compose_legacy_world_runtime_frame(
      framebuffer, raster, background.source(), spatial, roles, state, jitter,
      LegacyWorldFrameRuntimePorts{remaining, flagged, ordinary, audio});

  test.expect_true(
      result.status == LegacyWorldFrameRuntimeStatus::flagged_roles_failed &&
          result.composition.status ==
              LegacyWorldFrameCompositionStatus::stage_failed &&
          result.composition.failed_stage ==
              LegacyWorldFrameStage::flagged_spatial_objects_00413ea0 &&
          result.failed_stage_recorded && result.flagged_stage_executed &&
          !result.world_roles_stage_executed &&
          remaining.stages ==
              std::vector{
                  LegacyWorldFrameStage::pre_background_records_004151f0,
              },
      "checked spatial failure cannot be reported as a completed frame");
  test.expect_true(raster.clip_left == 0 && raster.clip_top == 0 &&
                       raster.clip_width == 640 && raster.clip_height == 480,
                   "stage failure restores the full software raster clip");
}

void test_delegated_failure_is_visible(openswd3::test::Context &test) {
  BackgroundFixture background;
  LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
  std::array<LegacyWorldRoleRecord, 1U> roles{};
  std::vector<openswd3::compat::i16> distances(roles.size());
  std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
  const LegacyWorldFrameRuntimeState state =
      make_runtime_state(distances, vertical_offsets);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster = framebuffer.geometry();
  LegacyRleRowJitterState jitter;
  RemainingPorts remaining;
  remaining.fail_stage = true;
  RecordingFlaggedPorts flagged;
  RecordingRolePorts ordinary;
  RecordingAudioPorts audio;

  const auto result = compose_legacy_world_runtime_frame(
      framebuffer, raster, background.source(), spatial, roles, state, jitter,
      LegacyWorldFrameRuntimePorts{remaining, flagged, ordinary, audio});
  test.expect_true(
      result.status == LegacyWorldFrameRuntimeStatus::delegated_stage_failed &&
          result.composition.status ==
              LegacyWorldFrameCompositionStatus::stage_failed &&
          result.failed_stage ==
              LegacyWorldFrameStage::pre_background_records_004151f0 &&
          !result.flagged_stage_executed && !result.world_roles_stage_executed,
      "an incomplete external stage has an explicit non-success boundary");
}

class RealExternalPorts final : public LegacyWorldFramePorts,
                                public LegacyWorldRoleExternalPorts {
public:
  [[nodiscard]] bool query_service(const u32) noexcept override {
    return false;
  }
  [[nodiscard]] bool query_control(const u32) noexcept override {
    return false;
  }
  [[nodiscard]] bool
  execute_stage(const LegacyWorldFrameStage) noexcept override {
    return true;
  }
  void draw_decorated_number(const i32, const i32, const u32,
                             const u32) noexcept override {}
  void play_positional_sample(const u16, const i32,
                              const i32) noexcept override {}
  [[nodiscard]] const LegacyActionRecord *
  resolve_overlay_action(const u32) noexcept override {
    return nullptr;
  }
  void emit_role_particles(const i32, const i32, const u16) noexcept override {}
  [[nodiscard]] std::span<const u8>
  resolve_label_bytes(const u32) noexcept override {
    return {};
  }
  [[nodiscard]] u16 label_color(const u32) noexcept override { return 0U; }
  void draw_label(std::span<const u8>, const i32, const i32, const u16,
                  const u32) noexcept override {}
};

void test_real_tsw_combined_frame(openswd3::test::Context &test,
                                  const std::filesystem::path &data_root) {
  BackgroundFixture background;
  LegacyRoleSpatialIndex spatial = make_spatial_index(background.height);
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  roles[1] = make_drawable_role(320U, 240U);
  roles[1].flags |= kLegacyWorldFlaggedRoleBit;
  spatial.row_heads[0U][kLegacySpatialRowPadding + 15U] = 1U;

  std::vector<openswd3::compat::i16> distances(roles.size());
  std::vector<openswd3::compat::i16> vertical_offsets(roles.size());
  const LegacyWorldFrameRuntimeState state =
      make_runtime_state(distances, vertical_offsets);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster = framebuffer.geometry();
  LegacyRleRowJitterState jitter;
  const LegacyBlitEffectState effects;
  LegacyActRuntime act_runtime{data_root};
  LegacyActActionStreamProvider stream_provider{act_runtime};
  LegacyActionUpdater action_updater{stream_provider};
  LegacyTswRuntime tsw_runtime{data_root};
  tsw_runtime.set_cache_limit(0x01000000U);
  RealExternalPorts external;
  LegacyActionDrawRuntimePorts flagged_ports{
      action_updater, tsw_runtime, framebuffer, raster, effects, jitter};
  LegacyWorldRoleRenderRuntimePorts ordinary_ports{tsw_runtime, framebuffer,
                                                   raster, effects, external};
  RecordingAudioPorts audio;

  const auto result = compose_legacy_world_runtime_frame(
      framebuffer, raster, background.source(), spatial, roles, state, jitter,
      LegacyWorldFrameRuntimePorts{external, flagged_ports, ordinary_ports,
                                   audio});
  test.expect_true(
      result.status == LegacyWorldFrameRuntimeStatus::completed &&
          result.flagged_roles.draw_count == 1U &&
          result.world_roles.draw_count == 1U &&
          result.flagged_roles.blit_failure_count == 0U &&
          result.world_roles.blit_failure_count == 0U,
      "real TSW reaches both spatial draw paths inside one world frame");
  const auto combined_hash = legacy_framebuffer_logical_fnv1a64(framebuffer);
  test.expect_equal(
      combined_hash, std::uint64_t{0xA6144A91E57939F9ULL},
      "combined background and real-role framebuffer vector is stable");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_spatial_stages_execute_in_frame_order(test);
  test_spatial_failure_stops_at_original_stage(test);
  test_delegated_failure_is_visible(test);
  if (argument_count == 2 && arguments != nullptr && arguments[1] != nullptr) {
    test_real_tsw_combined_frame(test, std::filesystem::path{arguments[1]});
  }
  return test.exit_code();
}
