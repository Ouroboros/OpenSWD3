#pragma once

#include <cstdint>
#include <vector>

namespace swd3
{

// A 16-bit RGB565 pixel buffer (the game's screen and off-screen surfaces). Rows are contiguous: the pitch is the
// width.
class Surface
{
public:
    Surface() = default;
    Surface(int width, int height);

    void Resize(int width, int height);
    void Fill(std::uint16_t color);

    int Width() const
    {
        return this->width;
    }

    int Height() const
    {
        return this->height;
    }

    std::uint16_t* Pixels()
    {
        return this->pixels.data();
    }

    const std::uint16_t* Pixels() const
    {
        return this->pixels.data();
    }

    std::uint16_t* Row(int y)
    {
        return this->pixels.data() + static_cast<std::size_t>(y) * this->width;
    }

    const std::uint16_t* Row(int y) const
    {
        return this->pixels.data() + static_cast<std::size_t>(y) * this->width;
    }

private:
    int width = 0;
    int height = 0;
    std::vector<std::uint16_t> pixels;
};

}  // namespace swd3
