#include "gfx/graphics.h"

#include <cstring>

#include "data/globals.h"

namespace swd3 {

using namespace w32;

namespace {

constexpr DWORD kCapsThreeD = 0x2000;
constexpr DWORD kCapsTexture = 0x1000;
constexpr DWORD kSetDisplayModeStandardVga = 1;
constexpr DWORD kSurfaceDescHeightPitch = DDSD_HEIGHT | DDSD_PITCH;
constexpr DWORD kCooperativeFullscreen = 0x13;

void zeroDesc(DDSURFACEDESC2& desc, DWORD flags) {
    std::memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(DDSURFACEDESC2);
    desc.dwFlags = flags;
}

}  // namespace

// swd32.exe 0x004311C0: message box naming a DirectDraw error code (unknown codes show nothing). The original
// returns a leftover switch value that no caller reads.
void ShowDirectDrawError(HWND window, const char* caption, HRESULT result) {
    const char* text = nullptr;
    switch (static_cast<std::uint32_t>(result)) {
    case 0x88760232u: text = "DDERR_DIRECTDRAWALREADYCREATED "; break;
    case 0x88760233u: text = "DDERR_NODIRECTDRAWHW "; break;
    case 0x88760234u: text = "DDERR_PRIMARYSURFACEALREADYEXISTS  "; break;
    case 0x88760235u: text = "DDERR_NOEMULATION  "; break;
    case 0x88760238u: text = "DDERR_NOCLIPPERATTACHED  "; break;
    case 0x8876023Au: text = "DDERR_HWNDSUBCLASSED  "; break;
    case 0x8876023Bu: text = "DDERR_HWNDALREADYSET  "; break;
    case 0x88760245u: text = "DDERR_EXCLUSIVEMODEALREADYSET  "; break;
    case 0x8876024Eu: text = "DDERR_UNSUPPORTEDMODE  "; break;
    case 0x88760250u: text = "DDERR_INVALIDSURFACETYPE  "; break;
    case 0x88760231u: text = "DDERR_INVALIDDIRECTDRAWGUID "; break;
    case 0x88760082u: text = "DDERR_INVALIDOBJECT  "; break;
    case 0x887600D4u: text = "DDERR_NOCOOPERATIVELEVELSET  "; break;
    case 0x887600E1u: text = "DDERR_NOEXCLUSIVEMODE "; break;
    case 0x887600E6u: text = "DDERR_NOFLIPHW "; break;
    case 0x8876017Cu: text = "DDERR_OUTOFVIDEOMEMORY   "; break;
    case 0x8876006Eu: text = "DDERR_INVALIDCLIPLIST "; break;
    case 0x8876005Fu: text = "DDERR_INCOMPATIBLEPRIMARY  "; break;
    case 0x88760064u: text = "DDERR_INVALIDCAPS "; break;
    case 0x80070057u: text = "DDERR_INVALIDPARAMS  "; break;
    case 0x80004005u: text = "DDERR_GENERIC "; break;
    case 0x8007000Eu: text = "DDERR_OUTOFMEMORY "; break;
    default: break;
    }
    if (text != nullptr) {
        MessageBoxA(window, text, caption, MB_OK);
    }
}

// swd32.exe 0x00437560
Graphics::Graphics() {
    m_directDraw = nullptr;
    m_primary = nullptr;
    m_back = nullptr;
}

// swd32.exe 0x00437570: mode kDisplayWindowed uses DDSCL_NORMAL, anything else fullscreen exclusive
int Graphics::Init(HWND window, int mode, int width, int height, int bpp) {
    m_window = window;
    m_width = width;
    m_height = height;
    if (CreateDirectDraw() != 1) {
        return 0;
    }
    const int result = mode == kDisplayWindowed ? SetCooperativeLevel(window, DDSCL_NORMAL)
                                                : SetCooperativeLevel(m_window, kCooperativeFullscreen);
    if (result != 1) {
        return 0;
    }
    if (SetDisplayMode(m_window, width, height, bpp) != 1) {
        return 0;
    }
    if (CreateSurfaces() != 1) {
        return 0;
    }
    DetectPixelFormat();
    return 1;
}

// swd32.exe 0x00437610: the IDirectDraw object is leaked when the IDirectDraw4 query fails
int Graphics::CreateDirectDraw() {
    IDirectDraw* directDraw = nullptr;
    const HRESULT result = DirectDrawCreate(nullptr, &directDraw, nullptr);
    if (result != 0) {
        ShowDirectDrawError(m_window, "DirectDraw Create Error!!", result);
        return 0;
    }
    if (directDraw->QueryInterface(G.iidDirectDraw4, reinterpret_cast<void**>(&m_directDraw)) != 0) {
        return 0;
    }
    directDraw->Release();
    return 1;
}

// swd32.exe 0x00437680
int Graphics::SetCooperativeLevel(HWND window, int flags) {
    const HRESULT result = m_directDraw->SetCooperativeLevel(window, static_cast<DWORD>(flags));
    m_cooperativeFlags = static_cast<DWORD>(flags);
    if (result == 0) {
        return 1;
    }
    ShowDirectDrawError(window, "DirectDraw SetCooper Error!!", result);
    m_directDraw->Release();
    return 0;
}

// swd32.exe 0x004376D0
int Graphics::SetDisplayMode(HWND window, int width, int height, int bpp) {
    const HRESULT result = m_directDraw->SetDisplayMode(static_cast<DWORD>(width), static_cast<DWORD>(height),
                                                        static_cast<DWORD>(bpp), 0, kSetDisplayModeStandardVga);
    if (result == 0) {
        return 1;
    }
    ShowDirectDrawError(window, "DirectDraw SetDisplayMode Error!!", result);
    m_directDraw->Release();
    return 0;
}

// swd32.exe 0x00437720
int Graphics::CreateClipper(HWND window) {
    HRESULT result = m_directDraw->CreateClipper(0, &m_clipper, nullptr);
    if (result != 0) {
        ShowDirectDrawError(window, "DirectDraw CreateClipper Error!!", result);
    } else {
        result = m_clipper->SetHWnd(0, window);
        if (result != 0) {
            ShowDirectDrawError(window, "DirectDraw Clipper SetHwnd error!!", result);
            m_clipper->Release();
        } else {
            result = m_primary->SetClipper(m_clipper);
            if (result == 0) {
                return 1;
            }
            ShowDirectDrawError(window, "DirectDraw SetClipper Error!!", result);
            m_clipper->Release();
        }
    }
    m_primary->Release();
    m_directDraw->Release();
    return 0;
}

// swd32.exe 0x004377D0: primary surface with clipper; in DDSCL_NORMAL also a system memory back buffer of the
// desktop size. Records the pitch and height of both (the primary twice when there is no back buffer).
int Graphics::CreateSurfaces() {
    QueryVideoMemory();
    DDSURFACEDESC2 desc;
    IDirectDrawSurface4* surface;
    if (m_cooperativeFlags == DDSCL_NORMAL) {
        zeroDesc(desc, DDSD_CAPS);
        desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (m_directDraw->CreateSurface(&desc, &surface, nullptr) != 0 ||
            surface->QueryInterface(G.iidDirectDrawSurface4, reinterpret_cast<void**>(&m_primary)) != 0) {
            m_directDraw->Release();
            return 0;
        }
        surface->Release();
        if (CreateClipper(m_window) != 1) {
            return 0;
        }
        m_width = GetSystemMetrics(SM_CXSCREEN);
        m_height = GetSystemMetrics(SM_CYSCREEN);
        const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        m_back = CreateOffscreen(screenWidth, screenHeight);
        zeroDesc(desc, kSurfaceDescHeightPitch);
        m_primary->GetSurfaceDesc(&desc);
        m_primaryPitch = desc.lPitch;
        m_primaryHeight = static_cast<LONG>(desc.dwHeight);
        zeroDesc(desc, kSurfaceDescHeightPitch);
        m_back->GetSurfaceDesc(&desc);
        m_backPitch = desc.lPitch;
        m_backHeight = static_cast<LONG>(desc.dwHeight);
        return 1;
    }
    zeroDesc(desc, DDSD_CAPS);
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    desc.dwBackBufferCount = 0;
    const HRESULT result = m_directDraw->CreateSurface(&desc, &surface, nullptr);
    if (result != 0) {
        ShowDirectDrawError(m_window, "DirectDraw Create Primary Surface Error!!", result);
        m_directDraw->Release();
        return 0;
    }
    if (surface->QueryInterface(G.iidDirectDrawSurface4, reinterpret_cast<void**>(&m_primary)) != 0) {
        m_directDraw->Release();
        return 0;
    }
    surface->Release();
    if (CreateClipper(m_window) != 1) {
        return 0;
    }
    zeroDesc(desc, kSurfaceDescHeightPitch);
    m_primary->GetSurfaceDesc(&desc);
    m_primaryPitch = desc.lPitch;
    m_primaryHeight = static_cast<LONG>(desc.dwHeight);
    m_backPitch = desc.lPitch;
    m_backHeight = static_cast<LONG>(desc.dwHeight);
    return 1;
}

// swd32.exe 0x00437A50
int Graphics::ReleaseSurfaces() {
    if (m_directDraw == nullptr) {
        return 0;
    }
    if (m_third != nullptr) {
        m_third->Release();
        m_third = nullptr;
    }
    if (m_back != nullptr) {
        m_back->Release();
        m_back = nullptr;
    }
    if (m_clipper != nullptr) {
        m_clipper->Release();
        m_clipper = nullptr;
    }
    if (m_extra != nullptr) {
        m_extra->Release();
        m_extra = nullptr;
    }
    if (m_primary != nullptr) {
        m_primary->Release();
        m_primary = nullptr;
    }
    if (static_cast<std::uint8_t>(m_cooperativeFlags) & DDSCL_EXCLUSIVE) {
        m_directDraw->RestoreDisplayMode();
    }
    return 1;
}

// swd32.exe 0x00437AD0
DWORD Graphics::ReleaseDirectDraw() {
    if (m_directDraw == nullptr) {
        return 0;
    }
    return m_directDraw->Release();
}

// swd32.exe 0x00437AE0: fullscreen flips after the vertical blank; windowed blits the back buffer to the primary
// surface, retrying until the blit succeeds
int Graphics::Present(int) {
    if (static_cast<std::uint8_t>(m_cooperativeFlags) & DDSCL_FULLSCREEN) {
        if (m_primary->IsLost() == DDERR_SURFACELOST) {
            m_primary->Restore();
        }
        m_directDraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, nullptr);
        m_primary->Flip(nullptr, DDFLIP_WAIT);
        return 1;
    }
    for (;;) {
        const HRESULT result = m_primary->Blt(nullptr, m_back, nullptr, DDBLT_WAIT, nullptr);
        if (result == 0) {
            break;
        }
        if (result == DDERR_SURFACELOST) {
            m_primary->Restore();
            m_back->Restore();
        }
    }
    return 1;
}

