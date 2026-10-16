#pragma once

#include <cstdint>

#include "swd3/w32.h"

// DirectDraw interfaces as used by swd32.exe (IDirectDraw4 / IDirectDrawSurface4, IIDs at 0x00499D88 / 0x00499DC8). The vtable layout follows ddraw.h (IUnknown first, then the
// methods in declaration order) so that the game code calls the same slots as the original. The
// implementation (src/platform/ddraw*.cpp) emulates DirectDraw on SDL3.
namespace w32 {

using HRESULT = std::int32_t;
using HDC = void*;

constexpr HRESULT DD_OK = 0;
constexpr HRESULT DDERR_GENERIC = static_cast<HRESULT>(0x80004005u);
constexpr HRESULT DDERR_INVALIDPARAMS = static_cast<HRESULT>(0x80070057u);
constexpr HRESULT DDERR_SURFACELOST = static_cast<HRESULT>(0x887601C2u);
constexpr HRESULT DDERR_WASSTILLDRAWING = static_cast<HRESULT>(0x8876021Cu);
constexpr HRESULT DDERR_NOTFOUND = static_cast<HRESULT>(0x887600FFu);
constexpr HRESULT E_NOINTERFACE = static_cast<HRESULT>(0x80004002u);

struct GUID {
    std::uint32_t Data1;
    std::uint16_t Data2;
    std::uint16_t Data3;
    std::uint8_t Data4[8];
};
using REFIID = const GUID&;

struct RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

struct DDCOLORKEY {
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
};

struct DDPIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwRGBAlphaBitMask;
};

struct DDSCAPS2 {
    DWORD dwCaps;
    DWORD dwCaps2;
    DWORD dwCaps3;
    DWORD dwCaps4;
};

struct DDSURFACEDESC2 {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    LONG lPitch;
    DWORD dwBackBufferCount;
    DWORD dwRefreshRate;
    DWORD dwAlphaBitDepth;
    DWORD dwReserved;
    LPVOID lpSurface;
    DDCOLORKEY ddckCKDestOverlay;
    DDCOLORKEY ddckCKDestBlt;
    DDCOLORKEY ddckCKSrcOverlay;
    DDCOLORKEY ddckCKSrcBlt;
    DDPIXELFORMAT ddpfPixelFormat;
    DDSCAPS2 ddsCaps;
    DWORD dwTextureStage;
};

struct DDCAPS_DX5 {
    DWORD dwSize;
    std::uint8_t rest[0x168];
};

struct DDBLTFX {
    DWORD dwSize;
    DWORD dwDDFX;
    DWORD dwROP;
    DWORD dwDDROP;
    DWORD dwRotationAngle;
    DWORD dwZBufferOpCode;
    DWORD dwZBufferLow;
    DWORD dwZBufferHigh;
    DWORD dwZBufferBaseDest;
    DWORD dwZDestConstBitDepth;
    DWORD dwZDestConst;
    DWORD dwZSrcConstBitDepth;
    DWORD dwZSrcConst;
    DWORD dwAlphaEdgeBlendBitDepth;
    DWORD dwAlphaEdgeBlend;
    DWORD dwReserved;
    DWORD dwAlphaDestConstBitDepth;
    DWORD dwAlphaDestConst;
    DWORD dwAlphaSrcConstBitDepth;
    DWORD dwAlphaSrcConst;
    DWORD dwFillColor;
    DDCOLORKEY ddckDestColorkey;
    DDCOLORKEY ddckSrcColorkey;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(DDPIXELFORMAT) == 0x20);
static_assert(sizeof(DDSURFACEDESC2) == 0x7C);
static_assert(sizeof(DDBLTFX) == 0x64);
#endif

constexpr DWORD DDSD_CAPS = 0x1;
constexpr DWORD DDSD_HEIGHT = 0x2;
constexpr DWORD DDSD_WIDTH = 0x4;
constexpr DWORD DDSD_PITCH = 0x8;
constexpr DWORD DDSD_PIXELFORMAT = 0x1000;
constexpr DWORD DDSCAPS_OFFSCREENPLAIN = 0x40;
constexpr DWORD DDSCAPS_PRIMARYSURFACE = 0x200;
constexpr DWORD DDSCAPS_SYSTEMMEMORY = 0x800;
constexpr DWORD DDSCAPS_VIDEOMEMORY = 0x4000;
constexpr DWORD DDLOCK_WAIT = 0x1;
constexpr DWORD DDBLT_COLORFILL = 0x400;
constexpr DWORD DDBLT_KEYSRC = 0x8000;
constexpr DWORD DDBLT_WAIT = 0x1000000;
constexpr DWORD DDBLTFAST_WAIT = 0x10;
constexpr DWORD DDFLIP_WAIT = 0x1;
constexpr DWORD DDPF_RGB = 0x40;
constexpr DWORD DDSCL_NORMAL = 0x8;
constexpr DWORD DDSCL_FULLSCREEN = 0x1;
constexpr DWORD DDSCL_ALLOWREBOOT = 0x2;
constexpr DWORD DDSCL_EXCLUSIVE = 0x10;
constexpr DWORD DDWAITVB_BLOCKBEGIN = 0x1;

