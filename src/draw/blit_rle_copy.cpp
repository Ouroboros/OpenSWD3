#include "draw/blit.h"
#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

// swd32.exe 0x00418350: RLE copy (both run kinds are transparent)
int BlitRle_418350(int, int, int, int, int) {
    waveBegin();
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    const std::uint8_t* src = G.drawSource;
    std::uint8_t* dest = G.drawDest;
    if (u16(src + 6) & 0x10) {
        src = skipLines(src + 8, G.drawSkipTop);
        int line = 0;
        for (;;) {
            const std::uint16_t header = u16(src);
            const std::uint8_t* lineStart = src;
            std::uint8_t* lineDest = dest;
            dest = waveLine(dest);
            src += 2;
            ++line;
            int x = 0;
            if (header == 0) {
                break;
            }
            if (line > G.drawRows) {
                break;
            }
            std::int32_t end = 0;
            std::uint32_t code = u16(src);
        nextCode:
            if (code & 0xC000) {
                goto skipRun;
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
                    src += 2 * n;
                    x += n;
                    code = u16(src);
                    goto nextCode;
                }
                const std::int32_t cut = left - x;
                x = left;
                src += 2 * cut;
                n -= cut;
                end = left + G.drawWidth;
                if (x + n > end) {
                    n = end - x;
                }
            } else {
                end = left + G.drawWidth;
                if (x + n > end) {
                    n = end - x;
                }
            }
            x += n;
            if (n <= 4) {
                do {
                    put16(dest, u16(src));
                    dest += 2;
                    src += 2;
                } while (--n != 0);
            } else {
                if (reinterpret_cast<std::uintptr_t>(dest) & 2) {
                    put16(dest, u16(src));
                    dest += 2;
                    src += 2;
                    --n;
                }
                do {
                    put32(dest, u32(src));
                    dest += 4;
                    src += 4;
                    n -= 2;
                } while (n >= 2);
                if (n & 1) {
                    put16(dest, u16(src));
                    dest += 2;
                    src += 2;
                }
            }
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
        }
        skipRun : {
            std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
            src += 2;
            if (n == 0) {
                goto lineDone;
            }
            const std::int32_t left = G.drawSkipLeft;
            if (x < left) {
                if (x + n <= left) {
                    x += n;
                    code = u16(src);
                    if (code & 0xC000) {
                        goto skipRun;
                    }
                    goto literal;
                }
                const std::int32_t cut = left - x;
                x = left;
                n -= cut;
            }
            end = left + G.drawWidth;
            if (x >= end) {
                goto lineDone;
            }
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            dest += 2 * n;
            if (x >= end) {
                goto lineDone;
            }
            code = u16(src);
            if (code & 0xC000) {
                goto skipRun;
            }
            goto literal;
        }
        lineDone:
            dest = lineDest + G.drawPitch;
            src = lineStart + (header & 0x7FFF);
        }
    }
    waveAdvancePhase();
    return 0;
}

