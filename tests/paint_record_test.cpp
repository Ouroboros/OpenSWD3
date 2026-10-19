#include <algorithm>
#include <array>
#include <span>
#include <vector>

#include "core/types.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "test.h"

using swd3::u16;

using swd3::Display;
using swd3::Painter;
using swd3::PaintRecord;
using swd3::Surface;

namespace
{

constexpr u16 kBackground = 0x1234;
constexpr u16 kPixelA = 0x7C00;
constexpr u16 kPixelB = 0x03E0;
constexpr int kWidth = 2;
constexpr int kPictureX = 10;
constexpr int kPictureY = 10;
constexpr int kTintRow = 50;
constexpr int kTintPixels = 8;
constexpr int kTintLevel = 3;
constexpr std::array<int, 2> kMove = {5, 3};

// A true colour RLE picture of literal runs (see painter_test.cpp).
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

// The picture at (x, y) over the background (moving by `motion` when recorded), the tint on a row after it.
void Draw(Painter& painter, const std::vector<u16>& picture, u16* row, int x, int y, std::array<int, 2> motion)
{
    {
        const PaintRecord::Motion moving(motion);
        painter.SetSource(picture.data());
        painter.DrawPicture(x, y, kWidth, 2, Painter::Copy);
    }

    painter.TintPixels(std::span<u16>(row, kTintPixels), kTintLevel, kTintLevel, kTintLevel);
}

std::size_t Pixels(const Painter& painter)
{
    return static_cast<std::size_t>(painter.ScreenPitch()) * Display::Height;
}

}  // namespace

TEST_CASE("paint record: the draws drawn again give the frame, a moving draw lands its share along")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    std::vector<u16> picture = RlePicture(kWidth, {{kWidth, kPixelA, kPixelB}, {kWidth, kPixelB, kPixelA}});

    PaintRecord record;
    record.Start(screen.Pixels(), Pixels(painter));
    Draw(painter, picture, screen.Row(kTintRow), kPictureX, kPictureY, {2 * kMove[0], 2 * kMove[1]});
    record.Stop();
    CHECK(record.Complete());
    CHECK_EQ(record.Draws(), std::size_t{2});
    CHECK_EQ(record.Moving(), std::size_t{1});

    // The record keeps copies: the picture changed afterwards does not change the draws.
    const std::vector<u16> kept = picture;
    std::fill(picture.begin(), picture.end(), u16{0});
    std::vector<u16> scratch;
    CHECK(record.Matches(painter, {screen.Pixels(), Pixels(painter)}, scratch, painter.ScreenPitch()));
    CHECK(painter.Target() == screen.Pixels());

    Surface expected(Display::Width, Display::Height);
    expected.Fill(kBackground);
    painter.SetTarget(expected.Pixels());
    Draw(painter, kept, expected.Row(kTintRow), kPictureX + kMove[0], kPictureY + kMove[1], {0, 0});
    painter.SetTarget(screen.Pixels());

    std::vector<u16> out(Pixels(painter));
    constexpr double kHalfway = 0.5;
    record.Replay(painter, out, painter.ScreenPitch(), kHalfway);
    CHECK(std::equal(out.begin(), out.end(), expected.Pixels()));
    CHECK(painter.Target() == screen.Pixels());
}

TEST_CASE("paint record: a write past the painter is found")
{
    Surface screen(Display::Width, Display::Height);
    screen.Fill(kBackground);
    Painter painter;
    painter.Init(screen);
    painter.SetTarget(screen.Pixels());
    const std::vector<u16> picture = RlePicture(kWidth, {{kWidth, kPixelA, kPixelB}});

    PaintRecord record;
    record.Start(screen.Pixels(), Pixels(painter));
    painter.SetSource(picture.data());
    painter.DrawPicture(kPictureX, kPictureY, kWidth, 1, Painter::Copy);
    painter.DrawPicture(kPictureX + kMove[0], kPictureY, kWidth, 1, Painter::Copy);
    painter.TintPixels(std::span<u16>(screen.Row(kTintRow), kTintPixels), kTintLevel, kTintLevel, kTintLevel);
    screen.Row(kTintRow + 1)[0] = kPixelA;
    record.Stop();

    std::vector<u16> scratch;
    CHECK(record.Complete());
    CHECK_EQ(record.Draws(), std::size_t{3});
    CHECK_EQ(record.Moving(), std::size_t{0});
    CHECK(!record.Matches(painter, {screen.Pixels(), Pixels(painter)}, scratch, painter.ScreenPitch()));
}
