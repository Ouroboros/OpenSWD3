#include <array>
#include <cstdio>
#include <string>

#include "core/types.h"
#include "core/words.h"
#include "platform/glyph_rasterizer.h"
#include "platform/surface.h"
#include "test.h"
#include "text/big5.h"

using swd3::u8;

namespace
{

constexpr std::array<int, 3> kSizes = {12, 16, 20};
constexpr int kMissingSize = 14;
constexpr swd3::u16 kEtenLetter = 0xB1C7;  // Big5 C7 B1: an ETen extension letter drawn from the outline
constexpr int kSurfaceSide = 64;
constexpr int kSize = 16;
constexpr swd3::u32 kWhiteRef = 0xFFFFFF;

}  // namespace

TEST_CASE("glyph library: MingLiU cells of the three sizes")
{
    swd3::GlyphLibrary library;

    if (!library.Load(swd3::GlyphLibraryPath()))
    {
        std::printf("  (asset/font.bin not found, skipped)\n");
        return;
    }

    const std::string one = swd3::Big5(u8"一");
    const swd3::u16 oneCode = swd3::MakeBytes(static_cast<u8>(one[1]), static_cast<u8>(one[0]));  // lead byte low

    for (const int size : kSizes)
    {
        const u8* letter = library.Find(size, 'A');
        const u8* han = library.Find(size, oneCode);
        const u8* cyrillic = library.Find(size, kEtenLetter);
        CHECK(letter != nullptr);
        CHECK(han != nullptr);
        CHECK(cyrillic != nullptr);
    }

    CHECK(library.Find(kMissingSize, 'A') == nullptr);
    CHECK(library.Find(kSize, 1) == nullptr);
    swd3::LibraryGlyphRasterizer rasterizer(library);
    swd3::Surface surface(kSurfaceSide, kSurfaceSide);
    surface.Fill(0);
    rasterizer.RenderText(surface, "", kSize, one, kWhiteRef);
    int set = 0;
    int outside = 0;

    for (int y = 0; y < kSurfaceSide; ++y)
    {
        for (int x = 0; x < kSurfaceSide; ++x)
        {
            if (surface.Row(y)[x] != 0)
            {
                ++(x < kSize && y < kSize ? set : outside);
                CHECK_EQ(surface.Row(y)[x], swd3::DevicePixel(kWhiteRef));
            }
        }
    }

    CHECK(set > 0);
    CHECK_EQ(outside, 0);
}
