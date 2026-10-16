#include <cstring>

#include "draw/painter.h"

namespace swd3
{

namespace
{

std::uint16_t Word(const std::uint8_t* bytes, std::ptrdiff_t index = 0)
{
    std::uint16_t value = 0;
    std::memcpy(&value, bytes + 2 * index, 2);
    return value;
}

}  // namespace

// swd32.exe 0x00422C70: the RLE source scaled by the zoom factors (1/1024) so that its anchor point stays in place;
// 0x4000 and 0x8000 runs are transparent. Source lines above the clip rectangle are consumed while the row is above
// it; the row remainder is then cut below 0x400, dropping pending repeats. Rows below the clip bottom drop the
// remaining repeats of their source line in the same way. Positions are tracked in pixels from the target start.
void Painter::DrawZoomed(int x, int y, int width, int height)
{
    const int scaleX = this->zoomX;
    const int scaleY = this->zoomY;
    const int left = this->zoomAnchorX - this->zoomAnchorX * scaleX / 1024 + x;
    int row = this->zoomAnchorY - this->zoomAnchorY * scaleY / 1024 + y;

    if (left >= this->clipX + this->clipWidth || width * scaleX / 1024 + left < this->clipX)
    {
        return;
    }

    if (row >= this->clipY + this->clipHeight || height * scaleY / 1024 + row < this->clipY)
    {
        return;
    }

    const std::ptrdiff_t pitch = this->screenPitch;
    const std::ptrdiff_t clipTop = this->clipY * pitch;
    std::ptrdiff_t rowClipLeft = row * pitch + this->clipX;
    std::ptrdiff_t rowClipRight = rowClipLeft + this->clipWidth;
    std::ptrdiff_t rowDest = row * pitch + left;
    const std::uint8_t* line = this->source + 8;
    int rowFraction = 0;

    if (Word(line) != 0)
    {
        bool reached = false;

        while (!reached)
        {
            rowFraction += scaleY;

            while (rowFraction >= 0x400)
            {
                if (rowClipLeft >= clipTop)
                {
                    reached = true;
                    break;
                }

                rowFraction -= 0x400;
                ++row;
                rowClipLeft += pitch;
                rowClipRight = rowClipLeft + this->clipWidth;
                rowDest += pitch;
            }

            if (reached)
            {
                break;
            }

            line += Word(line) & 0x3FFF;

            if (Word(line) == 0)
            {
                break;
            }
        }
    }

    rowFraction %= 0x400;

    if (Word(line) == 0)
    {
        return;
    }

    while (true)
    {
        rowFraction += scaleY;

        while (rowFraction >= 0x400)
        {
            rowFraction -= 0x400;

            if (row >= this->clipY + this->clipHeight)
            {
                break;
            }

            std::ptrdiff_t dest = rowDest;
            const std::uint8_t* code = line + 2;
            int columnFraction = 0;

            for (std::uint16_t value = Word(code); value != 0; value = Word(code))
            {
                const int count = value & 0x3FFF;
                const bool literal = (value & 0xC000) == 0;
                const int scaled = scaleX * count / 1024;
                const std::uint8_t* pixels = code + 2;
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
                        columnFraction %= 0x400;
                        bool stopped = false;

                        for (int i = 0; i < count && !stopped; ++i)
                        {
                            columnFraction += scaleX;

                            while (columnFraction >= 0x400)
                            {
                                columnFraction -= 0x400;

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
                code += literal ? 2 + 2 * count : 2;
            }

            ++row;
            rowClipLeft += pitch;
            rowClipRight = rowClipLeft + this->clipWidth;
            rowDest += pitch;
        }

        line += Word(line) & 0x3FFF;

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
    const int scaleX = this->zoomX;
    const int scaleY = this->zoomY;
    const int right = (width - this->zoomAnchorX) * scaleX / 1024 + this->zoomAnchorX + x;
    int row = this->zoomAnchorY - this->zoomAnchorY * scaleY / 1024 + y;

    if (right - width * scaleX / 1024 >= this->clipX + this->clipWidth || right < this->clipX)
    {
        return;
    }

    if (height * scaleY / 1024 + row >= this->clipY + this->clipHeight || row < this->clipY)
    {
        return;
    }

    const std::ptrdiff_t pitch = this->screenPitch;
    const std::ptrdiff_t clipTop = this->clipY * pitch;
    std::ptrdiff_t rowClipLeft = row * pitch + this->clipX;
    std::ptrdiff_t rowClipRight = rowClipLeft + this->clipWidth;
    std::ptrdiff_t rowDest = row * pitch + right;
    const std::uint8_t* line = this->source + 8;
    int rowFraction = 0;

    if (Word(line) != 0)
    {
        bool reached = false;

        while (!reached)
        {
            rowFraction += scaleY;

            while (rowFraction >= 0x400)
            {
                if (rowClipLeft >= clipTop)
                {
                    reached = true;
                    break;
                }

                rowFraction -= 0x400;
                ++row;
                rowClipLeft += pitch;
                rowClipRight = rowClipLeft + this->clipWidth;
                rowDest += pitch;
            }

            if (reached)
            {
                break;
            }

            line += Word(line) & 0x3FFF;

            if (Word(line) == 0)
            {
                break;
            }
        }
    }

    rowFraction %= 0x400;

    if (Word(line) == 0)
    {
        return;
    }

    while (true)
    {
        rowFraction += scaleY;

        while (rowFraction >= 0x400)
        {
            rowFraction -= 0x400;

            if (row >= this->clipY + this->clipHeight)
            {
                break;
            }

            std::ptrdiff_t dest = rowDest;
            const std::uint8_t* code = line + 2;
            int columnFraction = 0;

            for (std::uint16_t value = Word(code); value != 0; value = Word(code))
            {
                const int count = value & 0x3FFF;
                const bool literal = (value & 0xC000) == 0;
                const int scaled = scaleX * count / 1024;
                const std::uint8_t* pixels = code + 2;
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
                        columnFraction %= 0x400;
                        bool stopped = false;

                        for (int i = 0; i < count && !stopped; ++i)
                        {
                            columnFraction += scaleX;

                            while (columnFraction >= 0x400)
                            {
                                columnFraction -= 0x400;

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
                code += literal ? 2 + 2 * count : 2;
            }

            ++row;
            rowClipLeft += pitch;
            rowClipRight = rowClipLeft + this->clipWidth;
            rowDest += pitch;
        }

        line += Word(line) & 0x3FFF;

        if (Word(line) == 0)
        {
            return;
        }
    }
}

}  // namespace swd3
