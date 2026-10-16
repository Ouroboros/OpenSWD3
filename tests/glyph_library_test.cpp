#include <cstdio>

#include "core/types.h"
#include "platform/glyph_rasterizer.h"
#include "platform/surface.h"
#include "test.h"
#include "text/big5.h"

using swd3::u8;

TEST_CASE("glyph library: MingLiU cells of the three sizes")
{
    swd3::GlyphLibrary library;

    if (!library.Load(swd3::GlyphLibraryPath()))
    {
        std::printf("  (swd3font.bin not found, skipped)\n");
        return;
    }

    for (int size : {12, 16, 20})
    {
        const u8* letter = library.Find(size, 'A');
        const u8* han = library.Find(size, 0x40A4);       // Big5 A4 40: the character "one"
        const u8* cyrillic = library.Find(size, 0xB1C7);  // Big5 C7 B1: an ETen extension letter drawn from the outline
        CHECK(letter != nullptr);
        CHECK(han != nullptr);
        CHECK(cyrillic != nullptr);
    }

    CHECK(library.Find(14, 'A') == nullptr);
    CHECK(library.Find(16, 0x0001) == nullptr);
    swd3::LibraryGlyphRasterizer rasterizer(library);
    swd3::Surface surface(64, 64);
    surface.Fill(0);
    rasterizer.RenderText(surface, "", 16, swd3::Big5(u8"一"), 0xFFFFFF);
    int set = 0;
    int outside = 0;

    for (int y = 0; y < 64; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            if (surface.Row(y)[x] != 0)
            {
                ++(x < 16 && y < 16 ? set : outside);
                CHECK_EQ(surface.Row(y)[x], 0xFFFFu);
            }
        }
    }

    CHECK(set > 0);
    CHECK_EQ(outside, 0);
}
