#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

#include "core/types.h"
#include "core/words.h"
#include "draw/painter.h"
#include "game/game_state.h"
#include "game_data.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "test.h"

using swd3::u16;
using swd3::u32;
using swd3::u8;

using swd3::Display;
using swd3::DrawArgument;
using swd3::Painter;
using swd3::Surface;

namespace
{

// RLE picture: the header, then lines (byte length header + codes), 0 terminator.
std::vector<u16> RlePicture(int width, const std::vector<std::vector<u16>>& lines)
{
    std::vector<u16> data{
        swd3::RleHeader::Magic,
        static_cast<u16>(width),
        static_cast<u16>(lines.size()),
        swd3::RleHeader::TrueColor
    };

    for (const auto& line : lines)
    {
        data.push_back(static_cast<u16>((line.size() + 1) * sizeof(u16)));
        data.insert(data.end(), line.begin(), line.end());
    }

    data.push_back(0);
    return data;
}

constexpr u32 kWaveOffsetsAt = 0x004A06A8;
constexpr int kTopLevel = 31;  // a five-bit channel at its top
constexpr int kPastTop = 40;   // a ramp index past the top saturates
constexpr u16 kBackground = 0x1234;
constexpr u16 kPixelA = 0xAAAA;
constexpr u16 kPixelB = 0xBBBB;
constexpr u16 kWhite = 0xFFFF;
constexpr int kPictureX = 10;
constexpr int kPictureY = 10;
constexpr u32 kUnassignedMode = 0x3C;
constexpr u16 kTileColor = 0x5555;
constexpr int kTileX = 100;
constexpr u16 kAlphaBackground = 0x0F0F;
constexpr u8 kAlphaLevel = 60;
constexpr int kAlphaX = 20;
constexpr int kAlphaY = 20;
constexpr u16 kSkipTwo = swd3::RleHeader::SkipRun | 2;

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
    Surface screen(Display::Width, Display::Height);
    Painter painter;
    CHECK(painter.Init(screen));
    u32 red = 0;
    u32 green = 0;
    u32 blue = 0;
    painter.ColorMasks(red, green, blue);
    CHECK_EQ(red, Surface::RedMask);
    CHECK_EQ(green, Surface::GreenMask);
    CHECK_EQ(blue, Surface::BlueMask);
    constexpr u32 kGreenLowBit = 0x20;  // green keeps five bits like the other channels
    CHECK_EQ(painter.Green().mask, Surface::GreenMask & ~kGreenLowBit);
    const auto white = static_cast<u16>(Surface::RedMask | painter.Green().mask | Surface::BlueMask);
    CHECK_EQ(painter.MakeColor(kTopLevel, kTopLevel, kTopLevel), swd3::MakeWords(white, white));
    CHECK_EQ(painter.Red().Ramp(-1), 0u);
    CHECK_EQ(painter.Red().Ramp(kPastTop), Surface::RedMask);
}

TEST_CASE("painter: the fast tint matches the original tint for every colour")
{
    constexpr int kFurthest = 40;          // levels past a channel's range saturate alike
    constexpr std::size_t kChannels = 3;   // red, green and blue
    constexpr std::size_t kTintMixes = 3;  // all three channels alike, unlike, one of them still
    constexpr int kHalf = 2;
    constexpr int kThird = 3;
    constexpr std::size_t kColors = 0x10000;
    Surface screen(Display::Width, Display::Height);
    Painter painter;
    CHECK(painter.Init(screen));
    std::vector<u16> colors(kColors);
    std::vector<u16> slow(kColors);
    std::vector<u16> fast(kColors);

    for (std::size_t color = 0; color < kColors; ++color)
    {
        colors[color] = static_cast<u16>(color);
    }

    int mismatches = 0;
    constexpr std::array<Painter::TintPath, 3> kPaths = {
        Painter::TintPath::Best,
        Painter::TintPath::Sse2,
        Painter::TintPath::Scalar,
    };

    for (int level = -kFurthest; level <= kFurthest; ++level)
    {
        const std::array<std::array<int, kChannels>, kTintMixes> levels = {{
            {level, level, level},
            {level, -level / kHalf, level / kThird},
            {-level, level, 0},
        }};

        for (const std::array<int, kChannels>& tint : levels)
        {
            // One pixel at a time TintPixels runs the original's loop.
            slow = colors;

            for (u16& color : slow)
            {
                painter.TintPixels(std::span<u16>(&color, 1), tint[0], tint[1], tint[2]);
            }

            for (const Painter::TintPath path : kPaths)
            {
                Painter::SetTintPath(path);
                fast = colors;
                painter.TintPixelsFast(fast, tint[0], tint[1], tint[2]);
                mismatches += slow == fast ? 0 : 1;
            }
        }
    }

    Painter::SetTintPath(Painter::TintPath::Best);
    CHECK_EQ(mismatches, 0);
}

TEST_CASE("painter: unassigned modes draw nothing")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    const std::vector<u16> pixels = {kPixelA, kPixelB};
    const auto picture =
        RlePicture(static_cast<int>(pixels.size()), {{static_cast<u16>(pixels.size()), kPixelA, kPixelB}});
    painter.SetSource(picture.data());
    CHECK_EQ(painter.DrawPicture(kPictureX, kPictureY, static_cast<int>(pixels.size()), 1, kUnassignedMode), 0);
    CHECK_EQ(screen.Row(kPictureY)[kPictureX], kBackground);
}

