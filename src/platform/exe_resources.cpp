#include "platform/exe_resources.h"

#include <climits>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "core/byte_reader.h"
#include "core/format.h"
#include "platform/file_system.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

// A .bmp file: the file header ("BM" first), then the bitmap.
constexpr u16 kBitmapSignature = 0x4D42;
constexpr std::size_t kBitmapFileHeaderBytes = 14;

// The BITMAPINFOHEADER of a bitmap resource.
constexpr std::size_t kBitmapWidthAt = 4;
constexpr std::size_t kBitmapHeightAt = 8;
constexpr std::size_t kBitmapBitsAt = 14;
constexpr std::size_t kBitmapCompressionAt = 16;
constexpr std::size_t kBitmapColorsAt = 32;
constexpr u32 kBitmapHeaderBytes = 40;
constexpr u16 kIndexedBits = 8;
constexpr u16 kTrueColorBits = 24;
constexpr std::size_t kTrueColorBytes = 3;
constexpr u32 kPaletteColors = 256;
constexpr std::size_t kPaletteEntryBytes = 4;
constexpr std::size_t kRowAlign = 4;
constexpr int kRed565Drop = 3;  // 8-bit channels to RGB565
constexpr int kGreen565Drop = 2;
constexpr int kBlue565Drop = 3;
constexpr int kRed565Shift = 11;
constexpr int kGreen565Shift = 5;
constexpr std::size_t kGreenAt = 1;  // a pixel or palette entry: blue, green, red
constexpr std::size_t kRedAt = 2;

// An .ico or .cur file: a header with the member count, then per member an entry (a cursor's hotspot in place of the
// planes and bits) with its size and offset; a member is a bitmap of twice its height (the colours, then the 1-bit
// mask).
constexpr std::size_t kGroupCountAt = 4;
constexpr std::size_t kGroupEntriesAt = 6;
constexpr std::size_t kGroupEntryBytes = 16;
constexpr std::size_t kHotXAt = 4;
constexpr std::size_t kHotYAt = 6;
constexpr std::size_t kMemberBytesAt = 8;
constexpr std::size_t kMemberAt = 12;
constexpr u16 kMaskBits = 1;
constexpr u16 kHighColorBits = 32;
constexpr std::size_t kHighColorBytes = 4;
constexpr u32 kOpaque = 0xFF000000u;
constexpr u32 kMaskedInverted = 0xFF000000u;  // a masked pixel of a colour inverts the screen: shown black
constexpr int kRedShift = 16;
constexpr int kGreenShift = 8;

class Reader
{
public:
    explicit Reader(std::span<const u8> bytes)
        : bytes(bytes)
    {
    }

    bool Has(std::size_t offset, std::size_t length) const
    {
        return offset <= this->bytes.size() && length <= this->bytes.size() - offset;
    }

    u16 Word(std::size_t offset) const
    {
        return ByteReader::Word16At(this->bytes, offset);
    }

    u32 Dword(std::size_t offset) const
    {
        return ByteReader::Word32At(this->bytes, offset);
    }

private:
    std::span<const u8> bytes;
};

std::size_t BitmapStride(i32 width, u16 bits)
{
    constexpr std::size_t kRowAlignBits = kRowAlign * CHAR_BIT;
    return (static_cast<std::size_t>(width) * bits + kRowAlignBits - 1) / kRowAlignBits * kRowAlign;
}

