#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_directional_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::kLegacyAniDirectionalActionId;
using openswd3::asset_runtime::kLegacyAniDirectionalPhysicalSlotCount;
using openswd3::asset_runtime::kLegacyAniDirectionalResetWord;
using openswd3::asset_runtime::kLegacyAniDirectionalServiceId;
using openswd3::asset_runtime::kLegacyAniDirectionalUpdatedSlotCount;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyAniDirectionalColorSlot;
using openswd3::asset_runtime::LegacyAniDirectionalConfiguration;
using openswd3::asset_runtime::LegacyAniDirectionalEffect;
using openswd3::asset_runtime::LegacyAniDirectionalFrameInput;
using openswd3::asset_runtime::LegacyAniDirectionalInitializationStatus;
using openswd3::asset_runtime::LegacyAniDirectionalMotionSlot;
using openswd3::asset_runtime::LegacyAniDirectionalPorts;
using openswd3::asset_runtime::LegacyAniDirectionalRuntimePorts;
using openswd3::asset_runtime::LegacyAniDirectionalServicePort;
using openswd3::asset_runtime::LegacyAniDirectionalStatus;
using openswd3::asset_runtime::LegacyAniDirectionalTimingSlot;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::initialize_legacy_raster_geometry;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

constexpr LegacyAniDirectionalConfiguration kConfiguration{
    .map_width_tiles = 100,
    .map_height_tiles = 80,
    .base_variant = 0,
    .variant_count = 4,
    .spawn_direction = 0,
};

constexpr LegacyAniDirectionalFrameInput kFrame{
    .movement_scale = 4,
    .player_delta_x = 3,
    .player_delta_y = -5,
    .camera_x = 20,
    .camera_y = 30,
};

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 opacity_step{};
  i32 color_offset{};
};

class FakeServices final : public LegacyAniDirectionalServicePort {
public:
  [[nodiscard]] bool service_enabled(const u32 service_id) override {
    ++call_count;
    last_service_id = service_id;
    return enabled;
  }

  bool enabled{};
  u32 call_count{};
  u32 last_service_id{};
};

class FakePorts final : public LegacyAniDirectionalPorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    const std::size_t index = variants.size();
    action_ids.push_back(record.action_id);
    variants.push_back(record.base_variant);
    if (index == failed_update_call) {
      return LegacyActionUpdateStatus::stream_load_failed;
    }
    record.draw_offset_x = static_cast<u32>(10U + index);
    record.draw_offset_y = static_cast<u32>(20U + index);
    record.field_4a = static_cast<u16>(100U + index);
    record.field_4c = static_cast<u16>(200U + index);
    record.field_8a = static_cast<openswd3::compat::u8>(7U + index);
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    const std::size_t index = loads.size();
    loads.emplace_back(resource_id, frame_index);
    if (index == failed_load_call) {
      return false;
    }
    piece.width = static_cast<u16>(8U + index);
    piece.height = static_cast<u16>(6U + index);
    return true;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 opacity_step,
                   const i32 color_offset) noexcept override {
    const std::size_t index = draws.size();
    draws.push_back(DrawCall{
        .x = destination_x,
        .y = destination_y,
        .flags = flags,
        .opacity_step = opacity_step,
        .color_offset = color_offset,
    });
    if (index == failed_draw_call) {
      return LegacyBlitExecutionStatus::malformed_source;
    }
    return LegacyBlitExecutionStatus::completed;
  }

  std::size_t failed_update_call{std::numeric_limits<std::size_t>::max()};
  std::size_t failed_load_call{std::numeric_limits<std::size_t>::max()};
  std::size_t failed_draw_call{std::numeric_limits<std::size_t>::max()};
  std::vector<u32> action_ids;
  std::vector<u32> variants;
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
};

[[nodiscard]] LegacyActionRecord make_action_record() {
  LegacyActionRecord record{};
  initialize_legacy_action_record(record);
  record.action_id = kLegacyAniDirectionalActionId;
  record.base_variant = 0x38U;
  return record;
}

