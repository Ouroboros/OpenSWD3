#include "openswd3/world_map/legacy_world_frame_tail.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] compat::i32 wrapping_increment(const compat::i32 value) noexcept {
  return std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(value) + 1U);
}

[[nodiscard]] compat::i32
reflected_direction(const compat::i32 direction) noexcept {
  const compat::u32 bits = std::bit_cast<compat::u32>(direction);
  return std::bit_cast<compat::i32>(bits == 1U ? 0xFFFFFFFFU : 1U);
}

} // namespace

void advance_legacy_world_tile_layer_animation(
    LegacyWorldTileLayerAnimationState &state) noexcept {
  state.cycle_counter = wrapping_increment(state.cycle_counter);
  if (state.cycle_counter < state.cycle_interval) {
    return;
  }
  if (state.frame_count == 1U) {
    return;
  }

  state.frame_index += std::bit_cast<compat::u32>(state.frame_direction);
  state.cycle_counter = 0;
  if (state.frame_index >= state.frame_count) {
    state.frame_direction = reflected_direction(state.frame_direction);
    state.frame_index += std::bit_cast<compat::u32>(state.frame_direction) * 2U;
  }
  state.tile_layer_offset = state.tile_layer_stride * state.frame_index;
}

void restore_legacy_world_viewport_after_selection_scroll(
    LegacyWorldCameraRect &camera,
    const LegacyWorldViewportRestoreState &state) noexcept {
  if (state.first_selection_word == 0xCFCFU || state.map_id == 0x16U) {
    return;
  }

  camera.left = state.saved_left;
  camera.top = state.saved_top;
  camera.right = state.saved_left + 640U;
  camera.bottom = state.saved_top + 480U;
}

} // namespace openswd3::world_map