struct IUnknownLike {
    virtual HRESULT QueryInterface(REFIID riid, void** object) = 0;
    virtual DWORD AddRef() = 0;
    virtual DWORD Release() = 0;
};

struct IDirectDraw : IUnknownLike {};

struct IDirectDrawClipper : IUnknownLike {
    virtual HRESULT GetClipList(RECT* rect, void* clipList, DWORD* size) = 0;
    virtual HRESULT GetHWnd(HWND* window) = 0;
    virtual HRESULT Initialize(void* directDraw, DWORD flags) = 0;
    virtual HRESULT IsClipListChanged(BOOL* changed) = 0;
    virtual HRESULT SetClipList(void* clipList, DWORD flags) = 0;
    virtual HRESULT SetHWnd(DWORD flags, HWND window) = 0;
};

struct IDirectDrawPalette : IUnknownLike {
    virtual HRESULT GetCaps(DWORD* caps) = 0;
    virtual HRESULT GetEntries(DWORD flags, DWORD base, DWORD count, void* entries) = 0;
    virtual HRESULT Initialize(void* directDraw, DWORD flags, void* entries) = 0;
    virtual HRESULT SetEntries(DWORD flags, DWORD start, DWORD count, void* entries) = 0;
};

struct IDirectDrawSurface4 : IUnknownLike {
    virtual HRESULT AddAttachedSurface(IDirectDrawSurface4* surface) = 0;
    virtual HRESULT AddOverlayDirtyRect(RECT* rect) = 0;
    virtual HRESULT Blt(RECT* destRect, IDirectDrawSurface4* source, RECT* sourceRect, DWORD flags, DDBLTFX* fx) = 0;
    virtual HRESULT BltBatch(void* batch, DWORD count, DWORD flags) = 0;
    virtual HRESULT BltFast(DWORD x, DWORD y, IDirectDrawSurface4* source, RECT* sourceRect, DWORD trans) = 0;
    virtual HRESULT DeleteAttachedSurface(DWORD flags, IDirectDrawSurface4* surface) = 0;
    virtual HRESULT EnumAttachedSurfaces(void* context, void* callback) = 0;
    virtual HRESULT EnumOverlayZOrders(DWORD flags, void* context, void* callback) = 0;
    virtual HRESULT Flip(IDirectDrawSurface4* target, DWORD flags) = 0;
    virtual HRESULT GetAttachedSurface(DDSCAPS2* caps, IDirectDrawSurface4** surface) = 0;
    virtual HRESULT GetBltStatus(DWORD flags) = 0;
    virtual HRESULT GetCaps(DDSCAPS2* caps) = 0;
    virtual HRESULT GetClipper(IDirectDrawClipper** clipper) = 0;
    virtual HRESULT GetColorKey(DWORD flags, DDCOLORKEY* key) = 0;
    virtual HRESULT GetDC(HDC* dc) = 0;
    virtual HRESULT GetFlipStatus(DWORD flags) = 0;
    virtual HRESULT GetOverlayPosition(LONG* x, LONG* y) = 0;
    virtual HRESULT GetPalette(IDirectDrawPalette** palette) = 0;
    virtual HRESULT GetPixelFormat(DDPIXELFORMAT* format) = 0;
    virtual HRESULT GetSurfaceDesc(DDSURFACEDESC2* desc) = 0;
    virtual HRESULT Initialize(void* directDraw, DDSURFACEDESC2* desc) = 0;
    virtual HRESULT IsLost() = 0;
    virtual HRESULT Lock(RECT* rect, DDSURFACEDESC2* desc, DWORD flags, HANDLE event) = 0;
    virtual HRESULT ReleaseDC(HDC dc) = 0;
    virtual HRESULT Restore() = 0;
    virtual HRESULT SetClipper(IDirectDrawClipper* clipper) = 0;
    virtual HRESULT SetColorKey(DWORD flags, DDCOLORKEY* key) = 0;
    virtual HRESULT SetOverlayPosition(LONG x, LONG y) = 0;
    virtual HRESULT SetPalette(IDirectDrawPalette* palette) = 0;
    virtual HRESULT Unlock(LPVOID surface) = 0;
    virtual HRESULT UpdateOverlay(RECT* sourceRect, IDirectDrawSurface4* dest, RECT* destRect, DWORD flags, void* fx) = 0;
    virtual HRESULT UpdateOverlayDisplay(DWORD flags) = 0;
    virtual HRESULT UpdateOverlayZOrder(DWORD flags, IDirectDrawSurface4* reference) = 0;
    virtual HRESULT GetDDInterface(void** directDraw) = 0;
    virtual HRESULT PageLock(DWORD flags) = 0;
    virtual HRESULT PageUnlock(DWORD flags) = 0;
    virtual HRESULT SetSurfaceDesc(DDSURFACEDESC2* desc, DWORD flags) = 0;
    virtual HRESULT SetPrivateData(REFIID tag, void* data, DWORD size, DWORD flags) = 0;
    virtual HRESULT GetPrivateData(REFIID tag, void* buffer, DWORD* size) = 0;
    virtual HRESULT FreePrivateData(REFIID tag) = 0;
    virtual HRESULT GetUniquenessValue(DWORD* value) = 0;
    virtual HRESULT ChangeUniquenessValue() = 0;
};

