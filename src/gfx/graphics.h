#pragma once

#include <memory>
#include <string_view>

#include "core/types.h"

namespace swd3
{

class Display;
class ErrorReporter;
class Surface;

// A COLORREF of the game: 0x00BBGGRR; kInvalidColor (CLR_INVALID) stands for "the current pixel".
using ColorRef = u32;
constexpr ColorRef kInvalidColor = 0xFFFFFFFFu;

// The display surfaces and their presentation (the DirectDraw wrapper at 0x004AB870 in the original). The original
// sets up DirectDraw with a cooperative level, a display mode, a clipper and a primary surface (plus a system-memory
// back buffer in windowed mode); here the surfaces are plain RGB565 buffers shown by the Display window.
class Graphics
{
public:
    enum class SurfaceId
    {
        Primary = 0x2711,
        Back = 0x2712,
        Third = 0x2713,
    };

    // Init modes: kWindowed uses the normal cooperative level, anything else exclusive full screen.
    static constexpr int Windowed = 0x4E21;
    static constexpr int FullScreen = 0x4E22;

    Graphics(Display& display, ErrorReporter& errors);
    ~Graphics();

    bool Init(int mode, int width, int height, int bitsPerPixel);
    bool CreateDevice();
    bool SetCooperativeLevel(u32 flags);
    bool SetDisplayMode(int width, int height, int bitsPerPixel);
    bool CreateClipper();
    bool CreateSurfaces();
    bool ReleaseSurfaces();
    u32 ReleaseDevice();
    bool Present();
    std::unique_ptr<Surface> CreateOffscreen(int width, int height);
    void ClearSurface(Surface& surface);
    void SetClearColor(u32 color);
    u32 BitDepth() const;
    bool QueryVideoMemory();
    Surface* GetSurface(SurfaceId id);
    Graphics& Device();
    Surface* GdiSurface();
    u32 DetectPixelFormat();
    i32 BackSize(i32& pitch, i32& height) const;

    int Width() const;
    int Height() const;

private:
    // Cooperative level flags as the original passes them.
    static constexpr u32 Normal = 0x08;     // DDSCL_NORMAL
    static constexpr u32 Exclusive = 0x13;  // DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE
    static constexpr u32 FullScreenFlag = 0x01;
    static constexpr u32 ExclusiveFlag = 0x10;

    Display& display;
    ErrorReporter& errors;
    bool device = false;
    std::unique_ptr<Surface> primary;
    std::unique_ptr<Surface> back;
    std::unique_ptr<Surface> third;
    u32 videoMemoryUsed = 0;
    u32 videoMemoryTotal = 0;
    u32 videoMemoryFree = 0;
    u32 cooperativeFlags = 0;
    int width = 0;
    int height = 0;
    int bitsPerPixel = 0;
    u8 pixelFormat = 0;  // 0 / 1 (blue 0x3F) / 2 (green 0x7E0) / 3 (red 0xFC00)
    i32 primaryPitch = 0;
    i32 primaryHeight = 0;
    i32 backPitch = 0;
    i32 backHeight = 0;
    u32 clearColor = 0;  // 0x004AB8F8 (+0x88) the fill colour pair of the painter, set by InitDrawKernel
};

void ShowDeviceError(ErrorReporter& errors, std::u8string_view caption, u32 result);
void FillSurfaceColor(Surface& surface, ColorRef color);
u32 MatchSurfaceColor(Surface& surface, ColorRef color);

}  // namespace swd3
