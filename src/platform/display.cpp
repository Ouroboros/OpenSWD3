#include "platform/display.h"

#include <SDL3/SDL.h>

#include <algorithm>

#include "platform/surface.h"

namespace swd3 {

Viewport FitViewport(int outputWidth, int outputHeight) {
    const float scale = std::min(static_cast<float>(outputWidth) / Display::kWidth,
                                 static_cast<float>(outputHeight) / Display::kHeight);
    const float width = Display::kWidth * scale;
    const float height = Display::kHeight * scale;
    return {(outputWidth - width) / 2, (outputHeight - height) / 2, width, height};
}

LogicalPoint ToLogical(const Viewport& viewport, float outputX, float outputY) {
    if (viewport.width <= 0 || viewport.height <= 0) {
        return {0, 0};
    }
    const auto x = static_cast<int>((outputX - viewport.x) * Display::kWidth / viewport.width);
    const auto y = static_cast<int>((outputY - viewport.y) * Display::kHeight / viewport.height);
    return {std::clamp(x, 0, Display::kWidth - 1), std::clamp(y, 0, Display::kHeight - 1)};
}

Display::~Display() {
    if (m_texture != nullptr) {
        SDL_DestroyTexture(m_texture);
    }
    if (m_renderer != nullptr) {
        SDL_DestroyRenderer(m_renderer);
    }
    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
    }
}

bool Display::Open(const char* title) {
    if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        return false;
    }
    if (!SDL_CreateWindowAndRenderer(title, kDefaultWindowWidth, kDefaultWindowHeight,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &m_window, &m_renderer)) {
        return false;
    }
    SDL_SetWindowMinimumSize(m_window, kWidth / 2, kHeight / 2);
    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, kWidth, kHeight);
    if (m_texture == nullptr) {
        return false;
    }
    SDL_SetTextureScaleMode(m_texture, SDL_SCALEMODE_NEAREST);
    return true;
}

Viewport Display::CurrentViewport() const {
    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(m_renderer, &width, &height);
    return FitViewport(width, height);
}

void Display::Present(const Surface& screen) {
    if (!IsOpen()) {
        return;
    }
    SDL_UpdateTexture(m_texture, nullptr, screen.Pixels(), screen.Width() * 2);
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
    const Viewport viewport = CurrentViewport();
    const SDL_FRect target{viewport.x, viewport.y, viewport.width, viewport.height};
    SDL_RenderTexture(m_renderer, m_texture, nullptr, &target);
    SDL_RenderPresent(m_renderer);
}

bool Display::HandleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT) != 0 &&
        !event.key.repeat) {
        ToggleFullscreen();
        return true;
    }
    return false;
}

void Display::ToggleFullscreen() {
    if (m_window == nullptr) {
        return;
    }
    const bool fullscreen = (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;
    SDL_SetWindowFullscreenMode(m_window, nullptr);
    SDL_SetWindowFullscreen(m_window, !fullscreen);
}

LogicalPoint Display::WindowToLogical(float windowX, float windowY) const {
    if (!IsOpen()) {
        return {0, 0};
    }
    int windowWidth = 0;
    int windowHeight = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetWindowSize(m_window, &windowWidth, &windowHeight);
    SDL_GetRenderOutputSize(m_renderer, &outputWidth, &outputHeight);
    const float scaleX = windowWidth > 0 ? static_cast<float>(outputWidth) / windowWidth : 1.0f;
    const float scaleY = windowHeight > 0 ? static_cast<float>(outputHeight) / windowHeight : 1.0f;
    return ToLogical(CurrentViewport(), windowX * scaleX, windowY * scaleY);
}

}  // namespace swd3