struct IDirectDraw4 : IUnknownLike {
    virtual HRESULT Compact() = 0;
    virtual HRESULT CreateClipper(DWORD flags, IDirectDrawClipper** clipper, void* outer) = 0;
    virtual HRESULT CreatePalette(DWORD flags, void* entries, IDirectDrawPalette** palette, void* outer) = 0;
    virtual HRESULT CreateSurface(DDSURFACEDESC2* desc, IDirectDrawSurface4** surface, void* outer) = 0;
    virtual HRESULT DuplicateSurface(IDirectDrawSurface4* source, IDirectDrawSurface4** copy) = 0;
    virtual HRESULT EnumDisplayModes(DWORD flags, DDSURFACEDESC2* desc, void* context, void* callback) = 0;
    virtual HRESULT EnumSurfaces(DWORD flags, DDSURFACEDESC2* desc, void* context, void* callback) = 0;
    virtual HRESULT FlipToGDISurface() = 0;
    virtual HRESULT GetCaps(DDCAPS_DX5* driverCaps, DDCAPS_DX5* helCaps) = 0;
    virtual HRESULT GetDisplayMode(DDSURFACEDESC2* desc) = 0;
    virtual HRESULT GetFourCCCodes(DWORD* count, DWORD* codes) = 0;
    virtual HRESULT GetGDISurface(IDirectDrawSurface4** surface) = 0;
    virtual HRESULT GetMonitorFrequency(DWORD* frequency) = 0;
    virtual HRESULT GetScanLine(DWORD* line) = 0;
    virtual HRESULT GetVerticalBlankStatus(BOOL* inBlank) = 0;
    virtual HRESULT Initialize(GUID* guid) = 0;
    virtual HRESULT RestoreDisplayMode() = 0;
    virtual HRESULT SetCooperativeLevel(HWND window, DWORD flags) = 0;
    virtual HRESULT SetDisplayMode(DWORD width, DWORD height, DWORD bpp, DWORD refreshRate, DWORD flags) = 0;
    virtual HRESULT WaitForVerticalBlank(DWORD flags, HANDLE event) = 0;
    virtual HRESULT GetAvailableVidMem(DDSCAPS2* caps, DWORD* total, DWORD* free) = 0;
    virtual HRESULT GetSurfaceFromDC(HDC dc, IDirectDrawSurface4** surface) = 0;
    virtual HRESULT RestoreAllSurfaces() = 0;
    virtual HRESULT TestCooperativeLevel() = 0;
    virtual HRESULT GetDeviceIdentifier(void* identifier, DWORD flags) = 0;
};

// GDI pixel access on a surface DC (GetDC / ReleaseDC).
COLORREF GetPixel(HDC dc, int x, int y);
COLORREF SetPixel(HDC dc, int x, int y, COLORREF color);

// Returns an IDirectDraw object; the game immediately queries IID_IDirectDraw4.
HRESULT DirectDrawCreate(GUID* driver, IDirectDraw** directDraw, void* outer);

}  // namespace w32
