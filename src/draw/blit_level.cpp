#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

namespace {

void skipForward(std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; }
void skipBackward(std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; }

}  // namespace

// swd32.exe 0x0041B9F0: RLE picture faded in from the top: the blend level starts at 15 and drops by one every
// (height + 16) >> 4 lines (skipped top lines included); both run kinds are transparent. The original indexes
// a jump table with the level, so a level below 0 (more lines than the height allows) would jump through
// unrelated stack data; the game's pictures never reach it.
int BlitRle_41B9F0(int, int, int, int height, int) {
    const std::int32_t period = (height + 16) >> 4;
    std::int32_t countdown = period;
    std::int32_t level = 0x3C;
    walk3ForwardHooked(
        Walk3Options{0x84, true, false, false},
        [&] {
            if (--countdown == 0) {
                countdown = period;
                level -= 4;
            }
        },
        [&level](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                blendPixel(dest, u16(src), level >> 2);
                dest += 2;
                src += 2;
            } while (--n != 0);
        },
        skipForward, skipForward);
    return 0;
}

// swd32.exe 0x0041D340: RLE picture blended at level G.drawAlpha (0..15), which is left multiplied by 4;
// lines advance by G.drawPitch
int BlitRle_41D340(int, int, int, int, int) {
    G.drawAlpha <<= 2;
    walk3Forward(
        Walk3Options{0x84, true, false, true},
        [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                blendPixel(dest, u16(src), G.drawAlpha >> 2);
                dest += 2;
                src += 2;
            } while (--n != 0);
        },
        skipForward, skipForward);
    return 0;
}

// swd32.exe 0x0041E5C0: mirrored 0x0041D340
int BlitRle_41E5C0(int, int, int width, int, int) {
    G.drawAlpha <<= 2;
    walk3Mirror(
        Walk3Options{0x84, true, false, true}, width,
        [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            do {
                dest -= 2;
                blendPixel(dest, u16(src), G.drawAlpha >> 2);
                src += 2;
            } while (--n != 0);
        },
        skipBackward, skipBackward);
    return 0;
}

}  // namespace swd3
