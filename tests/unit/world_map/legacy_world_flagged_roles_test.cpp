#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/world_map/legacy_world_flagged_roles.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <limits>
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
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::world_map::draw_legacy_world_flagged_role;
using openswd3::world_map::draw_legacy_world_flagged_roles;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldFlaggedRoleBit;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldFlaggedRoleDrawStatus;
using openswd3::world_map::LegacyWorldFlaggedRolesStatus;
using openswd3::world_map::LegacyWorldRenderCamera;
using openswd3::world_map::LegacyWorldRoleRecord;

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 opacity_step{};
};

class RecordingPorts final : public LegacyActionDrawPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &) override {
    ++unexpected_updates;
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    loads.emplace_back(resource_id, frame_index);
    piece.width = 16U;
    piece.height = 20U;
    return load_succeeds;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 opacity_step) noexcept override {
    draws.push_back({destination_x, destination_y, flags, opacity_step});
    return draw_status;
  }

  bool load_succeeds{true};
  LegacyBlitExecutionStatus draw_status{LegacyBlitExecutionStatus::completed};
  u32 unexpected_updates{};
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
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

void test_single_role_contract(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role{};
  role.flags = 0x00008000U;
  role.world_x = 0x80000010U;
  role.world_y = 0x7FFFFFF0U;
  role.field_28 = 9U;
  role.field_2a = 7U;
  role.action.action_id = 1U;
  role.action.draw_offset_x = 0x80000001U;
  role.action.draw_offset_y = 0x77777777U;
  role.action.mode_flags = 0xFFFFFFFFU;
  role.action.field_4a = 0x1357U;
  role.action.field_4c = 0x2468U;
  RecordingPorts ports;

  const auto draw = draw_legacy_world_flagged_role(
      role,
      LegacyWorldRenderCamera{
          .left = std::numeric_limits<i32>::min(),
          .top = std::numeric_limits<i32>::max(),
      },
      ports);

  test.expect_true(draw.status == LegacyWorldFlaggedRoleDrawStatus::completed &&
                       draw.drawable && draw.horizontally_visible &&
                       draw.frame_requested && draw.drawn,
                   "0x00413F00 accepts bit15 set and bit10 clear");
  test.expect_true(
      ports.loads == std::vector<std::pair<u16, u16>>{{0x1357U, 0x2468U}} &&
          ports.draws.size() == 1U && ports.unexpected_updates == 0U,
      "flagged role resolves one TSW frame without updating its action");
  test.expect_true(
      ports.draws[0].x == std::bit_cast<i32>(u32{0x80000018U}) &&
          ports.draws[0].y == 0 && ports.draws[0].flags == 0x80000017U &&
          ports.draws[0].opacity_step == 4,
      "coordinates wrap and the fixed 0x16 mode uses opacity step four");
}

void test_selector_and_strict_culling(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role{};
  role.flags = 0x00008000U;
  role.action.field_4a = 7U;
  role.action.field_4c = 8U;
  RecordingPorts ports;

  role.world_x = std::bit_cast<u32>(i32{-320});
  const auto left =
      draw_legacy_world_flagged_role(role, LegacyWorldRenderCamera{}, ports);
  role.world_x = 960U;
  const auto right =
      draw_legacy_world_flagged_role(role, LegacyWorldRenderCamera{}, ports);
  test.expect_true(!left.horizontally_visible && !right.horizontally_visible &&
                       ports.loads.empty(),
                   "0x00413F00 excludes both exact horizontal boundaries");

  role.world_x = std::bit_cast<u32>(i32{-319});
  const auto visible =
      draw_legacy_world_flagged_role(role, LegacyWorldRenderCamera{}, ports);
  test.expect_true(visible.drawn && ports.loads.back().first == 0xFFFFU,
                   "zero action id selects special TSW resource FFFF");

  role.flags |= 0x00000400U;
  const auto bit10 =
      draw_legacy_world_flagged_role(role, LegacyWorldRenderCamera{}, ports);
  role.flags &= ~0x00008000U;
  const auto no_bit15 =
      draw_legacy_world_flagged_role(role, LegacyWorldRenderCamera{}, ports);
  test.expect_true(!bit10.drawable && !no_bit15.drawable &&
                       ports.loads.size() == 1U,
                   "bit10 suppresses drawing and bit15 is required");
}

