#include "ui/marching_rect.h"

#include <array>

#include "draw/painter.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr int kScreenWidth = Display::Width;
constexpr int kScreenHeight = Display::Height;
constexpr u32 kPhases = 0x1F;
constexpr u32 kShadeStart = 0x3F;
constexpr u32 kChannelMask = 0x1F;  // a grey of the same 5-bit level in the three channels
constexpr int kChannelBits = 5;
constexpr int kShrink = 4;  // ShrinkScreenQuarter keeps every fourth pixel and row

}  // namespace

// swd32.exe 0x0040DE50: dashes around the rectangle (the top left to right, the right down, the bottom right to left,
// the left up), the grey cycling over 32 shades; nothing when the rectangle leaves the 640x480 screen
void MarchingRect::Draw(Surface& surface, const Painter& painter, int x, int y, int width, int height)
{
    if (x < 0 || y < 0 || width + x >= kScreenWidth || y + height >= kScreenHeight)
    {
        return;
    }

    u32 shadePhase = static_cast<u32>(this->phase);

    auto shade = [&]
    {
        const u32 level = (kShadeStart - shadePhase) & kChannelMask;
        const auto grey = static_cast<u16>(level | (level << kChannelBits) | (level << (2 * kChannelBits)));
        std::array<u16, 2> pair = {grey, grey};
        painter.ConvertPixels(pair);
        shadePhase = (shadePhase + 1) & kPhases;
        return pair[0];
    };

    for (int i = 0; i < width; ++i)
    {
        surface.Row(y)[x + i] = shade();
    }

    for (int i = 0; i < height; ++i)
    {
        surface.Row(y + i)[x + width] = shade();
    }

    for (int i = 0; i < width; ++i)
    {
        surface.Row(y + height)[x + width - i] = shade();
    }

    for (int i = 0; i < height; ++i)
    {
        surface.Row(y + height - i)[x] = shade();
    }

    this->phase = static_cast<i32>((static_cast<u32>(this->phase) + 1) & kPhases);
}

// swd32.exe 0x0040E080
void ShrinkScreenQuarter(Surface& screen)
{
    for (int row = 0; row < Display::Height / kShrink; ++row)
    {
        const u16* source = screen.Row(kShrink * row);
        u16* target = screen.Pixels() + static_cast<std::size_t>(row) * (Display::Width / kShrink);

        for (int column = 0; column < Display::Width / kShrink; ++column)
        {
            target[column] = source[kShrink * column];
        }
    }
}

}  // namespace swd3
