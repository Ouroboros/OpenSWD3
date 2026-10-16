#include "platform/surface.h"

#include <algorithm>

namespace swd3 {

Surface::Surface(int width, int height) { Resize(width, height); }

void Surface::Resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_pixels.assign(static_cast<std::size_t>(width) * height, 0);
}

void Surface::Fill(std::uint16_t color) { std::fill(m_pixels.begin(), m_pixels.end(), color); }

}  // namespace swd3
