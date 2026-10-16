#pragma once

#include <cstdint>

#include "swd3/ddraw.h"

namespace swd3 {

// Surface ids of Graphics::GetSurface.
constexpr int kSurfacePrimary = 0x2711;
constexpr int kSurfaceBack = 0x2712;
constexpr int kSurfaceThird = 0x2713;

// Display mode id of Graphics::Init selecting the windowed (DDSCL_NORMAL) setup.
constexpr int kDisplayWindowed = 0x4E21;

// DirectDraw wrapper (global object at 0x004AB870, size 0x8C).
class Graphics {
public:
    Graphics();
    int Init(w32::HWND window, int mode, int width, int height, int bpp);
    int CreateDirectDraw();
    int SetCooperativeLevel(w32::HWND window, int flags);
    int SetDisplayMode(w32::HWND window, int width, int height, int bpp);
    int CreateClipper(w32::HWND window);
    int CreateSurfaces();
    int ReleaseSurfaces();
    w32::DWORD ReleaseDirectDraw();
    int Present(int unused);
    w32::IDirectDrawSurface4* CreateOffscreen(int width, int height);
    w32::HRESULT ClearSurface(w32::IDirectDrawSurface4* surface);
    w32::DWORD GetBitDepth();
    w32::BOOL QueryVideoMemory();
    w32::IDirectDrawSurface4* GetSurface(int id);
    w32::IDirectDraw4* GetDirectDraw();
    w32::DWORD DetectPixelFormat();
    w32::LONG GetBackSize(w32::LONG* pitch, w32::LONG* height);

    w32::IDirectDraw4* m_directDraw;         // +0x00
    w32::IDirectDrawSurface4* m_primary;     // +0x04
    w32::IDirectDrawSurface4* m_back;        // +0x08
    w32::IDirectDrawSurface4* m_third;       // +0x0C
    w32::IDirectDrawClipper* m_clipper;      // +0x10
    w32::IUnknownLike* m_extra;              // +0x14 released by ReleaseSurfaces, never set here
    std::uint8_t m_pad18[0x10];
    w32::DWORD m_videoMemoryUsed;            // +0x28 bytes of offscreen surfaces created
    w32::DWORD m_videoMemoryTotal;           // +0x2C
    w32::DWORD m_videoMemoryFree;            // +0x30
    w32::DWORD m_cooperativeFlags;           // +0x34
    w32::LONG m_width;                       // +0x38
    w32::LONG m_height;                      // +0x3C
    std::uint8_t m_pad40[8];
    std::uint8_t m_pixelFormat;              // +0x48 0 / 1 (blue 0x3F) / 2 (green 0x7E0) / 3 (red 0xFC00)
    std::uint8_t m_pad49[0xB];
    w32::LONG m_primaryPitch;                // +0x54
    w32::LONG m_primaryHeight;               // +0x58
    w32::LONG m_backPitch;                   // +0x5C
    w32::LONG m_backHeight;                  // +0x60
    std::uint8_t m_pad64[0x20];
    w32::HWND m_window;                      // +0x84
    w32::DWORD m_clearColor;                 // +0x88 fill colour of ClearSurface
};

static_assert(sizeof(Graphics) == 0x8C);

void ShowDirectDrawError(w32::HWND window, const char* caption, w32::HRESULT result);
w32::HRESULT FillSurfaceColor(w32::IDirectDrawSurface4* surface, w32::COLORREF color);
w32::DWORD MatchSurfaceColor(w32::IDirectDrawSurface4* surface, w32::COLORREF color);

}  // namespace swd3
