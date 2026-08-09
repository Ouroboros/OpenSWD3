#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::draw_legacy_tsw_frame;
using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionDrawRuntimePorts;
using openswd3::asset_runtime::LegacyActionDrawStatus;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::asset_runtime::update_draw_legacy_action;
using openswd3::asset_runtime::update_draw_legacy_action_with_flags;
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

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 opacity_step{};
};

class FakePorts final : public LegacyActionDrawPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    ++update_count;
    if (update_status != LegacyActionUpdateStatus::completed) {
      return update_status;
    }
    record.draw_offset_x = updated_offset_x;
    record.draw_offset_y = updated_offset_y;
    record.mode_flags = updated_flags;
    record.field_4a = updated_resource_id;
    record.field_4c = updated_frame_index;
    record.field_8a = updated_opacity;
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    loads.emplace_back(resource_id, frame_index);
    piece.width = 12U;
    piece.height = 20U;
    return load_succeeds;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 opacity_step) noexcept override {
    draws.push_back(DrawCall{
        .x = destination_x,
        .y = destination_y,
        .flags = flags,
        .opacity_step = opacity_step,
    });
    return draw_status;
  }

  LegacyActionUpdateStatus update_status{LegacyActionUpdateStatus::completed};
  bool load_succeeds{true};
  LegacyBlitExecutionStatus draw_status{LegacyBlitExecutionStatus::completed};
  u32 updated_offset_x{};
  u32 updated_offset_y{};
  u32 updated_flags{};
  u16 updated_resource_id{};
  u16 updated_frame_index{};
  openswd3::compat::u8 updated_opacity{};
  u32 update_count{};
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
};

void test_normal_bridge_reads_updated_record(openswd3::test::Context &test) {
  FakePorts ports;
  ports.updated_offset_x = 0x80000001U;
  ports.updated_offset_y = 7U;
  ports.updated_flags = 0x81234567U;
  ports.updated_resource_id = 0x1357U;
  ports.updated_frame_index = 0x2468U;
  ports.updated_opacity = 0xFEU;

  LegacyActionRecord record{};
  record.mode_flags = 0xAAAAAAAAU;
  const auto result =
      update_draw_legacy_action(record, std::numeric_limits<i32>::min(),
                                std::numeric_limits<i32>::min(), ports);

  test.expect_equal(result.status, LegacyActionDrawStatus::ready,
                    "normal bridge completes");
  test.expect_equal(result.action_update_count, u32{1U}, "action updates once");
  test.expect_equal(result.frame_request_count, u32{1U},
                    "updated frame is requested once");
  test.expect_equal(result.draw_count, u32{1U}, "frame draws once");
  test.expect_equal(ports.loads.front(), std::pair<u16, u16>{0x1357U, 0x2468U},
                    "updated TSW keys are used");
  test.expect_equal(ports.draws.front().x, i32{-1},
                    "x subtraction wraps as x86 dword arithmetic");
  test.expect_equal(ports.draws.front().y, i32{2147483641},
                    "y subtraction wraps as x86 dword arithmetic");
  test.expect_equal(ports.draws.front().flags, 0x81234567U,
                    "full flags are read after the action update");
  test.expect_equal(ports.draws.front().opacity_step, i32{0xFE},
                    "opacity byte is zero extended after the update");
}

void test_direct_bridge_truncates_slots(openswd3::test::Context &test) {
  FakePorts ports;
  const auto result =
      draw_legacy_tsw_frame(0xAAAA1357U, 0xBBBB2468U, -20, 30, ports);

  test.expect_equal(result.status, LegacyActionDrawStatus::ready,
                    "direct bridge completes");
  test.expect_equal(result.action_update_count, u32{0U},
                    "direct bridge skips action update");
  test.expect_equal(ports.loads.front(), std::pair<u16, u16>{0x1357U, 0x2468U},
                    "only the low words identify the TSW frame");
  test.expect_equal(ports.draws.front().x, i32{-20},
                    "direct bridge preserves x");
  test.expect_equal(ports.draws.front().y, i32{30},
                    "direct bridge preserves y");
  test.expect_equal(ports.draws.front().flags, u32{0U},
                    "direct bridge clears draw flags");
  test.expect_equal(ports.draws.front().opacity_step, i32{0},
                    "direct bridge clears opacity");
}

void test_masked_bridge_reads_updated_flags(openswd3::test::Context &test) {
  FakePorts ports;
  ports.updated_offset_x = 4U;
  ports.updated_offset_y = 6U;
  ports.updated_flags = 0xC0000007U;
  ports.updated_resource_id = 10U;
  ports.updated_frame_index = 11U;
  ports.updated_opacity = 12U;

  LegacyActionRecord record{};
  record.mode_flags = 0x0000FF00U;
  const auto result =
      update_draw_legacy_action_with_flags(record, 30, 40, 0x100U, ports);

  test.expect_equal(result.status, LegacyActionDrawStatus::ready,
                    "masked bridge completes");
  test.expect_equal(ports.draws.front().flags, 0x80000103U,
                    "caller flags combine with updated bits 31, 1 and 0");
  test.expect_equal(ports.draws.front().x, i32{26},
                    "masked bridge subtracts updated x offset");
  test.expect_equal(ports.draws.front().y, i32{34},
                    "masked bridge subtracts updated y offset");
}

