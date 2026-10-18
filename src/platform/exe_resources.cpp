#include "platform/exe_resources.h"

#include <climits>
#include <cstring>

#include "core/byte_reader.h"
#include "platform/file_system.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr u32 kResourceDirectory = 2;
constexpr u32 kNamedEntry = 0x80000000u;
constexpr int kMaxDepth = 3;
constexpr int kTypeLevel = 0;  // the levels of the resource tree: type, id, language
constexpr int kIdLevel = 1;

// The PE format.
constexpr u16 kDosSignature = 0x5A4D;  // "MZ"
constexpr std::size_t kNewHeaderAt = 0x3C;
constexpr u32 kPeSignature = 0x4550;  // "PE"
constexpr std::size_t kSectionCountAt = 6;
constexpr std::size_t kOptionalSizeAt = 20;
constexpr std::size_t kOptionalHeaderAt = 24;
constexpr std::size_t kDataDirectoriesAt = 96;
constexpr std::size_t kDataDirectoryBytes = 8;
constexpr std::size_t kSectionBytes = 40;
constexpr std::size_t kVirtualSizeAt = 8;
constexpr std::size_t kVirtualAddressAt = 12;
constexpr std::size_t kRawSizeAt = 16;
constexpr std::size_t kRawAddressAt = 20;
constexpr std::size_t kDirectoryBytes = 16;
constexpr std::size_t kNamedEntriesAt = 12;
constexpr std::size_t kIdEntriesAt = 14;
constexpr std::size_t kDirectoryEntryBytes = 8;

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

// Icon and cursor resources: a group lists its members, a member is a bitmap of twice its height (the colours, then
// the 1-bit mask), a cursor member starts with its hotspot.
constexpr std::size_t kGroupCountAt = 4;
constexpr std::size_t kGroupEntriesAt = 6;
constexpr std::size_t kGroupEntryBytes = 14;
constexpr std::size_t kGroupBitsAt = 6;
constexpr std::size_t kGroupIdAt = 12;
constexpr std::size_t kHotYAt = 2;
constexpr std::size_t kCursorBitmapAt = 4;
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

