// DirectDraw emulation: 16-bit RGB565 system memory surfaces; the primary surface is shown through SDL3
// whenever it is blitted to or flipped.
#include <cstring>
#include <vector>

#include "platform/gdi_surface.h"
#include "platform/video.h"
#include "swd3/ddraw.h"

namespace w32 {
namespace {

constexpr GUID kIidDirectDraw4{0x9C59509Au, 0x39BD, 0x11D1, {0x8C, 0x4A, 0x00, 0xC0, 0x4F, 0xD9, 0x30, 0xC5}};
constexpr GUID kIidDirectDrawSurface4{0x0B2B8630u, 0xAD35, 0x11D0, {0x8E, 0xA6, 0x00, 0x60, 0x97, 0x97, 0xEA, 0x5B}};
constexpr HRESULT kCantCreateDC = static_cast<HRESULT>(0x887602A1u);
constexpr HRESULT kNoColorKey = static_cast<HRESULT>(0x887600D7u);
constexpr DWORD kKeySrcBlt = 0x8;
constexpr DWORD kBltKeySrc = 0x8000;
constexpr DWORD kBltColorFill = 0x400;
constexpr DWORD kBltFastSrcColorKey = 0x1;

bool sameGuid(const GUID& a, const GUID& b) { return std::memcmp(&a, &b, sizeof(GUID)) == 0; }

void fillFormat(DDPIXELFORMAT& format) {
    format.dwSize = sizeof(DDPIXELFORMAT);
    format.dwFlags = DDPF_RGB;
    format.dwFourCC = 0;
    format.dwRGBBitCount = 16;
    format.dwRBitMask = 0xF800;
    format.dwGBitMask = 0x07E0;
    format.dwBBitMask = 0x001F;
    format.dwRGBAlphaBitMask = 0;
}

struct Rect {
    LONG left, top, right, bottom;
};

class Clipper final : public IDirectDrawClipper {
public:
    HRESULT QueryInterface(REFIID, void** object) override {
        *object = nullptr;
        return E_NOINTERFACE;
    }
    DWORD AddRef() override { return ++m_refs; }
    DWORD Release() override {
        const DWORD refs = --m_refs;
        if (refs == 0) {
            delete this;
        }
        return refs;
    }
    HRESULT GetClipList(RECT*, void*, DWORD*) override { return DDERR_GENERIC; }
    HRESULT GetHWnd(HWND* window) override {
        *window = m_window;
        return DD_OK;
    }
    HRESULT Initialize(void*, DWORD) override { return DD_OK; }
    HRESULT IsClipListChanged(BOOL* changed) override {
        *changed = 0;
        return DD_OK;
    }
    HRESULT SetClipList(void*, DWORD) override { return DD_OK; }
    HRESULT SetHWnd(DWORD, HWND window) override {
        m_window = window;
        return DD_OK;
    }

private:
    DWORD m_refs = 1;
    HWND m_window = nullptr;
};

class Surface final : public IDirectDrawSurface4 {
public:
    Surface(int width, int height, bool primary)
        : m_width(width), m_height(height), m_pitch(((width * 2) + 3) & ~3), m_primary(primary),
          m_pixels(static_cast<std::size_t>(m_pitch) * height / 2 + 2) {}

