#include "core/types.h"
#include "gfx/graphics.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "recording_reporter.h"
#include "test.h"

using swd3::Graphics;
using swd3::i32;

TEST_CASE("graphics: full screen setup as the game requests it")
{
    swd3::Display display;
    swd3test::RecordingReporter errors;
    Graphics graphics(display, errors);
    CHECK(graphics.Init(Graphics::kFullScreen, 640, 480, 16));
    swd3::Surface* primary = graphics.GetSurface(Graphics::SurfaceId::Primary);
    CHECK(primary != nullptr);
    CHECK(graphics.GetSurface(Graphics::SurfaceId::Back) == nullptr);
    CHECK(graphics.GdiSurface() == primary);
    i32 pitch = 0;
    i32 height = 0;
    CHECK_EQ(graphics.BackSize(pitch, height), 480);
    CHECK_EQ(pitch, 1280);
    CHECK_EQ(graphics.BitDepth(), 16u);
    CHECK_EQ(graphics.DetectPixelFormat(), 0x7E0u);
    CHECK(graphics.Present());
    CHECK(errors.messages.empty());
}

TEST_CASE("graphics: other depths fail like an unsupported display mode")
{
    swd3::Display display;
    swd3test::RecordingReporter errors;
    Graphics graphics(display, errors);
    CHECK(!graphics.Init(Graphics::kFullScreen, 640, 480, 8));
    CHECK_EQ(errors.messages.size(), 1u);
    CHECK(errors.messages[0] == "DirectDraw SetDisplayMode Error!!: DDERR_UNSUPPORTEDMODE  ");
}

TEST_CASE("graphics: windowed mode presents the back buffer")
{
    swd3::Display display;
    swd3test::RecordingReporter errors;
    Graphics graphics(display, errors);
    CHECK(graphics.Init(Graphics::kWindowed, 320, 200, 16));
    swd3::Surface* back = graphics.GetSurface(Graphics::SurfaceId::Back);
    CHECK(back != nullptr);
    CHECK_EQ(back->Width(), 640);
    back->Fill(0x1234);
    graphics.Present();
    CHECK_EQ(graphics.GetSurface(Graphics::SurfaceId::Primary)->Row(479)[639], 0x1234u);
}

TEST_CASE("graphics: device colours of COLORREF values")
{
    swd3::Surface surface(4, 4);
    surface.Fill(0x0F0F);
    CHECK_EQ(swd3::MatchSurfaceColor(surface, 0x00FFFFFF), 0xFFFFu);
    CHECK_EQ(swd3::MatchSurfaceColor(surface, 0x000000FF), 0xF800u);
    CHECK_EQ(swd3::MatchSurfaceColor(surface, 0x00FF0000), 0x001Fu);
    CHECK_EQ(swd3::MatchSurfaceColor(surface, swd3::kInvalidColor), 0x0F0Fu);
    CHECK_EQ(surface.Row(0)[0], 0x0F0Fu);
    swd3::FillSurfaceColor(surface, 0x0000FF00);
    CHECK_EQ(surface.Row(3)[3], 0x07E0u);
}
