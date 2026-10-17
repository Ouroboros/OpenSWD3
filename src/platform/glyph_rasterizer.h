#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3
{

class Surface;

// Draws Big5 text the way the game draws it with GDI: TextOutA at (0, 0) with the Big5 font "細明體" (MingLiU) of
// `size` pixels, the given colour on black.
class GlyphRasterizer
{
public:
    virtual ~GlyphRasterizer() = default;
    virtual void RenderText(Surface& surface, std::string_view face, int size, std::string_view text, u32 color) = 0;
};

// The game's glyph library (swd3font.bin, made by scripts/make_font_library.py from the embedded 1-bit bitmaps of
// MingLiU): for each size the original uses (12, 16, 20 pixels) the cell GDI draws for every Big5 code and ASCII byte.
// It replaces the system font, so text looks the same on every system.
class GlyphLibrary
{
public:
    bool Load(const std::u8string& path);
    bool Loaded() const;
    // The cell (size rows of ceil(size / 8) bytes, the leftmost pixel in bit 7) of a character code (first byte in the
    // low byte), or nullptr.
    const u8* Find(int size, u16 code) const;

private:
    struct Strike
    {
        int size = 0;
        int stride = 0;
        std::vector<u16> codes;
        std::vector<u8> cells;
    };

    std::map<int, Strike> strikes;
};

class LibraryGlyphRasterizer : public GlyphRasterizer
{
public:
    explicit LibraryGlyphRasterizer(const GlyphLibrary& library);

    void RenderText(Surface& surface, std::string_view face, int size, std::string_view text, u32 color) override;

private:
    const GlyphLibrary& library;
};

// The location of the glyph library: next to the executable.
std::u8string GlyphLibraryPath();

}  // namespace swd3
