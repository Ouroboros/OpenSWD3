#include "platform/display.h"
#include "test.h"

using swd3::Display;
using swd3::FitViewport;
using swd3::ToLogical;

TEST_CASE("display: default window fills exactly at 2x")
{
    const auto viewport = FitViewport(Display::DefaultWindowWidth, Display::DefaultWindowHeight);
    CHECK(viewport.x == 0 && viewport.y == 0);
    CHECK(viewport.width == 1280 && viewport.height == 960);
}

TEST_CASE("display: wide window gets side bars, tall window top and bottom bars")
{
    const auto wide = FitViewport(1920, 1080);
    CHECK(wide.height == 1080 && wide.width == 1440 && wide.x == 240 && wide.y == 0);
    const auto tall = FitViewport(800, 1000);
    CHECK(tall.width == 800 && tall.height == 600 && tall.x == 0 && tall.y == 200);
}

TEST_CASE("display: non-integer scale maps window positions to the logical screen")
{
    const auto viewport = FitViewport(1000, 750);
    const auto centre = ToLogical(viewport, 500, 375);
    CHECK_EQ(centre.x, 320);
    CHECK_EQ(centre.y, 240);
    const auto corner = ToLogical(viewport, 999.9f, 749.9f);
    CHECK_EQ(corner.x, 639);
    CHECK_EQ(corner.y, 479);
}

TEST_CASE("display: positions on the black bars clamp to the screen edge")
{
    const auto viewport = FitViewport(1920, 1080);
    const auto left = ToLogical(viewport, 10, 540);
    CHECK_EQ(left.x, 0);
    CHECK_EQ(left.y, 240);
    const auto right = ToLogical(viewport, 1910, 1079);
    CHECK_EQ(right.x, 639);
    CHECK_EQ(right.y, 479);
}
