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

// The game's text renderer (three global objects at 0x004AB998, 0x004C9A28 and 0x004A9ED0): glyphs are drawn with
// the platform rasterizer into a 64x64 surface, captured and kept as 1-bit bitmaps sorted by their (Big5) character
// code, at most 2000; strings are drawn from those bitmaps into a 16-bit target buffer.
class TextFont
{
public:
    // Styles of TextFont::DrawText (tested in this order) and the colour gradient bits.
    enum Style : int
    {
        Plain = 0x1,
        Shadow = 0x2,
        BoldShadow = 0x4,
        Outline = 0x8,
        BoldOutline = 0x10,
        GradientUp = 0x80,     // colour + 1 per glyph row
        GradientDown = 0x100,  // colour - 1 per glyph row
    };

    static constexpr int MaxGlyphs = 2000;
    static constexpr int DefaultGlyphSize = 16;
    static constexpr Pixel NoBackground = 0xFFFE;
    static constexpr std::u8string_view DefaultFace = u8"細明體";  // swd32.exe 0x0049FB74 "MingLiU" in Big5

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
    void SetFont(std::u8string_view face, int size);
    void SetTarget(int width, int height);
    void SetClip(int x, int y, int width, int height);
    void SetAdvance(int advance);
    void SetBackground(Pixel color);
    void SetShadow(Pixel color);

    // The rebuild's smooth battle (deviations.md): what a draw reads besides its arguments and the glyphs, kept with a
    // recorded draw to draw it again alike.
    struct DrawState
    {
        int targetWidth = 0;
        int targetHeight = 0;
        int advance = 0;
        Pixel shadow = 0;
        Pixel background = 0;
        int clipX = 0;
        int clipY = 0;
        int clipWidth = 0;
        int clipHeight = 0;
    };

    DrawState SaveDrawState() const;
    void RestoreDrawState(const DrawState& state);
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
    // The advance of a double-byte character (a single byte takes half).
    int Advance() const;
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
    std::vector<i32> targetRows;         // pixel offset of each target row
    std::vector<Pixel> glyphPixels;      // the captured glyph (glyphWidth x glyphHeight)
    int glyphRowBytes = 0;               // bytes per row of a cached 1-bit glyph
    std::array<u16, MaxGlyphs> codes{};  // sorted character codes of the cache
    std::vector<u8> glyphs;              // MaxGlyphs 1-bit glyph bitmaps
    int glyphCount = 0;
    std::array<char, 2> character = {};  // the current character (one or two bytes)
    std::string fontFace;
    int fontSize = 0;
    int glyphWidth = DefaultGlyphSize;
    int glyphHeight = DefaultGlyphSize;
    int surfacePitch = 0;  // pixels per row of the glyph surface
    int surfaceHeight = 0;
    int advance = 0;  // horizontal advance of a double-byte character
    Pixel shadow = 0;
    Pixel background = NoBackground;
    int clipX = 0;
    int clipY = 0;
    int clipWidth = 0;
    int clipHeight = 0;
};

}  // namespace swd3