bool DecodeIconBitmap(std::span<const u8> bitmap, IconImage& out)
{
    const Reader reader(bitmap);
    const u32 headerSize = reader.Dword(0);
    const auto width = static_cast<i32>(reader.Dword(kBitmapWidthAt));
    const auto height = static_cast<i32>(reader.Dword(kBitmapHeightAt)) / 2;
    const u16 bits = reader.Word(kBitmapBitsAt);

    if (!reader.Has(0, kBitmapHeaderBytes) || headerSize < kBitmapHeaderBytes ||
        reader.Dword(kBitmapCompressionAt) != 0 || width <= 0 || height <= 0 ||
        (bits != 1 && bits != 4 && bits != kIndexedBits && bits != kTrueColorBits && bits != kHighColorBits))
    {
        return false;
    }

    u32 colors = 0;

    if (bits <= kIndexedBits)
    {
        colors = reader.Dword(kBitmapColorsAt);

        if (colors == 0)
        {
            colors = 1u << bits;
        }
    }

    const std::size_t palette = headerSize;
    const std::size_t pixels = palette + kPaletteEntryBytes * static_cast<std::size_t>(colors);
    const std::size_t stride = BitmapStride(width, bits);
    const std::size_t mask = pixels + stride * static_cast<std::size_t>(height);
    const std::size_t maskStride = BitmapStride(width, kMaskBits);

    if (!reader
             .Has(pixels, stride * static_cast<std::size_t>(height) + maskStride * static_cast<std::size_t>(height)) ||
        !reader.Has(palette, kPaletteEntryBytes * static_cast<std::size_t>(colors)))
    {
        return false;
    }

    out.width = width;
    out.height = height;
    out.pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);

    auto rgb = [&bitmap](std::size_t at)
    {
        return static_cast<u32>(bitmap[at + kRedAt]) << kRedShift |
               static_cast<u32>(bitmap[at + kGreenAt]) << kGreenShift | static_cast<u32>(bitmap[at]);
    };

    for (int row = 0; row < height; ++row)
    {
        const auto stored = static_cast<std::size_t>(height - 1 - row);
        const std::size_t source = pixels + stride * stored;
        const std::size_t maskRow = mask + maskStride * stored;

        for (int column = 0; column < width; ++column)
        {
            const auto at = static_cast<std::size_t>(column);
            u32 color = 0;

            if (bits <= kIndexedBits)
            {
                const std::size_t bit = at * bits;
                const u32 byte = bitmap[source + bit / CHAR_BIT];
                const u32 index = byte >> (CHAR_BIT - bits - bit % CHAR_BIT) & ((1u << bits) - 1);
                color = rgb(palette + kPaletteEntryBytes * (index < colors ? index : 0));
            }
            else
            {
                color = rgb(source + (bits == kTrueColorBits ? kTrueColorBytes : kHighColorBytes) * at);
            }

            const bool masked = (bitmap[maskRow + at / CHAR_BIT] >> (CHAR_BIT - 1 - at % CHAR_BIT) & 1) != 0;
            out.pixels[static_cast<std::size_t>(row) * static_cast<std::size_t>(width) + at] =
                !masked ? (color | kOpaque) : (color != 0 ? kMaskedInverted : 0);
        }
    }

    return true;
}

}  // namespace

void ExeResources::Open(std::u8string directory)
{
    this->directory = std::move(directory);
}

std::vector<u8> ExeResources::Read(u32 id, std::u8string_view extension) const
{
    std::vector<u8> bytes;

    if (!ReadSharedFile(this->directory + Utf8Bytes(std::to_string(id)) + std::u8string(extension), bytes))
    {
        bytes.clear();
    }

    return bytes;
}

bool ExeResources::LoadBitmap(u32 id, Surface& out) const
{
    const std::vector<u8> file = this->Read(id, u8".bmp");
    const Reader reader(file);

    if (!reader.Has(0, kBitmapFileHeaderBytes) || reader.Word(0) != kBitmapSignature)
    {
        return false;
    }

    return DecodeBitmap(std::span<const u8>(file).subspan(kBitmapFileHeaderBytes), out);
}

bool ExeResources::LoadIcon(u32 id, IconImage& out) const
{
    return this->LoadGroup(id, u8".ico", out);
}

bool ExeResources::LoadCursor(u32 id, IconImage& out) const
{
    return this->LoadGroup(id, u8".cur", out);
}

std::vector<u8> ExeResources::ReadSound(u32 id) const
{
    return this->Read(id, u8".wav");
}

std::vector<u8> ExeResources::ReadDialog(u32 id) const
{
    return this->Read(id, u8".dlg");
}

