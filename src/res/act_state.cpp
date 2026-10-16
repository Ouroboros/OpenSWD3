#include "res/act_state.h"

namespace swd3
{

// swd32.exe 0x0040DC00
void ResetPlayback(ActState& state)
{
    state.unknown1C = -1;
    state.unknown20 = -1;
    state.unknown3C = -1;
    state.speed = 0;
    state.frameDelay = 0;
    state.delay = 0;
    state.position = 0;
    state.hold = 0;
}

}  // namespace swd3
