#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"

namespace openswd3::world_map {

struct LegacyWorldCameraPanState {
  compat::i32 remaining_x{};
  compat::i32 remaining_y{};
  compat::i32 step_x{};
  compat::i32 step_y{};
};

// Script-driven camera pan advanced by sub_414570. A false result means that
// both remaining axes were already zero and the original function returned
// before touching either the viewport or the step fields.
[[nodiscard]] bool
advance_legacy_world_camera_pan(LegacyWorldCameraRect &camera,
                                LegacyWorldCameraPanState &state) noexcept;

} // namespace openswd3::world_map
