#pragma once

#include "draw/blit.h"

// Line walkers shared by the RLE blitters whose control flow is identical to 0x00418350 (forward) and
// 0x004185C0 (mirrored); the routines differ only in the literal-span operation and in the options below.
namespace swd3::blit {

struct WalkOptions {
    bool drawPitch;     // advance lines by G.drawPitch (true) or by G.screenPitch (false)
    bool dither;        // G.drawOption300: drop every third line using G.drawDitherPhase
    bool advanceWave;   // advance G.wavePhase after the picture
};

// Span(dest, src, n): processes n literal pixels; forward walkers pass dest at the first pixel and expect
// dest/src advanced by n, mirrored walkers pass dest one past the first pixel and expect it moved back by n.
template <typename Span>
void walkForward(const WalkOptions& options, Span&& span) {
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
            if (options.dither && G.drawOption300 != 0) {
                if (++G.drawDitherPhase >= 3) {
                    G.drawDitherPhase = 0;
                    goto lineDone;
                }
            }
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
            }
            end = left + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            span(dest, src, n);
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
            dest = lineDest + (options.drawPitch ? G.drawPitch : G.screenPitch);
            src = lineStart + (header & 0x7FFF);
        }
    }
    if (options.advanceWave) {
        waveAdvancePhase();
    }
}

template <typename Span>
void walkMirror(const WalkOptions& options, int width, Span&& span) {
    waveBegin();
    const std::int32_t start = width - (G.drawSkipLeft + G.drawWidth);
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    std::uint8_t* dest = G.drawDest + G.drawWidth * 2;
    const std::uint8_t* src = G.drawSource;
    if (u16(src + 6) & 0x10) {
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
            if (options.dither && G.drawOption300 != 0) {
                if (++G.drawDitherPhase >= 3) {
                    G.drawDitherPhase = 0;
                    goto lineDone;
                }
            }
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
            }
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            span(dest, src, n);
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
            }
            end = start + G.drawWidth;
            if (x + n > end) {
                n = end - x;
            }
            x += n;
            dest -= 2 * n;
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
            dest = lineDest + (options.drawPitch ? G.drawPitch : G.screenPitch);
            src = lineStart + (header & 0x7FFF);
        }
    }
    if (options.advanceWave) {
        waveAdvancePhase();
    }
}

// Walkers for the blitters that distinguish 0x4000 and 0x8000 runs (reference: 0x0041B280 forward,
// 0x0041B620 mirrored). Dispatch order: at a line start 0x4000 before 0x8000; after a literal run 0x4000
// else the 0x8000 handler; after a 0x4000 run 0x8000 else literal; after a 0x8000 run 0x4000 else literal.
// Lit(dest, src, n) handles n literal pixels; RunA/RunB(dest, n) handle the visible part of 0x4000 / 0x8000
// runs. Forward ops advance dest by 2n; mirrored ops receive dest one past the first pixel and move it back.
struct Walk3Options {
    std::int32_t waveStride;
    bool advanceWave;
    bool storeStart;  // mirrored walkers that keep the mirrored clip start in G.drawSkipLeft
    bool drawPitch;   // advance lines by G.drawPitch instead of G.screenPitch
    bool scaled = false;      // vertical scaling by G.drawScaleFrac / G.drawScaleUp (see scaledSkipTop, scaledNextLine)
};

// One output line of vertical scaling: returns the number of extra source lines (shrink) or, when
// G.drawScaleUp is set, whether to move to the next source line at all.
inline std::uint32_t scaleStep(std::uint32_t& frac) {
    const std::uint32_t acc = frac + static_cast<std::uint32_t>(G.drawScaleFrac);
    frac = acc & 0x3FF;
    return acc >> 10;
}

// Skips G.drawSkipTop lines for the scaled blitters (0x004208D0, 0x00420D70). Each round skips one line plus
// the scaled extra lines, and the round counter is incremented twice per round, so only about half of the
// requested rounds are made. Returns nullptr when the end of the picture is reached.
inline const std::uint8_t* scaledSkipTop(const std::uint8_t* src, std::uint32_t& frac) {
    std::int32_t counter = 0;
    while (counter != G.drawSkipTop) {
        ++counter;
        src += u16(src) & 0x7FFF;
        std::uint32_t step = scaleStep(frac);
        if (!(G.drawScaleUp & 1)) {
            while (step-- != 0) {
                const std::uint16_t h = u16(src);
                if (h == 0) {
                    return nullptr;
                }
                src += h & 0x7FFF;
            }
        } else if (step != 0) {
            const std::uint16_t h = u16(src);
            if (h == 0) {
                return nullptr;
            }
            src += h & 0x7FFF;
        }
        if (counter == G.drawSkipTop) {
            break;
        }
        ++counter;
    }
    return src;
}

