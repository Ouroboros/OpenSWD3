#include "draw/draw.h"

#include <cstring>

#include "data/code_ptr.h"
#include "data/globals.h"
#include "draw/blit.h"
#include "pic/pixel_format.h"

namespace swd3 {
namespace {

std::uint16_t readU16(const std::uint8_t* p) {
    std::uint16_t v;
    std::memcpy(&v, p, 2);
    return v;
}

void writeU16(std::uint8_t* p, std::uint16_t v) { std::memcpy(p, &v, 2); }

std::uint32_t readU32(const std::uint8_t* p) {
    std::uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

void writeU32(std::uint8_t* p, std::uint32_t v) { std::memcpy(p, &v, 4); }

int halfPitch() {
    const int pitch = G.screenPitch;
    return pitch / 2;
}

}  // namespace

// swd32.exe 0x00416D30
int DrawInit(w32::IDirectDrawSurface4* surface) {
    if (!PixelFormatInit(surface)) {
        return 0;
    }
    const int height = G.screenHeight;
    G.clipWidth = G.screenWidth;
    G.clipX = 0;
    G.clipY = 0;
    G.clipHeight = height;
    std::int32_t offset = 0;
    for (int y = 0; y < height; ++y) {
        G.rowOffsets[y] = offset;
        offset += G.screenPitch;
    }
    DrawTableInit();
    return 1;
}

// swd32.exe 0x00416D90
void DrawTableInit() {
    auto set = [](code_ptr& slot, BlitFn* fn) { slot = codeOf(fn); };
    code_ptr* rle = G.drawRleTable;
    set(rle[0], &BlitRle_418350);
    set(rle[1], &BlitRle_4185C0);
    set(rle[2], &BlitRle_418350);
    set(rle[3], &BlitRle_4185C0);
    set(rle[0xC], &BlitRle_41F8D0);
    set(rle[0xD], &BlitRle_41FEA0);
    set(rle[0xE], &BlitRle_41F8D0);
    set(rle[0xF], &BlitRle_41FEA0);
    set(rle[0x14], &BlitRle_41D340);
    set(rle[0x15], &BlitRle_41E5C0);
    set(rle[0x16], &BlitRle_41D340);
    set(rle[0x17], &BlitRle_41E5C0);
    set(rle[0x24], &BlitRle_421230);
    set(rle[0x25], &BlitRle_421540);
    set(rle[0x26], &BlitRle_421230);
    set(rle[0x27], &BlitRle_421540);
    set(rle[0x28], &BlitRle_421850);
    set(rle[0x29], &BlitRle_421BE0);
    set(rle[0x2A], &BlitRle_421850);
    set(rle[0x2B], &BlitRle_421BE0);
    set(rle[4], &BlitRle_418840);
    set(rle[5], &BlitRle_418EB0);
    set(rle[8], &BlitRle_419570);
    set(rle[9], &BlitRle_41A3B0);
    set(rle[0x10], &BlitRle_41B280);
    set(rle[0x11], &BlitRle_41B620);
    set(rle[0x18], &BlitRle_41CCF0);
    set(rle[0x19], &BlitRle_41D010);
    set(rle[0x1C], &BlitRle_41B9F0);
    set(rle[0x20], &BlitRle_4208D0);
    set(rle[0x21], &BlitRle_420D70);
    set(rle[0x2C], &BlitRle_422030);
    set(rle[0x2D], &BlitRle_4223A0);
    set(rle[0x30], &BlitRle_422730);
    set(rle[0x31], &BlitRle_4229C0);
    set(rle[0x32], &BlitRle_422730);
    set(rle[0x33], &BlitRle_4229C0);
    code_ptr* raw = G.drawRawTable;
    set(raw[0], &BlitRawCopy);
    set(raw[1], &BlitRawCopyMirror);
    set(raw[4], &BlitRawKeyed);
    set(raw[5], &BlitRawKeyedMirror);
    set(raw[8], &BlitRaw_417EC0);
    set(raw[0x14], &BlitRaw_417950);
}

// swd32.exe 0x00416F10
std::uint8_t* LockSurface(w32::IDirectDrawSurface4* surface) {
    w32::DDSURFACEDESC2 desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.dwSize = 0x7C;
    if (surface->Lock(nullptr, &desc, w32::DDLOCK_WAIT, nullptr) != 0) {
        return nullptr;
    }
    G.surfacePitchPixels = desc.lPitch >> 1;
    return static_cast<std::uint8_t*>(desc.lpSurface);
}

// swd32.exe 0x00416F60
w32::HRESULT UnlockSurface(w32::IDirectDrawSurface4* surface, void* memory) { return surface->Unlock(memory); }

// swd32.exe 0x00416F80
int SetClipRect(const w32::RECT* rect) {
    if (rect->left < 0 || rect->top < 0 || rect->right > G.screenWidth || rect->bottom > G.screenHeight) {
        return 0;
    }
    G.clipX = rect->left;
    G.clipY = rect->top;
    G.clipWidth = rect->right - rect->left;
    G.clipHeight = rect->bottom - rect->top;
    return 1;
}

// swd32.exe 0x00416FF0
int SetClipRectClamped(int left, int top, int right, int bottom) {
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right > G.screenWidth) {
        right = G.screenWidth;
    }
    if (bottom > G.screenHeight) {
        bottom = G.screenHeight;
    }
    G.clipX = left;
    G.clipHeight = bottom - top;
    G.clipY = top;
    G.clipWidth = right - left;
    return 1;
}

// swd32.exe 0x00417050: picture drawn at the four diagonal neighbours (mode bits | 0x24)
void DrawOutline(int x, int y, int width, int height, int flags, int arg) {
    flags |= 0x24;
    DrawPicture(x + 1, y + 1, width, height, flags, arg);
    DrawPicture(x - 1, y - 1, width, height, flags, arg);
    DrawPicture(x - 1, y + 1, width, height, flags, arg);
    DrawPicture(x + 1, y - 1, width, height, flags, arg);
}

// swd32.exe 0x004170E0
int DrawPicture(int x, int y, int width, int height, int flags, int arg) {
    int result = 0;
    int bottomSkip = 0;
    std::uint32_t uflags = static_cast<std::uint32_t>(flags);
    int dy;
    int dx;
    int rows;
    if (readU16(G.drawSource) == 0xFFFF) {
        if (G.drawPalette == nullptr) {
            uflags |= 0x80000000u;
        }
    }
    if ((uflags & 0xFFFC) == 0x14) {
        if (G.drawAlpha <= 0) {
            goto finish;
        }
        if (G.drawAlpha > 0xF) {
            uflags &= 0x80000003u;
        }
    }
    rows = G.drawScaleHeight;
    G.drawScaleFrac = 0;
    G.drawShearStep = 0;
    if (rows == 0) {
        dy = y;
        rows = height;
        G.drawSkipTop = 0;
        G.drawSkipBottom = 0;
        G.drawRows = rows;
        if (G.clipY > y) {
            rows = y - G.clipY + height;
            G.drawRows = rows;
            if (rows <= 0) {
                goto finish;
            }
            G.drawSkipTop = height - rows;
            dy = G.clipY;
        }
        if (rows + dy >= G.clipHeight + G.clipY) {
            bottomSkip = dy - G.clipHeight - G.clipY + height;
            rows = G.clipHeight - dy + G.clipY;
            G.drawSkipBottom = bottomSkip;
            G.drawRows = rows;
            if (rows <= 0) {
                goto finishAfterCall;
            }
        }
        if (G.drawShear != 0) {
            goto shear;
        }
        {
            dx = x;
            G.drawSkipLeft = 0;
            int visible = width;
            G.drawWidth = width;
            if (G.clipX > x) {
                visible = x - G.clipX + width;
                G.drawWidth = visible;
                if (visible <= 0) {
                    goto finishAfterCall;
                }
                G.drawSkipLeft = width - visible;
                dx = G.clipX;
            }
            if (visible + dx >= G.clipWidth + G.clipX) {
                visible = G.clipWidth - dx + G.clipX;
                G.drawWidth = visible;
                if (visible <= 0) {
                    goto finishAfterCall;
                }
            }
            goto dispatch;
        }
    }
    G.drawScaleUp = 0;
    if (rows > height) {
        G.drawScaleFrac = (height << 10) / rows;
        G.drawScaleUp = 1;
    } else {
        G.drawScaleFrac = (height << 10) / rows - 0x400;
    }
    dy = y;
    G.drawRows = rows;
    G.drawSkipTop = 0;
    G.drawSkipBottom = 0;
    if (G.clipY > y) {
        rows = rows - G.clipY + y - 1;
        G.drawRows = rows;
        if (rows <= 0) {
            goto finish;
        }
        G.drawSkipTop = G.clipY - y;
        dy = G.clipY;
    }
    if (rows + dy >= G.clipHeight + G.clipY) {
        bottomSkip = rows - G.clipHeight - G.clipY + dy;
        rows = G.clipHeight - dy + G.clipY - 1;
        G.drawSkipBottom = bottomSkip;
        G.drawRows = rows;
        if (rows <= 0) {
            goto finishAfterCall;
        }
    }
    if (G.drawShear == 0) {
        G.drawShear = 1;
    }
shear : {
    const int shear = G.drawShear;
    G.drawShearDir = 1;
    if (shear > 0) {
        G.drawShearDir = -1;
    }
    G.drawSkipLeft = 0;
    int visible = width;
    G.drawWidth = width;
    const int magnitude = shear < 0 ? -shear : shear;
    const int span = G.drawScaleHeight != 0 ? G.drawScaleHeight : height;
    G.drawShearStep = (magnitude << 10) / span;
    dx = x;
    if (G.clipX > x) {
        visible = x - G.clipX + width;
        G.drawWidth = visible;
        if (visible <= 0) {
            goto finishAfterCall;
        }
        G.drawSkipLeft = G.clipX - x;
        dx = G.clipX;
    }
    if (visible + dx >= G.clipWidth + G.clipX) {
        visible = G.clipWidth - dx + G.clipX;
        G.drawWidth = visible;
        if (visible <= 0) {
            goto finishAfterCall;
        }
    }
}
dispatch : {
    std::uint8_t* dest = G.drawSurface + G.rowOffsets[dy] + dx * 2;
    G.drawDest = dest;
    std::uint8_t* destEnd = dest + G.rowOffsets[rows];
    G.drawDestEnd = destEnd;
    G.drawPitch = G.screenPitch;
    int skip;
    if (uflags & 2) {
        G.drawSkipTop = bottomSkip;
        skip = bottomSkip;
        G.drawPitch = -G.screenPitch;
        G.drawDest = destEnd;
    } else {
        skip = G.drawSkipTop;
    }
    const std::uint32_t rle = uflags & 0x80000000u;
    if (rle == 0 && (uflags & 1)) {
        G.drawSkipLeft = width - G.drawSkipLeft - 1;
    }
    if (rle == 0) {
        if (G.drawPalette == nullptr) {
            G.drawSourceLine = G.drawSource + (skip * width + G.drawSkipLeft) * 2;
        } else {
            G.drawSourceLine = G.drawSource + skip * width + G.drawSkipLeft;
        }
    }
    G.drawDitherPhase = (dy + 0x1E0) % 3;
    const std::uint32_t index = (uflags & 0xFFFF) + (rle ? 0 : 0x80);
    result = codeAs<BlitFn>(G.drawRleTable[index])(dx, dy, width, height, arg);
}
finishAfterCall:
finish:
    G.drawOption300 = 0;
    G.drawOption304 = 0;
    G.drawOption30C = 0;
    G.drawOption71C = 0;
    G.drawOptionBE0 = 0;
    G.drawScaleHeight = 0;
    G.drawShear = 0;
    G.drawAlpha = 0;
    return result;
}

// swd32.exe 0x004174D0: 16x16 tile of 16-bit pixels, no clipping
int DrawTile16(int x, int y) {
    std::uint8_t* dest = G.drawSurface + x * 2 + G.rowOffsets[y];
    const std::uint8_t* src = G.drawSource;
    for (int row = 0; row < 0x10; ++row) {
        for (int i = 0x20; i != 0; i -= 8) {
            const std::uint32_t a = readU32(src + i - 4);
            const std::uint32_t b = readU32(src + i - 8);
            writeU32(dest + i - 4, a);
            writeU32(dest + i - 8, b);
        }
        src += 0x20;
        dest += G.screenPitch;
    }
    return 0;
}

// swd32.exe 0x00417530: 16x16 tile of 16-bit pixels, fill colour transparent
void DrawTile16Keyed(int x, int y) {
    const int skip = G.screenPitch - 0x20;
    std::uint8_t* dest = G.drawSurface + x * 2 + G.rowOffsets[y];
    const std::uint8_t* src = G.drawSource;
    const std::uint32_t key = static_cast<std::uint32_t>(G.fillColorPair) & 0xFFFF;
    for (int row = 0x10; row != 0; --row) {
        for (int i = 0x10; i != 0; --i) {
            const std::uint16_t v = readU16(src);
            src += 2;
            dest += 2;
            if (v != key) {
                writeU16(dest - 2, v);
            }
        }
        dest += skip;
    }
}

// swd32.exe 0x004175B0: 16x16 tile of palette indices
void DrawTile8(int x, int y) {
    std::uint8_t* dest = G.drawSurface + G.rowOffsets[y] + x * 2;
    const std::uint8_t* src = G.drawSource;
    const std::uint16_t* palette = G.drawPalette;
    const int skip = halfPitch() - 0x10;
    for (int row = 0x10; row != 0; --row) {
        for (int quad = 4; quad != 0; --quad) {
            for (int k = 0; k < 4; ++k) {
                writeU16(dest, palette[*src++]);
                dest += 2;
            }
        }
        dest += skip * 2;
    }
}

// swd32.exe 0x00417650: 16x16 tile of palette indices, index 1 transparent
void DrawTile8Keyed(int x, int y) {
    std::uint8_t* dest = G.drawSurface + G.rowOffsets[y] + x * 2;
    const std::uint8_t* src = G.drawSource;
    const std::uint16_t* palette = G.drawPalette;
    const int skip = halfPitch() - 0x10;
    for (int row = 0x10; row != 0; --row) {
        for (int pair = 8; pair != 0; --pair) {
            dest += 4;
            std::uint32_t index = *src++;
            if (index != 1) {
                writeU16(dest - 4, palette[index]);
            }
            index = *src++;
            if (index != 1) {
                writeU16(dest - 2, palette[index]);
            }
        }
        dest += skip * 2;
    }
}

// swd32.exe 0x004176D0
int BlitRawCopy(int, int, int width, int, int) {
    if (G.drawPalette == nullptr) {
        const int visible = G.drawWidth;
        G.drawLineSkip = G.screenPitch - visible - visible;
        G.drawSourceSkip = (width - visible) * 2;
        const std::uint8_t* src = G.drawSourceLine;
        std::uint8_t* dest = G.drawDest;
        int rows = G.drawRows;
        do {
            int n = visible;
            do {
                writeU16(dest, readU16(src));
                src += 2;
                dest += 2;
            } while (--n != 0);
            src += G.drawSourceSkip;
            dest += G.drawLineSkip;
        } while (--rows != 0);
        return 0;
    }
    const std::uint16_t* palette = G.drawPalette;
    const std::uint8_t* src = G.drawSourceLine;
    std::uint8_t* dest = G.drawDest;
    const int sourceSkip = width - G.drawWidth;
    const int destSkip = halfPitch() - G.drawWidth;
    for (int row = 0; row < G.drawRows; ++row) {
        for (int i = 0; i < G.drawWidth; ++i) {
            dest += 2;
            writeU16(dest - 2, palette[*src++]);
        }
        src += sourceSkip;
        dest += destSkip * 2;
    }
    return 0;
}

// swd32.exe 0x004177D0
int BlitRawCopyMirror(int, int, int width, int, int) {
    const int visible = G.drawWidth;
    G.drawLineSkip = G.screenPitch - visible - visible;
    G.drawSourceSkip = (width + visible) * 2;
    const std::uint8_t* src = G.drawSourceLine;
    std::uint8_t* dest = G.drawDest;
    int rows = G.drawRows;
    do {
        int n = visible;
        do {
            const std::uint16_t v = readU16(src);
            src -= 2;
            dest += 2;
            writeU16(dest - 2, v);
        } while (--n != 0);
        src += G.drawSourceSkip;
        dest += G.drawLineSkip;
    } while (--rows != 0);
    return 0;
}

// swd32.exe 0x00417840
int BlitRawKeyed(int, int, int width, int, int) {
    if (G.drawPalette == nullptr) {
        const int visible = G.drawWidth;
        G.drawLineSkip = G.screenPitch - visible - visible;
        G.drawSourceSkip = (width - visible) * 2;
        const std::uint8_t* src = G.drawSourceLine;
        std::uint8_t* dest = G.drawDest;
        const std::uint32_t key = static_cast<std::uint32_t>(G.fillColorPair) & 0xFFFF;
        int rows = G.drawRows;
        do {
            int n = G.drawWidth;
            do {
                const std::uint32_t v = readU16(src);
                --n;
                src += 2;
                dest += 2;
                if (v != key) {
                    writeU16(dest - 2, static_cast<std::uint16_t>(v));
                }
            } while (n != 0);
            src += G.drawSourceSkip;
            dest += G.drawLineSkip;
        } while (--rows != 0);
        return 0;
    }
    const std::uint16_t* palette = G.drawPalette;
    const std::uint8_t* src = G.drawSourceLine;
    std::uint8_t* dest = G.drawDest;
    const int destSkip = halfPitch() - G.drawWidth;
    const int sourceSkip = width - G.drawWidth;
    for (int row = 0; row < G.drawRows; ++row) {
        for (int i = 0; i < G.drawWidth; ++i) {
            dest += 2;
            const std::uint32_t index = *src++;
            if (index != 1) {
                writeU16(dest - 2, palette[index]);
            }
        }
        src += sourceSkip;
        dest += destSkip * 2;
    }
    return 0;
}

// swd32.exe 0x00417DE0: pairs = pairs/2 + color/4 + pairs/8 + pairs/16 (count halved, at least one pass)
void BlendPairs(std::uint32_t* pairs, std::uint32_t color, std::uint32_t count) {
    std::uint32_t n = count >> 1;
    const std::uint32_t m1 = static_cast<std::uint32_t>(G.halfMaskPairs[0]);
    const std::uint32_t m2 = static_cast<std::uint32_t>(G.halfMaskPairs[1]);
    do {
        const std::uint32_t p = *pairs;
        *pairs = ((p >> 1) & m1) + ((color >> 2) & m2) + ((p >> 3) & static_cast<std::uint32_t>(G.halfMaskPairs[2])) +
                 ((p >> 4) & static_cast<std::uint32_t>(G.halfMaskPairs[3]));
        ++pairs;
    } while (--n != 0);
}

// swd32.exe 0x00417E40: the key is compared with the whole fill colour pair, so no pixel is ever skipped
int BlitRawKeyedMirror(int, int, int width, int, int) {
    const int visible = G.drawWidth;
    G.drawLineSkip = G.screenPitch - visible - visible;
    G.drawSourceSkip = (width + visible) * 2;
    const std::uint8_t* src = G.drawSourceLine;
    std::uint8_t* dest = G.drawDest;
    const std::uint32_t key = static_cast<std::uint32_t>(G.fillColorPair);
    int rows = G.drawRows;
    do {
        int n = G.drawWidth;
        do {
            const std::uint32_t v = readU16(src);
            src -= 2;
            dest += 2;
            --n;
            if (v != key) {
                writeU16(dest - 2, static_cast<std::uint16_t>(v));
            }
        } while (n != 0);
        src += G.drawSourceSkip;
        dest += G.drawLineSkip;
    } while (--rows != 0);
    return 0;
}

}  // namespace swd3
