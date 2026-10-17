#pragma once

#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
union SDL_Event;

namespace swd3
{

class Surface;

// Where the logical screen is drawn inside the render output (in output pixels).
struct Viewport
{
    float x;
    float y;
    float width;
    float height;
};

struct LogicalPoint
{
    int x;
    int y;
};

// The largest rectangle with the logical aspect ratio that fits the output, centred; the rest stays black.
Viewport FitViewport(int outputWidth, int outputHeight);

// An output position mapped to logical screen coordinates; positions on the black bars clamp to the screen edge.
LogicalPoint ToLogical(const Viewport& viewport, float outputX, float outputY);

// The game window. The original ran full screen at 640x480; here the logical screen keeps that size while the
// window can be resized freely: the screen is scaled with nearest sampling at any ratio that keeps its aspect, with
// black bars on the remaining sides. Alt+Enter toggles borderless full screen.
class Display
{
public:
    static constexpr int Width = 640;
    static constexpr int Height = 480;
    static constexpr int Pixels = Width * Height;
    static constexpr int DefaultWindowWidth = 1280;
    static constexpr int DefaultWindowHeight = 960;

    Display() = default;
    ~Display();
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    // The window of `width` x `height` (maximised with `maximized`).
    bool Open(
        const std::string& title,
        int width = DefaultWindowWidth,
        int height = DefaultWindowHeight,
        bool maximized = false
    );

    bool IsOpen() const
    {
        return this->renderer != nullptr;
    }

    void Present(const Surface& screen);
    // Handles Alt+Enter; true when the event was consumed.
    bool HandleEvent(const SDL_Event& event);
    void ToggleFullscreen();
    // A window position (as reported by mouse events) in logical screen coordinates.
    LogicalPoint WindowToLogical(float windowX, float windowY) const;
    void WarpPointer(int x, int y);

    SDL_Window* Window() const
    {
        return this->window;
    }

    // The window as the user left it: its size when last neither maximised nor full screen, and whether it is
    // maximised (full screen counts as the state before it). `opened` is false when no window was made.
    struct WindowState
    {
        int width = 0;
        int height = 0;
        bool maximized = false;
        bool opened = false;
    };

    WindowState State() const;

private:
    Viewport CurrentViewport() const;
    void TrackWindow(const SDL_Event& event);

    int restoredWidth = DefaultWindowWidth;
    int restoredHeight = DefaultWindowHeight;
    bool maximized = false;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};

}  // namespace swd3
