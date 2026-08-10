#include "test.hpp"

#include "openswd3/world_map/legacy_world_roles.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <span>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::world_map::draw_legacy_world_role;
using openswd3::world_map::draw_legacy_world_roles;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldGuidLookupRoleBit;
using openswd3::world_map::kLegacyWorldRoleFlashBit;
using openswd3::world_map::kLegacyWorldRoleParticleBit;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldRenderCamera;
using openswd3::world_map::LegacyWorldRoleBlitRequest;
using openswd3::world_map::LegacyWorldRoleDrawStatus;
using openswd3::world_map::LegacyWorldRoleFrame;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleRenderPorts;
using openswd3::world_map::LegacyWorldRoleRenderRuntimePorts;
using openswd3::world_map::LegacyWorldRoleRenderState;
using openswd3::world_map::LegacyWorldRolesStatus;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;

struct RecordedDraw {
  LegacyWorldRoleBlitRequest request;
  i32 group_before{};
  u32 phase_before{};
};

struct RecordedLabel {
  std::vector<u8> bytes;
  i32 x{};
  i32 y{};
  u16 color{};
  u32 style{};
};

class RecordingPorts final : public LegacyWorldRoleRenderPorts {
public:
  [[nodiscard]] bool query_service(const u32 service_id) noexcept override {
    service_queries.push_back(service_id);
    if (service_id == 0x0BU) {
      return service_0b;
    }
    if (service_id == 0x48U) {
      return service_48;
    }
    return false;
  }

  void play_positional_sample(const u16 sound_id, const i32 world_x,
                              const i32 world_y) noexcept override {
    positional_samples.push_back({sound_id, {world_x, world_y}});
  }

  [[nodiscard]] bool load_frame(const u16 resource_id, const u16 frame_index,
                                LegacyWorldRoleFrame &frame) override {
    loads.emplace_back(resource_id, frame_index);
    if (!load_succeeds) {
      return false;
    }
    frame.width = 16U;
    frame.height = 20U;
    frame.auxiliary = auxiliary;
    return true;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame(const LegacyWorldRoleFrame &,
             const LegacyWorldRoleBlitRequest &request,
             LegacyRleRowJitterState &jitter) noexcept override {
    draws.push_back({request, jitter.group, jitter.phase_bytes});
    jitter.phase_bytes += 4U;
    if (jitter.phase_bytes >= 0x84U) {
      jitter.phase_bytes = 0U;
    }
    return draw_status;
  }

  [[nodiscard]] const LegacyActionRecord *
  resolve_overlay_action(const u32 token) noexcept override {
    overlay_tokens.push_back(token);
    return overlay_available ? &overlay : nullptr;
  }

  void emit_role_particles(const i32 world_x, const i32 world_y,
                           const u16 guid) noexcept override {
    particles.push_back({world_x, world_y, guid});
  }

  [[nodiscard]] std::span<const u8>
  resolve_label_bytes(const u32 token) noexcept override {
    label_tokens.push_back(token);
    return label_available ? std::span<const u8>{label} : std::span<const u8>{};
  }

  [[nodiscard]] u16 label_color(const u32 index) noexcept override {
    color_indices.push_back(index);
    return label_color_value;
  }

  void draw_label(const std::span<const u8> bytes, const i32 x, const i32 y,
                  const u16 color, const u32 style) noexcept override {
    labels.push_back(
        {std::vector<u8>{bytes.begin(), bytes.end()}, x, y, color, style});
  }

  bool service_0b{};
  bool service_48{};
  bool load_succeeds{true};
  bool overlay_available{true};
  bool label_available{true};
  LegacyBlitExecutionStatus draw_status{LegacyBlitExecutionStatus::completed};
  LegacyActionRecord overlay{};
  std::array<u8, 3U> auxiliary{1U, 2U, 3U};
  std::vector<u8> label{'A', 'B', 0U};
  u16 label_color_value{0x1234U};
  std::vector<u32> service_queries;
  std::vector<std::pair<u16, std::pair<i32, i32>>> positional_samples;
  std::vector<std::pair<u16, u16>> loads;
  std::vector<RecordedDraw> draws;
  std::vector<u32> overlay_tokens;
  std::vector<std::array<i32, 3U>> particles;
  std::vector<u32> label_tokens;
  std::vector<u32> color_indices;
  std::vector<RecordedLabel> labels;
};

class RecordingAudioPorts final : public LegacyWorldSpatialAudioPorts {
public:
  void play_sample(const u16 sound_id, const i32, const i32,
                   const i32) noexcept override {
    plays.push_back(sound_id);
  }

