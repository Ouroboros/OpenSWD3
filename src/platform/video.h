#pragma once

#include <cstdint>

struct SDL_Window;

// SDL3 window and presentation of the emulated DirectDraw primary surface.
namespace swd3::platform {

// Size of the emulated desktop and display mode (GetSystemMetrics, primary surface).
constexpr int kScreenWidth = 640;
constexpr int kScreenHeight = 480;

// The game window (created on first use; nullptr when SDL video is unavailable).
SDL_Window* gameWindow();

// Shows a frame of RGB565 pixels in the game window (creates the window on first use).
void presentFrame(const std::uint16_t* pixels, int width, int height, int pitchBytes);

}  // namespace swd3::platform
