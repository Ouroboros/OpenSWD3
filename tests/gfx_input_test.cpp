#include <SDL3/SDL.h>

#include <new>

#include "data/globals.h"
#include "gfx/graphics.h"
#include "input/input.h"
#include "test.h"

using namespace swd3;

namespace {

void headlessVideo() { SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen,dummy"); }

}  // namespace

TEST_CASE("gfx.windowed init creates a desktop sized back buffer and classifies RGB565") {
    headlessVideo();
    Graphics* graphics = new (&G.graphics) Graphics();
    CHECK_EQ(graphics->Init(nullptr, kDisplayWindowed, 640, 480, 16), 1);
    CHECK(graphics->GetSurface(kSurfacePrimary) != nullptr);
    CHECK(graphics->GetSurface(kSurfaceBack) == graphics->m_back);
    CHECK_EQ(graphics->m_cooperativeFlags, 8u);
    CHECK_EQ(graphics->m_width, 640);
    CHECK_EQ(graphics->m_backPitch, 1280);
    CHECK_EQ(graphics->m_backHeight, 480);
    CHECK_EQ(graphics->m_pixelFormat, 2);
    CHECK_EQ(graphics->m_videoMemoryUsed, 640u * 480u * 2u);
    graphics->m_clearColor = 0x1234;
    CHECK_EQ(graphics->ClearSurface(graphics->m_back), 0);
    w32::DDSURFACEDESC2 desc{};
    desc.dwSize = sizeof(desc);
    CHECK_EQ(graphics->m_back->Lock(nullptr, &desc, 0, nullptr), 0);
    CHECK_EQ(static_cast<std::uint16_t*>(desc.lpSurface)[640 * 479 + 639], 0x1234);
    graphics->m_back->Unlock(nullptr);
    CHECK_EQ(graphics->Present(0), 1);
    CHECK_EQ(graphics->ReleaseSurfaces(), 1);
    CHECK(graphics->m_back == nullptr);
    graphics->ReleaseDirectDraw();
}

TEST_CASE("input keys and mouse origin arithmetic") {
    headlessVideo();
    Input* input = new (&G.input) Input();
    CHECK_EQ(input->m_sensitivity, 10);
    CHECK_EQ(input->Init(nullptr, nullptr), 0);
    CHECK_EQ(input->FirstKeyDown(), 0);
    CHECK_EQ(input->PressKey(0x1C), 0x1C);
    CHECK(input->IsKeyDown(0x1C) != 0);
    CHECK_EQ(input->FirstKeyDown(), 0x1C);
    int x = -1;
    int y = -1;
    input->SetMousePosition(100, 50);
    CHECK_EQ(input->ReadMouse(&x, &y), 0);
    CHECK_EQ(x, 100);
    CHECK_EQ(y, 50);
    input->SetSensitivity(2.5);
    CHECK_EQ(input->m_sensitivity, 25);
    // x / sensitivity is truncated before scaling back: 639 / 25 = 25 -> origin 250 axis units left
    input->SetMousePosition(1000, -5);
    input->ReadMouse(&x, &y);
    CHECK_EQ(x, 625);
    CHECK_EQ(y, 0);
    input->~Input();
    CHECK(input->m_directInput == nullptr);
}
