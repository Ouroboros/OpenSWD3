#include "ui/marching_rect.h"

#include <array>

#include "draw/painter.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr int kScreenWidth = 0x280;
constexpr int kScreenHeight = 0x1E0;
constexpr u32 kPhases = 0x1F;

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
        const u32 level = (0x3F - shadePhase) & 0x1F;
        const auto grey = static_cast<u16>(level | (level << 5) | (level << 10));
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
    for (int row = 0; row < 0x78; ++row)
    {
        const u16* source = screen.Row(4 * row);
        u16* target = screen.Pixels() + static_cast<std::size_t>(row) * 0xA0;

        for (int column = 0; column < 0xA0; ++column)
        {
            target[column] = source[4 * column];
        }
    }
}

}  // namespace swd3
