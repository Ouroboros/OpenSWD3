#include "platform/display.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstring>

#include "core/log.h"
#include "core/types.h"
#include "platform/exe_resources.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr float kHalf = 0.5F;            // the picture is centred in the window
constexpr int kMostCanvasWidth = 2048;   // the widest canvas the map fills a window with
constexpr int kMostCanvasHeight = 1024;  // the painter's row table holds this many rows

// A copy of `icon` as an SDL surface; null when it is empty.
SDL_Surface* MakeIconSurface(const IconImage& icon)
{
    if (icon.width <= 0 || icon.height <= 0 ||
        icon.pixels.size() != static_cast<std::size_t>(icon.width) * static_cast<std::size_t>(icon.height))
    {
        return nullptr;
    }

    SDL_Surface* surface = SDL_CreateSurface(icon.width, icon.height, SDL_PIXELFORMAT_ARGB8888);

    if (surface == nullptr)
    {
        return nullptr;
    }

    for (int row = 0; row < icon.height; ++row)
    {
        std::memcpy(
            static_cast<u8*>(surface->pixels) + static_cast<std::ptrdiff_t>(row) * surface->pitch,
            icon.pixels.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(icon.width),
            static_cast<std::size_t>(icon.width) * sizeof(u32)
        );
    }

    return surface;
}

}  // namespace

Viewport FitViewport(int outputWidth, int outputHeight, const ScreenLayout& layout)
{
    const int logicalWidth = layout.width > 0 ? layout.width : Display::Width;
    const int logicalHeight = layout.height > 0 ? layout.height : Display::Height;
    const float scaleX = static_cast<float>(outputWidth) / static_cast<float>(logicalWidth);
    const float scaleY = static_cast<float>(outputHeight) / static_cast<float>(logicalHeight);
    const float scale = std::min(scaleX, scaleY);
    const float width = static_cast<float>(logicalWidth) * scale;
    const float height = static_cast<float>(logicalHeight) * scale;
    return {
        (static_cast<float>(outputWidth) - width) * kHalf,
        (static_cast<float>(outputHeight) - height) * kHalf,
        width,
        height
    };
}

LogicalPoint ToLogical(const Viewport& viewport, float outputX, float outputY, const ScreenLayout& layout)
{
    if (viewport.width <= 0 || viewport.height <= 0)
    {
        return {0, 0};
    }

    const int logicalWidth = layout.width > 0 ? layout.width : Display::Width;
    const int logicalHeight = layout.height > 0 ? layout.height : Display::Height;
    const auto x = static_cast<int>((outputX - viewport.x) * static_cast<float>(logicalWidth) / viewport.width);
    const auto y = static_cast<int>((outputY - viewport.y) * static_cast<float>(logicalHeight) / viewport.height);
    return {
        std::clamp(x, 0, logicalWidth - 1) - layout.screenX,
        std::clamp(y, 0, logicalHeight - 1) - layout.screenY,
    };
}

