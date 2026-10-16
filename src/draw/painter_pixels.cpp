#include <cstring>

#include "draw/painter.h"

namespace swd3
{

namespace
{

std::uint32_t ReadPair(const std::uint16_t* pixels)
{
    std::uint32_t value = 0;
    std::memcpy(&value, pixels, 4);
    return value;
}

void WritePair(std::uint16_t* pixels, std::uint32_t value)
{
    std::memcpy(pixels, &value, 4);
}

// One channel of two pixels per step (0x00420600 / 0x004206F0). Only the high pixel's overflow is detected with the
// inverse mask shifted up, the low pixel's on the low word; going down, a borrow of the low pixel runs into the high
// pixel's channel. The original steps the count by 2 and stops only at exactly 0, running past the buffer for an odd
// count; here a single trailing pixel is left as it is.
void ShiftChannelPairs(std::span<std::uint16_t> pixels, int delta, const ColorChannel& channel)
{
    const std::uint32_t highInverse = channel.inverseMask << 16;
    const std::uint32_t maskPair = channel.mask | (channel.mask << 16);
    const std::uint32_t otherBits = 0xFFFFFFFFu - maskPair;
    const auto step = static_cast<std::int32_t>(channel.halves[4]);
    std::uint32_t offset = static_cast<std::uint32_t>(delta >= 0 ? step * delta : -(step * delta));
    offset |= offset << 16;
    std::uint32_t saturated = channel.mask << 16;

    for (std::size_t i = 0; i + 1 < pixels.size(); i += 2)
    {
        const std::uint32_t dest = ReadPair(&pixels[i]);
        std::uint32_t value = 0;

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
void Painter::TintPixels(std::span<std::uint16_t> pixels, int red, int green, int blue) const
{
    const Tint tint = this->MakeTint(red, green, blue);

    for (std::uint16_t& pixel : pixels)
    {
        pixel = this->TintPixel(tint, pixel);
    }
}

// swd32.exe 0x00420560: the red channel moved by `delta` tint steps, saturating at the mask or 0
void Painter::ShiftRed(std::span<std::uint16_t> pixels, int delta) const
{
    const std::uint32_t mask = this->red.mask;
    const std::uint32_t otherBits = 0xFFFFFFFFu - mask;
    const auto step = static_cast<std::int32_t>(this->red.halves[4]);

    for (std::uint16_t& pixel : pixels)
    {
        const std::uint32_t dest = pixel;
        std::uint32_t value = 0;

        if (delta >= 0)
        {
            value = (dest & mask) + static_cast<std::uint32_t>(step * delta);

            if ((otherBits & value) != 0)
            {
                value = mask;
            }
        }
        else
        {
            value = (dest & mask) - static_cast<std::uint32_t>(-(step * delta));

            if ((otherBits & value) != 0)
            {
                value = 0;
            }
        }

        pixel = static_cast<std::uint16_t>((dest & otherBits) | value);
    }
}

// swd32.exe 0x00420600: the green channel, two pixels per step
void Painter::ShiftGreenPairs(std::span<std::uint16_t> pixels, int delta) const
{
    ShiftChannelPairs(pixels, delta, this->green);
}

// swd32.exe 0x004206F0: the blue channel, two pixels per step
void Painter::ShiftBluePairs(std::span<std::uint16_t> pixels, int delta) const
{
    ShiftChannelPairs(pixels, delta, this->blue);
}

// swd32.exe 0x004207E0: every pixel becomes the per-channel saturated sum of itself and the matching pixel of `other`
// (through the channel ramps). The channel masks are cut to 16 bits first, as in the original.
void Painter::MixPixels(std::span<const std::uint16_t> other, std::span<std::uint16_t> pixels)
{
    this->red.mask &= 0xFFFF;
    this->green.mask &= 0xFFFF;
    this->blue.mask &= 0xFFFF;

    for (std::size_t i = 0; i < pixels.size(); ++i)
    {
        const std::uint32_t added = other[i];
        const std::uint32_t own = pixels[i];

        auto sum = [added, own](const ColorChannel& channel)
        {
            return channel.Ramp(static_cast<int>(((channel.mask & own) + (channel.mask & added)) >> channel.shift));
        };

        pixels[i] = static_cast<std::uint16_t>(sum(this->blue) + sum(this->green) + sum(this->red));
    }
}

// swd32.exe 0x00421FB0: every pixel grey ((r + g + b) / 4 in each channel)
void Painter::GrayPixels(std::span<std::uint16_t> pixels) const
{
    for (std::uint16_t& pixel : pixels)
    {
        pixel = this->Gray(pixel);
    }
}

}  // namespace swd3