void set_second_slot_outside(LegacyAniDirectionalEffect &effect) {
  effect.state().motion[1U] = LegacyAniDirectionalMotionSlot{
      .world_x = -641,
      .world_y = 0,
  };
}

void test_physical_groups_reset_and_initialization(
    openswd3::test::Context &test) {
  test.expect_equal(sizeof(LegacyAniDirectionalMotionSlot), std::size_t{0x10U},
                    "the motion group retains its 0x10-byte stride");
  test.expect_equal(sizeof(LegacyAniDirectionalColorSlot), std::size_t{0x10U},
                    "the color group retains its 0x10-byte stride");
  test.expect_equal(sizeof(LegacyAniDirectionalTimingSlot), std::size_t{0x10U},
                    "the timing group retains its 0x10-byte stride");
  test.expect_equal(kLegacyAniDirectionalPhysicalSlotCount, std::size_t{4U},
                    "the loader initializes four physical slots");
  test.expect_equal(kLegacyAniDirectionalUpdatedSlotCount, std::size_t{2U},
                    "the frame function updates only the first two slots");

  LegacyAniDirectionalEffect effect;
  const i32 reset_value = std::bit_cast<i32>(kLegacyAniDirectionalResetWord);
  test.expect_true(
      std::ranges::all_of(effect.state().motion,
                          [](const LegacyAniDirectionalMotionSlot &slot) {
                            return slot.world_x == reset_value &&
                                   slot.world_y == reset_value &&
                                   slot.velocity_x == reset_value &&
                                   slot.velocity_y == reset_value;
                          }),
      "startup fills the full motion block with 0x0f bytes");

  effect.state().color[0U].current_offset = -7;
  effect.state().timing[0U].frame_counter = 9;
  effect.state().timing[0U].current_interval = 6;
  effect.state().motion[0U] = LegacyAniDirectionalMotionSlot{};
  effect.reset_motion_block();
  test.expect_equal(effect.state().color[0U].current_offset, i32{-7},
                    "motion reset preserves the remote color group");
  test.expect_equal(effect.state().timing[0U].frame_counter, i32{9},
                    "motion reset preserves the remote timing group");

  LegacySecondaryRng random;
  random.seed(39U);
  LegacyAniDirectionalConfiguration configuration = kConfiguration;
  configuration.base_variant = 10U;
  const auto initialized = effect.initialize_slots(configuration, random);
  test.expect_equal(initialized.status,
                    LegacyAniDirectionalInitializationStatus::ready,
                    "four-slot initialization completes");
  test.expect_equal(initialized.initialized_slot_count, u32{4U},
                    "all four physical slots initialize");
  test.expect_equal(initialized.random_call_count, u32{24U},
                    "six bounded calls are consumed per physical slot");
  test.expect_equal(random.index(), std::size_t{48U},
                    "the fixed initialization vector consumes 48 raw words");

  const std::array<LegacyAniDirectionalMotionSlot, 4U> expected_motion{
      LegacyAniDirectionalMotionSlot{1491, 896, -1, -2},
      LegacyAniDirectionalMotionSlot{995, 1170, -2, -2},
      LegacyAniDirectionalMotionSlot{1422, 850, -2, -2},
      LegacyAniDirectionalMotionSlot{1246, 369, -1, -2},
  };
  const std::array<i32, 4U> expected_target_intervals{1, 3, 2, 1};
  const std::array<i32, 4U> expected_variants{11, 10, 11, 10};
  for (std::size_t index = 0U; index < expected_motion.size(); ++index) {
    test.expect_equal(effect.state().motion[index].world_x,
                      expected_motion[index].world_x,
                      "initialized x follows the fixed RNG vector");
    test.expect_equal(effect.state().motion[index].world_y,
                      expected_motion[index].world_y,
                      "initialized y follows the fixed RNG vector");
    test.expect_equal(effect.state().motion[index].velocity_x,
                      expected_motion[index].velocity_x,
                      "initialized horizontal velocity follows RNG");
    test.expect_equal(effect.state().motion[index].velocity_y,
                      expected_motion[index].velocity_y,
                      "initialized vertical velocity follows RNG");
    test.expect_equal(effect.state().timing[index].target_interval,
                      expected_target_intervals[index],
                      "initialized target interval is random(3)+1");
    test.expect_equal(effect.state().timing[index].variant,
                      expected_variants[index],
                      "initialized variant includes the base offset");
    test.expect_equal(effect.state().color[index].target_offset, i32{0},
                      "initialization resets only the target color");
  }
  test.expect_equal(effect.state().color[0U].current_offset, i32{-7},
                    "initialization preserves current color");
  test.expect_equal(effect.state().timing[0U].frame_counter, i32{9},
                    "initialization preserves the frame counter");
  test.expect_equal(effect.state().timing[0U].current_interval, i32{6},
                    "initialization preserves the current interval");
}

