#include "platform/display.h"
#include "test.h"

using swd3::Display;
using swd3::FitViewport;
using swd3::ToLogical;

namespace
{

constexpr int kWideWidth = 1920;  // a 16:9 window: 1080 rows of the 4:3 picture, bars at both sides
constexpr int kWideHeight = 1080;
constexpr int kWidePicture = kWideHeight * Display::Width / Display::Height;
constexpr int kTallWidth = 800;  // a window taller than 4:3: bars above and below
constexpr int kTallHeight = 1000;
constexpr int kTallPicture = kTallWidth * Display::Height / Display::Width;
constexpr float kOddWidth = 1000;  // a scale of 1.5625
constexpr float kOddHeight = 750;
constexpr float kInside = 0.1F;  // short of the far edge
constexpr float kOnBar = 10;     // a position on a side bar

}  // namespace

TEST_CASE("display: default window fills exactly at 2x")
{
    const auto viewport = FitViewport(Display::DefaultWindowWidth, Display::DefaultWindowHeight);
    CHECK(viewport.x == 0 && viewport.y == 0);
    CHECK(viewport.width == Display::DefaultWindowWidth && viewport.height == Display::DefaultWindowHeight);
}

TEST_CASE("display: wide window gets side bars, tall window top and bottom bars")
{
    const auto wide = FitViewport(kWideWidth, kWideHeight);
    CHECK(wide.height == kWideHeight && wide.width == kWidePicture);
    CHECK(wide.x == (kWideWidth - kWidePicture) / 2 && wide.y == 0);
    const auto tall = FitViewport(kTallWidth, kTallHeight);
    CHECK(tall.width == kTallWidth && tall.height == kTallPicture);
    CHECK(tall.x == 0 && tall.y == (kTallHeight - kTallPicture) / 2);
}

TEST_CASE("display: non-integer scale maps window positions to the logical screen")
{
    const auto viewport = FitViewport(static_cast<int>(kOddWidth), static_cast<int>(kOddHeight));
    const auto centre = ToLogical(viewport, kOddWidth / 2, kOddHeight / 2);
    CHECK_EQ(centre.x, Display::Width / 2);
    CHECK_EQ(centre.y, Display::Height / 2);
    const auto corner = ToLogical(viewport, kOddWidth - kInside, kOddHeight - kInside);
    CHECK_EQ(corner.x, Display::Width - 1);
    CHECK_EQ(corner.y, Display::Height - 1);
}

TEST_CASE("display: positions on the black bars clamp to the screen edge")
{
    const auto viewport = FitViewport(kWideWidth, kWideHeight);
    const auto left = ToLogical(viewport, kOnBar, kWideHeight / 2);
    CHECK_EQ(left.x, 0);
    CHECK_EQ(left.y, Display::Height / 2);
    const auto right = ToLogical(viewport, kWideWidth - kOnBar, kWideHeight - 1);
    CHECK_EQ(right.x, Display::Width - 1);
    CHECK_EQ(right.y, Display::Height - 1);
}
