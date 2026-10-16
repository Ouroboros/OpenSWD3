#include <cstring>

#include "draw/painter.h"

namespace swd3
{

namespace
{

u32 ReadPair(const u16* pixels)
{
    u32 value = 0;
    std::memcpy(&value, pixels, 4);
    return value;
}

void WritePair(u16* pixels, u32 value)
{
    std::memcpy(pixels, &value, 4);
}

// One channel of two pixels per step (0x00420600 / 0x004206F0). Only the high pixel's overflow is detected with the
// inverse mask shifted up, the low pixel's on the low word; going down, a borrow of the low pixel runs into the high
// pixel's channel. The original steps the count by 2 and stops only at exactly 0, running past the buffer for an odd
// count; here a single trailing pixel is left as it is.
void ShiftChannelPairs(std::span<u16> pixels, int delta, const ColorChannel& channel)
{
    const u32 highInverse = channel.inverseMask << 16;
    const u32 maskPair = channel.mask | (channel.mask << 16);
    const u32 otherBits = 0xFFFFFFFFu - maskPair;
    const auto step = static_cast<i32>(channel.halves[4]);
    u32 offset = static_cast<u32>(delta >= 0 ? step * delta : -(step * delta));
    offset |= offset << 16;
    u32 saturated = channel.mask << 16;

    for (std::size_t i = 0; i + 1 < pixels.size(); i += 2)
    {
        const u32 dest = ReadPair(&pixels[i]);
        u32 value = 0;

        if (delta >= 0)
        {
            value = (dest & maskPair) + offset;

            if ((otherBits & value & 0xFFFF) != 0)
            {
                value = (value & 0xFFFF0000u) | (channel.mask & 0xFFFF);
            }

            if ((highInverse & value) != 0)
            {
                saturated = (saturated & 0xFFFF0000u) | (value & 0xFFFF);
                value = saturated;
            }
        }
        else
        {
            value = (dest & maskPair) - offset;

            if ((otherBits & value & 0xFFFF) != 0)
            {
                value &= 0xFFFF0000u;
            }

            if ((highInverse & value) != 0)
            {
                value &= 0xFFFF;
            }
        }

        WritePair(&pixels[i], (dest & otherBits) | value);
    }
}

}  // namespace

// swd32.exe 0x00420490: tint steps added to every pixel, saturating like the tint effect. The original treats a
// count of 0 as 2^32; an empty span tints nothing here.
void Painter::TintPixels(std::span<u16> pixels, int red, int green, int blue) const
{
    const Tint tint = this->MakeTint(red, green, blue);

    for (u16& pixel : pixels)
    {
        pixel = this->TintPixel(tint, pixel);
    }
}

// swd32.exe 0x00420560: the red channel moved by `delta` tint steps, saturating at the mask or 0
void Painter::ShiftRed(std::span<u16> pixels, int delta) const
{
    const u32 mask = this->red.mask;
    const u32 otherBits = 0xFFFFFFFFu - mask;
    const auto step = static_cast<i32>(this->red.halves[4]);

    for (u16& pixel : pixels)
    {
        const u32 dest = pixel;
        u32 value = 0;

        if (delta >= 0)
        {
            value = (dest & mask) + static_cast<u32>(step * delta);

            if ((otherBits & value) != 0)
            {
                value = mask;
            }
        }
        else
        {
            value = (dest & mask) - static_cast<u32>(-(step * delta));

            if ((otherBits & value) != 0)
            {
                value = 0;
            }
        }

        pixel = static_cast<u16>((dest & otherBits) | value);
    }
}

// swd32.exe 0x00420600: the green channel, two pixels per step
void Painter::ShiftGreenPairs(std::span<u16> pixels, int delta) const
{
    ShiftChannelPairs(pixels, delta, this->green);
}

// swd32.exe 0x004206F0: the blue channel, two pixels per step
void Painter::ShiftBluePairs(std::span<u16> pixels, int delta) const
{
    ShiftChannelPairs(pixels, delta, this->blue);
}

// swd32.exe 0x004207E0: every pixel becomes the per-channel saturated sum of itself and the matching pixel of `other`
// (through the channel ramps). The channel masks are cut to 16 bits first, as in the original.
void Painter::MixPixels(std::span<const u16> other, std::span<u16> pixels)
{
    this->red.mask &= 0xFFFF;
    this->green.mask &= 0xFFFF;
    this->blue.mask &= 0xFFFF;

    for (std::size_t i = 0; i < pixels.size(); ++i)
    {
        const u32 added = other[i];
        const u32 own = pixels[i];

        auto sum = [added, own](const ColorChannel& channel)
        {
            return channel.Ramp(static_cast<int>(((channel.mask & own) + (channel.mask & added)) >> channel.shift));
        };

        pixels[i] = static_cast<u16>(sum(this->blue) + sum(this->green) + sum(this->red));
    }
}

// swd32.exe 0x00421FB0: every pixel grey ((r + g + b) / 4 in each channel)
void Painter::GrayPixels(std::span<u16> pixels) const
{
    for (u16& pixel : pixels)
    {
        pixel = this->Gray(pixel);
    }
}

}  // namespace swd3
