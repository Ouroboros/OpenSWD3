#include "draw/blit.h"

namespace swd3 {

using namespace blit;

// swd32.exe 0x00417950: raw picture blended at level G.drawAlpha - 1 (rewrites G.drawAlpha to the table
// offset), fill colour transparent
int BlitRaw_417950(int, int, int width, int, int) {
    const std::uint32_t key = static_cast<std::uint32_t>(G.fillColorPair) & 0xFFFF;
    G.drawAlpha = (G.drawAlpha - 1) << 2;
    const int visible = G.drawWidth;
    G.drawLineSkip = G.screenPitch - visible - visible;
    G.drawSourceSkip = (width - visible) * 2;
    const std::uint8_t* src = G.drawSourceLine;
    std::uint8_t* dest = G.drawDest;
    int rows = G.drawRows;
    do {
        int n = G.drawWidth;
        do {
            const std::uint32_t s = u16(src);
            if (s != key) {
                blendPixel(dest, s, G.drawAlpha >> 2);
            }
            --n;
            src += 2;
            dest += 2;
        } while (n != 0);
        --rows;
        src += G.drawSourceSkip;
        dest += G.drawLineSkip;
    } while (rows != 0);
    return 0;
}

// swd32.exe 0x00417EC0: destination blended towards the colour in the first word of the picture data,
// level 15 at the top decreasing every (height >> 4) + 1 rows
int BlitRaw_417EC0(int, int, int width, int height, int) {
    const std::uint32_t color = u16(G.drawSource);
    const int stepRows = height >> 4;
    int level = 0x3C;
    int rowInStep = 0;
    const int visible = G.drawWidth;
    G.drawLineSkip = G.screenPitch - visible - visible;
    G.drawSourceSkip = (width - visible) * 2;
    std::uint8_t* dest = G.drawDest;
    int rows = G.drawRows;
    do {
        int n = G.drawWidth;
        do {
            blendPixel(dest, color, level >> 2);
            --n;
            dest += 2;
        } while (n != 0);
        --rows;
        dest += G.drawLineSkip;
        ++rowInStep;
        if (stepRows < rowInStep) {
            rowInStep = 0;
            level -= 4;
        }
    } while (rows != 0);
    return 0;
}

}  // namespace swd3