// Source line for the next output line of the scaled blitters; nullptr ends the picture.
inline const std::uint8_t* scaledNextLine(const std::uint8_t* lineStart, std::uint16_t header, std::uint32_t& frac) {
    std::uint32_t step = scaleStep(frac);
    if (!(G.drawScaleUp & 1)) {
        const std::uint8_t* src = lineStart + (header & 0x7FFF);
        while (step-- != 0) {
            const std::uint16_t h = u16(src);
            if (h == 0) {
                return nullptr;
            }
            src += h & 0x7FFF;
        }
        return src;
    }
    if (step == 0) {
        return lineStart;
    }
    const std::uint8_t* src = lineStart + (header & 0x7FFF);
    return u16(src) == 0 ? nullptr : src;
}

template <typename Lit, typename RunA, typename RunB>
void walk3LineForward(std::uint8_t* dest, const std::uint8_t* src, Lit& literalOp, RunA& runAOp, RunB& runBOp) {
    int x = 0;
    std::int32_t end = 0;
    std::uint32_t code = u16(src);
dispatch:
    if (code & 0x4000) {
        goto runA;
    }
    if (code & 0x8000) {
        goto runB;
    }
literal : {
    std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
    src += 2;
    if (n == 0) {
        return;
    }
    const std::int32_t left = G.drawSkipLeft;
    if (x < left) {
        if (x + n <= left) {
            x += n;
            src += 2 * n;
            code = u16(src);
            goto dispatch;
        }
        const std::int32_t cut = left - x;
        x = left;
        n -= cut;
        src += 2 * cut;
    }
    end = left + G.drawWidth;
    if (x + n > end) {
        n = end - x;
    }
    x += n;
    literalOp(dest, src, n);
    if (x >= end) {
        return;
    }
    code = u16(src);
    if (code & 0x4000) {
        goto runA;
    }
    goto runB;
}
runA : {
    std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
    src += 2;
    if (n == 0) {
        return;
    }
    const std::int32_t left = G.drawSkipLeft;
    if (x < left) {
        if (x + n <= left) {
            x += n;
            code = u16(src);
            if (code & 0x8000) {
                goto runB;
            }
            goto literal;
        }
        const std::int32_t cut = left - x;
        x = left;
        n -= cut;
    }
    end = left + G.drawWidth;
    if (x >= end) {
        return;
    }
    if (x + n > end) {
        n = end - x;
    }
    x += n;
    runAOp(dest, n);
    if (x >= end) {
        return;
    }
    code = u16(src);
    if (!(code & 0x8000)) {
        goto literal;
    }
}
runB : {
    std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
    src += 2;
    if (n == 0) {
        return;
    }
    const std::int32_t left = G.drawSkipLeft;
    if (x < left) {
        if (x + n <= left) {
            x += n;
            code = u16(src);
            if (code & 0x4000) {
                goto runA;
            }
            goto literal;
        }
        const std::int32_t cut = left - x;
        x = left;
        n -= cut;
    }
    end = left + G.drawWidth;
    if (x >= end) {
        return;
    }
    if (x + n > end) {
        n = end - x;
    }
    x += n;
    runBOp(dest, n);
    if (x >= end) {
        return;
    }
    code = u16(src);
    if (code & 0x4000) {
        goto runA;
    }
    goto literal;
}
}

template <typename Hook, typename Lit, typename RunA, typename RunB>
void walk3ForwardHooked(const Walk3Options& options, Hook&& lineHook, Lit&& literalOp, RunA&& runAOp, RunB&& runBOp) {
    waveBegin(options.waveStride);
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    const std::uint8_t* src = G.drawSource;
    std::uint8_t* dest = G.drawDest;
    if (u16(src + 6) & 0x10) {
        std::uint32_t frac = 0;
        if (options.scaled) {
            src = scaledSkipTop(src + 8, frac);
        } else {
            src += 8;
            for (std::int32_t i = 0; i != G.drawSkipTop; ++i) {
                src += u16(src) & 0x7FFF;
                lineHook();
            }
        }
        int line = 0;
        while (src != nullptr) {
            const std::uint16_t header = u16(src);
            const std::uint8_t* lineStart = src;
            std::uint8_t* lineDest = dest;
            dest = waveLine(dest);
            lineHook();
            src += 2;
            if (header == 0) {
                break;
            }
            ++line;
            if (line > G.drawRows) {
                break;
            }
            walk3LineForward(dest, src, literalOp, runAOp, runBOp);
            dest = lineDest + (options.drawPitch ? G.drawPitch : G.screenPitch);
            src = options.scaled ? scaledNextLine(lineStart, header, frac) : lineStart + (header & 0x7FFF);
        }
    }
    if (options.advanceWave) {
        waveAdvancePhase();
    }
}

