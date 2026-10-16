#include "platform/gdi_surface.h"

#include "swd3/ddraw.h"

// No GDI outside Windows: surfaces have no device context, so the game's GDI text and colour matching paths
// take their failure branches.
namespace swd3::platform {

void* beginSurfaceDC(const std::uint16_t*, int, int, int) { return nullptr; }

void endSurfaceDC(void*, std::uint16_t*, int, int, int) {}

}  // namespace swd3::platform

namespace w32 {

COLORREF GetPixel(HDC, int, int) { return 0xFFFFFFFFu; }

COLORREF SetPixel(HDC, int, int, COLORREF) { return 0xFFFFFFFFu; }

}  // namespace w32

#include "swd3/gdi.h"

namespace w32 {
namespace {

int g_fontToken;

}  // namespace

HFONT CreateFontIndirectA(const LOGFONTA*) { return &g_fontToken; }

BOOL DeleteObject(HGDIOBJ object) { return object != nullptr; }

HGDIOBJ SelectObject(HDC, HGDIOBJ) { return nullptr; }

COLORREF SetBkColor(HDC, COLORREF) { return 0xFFFFFFFFu; }

int SetBkMode(HDC, int) { return 0; }

COLORREF SetTextColor(HDC, COLORREF) { return 0xFFFFFFFFu; }

BOOL TextOutA(HDC, int, int, LPCSTR, int) { return 0; }

}  // namespace w32
