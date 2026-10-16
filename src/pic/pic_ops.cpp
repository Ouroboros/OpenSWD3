#include "pic/pic_ops.h"

#include <cstring>

#include "crt/crt.h"

namespace swd3 {

// swd32.exe 0x00433AA0: whether (x, y) relative to (originX, originY) is on a literal pixel of the RLE picture.
// The width is only checked above the picture, rows above it scan past the last row, every run with bit 14 or 15
// set counts as (code + 0x4000) & 0xFFFF pixels (so only 0xC000 skip runs have their real length) and both run
// bounds are inclusive.
int PictureHitTest(const Picture* picture, int x, int y, int originX, int originY) {
    const auto* data = static_cast<const std::uint16_t*>(picture->pixels);
    const int column = x - originX;
    const int row = y - originY;
    if (column < 0 || (row < 0 && column >= picture->width)) {
        return 0;
    }
    const int height = picture->height;
    if (row >= height || data[0] != 0xFFFF) {
        return 0;
    }
    const std::uint16_t* code = data + 4;
    for (int line = 0; line < height; ++line) {
        if (line == row) {
            ++code;
            break;
        }
        code += (*code >> 1) & 0x3FFF;
    }
    if (*code == 0) {
        return 0;
    }
    const int width = picture->width;
    int position = 0;
    while (position < width) {
        const std::uint32_t run = *code;
        if ((run & 0xC000) != 0) {
            const std::uint32_t count = (run + 0x4000) & 0xFFFF;
            if (column >= position && column <= static_cast<int>(count) + position) {
                return 0;
            }
            position += static_cast<int>(count);
            ++code;
            continue;
        }
        if (column >= position && column <= static_cast<int>(run) + position) {
            return 1;
        }
        position += static_cast<int>(run);
        code += run + 1;
    }
    return 0;
}

// swd32.exe 0x00433F70: rotates the rows (up / down) or the pixels of every row (left / right) of an
// uncompressed picture (0xFFFF, width, height, rows of 2 header words + width pixels + 1 word) by `amount`. Bit 15
// of the first row word is cleared; pictures with bit 14 there are left alone. Amounts beyond the picture size are
// not checked.
void ShiftPicture(std::uint16_t* data, int mode, int amount) {
    if (amount <= 0 || data[0] != 0xFFFF) {
        return;
    }
    const int height = data[2];
    data[4] &= 0x7FFF;
    auto* rows = reinterpret_cast<std::uint8_t*>(data + 4);
    const int stride = data[1] + 3;
    if ((data[4] & 0xC000) != 0) {
        return;
    }
    const std::size_t rowBytes = static_cast<std::size_t>(2 * stride);
    switch (mode) {
    case kShiftUp: {
        const std::size_t moved = static_cast<std::size_t>(2 * amount * stride);
        auto* saved = static_cast<std::uint8_t*>(crt::malloc(moved));
        std::memcpy(saved, rows, moved);
        int kept = 0;
        if (height - amount > 0) {
            for (int row = 0; row < height - amount; ++row) {
                std::memcpy(rows + rowBytes * row, rows + moved + rowBytes * row, rowBytes);
            }
            kept = height - amount;
        }
        std::memcpy(rows + rowBytes * kept, saved, moved);
        crt::free(saved);
        break;
    }
    case kShiftDown: {
        const std::size_t moved = static_cast<std::size_t>(2 * amount * stride);
        auto* saved = static_cast<std::uint8_t*>(crt::malloc(moved));
        std::memcpy(saved, rows + rowBytes * (height - amount), moved);
        if (height - 1 >= amount) {
            for (int row = height - 1, count = height - amount; count != 0; --row, --count) {
                std::memcpy(rows + rowBytes * row, rows + rowBytes * row - moved, rowBytes);
            }
        }
        std::memcpy(rows, saved, moved);
        crt::free(saved);
        break;
    }
    case kShiftLeft:
    case kShiftRight: {
        const std::size_t pixelBytes = rowBytes - 6;
        auto* saved = static_cast<std::uint8_t*>(crt::malloc(pixelBytes));
        const auto shifted = static_cast<std::size_t>(2 * amount);
        for (int row = 0; row < height; ++row) {
            std::uint8_t* pixels = rows + rowBytes * row + 4;
            std::memcpy(saved, pixels, pixelBytes);
            if (mode == kShiftLeft) {
                std::memcpy(pixels + pixelBytes - shifted, saved, shifted);
                std::memcpy(pixels, saved + shifted, pixelBytes - shifted);
            } else {
                std::memcpy(pixels, saved + pixelBytes - shifted, shifted);
                std::memcpy(pixels + shifted, saved, pixelBytes - shifted);
            }
        }
        crt::free(saved);
        break;
    }
    default:
        break;
    }
}

}  // namespace swd3