  void stop_sample(const u16 sound_id) noexcept override {
    stops.push_back(sound_id);
  }

  void set_sample_volume(const u16 sound_id,
                         const i32 volume) noexcept override {
    volumes.push_back({sound_id, volume});
  }

  void set_sample_pan(const u16 sound_id, const i32 pan) noexcept override {
    pans.push_back({sound_id, pan});
  }

  std::vector<u16> plays;
  std::vector<u16> stops;
  std::vector<std::pair<u16, i32>> volumes;
  std::vector<std::pair<u16, i32>> pans;
};

[[nodiscard]] LegacyRoleSpatialIndex make_spatial_index(const u32 map_height) {
  LegacyRoleSpatialIndex spatial;
  spatial.map_height = map_height;
  for (auto &group : spatial.row_heads) {
    group.resize(static_cast<std::size_t>(map_height) +
                     2U * static_cast<std::size_t>(kLegacySpatialRowPadding),
                 0U);
  }
  return spatial;
}

[[nodiscard]] LegacyWorldRoleRecord make_role() {
  LegacyWorldRoleRecord role{};
  role.flags = 0x00008000U;
  role.world_x = 100U;
  role.world_y = 200U;
  role.guid = 9U;
  role.field_28 = 5U;
  role.field_2a = 6U;
  role.action.action_id = 1U;
  role.action.draw_offset_x = 10U;
  role.action.draw_offset_y = 20U;
  role.action.mode_flags = 0x80000003U;
  role.action.field_4a = 7U;
  role.action.field_4c = 8U;
  role.action.field_58 = 9U;
  role.action.field_88 = 2U;
  role.action.field_89 = 8U;
  role.action.field_8a = 5U;
  return role;
}

[[nodiscard]] LegacyWorldRoleRenderState make_state() {
  return {
      .camera = LegacyWorldRenderCamera{.left = 30, .top = 40},
      .frame_counter = 4U,
      .talk_target = 0xFFFFU,
  };
}

void test_main_draw_contract(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role = make_role();
  RecordingPorts ports;
  LegacyRleRowJitterState jitter;
  const auto result = draw_legacy_world_role(role, make_state(), jitter, ports);

  test.expect_true(result.status == LegacyWorldRoleDrawStatus::completed &&
                       result.drawable && result.horizontally_visible &&
                       result.main_drawn && result.draw_count == 1U,
                   "0x00413910 reaches one ordinary main draw");
  test.expect_true(
      ports.loads == std::vector<std::pair<u16, u16>>{{7U, 8U}} &&
          ports.positional_samples.size() == 1U && role.action.field_58 == 0U,
      "resource selection and one-shot sound clearing preserve order");
  const RecordedDraw &draw = ports.draws[0];
  test.expect_true(
      draw.request.destination_x == 65 && draw.request.destination_y == 146 &&
          draw.request.flags == 0x80000003U && draw.request.opacity_step == 5 &&
          draw.group_before == 2 && draw.request.auxiliary.size() == 3U &&
          draw.phase_before == 8U && role.action.field_89 == 12U,
      "main coordinates effects auxiliary and jitter phase are exact");
}

void test_inclusive_cull_and_service(openswd3::test::Context &test) {
  RecordingPorts ports;
  LegacyRleRowJitterState jitter;
  LegacyWorldRoleRecord role = make_role();
  role.action.field_58 = 0U;
  role.world_x = std::bit_cast<u32>(i32{-320});
  auto left =
      draw_legacy_world_role(role, LegacyWorldRoleRenderState{}, jitter, ports);
  role.world_x = 960U;
  auto right =
      draw_legacy_world_role(role, LegacyWorldRoleRenderState{}, jitter, ports);
  test.expect_true(
      left.main_drawn && right.main_drawn && ports.draws.size() == 2U,
      "ordinary role culling includes both -320 and 960 boundaries");

  role.world_x = std::bit_cast<u32>(i32{-321});
  auto outside =
      draw_legacy_world_role(role, LegacyWorldRoleRenderState{}, jitter, ports);
  test.expect_true(!outside.horizontally_visible && ports.draws.size() == 2U,
                   "ordinary role culling excludes values beyond the boundary");

  role.world_x = 0U;
  ports.service_0b = true;
  auto service =
      draw_legacy_world_role(role, LegacyWorldRoleRenderState{}, jitter, ports);
  test.expect_true(service.suppressed_by_service && ports.loads.size() == 2U,
                   "service 0B suppresses non-FFFF roles before frame loading");
}

void test_ghost_pass(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role = make_role();
  role.action.field_58 = 0U;
  role.flags |= kLegacyWorldRoleFlashBit;
  LegacyWorldRoleRenderState state = make_state();
  state.frame_counter = 1U;
  RecordingPorts ports;
  LegacyRleRowJitterState jitter{.group = 6, .phase_bytes = 20U};

  const auto result = draw_legacy_world_role(role, state, jitter, ports);
  test.expect_true(result.ghost_drawn && result.main_drawn &&
                       result.draw_count == 2U && ports.draws.size() == 2U,
                   "flash bit and low counter phase add the ghost pass");
  const auto &ghost = ports.draws[0].request;
  test.expect_true(ghost.destination_x == 56 && ghost.destination_y == 154 &&
                       ghost.target_height == 10 &&
                       ghost.horizontal_resample_displacement == 5 &&
                       ghost.flags == 0x8000000FU && ghost.red_offset == -8 &&
                       ghost.green_offset == -8 && ghost.blue_offset == -8 &&
                       ports.draws[0].group_before == 6 &&
                       ghost.auxiliary.empty(),
                   "0x004145F0 phase-one geometry flags and colors are exact");
  test.expect_true(
      ports.draws[0].phase_before == 20U && ports.draws[1].phase_before == 8U &&
          role.action.field_89 == 12U && jitter.group == 2 &&
          jitter.phase_bytes == 12U,
      "ghost inherits prior global jitter before main loads the role state");
}

void test_additive_overlay_particles_and_label(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role = make_role();
  role.action.field_58 = 0U;
  role.flags |=
      kLegacyWorldRoleFlashBit | kLegacyWorldRoleParticleBit | (2U << 20U);
  role.field_3c = 1U;
  role.path_payload_pointer_32 = 2U;
  role.path_payload_relation = 3U;
  RecordingPorts ports;
  ports.overlay.draw_offset_x = 2U;
  ports.overlay.draw_offset_y = 3U;
  ports.overlay.mode_flags = 0x55U;
  ports.overlay.field_4a = 4U;
  ports.overlay.field_4c = 5U;
  LegacyWorldRoleRenderState state = make_state();
  state.flash_red_offset = 1;
  state.flash_green_offset = 2;
  state.flash_blue_offset = 3;
  LegacyRleRowJitterState jitter;

  const auto result = draw_legacy_world_role(role, state, jitter, ports);
  test.expect_true(
      result.main_drawn && result.additive_drawn && result.overlay_drawn &&
          result.particles_emitted && result.label_drawn &&
          result.draw_count == 3U,
      "optional ordinary-role stages all execute in original order");
  const auto &additive = ports.draws[1].request;
  test.expect_true(additive.flags == 0x80000013U && additive.red_offset == 5 &&
                       additive.green_offset == 6 && additive.blue_offset == 7,
                   "additive table offset combines with three global colors");
  const auto &overlay = ports.draws[2].request;
  test.expect_true(overlay.destination_x == 73 &&
                       overlay.destination_y == 171 && overlay.flags == 0x55U &&
                       overlay.auxiliary.empty(),
                   "overlay subtracts both Y offsets and adds literal 28");
  test.expect_true(ports.particles ==
                           std::vector<std::array<i32, 3U>>{{100, 200, 9}} &&
                       ports.service_queries == std::vector<u32>{0x0BU, 0x48U},
                   "particle emission uses role coordinates after service 48");
  test.expect_true(
      ports.labels.size() == 1U &&
          ports.labels[0].bytes == std::vector<u8>{'A', 'B'} &&
          ports.labels[0].x == 75 && ports.labels[0].y == 140 &&
          ports.labels[0].color == 0x1234U && ports.labels[0].style == 4U &&
          ports.color_indices == std::vector<u32>{3U},
      "label uses byte length centering color table and style four");
}

void test_checked_failures_and_blit_diagnostics(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role = make_role();
  role.action.field_58 = 0U;
  RecordingPorts missing_frame;
  missing_frame.load_succeeds = false;
  LegacyRleRowJitterState jitter;
  const auto frame =
      draw_legacy_world_role(role, make_state(), jitter, missing_frame);
  test.expect_equal(frame.status, LegacyWorldRoleDrawStatus::frame_load_failed,
                    "missing TSW frame stops before original null dereference");

  role = make_role();
  role.action.field_58 = 0U;
  role.field_3c = 1U;
  RecordingPorts missing_overlay;
  missing_overlay.overlay_available = false;
  const auto overlay =
      draw_legacy_world_role(role, make_state(), jitter, missing_overlay);
  test.expect_equal(
      overlay.status, LegacyWorldRoleDrawStatus::overlay_resolve_failed,
      "invalid legacy overlay pointer is isolated after main draw");

  role = make_role();
  role.action.field_58 = 0U;
  role.path_payload_pointer_32 = 2U;
  RecordingPorts bad_label;
  bad_label.label = {'A', 'B'};
  const auto label =
      draw_legacy_world_role(role, make_state(), jitter, bad_label);
  test.expect_equal(
      label.status, LegacyWorldRoleDrawStatus::label_missing_terminator,
      "unterminated legacy label is isolated before lstrlen overrun");

  role = make_role();
  role.action.field_58 = 0U;
  RecordingPorts malformed;
  malformed.draw_status = LegacyBlitExecutionStatus::malformed_source;
  const auto diagnostic =
      draw_legacy_world_role(role, make_state(), jitter, malformed);
  test.expect_equal(
      diagnostic.blit_failure_count, u32{1U},
      "ignored original blit failure remains diagnostic evidence");
}

void test_spatial_traversal_order_padding_and_audio(
    openswd3::test::Context &test) {
  LegacyRoleSpatialIndex spatial = make_spatial_index(3U);
  std::array<LegacyWorldRoleRecord, 5U> roles{};
  for (u32 index = 1U; index < roles.size(); ++index) {
    roles[index] = make_role();
    roles[index].action.field_58 = 0U;
    roles[index].action.field_4a = static_cast<u16>(index * 10U);
    roles[index].action.field_4c = static_cast<u16>(index);
    roles[index].action.field_88 = static_cast<u8>(index + 1U);
    roles[index].action.field_89 = static_cast<u8>(index * 4U);
  }
  roles[1].action.field_88 = 2U;
  roles[1].action.field_89 = 8U;
  roles[4].flags |= kLegacyWorldRoleFlashBit;
  roles[4].action.field_88 = 3U;
  roles[4].action.field_89 = 20U;
  roles[2].flags |= kLegacyWorldGuidLookupRoleBit;
  roles[2].guid = 7U;
  roles[2].field_2c = 42U;
  roles[2].field_30 = 0xFFFF0000U;

  spatial.row_heads[2U][kLegacySpatialRowPadding] = 1U;
  spatial.row_heads[2U][spatial.row_heads[2U].size() - 1U] = 4U;
  spatial.row_heads[0U][kLegacySpatialRowPadding] = 2U;
  spatial.row_heads[1U][kLegacySpatialRowPadding] = 3U;

  std::array<openswd3::compat::i16, 5U> distances{};
  std::array<openswd3::compat::i16, 5U> vertical_offsets{};
  const LegacyWorldSpatialAudioState audio_state{
      .controlled_role_index = 0U,
      .mix_level = 11,
      .distance_by_role = distances,
      .vertical_offset_by_role = vertical_offsets,
  };
  LegacyWorldRoleRenderState render_state = make_state();
  render_state.camera = {};
  render_state.frame_counter = 1U;
  RecordingPorts render_ports;
  RecordingAudioPorts audio_ports;
  LegacyRleRowJitterState jitter;

  const auto result =
      draw_legacy_world_roles(spatial, roles, render_state, audio_state, jitter,
                              render_ports, audio_ports);
  test.expect_true(
      result.status == LegacyWorldRolesStatus::completed &&
          result.visited_groups == 3U && result.scanned_rows == 210U &&
          result.visited_rows == 69U && result.visited_roles == 4U,
      "0x00413870 scans 70 rows in each physical group including bottom "
      "padding");
  test.expect_true(
      render_ports.loads ==
              std::vector<std::pair<u16, u16>>{
                  {10U, 1U}, {40U, 4U}, {20U, 2U}, {30U, 3U}} &&
          result.frame_requests == 4U && result.draw_count == 5U,
      "spatial groups preserve the physical 2 0 1 traversal order");
  test.expect_true(
      render_ports.draws[1].group_before == 2 &&
          render_ports.draws[1].phase_before == 12U &&
          render_ports.draws[2].group_before == 3 &&
          render_ports.draws[2].phase_before == 20U,
      "a later ghost inherits the preceding role jitter before its main draw");
  test.expect_true(
      result.spatial_audio_roles == 1U && result.samples_started == 1U &&
          result.sample_parameters_updated == 1U &&
          audio_ports.plays == std::vector<u16>{42U} &&
          audio_ports.volumes.size() == 1U && audio_ports.pans.size() == 1U,
      "a nonzero +0x2C low word dispatches audio after drawing the role");
}

void test_spatial_traversal_checked_failures(openswd3::test::Context &test) {
  LegacyRoleSpatialIndex spatial = make_spatial_index(1U);
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  roles[1] = make_role();
  roles[1].action.field_58 = 0U;
  spatial.row_heads[2U][kLegacySpatialRowPadding] = 1U;
  std::array<openswd3::compat::i16, 2U> distances{};
  std::array<openswd3::compat::i16, 2U> vertical_offsets{};
  LegacyWorldSpatialAudioState audio_state{
      .controlled_role_index = 0U,
      .mix_level = 11,
      .distance_by_role = distances,
      .vertical_offset_by_role = vertical_offsets,
  };
  RecordingPorts render_ports;
  RecordingAudioPorts audio_ports;
  LegacyRleRowJitterState jitter;

  render_ports.load_succeeds = false;
  const auto draw_failure =
      draw_legacy_world_roles(spatial, roles, LegacyWorldRoleRenderState{},
                              audio_state, jitter, render_ports, audio_ports);
  test.expect_true(
      draw_failure.status == LegacyWorldRolesStatus::role_draw_failed &&
          draw_failure.role_draw_status ==
              LegacyWorldRoleDrawStatus::frame_load_failed,
      "checked frame failure stops before the original invalid dereference");

  render_ports.load_succeeds = true;
  roles[1].spatial_next_link_32 = 1U;
  const auto bad_link =
      draw_legacy_world_roles(spatial, roles, LegacyWorldRoleRenderState{},
                              audio_state, jitter, render_ports, audio_ports);
  test.expect_equal(bad_link.status, LegacyWorldRolesStatus::invalid_role_link,
                    "cyclic legacy pointers are isolated by index traversal");

  roles[1].spatial_next_link_32 = 0U;
  spatial.row_heads[1U].pop_back();
  const auto short_index =
      draw_legacy_world_roles(spatial, roles, LegacyWorldRoleRenderState{},
                              audio_state, jitter, render_ports, audio_ports);
  test.expect_equal(
      short_index.status, LegacyWorldRolesStatus::invalid_spatial_index,
      "all three row-head allocations are checked before traversal");
}

void test_real_tsw_and_blitter(openswd3::test::Context &test,
                               const std::filesystem::path &data_root) {
  LegacyTswRuntime tsw_runtime{data_root};
  tsw_runtime.set_cache_limit(0x01000000U);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster;
  test.expect_true(openswd3::rendering::initialize_legacy_raster_geometry(
                       raster, LegacySurfaceGeometry{}),
                   "real ordinary-role raster initializes");
  const LegacyBlitEffectState effects;
  RecordingPorts external_ports;
  LegacyWorldRoleRenderRuntimePorts ports{tsw_runtime, framebuffer, raster,
                                          effects, external_ports};
  LegacyWorldRoleRecord role{};
  role.flags = 0x00008000U;
  role.world_x = 320U;
  role.world_y = 240U;
  role.action.action_id = 1U;
  role.action.field_4a = 1U;
  role.action.field_4c = 0U;
  LegacyRleRowJitterState jitter;

  const auto result =
      draw_legacy_world_role(role, LegacyWorldRoleRenderState{}, jitter, ports);
  test.expect_true(
      result.status == LegacyWorldRoleDrawStatus::completed &&
          result.frame_requests == 1U && result.draw_count == 1U &&
          result.blit_failure_count == 0U,
      "real TSW frame reaches the ordinary-role software blitter path");
  test.expect_true(
      std::ranges::any_of(framebuffer.physical_pixels(),
                          [](const u16 pixel) { return pixel != 0U; }),
      "ordinary-role runtime adapter produces nonempty framebuffer pixels");
  const std::uint64_t framebuffer_hash =
      openswd3::rendering::legacy_framebuffer_logical_fnv1a64(framebuffer);
  test.expect_equal(framebuffer_hash, std::uint64_t{0xA4766C928B05DC88ULL},
                    "real ordinary-role framebuffer vector is stable");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_main_draw_contract(test);
  test_inclusive_cull_and_service(test);
  test_ghost_pass(test);
  test_additive_overlay_particles_and_label(test);
  test_checked_failures_and_blit_diagnostics(test);
  test_spatial_traversal_order_padding_and_audio(test);
  test_spatial_traversal_checked_failures(test);
  if (argument_count == 2) {
    test_real_tsw_and_blitter(test, std::filesystem::path{arguments[1]});
  }
  return test.exit_code();
}
