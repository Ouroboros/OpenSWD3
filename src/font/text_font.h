#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/types.h"
#include "text/big5.h"

namespace swd3
{

class Graphics;
class GlyphRasterizer;
class Surface;

// Styles of TextFont::DrawText (tested in this order) and the colour gradient bits.
namespace text_style
{

constexpr int kPlain = 0x1;
constexpr int kShadow = 0x2;
constexpr int kBoldShadow = 0x4;
constexpr int kOutline = 0x8;
constexpr int kBoldOutline = 0x10;
constexpr int kGradientUp = 0x80;     // colour + 1 per glyph row
constexpr int kGradientDown = 0x100;  // colour - 1 per glyph row

}  // namespace text_style

// The game's text renderer (three global objects at 0x004AB998, 0x004C9A28 and 0x004A9ED0): glyphs are drawn with
// the platform rasterizer into a 64x64 surface, captured and kept as 1-bit bitmaps sorted by their (Big5) character
// code, at most 2000; strings are drawn from those bitmaps into a 16-bit target buffer.
class TextFont
{
public:
    static constexpr int kMaxGlyphs = 2000;
    static constexpr Pixel kNoBackground = 0xFFFE;
    static constexpr std::u8string_view kDefaultFace = u8"細明體";  // swd32.exe 0x0049FB74 "MingLiU" in Big5

    explicit TextFont(GlyphRasterizer& rasterizer);
    ~TextFont();

    int Init(Graphics& graphics);
    void Shutdown();
    void CreateGlyphSurface(int width, int height);
    void ReleaseGlyphSurface();
    void RenderString(int x, int y, std::string_view text, u32 color);
    void SetGlyphSize(int width, int height);
    void FreeGlyphPixels();
    void AllocGlyphCache();
    void FreeGlyphCache();
    void SetFont(std::string_view face, int size);
    void SetTarget(int width, int height);
    void SetClip(int x, int y, int width, int height);
    void SetAdvance(int advance);
    void SetBackground(Pixel color);
    void SetShadow(Pixel color);
    void DrawGlyph(Pixel* target, int x, int y, int glyph, Pixel color, int style);
    void DrawGlyphBold(Pixel* target, int x, int y, int glyph, Pixel color, int style);
    void DrawGlyphShadow(Pixel* target, int x, int y, int glyph, Pixel color, int style);
    void DrawGlyphBoldShadow(Pixel* target, int x, int y, int glyph, Pixel color, int style);
    void DrawGlyphOutline(Pixel* target, int x, int y, int glyph, Pixel color, int style);
    void DrawGlyphBoldOutline(Pixel* target, int x, int y, int glyph, Pixel color, int style);
    void CaptureGlyphSurface();
    void RasterizeGlyph(std::string_view text, u8* bitmap);
    int FindGlyph(u16 code) const;
    int InsertGlyph(u16 code);
    void DrawText(Pixel* target, int x, int y, std::string_view text, Pixel color, int style);
    void DrawText(Pixel* target, int x, int y, std::u8string_view text, Pixel color, int style);
    void FillRect(Pixel* target, int x, int y, int width, int height, Pixel color);

    int GlyphCount() const;
    int GlyphWidth() const;
    int GlyphHeight() const;
    u8* GlyphBits(int glyph);
    u16 GlyphCode(int glyph) const;

private:
    const u8* Bits(int glyph) const;
    std::ptrdiff_t RowOffset(int row) const;

    GlyphRasterizer& rasterizer;
    Graphics* graphics = nullptr;
    std::unique_ptr<Surface> glyphSurface;  // 64x64 GDI drawing surface
    int targetWidth = 0;                    // pixels per row of the target buffers
    int targetHeight = 0;
    int targetBytes = 0;
    std::vector<i32> targetRows;          // pixel offset of each target row
    std::vector<Pixel> glyphPixels;       // the captured glyph (glyphWidth x glyphHeight)
    int glyphRowBytes = 0;                // bytes per row of a cached 1-bit glyph
    std::array<u16, kMaxGlyphs> codes{};  // sorted character codes of the cache
    std::vector<u8> glyphs;               // kMaxGlyphs 1-bit glyph bitmaps
    int glyphCount = 0;
    std::array<char, 2> character = {};  // the current character (one or two bytes)
    std::string fontFace;
    int fontSize = 0;
    int glyphWidth = 16;
    int glyphHeight = 16;
    int surfacePitch = 0;  // pixels per row of the glyph surface
    int surfaceHeight = 0;
    int advance = 0;  // horizontal advance of a double-byte character
    Pixel shadow = 0;
    Pixel background = kNoBackground;
    int clipX = 0;
    int clipY = 0;
    int clipWidth = 0;
    int clipHeight = 0;
};

}  // namespace swd3