Display::~Display()
{
    if (this->cursor != nullptr)
    {
        SDL_DestroyCursor(this->cursor);
    }

    if (this->texture != nullptr)
    {
        SDL_DestroyTexture(this->texture);
    }

    if (this->scaledTexture != nullptr)
    {
        SDL_DestroyTexture(this->scaledTexture);
    }

    if (this->backdrop != nullptr)
    {
        SDL_DestroyTexture(this->backdrop);
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

    SDL_SetWindowMinimumSize(this->window, Display::Width, Display::Height);
    this->restoredWidth = width;
    this->restoredHeight = height;
    this->maximized = maximized;
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
    return FitViewport(width, height, this->layout);
}

void Display::SetLayout(const ScreenLayout& layout)
{
    this->layout = layout;
}

const ScreenLayout& Display::Layout() const
{
    return this->layout;
}

ScreenLayout Display::FillLayout() const
{
    ScreenLayout fill{Display::Width, Display::Height, 0, 0};
    int width = 0;
    int height = 0;

    if (!this->IsOpen() || !SDL_GetRenderOutputSize(this->renderer, &width, &height) || width <= 0 || height <= 0)
    {
        return fill;
    }

    // Even margins keep the screen on whole pixels in the middle.
    if (static_cast<i64>(width) * Display::Height > static_cast<i64>(height) * Display::Width)
    {
        const auto wide = static_cast<int>(static_cast<i64>(Display::Height) * width / height);
        fill.width = std::min(wide & ~1, kMostCanvasWidth);
    }
    else
    {
        const auto tall = static_cast<int>(static_cast<i64>(Display::Width) * height / width);
        fill.height = std::min(tall & ~1, kMostCanvasHeight);
    }

    fill.width = std::max(fill.width, Display::Width);
    fill.height = std::max(fill.height, Display::Height);
    fill.screenX = (fill.width - Display::Width) / 2;
    fill.screenY = (fill.height - Display::Height) / 2;
    return fill;
}

void Display::Present(const Surface& screen)
{
    if (!this->IsOpen())
    {
        return;
    }

    SDL_Texture* shown = this->texture;

    if (screen.Width() != Display::Width || screen.Height() != Display::Height)
    {
        if (this->scaledTexture == nullptr || this->scaledWidth != screen.Width() ||
            this->scaledHeight != screen.Height())
        {
            if (this->scaledTexture != nullptr)
            {
                SDL_DestroyTexture(this->scaledTexture);
            }

            this->scaledTexture = SDL_CreateTexture(
                this->renderer,
                SDL_PIXELFORMAT_RGB565,
                SDL_TEXTUREACCESS_STREAMING,
                screen.Width(),
                screen.Height()
            );
            this->scaledWidth = screen.Width();
            this->scaledHeight = screen.Height();

            if (this->scaledTexture == nullptr)
            {
                LogError(u8"scaled frame texture creation failed: %s", AsUtf8(SDL_GetError()));
                return;
            }

            SDL_SetTextureScaleMode(this->scaledTexture, SDL_SCALEMODE_NEAREST);
            LogInfo(u8"scaled frame texture %dx%d", screen.Width(), screen.Height());
        }

        shown = this->scaledTexture;
    }

    SDL_UpdateTexture(shown, nullptr, screen.Pixels(), screen.Width() * static_cast<int>(sizeof(Pixel)));
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(this->renderer);
    const Viewport viewport = this->CurrentViewport();
    SDL_FRect target{viewport.x, viewport.y, viewport.width, viewport.height};

    // A frame of the 640x480 screen on a larger layout lies on the screen's place, the backdrop around it.
    if (static_cast<i64>(screen.Width()) * this->layout.height !=
        static_cast<i64>(screen.Height()) * this->layout.width)
    {
        if (this->backdropShown && this->backdrop != nullptr)
        {
            SDL_RenderTexture(this->renderer, this->backdrop, nullptr, &target);
        }

        const float scaleX = viewport.width / static_cast<float>(this->layout.width);
        const float scaleY = viewport.height / static_cast<float>(this->layout.height);
        target = {
            viewport.x + static_cast<float>(this->layout.screenX) * scaleX,
            viewport.y + static_cast<float>(this->layout.screenY) * scaleY,
            static_cast<float>(Display::Width) * scaleX,
            static_cast<float>(Display::Height) * scaleY,
        };
    }

    SDL_RenderTexture(this->renderer, shown, nullptr, &target);
    SDL_RenderPresent(this->renderer);
}

void Display::SetBackdrop(const Surface* canvas)
{
    this->backdropShown = false;

    if (canvas == nullptr || !this->IsOpen())
    {
        return;
    }

    if (this->backdrop == nullptr || this->backdropWidth != canvas->Width() || this->backdropHeight != canvas->Height())
    {
        if (this->backdrop != nullptr)
        {
            SDL_DestroyTexture(this->backdrop);
        }

        this->backdrop = SDL_CreateTexture(
            this->renderer,
            SDL_PIXELFORMAT_RGB565,
            SDL_TEXTUREACCESS_STREAMING,
            canvas->Width(),
            canvas->Height()
        );
        this->backdropWidth = canvas->Width();
        this->backdropHeight = canvas->Height();

        if (this->backdrop == nullptr)
        {
            LogError(u8"backdrop texture creation failed: %s", AsUtf8(SDL_GetError()));
            return;
        }

        SDL_SetTextureScaleMode(this->backdrop, SDL_SCALEMODE_NEAREST);
    }

    SDL_UpdateTexture(this->backdrop, nullptr, canvas->Pixels(), canvas->Width() * static_cast<int>(sizeof(Pixel)));
    this->backdropShown = true;
}

int Display::FitScale() const
{
    if (!this->IsOpen())
    {
        return 1;
    }

    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(this->renderer, &width, &height);
    return std::max(1, std::min(width / this->layout.width, height / this->layout.height));
}

bool Display::SetVSync(bool enabled)
{
    if (!this->IsOpen() || !SDL_SetRenderVSync(this->renderer, enabled ? 1 : 0))
    {
        LogWarning(
            u8"vsync %s failed: %s",
            enabled ? std::u8string_view(u8"on") : std::u8string_view(u8"off"),
            AsUtf8(SDL_GetError())
        );
        return false;
    }

    return true;
}

bool Display::HandleEvent(const SDL_Event& event)
{
    this->TrackWindow(event);
    const bool altEnter = event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN;

    if (altEnter && (event.key.mod & SDL_KMOD_ALT) != 0 && !event.key.repeat)
    {
        this->ToggleFullscreen();
        return true;
    }

    return false;
}

void Display::TrackWindow(const SDL_Event& event)
{
    if (this->window == nullptr)
    {
        return;
    }

    const SDL_WindowFlags flags = SDL_GetWindowFlags(this->window);
    const SDL_WindowFlags changed = SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED;

    if ((flags & SDL_WINDOW_FULLSCREEN) != 0)
    {
        return;
    }

    if (event.type == SDL_EVENT_WINDOW_RESIZED && (flags & changed) == 0)
    {
        this->restoredWidth = event.window.data1;
        this->restoredHeight = event.window.data2;
    }
    else if (event.type == SDL_EVENT_WINDOW_MAXIMIZED)
    {
        this->maximized = true;
    }
    else if (event.type == SDL_EVENT_WINDOW_RESTORED && (flags & SDL_WINDOW_MINIMIZED) == 0)
    {
        this->maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
    }
}

Display::WindowState Display::State() const
{
    WindowState state{this->restoredWidth, this->restoredHeight, this->maximized, this->window != nullptr};

    if (this->window != nullptr && (SDL_GetWindowFlags(this->window) & SDL_WINDOW_FULLSCREEN) == 0)
    {
        state.maximized = (SDL_GetWindowFlags(this->window) & SDL_WINDOW_MAXIMIZED) != 0;
    }

    return state;
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
    return ToLogical(this->CurrentViewport(), windowX * scaleX, windowY * scaleY, this->layout);
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
    const float outputX = viewport.x + static_cast<float>(x + this->layout.screenX) * viewport.width /
                                           static_cast<float>(this->layout.width);
    const float outputY = viewport.y + static_cast<float>(y + this->layout.screenY) * viewport.height /
                                           static_cast<float>(this->layout.height);
    SDL_WarpMouseInWindow(this->window, outputX * scaleX, outputY * scaleY);
}

void Display::SetIcon(const IconImage& icon)
{
    SDL_Surface* surface = MakeIconSurface(icon);

    if (this->window != nullptr && surface != nullptr && !SDL_SetWindowIcon(this->window, surface))
    {
        LogError(u8"window icon not set: %s", AsUtf8(SDL_GetError()));
    }

    SDL_DestroySurface(surface);
}

void Display::SetCursor(const IconImage& cursor)
{
    SDL_Surface* surface = MakeIconSurface(cursor);

    if (surface == nullptr)
    {
        return;
    }

    SDL_Cursor* made = SDL_CreateColorCursor(surface, cursor.hotX, cursor.hotY);
    SDL_DestroySurface(surface);

    if (made == nullptr)
    {
        LogError(u8"cursor not made: %s", AsUtf8(SDL_GetError()));
        return;
    }

    SDL_SetCursor(made);

    if (this->cursor != nullptr)
    {
        SDL_DestroyCursor(this->cursor);
    }

    this->cursor = made;
}

}  // namespace swd3
