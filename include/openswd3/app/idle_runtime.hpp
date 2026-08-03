#pragma once

#include "openswd3/app/frame_dispatch.hpp"

namespace openswd3::app {

class IdleRuntimePorts {
public:
    virtual ~IdleRuntimePorts() = default;

    virtual void step_video() = 0;
    virtual void maintain_audio() = 0;
    virtual void yield() = 0;
    virtual void step_game_frame() = 0;
    virtual void present_pause() = 0;
};

void run_idle_iteration(const IdleState& state, IdleRuntimePorts& ports);

}  // namespace openswd3::app
