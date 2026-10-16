#include "pic/rle.h"

#include <cstring>

#include "draw/painter.h"

namespace swd3
{

namespace
{

constexpr u16 kLiteral = 0x0000;
constexpr u16 kSkip = 0x8000;
constexpr u16 kFill = 0xC000;
constexpr u16 kKindMask = 0xC000;
constexpr u16 kCountMask = 0x3FFF;
constexpr u16 kStartsWithLiteral = 0x8000;

u16 ReadWord(const u8* bytes)
{
    u16 value = 0;
    std::memcpy(&value, bytes, 2);
    return value;
}

void WriteWord(u8* bytes, u16 value)
{
    std::memcpy(bytes, &value, 2);
}

void AppendWord(std::vector<u8>& out, u16 value)
{
    out.push_back(static_cast<u8>(value));
    out.push_back(static_cast<u8>(value >> 8));
}

// Encoder of one picture line. Fill and skip pixels become runs, everything else literal runs. A pixel equal to both
// colours counts as skip (the fill test of the original requires it to differ from the skip colour).
template <typename Pixel>
void EncodeLine(const Pixel*& source, int width, Pixel fill, Pixel skip, bool fillOnlyFlag, std::vector<u8>& out)
{
    const std::size_t header = out.size();
    AppendWord(out, 0);
    u32 lineBytes = 2;
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

                if constexpr (sizeof(Pixel) == 2)
                {
                    AppendWord(out, value);
                }
                else
                {
                    out.push_back(value);
                }
            }

            lineBytes += 2 + static_cast<u32>(length * sizeof(Pixel));
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
        lineBytes += 2;
        source += length;
        x += length;
    } while (x <= width);

    AppendWord(out, 0);
    WriteWord(out.data() + header, static_cast<u16>((lineBytes + 2) | flag));
}

}  // namespace

