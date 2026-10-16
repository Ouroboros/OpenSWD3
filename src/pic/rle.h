#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "core/types.h"
#include "pic/picture.h"

namespace swd3
{

class Painter;

struct RleHeader
{
    u16 width = 0;
    u16 height = 0;
    u16 format = 0;
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

    std::vector<u8> Encode(const void* pixels, int width, int height, int format) const;
    std::optional<RawPicture> Decode(std::span<const u8> rle) const;
    bool ConvertToDisplay(std::span<u8> rle, RleHeader& header) const;
    void ConvertTo16(Picture& picture) const;
    void ConvertTileTo16(std::vector<u8>& tile) const;

private:
    const Painter& painter;
};

}  // namespace swd3