void test_failures_stop_at_original_boundaries(openswd3::test::Context &test) {
  LegacyActionRecord record{};

  FakePorts update_failure;
  update_failure.update_status = LegacyActionUpdateStatus::malformed_stream;
  const auto stopped = update_draw_legacy_action(record, 1, 2, update_failure);
  test.expect_equal(stopped.status,
                    LegacyActionDrawStatus::action_update_failed,
                    "failed action update stops the bridge");
  test.expect_true(update_failure.loads.empty(),
                   "failed action update performs no TSW query");
  test.expect_true(update_failure.draws.empty(),
                   "failed action update performs no draw");

  FakePorts load_failure;
  load_failure.load_succeeds = false;
  const auto unloaded = update_draw_legacy_action(record, 1, 2, load_failure);
  test.expect_equal(unloaded.status, LegacyActionDrawStatus::frame_load_failed,
                    "failed TSW query is explicit at the modern boundary");
  test.expect_true(load_failure.draws.empty(),
                   "failed TSW query performs no draw");

  FakePorts draw_failure;
  draw_failure.draw_status = LegacyBlitExecutionStatus::malformed_source;
  const auto diagnosed = update_draw_legacy_action(record, 1, 2, draw_failure);
  test.expect_equal(diagnosed.status, LegacyActionDrawStatus::ready,
                    "legacy callers do not branch on the blit result");
  test.expect_equal(diagnosed.blit_failure_count, u32{1U},
                    "modern diagnostics retain the blit failure");
}

void test_non_error_blit_exits_are_accepted(openswd3::test::Context &test) {
  LegacyActionRecord record{};
  for (const auto status : {LegacyBlitExecutionStatus::clipped_out,
                            LegacyBlitExecutionStatus::opacity_disabled}) {
    FakePorts ports;
    ports.draw_status = status;
    const auto result = update_draw_legacy_action(record, 1, 2, ports);
    test.expect_equal(result.blit_failure_count, u32{0U},
                      "legacy early draw exits are not failures");
  }
}

void test_real_act_tsw_and_blitter(openswd3::test::Context &test,
                                   const std::filesystem::path &data_root) {
  LegacyActRuntime act_runtime{data_root};
  act_runtime.set_cache_limit(0x00080000U);
  LegacyActActionStreamProvider stream_provider{act_runtime};
  LegacyActionUpdater action_updater{stream_provider};
  LegacyTswRuntime tsw_runtime{data_root};
  tsw_runtime.set_cache_limit(0x01000000U);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster;
  test.expect_true(openswd3::rendering::initialize_legacy_raster_geometry(
                       raster, LegacySurfaceGeometry{}),
                   "real action-draw raster initializes");
  const LegacyBlitEffectState effects;
  LegacyRleRowJitterState jitter;
  LegacyActionDrawRuntimePorts ports{
      action_updater, tsw_runtime, framebuffer, raster, effects, jitter,
  };
  LegacyActionRecord record{};
  initialize_legacy_action_record(record);
  record.action_id = 0x232BU;
  record.base_variant = 0U;

  const auto result = update_draw_legacy_action(record, 320, 240, ports);
  test.expect_equal(result.status, LegacyActionDrawStatus::ready,
                    "real action variant resolves through ACT and TSW");
  test.expect_equal(result.action_update_count, u32{1U},
                    "real bridge updates one action record");
  test.expect_equal(result.frame_request_count, u32{1U},
                    "real bridge resolves one TSW frame");
  test.expect_equal(result.draw_count, u32{1U}, "real bridge submits one blit");
  test.expect_equal(result.blit_failure_count, u32{0U},
                    "real frame uses a supported blitter path");
  test.expect_true(
      std::ranges::any_of(framebuffer.physical_pixels(),
                          [](const u16 pixel) { return pixel != 0U; }),
      "real bridge produces nonempty framebuffer pixels");
  test.expect_equal(
      openswd3::rendering::legacy_framebuffer_logical_fnv1a64(framebuffer),
      std::uint64_t{0xE216591950463029ULL},
      "real action-draw framebuffer vector is stable");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_normal_bridge_reads_updated_record(test);
  test_direct_bridge_truncates_slots(test);
  test_masked_bridge_reads_updated_flags(test);
  test_failures_stop_at_original_boundaries(test);
  test_non_error_blit_exits_are_accepted(test);
  if (argument_count == 2) {
    test_real_act_tsw_and_blitter(test, std::filesystem::path{arguments[1]});
  }
  return test.exit_code();
}
