#pragma once

#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Cursor;
union SDL_Event;

namespace swd3
{

class Surface;
struct IconImage;

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

    // The screen shown scaled to the window; a screen larger than the logical one (the map drawn at a render scale)
    // goes through a texture of its own size.
    void Present(const Surface& screen);
    // The largest whole multiple of the logical screen the window's output holds (at least 1).
    int FitScale() const;
    // Presenting waits for the display's refresh (`enabled`); false when the renderer cannot.
    bool SetVSync(bool enabled);
    // Handles Alt+Enter; true when the event was consumed.
    bool HandleEvent(const SDL_Event& event);
    void ToggleFullscreen();
    // A window position (as reported by mouse events) in logical screen coordinates.
    LogicalPoint WindowToLogical(float windowX, float windowY) const;
    void WarpPointer(int x, int y);
    // The icon of the window, and the pointer shown over it while the system cursor is visible (swd32.exe 0x00409EC0
    // registers its window class with icon 0x65 and the blank cursor 0x67).
    void SetIcon(const IconImage& icon);
    void SetCursor(const IconImage& cursor);

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
    SDL_Texture* scaledTexture = nullptr;  // for screens larger than Width x Height
    int scaledWidth = 0;
    int scaledHeight = 0;
    SDL_Cursor* cursor = nullptr;
};

}  // namespace swd3