void test_spatial_scan_order_and_group(openswd3::test::Context &test) {
  LegacyRoleSpatialIndex spatial = make_spatial_index(80U);
  std::array<LegacyWorldRoleRecord, 7U> roles{};
  for (u32 index = 1U; index < roles.size(); ++index) {
    roles[index].flags = 0x00008000U | kLegacyWorldFlaggedRoleBit;
    roles[index].world_x = 100U + index;
    roles[index].action.action_id = 1U;
    roles[index].action.field_4a = static_cast<u16>(index);
    roles[index].action.field_4c = static_cast<u16>(index + 10U);
  }
  roles[1].spatial_next_link_32 = 2U;
  roles[2].spatial_next_link_32 = 0U;
  roles[3].flags &= ~kLegacyWorldFlaggedRoleBit;
  roles[4].spatial_next_link_32 = 0U;
  roles[5].spatial_next_link_32 = 0U;
  roles[6].spatial_next_link_32 = 0U;

  // camera top 160 -> first logical row 5, then exactly forty row slots.
  spatial.row_heads[0U][kLegacySpatialRowPadding + 5U] = 1U;
  spatial.row_heads[0U][kLegacySpatialRowPadding + 6U] = 3U;
  spatial.row_heads[0U][kLegacySpatialRowPadding + 44U] = 4U;
  spatial.row_heads[0U][kLegacySpatialRowPadding + 45U] = 5U;
  spatial.row_heads[1U][kLegacySpatialRowPadding + 5U] = 6U;
  RecordingPorts ports;

  const auto result = draw_legacy_world_flagged_roles(
      spatial, roles, LegacyWorldRenderCamera{.top = 160}, ports);
  test.expect_true(
      result.status == LegacyWorldFlaggedRolesStatus::completed &&
          result.visited_rows == 40U && result.visited_roles == 4U &&
          result.flagged_roles == 3U && result.frame_requests == 3U &&
          result.draw_count == 3U,
      "0x00413EA0 scans forty rows and counts only group-zero bit29 roles");
  test.expect_true(ports.loads == std::vector<std::pair<u16, u16>>{{1U, 11U},
                                                                   {2U, 12U},
                                                                   {4U, 14U}},
                   "row and linked-list traversal order is preserved");
}

void test_negative_rows_and_checked_links(openswd3::test::Context &test) {
  LegacyRoleSpatialIndex spatial = make_spatial_index(3U);
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  roles[1].flags = 0x00008000U | kLegacyWorldFlaggedRoleBit;
  roles[1].world_x = 1U;
  roles[1].action.action_id = 1U;
  roles[1].action.field_4a = 1U;
  spatial.row_heads[0U][kLegacySpatialRowPadding] = 1U;
  RecordingPorts ports;

  const auto short_map = draw_legacy_world_flagged_roles(
      spatial, roles, LegacyWorldRenderCamera{}, ports);
  test.expect_true(
      short_map.status == LegacyWorldFlaggedRolesStatus::completed &&
          short_map.visited_rows == 8U && short_map.draw_count == 1U,
      "five negative rows are skipped before map rows and map height stops "
      "scanning");

  spatial.row_heads[0U][kLegacySpatialRowPadding] = 99U;
  const auto invalid = draw_legacy_world_flagged_roles(
      spatial, roles, LegacyWorldRenderCamera{}, ports);
  test.expect_equal(
      invalid.status, LegacyWorldFlaggedRolesStatus::invalid_role_link,
      "impossible pointer links are isolated at the modern index boundary");

  spatial.row_heads[0U].pop_back();
  const auto short_index = draw_legacy_world_flagged_roles(
      spatial, roles, LegacyWorldRenderCamera{}, ports);
  test.expect_equal(short_index.status,
                    LegacyWorldFlaggedRolesStatus::invalid_spatial_index,
                    "short row-head allocation is rejected before traversal");
}

