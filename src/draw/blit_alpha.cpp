#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

namespace {

// Premultiplied source with a per-pixel level from the alpha map: the destination terms 1/2, 1/4, 1/8, 1/16
// are kept for the bits (8, 4, 2, 1) that are clear in the level. Level 0 leaves the pixel, level 15 copies
// the source. The original uses the alpha byte as a byte offset into a 16-entry jump table, so valid maps
// hold multiples of 4 below 64; larger values are clamped here.
inline void alphaPixel(std::uint8_t* dest, std::uint32_t source, std::uint32_t alpha) {
    std::uint32_t level = alpha >> 2;
    if (level > 15) {
        level = 15;
    }
    if (level == 0) {
        return;
    }
    if (level == 15) {
        put16(dest, source);
        return;
    }
    const std::uint32_t d = u16(dest);
    std::uint32_t v = source;
    if (!(level & 8)) {
        v += (d >> 1) & static_cast<std::uint32_t>(G.halfMaskPairs[0]);
    }
    if (!(level & 4)) {
        v += (d >> 2) & static_cast<std::uint32_t>(G.halfMaskPairs[1]);
    }
    if (!(level & 2)) {
        v += (d >> 3) & static_cast<std::uint32_t>(G.halfMaskPairs[2]);
    }
    if (!(level & 1)) {
        v += (d >> 4) & static_cast<std::uint32_t>(G.halfMaskPairs[3]);
    }
    put16(dest, v);
}

// Pair variant used by the mirrored blitter: the destination pair at `dest` (this pixel and its right
// neighbour) and the source pair rotated by 16 bits are combined and both lanes are written.
inline void alphaPair(std::uint8_t* dest, const std::uint8_t* src, std::uint32_t alpha) {
    std::uint32_t level = alpha >> 2;
    if (level > 15) {
        level = 15;
    }
    if (level == 0) {
        return;
    }
    std::uint32_t s = u32(src);
    s = (s << 16) | (s >> 16);
    if (level == 15) {
        put32(dest, s);
        return;
    }
    const std::uint32_t d = u32(dest);
    std::uint32_t v = 0;
    if (!(level & 8)) {
        v += (d >> 1) & static_cast<std::uint32_t>(G.halfMaskPairs[0]);
    }
    if (!(level & 4)) {
        v += (d >> 2) & static_cast<std::uint32_t>(G.halfMaskPairs[1]);
    }
    v += s;
    if (!(level & 2)) {
        v += (d >> 3) & static_cast<std::uint32_t>(G.halfMaskPairs[2]);
    }
    if (!(level & 1)) {
        v += (d >> 4) & static_cast<std::uint32_t>(G.halfMaskPairs[3]);
    }
    put32(dest, v);
}

constexpr std::uint16_t kMarker = 0xFFF0;

}  // namespace

// swd32.exe 0x00419570: RLE picture with an alpha map (arg: one byte per pixel, rows of `width` bytes).
// 0x4000 runs are transparent, 0x8000 runs are filled with 0xFFF0.
int BlitRle_419570(int, int, int width, int, int arg) {
    auto* alphaRow = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(arg)));
    waveBegin();
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    const std::uint8_t* src = G.drawSource;
    std::uint8_t* dest = G.drawDest;
    if (u16(src + 6) & 0x10) {
        src += 8;
        for (std::int32_t i = 0; i != G.drawSkipTop; ++i) {
            alphaRow += width;
            src += u16(src) & 0x7FFF;
        }
        int line = 0;
        for (;;) {
            const std::uint16_t header = u16(src);
            const std::uint8_t* lineStart = src;
            const std::uint8_t* alpha = alphaRow;
            std::uint8_t* lineDest = dest;
            dest = waveLine(dest);
            src += 2;
            int x = 0;
            if (header == 0) {
                break;
            }
            ++line;
            if (line > G.drawRows) {
                break;
            }
            std::int32_t end = 0;
            std::uint32_t code = u16(src);
        dispatchLineStart:
            if (code & 0x4000) {
                goto clearRun;
            }
            if (code & 0x8000) {
                goto markerRun;
            }
        literal : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            const std::int32_t left = G.drawSkipLeft;
            if (x < left) {
                if (x + n <= left) {
                    x += n;
                    src += 2 * n;
                    alpha += n;
                    code = u16(src);
                    goto dispatchLineStart;
                }
                const std::int32_t cut = left - x;
                x = left;
                n -= cut;
                src += 2 * cut;
                alpha += cut;
            }
            end = left + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            if (n > 4 && (reinterpret_cast<std::uintptr_t>(dest) & 2)) {
                // The alignment-pixel jump table of the original has a no-op at level 12.
                if ((*alpha >> 2) != 12) {
                    alphaPixel(dest, u16(src), *alpha);
                }
                dest += 2;
                src += 2;
                alpha += 1;
                --n;
            }
            while (n != 0) {
                alphaPixel(dest, u16(src), *alpha);
                dest += 2;
                src += 2;
                alpha += 1;
                --n;
            }
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (code & 0x4000) {
                goto clearRun;
            }
            goto markerRun;
        }
        clearRun : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            const std::int32_t left = G.drawSkipLeft;
            if (x < left) {
                if (x + n <= left) {
                    x += n;
                    alpha += n;
                    code = u16(src);
                    if (code & 0x8000) {
                        goto markerRun;
                    }
                    goto literal;
                }
                const std::int32_t cut = left - x;
                x = left;
                alpha += cut;
                n -= cut;
            }
            end = left + G.drawWidth;
            if (x >= end) {
                goto lineDone;
            }
            if (x + n > end) {
                n = end - x;
            }
            alpha += n;
            x += n;
            dest += 2 * n;
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (!(code & 0x8000)) {
                goto literal;
            }
        }
        markerRun : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            const std::int32_t left = G.drawSkipLeft;
            if (x < left) {
                if (x + n <= left) {
                    alpha += n;
                    x += n;
                    code = u16(src);
                    if (code & 0x4000) {
                        goto clearRun;
                    }
                    goto literal;
                }
                const std::int32_t cut = left - x;
                x = left;
                alpha += cut;
                n -= cut;
            }
            end = left + G.drawWidth;
            if (x >= end) {
                goto lineDone;
            }
            if (x + n > end) {
                n = end - x;
            }
            alpha += n;
            x += n;
            do {
                put16(dest, kMarker);
                dest += 2;
            } while (--n != 0);
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (code & 0x4000) {
                goto clearRun;
            }
            goto literal;
        }
        lineDone:
            alphaRow += width;
            dest = lineDest + G.screenPitch;
            src = lineStart + (header & 0x7FFF);
        }
    }
    waveAdvancePhase();
    return 0;
}