    HRESULT QueryInterface(REFIID iid, void** object) override {
        if (sameGuid(iid, kIidDirectDrawSurface4)) {
            AddRef();
            *object = static_cast<IDirectDrawSurface4*>(this);
            return DD_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    DWORD AddRef() override { return ++m_refs; }
    DWORD Release() override {
        const DWORD refs = --m_refs;
        if (refs == 0) {
            if (m_clipper != nullptr) {
                m_clipper->Release();
            }
            delete this;
        }
        return refs;
    }
    HRESULT AddAttachedSurface(IDirectDrawSurface4*) override { return DDERR_GENERIC; }
    HRESULT AddOverlayDirtyRect(RECT*) override { return DDERR_GENERIC; }

    HRESULT Blt(RECT* destRect, IDirectDrawSurface4* source, RECT* sourceRect, DWORD flags, DDBLTFX* fx) override {
        const Rect dest = clampRect(destRect, m_width, m_height);
        if (flags & kBltColorFill) {
            const auto color = static_cast<std::uint16_t>(fx != nullptr ? fx->dwFillColor : 0);
            for (LONG y = dest.top; y < dest.bottom; ++y) {
                for (LONG x = dest.left; x < dest.right; ++x) {
                    at(x, y) = color;
                }
            }
        } else if (source != nullptr) {
            auto* src = static_cast<Surface*>(source);
            const Rect from = clampRect(sourceRect, src->m_width, src->m_height);
            const bool keyed = (flags & kBltKeySrc) != 0 && src->m_hasSourceKey;
            stretch(*src, from, dest, keyed);
        }
        presentIfPrimary();
        return DD_OK;
    }
    HRESULT BltBatch(void*, DWORD, DWORD) override { return DDERR_GENERIC; }
    HRESULT BltFast(DWORD x, DWORD y, IDirectDrawSurface4* source, RECT* sourceRect, DWORD trans) override {
        auto* src = static_cast<Surface*>(source);
        const Rect from = clampRect(sourceRect, src->m_width, src->m_height);
        const Rect dest{static_cast<LONG>(x), static_cast<LONG>(y), static_cast<LONG>(x) + (from.right - from.left),
                        static_cast<LONG>(y) + (from.bottom - from.top)};
        stretch(*src, from, dest, (trans & kBltFastSrcColorKey) != 0 && src->m_hasSourceKey);
        presentIfPrimary();
        return DD_OK;
    }
    HRESULT DeleteAttachedSurface(DWORD, IDirectDrawSurface4*) override { return DDERR_GENERIC; }
    HRESULT EnumAttachedSurfaces(void*, void*) override { return DDERR_GENERIC; }
    HRESULT EnumOverlayZOrders(DWORD, void*, void*) override { return DDERR_GENERIC; }
    HRESULT Flip(IDirectDrawSurface4*, DWORD) override {
        presentIfPrimary();
        return DD_OK;
    }
    HRESULT GetAttachedSurface(DDSCAPS2*, IDirectDrawSurface4** surface) override {
        *surface = nullptr;
        return DDERR_NOTFOUND;
    }
    HRESULT GetBltStatus(DWORD) override { return DD_OK; }
    HRESULT GetCaps(DDSCAPS2* caps) override {
        *caps = DDSCAPS2{};
        caps->dwCaps = m_primary ? DDSCAPS_PRIMARYSURFACE : DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
        return DD_OK;
    }
    HRESULT GetClipper(IDirectDrawClipper** clipper) override {
        *clipper = m_clipper;
        if (m_clipper != nullptr) {
            m_clipper->AddRef();
        }
        return m_clipper != nullptr ? DD_OK : DDERR_NOTFOUND;
    }
    HRESULT GetColorKey(DWORD flags, DDCOLORKEY* key) override {
        if ((flags & kKeySrcBlt) && m_hasSourceKey) {
            *key = m_sourceKey;
            return DD_OK;
        }
        return kNoColorKey;
    }
    HRESULT GetDC(HDC* dc) override {
        *dc = swd3::platform::beginSurfaceDC(m_pixels.data(), m_width, m_height, m_pitch);
        return *dc != nullptr ? DD_OK : kCantCreateDC;
    }
    HRESULT GetFlipStatus(DWORD) override { return DD_OK; }
    HRESULT GetOverlayPosition(LONG*, LONG*) override { return DDERR_GENERIC; }
    HRESULT GetPalette(IDirectDrawPalette**) override { return DDERR_GENERIC; }
    HRESULT GetPixelFormat(DDPIXELFORMAT* format) override {
        fillFormat(*format);
        return DD_OK;
    }
    HRESULT GetSurfaceDesc(DDSURFACEDESC2* desc) override {
        describe(*desc);
        return DD_OK;
    }
    HRESULT Initialize(void*, DDSURFACEDESC2*) override { return DD_OK; }
    HRESULT IsLost() override { return DD_OK; }
    HRESULT Lock(RECT* rect, DDSURFACEDESC2* desc, DWORD, HANDLE) override {
        describe(*desc);
        if (rect != nullptr) {
            desc->lpSurface = &at(rect->left, rect->top);
        }
        return DD_OK;
    }
    HRESULT ReleaseDC(HDC dc) override {
        swd3::platform::endSurfaceDC(dc, m_pixels.data(), m_width, m_height, m_pitch);
        presentIfPrimary();
        return DD_OK;
    }
    HRESULT Restore() override { return DD_OK; }
    HRESULT SetClipper(IDirectDrawClipper* clipper) override {
        if (clipper != nullptr) {
            clipper->AddRef();
        }
        if (m_clipper != nullptr) {
            m_clipper->Release();
        }
        m_clipper = clipper;
        return DD_OK;
    }
    HRESULT SetColorKey(DWORD flags, DDCOLORKEY* key) override {
        if (flags & kKeySrcBlt) {
            m_hasSourceKey = key != nullptr;
            if (key != nullptr) {
                m_sourceKey = *key;
            }
        }
        return DD_OK;
    }
    HRESULT SetOverlayPosition(LONG, LONG) override { return DDERR_GENERIC; }
    HRESULT SetPalette(IDirectDrawPalette*) override { return DDERR_GENERIC; }
    HRESULT Unlock(LPVOID) override {
        presentIfPrimary();
        return DD_OK;
    }
    HRESULT UpdateOverlay(RECT*, IDirectDrawSurface4*, RECT*, DWORD, void*) override { return DDERR_GENERIC; }
    HRESULT UpdateOverlayDisplay(DWORD) override { return DDERR_GENERIC; }
    HRESULT UpdateOverlayZOrder(DWORD, IDirectDrawSurface4*) override { return DDERR_GENERIC; }
    HRESULT GetDDInterface(void**) override { return DDERR_GENERIC; }
    HRESULT PageLock(DWORD) override { return DD_OK; }
    HRESULT PageUnlock(DWORD) override { return DD_OK; }
    HRESULT SetSurfaceDesc(DDSURFACEDESC2*, DWORD) override { return DDERR_GENERIC; }
    HRESULT SetPrivateData(REFIID, void*, DWORD, DWORD) override { return DDERR_GENERIC; }
    HRESULT GetPrivateData(REFIID, void*, DWORD*) override { return DDERR_GENERIC; }
    HRESULT FreePrivateData(REFIID) override { return DDERR_GENERIC; }
    HRESULT GetUniquenessValue(DWORD* value) override {
        *value = 0;
        return DD_OK;
    }
    HRESULT ChangeUniquenessValue() override { return DD_OK; }

private:
    std::uint16_t& at(LONG x, LONG y) { return m_pixels[static_cast<std::size_t>(y) * (m_pitch / 2) + x]; }

    static Rect clampRect(const RECT* rect, int width, int height) {
        Rect r{0, 0, width, height};
        if (rect != nullptr) {
            r = Rect{rect->left, rect->top, rect->right, rect->bottom};
        }
        if (r.left < 0) r.left = 0;
        if (r.top < 0) r.top = 0;
        if (r.right > width) r.right = width;
        if (r.bottom > height) r.bottom = height;
        return r;
    }

    void stretch(Surface& src, const Rect& from, const Rect& to, bool keyed) {
        const LONG fromWidth = from.right - from.left;
        const LONG fromHeight = from.bottom - from.top;
        const LONG toWidth = to.right - to.left;
        const LONG toHeight = to.bottom - to.top;
        if (fromWidth <= 0 || fromHeight <= 0 || toWidth <= 0 || toHeight <= 0) {
            return;
        }
        const auto key = static_cast<std::uint16_t>(src.m_sourceKey.dwColorSpaceLowValue);
        std::vector<std::uint16_t> row(static_cast<std::size_t>(toWidth));
        for (LONG y = 0; y < toHeight; ++y) {
            const LONG dy = to.top + y;
            if (dy < 0 || dy >= m_height) {
                continue;
            }
            const LONG sy = from.top + y * fromHeight / toHeight;
            for (LONG x = 0; x < toWidth; ++x) {
                row[static_cast<std::size_t>(x)] = src.at(from.left + x * fromWidth / toWidth, sy);
            }
            for (LONG x = 0; x < toWidth; ++x) {
                const LONG dx = to.left + x;
                if (dx < 0 || dx >= m_width) {
                    continue;
                }
                const std::uint16_t pixel = row[static_cast<std::size_t>(x)];
                if (!keyed || pixel != key) {
                    at(dx, dy) = pixel;
                }
            }
        }
    }

    void describe(DDSURFACEDESC2& desc) {
        desc.dwSize = sizeof(DDSURFACEDESC2);
        desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
        desc.dwHeight = static_cast<DWORD>(m_height);
        desc.dwWidth = static_cast<DWORD>(m_width);
        desc.lPitch = m_pitch;
        desc.lpSurface = m_pixels.data();
        fillFormat(desc.ddpfPixelFormat);
        desc.ddsCaps = DDSCAPS2{};
        desc.ddsCaps.dwCaps = m_primary ? DDSCAPS_PRIMARYSURFACE : DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    }

    void presentIfPrimary() {
        if (m_primary) {
            swd3::platform::presentFrame(m_pixels.data(), m_width, m_height, m_pitch);
        }
    }

    DWORD m_refs = 1;
    int m_width;
    int m_height;
    int m_pitch;
    bool m_primary;
    std::vector<std::uint16_t> m_pixels;
    IDirectDrawClipper* m_clipper = nullptr;
    bool m_hasSourceKey = false;
    DDCOLORKEY m_sourceKey{};
};

class DirectDraw4 final : public IDirectDraw4 {
public:
    HRESULT QueryInterface(REFIID iid, void** object) override {
        if (sameGuid(iid, kIidDirectDraw4)) {
            AddRef();
            *object = static_cast<IDirectDraw4*>(this);
            return DD_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    DWORD AddRef() override { return ++m_refs; }
    DWORD Release() override {
        const DWORD refs = --m_refs;
        if (refs == 0) {
            delete this;
        }
        return refs;
    }
    HRESULT Compact() override { return DD_OK; }
    HRESULT CreateClipper(DWORD, IDirectDrawClipper** clipper, void*) override {
        *clipper = new Clipper();
        return DD_OK;
    }
    HRESULT CreatePalette(DWORD, void*, IDirectDrawPalette** palette, void*) override {
        *palette = nullptr;
        return DDERR_GENERIC;
    }
    HRESULT CreateSurface(DDSURFACEDESC2* desc, IDirectDrawSurface4** surface, void*) override {
        const bool primary = (desc->dwFlags & DDSD_CAPS) && (desc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE);
        int width = m_width;
        int height = m_height;
        if (!primary) {
            if (!(desc->dwFlags & DDSD_WIDTH) || !(desc->dwFlags & DDSD_HEIGHT) || desc->dwWidth == 0 ||
                desc->dwHeight == 0) {
                *surface = nullptr;
                return DDERR_INVALIDPARAMS;
            }
            width = static_cast<int>(desc->dwWidth);
            height = static_cast<int>(desc->dwHeight);
        }
        *surface = new Surface(width, height, primary);
        return DD_OK;
    }
    HRESULT DuplicateSurface(IDirectDrawSurface4*, IDirectDrawSurface4**) override { return DDERR_GENERIC; }
    HRESULT EnumDisplayModes(DWORD, DDSURFACEDESC2*, void*, void*) override { return DDERR_GENERIC; }
    HRESULT EnumSurfaces(DWORD, DDSURFACEDESC2*, void*, void*) override { return DDERR_GENERIC; }
    HRESULT FlipToGDISurface() override { return DD_OK; }
    HRESULT GetCaps(DDCAPS_DX5* driverCaps, DDCAPS_DX5* helCaps) override {
        if (driverCaps != nullptr) {
            std::memset(driverCaps->rest, 0, sizeof(driverCaps->rest));
        }
        if (helCaps != nullptr) {
            std::memset(helCaps->rest, 0, sizeof(helCaps->rest));
        }
        return DD_OK;
    }
    HRESULT GetDisplayMode(DDSURFACEDESC2* desc) override {
        desc->dwSize = sizeof(DDSURFACEDESC2);
        desc->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
        desc->dwWidth = static_cast<DWORD>(m_width);
        desc->dwHeight = static_cast<DWORD>(m_height);
        desc->lPitch = m_width * 2;
        fillFormat(desc->ddpfPixelFormat);
        return DD_OK;
    }
    HRESULT GetFourCCCodes(DWORD* count, DWORD*) override {
        *count = 0;
        return DD_OK;
    }
    HRESULT GetGDISurface(IDirectDrawSurface4**) override { return DDERR_NOTFOUND; }
    HRESULT GetMonitorFrequency(DWORD* frequency) override {
        *frequency = 60;
        return DD_OK;
    }
    HRESULT GetScanLine(DWORD* line) override {
        *line = 0;
        return DD_OK;
    }
    HRESULT GetVerticalBlankStatus(BOOL* inBlank) override {
        *inBlank = 1;
        return DD_OK;
    }
    HRESULT Initialize(GUID*) override { return DD_OK; }
    HRESULT RestoreDisplayMode() override { return DD_OK; }
    HRESULT SetCooperativeLevel(HWND, DWORD) override { return DD_OK; }
    HRESULT SetDisplayMode(DWORD width, DWORD height, DWORD bpp, DWORD, DWORD) override {
        if (bpp != 16) {
            return static_cast<HRESULT>(0x8876024Eu);
        }
        m_width = static_cast<int>(width);
        m_height = static_cast<int>(height);
        return DD_OK;
    }
    HRESULT WaitForVerticalBlank(DWORD, HANDLE) override { return DD_OK; }
    HRESULT GetAvailableVidMem(DDSCAPS2*, DWORD* total, DWORD* free) override {
        if (total != nullptr) *total = 0x2000000;
        if (free != nullptr) *free = 0x2000000;
        return DD_OK;
    }
    HRESULT GetSurfaceFromDC(HDC, IDirectDrawSurface4**) override { return DDERR_NOTFOUND; }
    HRESULT RestoreAllSurfaces() override { return DD_OK; }
    HRESULT TestCooperativeLevel() override { return DD_OK; }
    HRESULT GetDeviceIdentifier(void*, DWORD) override { return DDERR_GENERIC; }

private:
    DWORD m_refs = 1;
    int m_width = swd3::platform::kScreenWidth;
    int m_height = swd3::platform::kScreenHeight;
};

class DirectDraw final : public IDirectDraw {
public:
    HRESULT QueryInterface(REFIID iid, void** object) override {
        if (sameGuid(iid, kIidDirectDraw4)) {
            *object = static_cast<IDirectDraw4*>(new DirectDraw4());
            return DD_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }
    DWORD AddRef() override { return ++m_refs; }
    DWORD Release() override {
        const DWORD refs = --m_refs;
        if (refs == 0) {
            delete this;
        }
        return refs;
    }

private:
    DWORD m_refs = 1;
};

}  // namespace

HRESULT DirectDrawCreate(GUID*, IDirectDraw** directDraw, void*) {
    *directDraw = new DirectDraw();
    return DD_OK;
}

}  // namespace w32