struct Section
{
    u32 address = 0;
    u32 size = 0;
    u32 raw = 0;
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

bool ExeResources::Open(const std::u8string& path)
{
    if (!ReadSharedFile(ResolvePath(path), this->image))
    {
        return false;
    }

    this->entries.clear();
    return this->Parse();
}

bool ExeResources::Parse()
{
    const Reader reader(this->image);

    if (reader.Word(0) != kDosSignature)
    {
        return false;
    }

    const u32 header = reader.Dword(kNewHeaderAt);

    if (reader.Dword(header) != kPeSignature)
    {
        return false;
    }

    const u16 count = reader.Word(header + kSectionCountAt);
    const u16 optionalSize = reader.Word(header + kOptionalSizeAt);
    const std::size_t table = header + kOptionalHeaderAt + optionalSize;
    std::vector<Section> sections;

    for (u16 index = 0; index < count; ++index)
    {
        const std::size_t entry = table + kSectionBytes * static_cast<std::size_t>(index);
        const u32 virtualSize = reader.Dword(entry + kVirtualSizeAt);
        const u32 rawSize = reader.Dword(entry + kRawSizeAt);
        sections.push_back(
            {reader.Dword(entry + kVirtualAddressAt),
             virtualSize > rawSize ? virtualSize : rawSize,
             reader.Dword(entry + kRawAddressAt)}
        );
    }

    auto toOffset = [&](u32 address) -> std::size_t
    {
        for (const Section& section : sections)
        {
            if (address >= section.address && address - section.address < section.size)
            {
                return section.raw + (address - section.address);
            }
        }

        return this->image.size();
    };

    const u32 resourceAddress =
        reader.Dword(header + kOptionalHeaderAt + kDataDirectoriesAt + kDataDirectoryBytes * kResourceDirectory);

    if (resourceAddress == 0)
    {
        return false;
    }

    const std::size_t base = toOffset(resourceAddress);

    auto nameOf = [&](u32 value) -> std::string
    {
        const std::size_t at = base + (value & ~kNamedEntry);
        const u16 length = reader.Word(at);
        std::string name;

        for (u16 index = 0; index < length; ++index)
        {
            name.push_back(
                static_cast<char>(reader.Word(at + sizeof(u16) + sizeof(u16) * static_cast<std::size_t>(index)))
            );
        }

        return name;
    };

    Entry current;

    auto walk = [&](auto& self, std::size_t directory, int depth) -> void
    {
        if (depth >= kMaxDepth || !reader.Has(directory, kDirectoryBytes))
        {
            return;
        }

        const int items = reader.Word(directory + kNamedEntriesAt) + reader.Word(directory + kIdEntriesAt);

        for (int index = 0; index < items; ++index)
        {
            const std::size_t at = directory + kDirectoryBytes + kDirectoryEntryBytes * static_cast<std::size_t>(index);
            const u32 name = reader.Dword(at);
            const u32 target = reader.Dword(at + sizeof(u32));

            if (depth == kTypeLevel)
            {
                current.typeName = (name & kNamedEntry) != 0 ? nameOf(name) : std::string();
                current.type = (name & kNamedEntry) != 0 ? 0 : name;
            }
            else if (depth == kIdLevel)
            {
                current.id = (name & kNamedEntry) != 0 ? 0 : name;
            }

            if ((target & kNamedEntry) != 0)
            {
                self(self, base + (target & ~kNamedEntry), depth + 1);
                continue;
            }

            Entry found = current;
            found.offset = toOffset(reader.Dword(base + target));
            found.size = reader.Dword(base + target + sizeof(u32));

            if (reader.Has(found.offset, found.size))
            {
                this->entries.push_back(found);
            }
        }
    };

    walk(walk, base, 0);
    return !this->entries.empty();
}

std::span<const u8> ExeResources::Find(u32 type, u32 id) const
{
    for (const Entry& entry : this->entries)
    {
        if (entry.typeName.empty() && entry.type == type && entry.id == id)
        {
            return std::span<const u8>(this->image).subspan(entry.offset, entry.size);
        }
    }

    return {};
}

std::span<const u8> ExeResources::Find(std::string_view type, u32 id) const
{
    for (const Entry& entry : this->entries)
    {
        if (entry.typeName == type && entry.id == id)
        {
            return std::span<const u8>(this->image).subspan(entry.offset, entry.size);
        }
    }

    return {};
}

bool ExeResources::FindIcon(u32 groupType, u32 group, IconImage& out) const
{
    const std::span<const u8> directory = this->Find(groupType, group);
    const Reader reader(directory);

    if (!reader.Has(0, kGroupEntriesAt))
    {
        return false;
    }

    const u16 count = reader.Word(kGroupCountAt);
    u16 bestBits = 0;
    u32 best = 0;

    for (u16 index = 0; index < count; ++index)
    {
        const std::size_t entry = kGroupEntriesAt + kGroupEntryBytes * index;

        if (!reader.Has(entry, kGroupEntryBytes))
        {
            break;
        }

        const u16 bits = reader.Word(entry + kGroupBitsAt);

        if (best == 0 || bits > bestBits)
        {
            bestBits = bits;
            best = reader.Word(entry + kGroupIdAt);
        }
    }

    if (best == 0)
    {
        return false;
    }

    const bool cursor = groupType == ExeResources::GroupCursor;
    const std::span<const u8> member = this->Find(cursor ? ExeResources::Cursor : ExeResources::Icon, best);
    const Reader memberReader(member);

    if (cursor && !memberReader.Has(0, kCursorBitmapAt))
    {
        return false;
    }

    if (!DecodeIconBitmap(cursor ? member.subspan(kCursorBitmapAt) : member, out))
    {
        return false;
    }

    out.hotX = cursor ? memberReader.Word(0) : 0;
    out.hotY = cursor ? memberReader.Word(kHotYAt) : 0;
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
