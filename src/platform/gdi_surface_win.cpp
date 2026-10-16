#include "platform/gdi_surface.h"
#include "swd3/ddraw.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>
#include <map>

namespace swd3::platform {
namespace {

struct SurfaceDC {
    HBITMAP bitmap;
    HGDIOBJ previous;
    void* bits;
};

std::map<::HDC, SurfaceDC> g_surfaceDCs;

void copyRows(std::uint8_t* dest, int destPitch, const std::uint8_t* src, int srcPitch, int width, int height) {
    for (int y = 0; y < height; ++y) {
        std::memcpy(dest + y * destPitch, src + y * srcPitch, static_cast<std::size_t>(width) * 2);
    }
}

}  // namespace

void* beginSurfaceDC(const std::uint16_t* pixels, int width, int height, int pitchBytes) {
    struct {
        BITMAPINFOHEADER header;
        ::DWORD masks[3];
    } info{};
    info.header.biSize = sizeof(BITMAPINFOHEADER);
    info.header.biWidth = width;
    info.header.biHeight = -height;
    info.header.biPlanes = 1;
    info.header.biBitCount = 16;
    info.header.biCompression = BI_BITFIELDS;
    info.masks[0] = 0xF800;
    info.masks[1] = 0x07E0;
    info.masks[2] = 0x001F;
    ::HDC dc = CreateCompatibleDC(nullptr);
    if (dc == nullptr) {
        return nullptr;
    }
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap == nullptr) {
        DeleteDC(dc);
        return nullptr;
    }
    const int dibPitch = ((width * 2) + 3) & ~3;
    copyRows(static_cast<std::uint8_t*>(bits), dibPitch, reinterpret_cast<const std::uint8_t*>(pixels), pitchBytes,
             width, height);
    g_surfaceDCs[dc] = SurfaceDC{bitmap, SelectObject(dc, bitmap), bits};
    return dc;
}

void endSurfaceDC(void* handle, std::uint16_t* pixels, int width, int height, int pitchBytes) {
    ::HDC dc = static_cast<::HDC>(handle);
    auto it = g_surfaceDCs.find(dc);
    if (it == g_surfaceDCs.end()) {
        return;
    }
    GdiFlush();
    const int dibPitch = ((width * 2) + 3) & ~3;
    copyRows(reinterpret_cast<std::uint8_t*>(pixels), pitchBytes, static_cast<const std::uint8_t*>(it->second.bits),
             dibPitch, width, height);
    SelectObject(dc, it->second.previous);
    DeleteObject(it->second.bitmap);
    DeleteDC(dc);
    g_surfaceDCs.erase(it);
}

}  // namespace swd3::platform

namespace w32 {

COLORREF GetPixel(HDC dc, int x, int y) { return ::GetPixel(static_cast<::HDC>(dc), x, y); }

COLORREF SetPixel(HDC dc, int x, int y, COLORREF color) { return ::SetPixel(static_cast<::HDC>(dc), x, y, color); }

}  // namespace w32
