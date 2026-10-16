#pragma once

namespace swd3
{

// swd32.exe 0x0044A240: the game's debug log. The shipped build compiled it to an empty function (it is also the
// default pixel converter, see PixelFormat), so every message is discarded; the calls are kept where the original
// makes them.
template <typename... Args>
void DebugLog(const char* /*format*/, const Args&... /*args*/)
{
}

}  // namespace swd3
