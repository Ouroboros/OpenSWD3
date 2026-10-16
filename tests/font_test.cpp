#include <cstring>
#include <new>

#include "crt/crt.h"
#include "font/text_font.h"
#include "test.h"

using namespace swd3;

namespace {

// An 8x2 glyph cache with hand made bitmaps on a 16x8 target (no GDI involved).
// Constructed on zeroed storage like the global objects of the game.
struct Fixture {
    alignas(TextFont) std::uint8_t storage[sizeof(TextFont)] = {};
    TextFont& font = *new (storage) TextFont();
    std::uint16_t target[16 * 8] = {};

    ~Fixture() { font.~TextFont(); }

    Fixture() {
        font.SetTarget(16, 8);
        font.m_glyphWidth = 8;
        font.m_glyphHeight = 2;
        font.m_glyphRowBytes = 1;
        font.m_glyphs = static_cast<std::uint8_t*>(crt::malloc(2000 * 2));
        std::memset(font.m_glyphs, 0, 2000 * 2);
        font.m_glyphCount = 1;
        font.m_glyphs[0] = 0x81;
        font.m_glyphs[1] = 0x80;
    }

    std::uint16_t at(int x, int y) const { return target[y * 16 + x]; }
};

}  // namespace

TEST_CASE("font.plain glyph never draws the last clip column, gradient steps per row") {
    Fixture f;
    f.font.DrawGlyph(f.target, 8, 3, 0, 0x100, kTextPlain | kTextGradientDown);
    CHECK_EQ(f.at(8, 3), 0x100);
    CHECK_EQ(f.at(15, 3), 0);
    CHECK_EQ(f.at(8, 4), 0xFF);
    f.font.DrawGlyph(f.target, 0, 0, 0, 0x100, kTextPlain);
    CHECK_EQ(f.at(7, 0), 0x100);
    f.font.SetClip(0, 0, 8, 8);
    f.target[7] = 0;
    f.font.DrawGlyph(f.target, 0, 0, 0, 0x200, kTextPlain);
    CHECK_EQ(f.at(0, 0), 0x200);
    CHECK_EQ(f.at(7, 0), 0);
}

TEST_CASE("font.shadow, bold shadow and outline offsets") {
    Fixture f;
    f.font.SetShadow(0x7);
    f.font.m_glyphs[0] = 0x40;
    f.font.m_glyphs[1] = 0;
    f.font.DrawGlyphShadow(f.target, 4, 2, 0, 0x55, kTextShadow);
    CHECK_EQ(f.at(5, 2), 0x55);
    CHECK_EQ(f.at(5, 3), 0x7);
    CHECK_EQ(f.at(6, 3), 0x7);
    CHECK_EQ(f.at(4, 3), 0);

    Fixture g;
    g.font.SetShadow(0x7);
    g.font.m_glyphs[0] = 0x40;
    g.font.m_glyphs[1] = 0;
    g.font.DrawGlyphBoldShadow(g.target, 4, 2, 0, 0x55, kTextBoldShadow);
    CHECK_EQ(g.at(5, 2), 0x55);
    CHECK_EQ(g.at(6, 2), 0x55);
    CHECK_EQ(g.at(6, 3), 0x7);
    CHECK_EQ(g.at(7, 3), 0x7);
    CHECK_EQ(g.at(5, 3), 0);

    Fixture h;
    h.font.SetShadow(0x7);
    h.font.m_glyphs[0] = 0x40;
    h.font.m_glyphs[1] = 0;
    h.font.DrawGlyphOutline(h.target, 4, 2, 0, 0x55, kTextOutline);
    CHECK_EQ(h.at(5, 1), 0x7);
    CHECK_EQ(h.at(4, 2), 0x7);
    CHECK_EQ(h.at(5, 2), 0x55);
    CHECK_EQ(h.at(4, 3), 0x7);
    CHECK_EQ(h.at(5, 3), 0x7);
    CHECK_EQ(h.at(6, 3), 0x7);
}

TEST_CASE("font.glyph cache keeps codes sorted and finds them") {
    Fixture f;
    f.font.m_glyphCount = 0;
    CHECK_EQ(f.font.InsertGlyph(0x5000), 0);
    f.font.m_glyphCount = 1;
    f.font.m_glyphs[0] = 0xAA;
    CHECK_EQ(f.font.InsertGlyph(0x4000), 0);
    f.font.m_glyphCount = 2;
    CHECK_EQ(f.font.m_codes[1], 0x5000);
    CHECK_EQ(f.font.m_glyphs[2], 0xAA);
    CHECK_EQ(f.font.m_glyphs[0], 0);
    CHECK_EQ(f.font.InsertGlyph(0x6000), 2);
    f.font.m_glyphCount = 3;
    CHECK_EQ(f.font.FindGlyph(0x5000), 1);
    CHECK_EQ(f.font.FindGlyph(0x6000), 2);
    CHECK_EQ(f.font.FindGlyph(0x4500), -1);
}

TEST_CASE("font.background fill clamps right and bottom to the target") {
    Fixture f;
    f.font.FillRect(f.target, 14, 6, 5, 5, 0x9);
    CHECK_EQ(f.at(14, 6), 0x9);
    CHECK_EQ(f.at(15, 7), 0x9);
    CHECK_EQ(f.at(13, 6), 0);
}