TEST_CASE("painter: rows below the screen land on row 0")
{
    using swd3::MapHeader;
    Surface screen(Display::Width, Display::Height);
    screen.Fill(0);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    std::vector<u16> tile(MapHeader::TileSize * MapHeader::TileSize, kTileColor);
    painter.SetSource(tile.data());
    painter.DrawTile16(kTileX, Display::Height);
    CHECK_EQ(screen.Row(0)[kTileX], kTileColor);
    CHECK_EQ(screen.Row(MapHeader::TileFraction)[kTileX + MapHeader::TileFraction], kTileColor);
}

TEST_CASE("painter: skip runs of the mirrored alpha map leave the destination")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kAlphaBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    // Two white pixels, then a skip run of two; mirrored, the skip comes first.
    constexpr int kWidth = 4;
    const std::vector<u16> whites = {kWhite, kWhite};
    const auto picture = RlePicture(kWidth, {{static_cast<u16>(whites.size()), kWhite, kWhite, kSkipTwo}});
    std::vector<u8> levels(kWidth, kAlphaLevel);
    painter.SetSource(picture.data());
    DrawArgument argument;
    argument.alphaMap = levels.data();
    painter.DrawPicture(kAlphaX, kAlphaY, kWidth, 1, Painter::AlphaMap | Painter::Mirror, argument);
    CHECK_EQ(screen.Row(kAlphaY)[kAlphaX], kAlphaBackground);
    CHECK_EQ(screen.Row(kAlphaY)[kAlphaX + 1], kAlphaBackground);
    CHECK_EQ(screen.Row(kAlphaY)[kAlphaX + kWidth - 1], kWhite);
}

TEST_CASE("painter: the alpha map takes a literal run after a drawn literal run as a 0x8000 run")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kAlphaBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    // Two literal runs of one pixel: after the first is drawn, 0x0041A22B tests only 0x4000, so the second code is a
    // 0x8000 run (filled with 0xFFF0) and its pixel is not passed over.
    constexpr int kWidth = 2;
    constexpr u16 kAlphaMarker = 0xFFF0;
    const auto picture = RlePicture(kWidth, {{1, kPixelA, 1, kPixelB}});
    std::vector<u8> levels(kWidth, kAlphaLevel);
    painter.SetSource(picture.data());
    DrawArgument argument;
    argument.alphaMap = levels.data();
    painter.DrawPicture(kAlphaX, kAlphaY, kWidth, 1, Painter::AlphaMap, argument);
    CHECK_EQ(screen.Row(kAlphaY)[kAlphaX], kPixelA);
    CHECK_EQ(screen.Row(kAlphaY)[kAlphaX + 1], kAlphaMarker);
}

TEST_CASE("painter: the copy takes the run after a drawn literal run as transparent")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    // After the first literal run is drawn, 0x00418503 reads the next code as a transparent run without testing it:
    // the second pixel is passed over and its data is read as the next code.
    constexpr int kWidth = 2;
    const auto picture = RlePicture(kWidth, {{1, kPixelA, 1, kPixelB}});
    painter.SetSource(picture.data());
    painter.DrawPicture(kPictureX, kPictureY, kWidth, 1, Painter::Copy);
    CHECK_EQ(screen.Row(kPictureY)[kPictureX], kPixelA);
    CHECK_EQ(screen.Row(kPictureY)[kPictureX + 1], kBackground);
}

TEST_CASE("painter: the grey effect draws without a tint option")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    // 0x00421850 computes the tint steps but never tests them: the destination turns grey with zero tints too.
    const auto picture = RlePicture(1, {{1, kPixelA}});
    painter.SetSource(picture.data());
    painter.DrawPicture(kPictureX, kPictureY, 1, 1, Painter::GrayUnder);
    CHECK(screen.Row(kPictureY)[kPictureX] != kBackground);
}

TEST_CASE("painter: a translucency fraction mixes two levels and keeps a whole level as it was")
{
    constexpr int kLevel = 7;
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    const std::vector<u16> pixels = {kPixelA, kPixelB};
    const auto picture =
        RlePicture(static_cast<int>(pixels.size()), {{static_cast<u16>(pixels.size()), kPixelA, kPixelB}});
    const auto width = static_cast<int>(pixels.size());

    auto draw = [&](int row, int level, double fraction)
    {
        painter.SetSource(picture.data());
        painter.Options().alpha = level;
        painter.Options().alphaFraction = fraction;
        painter.DrawPicture(kPictureX, kPictureY + row, width, 1, Painter::Translucent);
        return std::vector<u16>(
            screen.Row(kPictureY + row) + kPictureX,
            screen.Row(kPictureY + row) + kPictureX + width
        );
    };

    const std::vector<u16> low = draw(0, kLevel, 0.0);
    const std::vector<u16> high = draw(1, kLevel + 1, 0.0);
    CHECK(low != high);
    CHECK(draw(2, kLevel, 1.0) == high);
    CHECK_EQ(painter.Options().alphaFraction, 0.0);
}
