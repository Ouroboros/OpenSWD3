#include <cstdio>
#include <cstring>
#include <vector>

#include "core/types.h"
#include "draw/painter.h"
#include "game_data.h"
#include "platform/surface.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u8;

using swd3::DrawArgument;
using swd3::Painter;
using swd3::Surface;

namespace
{

// RLE picture: magic, width, height, flags 0x10, then lines (byte length header + codes), 0 terminator.
std::vector<u16> RlePicture(int width, const std::vector<std::vector<u16>>& lines)
{
    std::vector<u16> data{0xFFFF, static_cast<u16>(width), static_cast<u16>(lines.size()), 0x10};

    for (const auto& line : lines)
    {
        data.push_back(static_cast<u16>((line.size() + 1) * 2));
        data.insert(data.end(), line.begin(), line.end());
    }

    data.push_back(0);
    return data;
}

constexpr u32 kWaveOffsetsAt = 0x004A06A8;

}  // namespace

TEST_CASE("painter: wave offsets match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    const auto table = Painter::WaveOffsets();
    CHECK(swd3test::SameTable(exe, kWaveOffsetsAt, table));
}

TEST_CASE("painter: RGB565 display tables")
{
    Surface screen(640, 480);
    Painter painter;
    CHECK(painter.Init(screen));
    u32 red = 0;
    u32 green = 0;
    u32 blue = 0;
    painter.ColorMasks(red, green, blue);
    CHECK_EQ(red, 0xF800u);
    CHECK_EQ(green, 0x7E0u);
    CHECK_EQ(blue, 0x1Fu);
    CHECK_EQ(painter.Green().mask, 0x7C0u);
    CHECK_EQ(painter.MakeColor(31, 31, 31), 0xFFDFFFDFu);
    CHECK_EQ(painter.Red().Ramp(-1), 0u);
    CHECK_EQ(painter.Red().Ramp(40), 0xF800u);
}

TEST_CASE("painter: unassigned modes draw nothing")
{
    Surface screen(640, 480);
    screen.Fill(0x1234);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    const auto picture = RlePicture(2, {{0x0002, 0xAAAA, 0xBBBB}});
    painter.SetSource(picture.data());
    CHECK_EQ(painter.DrawPicture(10, 10, 2, 1, 0x3C), 0);
    CHECK_EQ(screen.Row(10)[10], 0x1234u);
}

TEST_CASE("painter: rows below the screen land on row 0")
{
    Surface screen(640, 480);
    screen.Fill(0);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    std::vector<u16> tile(256, 0x5555);
    painter.SetSource(tile.data());
    painter.DrawTile16(100, 480);
    CHECK_EQ(screen.Row(0)[100], 0x5555u);
    CHECK_EQ(screen.Row(15)[115], 0x5555u);
}

TEST_CASE("painter: skip runs of the mirrored alpha map leave the destination")
{
    Surface screen(640, 480);
    screen.Fill(0x0F0F);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    const auto picture = RlePicture(4, {{0x0002, 0xFFFF, 0xFFFF, 0x8002}});
    std::vector<u8> levels(4, 60);
    painter.SetSource(picture.data());
    DrawArgument argument;
    argument.alphaMap = levels.data();
    painter.DrawPicture(20, 20, 4, 1, swd3::Painter::AlphaMap | swd3::Painter::Mirror, argument);
    CHECK_EQ(screen.Row(20)[20], 0x0F0Fu);
    CHECK_EQ(screen.Row(20)[21], 0x0F0Fu);
    CHECK_EQ(screen.Row(20)[23], 0xFFFFu);
}
