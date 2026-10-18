#include <cstring>

#include "draw/paint_record.h"
#include "draw/painter.h"
#include "pic/rle.h"

namespace swd3
{

namespace
{

constexpr int kZoomUnit = 1 << Painter::FractionBits;  // the zoom scales are in 1/1024

u16 Word(const u8* bytes, std::ptrdiff_t index = 0)
{
    u16 value = 0;
    std::memcpy(&value, bytes + sizeof(value) * index, sizeof(value));
    return value;
}

}  // namespace

// swd32.exe 0x00422C70: the RLE source scaled by the zoom factors (1/1024) so that its anchor point stays in place;
// 0x4000 and 0x8000 runs are transparent. Source lines above the clip rectangle are consumed while the row is above
// it; the row remainder is then cut below 0x400, dropping pending repeats. Rows below the clip bottom drop the
// remaining repeats of their source line in the same way. Positions are tracked in pixels from the target start.
void Painter::DrawZoomed(int x, int y, int width, int height)
{
    if (PaintRecord* record = PaintRecord::Active(); record != nullptr)
    {
        record->Zoomed(*this, x, y, width, height, false);
    }

    const int scaleX = this->zoomX;
    const int scaleY = this->zoomY;
    const int left = this->zoomAnchorX - this->zoomAnchorX * scaleX / kZoomUnit + x;
    int row = this->zoomAnchorY - this->zoomAnchorY * scaleY / kZoomUnit + y;

    if (left >= this->clipX + this->clipWidth || width * scaleX / kZoomUnit + left < this->clipX)
    {
        return;
    }

    if (row >= this->clipY + this->clipHeight || height * scaleY / kZoomUnit + row < this->clipY)
    {
        return;
    }

    const std::ptrdiff_t pitch = this->screenPitch;
    const std::ptrdiff_t clipTop = this->clipY * pitch;
    std::ptrdiff_t rowClipLeft = row * pitch + this->clipX;
    std::ptrdiff_t rowClipRight = rowClipLeft + this->clipWidth;
    std::ptrdiff_t rowDest = row * pitch + left;
    const u8* line = this->source + RleHeader::HeaderBytes;
    int rowFraction = 0;

    if (Word(line) != 0)
    {
        bool reached = false;

        while (!reached)
        {
            rowFraction += scaleY;

            while (rowFraction >= kZoomUnit)
            {
                if (rowClipLeft >= clipTop)
                {
                    reached = true;
                    break;
                }

                rowFraction -= kZoomUnit;
                ++row;
                rowClipLeft += pitch;
                rowClipRight = rowClipLeft + this->clipWidth;
                rowDest += pitch;
            }

            if (reached)
            {
                break;
            }

            line += Word(line) & RleHeader::CountMask;

            if (Word(line) == 0)
            {
                break;
            }
        }
    }

    rowFraction %= kZoomUnit;

    if (Word(line) == 0)
    {
        return;
    }

    while (true)
    {
        rowFraction += scaleY;

        while (rowFraction >= kZoomUnit)
        {
            rowFraction -= kZoomUnit;

            if (row >= this->clipY + this->clipHeight)
            {
                break;
            }

            std::ptrdiff_t dest = rowDest;
            const u8* code = line + sizeof(u16);
            int columnFraction = 0;

            for (u16 value = Word(code); value != 0; value = Word(code))
            {
                const int count = value & RleHeader::CountMask;
                const bool literal = (value & RleHeader::KindMask) == 0;
                const int scaled = scaleX * count / kZoomUnit;
                const u8* pixels = code + sizeof(u16);
                const std::ptrdiff_t fromLeft = dest - rowClipLeft;
                const std::ptrdiff_t over = dest + scaled - rowClipRight;
                std::ptrdiff_t lower = 0;
                std::ptrdiff_t upper = 0;
                bool drawn = true;

                if (fromLeft < 0)
                {
                    lower = rowClipLeft;
                    upper = dest + scaled;

                    if (fromLeft + scaled <= 0)
                    {
                        drawn = false;
                    }
                    else if (over > 0)
                    {
                        upper += -1 - over;
                    }
                }
                else if (fromLeft >= this->clipWidth)
                {
                    drawn = false;
                }
                else
                {
                    lower = dest;
                    upper = dest + scaled;

                    if (over > 0)
                    {
                        upper += -1 - over;
                    }
                }

                if (drawn)
                {
                    if (!literal)
                    {
                        columnFraction += scaleX * scaled;
                    }
                    else
                    {
                        std::ptrdiff_t out = dest;
                        columnFraction %= kZoomUnit;
                        bool stopped = false;

                        for (int i = 0; i < count && !stopped; ++i)
                        {
                            columnFraction += scaleX;

                            while (columnFraction >= kZoomUnit)
                            {
                                columnFraction -= kZoomUnit;

                                if (out > upper)
                                {
                                    stopped = true;
                                    break;
                                }

                                if (out >= lower)
                                {
                                    this->target[out] = Word(pixels, i);
                                }

                                ++out;
                            }
                        }
                    }
                }

                dest += scaled;
                code += literal ? sizeof(u16) + sizeof(u16) * count : sizeof(u16);
            }

            ++row;
            rowClipLeft += pitch;
            rowClipRight = rowClipLeft + this->clipWidth;
            rowDest += pitch;
        }

        line += Word(line) & RleHeader::CountMask;

        if (Word(line) == 0)
        {
            return;
        }
    }
}

// swd32.exe 0x00423020: mirrored 0x00422C70. The anchor is mirrored ((width - anchor) scaled), and the picture is not
// drawn at all when its scaled bottom reaches the clip bottom or its top lies above the clip top. The pixels of a
// run are written from the current right edge itself leftwards (one pixel right of the mirrored span); the right
// limit is that edge, or edge + span for runs cut by the left clip edge.
void Painter::DrawZoomedMirrored(int x, int y, int width, int height)
{
    if (PaintRecord* record = PaintRecord::Active(); record != nullptr)
    {
        record->Zoomed(*this, x, y, width, height, true);
    }

    const int scaleX = this->zoomX;
    const int scaleY = this->zoomY;
    const int right = (width - this->zoomAnchorX) * scaleX / kZoomUnit + this->zoomAnchorX + x;
    int row = this->zoomAnchorY - this->zoomAnchorY * scaleY / kZoomUnit + y;

    if (right - width * scaleX / kZoomUnit >= this->clipX + this->clipWidth || right < this->clipX)
    {
        return;
    }

    if (height * scaleY / kZoomUnit + row >= this->clipY + this->clipHeight || row < this->clipY)
    {
        return;
    }

    const std::ptrdiff_t pitch = this->screenPitch;
    const std::ptrdiff_t clipTop = this->clipY * pitch;
    std::ptrdiff_t rowClipLeft = row * pitch + this->clipX;
    std::ptrdiff_t rowClipRight = rowClipLeft + this->clipWidth;
    std::ptrdiff_t rowDest = row * pitch + right;
    const u8* line = this->source + RleHeader::HeaderBytes;
    int rowFraction = 0;

    if (Word(line) != 0)
    {
        bool reached = false;

        while (!reached)
        {
            rowFraction += scaleY;

            while (rowFraction >= kZoomUnit)
            {
                if (rowClipLeft >= clipTop)
                {
                    reached = true;
                    break;
                }

                rowFraction -= kZoomUnit;
                ++row;
                rowClipLeft += pitch;
                rowClipRight = rowClipLeft + this->clipWidth;
                rowDest += pitch;
            }

            if (reached)
            {
                break;
            }

            line += Word(line) & RleHeader::CountMask;

            if (Word(line) == 0)
            {
                break;
            }
        }
    }

    rowFraction %= kZoomUnit;

    if (Word(line) == 0)
    {
        return;
    }

    while (true)
    {
        rowFraction += scaleY;

        while (rowFraction >= kZoomUnit)
        {
            rowFraction -= kZoomUnit;

            if (row >= this->clipY + this->clipHeight)
            {
                break;
            }

            std::ptrdiff_t dest = rowDest;
            const u8* code = line + sizeof(u16);
            int columnFraction = 0;

            for (u16 value = Word(code); value != 0; value = Word(code))
            {
                const int count = value & RleHeader::CountMask;
                const bool literal = (value & RleHeader::KindMask) == 0;
                const int scaled = scaleX * count / kZoomUnit;
                const u8* pixels = code + sizeof(u16);
                const std::ptrdiff_t next = dest - scaled;
                const std::ptrdiff_t fromLeft = next - rowClipLeft;
                const std::ptrdiff_t over = dest - rowClipRight;
                std::ptrdiff_t lower = 0;
                std::ptrdiff_t upper = 0;
                bool drawn = true;

                if (fromLeft < 0)
                {
                    lower = rowClipLeft;
                    upper = dest + scaled;

                    if (fromLeft + scaled <= 0)
                    {
                        drawn = false;
                    }
                    else if (over > 0)
                    {
                        upper += -1 - over;
                    }
                }
                else if (fromLeft >= this->clipWidth)
                {
                    drawn = false;
                }
                else
                {
                    lower = next;
                    upper = dest;

                    if (over > 0)
                    {
                        upper -= over + 1;
                    }
                }

                if (drawn)
                {
                    if (!literal)
                    {
                        columnFraction += scaleX * scaled;
                    }
                    else
                    {
                        std::ptrdiff_t out = dest;
                        columnFraction %= kZoomUnit;
                        bool stopped = false;

                        for (int i = 0; i < count && !stopped; ++i)
                        {
                            columnFraction += scaleX;

                            while (columnFraction >= kZoomUnit)
                            {
                                columnFraction -= kZoomUnit;

                                if (out < lower)
                                {
                                    stopped = true;
                                    break;
                                }

                                if (out <= upper)
                                {
                                    this->target[out] = Word(pixels, i);
                                }

                                --out;
                            }
                        }
                    }
                }

                dest = next;
                code += literal ? sizeof(u16) + sizeof(u16) * count : sizeof(u16);
            }

            ++row;
            rowClipLeft += pitch;
            rowClipRight = rowClipLeft + this->clipWidth;
            rowDest += pitch;
        }

        line += Word(line) & RleHeader::CountMask;

        if (Word(line) == 0)
        {
            return;
        }
    }
}

}  // namespace swd3
