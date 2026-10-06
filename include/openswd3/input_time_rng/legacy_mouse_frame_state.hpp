#pragma once

#include "openswd3/input_time_rng/legacy_input.hpp"

namespace openswd3::input_time_rng {

// Shared normalized mouse coordinates at 0x004A9924/0x004A9928.
// Platform ports override these accessors to borrow current_mouse.
class LegacyMouseFrameStatePort {
public:
    [[nodiscard]] virtual LegacyMouseFrame& mouse_frame_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyMouseFrame&
    mouse_frame_state() const noexcept {
        return state_;
    }

protected:
    LegacyMouseFrameStatePort() = default;
    ~LegacyMouseFrameStatePort() = default;

private:
    LegacyMouseFrame state_{};
};

}  // namespace openswd3::input_time_rng
