#pragma once

#include <string_view>

namespace swd3
{

// swd32.exe 0x0044A240: the game's debug log. The shipped build compiled it to an empty function (it is also the
// default pixel converter, see PixelFormat), so every message is discarded; the calls are kept where the original
// makes them.
template <typename... Args>
void DebugLog(std::string_view /*format*/, const Args&... /*args*/)
{
}

template <typename... Args>
void DebugLog(std::u8string_view /*format*/, const Args&... /*args*/)
{
}

// swd32.exe 0x00411F90: the error log, also compiled to an empty function in the shipped build.
template <typename... Args>
void ErrorLog(std::string_view /*format*/, const Args&... /*args*/)
{
}

}  // namespace swd3