bool ExeResources::LoadGroup(u32 id, std::u8string_view extension, IconImage& out) const
{
    const std::vector<u8> file = this->Read(id, extension);
    const std::span<const u8> bytes(file);
    const Reader reader(bytes);

    if (!reader.Has(0, kGroupEntriesAt))
    {
        return false;
    }

    const u16 count = reader.Word(kGroupCountAt);
    u16 bestBits = 0;
    std::size_t best = 0;
    bool found = false;

    for (u16 index = 0; index < count; ++index)
    {
        const std::size_t entry = kGroupEntriesAt + kGroupEntryBytes * index;

        if (!reader.Has(entry, kGroupEntryBytes))
        {
            break;
        }

        const u32 at = reader.Dword(entry + kMemberAt);

        if (!reader.Has(at, kBitmapHeaderBytes) || !reader.Has(at, reader.Dword(entry + kMemberBytesAt)))
        {
            continue;
        }

        const u16 bits = reader.Word(at + kBitmapBitsAt);

        if (!found || bits > bestBits)
        {
            bestBits = bits;
            best = entry;
            found = true;
        }
    }

    if (!found ||
        !DecodeIconBitmap(bytes.subspan(reader.Dword(best + kMemberAt), reader.Dword(best + kMemberBytesAt)), out))
    {
        return false;
    }

    const bool cursor = extension == u8".cur";
    out.hotX = cursor ? reader.Word(best + kHotXAt) : 0;
    out.hotY = cursor ? reader.Word(best + kHotYAt) : 0;
    return true;
}

bool DecodeBitmap(std::span<const u8> bitmap, Surface& out)
{
    const Reader reader(bitmap);
    const u32 headerSize = reader.Dword(0);
    const auto width = static_cast<i32>(reader.Dword(kBitmapWidthAt));
    const auto height = static_cast<i32>(reader.Dword(kBitmapHeightAt));
    const u16 bits = reader.Word(kBitmapBitsAt);

    if (headerSize < kBitmapHeaderBytes || reader.Dword(kBitmapCompressionAt) != 0 || width <= 0 || height == 0 ||
        (bits != kIndexedBits && bits != kTrueColorBits))
    {
        return false;
    }

    const int rows = height > 0 ? height : -height;
    u32 colors = reader.Dword(kBitmapColorsAt);

    if (bits == kIndexedBits && colors == 0)
    {
        colors = kPaletteColors;
    }

    const std::size_t palette = headerSize;
    const std::size_t pixels =
        palette + (bits == kIndexedBits ? kPaletteEntryBytes * static_cast<std::size_t>(colors) : 0);
    const std::size_t stride = (static_cast<std::size_t>(width) * bits / CHAR_BIT + kRowAlign - 1) & ~(kRowAlign - 1);

    if (!reader.Has(pixels, stride * static_cast<std::size_t>(rows)))
    {
        return false;
    }

    out.Resize(width, rows);

    auto pack = [](u32 blue, u32 green, u32 red)
    {
        return static_cast<u16>(
            (red >> kRed565Drop) << kRed565Shift | (green >> kGreen565Drop) << kGreen565Shift | blue >> kBlue565Drop
        );
    };

    for (int row = 0; row < rows; ++row)
    {
        const std::size_t source = pixels + stride * static_cast<std::size_t>(height > 0 ? rows - 1 - row : row);
        u16* target = out.Row(row);

        for (int column = 0; column < width; ++column)
        {
            if (bits == kIndexedBits)
            {
                const u8 index = bitmap[source + static_cast<std::size_t>(column)];
                const std::size_t entry =
                    palette + kPaletteEntryBytes * static_cast<std::size_t>(index < colors ? index : 0);
                target[column] = pack(bitmap[entry], bitmap[entry + kGreenAt], bitmap[entry + kRedAt]);
            }
            else
            {
                const std::size_t at = source + kTrueColorBytes * static_cast<std::size_t>(column);
                target[column] = pack(bitmap[at], bitmap[at + kGreenAt], bitmap[at + kRedAt]);
            }
        }
    }

    return true;
}

}  // namespace swd3
