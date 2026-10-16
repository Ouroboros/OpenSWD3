#include "ui/ui_draw.h"

#include <span>

#include "core/words.h"
#include "draw/painter.h"
#include "pic/picture.h"
#include "platform/display.h"
#include "res/tsw_archive.h"

namespace swd3
{

namespace
{

constexpr u32 kFramePicture = 0x234A;
constexpr int kTagFrame = 9;
constexpr int kTopLeftFrame = 0;  // the frames of a window picture, row by row
constexpr int kTopFrame = 1;
constexpr int kTopRightFrame = 2;
constexpr int kLeftFrame = 3;
constexpr int kBodyFrame = 4;
constexpr int kRightFrame = 5;
constexpr int kBottomLeftFrame = 6;
constexpr int kBottomFrame = 7;
constexpr int kBottomRightFrame = 8;
constexpr int kThicknessMask = 0x7FFFFFFF;  // `border` without its frame-only bit
constexpr int kTagRise = 0xC;
constexpr int kQuarterShift = 2;
constexpr int kEighthShift = 3;
constexpr int kPixelsPerPair = 2;
constexpr int kLevelBits = 10;  // the gradient levels are fixed point
constexpr int kLevelUnit = 1 << kLevelBits;
constexpr u32 kTranslucent = Painter::Translucent;
constexpr int kScreenWidth = Display::Width;
constexpr int kScreenHeight = Display::Height;

// Blends `pairs` pixel pairs from `row` with the colour, two pixels as one 32-bit pair as the original walks them.
void BlendPairs(const Painter& painter, u16* row, int pairs, u32 color)
{
    for (int i = 0; i < pairs; ++i)
    {
        u32 pair = row[2 * i] | (static_cast<u32>(row[2 * i + 1]) << 16);
        painter.BlendPairsWithColor(std::span<u32>(&pair, 1), color);
        row[2 * i] = static_cast<u16>(pair);
        row[2 * i + 1] = static_cast<u16>(pair >> 16);
    }
}

// Picture `frame` of `id` as the source; nullptr when the archive has none.
const Picture* Piece(Painter& painter, TswArchive& archive, u32 id, int frame)
{
    const Picture* picture = archive.GetPicture(id, static_cast<u32>(frame));

    if (picture != nullptr)
    {
        painter.SetSource(picture->data.data());
    }

    return picture;
}

void Draw(Painter& painter, const Picture* piece, int x, int y, int alpha)
{
    if (piece == nullptr)
    {
        return;
    }

    painter.Options().alpha = alpha;
    painter.DrawPicture(x, y, piece->width, piece->height, alpha != 0 ? kTranslucent : 0);
}

}  // namespace

// swd32.exe 0x0043B110: the box is cut to the clip rectangle first. The blend walks width / 2 pixel pairs in a loop
// that runs at least once and wraps around for a width below 2 (writing on until it faults); here it then blends the
// first pair only.
void FillBox(Painter& painter, int x, int y, int width, int height, int red, int green, int blue, FillMode mode)
{
    const auto [clipX, clipY, clipWidth, clipHeight] = painter.ClipRect();

    if (x < clipX)
    {
        width = x - clipX + width;
        x = clipX;
    }

    if (y < clipY)
    {
        height = y - clipY + height;
        y = clipY;
    }

    if (x + width > clipWidth + clipX)
    {
        width = clipX + clipWidth - x;
    }

    if (y + height > clipHeight + clipY)
    {
        height = clipY + clipHeight - y;
    }

    if (width <= 0 || height <= 0)
    {
        return;
    }

    const u32 color = painter.MakeColor(static_cast<u32>(red), static_cast<u32>(green), static_cast<u32>(blue));
    const int pitch = painter.ScreenPitch();
    u16* surface = painter.Target();
    u16* row = surface + x + y * pitch;

    switch (mode)
    {
        case FillMode::Blend:
            for (int line = 0; line < height; ++line, row += pitch)
            {
                BlendPairs(painter, row, width >> 1 != 0 ? width >> 1 : 1, color);
            }

            break;

        case FillMode::Tint:
            for (int line = 0; line < height; ++line, row += pitch)
            {
                painter.TintPixels(std::span<u16>(row, static_cast<std::size_t>(width)), red, green, blue);
            }

            break;

        case FillMode::Quarter:
        case FillMode::Eighth:
        {
            const int shift = mode == FillMode::Quarter ? kQuarterShift : kEighthShift;
            const u32 mask = painter.HalfMaskPair(shift - 1);

            for (int line = 0; line < height; ++line, row += pitch)
            {
                for (int i = 0; i < width / kPixelsPerPair; ++i)
                {
                    const u32 pair = MakeWords(row[kPixelsPerPair * i + 1], row[kPixelsPerPair * i]);
                    const u32 darker = mask & (pair >> shift);
                    row[kPixelsPerPair * i] = LowWord(darker);
                    row[kPixelsPerPair * i + 1] = HighWord(darker);
                }
            }

            break;
        }

        case FillMode::Gray:
            for (int line = 0; line < height; ++line, row += pitch)
            {
                painter.GrayPixels(std::span<u16>(row, static_cast<std::size_t>(width)));
            }

            break;

        case FillMode::Gradient:
        {
            u16* top = row;
            u16* bottom = top + pitch * (height - 1);
            const int steps = (height + 1) / 2;
            int redLevel = 0;
            int greenLevel = 0;
            int blueLevel = 0;

            for (int i = steps; i > 0; --i)
            {
                const std::size_t pixels = static_cast<std::size_t>(width);
                painter.TintPixels(
                    std::span<u16>(top, pixels),
                    redLevel / kLevelUnit,
                    greenLevel / kLevelUnit,
                    blueLevel / kLevelUnit
                );
                painter.TintPixels(
                    std::span<u16>(bottom, pixels),
                    redLevel / kLevelUnit,
                    greenLevel / kLevelUnit,
                    blueLevel / kLevelUnit
                );
                bottom -= pitch;
                top += pitch;
                redLevel += (red << kLevelBits) / steps;
                greenLevel += (green << kLevelBits) / steps;
                blueLevel += (blue << kLevelBits) / steps;
            }

            break;
        }
    }
}

// swd32.exe 0x0042E850: a frame of picture `id` around (left, top)-(right, bottom): frames 0..8 are the corners and
// edges (0 1 2 / 3 4 5 / 6 7 8, 4 the tiled body) and `border` the frame thickness. Bit 31 of `border` leaves out the
// body; a non-zero `alpha` draws translucent; picture 0x234A also gets frame 9 at the middle of its right edge.
void DrawWindowFrame(
    Painter& painter,
    TswArchive& archive,
    int id,
    int left,
    int top,
    int right,
    int bottom,
    int alpha,
    int border
)
{
    const bool frameOnly = border < 0;
    const int size = border & kThicknessMask;
    const auto picture = static_cast<u32>(id);

    if (!frameOnly)
    {
        if (static_cast<u16>(id) == kFramePicture)
        {
            const Picture* tag = Piece(painter, archive, kFramePicture, kTagFrame);

            if (tag != nullptr)
            {
                Draw(painter, tag, right + tag->width - size, ((bottom + top) >> 1) - kTagRise, alpha);
            }
        }

        painter.SetClipClamped(left, top, right, bottom);
        const Picture* body = Piece(painter, archive, picture, kBodyFrame);

        if (body != nullptr && body->width != 0 && body->height != 0)
        {
            for (int y = top; y < bottom; y += body->height)
            {
                for (int x = left; x < right; x += body->width)
                {
                    Draw(painter, body, x, y, alpha);
                }
            }
        }
    }

    const int outerTop = top - size;
    const int outerLeft = left - size;
    painter.SetClipClamped(outerLeft, outerTop, left, top);
    const Picture* piece = Piece(painter, archive, picture, kTopLeftFrame);
    Draw(painter, piece, outerLeft, outerTop, alpha);
    painter.SetClipClamped(left, outerTop, right, top);
    Piece(painter, archive, picture, kTopFrame);

    for (int x = left; piece != nullptr && piece->width != 0 && x < right; x += piece->width)
    {
        piece = Piece(painter, archive, picture, kTopFrame);
        Draw(painter, piece, x, outerTop, alpha);
    }

    painter.SetClipClamped(right, outerTop, right + size, top);
    piece = Piece(painter, archive, picture, kTopRightFrame);

    if (piece != nullptr)
    {
        Draw(painter, piece, size + right - piece->width, outerTop, alpha);
    }

    for (int y = top; piece != nullptr && piece->height != 0 && y < bottom; y += piece->height)
    {
        painter.SetClipClamped(outerLeft, top, left, bottom);
        piece = Piece(painter, archive, picture, kLeftFrame);
        Draw(painter, piece, outerLeft, y, alpha);
        painter.SetClipClamped(right, top, right + size, bottom);
        piece = Piece(painter, archive, picture, kRightFrame);

        if (piece != nullptr)
        {
            Draw(painter, piece, size + right - piece->width, y, alpha);
        }
    }

    painter.SetClipClamped(outerLeft, bottom, left, bottom + size);
    piece = Piece(painter, archive, picture, kBottomLeftFrame);

    if (piece != nullptr)
    {
        Draw(painter, piece, outerLeft, size + bottom - piece->height, alpha);
    }

    painter.SetClipClamped(left, bottom, right, bottom + size);
    piece = Piece(painter, archive, picture, kBottomFrame);

    if (piece != nullptr && piece->width != 0)
    {
        const int edgeY = size + bottom - piece->height;

        for (int x = left; x < right; x += piece->width)
        {
            Draw(painter, piece, x, edgeY, alpha);
        }
    }

    painter.SetClipClamped(right, bottom, right + size, bottom + size);
    piece = Piece(painter, archive, picture, kBottomRightFrame);

    if (piece != nullptr)
    {
        Draw(painter, piece, size + right - piece->width, size + bottom - piece->height, alpha);
    }

    painter.SetClipClamped(0, 0, kScreenWidth, kScreenHeight);
}

}  // namespace swd3
