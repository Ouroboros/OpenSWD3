#pragma once

#include <cstddef>
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
    // The format (see RleCodec): a header of HeaderWords words, then lines of run codes.
    static constexpr u16 Magic = 0xFFFF;
    static constexpr std::size_t HeaderWords = 4;
    static constexpr std::size_t HeaderBytes = HeaderWords * sizeof(u16);
    static constexpr std::size_t WidthWord = 1;
    static constexpr std::size_t HeightWord = 2;
    static constexpr std::size_t FormatWord = 3;
    static constexpr std::size_t WidthAt = sizeof(u16);  // the header fields in bytes
    static constexpr std::size_t HeightAt = 2 * sizeof(u16);
    static constexpr std::size_t FormatAt = FormatWord * sizeof(u16);
    static constexpr u16 FormatMask = 0x7FFF;
    static constexpr u16 FormatFlag15 = 0x8000;  // a format flag kept when a picture is converted
    static constexpr u16 TrueColor = 0x10;       // format: 16-bit pixels
    static constexpr u16 Indexed = 8;            // format: palette indices
    static constexpr u16 LengthMask = 0x7FFF;    // a line header: its byte length
    static constexpr u16 LiteralStart = 0x8000;  // and whether it starts with literal pixels
    static constexpr u16 CountMask = 0x3FFF;     // a run code: its pixel count
    static constexpr u16 FillRun = 0x4000;
    static constexpr u16 SkipRun = 0x8000;
    static constexpr u16 KindMask = FillRun | SkipRun;

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
