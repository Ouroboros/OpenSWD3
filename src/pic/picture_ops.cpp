#include "pic/picture_ops.h"

#include <algorithm>
#include <cstring>

#include "pic/picture.h"

namespace swd3
{

namespace
{

constexpr u16 kRleMark = 0xFFFF;
constexpr u32 kRunFlags = 0xC000;
constexpr std::size_t kHeaderWords = 4;
constexpr std::size_t kRowHeaderBytes = 4;
constexpr int kRowExtraWords = 3;

// The word at `index` of the data; words past the end read as 0.
u16 WordAt(const std::vector<u8>& data, std::size_t index)
{
    if (2 * index + 1 >= data.size())
    {
        return 0;
    }

    return static_cast<u16>(data[2 * index] | (data[2 * index + 1] << 8));
}

}  // namespace

// swd32.exe 0x00433AA0: whether (x, y) relative to (originX, originY) is on a literal pixel of the RLE picture. The
// width is only checked above the picture, rows above it scan past the last row, every run with bit 14 or 15 counts
// as (code + 0x4000) & 0xFFFF pixels (so only 0xC000 runs have their real length) and both run bounds are inclusive.
// Words past the picture data (the original reads whatever follows) count as 0.
int PictureHitTest(const Picture& picture, int x, int y, int originX, int originY)
{
    const int column = x - originX;
    const int row = y - originY;

    if (column < 0 || (row < 0 && column >= picture.width))
    {
        return 0;
    }

    const int height = picture.height;

    if (row >= height || WordAt(picture.data, 0) != kRleMark)
    {
        return 0;
    }

    std::size_t code = kHeaderWords;

    for (int line = 0; line < height; ++line)
    {
        if (line == row)
        {
            ++code;
            break;
        }

        code += (WordAt(picture.data, code) >> 1) & 0x3FFF;
    }

    if (WordAt(picture.data, code) == 0)
    {
        return 0;
    }

    const int width = picture.width;
    int position = 0;

    while (position < width)
    {
        const u32 run = WordAt(picture.data, code);

        if ((run & kRunFlags) != 0)
        {
            const u32 count = (run + 0x4000) & 0xFFFF;

            if (column >= position && column <= static_cast<int>(count) + position)
            {
                return 0;
            }

            position += static_cast<int>(count);
            ++code;
            continue;
        }

        if (column >= position && column <= static_cast<int>(run) + position)
        {
            return 1;
        }

        position += static_cast<int>(run);
        code += run + 1;
    }

    return 0;
}

// swd32.exe 0x00433F70: rotates the rows (up or down) or the pixels of every row (left or right) of an uncompressed
// picture (0xFFFF, width, height, then rows of 2 header words, the pixels and 1 word) by `amount`. Bit 15 of the first
// row word is cleared; pictures with bit 14 there are left alone. The original does not check the amount against the
// picture size and moves memory outside it; such an amount leaves the picture unchanged here.
void ShiftPicture(std::vector<u8>& data, ShiftMode mode, int amount)
{
    if (amount <= 0 || WordAt(data, 0) != kRleMark)
    {
        return;
    }

    const int width = WordAt(data, 1);
    const int height = WordAt(data, 2);
    const std::size_t first = 2 * kHeaderWords;

    if (data.size() < first + 2)
    {
        return;
    }

    data[first + 1] &= 0x7F;

    if ((WordAt(data, kHeaderWords) & kRunFlags) != 0)
    {
        return;
    }

    const auto rowBytes = static_cast<std::size_t>(2 * (width + kRowExtraWords));
    const bool vertical = mode == ShiftMode::Up || mode == ShiftMode::Down;

    if ((vertical && amount > height) || (!vertical && amount > width) ||
        first + rowBytes * static_cast<std::size_t>(height) > data.size())
    {
        return;
    }

    u8* rows = data.data() + first;
    const std::size_t moved = rowBytes * static_cast<std::size_t>(amount);

    switch (mode)
    {
        case ShiftMode::Up:
        {
            const std::vector<u8> saved(rows, rows + moved);
            const int kept = std::max(height - amount, 0);

            for (int row = 0; row < kept; ++row)
            {
                std::memcpy(rows + rowBytes * row, rows + moved + rowBytes * row, rowBytes);
            }

            std::memcpy(rows + rowBytes * kept, saved.data(), moved);
            break;
        }

        case ShiftMode::Down:
        {
            const u8* tail = rows + rowBytes * static_cast<std::size_t>(height - amount);
            const std::vector<u8> saved(tail, tail + moved);

            for (int row = height - 1, count = height - amount; count > 0; --row, --count)
            {
                std::memcpy(rows + rowBytes * row, rows + rowBytes * row - moved, rowBytes);
            }

            std::memcpy(rows, saved.data(), moved);
            break;
        }

        case ShiftMode::Left:
        case ShiftMode::Right:
        {
            const std::size_t pixelBytes = rowBytes - 6;
            const auto shifted = static_cast<std::size_t>(2 * amount);
            std::vector<u8> saved(pixelBytes);

            for (int row = 0; row < height; ++row)
            {
                u8* pixels = rows + rowBytes * row + kRowHeaderBytes;
                std::memcpy(saved.data(), pixels, pixelBytes);

                if (mode == ShiftMode::Left)
                {
                    std::memcpy(pixels + pixelBytes - shifted, saved.data(), shifted);
                    std::memcpy(pixels, saved.data() + shifted, pixelBytes - shifted);
                }
                else
                {
                    std::memcpy(pixels, saved.data() + pixelBytes - shifted, shifted);
                    std::memcpy(pixels + shifted, saved.data(), pixelBytes - shifted);
                }
            }

            break;
        }
    }
}

}  // namespace swd3
