#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"

namespace openswd3::world_map {

struct LegacyWorldTileLayerAnimationState {
    compat::i32 cycle_counter{};
    compat::i32 cycle_interval{};
    compat::u32 frame_count{};
    compat::u32 frame_index{};
    compat::i32 frame_direction{};
    compat::u32 tile_layer_stride{};
    compat::u32 tile_layer_offset{};
};

void advance_legacy_world_tile_layer_animation(
    LegacyWorldTileLayerAnimationState& state
) noexcept;

struct LegacyWorldViewportRestoreState {
    compat::u16 first_selection_word{0xCFCFU};
    compat::u32 map_id{};
    compat::u32 saved_left{};
    compat::u32 saved_top{};
};

void restore_legacy_world_viewport_after_selection_scroll(
    LegacyWorldCameraRect& camera, const LegacyWorldViewportRestoreState& state
) noexcept;

}  // namespace openswd3::world_map
