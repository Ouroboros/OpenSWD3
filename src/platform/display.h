#pragma once

#include <array>
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

// What the window shows: a canvas of width x height logical pixels, and where the game's 640x480 screen lies on it
// (its left and top), which the pointer's coordinates count from. The map fills the window with a larger canvas
// (deviations.md); everything else is the 640x480 screen alone.
struct ScreenLayout
{
    int width = 0;
    int height = 0;
    int screenX = 0;
    int screenY = 0;
};

// The largest rectangle with the aspect ratio of `layout` (of the 640x480 screen without one) that fits the output,
// centred; the rest stays black.
Viewport FitViewport(int outputWidth, int outputHeight, const ScreenLayout& layout = {});

// An output position mapped to the coordinates of the game's screen on `layout`; positions off the canvas clamp to
// its edge.
LogicalPoint ToLogical(const Viewport& viewport, float outputX, float outputY, const ScreenLayout& layout = {});

// The game window. The original ran full screen at 640x480; here the logical screen keeps that size while the
// window can be resized freely: the screen is scaled with nearest sampling at any ratio that keeps its aspect, with
// black bars on the remaining sides. Alt+Enter toggles borderless full screen. The map can fill it instead
// (SetLayout).
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
    // What the frames shown from now on are (the 640x480 screen alone by default).
    void SetLayout(const ScreenLayout& layout);
    // A picture of the whole canvas shown behind the 640x480 frames presented on a larger layout (nothing behind them
    // without one: black); taken once here.
    void SetBackdrop(const Surface* canvas);
    const ScreenLayout& Layout() const;
    // The canvas the map would fill the window with: as high as the screen and as wide as the window's shape asks
    // (as wide and taller in a tall window), at least the 640x480 screen.
    ScreenLayout FillLayout() const;
    // The largest whole multiple of the canvas the window's output holds (at least 1).
    int FitScale() const;
    // Presenting waits for the display's refresh (`enabled`); false when the renderer cannot.
    bool SetVSync(bool enabled);
    // Handles Alt+Enter; true when the event was consumed.
    bool HandleEvent(const SDL_Event& event);
    void ToggleFullscreen();
    // A window position (as reported by mouse events) in logical screen coordinates.
    LogicalPoint WindowToLogical(float windowX, float windowY) const;
    void WarpPointer(int x, int y);
    // Where a text being typed lies on the logical screen, so that an input method shows its candidates beside it.
    void SetTextInputArea(int x, int y, int width, int height);
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
    // A logical screen point in window coordinates (what SDL_WarpMouseInWindow and SDL_SetTextInputArea take).
    void LogicalToWindow(int x, int y, float& windowX, float& windowY) const;

    int restoredWidth = DefaultWindowWidth;
    int restoredHeight = DefaultWindowHeight;
    bool maximized = false;
    ScreenLayout layout{Width, Height, 0, 0};
    std::array<int, 4> textInputArea{};  // SetTextInputArea: the area last given to SDL (x, y, width, height)

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    SDL_Texture* scaledTexture = nullptr;  // for screens larger than Width x Height
    int scaledWidth = 0;
    int scaledHeight = 0;
    SDL_Texture* backdrop = nullptr;  // SetBackdrop
    int backdropWidth = 0;
    int backdropHeight = 0;
    bool backdropShown = false;
    SDL_Cursor* cursor = nullptr;
};

}  // namespace swd3
