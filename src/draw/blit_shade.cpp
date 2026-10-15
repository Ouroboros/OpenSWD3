#include "draw/blit_tint.h"
#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

// swd32.exe 0x0041B280: tints the destination under the literal pixels of the picture (source pixels are
// skipped, both run kinds are transparent); nothing is drawn without a tint. The wave row stride is 0x528.
int BlitRle_41B280(int, int, int, int, int) {
    if (!tinted()) {
        return 0;
    }
    const Tint t = makeTint();
    walk3Forward(
        Walk3Options{0x528, true, false, false},
        [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            src += 2 * n;
            do {
                put16(dest, tintStep(t, u16(dest)));
                dest += 2;
            } while (--n != 0);
        },
        [](std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; },
        [](std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; });
    return 0;
}

// swd32.exe 0x0041B620: mirrored 0x0041B280; leaves the mirrored clip start in G.drawSkipLeft
int BlitRle_41B620(int, int, int width, int, int) {
    if (!tinted()) {
        return 0;
    }
    const Tint t = makeTint();
    walk3Mirror(
        Walk3Options{0x84, true, true, false}, width,
        [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            src += 2 * n;
            do {
                dest -= 2;
                put16(dest, tintStep(t, u16(dest)));
            } while (--n != 0);
        },
        [](std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; },
        [](std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; });
    return 0;
}

}  // namespace swd3
