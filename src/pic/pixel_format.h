#pragma once

#include <cstdint>

#include "swd3/ddraw.h"

namespace swd3 {

using PixelConvertFn = void(std::uint16_t* pixels, int count);

void GetColorMasks(std::int32_t* red, std::int32_t* green, std::int32_t* blue);
int PixelFormatInit(w32::IDirectDrawSurface4* surface);
void PixelConvert(std::uint16_t* pixels, int count);
void PixelConvertBack(std::uint16_t* pixels, int count);
void PixelConvert5551(std::uint16_t* pixels, int count);
void PixelConvert555To565(std::uint16_t* pixels, int count);
void PixelConvert565To555(std::uint16_t* pixels, int count);
void PixelConvert6551(std::uint16_t* pixels, int count);
std::uint32_t MakeColor(std::uint32_t red, std::uint32_t green, std::uint32_t blue);
void PixelConvertNone(std::uint16_t* pixels, int count);

}  // namespace swd3
