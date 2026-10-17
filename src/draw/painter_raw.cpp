#include <cstring>

#include "draw/painter.h"

namespace swd3
{

namespace
{

constexpr int kGradientShift = 4;  // BlitRawGradient: the level falls every sixteenth of the height
constexpr int kHalfShift = 1;      // the shares of a blend
constexpr int kQuarterShift = 2;
constexpr int kEighthShift = 3;
constexpr int kSixteenthShift = 4;

u16 Word(const u8* bytes, std::ptrdiff_t index = 0)
{
    u16 value = 0;
    std::memcpy(&value, bytes + sizeof(value) * index, sizeof(value));
    return value;
}

}  // namespace

// swd32.exe 0x004176D0: 16-bit pixels copied, or palette indices looked up. Lines advance by the screen pitch from
// the placement's first line, so a flip starts one picture height lower.
int Painter::BlitRawCopy(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const u8* line = place.sourceLine;
    u16* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            dest[column] = this->palette == nullptr ? Word(line, column) : this->palette[line[column]];
        }

        line += static_cast<std::ptrdiff_t>(width) * (this->palette == nullptr ? sizeof(u16) : sizeof(u8));
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x004177D0: 16-bit pixels mirrored (palette pictures are read as 16-bit pixels too)
int Painter::BlitRawCopyMirrored(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const u8* line = place.sourceLine;
    u16* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            dest[column] = Word(line, -column);
        }

        line += static_cast<std::ptrdiff_t>(sizeof(u16)) * width;
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417840: the fill colour (16-bit) or index 1 (palette) transparent
int Painter::BlitRawKeyed(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const u8* line = place.sourceLine;
    u16* dest = place.dest;
    const auto key = static_cast<u16>(this->fillColorPair);

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            if (this->palette == nullptr)
            {
                const u16 pixel = Word(line, column);

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

        line += static_cast<std::ptrdiff_t>(width) * (this->palette == nullptr ? sizeof(u16) : sizeof(u8));
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417E40: mirrored 16-bit keyed copy. The pixel is compared with the whole fill colour pair, so no
// pixel is ever transparent.
int Painter::BlitRawKeyedMirrored(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const u8* line = place.sourceLine;
    u16* dest = place.dest;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            const u32 pixel = Word(line, -column);

            if (pixel != this->fillColorPair)
            {
                dest[column] = static_cast<u16>(pixel);
            }
        }

        line += static_cast<std::ptrdiff_t>(sizeof(u16)) * width;
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417950: 16-bit pixels blended at DrawOptions::alpha - 1, the fill colour transparent
int Painter::BlitRawTranslucent(int, int, int width, int, const DrawArgument&)
{
    const Placement& place = this->placement;
    const u8* line = place.sourceLine;
    u16* dest = place.dest;
    const auto key = static_cast<u16>(this->fillColorPair);
    const int level = this->options.alpha - 1;

    for (int row = 0; row < place.rows; ++row)
    {
        for (int column = 0; column < place.width; ++column)
        {
            const u16 pixel = Word(line, column);

            if (pixel != key)
            {
                dest[column] = this->Blend(dest[column], pixel, level);
            }
        }

        line += static_cast<std::ptrdiff_t>(sizeof(u16)) * width;
        dest += this->screenPitch;
    }

    return 0;
}

// swd32.exe 0x00417EC0: the destination blended towards the first pixel of the source, at level 15 for the first
// (height >> 4) + 1 rows and one less for each further group of rows
int Painter::BlitRawGradient(int, int, int, int height, const DrawArgument&)
{
    const Placement& place = this->placement;
    const u16 color = Word(this->source);
    const int rowsPerStep = height >> kGradientShift;
    int level = Painter::MaxAlpha;
    int rowInStep = 0;
    u16* dest = place.dest;

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
void Painter::BlendPairsWithColor(std::span<u32> pairs, u32 colorPair) const
{
    for (u32& pair : pairs)
    {
        const u32 value = pair;
        pair = ((value >> kHalfShift) & this->halfMaskPairs[kHalfShift - 1]) +
               ((colorPair >> kQuarterShift) & this->halfMaskPairs[kQuarterShift - 1]) +
               ((value >> kEighthShift) & this->halfMaskPairs[kEighthShift - 1]) +
               ((value >> kSixteenthShift) & this->halfMaskPairs[kSixteenthShift - 1]);
    }
}

}  // namespace swd3