void test_disabled_path(openswd3::test::Context &test) {
  LegacyAniDirectionalEffect effect;
  LegacySecondaryRng random;
  random.seed(39U);
  FakeServices services;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();
  const auto result =
      effect.update(kConfiguration, kFrame, random, services, action, ports);

  test.expect_equal(result.status, LegacyAniDirectionalStatus::disabled,
                    "service five false returns immediately");
  test.expect_equal(result.service_query_count, u32{1U},
                    "the service is queried exactly once");
  test.expect_equal(services.last_service_id, kLegacyAniDirectionalServiceId,
                    "the gate uses service id five");
  test.expect_equal(random.index(), std::size_t{0U},
                    "disabled path consumes no RNG state");
  test.expect_true(ports.draws.empty(),
                   "disabled path produces no asset or draw work");
}

void test_inclusive_bounds_without_respawn(openswd3::test::Context &test) {
  const auto expect_outside = [&test](const i32 x, const i32 y,
                                      const char *message) {
    LegacyAniDirectionalEffect effect;
    effect.state().motion[0U] =
        LegacyAniDirectionalMotionSlot{.world_x = x, .world_y = y};
    set_second_slot_outside(effect);
    LegacySecondaryRng random;
    random.seed(39U);
    FakeServices services;
    services.enabled = true;
    FakePorts ports;
    LegacyActionRecord action = make_action_record();
    const auto result =
        effect.update(kConfiguration, kFrame, random, services, action, ports);
    test.expect_equal(result.status, LegacyAniDirectionalStatus::ready,
                      message);
    test.expect_equal(result.skipped_outside_slot_count, u32{2U},
                      "both outside slots fail the one-percent respawn gate");
    test.expect_equal(
        result.random_call_count, u32{2U},
        "every physical update slot consumes its probability roll");
    test.expect_equal(effect.state().motion[0U].world_x, x,
                      "failed respawn leaves outside x unchanged");
    test.expect_equal(effect.state().motion[0U].world_y, y,
                      "failed respawn leaves outside y unchanged");
    test.expect_true(ports.draws.empty(), "outside slots do not draw");
  };

  expect_outside(-640, 0, "the left bound is inclusive");
  expect_outside(2240, 0, "the right bound is inclusive");
  expect_outside(0, -320, "the top bound is inclusive");
  expect_outside(0, 1600, "the bottom bound is inclusive");
}

