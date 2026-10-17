#include "pic/rle.h"

#include <algorithm>
#include <climits>
#include <cstring>

#include "core/words.h"
#include "draw/painter.h"

namespace swd3
{

namespace
{

constexpr u16 kLiteral = 0;
constexpr u16 kSkip = RleHeader::SkipRun;
constexpr u16 kFill = RleHeader::KindMask;  // the fill code has both kind bits
constexpr u16 kKindMask = RleHeader::KindMask;
constexpr u16 kCountMask = RleHeader::CountMask;
constexpr u16 kStartsWithLiteral = RleHeader::LiteralStart;
constexpr u32 kLineOverhead = 2 * sizeof(u16);  // a line header and its end code
constexpr u16 kDebugHeapFill = 0xCDCD;
constexpr u8 kDebugHeapByte = 0xCD;     // the debug heap fills a new block with it
constexpr int kWrappedCount = 0x10000;  // a 16-bit count that starts at 0 comes round after this many

u16 ReadWord(const u8* bytes)
{
    u16 value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

void WriteWord(u8* bytes, u16 value)
{
    std::memcpy(bytes, &value, sizeof(value));
}

void AppendWord(std::vector<u8>& out, u16 value)
{
    out.push_back(static_cast<u8>(value));
    out.push_back(static_cast<u8>(value >> CHAR_BIT));
}

// Encoder of one picture line. Fill and skip pixels become runs, everything else literal runs. A pixel equal to both
// colours counts as skip (the fill test of the original requires it to differ from the skip colour).
template <typename Pixel>
void EncodeLine(const Pixel*& source, int width, Pixel fill, Pixel skip, bool fillOnlyFlag, std::vector<u8>& out)
{
    const std::size_t header = out.size();
    AppendWord(out, 0);
    u32 lineBytes = sizeof(u16);  // the line header
    u16 flag = kStartsWithLiteral;

    if (*source == fill || (!fillOnlyFlag && *source == skip))
    {
        flag = 0;
    }

    int x = 1;

    do
    {
        const Pixel pixel = *source;

        if (pixel != fill && pixel != skip)
        {
            const std::size_t count = out.size();
            AppendWord(out, 0);
            int length = 0;

            for (; x + length <= width; ++length)
            {
                const Pixel value = source[length];

                if (length > 0 && (value == fill || value == skip))
                {
                    break;
                }

                if constexpr (sizeof(Pixel) == sizeof(u16))
                {
                    AppendWord(out, value);
                }
                else
                {
                    out.push_back(value);
                }
            }

            lineBytes += sizeof(u16) + static_cast<u32>(length * sizeof(Pixel));
            WriteWord(out.data() + count, static_cast<u16>(length));
            source += length;
            x += length;
            continue;
        }

        const Pixel run = pixel == fill && pixel != skip ? fill : skip;
        int length = 1;

        while (x + length <= width && source[length] == run)
        {
            ++length;
        }

        AppendWord(out, static_cast<u16>(length | (run == fill && fill != skip ? kFill : kSkip)));
        lineBytes += sizeof(u16);
        source += length;
        x += length;
    } while (x <= width);

    AppendWord(out, 0);
    WriteWord(out.data() + header, static_cast<u16>((lineBytes + sizeof(u16)) | flag));
}

}  // namespace

RleCodec::RleCodec(const Painter& painter)
    : painter(painter)
{
}

// swd32.exe 0x004014F0: format 0x10 encodes 16-bit pixels, any other format bytes (its value goes into the header).
// A 16-bit line is flagged as starting with literal pixels unless its first pixel is the fill or the skip colour; an
// 8-bit line only checks the fill colour.
std::vector<u8> RleCodec::Encode(const void* pixels, int width, int height, int format) const
{
    const RleColors& colors = this->painter.Rle();
    std::vector<u8> out;
    AppendWord(out, static_cast<u16>(colors.magic));
    AppendWord(out, static_cast<u16>(width));
    AppendWord(out, static_cast<u16>(height));
    AppendWord(out, static_cast<u16>(format == RleHeader::TrueColor ? RleHeader::TrueColor : format));

    if (format == RleHeader::TrueColor)
    {
        const auto* source = static_cast<const u16*>(pixels);

        for (int row = 0; row < height; ++row)
        {
            EncodeLine<u16>(source, width, colors.fill16, colors.skip16, false, out);
        }
    }
    else
    {
        const auto* source = static_cast<const u8*>(pixels);

        for (int row = 0; row < height; ++row)
        {
            EncodeLine<u8>(source, width, colors.fill8, colors.skip8, true, out);
        }
    }

    AppendWord(out, 0);
    return out;
}

// swd32.exe 0x004019A0: fill and skip runs become the RLE colours themselves (in the stored format, not converted);
// the line lengths are not used. Formats other than 0x10 and 8 give nothing. A code counting 0 repeats 65536 times
// (the original counts in 16 bits until it comes round). Output beyond width * height pixels and reads past the data
// (malformed data, out of bounds in the original) are dropped: the pixels are not written, the words read as 0.
std::optional<RawPicture> RleCodec::Decode(std::span<const u8> rle) const
{
    const RleColors& colors = this->painter.Rle();

    if (rle.size() < RleHeader::HeaderBytes || ReadWord(rle.data()) != colors.magic)
    {
        return std::nullopt;
    }

    RawPicture picture;
    picture.width = ReadWord(rle.data() + RleHeader::WidthAt);
    picture.height = ReadWord(rle.data() + RleHeader::HeightAt);
    picture.format = ReadWord(rle.data() + RleHeader::FormatAt) & RleHeader::CountMask;

    if (picture.format != RleHeader::TrueColor && picture.format != RleHeader::Indexed)
    {
        return std::nullopt;
    }

    const std::size_t pixelSize = picture.format == RleHeader::TrueColor ? sizeof(u16) : sizeof(u8);
    // The new block of the debug heap: pixels no code covers keep its fill.
    picture.pixels.assign(static_cast<std::size_t>(picture.width) * picture.height * pixelSize, kDebugHeapByte);
    std::size_t written = 0;

    auto put = [&picture, &written, pixelSize](u16 value)
    {
        if (written + pixelSize <= picture.pixels.size())
        {
            if (pixelSize == sizeof(u16))
            {
                WriteWord(picture.pixels.data() + written, value);
            }
            else
            {
                picture.pixels[written] = static_cast<u8>(value);
            }
        }

        written += pixelSize;
    };

    const u8* end = rle.data() + rle.size();

    auto word = [end](const u8* at)
    {
        return at + sizeof(u16) <= end ? ReadWord(at) : u16{0};
    };

    auto byte = [end](const u8* at)
    {
        return at < end ? *at : u8{0};
    };

    const u8* line = rle.data() + RleHeader::HeaderBytes;

    while (word(line) != 0)
    {
        const u8* code = line + sizeof(u16);

        for (u16 value = word(code); value != 0; value = word(code))
        {
            code += sizeof(u16);
            const int count = (value & kCountMask) != 0 ? value & kCountMask : kWrappedCount;

            for (int i = 0; i < count; ++i)
            {
                switch (value & kKindMask)
                {
                    case kLiteral:
                        put(pixelSize == sizeof(u16) ? word(code + sizeof(u16) * i) : byte(code + i));
                        break;

                    case kSkip:
                        put(pixelSize == sizeof(u16) ? colors.skip16 : colors.skip8);
                        break;

                    case kFill:
                        put(pixelSize == sizeof(u16) ? colors.fill16 : colors.fill8);
                        break;

                    default:
                        break;
                }
            }

            if ((value & kKindMask) == kLiteral)
            {
                code += count * pixelSize;
            }
        }

        line = code + sizeof(u16);
    }

    return picture;
}

// swd32.exe 0x00401B70: reads the header and, for a 16-bit picture, converts its literal pixels to the display format
// in place. False for another magic or format (the header fields are still read for another format). Malformed data
// running past its end stops there (read and written out of bounds in the original).
bool RleCodec::ConvertToDisplay(std::span<u8> rle, RleHeader& header) const
{
    if (rle.size() < RleHeader::HeaderBytes || ReadWord(rle.data()) != this->painter.Rle().magic)
    {
        return false;
    }

    header.width = ReadWord(rle.data() + RleHeader::WidthAt);
    header.height = ReadWord(rle.data() + RleHeader::HeightAt);
    header.format = ReadWord(rle.data() + RleHeader::FormatAt) & RleHeader::FormatMask;

    if (header.format != RleHeader::TrueColor)
    {
        return false;
    }

    u8* const end = rle.data() + rle.size();

    auto word = [end](const u8* at)
    {
        return at + sizeof(u16) <= end ? ReadWord(at) : u16{0};
    };

    u8* line = rle.data() + RleHeader::HeaderBytes;

    while (word(line) != 0)
    {
        u8* code = line + sizeof(u16);

        for (u16 value = word(code); value != 0; value = word(code))
        {
            code += sizeof(u16);

            if ((value & kKindMask) == kLiteral)
            {
                const auto room = static_cast<std::size_t>(end > code ? (end - code) / sizeof(u16) : 0);
                const std::size_t count = std::min<std::size_t>(value & kCountMask, room);
                std::vector<u16> pixels(count);
                std::memcpy(pixels.data(), code, pixels.size() * sizeof(u16));
                this->painter.ConvertPixels(pixels);
                std::memcpy(code, pixels.data(), pixels.size() * sizeof(u16));
                code += sizeof(u16) * count;
            }
        }

        line = code + sizeof(u16);
    }

    return true;
}

// swd32.exe 0x00401C70: an 8-bit picture becomes a 16-bit one through its palette (converted to the display format)
// and loses the palette; the new line headers hold the plain length. A 16-bit picture is converted in place and
// takes its size from the header. Malformed data (a literal of count 0, data running past its end) stops at the end.
void RleCodec::ConvertTo16(Picture& picture) const
{
    const u8* source = picture.data.data();

    if (picture.data.size() < RleHeader::HeaderBytes || ReadWord(source) != this->painter.Rle().magic)
    {
        return;
    }

    const u16 format = ReadWord(source + RleHeader::FormatAt);

    if ((format & RleHeader::FormatMask) != RleHeader::Indexed)
    {
        RleHeader header;
        this->ConvertToDisplay(picture.data, header);
        picture.width = header.width;
        picture.height = header.height;
        return;
    }

    std::vector<u8> out;
    AppendWord(out, ReadWord(source));
    AppendWord(out, ReadWord(source + RleHeader::WidthAt));
    AppendWord(out, ReadWord(source + RleHeader::HeightAt));
    AppendWord(
        out,
        static_cast<u16>((format & (RleHeader::FormatFlag15 | RleHeader::TrueColor)) | RleHeader::TrueColor)
    );
    const u8* end = source + picture.data.size();

    auto word = [end](const u8* at)
    {
        return at + sizeof(u16) <= end ? ReadWord(at) : u16{0};
    };

    const u8* line = source + RleHeader::HeaderBytes;

    while (word(line) != 0)
    {
        const std::size_t header = out.size();
        AppendWord(out, 0);
        u32 lineBytes = kLineOverhead;
        const u8* code = line + sizeof(u16);
        u16 value = word(code);
        AppendWord(out, value);
        code += sizeof(u16);

        while (value != 0)
        {
            if ((value & kKindMask) == kLiteral)
            {
                const auto room = static_cast<std::size_t>(end > code ? end - code : 0);
                std::vector<u16> pixels(std::min<std::size_t>(value & kCountMask, room));

                for (std::size_t i = 0; i < pixels.size(); ++i)
                {
                    // Without a palette the original reads through a null pointer; the entries count as 0 here.
                    pixels[i] = code[i] < picture.palette.size() ? picture.palette[code[i]] : 0;
                }

                this->painter.ConvertPixels(pixels);

                for (u16 pixel : pixels)
                {
                    AppendWord(out, pixel);
                }

                lineBytes += static_cast<u32>(sizeof(u16) * pixels.size());
                code += pixels.size();
            }

            value = word(code);
            AppendWord(out, value);
            lineBytes += sizeof(u16);
            code += sizeof(u16);
        }

        WriteWord(out.data() + header, static_cast<u16>(lineBytes));
        line = code;
    }

    AppendWord(out, 0);
    // The original counts two bytes beyond the end and copies them from its fresh work buffer (0xCD in the debug heap).
    AppendWord(out, kDebugHeapFill);
    picture.data = std::move(out);
    picture.palette.clear();
}

// swd32.exe 0x00401E50: a map tile (a 256-entry palette followed by an 8-bit RLE picture) becomes a 16-bit RLE
// picture. The size is cut to 16 bits, as in the original (which then copies only that many bytes). Data running past
// its end stops there.
void RleCodec::ConvertTileTo16(std::vector<u8>& tile) const
{
    constexpr std::size_t kPaletteBytes = 0x200;

    if (tile.size() < kPaletteBytes + RleHeader::HeaderBytes ||
        ReadWord(tile.data() + kPaletteBytes) != this->painter.Rle().magic)
    {
        return;
    }

    const u8* source = tile.data() + kPaletteBytes;
    const u16 format = ReadWord(source + RleHeader::FormatAt);

    if ((format & RleHeader::FormatMask) != RleHeader::Indexed)
    {
        return;
    }

    std::vector<u8> out;
    AppendWord(out, ReadWord(source));
    AppendWord(out, ReadWord(source + RleHeader::WidthAt));
    AppendWord(out, ReadWord(source + RleHeader::HeightAt));
    AppendWord(
        out,
        static_cast<u16>((format & (RleHeader::FormatFlag15 | RleHeader::TrueColor)) | RleHeader::TrueColor)
    );
    std::size_t header = out.size();
    AppendWord(out, 0);
    const u8* end = tile.data() + tile.size();

    auto word = [end](const u8* at)
    {
        return at + sizeof(u16) <= end ? ReadWord(at) : u16{0};
    };

    const u8* line = source + RleHeader::HeaderBytes;

    while (word(line) != 0)
    {
        const u8* code = line + sizeof(u16);
        u16 value = word(code);
        AppendWord(out, value);
        code += sizeof(u16);
        u32 lineBytes = kLineOverhead;

        while (value != 0)
        {
            if ((value & kKindMask) == kLiteral)
            {
                const auto room = static_cast<std::size_t>(end > code ? end - code : 0);
                std::vector<u16> pixels(std::min<std::size_t>(value, room));

                for (std::size_t i = 0; i < pixels.size(); ++i)
                {
                    pixels[i] = ReadWord(tile.data() + sizeof(u16) * code[i]);
                }

                this->painter.ConvertPixels(pixels);

                for (u16 pixel : pixels)
                {
                    AppendWord(out, pixel);
                }

                lineBytes += static_cast<u32>(sizeof(u16) * pixels.size());
                code += pixels.size();
            }

            value = word(code);
            AppendWord(out, value);
            lineBytes += sizeof(u16);
            code += sizeof(u16);
        }

        WriteWord(out.data() + header, static_cast<u16>(lineBytes));
        header = out.size();
        AppendWord(out, 0);
        line = code;
    }

    // The original counts two bytes beyond the end and copies them from its fresh work buffer (0xCD in the debug heap).
    AppendWord(out, kDebugHeapFill);
    out.resize(LowWord(static_cast<u32>(out.size())));
    tile = std::move(out);
}

}  // namespace swd3
