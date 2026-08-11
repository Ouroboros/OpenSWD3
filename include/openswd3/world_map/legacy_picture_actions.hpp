#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <list>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyPictureActionNodeSize = 0xA4U;

struct LegacyPictureActionNode {
  compat::u16 screen_x{};
  compat::u16 screen_y{};
  compat::u16 field_04{};
  compat::u16 field_06{};
  asset_runtime::LegacyActionRecord action{};
  compat::u32 next_pointer_32{};
};

static_assert(sizeof(LegacyPictureActionNode) ==
              kLegacyPictureActionNodeSize);
static_assert(offsetof(LegacyPictureActionNode, action) == 0x08U);
static_assert(offsetof(LegacyPictureActionNode, next_pointer_32) == 0xA0U);

struct LegacyPictureActionLists {
  std::list<LegacyPictureActionNode> primary;
  std::list<LegacyPictureActionNode> secondary;
};

class LegacyPictureActionAudioPorts {
public:
  virtual ~LegacyPictureActionAudioPorts() = default;

  virtual void play_positional_sample(compat::u16 sound_id,
                                      compat::i32 world_x,
                                      compat::i32 world_y) noexcept = 0;
};

struct LegacyPictureActionResult {
  compat::u32 visited_count{};
  compat::u32 action_update_failure_count{};
  compat::u32 frame_request_count{};
  compat::u32 frame_failure_count{};
  compat::u32 draw_count{};
  compat::u32 blit_failure_count{};
  compat::u32 positional_sample_count{};
  compat::u32 removed_count{};
  rendering::LegacyBlitExecutionStatus last_blit_status{
      rendering::LegacyBlitExecutionStatus::completed};
};

// sub_4147E0: update, draw and retire one parent's +0xA0 picture-action list.
// The modern list owns host pointers, while every element retains the exact
// 0xA4 legacy payload, including the original 32-bit next slot at +0xA0.
[[nodiscard]] LegacyPictureActionResult update_draw_legacy_picture_actions(
    std::list<LegacyPictureActionNode> &nodes, compat::i32 camera_left,
    compat::i32 camera_top, asset_runtime::LegacyActionDrawPorts &action_ports,
    LegacyPictureActionAudioPorts &audio_ports);

} // namespace openswd3::world_map
