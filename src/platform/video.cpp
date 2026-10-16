#include "platform/video.h"

#include <SDL3/SDL.h>

#include "swd3/w32.h"

namespace swd3::platform {
namespace {

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
SDL_Texture* g_texture = nullptr;
int g_textureWidth = 0;
int g_textureHeight = 0;

bool ensureWindow(int width, int height) {
    if (g_renderer == nullptr) {
        if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            return false;
        }
        if (!SDL_CreateWindowAndRenderer("SWD3", kScreenWidth, kScreenHeight, SDL_WINDOW_RESIZABLE, &g_window,
                                         &g_renderer)) {
            return false;
        }
        SDL_SetRenderLogicalPresentation(g_renderer, kScreenWidth, kScreenHeight,
                                         SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    if (g_texture == nullptr || g_textureWidth != width || g_textureHeight != height) {
        if (g_texture != nullptr) {
            SDL_DestroyTexture(g_texture);
        }
        g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (g_texture == nullptr) {
            return false;
        }
        SDL_SetTextureScaleMode(g_texture, SDL_SCALEMODE_NEAREST);
        g_textureWidth = width;
        g_textureHeight = height;
    }
    return true;
}

}  // namespace

SDL_Window* gameWindow() {
    ensureWindow(kScreenWidth, kScreenHeight);
    return g_window;
}

void presentFrame(const std::uint16_t* pixels, int width, int height, int pitchBytes) {
    if (!ensureWindow(width, height)) {
        return;
    }
    SDL_UpdateTexture(g_texture, nullptr, pixels, pitchBytes);
    SDL_RenderClear(g_renderer);
    const SDL_FRect source{0, 0, static_cast<float>(width < kScreenWidth ? width : kScreenWidth),
                           static_cast<float>(height < kScreenHeight ? height : kScreenHeight)};
    SDL_RenderTexture(g_renderer, g_texture, &source, nullptr);
    SDL_RenderPresent(g_renderer);
}

}  // namespace swd3::platform

namespace w32 {

// The emulated desktop has the size of the game's display mode.
int GetSystemMetrics(int index) {
    switch (index) {
    case SM_CXSCREEN:
        return swd3::platform::kScreenWidth;
    case SM_CYSCREEN:
        return swd3::platform::kScreenHeight;
    default:
        return 0;
    }
}

}  // namespace w32
