#include "draw/blit_tint.h"
#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

namespace {

// Copy of n literal pixels as in the original: single pixels for n <= 4, otherwise an alignment pixel,
// pairs and a tail pixel (the result is the same as a pixel-by-pixel copy).
void copyForward(std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
    do {
        put16(dest, u16(src));
        dest += 2;
        src += 2;
    } while (--n != 0);
}

void copyBackward(std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
    do {
        dest -= 2;
        put16(dest, u16(src));
        src += 2;
    } while (--n != 0);
}

// Fill of n pixels with a colour pair: single writes use the low half; for n > 4 the pairs write the whole
// dword, so every second pixel of the pair loop gets the high half.
void fillForward(std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n, std::uint32_t color) {
    if (n <= 4) {
        do {
            dest += 2;
            src += 2;
            put16(dest - 2, color);
        } while (--n != 0);
        return;
    }
    if (reinterpret_cast<std::uintptr_t>(dest) & 2) {
        dest += 2;
        src += 2;
        put16(dest - 2, color);
        --n;
    }
    do {
        dest += 4;
        src += 4;
        n -= 2;
        put32(dest - 4, color);
    } while (n >= 2);
    if (n & 1) {
        dest += 2;
        src += 2;
        put16(dest - 2, color);
    }
}

void fillBackward(std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n, std::uint32_t color) {
    if (n <= 4) {
        do {
            dest -= 2;
            src += 2;
            put16(dest, color);
        } while (--n != 0);
        return;
    }
    if (reinterpret_cast<std::uintptr_t>(dest) & 2) {
        dest -= 2;
        src += 2;
        put16(dest, color);
        if (--n == 0) {
            return;
        }
    }
    do {
        const std::uint32_t swapped = (color << 16) | (color >> 16);
        dest -= 4;
        src += 4;
        n -= 2;
        put32(dest, swapped);
    } while (n >= 2);
    if (n & 1) {
        dest -= 2;
        src += 2;
        put16(dest, color);
    }
}

std::uint32_t grayPixel(std::uint32_t d) {
    const std::uint32_t r = (d & static_cast<std::uint32_t>(G.redMaskCopy)) >> G.shiftRed;
    const std::uint32_t g = (d & static_cast<std::uint32_t>(G.greenMaskCopy)) >> G.shiftGreen;
    const std::uint32_t b = (d & static_cast<std::uint32_t>(G.blueMaskCopy)) >> G.shiftBlue;
    const std::uint32_t avg = (r + g + b) >> 2;
    return (avg << G.shiftRed) + (avg << G.shiftGreen) + (avg << G.shiftBlue);
}

void skipForward(std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; }
void skipBackward(std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; }

std::uint32_t colorArg(int arg) {
    std::uint32_t color;
    std::memcpy(&color, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(arg))), 4);
    return color;
}

}  // namespace

// swd32.exe 0x0041CCF0: copy, then the first and the last pixel of each literal run are set to the low half
// of G.greenMaskPair
int BlitRle_41CCF0(int, int, int, int, int) {
    walk3Forward(
        Walk3Options{0x84, true, false, false},
        [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            std::uint8_t* first = dest;
            copyForward(dest, src, n);
            const auto mark = static_cast<std::uint32_t>(G.greenMaskPair);
            put16(first, mark);
            put16(dest - 2, mark);
        },
        skipForward, skipForward);
    return 0;
}

// swd32.exe 0x0041D010: mirrored 0x0041CCF0; the end mark lands one pixel beyond the run, the wave phase is
// not advanced
int BlitRle_41D010(int, int, int width, int, int) {
    walk3Mirror(
        Walk3Options{0x84, false, false, false}, width,
        [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            std::uint8_t* first = dest - 2;
            copyBackward(dest, src, n);
            const auto mark = static_cast<std::uint32_t>(G.greenMaskPair);
            put16(first, mark);
            put16(dest - 2, mark);
        },
        skipBackward, skipBackward);
    return 0;
}

// swd32.exe 0x00421230: literal pixels filled with the colour pair at *arg; lines advance by G.drawPitch
int BlitRle_421230(int, int, int, int, int arg) {
    const std::uint32_t color = colorArg(arg);
    walk3Forward(
        Walk3Options{0x84, true, false, true},
        [color](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) { fillForward(dest, src, n, color); },
        skipForward, skipForward);
    return 0;
}

// swd32.exe 0x00421540: mirrored 0x00421230 (wave phase not advanced)
int BlitRle_421540(int, int, int width, int, int arg) {
    const std::uint32_t color = colorArg(arg);
    walk3Mirror(
        Walk3Options{0x84, false, false, true}, width,
        [color](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) { fillBackward(dest, src, n, color); },
        skipBackward, skipBackward);
    return 0;
}

// swd32.exe 0x00421850: destination under the literal pixels turned grey ((r + g + b) / 4); like the tint
// routines it draws nothing unless a tint option is set. Wave row stride 0x528.
int BlitRle_421850(int, int, int, int, int) {
    if (!tinted()) {
        return 0;
    }
    walk3Forward(
        Walk3Options{0x528, true, false, false},
        [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            src += 2 * n;
            do {
                put16(dest, grayPixel(u16(dest)));
                dest += 2;
            } while (--n != 0);
        },
        skipForward, skipForward);
    return 0;
}

// swd32.exe 0x00421BE0: mirrored 0x00421850; leaves the mirrored clip start in G.drawSkipLeft
int BlitRle_421BE0(int, int, int width, int, int) {
    if (!tinted()) {
        return 0;
    }
    walk3Mirror(
        Walk3Options{0x84, true, true, false}, width,
        [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
            src += 2 * n;
            do {
                dest -= 2;
                put16(dest, grayPixel(u16(dest)));
            } while (--n != 0);
        },
        skipBackward, skipBackward);
    return 0;
}

}  // namespace swd3