void test_inside_timer_motion_color_and_draw(openswd3::test::Context &test) {
  LegacyAniDirectionalEffect effect;
  effect.state().motion[0U] = LegacyAniDirectionalMotionSlot{
      .world_x = 100,
      .world_y = 200,
      .velocity_x = 2,
      .velocity_y = -1,
  };
  effect.state().color[0U] = LegacyAniDirectionalColorSlot{
      .current_offset = -1,
      .target_offset = 0,
  };
  effect.state().timing[0U] = LegacyAniDirectionalTimingSlot{
      .frame_counter = 3,
      .variant = 10,
      .target_interval = 5,
      .current_interval = 3,
  };
  effect.state().motion[1U] = LegacyAniDirectionalMotionSlot{
      .world_x = 300,
      .world_y = 400,
      .velocity_x = 99,
      .velocity_y = 99,
  };
  effect.state().color[1U].current_offset = 5;
  effect.state().timing[1U] = LegacyAniDirectionalTimingSlot{
      .frame_counter = 0,
      .variant = 11,
      .target_interval = 2,
      .current_interval = 5,
  };

  LegacySecondaryRng random;
  random.seed(32U);
  FakeServices services;
  services.enabled = true;
  FakePorts ports;
  ports.failed_draw_call = 0U;
  LegacyActionRecord action = make_action_record();
  const auto result =
      effect.update(kConfiguration, kFrame, random, services, action, ports);

  test.expect_equal(result.status, LegacyAniDirectionalStatus::ready,
                    "inside slots complete despite a blitter error");
  test.expect_equal(result.random_call_count, u32{4U},
                    "roll 3 adds interval and color RNG before slot two");
  test.expect_equal(random.index(), std::size_t{8U},
                    "four bounded calls consume eight raw words");
  test.expect_equal(result.moved_slot_count, u32{1U},
                    "only the expired frame counter moves");
  test.expect_equal(result.action_update_count, u32{2U},
                    "both inside slots update the shared action record");
  test.expect_equal(result.draw_count, u32{2U}, "both inside slots draw");
  test.expect_equal(result.blit_failure_count, u32{1U},
                    "the original ignored draw failure remains diagnostic");

  test.expect_equal(effect.state().motion[0U].world_x, i32{105},
                    "x adds velocity and scaled player movement");
  test.expect_equal(effect.state().motion[0U].world_y, i32{194},
                    "negative player movement uses arithmetic shift");
  test.expect_equal(effect.state().timing[0U].frame_counter, i32{0},
                    "movement resets the frame counter");
  test.expect_equal(effect.state().timing[0U].target_interval, i32{1},
                    "roll below 25 publishes random(5)+1");
  test.expect_equal(effect.state().timing[0U].current_interval, i32{2},
                    "current interval moves one step toward its target");
  test.expect_equal(effect.state().color[0U].target_offset, i32{-3},
                    "roll below five publishes random(4)-5");
  test.expect_equal(effect.state().color[0U].current_offset, i32{-2},
                    "current color moves one step toward its target");

  test.expect_equal(effect.state().motion[1U].world_x, i32{300},
                    "an unexpired timer preserves slot-two x");
  test.expect_equal(effect.state().motion[1U].world_y, i32{400},
                    "an unexpired timer preserves slot-two y");
  test.expect_equal(effect.state().timing[1U].frame_counter, i32{1},
                    "the unexpired counter remains incremented");

  test.expect_equal(ports.action_ids,
                    std::vector<u32>{kLegacyAniDirectionalActionId,
                                     kLegacyAniDirectionalActionId},
                    "both updates force action 0x232b");
  test.expect_equal(ports.variants, std::vector<u32>{10U, 11U},
                    "each slot publishes its parallel variant");
  test.expect_equal(ports.loads[0U], std::pair<u16, u16>{100U, 200U},
                    "first updated action supplies the first TSW key");
  test.expect_equal(ports.loads[1U], std::pair<u16, u16>{101U, 201U},
                    "second update advances the same shared action record");
  test.expect_equal(ports.draws[0U].x, i32{75},
                    "first draw subtracts action and camera x");
  test.expect_equal(ports.draws[0U].y, i32{144},
                    "first draw subtracts action and camera y");
  test.expect_equal(ports.draws[1U].x, i32{269},
                    "second draw uses its own updated action offset");
  test.expect_equal(ports.draws[1U].y, i32{349},
                    "second draw preserves its timer-delayed position");
  test.expect_equal(ports.draws[0U].flags, u32{4U},
                    "four variants select draw flags four");
  test.expect_equal(ports.draws[0U].opacity_step, i32{7},
                    "action byte 0x8a is zero-extended into opacity");
  test.expect_equal(ports.draws[0U].color_offset, i32{-2},
                    "the first current color reaches all RGB channels");
  test.expect_equal(ports.draws[1U].color_offset, i32{5},
                    "the second slot retains an independent color");
}

