#pragma once

#include <cstdint>
#include <vector>

namespace swd3
{

// A loaded picture (the 0x18-byte slot of the tsw caches): RLE data with its header, or raw pixels, plus the palette
// of an 8-bit picture until it is converted to 16 bits.
struct Picture
{
    std::vector<std::uint8_t> data;
    std::vector<std::uint16_t> palette;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
};

// A picture decoded from RLE: 16-bit pixels (format 0x10) or palette indices (format 8).
struct RawPicture
{
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    int format = 0;
};

}  // namespace swd3