// swd32.exe 0x004185C0: RLE copy mirrored horizontally (wave phase is not advanced)
int BlitRle_4185C0(int, int, int width, int, int) {
    waveBegin();
    const std::int32_t start = width - (G.drawSkipLeft + G.drawWidth);
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    std::uint8_t* dest = G.drawDest + G.drawWidth * 2;
    const std::uint8_t* src = G.drawSource;
    if (!(u16(src + 6) & 0x10)) {
        return 0;
    }
    src = skipLines(src + 8, G.drawSkipTop);
    int line = 0;
    for (;;) {
        const std::uint16_t header = u16(src);
        const std::uint8_t* lineStart = src;
        std::uint8_t* lineDest = dest;
        dest = waveLine(dest);
        int x = 0;
        src += 2;
        if (header == 0) {
            break;
        }
        ++line;
        if (line > G.drawRows) {
            break;
        }
        std::int32_t end = 0;
        std::uint32_t code = u16(src);
    nextCode:
        if (code & 0xC000) {
            goto skipRun;
        }
    literal : {
        std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
        src += 2;
        if (n == 0) {
            goto lineDone;
        }
        if (x < start) {
            if (x + n <= start) {
                src += 2 * n;
                x += n;
                code = u16(src);
                goto nextCode;
            }
            const std::int32_t cut = start - x;
            x = start;
            n -= cut;
            src += 2 * cut;
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
        } else {
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
        }
        x += n;
        if (n <= 4) {
            do {
                const std::uint16_t v = u16(src);
                dest -= 2;
                src += 2;
                put16(dest, v);
            } while (--n != 0);
        } else {
            bool done = false;
            if (reinterpret_cast<std::uintptr_t>(dest) & 2) {
                const std::uint16_t v = u16(src);
                dest -= 2;
                src += 2;
                put16(dest, v);
                if (--n == 0) {
                    done = true;
                }
            }
            if (!done) {
                do {
                    std::uint32_t v = u32(src);
                    dest -= 4;
                    v = (v << 16) | (v >> 16);
                    src += 4;
                    n -= 2;
                    put32(dest, v);
                } while (n >= 2);
                if (n & 1) {
                    const std::uint16_t v = u16(src);
                    dest -= 2;
                    src += 2;
                    put16(dest, v);
                }
            }
        }
        if (x >= end) {
            goto lineDone;
        }
        code = u16(src);
        if (!(code & 0xC000)) {
            goto literal;
        }
    }
    skipRun : {
        std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
        src += 2;
        if (n == 0) {
            goto lineDone;
        }
        if (x < start) {
            if (x + n <= start) {
                x += n;
                code = u16(src);
                if (code & 0xC000) {
                    goto skipRun;
                }
                goto literal;
            }
            const std::int32_t cut = start - x;
            x = start;
            n -= cut;
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
        } else {
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
        }
        x += n;
        dest -= n;
        dest -= n;
        if (x >= end) {
            goto lineDone;
        }
        code = u16(src);
        if (code & 0xC000) {
            goto skipRun;
        }
        goto literal;
    }
    lineDone:
        dest = lineDest + G.drawPitch;
        src = lineStart + (header & 0x7FFF);
    }
    return 0;
}

namespace {

// Smear step: G.drawSmearCounter cycles 1..16 and picks the left neighbour, the pixel 0x500 bytes below,
// 0x500 bytes above or the right neighbour (0x500 is a fixed 640-pixel row); the 17th step resets the
// counter without writing. Source pixels are only skipped.
void smearPixel(std::uint8_t* at) {
    const std::int32_t step = G.drawSmearCounter + 1;
    if (step <= 4) {
        G.drawSmearCounter = step;
        put16(at, u16(at - 2));
    } else if (step <= 10) {
        G.drawSmearCounter = step;
        put16(at, u16(at + 0x500));
    } else if (step <= 13) {
        G.drawSmearCounter = step;
        put16(at, u16(at - 0x500));
    } else if (step <= 16) {
        G.drawSmearCounter = step;
        put16(at, u16(at + 2));
    } else {
        G.drawSmearCounter = 0;
    }
}

}  // namespace

// swd32.exe 0x00422730: smears the destination under the literal pixels
int BlitRle_422730(int, int, int, int, int) {
    walkForward(WalkOptions{true, false, true}, [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
        do {
            smearPixel(dest);
            src += 2;
            dest += 2;
        } while (--n != 0);
    });
    return 0;
}

// swd32.exe 0x004229C0: mirrored 0x00422730; writes at the current pointer before stepping back, so every
// pixel lands one to the right of the mirrored position
int BlitRle_4229C0(int, int, int width, int, int) {
    walkMirror(WalkOptions{true, false, false}, width,
               [](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
                   do {
                       smearPixel(dest);
                       src += 2;
                       dest -= 2;
                   } while (--n != 0);
               });
    return 0;
}

}  // namespace swd3