void test_failures_and_accepted_blit_exits(openswd3::test::Context &test) {
  LegacyWorldRoleRecord role{};
  role.flags = 0x00008000U;
  role.world_x = 1U;
  role.action.action_id = 1U;

  RecordingPorts missing;
  missing.load_succeeds = false;
  const auto load_failure =
      draw_legacy_world_flagged_role(role, LegacyWorldRenderCamera{}, missing);
  test.expect_equal(
      load_failure.status, LegacyWorldFlaggedRoleDrawStatus::frame_load_failed,
      "missing TSW frame stops before the impossible original dereference");

  for (const auto status : {LegacyBlitExecutionStatus::completed,
                            LegacyBlitExecutionStatus::clipped_out,
                            LegacyBlitExecutionStatus::opacity_disabled}) {
    RecordingPorts accepted;
    accepted.draw_status = status;
    const auto draw = draw_legacy_world_flagged_role(
        role, LegacyWorldRenderCamera{}, accepted);
    test.expect_equal(draw.blit_failure_count, u32{0U},
                      "legacy early blit exits are accepted");
  }

  RecordingPorts malformed;
  malformed.draw_status = LegacyBlitExecutionStatus::malformed_source;
  const auto draw = draw_legacy_world_flagged_role(
      role, LegacyWorldRenderCamera{}, malformed);
  test.expect_equal(
      draw.blit_failure_count, u32{1U},
      "ignored original blit result remains available for diagnostics");
}

void test_real_tsw_and_blitter(openswd3::test::Context &test,
                               const std::filesystem::path &data_root) {
  LegacyActRuntime act_runtime{data_root};
  LegacyActActionStreamProvider stream_provider{act_runtime};
  LegacyActionUpdater updater{stream_provider};
  LegacyTswRuntime tsw_runtime{data_root};
  tsw_runtime.set_cache_limit(0x01000000U);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster;
  test.expect_true(openswd3::rendering::initialize_legacy_raster_geometry(
                       raster, LegacySurfaceGeometry{}),
                   "real flagged-role raster initializes");
  const LegacyBlitEffectState effects;
  LegacyRleRowJitterState jitter;
  LegacyActionDrawRuntimePorts ports{
      updater, tsw_runtime, framebuffer, raster, effects, jitter,
  };

  LegacyRoleSpatialIndex spatial = make_spatial_index(40U);
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  initialize_legacy_action_record(roles[1].action);
  roles[1].flags = 0x00008001U | kLegacyWorldFlaggedRoleBit;
  roles[1].world_x = 320U;
  roles[1].world_y = 240U;
  roles[1].action.action_id = 1U;
  roles[1].action.field_4a = 1U;
  roles[1].action.field_4c = 0U;
  spatial.row_heads[0U][kLegacySpatialRowPadding + 15U] = 1U;

  const auto result = draw_legacy_world_flagged_roles(
      spatial, roles, LegacyWorldRenderCamera{}, ports);
  test.expect_true(
      result.status == LegacyWorldFlaggedRolesStatus::completed &&
          result.draw_count == 1U && result.blit_failure_count == 0U &&
          std::ranges::any_of(framebuffer.physical_pixels(),
                              [](const u16 pixel) { return pixel != 0U; }),
      "real TSW frame reaches the flagged-role opacity blitter path");
  const std::uint64_t framebuffer_hash =
      openswd3::rendering::legacy_framebuffer_logical_fnv1a64(framebuffer);
  test.expect_equal(framebuffer_hash, std::uint64_t{0xA6C3E08156F06060ULL},
                    "real flagged-role framebuffer vector is stable");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_single_role_contract(test);
  test_selector_and_strict_culling(test);
  test_spatial_scan_order_and_group(test);
  test_negative_rows_and_checked_links(test);
  test_failures_and_accepted_blit_exits(test);
  if (argument_count == 2) {
    test_real_tsw_and_blitter(test, std::filesystem::path{arguments[1]});
  }
  return test.exit_code();
}
