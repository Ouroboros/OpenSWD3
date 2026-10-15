#pragma once

#include <cstdint>

// Colour operations on runs of 16-bit pixels (palettes, line buffers, surface rows).
namespace swd3 {

void TintPixels(std::uint16_t* pixels, std::uint32_t count, int red, int green, int blue);
void ShiftRed(std::uint16_t* pixels, std::uint32_t count, int delta);
void ShiftGreenPairs(std::uint16_t* pixels, std::uint32_t count, int delta);
void ShiftBluePairs(std::uint16_t* pixels, std::uint32_t count, int delta);
void MixPixels(const std::uint16_t* other, std::uint16_t* pixels, std::uint32_t count);
void GrayPixels(std::uint16_t* pixels, std::uint32_t count);

}  // namespace swd3
