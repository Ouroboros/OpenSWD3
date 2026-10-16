#include "ui/ui_draw.h"

#include "data/globals.h"
#include "draw/draw.h"
#include "draw/pixel_ops.h"
#include "pic/pixel_format.h"
#include "res/tsw.h"

namespace swd3 {

// swd32.exe 0x0043B110: the box is clipped to the clip rectangle first; the return value is unused by callers
void FillBoxMode(int x, int y, int width, int height, int red, int green, int blue, int mode) {
    if (x < G.clipX) {
        width = x - G.clipX + width;
        x = G.clipX;
    }
    if (y < G.clipY) {
        height = y - G.clipY + height;
        y = G.clipY;
    }
    if (x + width > G.clipWidth + G.clipX) {
        width = G.clipX + G.clipWidth - x;
    }
    if (y + height > G.clipHeight + G.clipY) {
        height = G.clipY + G.clipHeight - y;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    const std::uint32_t color = MakeColor(static_cast<std::uint32_t>(red), static_cast<std::uint32_t>(green),
                                          static_cast<std::uint32_t>(blue));
    const int pitchPairs = G.screenPitch >> 2;
    const int pitchPixels = G.screenPitch >> 1;
    auto* surface = reinterpret_cast<std::uint16_t*>(G.drawSurface);
    switch (mode) {
        case kFillBlend: {
            std::uint16_t* row = surface + x + y * pitchPixels;
            do {
                BlendPairs(reinterpret_cast<std::uint32_t*>(row), color, static_cast<std::uint32_t>(width));
                --height;
                row += G.screenPitch >> 1;
            } while (height != 0);
            break;
        }
        case kFillTint:
            for (int line = 0; line < height; ++line) {
                TintPixels(surface + x + (G.screenPitch >> 1) * (y + line), static_cast<std::uint32_t>(width), red,
                           green, blue);
            }
            break;
        case kFillQuarter:
        case kFillEighth: {
            const int shift = mode == kFillQuarter ? 2 : 3;
            const auto mask = static_cast<std::uint32_t>(G.halfMaskPairs[shift - 1]);
            auto* pairs = reinterpret_cast<std::uint32_t*>(surface + x + y * pitchPixels);
            const int count = width / 2;
            const int skip = pitchPairs - count;
            do {
                for (int i = 0; i < count; ++i) {
                    *pairs = mask & (*pairs >> shift);
                    ++pairs;
                }
                --height;
                pairs += skip;
            } while (height != 0);
            break;
        }
        case kFillGray: {
            int offset = x + y * pitchPixels;
            do {
                GrayPixels(surface + offset, static_cast<std::uint32_t>(width));
                offset += pitchPixels;
                --height;
            } while (height != 0);
            break;
        }
        case kFillGradient: {
            std::uint16_t* top = surface + x + y * pitchPixels;
            std::uint16_t* bottom = top + pitchPixels * (height - 1);
            const int steps = (height + 1) / 2;
            if (steps > 0) {
                int redLevel = 0;
                int greenLevel = 0;
                int blueLevel = 0;
                const int blueStep = (blue << 10) / steps;
                for (int i = steps; i != 0; --i) {
                    TintPixels(top, static_cast<std::uint32_t>(width), redLevel / 0x400, greenLevel / 0x400,
                               blueLevel / 0x400);
                    TintPixels(bottom, static_cast<std::uint32_t>(width), redLevel / 0x400, greenLevel / 0x400,
                               blueLevel / 0x400);
                    bottom -= pitchPixels;
                    greenLevel += (green << 10) / steps;
                    top += pitchPixels;
                    redLevel += (red << 10) / steps;
                    blueLevel += blueStep;
                }
            }
            break;
        }
        default:
            break;
    }
}

namespace {

Picture* framePiece(int id, int frame) {
    Picture* picture = TswGetPicture(static_cast<std::uint32_t>(id), static_cast<std::uint32_t>(frame));
    G.drawSource = reinterpret_cast<std::uint8_t*>(picture->pixels);
    return picture;
}

}  // namespace

// swd32.exe 0x0042E850: frame of picture `id` around (left, top)-(right, bottom): frames 0..8 are the corners and
// edges (0 1 2 / 3 4 5 / 6 7 8, 4 the tiled body), `border` the frame thickness. Bit 31 of `border` skips the body,
// a non-zero `alpha` draws translucent; frame 9 of picture 0x234A is drawn at the right edge as well. The return
// value is unused by callers.
void DrawWindowFrame(int id, int left, int top, int right, int bottom, int alpha, int border) {
    const bool frameOnly = border < 0;
    const int size = border & 0x7FFFFFFF;
    const int mode = alpha != 0 ? 0x14 : 0;
    G.drawAlpha = alpha;
    if (!frameOnly) {
        if (static_cast<std::uint16_t>(id) == 0x234A) {
            Picture* tag = TswGetPicture(0x234A, 9);
            G.drawSource = reinterpret_cast<std::uint8_t*>(tag->pixels);
            G.drawAlpha = alpha;
            DrawPicture(right + tag->width - size, ((bottom + top) >> 1) - 0xC, tag->width, tag->height, mode, 0);
        }
        SetClipRectClamped(left, top, right, bottom);
        Picture* body = framePiece(id, 4);
        for (int y = top; y < bottom; y += body->height) {
            for (int x = left; x < right; x += body->width) {
                G.drawAlpha = alpha;
                DrawPicture(x, y, body->width, body->height, mode, 0);
            }
        }
    }
    const int outerTop = top - size;
    const int outerLeft = left - size;
    SetClipRectClamped(outerLeft, outerTop, left, top);
    Picture* piece = framePiece(id, 0);
    G.drawAlpha = alpha;
    DrawPicture(outerLeft, outerTop, piece->width, piece->height, mode, 0);
    SetClipRectClamped(left, outerTop, right, top);
    framePiece(id, 1);
    for (int x = left; x < right; x += piece->width) {
        piece = framePiece(id, 1);
        G.drawAlpha = alpha;
        DrawPicture(x, outerTop, piece->width, piece->height, mode, 0);
    }
    SetClipRectClamped(right, outerTop, right + size, top);
    piece = framePiece(id, 2);
    G.drawAlpha = alpha;
    DrawPicture(size + right - piece->width, outerTop, piece->width, piece->height, mode, 0);
    for (int y = top; y < bottom; y += piece->height) {
        SetClipRectClamped(outerLeft, top, left, bottom);
        piece = framePiece(id, 3);
        G.drawAlpha = alpha;
        DrawPicture(outerLeft, y, piece->width, piece->height, mode, 0);
        SetClipRectClamped(right, top, right + size, bottom);
        piece = framePiece(id, 5);
        G.drawAlpha = alpha;
        DrawPicture(size + right - piece->width, y, piece->width, piece->height, mode, 0);
    }
    SetClipRectClamped(outerLeft, bottom, left, bottom + size);
    piece = framePiece(id, 6);
    G.drawAlpha = alpha;
    DrawPicture(outerLeft, size + bottom - piece->height, piece->width, piece->height, mode, 0);
    SetClipRectClamped(left, bottom, right, bottom + size);
    piece = framePiece(id, 7);
    const int edgeY = size + bottom - piece->height;
    for (int x = left; x < right; x += piece->width) {
        G.drawAlpha = alpha;
        DrawPicture(x, edgeY, piece->width, piece->height, mode, 0);
    }
    SetClipRectClamped(right, bottom, right + size, bottom + size);
    piece = framePiece(id, 8);
    G.drawAlpha = alpha;
    DrawPicture(size + right - piece->width, size + bottom - piece->height, piece->width, piece->height, mode, 0);
    SetClipRectClamped(0, 0, 0x280, 0x1E0);
}

}  // namespace swd3
