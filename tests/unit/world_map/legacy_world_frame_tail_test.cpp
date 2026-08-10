#include "test.hpp"

#include "openswd3/world_map/legacy_world_frame_tail.hpp"

#include <limits>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::world_map::advance_legacy_world_tile_layer_animation;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldTileLayerAnimationState;
using openswd3::world_map::LegacyWorldViewportRestoreState;
using openswd3::world_map::restore_legacy_world_viewport_after_selection_scroll;

void test_cycle_threshold(openswd3::test::Context &test) {
  LegacyWorldTileLayerAnimationState state{
      .cycle_counter = 3,
      .cycle_interval = 5,
      .frame_count = 4U,
      .frame_index = 1U,
      .frame_direction = 1,
      .tile_layer_stride = 100U,
      .tile_layer_offset = 100U,
  };

  advance_legacy_world_tile_layer_animation(state);
  test.expect_true(
      state.cycle_counter == 4 && state.frame_index == 1U &&
          state.frame_direction == 1 && state.tile_layer_offset == 100U,
      "a signed counter below the map cycle interval does not advance");

  state.cycle_counter = std::numeric_limits<i32>::max();
  state.cycle_interval = 0;
  advance_legacy_world_tile_layer_animation(state);
  test.expect_true(
      state.cycle_counter == std::numeric_limits<i32>::min() &&
          state.frame_index == 1U && state.tile_layer_offset == 100U,
      "cycle counter increment and signed comparison preserve x86 wrap");
}

void test_single_frame_counter_quirk(openswd3::test::Context &test) {
  LegacyWorldTileLayerAnimationState state{
      .cycle_counter = 4,
      .cycle_interval = 5,
      .frame_count = 1U,
      .frame_index = 0U,
      .frame_direction = 1,
      .tile_layer_stride = 100U,
      .tile_layer_offset = 77U,
  };

  advance_legacy_world_tile_layer_animation(state);
  test.expect_true(
      state.cycle_counter == 5 && state.frame_index == 0U &&
          state.frame_direction == 1 && state.tile_layer_offset == 77U,
      "a one-frame map keeps its reached counter and prior layer offset");
}

void test_normal_advance(openswd3::test::Context &test) {
  LegacyWorldTileLayerAnimationState state{
      .cycle_counter = 4,
      .cycle_interval = 5,
      .frame_count = 4U,
      .frame_index = 1U,
      .frame_direction = 1,
      .tile_layer_stride = 100U,
      .tile_layer_offset = 100U,
  };

  advance_legacy_world_tile_layer_animation(state);
  test.expect_true(state.cycle_counter == 0 && state.frame_index == 2U &&
                       state.frame_direction == 1 &&
                       state.tile_layer_offset == 200U,
                   "an interior frame advances in the current direction");
}

void test_ping_pong_boundaries(openswd3::test::Context &test) {
  LegacyWorldTileLayerAnimationState upper{
      .cycle_counter = 4,
      .cycle_interval = 5,
      .frame_count = 4U,
      .frame_index = 3U,
      .frame_direction = 1,
      .tile_layer_stride = 100U,
  };
  advance_legacy_world_tile_layer_animation(upper);
  test.expect_true(upper.cycle_counter == 0 && upper.frame_index == 2U &&
                       upper.frame_direction == -1 &&
                       upper.tile_layer_offset == 200U,
                   "the upper endpoint reflects to the previous frame");

  LegacyWorldTileLayerAnimationState lower{
      .cycle_counter = 4,
      .cycle_interval = 5,
      .frame_count = 4U,
      .frame_index = 0U,
      .frame_direction = -1,
      .tile_layer_stride = 100U,
  };
  advance_legacy_world_tile_layer_animation(lower);
  test.expect_true(lower.cycle_counter == 0 && lower.frame_index == 1U &&
                       lower.frame_direction == 1 &&
                       lower.tile_layer_offset == 100U,
                   "the unsigned lower underflow reflects to frame one");
}

void test_noncanonical_animation_values(openswd3::test::Context &test) {
  LegacyWorldTileLayerAnimationState direction{
      .cycle_counter = 0,
      .cycle_interval = 1,
      .frame_count = 4U,
      .frame_index = 3U,
      .frame_direction = 2,
      .tile_layer_stride = 0x80000001U,
  };
  advance_legacy_world_tile_layer_animation(direction);
  test.expect_true(
      direction.frame_index == 7U && direction.frame_direction == 1 &&
          direction.tile_layer_offset == 0x80000007U,
      "the DEC NEG SBB reflection maps every non-one direction to plus one");

  LegacyWorldTileLayerAnimationState zero_frames{
      .cycle_counter = 0,
      .cycle_interval = 1,
      .frame_count = 0U,
      .frame_index = 0U,
      .frame_direction = 1,
      .tile_layer_stride = 3U,
  };
  advance_legacy_world_tile_layer_animation(zero_frames);
  test.expect_true(zero_frames.frame_index == 0xFFFFFFFFU &&
                       zero_frames.frame_direction == -1 &&
                       zero_frames.tile_layer_offset == 0xFFFFFFFDU,
                   "zero frame count follows the original unsigned bounce bug");
}

void test_viewport_restore_gates(openswd3::test::Context &test) {
  const LegacyWorldCameraRect original{10U, 20U, 650U, 500U};
  LegacyWorldCameraRect camera = original;
  restore_legacy_world_viewport_after_selection_scroll(
      camera, LegacyWorldViewportRestoreState{
                  .first_selection_word = 0xCFCFU,
                  .map_id = 1U,
                  .saved_left = 100U,
                  .saved_top = 200U,
              });
  test.expect_true(camera.left == original.left && camera.top == original.top &&
                       camera.right == original.right &&
                       camera.bottom == original.bottom,
                   "the empty selection sentinel skips viewport restoration");

  restore_legacy_world_viewport_after_selection_scroll(
      camera, LegacyWorldViewportRestoreState{
                  .first_selection_word = 0U,
                  .map_id = 0x16U,
                  .saved_left = 100U,
                  .saved_top = 200U,
              });
  test.expect_true(
      camera.left == original.left && camera.top == original.top &&
          camera.right == original.right && camera.bottom == original.bottom,
      "map 22 skips viewport restoration even with an active selection");
}

void test_viewport_restore_and_wrap(openswd3::test::Context &test) {
  LegacyWorldCameraRect camera{};
  restore_legacy_world_viewport_after_selection_scroll(
      camera, LegacyWorldViewportRestoreState{
                  .first_selection_word = 0U,
                  .map_id = 24U,
                  .saved_left = 0xFFFFFF00U,
                  .saved_top = 0xFFFFFF80U,
              });
  test.expect_true(
      camera.left == 0xFFFFFF00U && camera.top == 0xFFFFFF80U &&
          camera.right == 0x00000180U && camera.bottom == 0x00000160U,
      "active selection restores the saved 640x480 rectangle with u32 wrap");
}

} // namespace

int main() {
  openswd3::test::Context test;
  test_cycle_threshold(test);
  test_single_frame_counter_quirk(test);
  test_normal_advance(test);
  test_ping_pong_boundaries(test);
  test_noncanonical_animation_values(test);
  test_viewport_restore_gates(test);
  test_viewport_restore_and_wrap(test);
  return test.exit_code();
}
