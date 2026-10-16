#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "pic/picture.h"

namespace swd3
{

class Painter;

struct RleHeader
{
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint16_t format = 0;
};

// The RLE picture format: a header of four words (magic 0xFFFF, width, height, format: 0x10 for 16-bit pixels, 8 for
// palette indices, more flags above), then one block per line: a word with the byte length of the line (bit 15 set
// when the line starts with literal pixels), run codes and a zero code; a zero line header ends the picture. A code
// 0x0000 | n is followed by n literal pixels, 0x8000 | n stands for n pixels of the skip colour and 0xC000 | n for n
// pixels of the fill colour.
class RleCodec
{
public:
    explicit RleCodec(const Painter& painter);

    std::vector<std::uint8_t> Encode(const void* pixels, int width, int height, int format) const;
    std::optional<RawPicture> Decode(std::span<const std::uint8_t> rle) const;
    bool ConvertToDisplay(std::span<std::uint8_t> rle, RleHeader& header) const;
    void ConvertTo16(Picture& picture) const;
    void ConvertTileTo16(std::vector<std::uint8_t>& tile) const;

private:
    const Painter& painter;
};

}  // namespace swd3
