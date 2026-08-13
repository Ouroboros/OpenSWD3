#include "openswd3/world_map/legacy_world_camera_pan.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::u32 delta_bits(const compat::i32 value) noexcept {
    return std::bit_cast<compat::u32>(value);
}

}  // namespace

bool advance_legacy_world_camera_pan(
    LegacyWorldCameraRect& camera, LegacyWorldCameraPanState& state
) noexcept {
    if (state.remaining_x == 0 && state.remaining_y == 0) {
        return false;
    }

    const compat::u32 step_x = delta_bits(state.step_x);
    const compat::u32 step_y = delta_bits(state.step_y);
    camera.left += step_x;
    camera.right += step_x;
    camera.top += step_y;
    camera.bottom += step_y;

    state.remaining_x = wrapping_subtract(state.remaining_x, state.step_x);
    state.remaining_y = wrapping_subtract(state.remaining_y, state.step_y);
    if (state.remaining_x == 0) {
        state.step_x = 0;
    }
    if (state.remaining_y == 0) {
        state.step_y = 0;
    }
    return true;
}

}  // namespace openswd3::world_map
