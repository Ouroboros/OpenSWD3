#include "platform/surface.h"

#include <algorithm>

namespace swd3
{

Surface::Surface(int width, int height)
{
    this->Resize(width, height);
}

void Surface::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    this->storage.assign(3 * this->Guard(), 0);
}

void Surface::Fill(std::uint16_t color)
{
    std::fill_n(this->Pixels(), this->Guard(), color);
}

int Surface::Width() const
{
    return this->width;
}

int Surface::Height() const
{
    return this->height;
}

std::uint16_t* Surface::Pixels()
{
    return this->storage.data() + this->Guard();
}

const std::uint16_t* Surface::Pixels() const
{
    return this->storage.data() + this->Guard();
}

std::uint16_t* Surface::Row(int y)
{
    return this->Pixels() + static_cast<std::size_t>(y) * this->width;
}

const std::uint16_t* Surface::Row(int y) const
{
    return this->Pixels() + static_cast<std::size_t>(y) * this->width;
}

std::size_t Surface::Guard() const
{
    return static_cast<std::size_t>(this->width) * this->height;
}

}  // namespace swd3
