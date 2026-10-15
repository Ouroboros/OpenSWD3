#pragma once

// Miles Sound System (mss32.dll) entry points used by swd32.exe. Implemented over SDL3 audio.
namespace mss {

// Services timers and streams when called from the game loop. Mixing runs on the SDL audio thread,
// so there is nothing to service here.
void AIL_serve();

}  // namespace mss
