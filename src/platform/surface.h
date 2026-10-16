#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/types.h"

namespace swd3
{

// A 16-bit RGB565 pixel buffer (the game's screen and off-screen surfaces). Rows are contiguous: the pitch is the
// width. A guard area of one surface size precedes and follows the pixels: several original draw routines reach
// outside the drawn rectangle (a flipped raw picture starts one picture height lower, the smear effect reads the
// rows above and below), and the guard keeps those accesses inside this buffer. Its content is never shown.
class Surface
{
public:
    // The display format (the game window shows RGB565).
    static constexpr u32 kRedMask = 0xF800;
    static constexpr u32 kGreenMask = 0x7E0;
    static constexpr u32 kBlueMask = 0x1F;

    Surface() = default;
    Surface(int width, int height);

    void Resize(int width, int height);
    void Fill(u16 color);

    int Width() const;
    int Height() const;
    u16* Pixels();
    const u16* Pixels() const;
    u16* Row(int y);
    const u16* Row(int y) const;

private:
    std::size_t Guard() const;

    int width = 0;
    int height = 0;
    std::vector<u16> storage;
};

}  // namespace swd3
