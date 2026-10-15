#include "draw/zoom.h"

#include "draw/blit.h"

namespace swd3 {

using namespace blit;

namespace {

using uptr = std::uintptr_t;

uptr addr(const std::uint8_t* p) { return reinterpret_cast<uptr>(p); }

}  // namespace

// swd32.exe 0x00422C70: draws the RLE picture G.drawSource scaled by G.zoomX / G.zoomY (1/1024) so that the
// point (G.zoomAnchorX, G.zoomAnchorY) of the picture stays in place; 0x4000 and 0x8000 runs are transparent.
// Source lines above the clip rectangle are consumed while the row pointer is above it; the row remainder is
// then cut back to below 0x400, dropping pending repeats. Rows below the clip bottom drop the remaining repeats
// of their source line in the same way.
void DrawZoomed(int x, int y, int width, int height) {
    const std::int32_t zoomX = G.zoomX;
    const std::int32_t zoomY = G.zoomY;
    const std::int32_t left = G.zoomAnchorX - G.zoomAnchorX * zoomX / 1024 + x;
    std::int32_t row = G.zoomAnchorY - G.zoomAnchorY * zoomY / 1024 + y;
    if (left >= G.clipX + G.clipWidth) {
        return;
    }
    if (width * zoomX / 1024 + left < G.clipX) {
        return;
    }
    if (row >= G.clipY + G.clipHeight) {
        return;
    }
    if (height * zoomY / 1024 + row < G.clipY) {
        return;
    }
    const std::int32_t halfPitch = G.screenPitch / 2;
    const uptr clipTop = addr(G.drawSurface) + 2 * (G.clipY * halfPitch);
    std::uint8_t* rowClipLeft = G.drawSurface + 2 * (row * halfPitch + G.clipX);
    std::uint8_t* rowClipRight = rowClipLeft + 2 * G.clipWidth;
    std::uint8_t* rowDest = G.drawSurface + 2 * (G.screenPitch * row / 2 + left);
    const std::uint8_t* line = G.drawSource + 8;
    std::int32_t rowFrac = 0;
    if (u16(line) != 0) {
        for (;;) {
            rowFrac += zoomY;
            bool clipped = false;
            while (rowFrac >= 0x400) {
                if (addr(rowClipLeft) >= clipTop) {
                    clipped = true;
                    break;
                }
                rowFrac -= 0x400;
                ++row;
                rowClipLeft += 2 * halfPitch;
                rowClipRight = rowClipLeft + 2 * G.clipWidth;
                rowDest += 2 * halfPitch;
            }
            if (clipped) {
                break;
            }
            line += u16(line) & 0x3FFF;
            if (u16(line) == 0) {
                break;
            }
        }
    }
    rowFrac %= 0x400;
    if (u16(line) == 0) {
        return;
    }
    for (;;) {
        rowFrac += zoomY;
        while (rowFrac >= 0x400) {
            rowFrac -= 0x400;
            if (row >= G.clipY + G.clipHeight) {
                break;
            }
            std::uint8_t* dest = rowDest;
            const std::uint8_t* code = line + 2;
            std::int32_t colFrac = 0;
            std::uint16_t c = u16(code);
            while (c != 0) {
                const std::int32_t n = c & 0x3FFF;
                const std::uint8_t* runCode = code;
                const std::int32_t scaled = zoomX * n / 1024;
                if (!(c & 0xC000)) {
                    code += 2;
                }
                const std::int32_t span = 2 * scaled;
                const std::int32_t fromLeft = static_cast<std::int32_t>(dest - rowClipLeft) >> 1;
                const std::int32_t over = static_cast<std::int32_t>(dest + span - rowClipRight) >> 1;
                uptr lower;
                uptr upper;
                bool draw = true;
                if (fromLeft < 0) {
                    lower = addr(rowClipLeft);
                    upper = addr(dest) + span;
                    if (fromLeft + scaled <= 0) {
                        draw = false;
                    } else if (over > 0) {
                        upper += 2 * static_cast<std::uint32_t>(-1 - over);
                    }
                } else if (fromLeft >= G.clipWidth) {
                    draw = false;
                } else {
                    lower = addr(dest);
                    upper = addr(dest) + span;
                    if (over > 0) {
                        upper += 2 * static_cast<std::uint32_t>(-1 - over);
                    }
                }
                if (draw) {
                    if (u16(runCode) & 0xC000) {
                        colFrac += zoomX * scaled;
                    } else {
                        std::uint8_t* out = dest;
                        const std::uint8_t* pixel = code;
                        colFrac %= 0x400;
                        do {
                            colFrac += zoomX;
                            bool stop = false;
                            while (colFrac >= 0x400) {
                                colFrac -= 0x400;
                                if (addr(out) > upper) {
                                    stop = true;
                                    break;
                                }
                                if (addr(out) >= lower) {
                                    put16(out, u16(pixel));
                                }
                                out += 2;
                            }
                            if (stop) {
                                break;
                            }
                            pixel += 2;
                        } while (addr(pixel) < addr(code) + 2 * static_cast<std::uint32_t>(n));
                    }
                }
                dest += span;
                const std::uint16_t runHeader = u16(runCode);
                if (runHeader & 0xC000) {
                    code += 2;
                } else {
                    code += 2 * (runHeader & 0x3FFF);
                }
                c = u16(code);
            }
            ++row;
            const std::int32_t rowStep = G.screenPitch / 2 * 2;
            rowClipLeft += rowStep;
            rowClipRight = rowClipLeft + 2 * G.clipWidth;
            rowDest += rowStep;
        }
        line += u16(line) & 0x3FFF;
        if (u16(line) == 0) {
            return;
        }
    }
}

// swd32.exe 0x00423020: mirrored 0x00422C70. The anchor is mirrored ((width - G.zoomAnchorX) scaled), and
// the picture is skipped entirely when its scaled bottom reaches the clip bottom or its top is above the clip
// top. Pixels of a run are written from the current right edge itself leftwards (one pixel right of the
// mirrored span); the right limit is that edge, or edge + span for runs cut by the left clip edge.
void DrawZoomedMirror(int x, int y, int width, int height) {
    const std::int32_t zoomX = G.zoomX;
    const std::int32_t zoomY = G.zoomY;
    const std::int32_t right = (width - G.zoomAnchorX) * zoomX / 1024 + G.zoomAnchorX + x;
    std::int32_t row = G.zoomAnchorY - G.zoomAnchorY * zoomY / 1024 + y;
    if (right - width * zoomX / 1024 >= G.clipX + G.clipWidth) {
        return;
    }
    if (right < G.clipX) {
        return;
    }
    if (height * zoomY / 1024 + row >= G.clipY + G.clipHeight) {
        return;
    }
    if (row < G.clipY) {
        return;
    }
    const std::int32_t halfPitch = G.screenPitch / 2;
    const uptr clipTop = addr(G.drawSurface) + 2 * (G.clipY * halfPitch);
    std::uint8_t* rowClipLeft = G.drawSurface + 2 * (row * halfPitch + G.clipX);
    std::uint8_t* rowClipRight = rowClipLeft + 2 * G.clipWidth;
    std::uint8_t* rowDest = G.drawSurface + 2 * (G.screenPitch * row / 2 + right);
    const std::uint8_t* line = G.drawSource + 8;
    std::int32_t rowFrac = 0;
    if (u16(line) != 0) {
        for (;;) {
            rowFrac += zoomY;
            bool clipped = false;
            while (rowFrac >= 0x400) {
                if (addr(rowClipLeft) >= clipTop) {
                    clipped = true;
                    break;
                }
                rowFrac -= 0x400;
                ++row;
                rowClipLeft += 2 * halfPitch;
                rowClipRight = rowClipLeft + 2 * G.clipWidth;
                rowDest += 2 * halfPitch;
            }
            if (clipped) {
                break;
            }
            line += u16(line) & 0x3FFF;
            if (u16(line) == 0) {
                break;
            }
        }
    }
    rowFrac %= 0x400;
    if (u16(line) == 0) {
        return;
    }
    for (;;) {
        rowFrac += zoomY;
        while (rowFrac >= 0x400) {
            rowFrac -= 0x400;
            if (row >= G.clipY + G.clipHeight) {
                break;
            }
            std::uint8_t* dest = rowDest;
            const std::uint8_t* code = line + 2;
            std::int32_t colFrac = 0;
            std::uint16_t c = u16(code);
            while (c != 0) {
                const std::int32_t n = c & 0x3FFF;
                const std::uint8_t* runCode = code;
                const std::int32_t scaled = zoomX * n / 1024;
                if (!(c & 0xC000)) {
                    code += 2;
                }
                const std::int32_t span = 2 * scaled;
                std::uint8_t* next = dest - span;
                const std::int32_t fromLeft = static_cast<std::int32_t>(next - rowClipLeft) >> 1;
                const std::int32_t over = static_cast<std::int32_t>(dest - rowClipRight) >> 1;
                uptr lower;
                uptr upper;
                bool draw = true;
                if (fromLeft < 0) {
                    lower = addr(rowClipLeft);
                    upper = addr(dest) + span;
                    if (fromLeft + scaled <= 0) {
                        draw = false;
                    } else if (over > 0) {
                        upper += 2 * static_cast<std::uint32_t>(-1 - over);
                    }
                } else if (fromLeft >= G.clipWidth) {
                    draw = false;
                } else {
                    lower = addr(next);
                    upper = addr(dest);
                    if (over > 0) {
                        upper -= 2 * static_cast<std::uint32_t>(over) + 2;
                    }
                }
                if (draw) {
                    if (u16(runCode) & 0xC000) {
                        colFrac += zoomX * scaled;
                    } else {
                        std::uint8_t* out = dest;
                        const std::uint8_t* pixel = code;
                        colFrac %= 0x400;
                        do {
                            colFrac += zoomX;
                            bool stop = false;
                            while (colFrac >= 0x400) {
                                colFrac -= 0x400;
                                if (addr(out) < lower) {
                                    stop = true;
                                    break;
                                }
                                if (addr(out) <= upper) {
                                    put16(out, u16(pixel));
                                }
                                out -= 2;
                            }
                            if (stop) {
                                break;
                            }
                            pixel += 2;
                        } while (addr(pixel) < addr(code) + 2 * static_cast<std::uint32_t>(n));
                    }
                }
                dest = next;
                const std::uint16_t runHeader = u16(runCode);
                if (runHeader & 0xC000) {
                    code += 2;
                } else {
                    code += 2 * (runHeader & 0x3FFF);
                }
                c = u16(code);
            }
            ++row;
            const std::int32_t rowStep = G.screenPitch / 2 * 2;
            rowClipLeft += rowStep;
            rowClipRight = rowClipLeft + 2 * G.clipWidth;
            rowDest += rowStep;
        }
        line += u16(line) & 0x3FFF;
        if (u16(line) == 0) {
            return;
        }
    }
}

}  // namespace swd3
