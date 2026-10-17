#include "platform/display.h"

#include <SDL3/SDL.h>

#include <algorithm>

#include "core/log.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr float kHalf = 0.5F;      // the picture is centred in the window
constexpr int kMinimumShrink = 2;  // the window shrinks to half the logical size at most

}  // namespace

Viewport FitViewport(int outputWidth, int outputHeight)
{
    const float scaleX = static_cast<float>(outputWidth) / Display::Width;
    const float scaleY = static_cast<float>(outputHeight) / Display::Height;
    const float scale = std::min(scaleX, scaleY);
    const float width = Display::Width * scale;
    const float height = Display::Height * scale;
    return {(outputWidth - width) * kHalf, (outputHeight - height) * kHalf, width, height};
}

LogicalPoint ToLogical(const Viewport& viewport, float outputX, float outputY)
{
    if (viewport.width <= 0 || viewport.height <= 0)
    {
        return {0, 0};
    }

    const auto x = static_cast<int>((outputX - viewport.x) * Display::Width / viewport.width);
    const auto y = static_cast<int>((outputY - viewport.y) * Display::Height / viewport.height);
    return {std::clamp(x, 0, Display::Width - 1), std::clamp(y, 0, Display::Height - 1)};
}

Display::~Display()
{
    if (this->texture != nullptr)
    {
        SDL_DestroyTexture(this->texture);
    }

    if (this->renderer != nullptr)
    {
        SDL_DestroyRenderer(this->renderer);
    }

    if (this->window != nullptr)
    {
        SDL_DestroyWindow(this->window);
    }
}

bool Display::Open(const std::string& title, int width, int height, bool maximized)
{
    if (!SDL_WasInit(SDL_INIT_VIDEO) && !SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
        LogError(u8"video initialisation failed: %s", AsUtf8(SDL_GetError()));
        return false;
    }

    const SDL_WindowFlags flags =
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (maximized ? SDL_WINDOW_MAXIMIZED : SDL_WindowFlags{0});
    const bool created =
        SDL_CreateWindowAndRenderer(title.c_str(), width, height, flags, &this->window, &this->renderer);

    if (!created)
    {
        LogError(u8"window creation failed: %s", AsUtf8(SDL_GetError()));
        return false;
    }

    SDL_SetWindowMinimumSize(this->window, Display::Width / kMinimumShrink, Display::Height / kMinimumShrink);
    this->texture = SDL_CreateTexture(
        this->renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        Display::Width,
        Display::Height
    );

    if (this->texture == nullptr)
    {
        LogError(u8"frame texture creation failed: %s", AsUtf8(SDL_GetError()));
        return false;
    }

    SDL_SetTextureScaleMode(this->texture, SDL_SCALEMODE_NEAREST);
    const char* driver = SDL_GetRendererName(this->renderer);
    LogInfo(
        u8"display open window=%dx%d maximized=%d renderer=%s",
        width,
        height,
        maximized ? 1 : 0,
        AsUtf8(driver != nullptr ? driver : "")
    );
    return true;
}

Viewport Display::CurrentViewport() const
{
    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(this->renderer, &width, &height);
    return FitViewport(width, height);
}

void Display::Present(const Surface& screen)
{
    if (!this->IsOpen())
    {
        return;
    }

    SDL_UpdateTexture(this->texture, nullptr, screen.Pixels(), screen.Width() * static_cast<int>(sizeof(Pixel)));
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(this->renderer);
    const Viewport viewport = this->CurrentViewport();
    const SDL_FRect target{viewport.x, viewport.y, viewport.width, viewport.height};
    SDL_RenderTexture(this->renderer, this->texture, nullptr, &target);
    SDL_RenderPresent(this->renderer);
}

bool Display::HandleEvent(const SDL_Event& event)
{
    const bool altEnter = event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN;

    if (altEnter && (event.key.mod & SDL_KMOD_ALT) != 0 && !event.key.repeat)
    {
        this->ToggleFullscreen();
        return true;
    }

    return false;
}

void Display::ToggleFullscreen()
{
    if (this->window == nullptr)
    {
        return;
    }

    const bool fullscreen = (SDL_GetWindowFlags(this->window) & SDL_WINDOW_FULLSCREEN) != 0;
    SDL_SetWindowFullscreenMode(this->window, nullptr);
    SDL_SetWindowFullscreen(this->window, !fullscreen);
}

LogicalPoint Display::WindowToLogical(float windowX, float windowY) const
{
    if (!this->IsOpen())
    {
        return {0, 0};
    }

    int windowWidth = 0;
    int windowHeight = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);
    SDL_GetRenderOutputSize(this->renderer, &outputWidth, &outputHeight);
    const float scaleX = windowWidth > 0 ? static_cast<float>(outputWidth) / windowWidth : 1.0f;
    const float scaleY = windowHeight > 0 ? static_cast<float>(outputHeight) / windowHeight : 1.0f;
    return ToLogical(this->CurrentViewport(), windowX * scaleX, windowY * scaleY);
}

// SetCursorPos(x, y): the system pointer moved to logical point (x, y) of the window (the original runs full screen,
// where the screen point is the logical one)
void Display::WarpPointer(int x, int y)
{
    if (!this->IsOpen())
    {
        return;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    int outputWidth = 0;
    int outputHeight = 0;
    SDL_GetWindowSize(this->window, &windowWidth, &windowHeight);
    SDL_GetRenderOutputSize(this->renderer, &outputWidth, &outputHeight);
    const float scaleX = outputWidth > 0 ? static_cast<float>(windowWidth) / outputWidth : 1.0f;
    const float scaleY = outputHeight > 0 ? static_cast<float>(windowHeight) / outputHeight : 1.0f;
    const Viewport viewport = this->CurrentViewport();
    const float outputX = viewport.x + static_cast<float>(x) * viewport.width / Display::Width;
    const float outputY = viewport.y + static_cast<float>(y) * viewport.height / Display::Height;
    SDL_WarpMouseInWindow(this->window, outputX * scaleX, outputY * scaleY);
}
}  // namespace swd3