void test_four_spawn_directions(openswd3::test::Context &test) {
  constexpr std::array<i32, 4U> expected_y{1398, 1398, -300, -300};
  constexpr std::array<i32, 4U> expected_velocity_x{-2, 0, -2, 0};
  constexpr std::array<i32, 4U> expected_velocity_y{-2, 1, -1, 1};

  for (u32 direction = 0U; direction < 4U; ++direction) {
    LegacyAniDirectionalEffect effect;
    effect.state().motion[0U] =
        LegacyAniDirectionalMotionSlot{.world_x = -640, .world_y = 0};
    set_second_slot_outside(effect);
    LegacySecondaryRng random;
    random.seed(192U);
    FakeServices services;
    services.enabled = true;
    FakePorts ports;
    LegacyActionRecord action = make_action_record();
    LegacyAniDirectionalConfiguration configuration = kConfiguration;
    configuration.base_variant = 20U;
    configuration.spawn_direction = direction;
    const auto result =
        effect.update(configuration, kFrame, random, services, action, ports);

    test.expect_equal(result.status, LegacyAniDirectionalStatus::ready,
                      "directional respawn completes");
    test.expect_equal(result.respawned_slot_count, u32{1U},
                      "roll 994 respawns the first outside slot");
    test.expect_equal(result.skipped_outside_slot_count, u32{1U},
                      "the second outside slot fails with roll 836");
    test.expect_equal(result.random_call_count, u32{5U},
                      "variant binning reuses the roll, preserving RNG order");
    test.expect_equal(effect.state().color[0U].target_offset, i32{0},
                      "respawn clears the target color");
    test.expect_equal(effect.state().timing[0U].target_interval, i32{10},
                      "roll 994 maps to target interval ten");
    test.expect_equal(effect.state().timing[0U].variant, i32{23},
                      "respawn variant adds its configured base");
    test.expect_equal(effect.state().motion[0U].world_x, i32{211},
                      "all four cases share the same x RNG position");
    test.expect_equal(effect.state().motion[0U].world_y, expected_y[direction],
                      "direction chooses bottom or top spawn y");
    test.expect_equal(effect.state().motion[0U].velocity_x,
                      expected_velocity_x[direction],
                      "direction selects signed horizontal RNG formula");
    test.expect_equal(effect.state().motion[0U].velocity_y,
                      expected_velocity_y[direction],
                      "direction selects signed vertical RNG formula");
    test.expect_true(ports.draws.empty(),
                     "a respawned slot does not draw in the same frame");
  }
}

void test_i32_wrap_and_alternate_draw_flags(openswd3::test::Context &test) {
  LegacyAniDirectionalEffect effect;
  effect.state().motion[0U] = LegacyAniDirectionalMotionSlot{
      .world_x = 2147483620,
      .world_y = 2147483620,
      .velocity_x = 100,
      .velocity_y = 100,
  };
  effect.state().timing[0U] = LegacyAniDirectionalTimingSlot{
      .frame_counter = 0,
      .variant = 0,
      .target_interval = -1,
      .current_interval = -1,
  };
  effect.state().motion[1U] =
      LegacyAniDirectionalMotionSlot{.world_x = 300, .world_y = 400};
  effect.state().timing[1U] = LegacyAniDirectionalTimingSlot{
      .frame_counter = std::numeric_limits<i32>::max(),
      .variant = 1,
      .target_interval = 0,
      .current_interval = 0,
  };

  LegacyAniDirectionalConfiguration configuration = kConfiguration;
  configuration.map_width_tiles = 134217687;
  configuration.map_height_tiles = 134217707;
  configuration.variant_count = 8U;
  const LegacyAniDirectionalFrameInput frame{};
  LegacySecondaryRng random;
  random.seed(39U);
  FakeServices services;
  services.enabled = true;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();
  const auto result =
      effect.update(configuration, frame, random, services, action, ports);

  test.expect_equal(result.status, LegacyAniDirectionalStatus::ready,
                    "wrapping boundary state remains executable");
  test.expect_equal(result.moved_slot_count, u32{1U},
                    "only the first signed timer expires");
  test.expect_equal(effect.state().motion[0U].world_x, i32{-2147483576},
                    "x motion wraps across INT32_MAX");
  test.expect_equal(effect.state().motion[0U].world_y, i32{-2147483576},
                    "y motion wraps across INT32_MAX");
  test.expect_equal(effect.state().timing[1U].frame_counter,
                    std::numeric_limits<i32>::min(),
                    "frame counter increment wraps before signed comparison");
  test.expect_equal(ports.draws.size(), std::size_t{2U},
                    "both in-range slots still draw");
  test.expect_equal(ports.draws[0U].flags, u32{0x2CU},
                    "variant counts other than four select flags 0x2c");
  test.expect_equal(ports.draws[1U].flags, u32{0x2CU},
                    "alternate flags apply independently to both slots");
}