template <typename Lit, typename RunA, typename RunB>
void walk3Forward(const Walk3Options& options, Lit&& literalOp, RunA&& runAOp, RunB&& runBOp) {
    walk3ForwardHooked(options, [] {}, literalOp, runAOp, runBOp);
}

template <typename Lit, typename RunA, typename RunB>
void walk3LineMirror(std::uint8_t* dest, const std::uint8_t* src, std::int32_t start, Lit& literalOp, RunA& runAOp, RunB& runBOp) {
    int x = 0;
    std::int32_t end = 0;
    std::uint32_t code = u16(src);
dispatch:
    if (code & 0x4000) {
        goto runA;
    }
    if (code & 0x8000) {
        goto runB;
    }
literal : {
    std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
    src += 2;
    if (n == 0) {
        return;
    }
    if (x < start) {
        if (x + n <= start) {
            x += n;
            src += 2 * n;
            code = u16(src);
            goto dispatch;
        }
        const std::int32_t cut = start - x;
        x = start;
        n -= cut;
        src += 2 * cut;
    }
    end = start + G.drawWidth;
    if (x + n > end) {
        n = end - x;
    }
    x += n;
    literalOp(dest, src, n);
    if (x >= end) {
        return;
    }
    code = u16(src);
    if (code & 0x4000) {
        goto runA;
    }
    goto runB;
}
runA : {
    std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
    src += 2;
    if (n == 0) {
        return;
    }
    if (x < start) {
        if (x + n <= start) {
            x += n;
            code = u16(src);
            if (code & 0x8000) {
                goto runB;
            }
            goto literal;
        }
        const std::int32_t cut = start - x;
        x = start;
        n -= cut;
    }
    end = start + G.drawWidth;
    if (x + n > end) {
        n = end - x;
    }
    x += n;
    runAOp(dest, n);
    if (x >= end) {
        return;
    }
    code = u16(src);
    if (!(code & 0x8000)) {
        goto literal;
    }
}
runB : {
    std::int32_t n = static_cast<std::int32_t>(code & 0x3FFF);
    src += 2;
    if (n == 0) {
        return;
    }
    if (x < start) {
        if (x + n <= start) {
            x += n;
            code = u16(src);
            if (code & 0x4000) {
                goto runA;
            }
            goto literal;
        }
        const std::int32_t cut = start - x;
        x = start;
        n -= cut;
    }
    end = start + G.drawWidth;
    if (x + n > end) {
        n = end - x;
    }
    x += n;
    runBOp(dest, n);
    if (x >= end) {
        return;
    }
    code = u16(src);
    if (code & 0x4000) {
        goto runA;
    }
    goto literal;
}
}

template <typename Lit, typename RunA, typename RunB>
void walk3Mirror(const Walk3Options& options, int width, Lit&& literalOp, RunA&& runAOp, RunB&& runBOp) {
    waveBegin(options.waveStride);
    const std::int32_t start = width - (G.drawSkipLeft + G.drawWidth);
    if (options.storeStart) {
        G.drawSkipLeft = start;
    }
    G.drawLineSkip = G.screenPitch - G.drawWidth - G.drawWidth;
    std::uint8_t* dest = G.drawDest + G.drawWidth * 2;
    const std::uint8_t* src = G.drawSource;
    if (u16(src + 6) & 0x10) {
        std::uint32_t frac = 0;
        src = options.scaled ? scaledSkipTop(src + 8, frac) : skipLines(src + 8, G.drawSkipTop);
        int line = 0;
        while (src != nullptr) {
            const std::uint16_t header = u16(src);
            const std::uint8_t* lineStart = src;
            std::uint8_t* lineDest = dest;
            dest = waveLine(dest);
            src += 2;
            if (header == 0) {
                break;
            }
            ++line;
            if (line > G.drawRows) {
                break;
            }
            walk3LineMirror(dest, src, start, literalOp, runAOp, runBOp);
            dest = lineDest + (options.drawPitch ? G.drawPitch : G.screenPitch);
            src = options.scaled ? scaledNextLine(lineStart, header, frac) : lineStart + (header & 0x7FFF);
        }
    }
    if (options.advanceWave) {
        waveAdvancePhase();
    }
}

// Saturating add of one colour channel (mask), as in the additive blitters.
inline std::uint32_t addChannel(std::uint32_t a, std::uint32_t b, std::uint32_t mask, std::uint32_t inverse) {
    std::uint32_t v = (a & mask) + (b & mask);
    if (inverse & v) {
        v = mask;
    }
    return v;
}

}  // namespace swd3::blit
