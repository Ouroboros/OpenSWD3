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