// swd32.exe 0x00437B60: system memory surface; adds width * height * bytes per pixel to m_videoMemoryUsed
IDirectDrawSurface4* Graphics::CreateOffscreen(int width, int height) {
    DDSURFACEDESC2 desc;
    zeroDesc(desc, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH);
    desc.ddsCaps.dwCaps = kCapsThreeD | DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = static_cast<DWORD>(width);
    desc.dwHeight = static_cast<DWORD>(height);
    IDirectDrawSurface4* surface;
    if (m_directDraw->CreateSurface(&desc, &surface, nullptr) != 0) {
        return nullptr;
    }
    m_videoMemoryUsed += static_cast<DWORD>(height * width) * (GetBitDepth() >> 3);
    return surface;
}

// swd32.exe 0x00437C00: fills the surface with the device colour matching `color`
HRESULT FillSurfaceColor(IDirectDrawSurface4* surface, COLORREF color) {
    DDBLTFX fx{};
    fx.dwSize = sizeof(DDBLTFX);
    fx.dwFillColor = MatchSurfaceColor(surface, color);
    return surface->Blt(nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
}

// swd32.exe 0x00437C40
HRESULT Graphics::ClearSurface(IDirectDrawSurface4* surface) {
    DDBLTFX fx{};
    fx.dwFillColor = m_clearColor;
    fx.dwSize = sizeof(DDBLTFX);
    return surface->Blt(nullptr, nullptr, nullptr, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
}

// swd32.exe 0x00437C80: device colour of `color` (CLR_INVALID: the current top-left pixel) read back after a
// GDI SetPixel at (0, 0); restores that pixel afterwards
DWORD MatchSurfaceColor(IDirectDrawSurface4* surface, COLORREF color) {
    DWORD result = 0xFFFFFFFFu;
    HDC dc = nullptr;
    COLORREF saved = 0;
    if (color != 0xFFFFFFFFu && surface->GetDC(&dc) == 0) {
        saved = GetPixel(dc, 0, 0);
        SetPixel(dc, 0, 0, color);
        surface->ReleaseDC(dc);
    } else {
        saved = static_cast<COLORREF>(reinterpret_cast<std::uintptr_t>(dc));
    }
    DDSURFACEDESC2 desc;
    desc.dwSize = sizeof(DDSURFACEDESC2);
    HRESULT locked = surface->Lock(nullptr, &desc, 0, nullptr);
    while (locked == DDERR_WASSTILLDRAWING) {
        locked = surface->Lock(nullptr, &desc, 0, nullptr);
    }
    if (locked == 0) {
        std::memcpy(&result, desc.lpSurface, 4);
        if (desc.ddpfPixelFormat.dwRGBBitCount < 32) {
            result &= (1u << desc.ddpfPixelFormat.dwRGBBitCount) - 1;
        }
        surface->Unlock(nullptr);
    }
    if (color != 0xFFFFFFFFu && surface->GetDC(&dc) == 0) {
        SetPixel(dc, 0, 0, saved);
        surface->ReleaseDC(dc);
    }
    return result;
}

// swd32.exe 0x00437D80
DWORD Graphics::GetBitDepth() {
    DDSURFACEDESC2 desc;
    zeroDesc(desc, DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT);
    m_directDraw->GetDisplayMode(&desc);
    return desc.ddpfPixelFormat.dwRGBBitCount;
}

// swd32.exe 0x00437DC0
BOOL Graphics::QueryVideoMemory() {
    DDSCAPS2 caps{};
    caps.dwCaps = kCapsTexture;
    return m_directDraw->GetAvailableVidMem(&caps, &m_videoMemoryTotal, &m_videoMemoryFree) >= 0;
}

// swd32.exe 0x00437DF0
IDirectDrawSurface4* Graphics::GetSurface(int id) {
    switch (id) {
    case kSurfacePrimary:
        return m_primary;
    case kSurfaceBack:
        return m_back;
    case kSurfaceThird:
        return m_third;
    default:
        return nullptr;
    }
}

// swd32.exe 0x00437E20
IDirectDraw4* Graphics::GetDirectDraw() { return m_directDraw; }

// swd32.exe 0x00437E30: classifies the primary surface masks; returns the green mask
DWORD Graphics::DetectPixelFormat() {
    DDPIXELFORMAT format;
    format.dwSize = sizeof(DDPIXELFORMAT);
    format.dwFlags = DDPF_RGB;
    m_primary->GetPixelFormat(&format);
    m_pixelFormat = 0;
    if (format.dwGBitMask == 0x7E0) {
        m_pixelFormat = 2;
    }
    if (format.dwBBitMask == 0x3F) {
        m_pixelFormat = 1;
    }
    if (format.dwRBitMask == 0xFC00) {
        m_pixelFormat = 3;
    }
    return format.dwGBitMask;
}

// swd32.exe 0x00437E90
LONG Graphics::GetBackSize(LONG* pitch, LONG* height) {
    *pitch = m_backPitch;
    *height = m_backHeight;
    return m_backHeight;
}

}  // namespace swd3
