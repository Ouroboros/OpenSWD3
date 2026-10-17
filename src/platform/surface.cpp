#include "platform/surface.h"

#include <algorithm>
#include <climits>

namespace swd3
{

namespace
{

constexpr std::size_t kStoredFrames = 3;  // a frame-sized guard before and after the pixels

}  // namespace

Surface::Surface(int width, int height)
{
    this->Resize(width, height);
}

void Surface::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    this->storage.assign(kStoredFrames * this->Guard(), 0);
}

void Surface::Fill(u16 color)
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

u16* Surface::Pixels()
{
    return this->storage.data() + this->Guard();
}

const u16* Surface::Pixels() const
{
    return this->storage.data() + this->Guard();
}

u16* Surface::Row(int y)
{
    return this->Pixels() + static_cast<std::size_t>(y) * this->width;
}

const u16* Surface::Row(int y) const
{
    return this->Pixels() + static_cast<std::size_t>(y) * this->width;
}

std::size_t Surface::Guard() const
{
    return static_cast<std::size_t>(this->width) * this->height;
}

Pixel DevicePixel(u32 color)
{
    constexpr u32 kByteMask = 0xFF;
    constexpr int kRedDrop = 3;  // 8-bit channels to RGB565
    constexpr int kGreenDrop = 2;
    constexpr int kBlueDrop = 3;
    constexpr int kRedShift = 11;
    constexpr int kGreenShift = 5;
    constexpr int kBlueAt = 2 * CHAR_BIT;  // the third byte
    const u32 red = color & kByteMask;
    const u32 green = (color >> CHAR_BIT) & kByteMask;
    const u32 blue = (color >> kBlueAt) & kByteMask;
    return static_cast<Pixel>(
        ((red >> kRedDrop) << kRedShift) | ((green >> kGreenDrop) << kGreenShift) | (blue >> kBlueDrop)
    );
}

}  // namespace swd3
