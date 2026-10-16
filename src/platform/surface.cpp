#include "platform/surface.h"

#include <algorithm>

namespace swd3
{

Surface::Surface(int width, int height)
{
    Resize(width, height);
}

void Surface::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    this->pixels.assign(static_cast<std::size_t>(width) * height, 0);
}

void Surface::Fill(std::uint16_t color)
{
    std::fill(this->pixels.begin(), this->pixels.end(), color);
}

}  // namespace swd3
