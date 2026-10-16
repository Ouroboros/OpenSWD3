#include <cstring>

#include "draw/painter.h"

namespace swd3
{

namespace
{

std::uint16_t Word(const std::uint8_t* bytes, std::ptrdiff_t index = 0)
{
    std::uint16_t value = 0;
    std::memcpy(&value, bytes + 2 * index, 2);
    return value;
}

}  // namespace

// swd32.exe 0x004176D0: 16-bit pixels copied, or palette indices looked up. Lines advance by the screen pitch from
// the placement's first line, so a flip starts one picture height lower.
int Painter::BlitRawCopy(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const std::uint8_t* line = place.sourceLine;
    std::uint16_t* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            dest[column] = this->palette == nullptr ? Word(line, column) : this->palette[line[column]];
        }

        line += static_cast<std::ptrdiff_t>(width) * (this->palette == nullptr ? 2 : 1);
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x004177D0: 16-bit pixels mirrored (palette pictures are read as 16-bit pixels too)
int Painter::BlitRawCopyMirrored(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const std::uint8_t* line = place.sourceLine;
    std::uint16_t* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            dest[column] = Word(line, -column);
        }

        line += 2 * static_cast<std::ptrdiff_t>(width);
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417840: the fill colour (16-bit) or index 1 (palette) transparent
int Painter::BlitRawKeyed(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const std::uint8_t* line = place.sourceLine;
    std::uint16_t* dest = place.dest;
    const auto key = static_cast<std::uint16_t>(this->fillColorPair);

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            if (this->palette == nullptr)
            {
                const std::uint16_t pixel = Word(line, column);

                if (pixel != key)
                {
                    dest[column] = pixel;
                }
            }
            else if (line[column] != 1)
            {
                dest[column] = this->palette[line[column]];
            }
        }

        line += static_cast<std::ptrdiff_t>(width) * (this->palette == nullptr ? 2 : 1);
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417E40: mirrored 16-bit keyed copy. The pixel is compared with the whole fill colour pair, so no
// pixel is ever transparent.
int Painter::BlitRawKeyedMirrored(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const std::uint8_t* line = place.sourceLine;
    std::uint16_t* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            const std::uint32_t pixel = Word(line, -column);

            if (pixel != this->fillColorPair)
            {
                dest[column] = static_cast<std::uint16_t>(pixel);
            }
        }

        line += 2 * static_cast<std::ptrdiff_t>(width);
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417950: 16-bit pixels blended at DrawOptions::alpha - 1, the fill colour transparent
int Painter::BlitRawTranslucent(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const std::uint8_t* line = place.sourceLine;
    std::uint16_t* dest = place.dest;
    const auto key = static_cast<std::uint16_t>(this->fillColorPair);
    const int level = this->options.alpha - 1;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            const std::uint16_t pixel = Word(line, column);

            if (pixel != key)
            {
                dest[column] = this->Blend(dest[column], pixel, level);
            }
        }

        line += 2 * static_cast<std::ptrdiff_t>(width);
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417EC0: the destination blended towards the first pixel of the source, at level 15 for the first
// (height >> 4) + 1 rows and one less for each further group of rows
int Painter::BlitRawGradient(int, int, int, int height, const DrawArgument&)
{
    const Placement& place = this->placement;
    const std::uint16_t color = Word(this->source);
    const int rowsPerStep = height >> 4;
    int level = 15;
    int rowInStep = 0;
    std::uint16_t* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            dest[column] = this->Blend(dest[column], color, level);
        }

        dest += this->screenPitch;
        ++rowInStep;

        if (rowsPerStep < rowInStep)
        {
            rowInStep = 0;
            --level;
        }
    }

    return 0;
}

// swd32.exe 0x00417DE0: pair = pair / 2 + color / 4 + pair / 8 + pair / 16 per channel, two pixels at a time
void Painter::BlendPairsWithColor(std::span<std::uint32_t> pairs, std::uint32_t colorPair) const
{
    for (std::uint32_t& pair : pairs)
    {
        const std::uint32_t value = pair;
        pair = ((value >> 1) & this->halfMaskPairs[0]) + ((colorPair >> 2) & this->halfMaskPairs[1]) +
               ((value >> 3) & this->halfMaskPairs[2]) + ((value >> 4) & this->halfMaskPairs[3]);
    }
}

}  // namespace swd3
