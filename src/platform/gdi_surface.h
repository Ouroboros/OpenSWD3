#pragma once

#include <cstdint>

// GDI device contexts over emulated DirectDraw surfaces (IDirectDrawSurface4::GetDC / ReleaseDC). On Windows
// the surface pixels are mirrored into a 16-bit RGB565 DIB section for real GDI drawing; elsewhere no DC is
// available.
namespace swd3::platform {

void* beginSurfaceDC(const std::uint16_t* pixels, int width, int height, int pitchBytes);
void endSurfaceDC(void* dc, std::uint16_t* pixels, int width, int height, int pitchBytes);

}  // namespace swd3::platform
