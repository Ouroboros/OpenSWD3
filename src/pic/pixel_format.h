#pragma once

#include <cstdint>

namespace swd3 {

using PixelConvertFn = void(std::uint16_t* pixels, int count);

void PixelConvert(std::uint16_t* pixels, int count);
void PixelConvertNone(std::uint16_t* pixels, int count);

}  // namespace swd3
