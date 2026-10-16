#pragma once

#include <cstdint>
#include <vector>

namespace swd3 {

// A 16-bit RGB565 pixel buffer (the game's screen and off-screen surfaces). Rows are contiguous: the pitch is the
// width.
class Surface {
public:
    Surface() = default;
    Surface(int width, int height);

    void Resize(int width, int height);
    void Fill(std::uint16_t color);

    int Width() const { return m_width; }
    int Height() const { return m_height; }
    std::uint16_t* Pixels() { return m_pixels.data(); }
    const std::uint16_t* Pixels() const { return m_pixels.data(); }
    std::uint16_t* Row(int y) { return m_pixels.data() + static_cast<std::size_t>(y) * m_width; }
    const std::uint16_t* Row(int y) const { return m_pixels.data() + static_cast<std::size_t>(y) * m_width; }

private:
    int m_width = 0;
    int m_height = 0;
    std::vector<std::uint16_t> m_pixels;
};

}  // namespace swd3