RleCodec::RleCodec(const Painter& painter) : painter(painter)
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
    AppendWord(out, static_cast<u16>(format == 0x10 ? 0x10 : format));

    if (format == 0x10)
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
// the line lengths are not used. Formats other than 0x10 and 8 give nothing. Output beyond width * height pixels
// (malformed data, written past the buffer in the original) is dropped.
std::optional<RawPicture> RleCodec::Decode(std::span<const u8> rle) const
{
    const RleColors& colors = this->painter.Rle();

    if (rle.size() < 8 || ReadWord(rle.data()) != colors.magic)
    {
        return std::nullopt;
    }

    RawPicture picture;
    picture.width = ReadWord(rle.data() + 2);
    picture.height = ReadWord(rle.data() + 4);
    picture.format = ReadWord(rle.data() + 6) & 0x3FFF;

    if (picture.format != 0x10 && picture.format != 8)
    {
        return std::nullopt;
    }

    const std::size_t pixelSize = picture.format == 0x10 ? 2 : 1;
    picture.pixels.assign(static_cast<std::size_t>(picture.width) * picture.height * pixelSize, 0);
    std::size_t written = 0;

    auto put = [&picture, &written, pixelSize](u16 value)
    {
        if (written + pixelSize <= picture.pixels.size())
        {
            if (pixelSize == 2)
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

    const u8* line = rle.data() + 8;

    while (ReadWord(line) != 0)
    {
        const u8* code = line + 2;

        for (u16 value = ReadWord(code); value != 0; value = ReadWord(code))
        {
            code += 2;
            const int count = value & kCountMask;

            for (int i = 0; i < count; ++i)
            {
                switch (value & kKindMask)
                {
                    case kLiteral:
                        put(pixelSize == 2 ? ReadWord(code + 2 * i) : code[i]);
                        break;

                    case kSkip:
                        put(pixelSize == 2 ? colors.skip16 : colors.skip8);
                        break;

                    case kFill:
                        put(pixelSize == 2 ? colors.fill16 : colors.fill8);
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

        line = code + 2;
    }

    return picture;
}

// swd32.exe 0x00401B70: reads the header and, for a 16-bit picture, converts its literal pixels to the display format
// in place. False for another magic or format (the header fields are still read for another format).
bool RleCodec::ConvertToDisplay(std::span<u8> rle, RleHeader& header) const
{
    if (rle.size() < 8 || ReadWord(rle.data()) != this->painter.Rle().magic)
    {
        return false;
    }

    header.width = ReadWord(rle.data() + 2);
    header.height = ReadWord(rle.data() + 4);
    header.format = ReadWord(rle.data() + 6) & 0x7FFF;

    if (header.format != 0x10)
    {
        return false;
    }

    u8* line = rle.data() + 8;

    while (ReadWord(line) != 0)
    {
        u8* code = line + 2;

        for (u16 value = ReadWord(code); value != 0; value = ReadWord(code))
        {
            code += 2;

            if ((value & kKindMask) == kLiteral)
            {
                const int count = value & kCountMask;
                std::vector<u16> pixels(static_cast<std::size_t>(count));
                std::memcpy(pixels.data(), code, pixels.size() * 2);
                this->painter.ConvertPixels(pixels);
                std::memcpy(code, pixels.data(), pixels.size() * 2);
                code += 2 * count;
            }
        }

        line = code + 2;
    }

    return true;
}

// swd32.exe 0x00401C70: an 8-bit picture becomes a 16-bit one through its palette (converted to the display format)
// and loses the palette; the new line headers hold the plain length. A 16-bit picture is converted in place and
// takes its size from the header.
void RleCodec::ConvertTo16(Picture& picture) const
{
    const u8* source = picture.data.data();

    if (picture.data.size() < 8 || ReadWord(source) != this->painter.Rle().magic)
    {
        return;
    }

    const u16 format = ReadWord(source + 6);

    if ((format & 0x7FFF) != 8)
    {
        RleHeader header;
        this->ConvertToDisplay(picture.data, header);
        picture.width = header.width;
        picture.height = header.height;
        return;
    }

    std::vector<u8> out;
    AppendWord(out, ReadWord(source));
    AppendWord(out, ReadWord(source + 2));
    AppendWord(out, ReadWord(source + 4));
    AppendWord(out, static_cast<u16>((format & 0x8010) | 0x10));
    const u8* line = source + 8;

    while (ReadWord(line) != 0)
    {
        const std::size_t header = out.size();
        AppendWord(out, 0);
        u32 lineBytes = 4;
        const u8* code = line + 2;
        u16 value = ReadWord(code);
        AppendWord(out, value);
        code += 2;

        while (value != 0)
        {
            if ((value & kKindMask) == kLiteral)
            {
                std::vector<u16> pixels(value & kCountMask);

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

                lineBytes += static_cast<u32>(2 * pixels.size());
                code += pixels.size();
            }

            value = ReadWord(code);
            AppendWord(out, value);
            lineBytes += 2;
            code += 2;
        }

        WriteWord(out.data() + header, static_cast<u16>(lineBytes));
        line = code;
    }

    AppendWord(out, 0);
    // The original counts two bytes beyond the end and copies them from its fresh work buffer (0xCD in the debug heap).
    AppendWord(out, 0xCDCD);
    picture.data = std::move(out);
    picture.palette.clear();
}

// swd32.exe 0x00401E50: a map tile (a 256-entry palette followed by an 8-bit RLE picture) becomes a 16-bit RLE
// picture. The size is cut to 16 bits, as in the original (which then copies only that many bytes).
void RleCodec::ConvertTileTo16(std::vector<u8>& tile) const
{
    constexpr std::size_t kPaletteBytes = 0x200;

    if (tile.size() < kPaletteBytes + 8 || ReadWord(tile.data() + kPaletteBytes) != this->painter.Rle().magic)
    {
        return;
    }

    const u8* source = tile.data() + kPaletteBytes;
    const u16 format = ReadWord(source + 6);

    if ((format & 0x7FFF) != 8)
    {
        return;
    }

    std::vector<u8> out;
    AppendWord(out, ReadWord(source));
    AppendWord(out, ReadWord(source + 2));
    AppendWord(out, ReadWord(source + 4));
    AppendWord(out, static_cast<u16>((format & 0x8010) | 0x10));
    std::size_t header = out.size();
    AppendWord(out, 0);
    const u8* line = source + 8;

    while (ReadWord(line) != 0)
    {
        const u8* code = line + 2;
        u16 value = ReadWord(code);
        AppendWord(out, value);
        code += 2;
        u32 lineBytes = 4;

        while (value != 0)
        {
            if ((value & kKindMask) == kLiteral)
            {
                std::vector<u16> pixels(value);

                for (std::size_t i = 0; i < pixels.size(); ++i)
                {
                    pixels[i] = ReadWord(tile.data() + 2 * code[i]);
                }

                this->painter.ConvertPixels(pixels);

                for (u16 pixel : pixels)
                {
                    AppendWord(out, pixel);
                }

                lineBytes += static_cast<u32>(2 * pixels.size());
                code += pixels.size();
            }

            value = ReadWord(code);
            AppendWord(out, value);
            lineBytes += 2;
            code += 2;
        }

        WriteWord(out.data() + header, static_cast<u16>(lineBytes));
        header = out.size();
        AppendWord(out, 0);
        line = code;
    }

    // The original counts two bytes beyond the end and copies them from its fresh work buffer (0xCD in the debug heap).
    AppendWord(out, 0xCDCD);
    out.resize(out.size() & 0xFFFF);
    tile = std::move(out);
}

}  // namespace swd3
