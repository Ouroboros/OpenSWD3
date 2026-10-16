#include "platform/glyph_rasterizer.h"

#include <windows.h>

#include <cstring>
#include <string>

#include "platform/surface.h"

namespace swd3
{

namespace
{

// GDI draws into a 16-bit RGB565 DIB section of the surface size; the result is copied into the surface.
class GdiGlyphRasterizer : public GlyphRasterizer
{
public:
    ~GdiGlyphRasterizer() override
    {
        this->ReleaseFont();
    }

    void RenderText(Surface& surface, std::string_view face, int size, std::string_view text, u32 color) override
    {
        this->SelectFont(face, size);

        struct
        {
            BITMAPINFOHEADER header;
            DWORD masks[3];
        } info = {};

        info.header.biSize = sizeof(BITMAPINFOHEADER);
        info.header.biWidth = surface.Width();
        info.header.biHeight = -surface.Height();
        info.header.biPlanes = 1;
        info.header.biBitCount = 16;
        info.header.biCompression = BI_BITFIELDS;
        info.masks[0] = Surface::kRedMask;
        info.masks[1] = Surface::kGreenMask;
        info.masks[2] = Surface::kBlueMask;
        HDC screen = ::GetDC(nullptr);
        HDC dc = ::CreateCompatibleDC(screen);
        ::ReleaseDC(nullptr, screen);
        void* bits = nullptr;
        HBITMAP bitmap =
            ::CreateDIBSection(dc, reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, &bits, nullptr, 0);

        if (bitmap == nullptr)
        {
            ::DeleteDC(dc);
            return;
        }

        const std::size_t pixels = static_cast<std::size_t>(surface.Width()) * surface.Height();
        std::memcpy(bits, surface.Pixels(), pixels * 2);
        HGDIOBJ previousBitmap = ::SelectObject(dc, bitmap);
        HGDIOBJ previousFont = ::SelectObject(dc, this->font);
        ::SetBkColor(dc, 0);
        ::SetBkMode(dc, OPAQUE);
        ::SetTextColor(dc, color);
        ::TextOutA(dc, 0, 0, "  ", 2);
        ::TextOutA(dc, 0, 0, text.data(), static_cast<int>(text.size()));
        ::GdiFlush();
        std::memcpy(surface.Pixels(), bits, pixels * 2);
        ::SelectObject(dc, previousFont);
        ::SelectObject(dc, previousBitmap);
        ::DeleteObject(bitmap);
        ::DeleteDC(dc);
    }

private:
    void SelectFont(std::string_view face, int size)
    {
        if (this->font != nullptr && face == this->face && size == this->size)
        {
            return;
        }

        this->ReleaseFont();
        LOGFONTA logFont = {};
        logFont.lfHeight = -size;
        logFont.lfWeight = FW_NORMAL;
        logFont.lfCharSet = CHINESEBIG5_CHARSET;
        logFont.lfQuality = NONANTIALIASED_QUALITY;
        const std::size_t length = face.size() < LF_FACESIZE - 1 ? face.size() : LF_FACESIZE - 1;
        std::memcpy(logFont.lfFaceName, face.data(), length);
        this->font = ::CreateFontIndirectA(&logFont);
        this->face.assign(face);
        this->size = size;
    }

    void ReleaseFont()
    {
        if (this->font != nullptr)
        {
            ::DeleteObject(this->font);
            this->font = nullptr;
        }
    }

    HFONT font = nullptr;
    std::string face;
    int size = 0;
};

}  // namespace

std::unique_ptr<GlyphRasterizer> CreateGlyphRasterizer()
{
    return std::make_unique<GdiGlyphRasterizer>();
}

}  // namespace swd3