void test_invalid_direction_and_random_bounds(openswd3::test::Context &test) {
  LegacyAniDirectionalEffect invalid_direction;
  invalid_direction.state().motion[0U] =
      LegacyAniDirectionalMotionSlot{.world_x = -640, .world_y = 77};
  set_second_slot_outside(invalid_direction);
  LegacySecondaryRng direction_random;
  direction_random.seed(192U);
  FakeServices services;
  services.enabled = true;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();
  LegacyAniDirectionalConfiguration direction_configuration = kConfiguration;
  direction_configuration.base_variant = 20U;
  direction_configuration.spawn_direction = 4U;
  const auto direction_result =
      invalid_direction.update(direction_configuration, kFrame,
                               direction_random, services, action, ports);
  test.expect_equal(direction_result.invalid_direction_count, u32{1U},
                    "directions above three take the switch default");
  test.expect_equal(direction_result.random_call_count, u32{2U},
                    "invalid direction skips x and velocity RNG calls");
  test.expect_equal(invalid_direction.state().motion[0U].world_x, i32{-640},
                    "invalid direction preserves outside motion state");
  test.expect_equal(invalid_direction.state().motion[0U].world_y, i32{77},
                    "invalid direction preserves outside y");
  test.expect_equal(invalid_direction.state().timing[0U].variant, i32{23},
                    "variant is written before the switch default");

  LegacyAniDirectionalEffect invalid_bound;
  invalid_bound.state().motion[0U] =
      LegacyAniDirectionalMotionSlot{.world_x = -640, .world_y = 0};
  set_second_slot_outside(invalid_bound);
  LegacySecondaryRng bound_random;
  bound_random.seed(192U);
  LegacyAniDirectionalConfiguration bound_configuration = kConfiguration;
  bound_configuration.map_width_tiles = 0;
  const auto bound_result = invalid_bound.update(
      bound_configuration, kFrame, bound_random, services, action, ports);
  test.expect_equal(bound_result.status,
                    LegacyAniDirectionalStatus::invalid_random_bound,
                    "zero map width is isolated before host termination");
  test.expect_equal(bound_result.random_call_count, u32{1U},
                    "the outside probability roll still precedes the failure");
  test.expect_equal(invalid_bound.state().timing[0U].target_interval, i32{10},
                    "pre-failure target interval side effect is preserved");

  LegacyAniDirectionalEffect invalid_initialization;
  LegacySecondaryRng initialization_random;
  initialization_random.seed(39U);
  LegacyAniDirectionalConfiguration initialization_configuration =
      kConfiguration;
  initialization_configuration.variant_count = 0U;
  const auto initialization_result = invalid_initialization.initialize_slots(
      initialization_configuration, initialization_random);
  test.expect_equal(
      initialization_result.status,
      LegacyAniDirectionalInitializationStatus::invalid_random_bound,
      "loader-side random(variant_count) isolates a zero bound");
  test.expect_equal(initialization_result.random_call_count, u32{1U},
                    "loader target interval precedes the invalid variant call");
}

