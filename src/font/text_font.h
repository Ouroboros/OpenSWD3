#pragma once

#include <cstdint>

#include "swd3/gdi.h"

namespace swd3 {

class Graphics;

// Text styles of TextFont::DrawText (bit tested in this order).
constexpr int kTextPlain = 1;
constexpr int kTextShadow = 2;
constexpr int kTextBoldShadow = 4;
constexpr int kTextOutline = 8;
constexpr int kTextBoldOutline = 0x10;
// Bits 7 / 8 of the style make the colour step +1 / -1 per glyph row.
constexpr int kTextGradientUp = 0x80;
constexpr int kTextGradientDown = 0x100;
// Background colour value meaning "no background".
constexpr std::uint16_t kTextNoBackground = 0xFFFE;

// GDI glyph cache and 16-bit text renderer (global objects at 0x004AB998, 0x004C9A28 and 0x004A9ED0, size
// 0xFF8). Glyphs are drawn with GDI into a 64x64 surface, captured, and cached as 1-bit bitmaps keyed by
// their (DBCS) character code, at most 2000 of them.
class TextFont {
public:
    TextFont();
    ~TextFont();
    int Init(Graphics* graphics);
    void Shutdown();
    void CreateGlyphSurface(int width, int height);
    void ReleaseGlyphSurface();
    w32::HRESULT RenderString(w32::HGDIOBJ font, int x, int y, const char* text, w32::COLORREF color);
    void SetGlyphSize(int width, int height);
    void FreeGlyphPixels();
    void AllocGlyphCache();
    void FreeGlyphCache();
    void SetFont(const char* faceName, int size);
    void SetTarget(int width, int height);
    void SetClip(int x, int y, int width, int height);
    void SetAdvance(int advance);
    void SetBackground(std::uint16_t color);
    void SetShadow(std::uint16_t color);
    void DrawGlyph(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style);
    void DrawGlyphBold(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style);
    void DrawGlyphShadow(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style);
    void DrawGlyphBoldShadow(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style);
    void DrawGlyphOutline(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style);
    void DrawGlyphBoldOutline(std::uint16_t* target, int x, int y, int glyph, std::uint16_t color, int style);
    w32::HRESULT CaptureGlyphSurface();
    void RasterizeGlyph(const char* text, std::uint8_t* bitmap);
    int FindGlyph(std::uint16_t code);
    int InsertGlyph(std::uint16_t code);
    void DrawText(std::uint16_t* target, int x, int y, const char* text, std::uint16_t color, int style);
    void FillRect(std::uint16_t* target, int x, int y, int width, int height, std::uint16_t color);

    Graphics* m_graphics;                       // +0x000
    w32::IDirectDrawSurface4* m_glyphSurface;   // +0x004 64x64 GDI drawing surface
    std::int32_t m_targetWidth;                 // +0x008 pixels per row of the target buffers
    std::int32_t m_targetHeight;                // +0x00C
    std::int32_t m_targetBytes;                 // +0x010
    std::int32_t* m_targetRows;                 // +0x014 pixel offset of each target row
    std::uint16_t* m_glyphPixels;               // +0x018 captured glyph (m_glyphWidth x m_glyphHeight)
    std::int32_t m_glyphRowBytes;               // +0x01C bytes per row of a cached 1-bit glyph
    std::uint16_t m_codes[2000];                // +0x020 sorted character codes of the cache
    std::uint8_t* m_glyphs;                     // +0xFC0 2000 1-bit glyph bitmaps
    std::int32_t m_glyphCount;                  // +0xFC4
    char m_character[4];                        // +0xFC8 current character as a string (1 or 2 bytes)
    w32::HFONT m_font;                          // +0xFCC
    std::int32_t m_glyphWidth;                  // +0xFD0
    std::int32_t m_glyphHeight;                 // +0xFD4
    std::int32_t m_surfacePitch;                // +0xFD8 pixels per row of m_glyphSurface
    std::int32_t m_surfaceHeight;               // +0xFDC
    std::int32_t m_advance;                     // +0xFE0 horizontal advance of a double-byte character
    std::uint16_t m_shadow;                     // +0xFE4
    std::uint16_t m_background;                 // +0xFE6 kTextNoBackground: none
    std::int32_t m_clipX;                       // +0xFE8
    std::int32_t m_clipY;                       // +0xFEC
    std::int32_t m_clipWidth;                   // +0xFF0
    std::int32_t m_clipHeight;                  // +0xFF4
};

static_assert(sizeof(TextFont) == 0xFF8);

}  // namespace swd3
