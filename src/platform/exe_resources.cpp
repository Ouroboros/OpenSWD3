#include "platform/exe_resources.h"

#include <cstring>
#include <fstream>
#include <iterator>

#include "platform/file_system.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr u32 kResourceDirectory = 2;
constexpr u32 kNamedEntry = 0x80000000u;
constexpr int kMaxDepth = 3;

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
        return this->Has(offset, 2) ? static_cast<u16>(this->bytes[offset] | this->bytes[offset + 1] << 8) : u16{0};
    }

    u32 Dword(std::size_t offset) const
    {
        return this->Has(offset, 4)
                   ? static_cast<u32>(this->Word(offset) | static_cast<u32>(this->Word(offset + 2)) << 16)
                   : 0u;
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

}  // namespace

bool ExeResources::Open(const std::string& path)
{
    std::ifstream file(ResolvePath(path), std::ios::binary);

    if (!file)
    {
        return false;
    }

    this->image.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    this->entries.clear();
    return this->Parse();
}

bool ExeResources::Parse()
{
    const Reader reader(this->image);

    if (reader.Word(0) != 0x5A4D)
    {
        return false;
    }

    const u32 header = reader.Dword(0x3C);

    if (reader.Dword(header) != 0x4550)
    {
        return false;
    }

    const u16 count = reader.Word(header + 6);
    const u16 optionalSize = reader.Word(header + 20);
    const std::size_t table = header + 24 + optionalSize;
    std::vector<Section> sections;

    for (u16 index = 0; index < count; ++index)
    {
        const std::size_t entry = table + 40 * static_cast<std::size_t>(index);
        const u32 virtualSize = reader.Dword(entry + 8);
        const u32 rawSize = reader.Dword(entry + 16);
        sections.push_back(
            {reader.Dword(entry + 12), virtualSize > rawSize ? virtualSize : rawSize, reader.Dword(entry + 20)}
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

    const u32 resourceAddress = reader.Dword(header + 24 + 96 + 8 * kResourceDirectory);

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
            name.push_back(static_cast<char>(reader.Word(at + 2 + 2 * static_cast<std::size_t>(index))));
        }

        return name;
    };

    Entry current;

    auto walk = [&](auto& self, std::size_t directory, int depth) -> void
    {
        if (depth >= kMaxDepth || !reader.Has(directory, 16))
        {
            return;
        }

        const int items = reader.Word(directory + 12) + reader.Word(directory + 14);

        for (int index = 0; index < items; ++index)
        {
            const std::size_t at = directory + 16 + 8 * static_cast<std::size_t>(index);
            const u32 name = reader.Dword(at);
            const u32 target = reader.Dword(at + 4);

            if (depth == 0)
            {
                current.typeName = (name & kNamedEntry) != 0 ? nameOf(name) : std::string();
                current.type = (name & kNamedEntry) != 0 ? 0 : name;
            }
            else if (depth == 1)
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
            found.size = reader.Dword(base + target + 4);

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

bool DecodeBitmap(std::span<const u8> bitmap, Surface& out)
{
    const Reader reader(bitmap);
    const u32 headerSize = reader.Dword(0);
    const auto width = static_cast<i32>(reader.Dword(4));
    const auto height = static_cast<i32>(reader.Dword(8));
    const u16 bits = reader.Word(14);

    if (headerSize < 40 || reader.Dword(16) != 0 || width <= 0 || height == 0 || (bits != 8 && bits != 24))
    {
        return false;
    }

    const int rows = height > 0 ? height : -height;
    u32 colors = reader.Dword(32);

    if (bits == 8 && colors == 0)
    {
        colors = 256;
    }

    const std::size_t palette = headerSize;
    const std::size_t pixels = palette + (bits == 8 ? 4 * static_cast<std::size_t>(colors) : 0);
    const std::size_t stride = (static_cast<std::size_t>(width) * bits / 8 + 3) & ~std::size_t{3};

    if (!reader.Has(pixels, stride * static_cast<std::size_t>(rows)))
    {
        return false;
    }

    out.Resize(width, rows);

    auto pack = [](u32 blue, u32 green, u32 red)
    {
        return static_cast<u16>((red >> 3) << 11 | (green >> 2) << 5 | blue >> 3);
    };

    for (int row = 0; row < rows; ++row)
    {
        const std::size_t source = pixels + stride * static_cast<std::size_t>(height > 0 ? rows - 1 - row : row);
        u16* target = out.Row(row);

        for (int column = 0; column < width; ++column)
        {
            if (bits == 8)
            {
                const u8 index = bitmap[source + static_cast<std::size_t>(column)];
                const std::size_t entry = palette + 4 * static_cast<std::size_t>(index < colors ? index : 0);
                target[column] = pack(bitmap[entry], bitmap[entry + 1], bitmap[entry + 2]);
            }
            else
            {
                const std::size_t at = source + 3 * static_cast<std::size_t>(column);
                target[column] = pack(bitmap[at], bitmap[at + 1], bitmap[at + 2]);
            }
        }
    }

    return true;
}

}  // namespace swd3