void test_asset_failure_guards(openswd3::test::Context &test) {
  LegacyAniDirectionalEffect effect;
  effect.state().motion[0U] =
      LegacyAniDirectionalMotionSlot{.world_x = 100, .world_y = 200};
  effect.state().timing[0U].current_interval = 5;
  set_second_slot_outside(effect);
  FakeServices services;
  services.enabled = true;
  LegacyActionRecord action = make_action_record();

  LegacySecondaryRng update_random;
  update_random.seed(39U);
  FakePorts update_ports;
  update_ports.failed_update_call = 0U;
  const auto update_failure = effect.update(
      kConfiguration, kFrame, update_random, services, action, update_ports);
  test.expect_equal(update_failure.status,
                    LegacyAniDirectionalStatus::action_update_failed,
                    "ACT failure is isolated at the modern port");
  test.expect_equal(update_failure.random_call_count, u32{1U},
                    "failure stops before the second physical slot");

  LegacySecondaryRng load_random;
  load_random.seed(39U);
  FakePorts load_ports;
  load_ports.failed_load_call = 0U;
  const auto load_failure = effect.update(kConfiguration, kFrame, load_random,
                                          services, action, load_ports);
  test.expect_equal(load_failure.status,
                    LegacyAniDirectionalStatus::frame_load_failed,
                    "TSW failure is isolated before blitting");
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
  test.expect_true(
      initialize_legacy_raster_geometry(raster, LegacySurfaceGeometry{}),
      "real directional-effect raster initializes");
  LegacyBlitEffectState effects;
  LegacyRleRowJitterState jitter;
  LegacyAniDirectionalRuntimePorts ports{
      action_updater, tsw_runtime, framebuffer, raster, effects, jitter,
  };
  LegacyAniDirectionalEffect effect;
  effect.state().motion[0U] =
      LegacyAniDirectionalMotionSlot{.world_x = 320, .world_y = 240};
  effect.state().timing[0U].current_interval = 5;
  effect.state().timing[0U].variant = 0;
  set_second_slot_outside(effect);
  LegacyActionRecord action = make_action_record();
  LegacySecondaryRng random;
  random.seed(39U);
  FakeServices services;
  services.enabled = true;
  const LegacyAniDirectionalFrameInput frame{};

  const auto result =
      effect.update(kConfiguration, frame, random, services, action, ports);
  test.expect_equal(result.status, LegacyAniDirectionalStatus::ready,
                    "real variant zero resolves through ACT and TSW");
  test.expect_equal(result.draw_count, u32{1U},
                    "the inside real slot submits one frame");
  test.expect_equal(result.skipped_outside_slot_count, u32{1U},
                    "the second real slot remains outside");
  test.expect_equal(result.blit_failure_count, u32{0U},
                    "the real frame selects a supported blitter path");
  test.expect_true(
      std::ranges::any_of(framebuffer.physical_pixels(),
                          [](const u16 pixel) { return pixel != 0U; }),
      "real action and TSW data produce nonempty framebuffer pixels");
  const std::uint64_t hash =
      openswd3::rendering::legacy_framebuffer_logical_fnv1a64(framebuffer);
  test.expect_equal(hash, std::uint64_t{0xE216591950463029ULL},
                    "the real variant-zero framebuffer vector is stable");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_physical_groups_reset_and_initialization(test);
  test_disabled_path(test);
  test_inclusive_bounds_without_respawn(test);
  test_inside_timer_motion_color_and_draw(test);
  test_four_spawn_directions(test);
  test_i32_wrap_and_alternate_draw_flags(test);
  test_invalid_direction_and_random_bounds(test);
  test_asset_failure_guards(test);
  if (argument_count == 2) {
    test_real_act_tsw_and_blitter(test, std::filesystem::path{arguments[1]});
  }
  return test.exit_code();
}
