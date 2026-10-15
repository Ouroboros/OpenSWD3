#include "draw/blit_tint.h"
#include "draw/blit_walk.h"

namespace swd3 {

using namespace blit;

namespace {

// Per-line shear: G.drawShear moves by G.drawShearDir for every 0x400 of accumulated G.drawShearStep.
void shearAdvance(std::uint32_t& shearFrac) {
    const std::uint32_t acc = shearFrac + static_cast<std::uint32_t>(G.drawShearStep);
    shearFrac = acc & 0x3FF;
    G.drawShear += G.drawShearDir * static_cast<std::int32_t>(acc >> 10);
}

void skipForward(std::uint8_t*& dest, std::int32_t n) { dest += 2 * n; }
void skipBackward(std::uint8_t*& dest, std::int32_t n) { dest -= 2 * n; }

// Shared body of 0x0041F8D0 / 0x0041FEA0. The top skip loop always skips one line more than G.drawSkipTop
// and stops at the end of the picture. Every output line recomputes G.drawWidth, G.drawSkipLeft and
// G.drawDest from the sheared x position and the clip rectangle; the wave offset is consumed but never
// applied to the destination. A line clipped away entirely still advances the source.
template <typename Lit>
void shearWalk(bool mirror, int x, int y, int width, Lit&& literalOp) {
    if (G.drawSkipLeft != 0) {
        x = -G.drawSkipLeft;
    }
    std::uint32_t scaleFrac = 0;
    std::uint32_t shearFrac = 0;
    waveBegin();
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    const std::uint8_t* src = G.drawSource;
    std::uint8_t* dest = G.drawDest;
    if (!(u16(src + 6) & 0x10)) {
        goto finished;
    }
    src += 8;
    for (std::int32_t counter = 0;; ++counter) {
        const std::uint16_t h = u16(src);
        if (h == 0) {
            goto finished;
        }
        src += h & 0x7FFF;
        std::uint32_t step = scaleStep(scaleFrac);
        if (!(G.drawScaleUp & 1)) {
            while (step-- != 0) {
                const std::uint16_t next = u16(src);
                if (next == 0) {
                    goto finished;
                }
                src += next & 0x7FFF;
            }
        } else if (step != 0) {
            const std::uint16_t next = u16(src);
            if (next == 0) {
                goto finished;
            }
            src += next & 0x7FFF;
        }
        shearAdvance(shearFrac);
        if (counter == G.drawSkipTop) {
            break;
        }
    }
    {
        int line = 0;
        while (src != nullptr) {
            const std::uint16_t header = u16(src);
            const std::uint8_t* lineStart = src;
            dest = waveLine(dest);
            src += 2;
            if (header == 0) {
                break;
            }
            ++line;
            if (line > G.drawRows) {
                break;
            }
            shearAdvance(shearFrac);
            G.drawWidth = width;
            G.drawSkipLeft = 0;
            std::int32_t xpos = x + G.drawShear;
            if (G.clipX > xpos) {
                G.drawWidth = xpos + G.drawWidth - G.clipX;
                if (G.drawWidth <= 0) {
                    goto lineDone;
                }
                G.drawSkipLeft = G.clipX - xpos;
                xpos = G.clipX;
            }
            if (xpos + G.drawWidth >= G.clipX + G.clipWidth) {
                G.drawWidth = G.clipX + G.clipWidth - xpos;
                if (G.drawWidth <= 0) {
                    goto lineDone;
                }
            }
            {
                std::int32_t row = y + line;
                std::int32_t start = 0;
                if (mirror) {
                    xpos += G.drawWidth;
                    start = width - G.drawSkipLeft - G.drawWidth;
                }
                if (G.drawPitch < 0) {
                    row = y + G.drawRows - line;
                }
                G.drawDest = G.drawSurface + (&G.rowOffsets[0])[row] + xpos + xpos;
                dest = G.drawDest;
                if (mirror) {
                    walk3LineMirror(dest, src, start, literalOp, skipBackward, skipBackward);
                } else {
                    walk3LineForward(dest, src, literalOp, skipForward, skipForward);
                }
            }
        lineDone:
            dest += G.drawPitch;
            src = scaledNextLine(lineStart, header, scaleFrac);
        }
    }
finished:
    if (!mirror) {
        waveAdvancePhase();
    }
}

}  // namespace

// swd32.exe 0x0041F8D0: sheared and vertically scaled destination tint (always applied, also with zero
// tint options); x is replaced by -G.drawSkipLeft when that is non-zero
int BlitRle_41F8D0(int x, int y, int width, int, int) {
    const Tint t = makeTint();
    shearWalk(false, x, y, width, [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
        src += 2 * n;
        do {
            put16(dest, tintStep(t, u16(dest)));
            dest += 2;
        } while (--n != 0);
    });
    return 0;
}

// swd32.exe 0x0041FEA0: mirrored 0x0041F8D0 (wave phase not advanced)
int BlitRle_41FEA0(int x, int y, int width, int, int) {
    const Tint t = makeTint();
    shearWalk(true, x, y, width, [&t](std::uint8_t*& dest, const std::uint8_t*& src, std::int32_t n) {
        src += 2 * n;
        do {
            dest -= 2;
            put16(dest, tintStep(t, u16(dest)));
        } while (--n != 0);
    });
    return 0;
}

}  // namespace swd3
