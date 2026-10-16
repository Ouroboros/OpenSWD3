#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

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
    static constexpr std::uint32_t kRedMask = 0xF800;
    static constexpr std::uint32_t kGreenMask = 0x7E0;
    static constexpr std::uint32_t kBlueMask = 0x1F;

    Surface() = default;
    Surface(int width, int height);

    void Resize(int width, int height);
    void Fill(std::uint16_t color);

    int Width() const;
    int Height() const;
    std::uint16_t* Pixels();
    const std::uint16_t* Pixels() const;
    std::uint16_t* Row(int y);
    const std::uint16_t* Row(int y) const;

private:
    std::size_t Guard() const;

    int width = 0;
    int height = 0;
    std::vector<std::uint16_t> storage;
};

}  // namespace swd3