// swd32.exe 0x0041A3B0: mirrored 0x00419570. Literal runs longer than 4 pixels are processed one pixel per
// step but written as pairs (see alphaPair), so each step also re-blends the pixel to its right. 0x8000 runs
// map the pixels through the table at G.darkenTable, one pixel to the right of the run.
int BlitRle_41A3B0(int, int, int width, int, int arg) {
    auto* alphaRow = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(static_cast<std::uint32_t>(arg)));
    waveBegin();
    const std::int32_t start = width - (G.drawSkipLeft + G.drawWidth);
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    std::uint8_t* dest = G.drawDest + G.drawWidth * 2;
    const std::uint8_t* src = G.drawSource;
    if (u16(src + 6) & 0x10) {
        src += 8;
        for (std::int32_t i = 0; i != G.drawSkipTop; ++i) {
            alphaRow += width;
            src += u16(src) & 0x7FFF;
        }
        int line = 0;
        for (;;) {
            const std::uint16_t header = u16(src);
            const std::uint8_t* lineStart = src;
            const std::uint8_t* alpha = alphaRow;
            std::uint8_t* lineDest = dest;
            dest = waveLine(dest);
            src += 2;
            int x = 0;
            if (header == 0) {
                break;
            }
            ++line;
            if (line > G.drawRows) {
                break;
            }
            std::int32_t end = 0;
            std::uint32_t code = u16(src);
        dispatchLineStart:
            if (code & 0x4000) {
                goto clearRun;
            }
            if (code & 0x8000) {
                goto lookupRun;
            }
        literal : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            if (x < start) {
                if (x + n <= start) {
                    x += n;
                    src += 2 * n;
                    alpha += n;
                    code = u16(src);
                    goto dispatchLineStart;
                }
                const std::int32_t cut = start - x;
                x = start;
                n -= cut;
                src += 2 * cut;
                alpha += cut;
            }
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            if (n <= 4) {
                do {
                    dest -= 2;
                    alphaPixel(dest, u16(src), *alpha);
                    src += 2;
                    alpha += 1;
                } while (--n != 0);
            } else {
                bool done = false;
                if (reinterpret_cast<std::uintptr_t>(dest) & 2) {
                    dest -= 2;
                    alphaPixel(dest, u16(src), *alpha);
                    src += 2;
                    alpha += 1;
                    if (--n == 0) {
                        done = true;
                    }
                }
                if (!done) {
                    do {
                        dest -= 2;
                        alphaPair(dest, src, *alpha);
                        src += 2;
                        alpha += 1;
                    } while (--n >= 1);
                }
            }
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (code & 0x4000) {
                goto clearRun;
            }
            goto lookupRun;
        }
        clearRun : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            if (x < start) {
                if (x + n <= start) {
                    x += n;
                    alpha += n;
                    code = u16(src);
                    if (code & 0x8000) {
                        goto lookupRun;
                    }
                    goto literal;
                }
                const std::int32_t cut = start - x;
                x = start;
                n -= cut;
                alpha += cut;
            }
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            dest -= 2 * n;
            alpha += n;
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (!(code & 0x8000)) {
                goto literal;
            }
        }
        lookupRun : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            if (x < start) {
                if (x + n <= start) {
                    x += n;
                    alpha += n;
                    code = u16(src);
                    if (code & 0x4000) {
                        goto clearRun;
                    }
                    goto literal;
                }
                const std::int32_t cut = start - x;
                x = start;
                n -= cut;
                alpha += cut;
            }
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            alpha += n;
            do {
                const std::uint32_t index = (static_cast<std::uint32_t>(u16(dest)) >> 1) & 0xFFFFFFFEu;
                dest -= 2;
                put16(dest + 2, u16(G.darkenTable + index));
            } while (--n != 0);
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (code & 0x4000) {
                goto clearRun;
            }
            goto literal;
        }
        lineDone:
            alphaRow += width;
            dest = lineDest + G.screenPitch;
            src = lineStart + (header & 0x7FFF);
        }
    }
    waveAdvancePhase();
    return 0;
}

}  // namespace swd3
