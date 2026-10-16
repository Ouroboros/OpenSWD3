#include "map/map_cache.h"

#include <algorithm>
#include <array>

#include "codec/lzo.h"
#include "config/config.h"
#include "core/format.h"
#include "draw/painter.h"
#include "file/file.h"

namespace swd3
{

namespace
{

constexpr std::string_view kEnvFile = "env.dat";
constexpr std::string_view kUnitFormat = "%sdata\\%d.cm";
constexpr std::string_view kTableFile = "mcache.dat";
constexpr i32 kMaskOffset = 0x1E;

}  // namespace

MapCache::MapCache(const Config& config, const Painter& painter) : config(config), painter(painter)
{
}

// swd32.exe 0x00425570: mode 0 empties the cache when the session mark at the end of env.dat is set (and clears it);
// mode 1 empties it when the display's colour masks differ from those env.dat records (and records the new ones).
// The bytes the cache holds afterwards.
u32 MapCache::Check(int mode)
{
    File file;
    bool clear = false;

    if (file.Open(
            Concat({this->config.installDirectory, kEnvFile}),
            File::Disposition::OpenExisting,
            File::Access::ReadWrite
        ))
    {
        if (mode == 0)
        {
            file.SeekEnd(-1);
            u8 mark = 0;
            file.ReadByte(mark);

            if (mark != 0)
            {
                file.SeekEnd(-1);
                file.WriteByte(0);
                clear = true;
            }
        }
        else if (mode == 1)
        {
            u32 red = 0;
            u32 green = 0;
            u32 blue = 0;
            this->painter.ColorMasks(red, green, blue);
            const std::array<u16, 3>& recorded = this->config.channelMasks;

            if (red != recorded[0] || green != recorded[1] || blue != recorded[2])
            {
                clear = true;
                std::array<u16, 0x20> masks = {};
                u32 bytes = 6;
                file.SeekBegin(kMaskOffset);
                file.Read(masks.data(), bytes);
                masks[0] = static_cast<u16>(red);
                masks[1] = static_cast<u16>(green);
                masks[2] = static_cast<u16>(blue);
                file.SeekBegin(kMaskOffset);
                bytes = 6;
                file.Write(masks.data(), bytes);
            }
        }
    }

    file.Close();

    if (clear)
    {
        this->Clear();
    }

    return this->CacheBytes();
}

// Empties every unit file and resets the table.
void MapCache::Clear()
{
    for (int unit = 0; unit < MapCache::kUnits; ++unit)
    {
        File file;

        if (file.Open(
                WinFormat(kUnitFormat, {this->config.installDirectory, unit}),
                File::Disposition::OpenExisting,
                File::Access::ReadWrite
            ))
        {
            file.SeekBegin(0);
            file.Truncate();
        }

        file.Close();
    }

    File table;

    if (table.Open(
            WinFormat("%sdata\\%s", {this->config.installDirectory, kTableFile}),
            File::Disposition::OpenExisting,
            File::Access::ReadWrite
        ))
    {
        std::array<Slot, MapCache::kUnits> slots;

        for (int unit = 0; unit < MapCache::kUnits; ++unit)
        {
            slots[static_cast<std::size_t>(unit)].unit = static_cast<u32>(unit);
        }

        u32 bytes = MapCache::kTableBytes;
        table.SeekBegin(0);
        table.Truncate();
        table.Write(slots.data(), bytes);
    }

    table.Close();
}

// swd32.exe 0x004258E0: clears the session mark (at a clean exit)
void MapCache::MarkClean()
{
    File file;

    if (file.Open(
            Concat({this->config.installDirectory, kEnvFile}),
            File::Disposition::OpenExisting,
            File::Access::Write
        ))
    {
        file.SeekEnd(-1);
        file.WriteByte(0);
    }

    file.Close();
}

// swd32.exe 0x004259B0: sets the session mark (once the game runs)
void MapCache::MarkDirty()
{
    File file;

    if (file.Open(
            Concat({this->config.installDirectory, kEnvFile}),
            File::Disposition::OpenExisting,
            File::Access::Write
        ))
    {
        file.SeekEnd(-1);
        file.WriteByte(1);
    }

    file.Close();
}

// swd32.exe 0x00425A70: the sizes of the unit files that exist
u32 MapCache::CacheBytes() const
{
    u32 total = 0;

    for (int unit = 0; unit < MapCache::kUnits; ++unit)
    {
        File file;

        if (file.Open(
                WinFormat(kUnitFormat, {this->config.installDirectory, unit}),
                File::Disposition::OpenExisting,
                File::Access::Read
            ))
        {
            total += file.Size();
        }

        file.Close();
    }

    return total;
}

namespace
{

constexpr int kTableSlots = 0x20;  // the 0x200-byte buffer the table is read into
constexpr u32 kKeptHeader = 0x10;
constexpr int kDictionaryBits = 14;

u32 HeaderDword(const std::array<u8, MapCache::kHeaderBytes>& header, std::size_t at)
{
    return static_cast<u32>(
        header[at] | header[at + 1] << 8 | header[at + 2] << 16 | static_cast<u32>(header[at + 3]) << 24
    );
}

}  // namespace

// swd32.exe 0x004267E0: 14-bit LZO1X-1
std::vector<u8> MapCache::Pack(std::span<const u8> data)
{
    return LzoEncoder(kDictionaryBits).Compress(data);
}

// swd32.exe 0x00426820
void MapCache::Unpack(std::span<const u8> packed, std::span<u8> output)
{
    LzoDecoder(packed, output).Run();
}

// swd32.exe 0x004270F0: the unpacked size of a map's unit (the dword at +0x10 of its block header); nothing when the
// map has no unit
void MapCache::ReadUnitBytes(u32 base, u32 size, u32& bytes)
{
    if (size == 0)
    {
        return;
    }

    this->lmf.SeekBegin(static_cast<i32>(base + size + 0x10));
    this->lmf.ReadDword(bytes);
}

// swd32.exe 0x00426DF0: unpacks the unit of a map from HUGE.lmf block by block into data\<unit>.cm (16-bit pixels
// converted to the display), the loading screen from 15 to 60
void MapCache::DecompressMap(int unit, u32 base, u32 size, int bitsPerPixel, u32& bytes)
{
    if (size == 0)
    {
        return;
    }

    File cached;

    if (!cached.Open(
            WinFormat(kUnitFormat, {this->config.installDirectory, unit}),
            File::Disposition::OpenAlways,
            File::Access::ReadWrite
        ))
    {
        return;
    }

    cached.SeekBegin(0);
    this->lmf.SeekBegin(static_cast<i32>(size + base));
    std::array<u8, MapCache::kHeaderBytes> header = {};
    u32 read = MapCache::kHeaderBytes;

    if (!this->lmf.Read(header.data(), read))
    {
        cached.Close();
        return;
    }

    const u32 total = HeaderDword(header, 0x10);
    bytes = total;
    const u32 blockSize = HeaderDword(header, 0x14);

    if (blockSize == 0)
    {
        cached.Close();
        return;
    }

    std::vector<u8> packed((blockSize >> 0xA) + blockSize, 0);
    std::vector<u8> output((blockSize >> 0xA) + blockSize, 0);
    const u32 blocks = (blockSize + total) / blockSize;
    u32 remaining = total;
    u32 step = 0;

    for (u32 block = 0; block < blocks; ++block)
    {
        const std::size_t entry = 0x1C + 8 * static_cast<std::size_t>(block);

        if (entry + 4 > header.size())
        {
            break;
        }

        u32 length = HeaderDword(header, entry);
        packed.resize(std::max<std::size_t>(packed.size(), length));

        if (!this->lmf.Read(packed.data(), length))
        {
            cached.Close();
            return;
        }

        MapCache::Unpack(std::span<const u8>(packed).first(length), output);
        u32 written = std::min(blockSize, remaining);

        if (bitsPerPixel == 0x10)
        {
            this->painter.ConvertPixels(std::span<u16>(reinterpret_cast<u16*>(output.data()), written >> 1));
        }

        cached.Write(output.data(), written);
        remaining -= blockSize;

        if (this->progress)
        {
            this->progress(static_cast<int>(step / blocks + 0xF));
        }

        step += 0x2D;
    }

    cached.Close();
}

// swd32.exe 0x00427140: maps data\<unit>.cm as the view of the map (the previous view released)
const u8* MapCache::OpenUnit(int unit)
{
    const std::string path = WinFormat(kUnitFormat, {this->config.installDirectory, unit});
    this->unitFile.UnmapView(this->view);
    this->view = nullptr;
    this->unitFile.CloseMapping();
    this->unitFile.Close();
    this->unitPath.clear();

    if (!this->unitFile.Open(path, File::Disposition::OpenAlways, File::Access::Read))
    {
        return nullptr;
    }

    this->unitFile.CreateMapping();
    this->view = this->unitFile.MapView(0, 0);
    this->unitPath = path;
    return this->view;
}

// swd32.exe 0x00426840: the view of map `map` from the cache: every slot ages; a cached map is opened (its age back to
// 0); otherwise slots are emptied (an old one, else the smallest) while the cache would exceed its size or no slot is
// free, and the map is unpacked into the first free slot. An empty table is started with the map in slot 0.
const u8* MapCache::GetCachedMap(u32 map, u32 base, u32 size, int bitsPerPixel)
{
    File table;
    const std::string tablePath = WinFormat("%sdata\\%s", {this->config.installDirectory, kTableFile});

    if (!table.Open(tablePath, File::Disposition::OpenAlways, File::Access::ReadWrite))
    {
        return nullptr;
    }

    std::array<Slot, kTableSlots> slots;
    u32 bytes = table.Size();

    if (bytes == 0)
    {
        u32 unpacked = 0;
        this->DecompressMap(0, base, size, bitsPerPixel, unpacked);
        slots[0] = {map, unpacked, 0, 0};

        for (int unit = 1; unit < MapCache::kUnits; ++unit)
        {
            slots[static_cast<std::size_t>(unit)].unit = static_cast<u32>(unit);
        }

        u32 written = MapCache::kTableBytes;
        table.SeekBegin(0);
        table.Write(slots.data(), written);
        table.Truncate();
        table.Close();
        return this->OpenUnit(0);
    }

    table.SeekBegin(0);
    u32 read = std::min<u32>(bytes, sizeof(slots));
    table.Read(slots.data(), read);
    const u32 count = bytes >> 4;
    const u32 kept = std::min<u32>(count, kTableSlots);

    for (u32 slot = 0; slot < kept; ++slot)
    {
        ++slots[slot].age;
    }

    u32 used = 0;
    u32 found = 0;

    while (found < kept && slots[found].map != map)
    {
        used += slots[found].bytes;
        ++found;
    }

    if (found < count && found < kept)
    {
        const u8* opened = this->OpenUnit(static_cast<int>(slots[found].unit));
        slots[found].age = 0;
        table.SeekBegin(0);
        u32 written = 0x10 * kept;
        table.Write(slots.data(), written);
        table.Close();
        return opened;
    }

    u32 needed = 0;
    this->ReadUnitBytes(base, size, needed);
    bool full = true;

    for (u32 slot = 0; slot < kept; ++slot)
    {
        if (slots[slot].map == 0xFFFFFFFF)
        {
            full = false;
            break;
        }
    }

    const auto limit = static_cast<u32>(this->config.cacheSize) << 0x14;
    u32 victim = count;
    u32 victimUnit = 0;

    while ((used + needed > limit && count != 0) || full)
    {
        u32 smallest = 0xFFFFFFFF;

        for (u32 slot = 0; slot < kept; ++slot)
        {
            const Slot& entry = slots[slot];

            if (entry.map == 0xFFFFFFFF)
            {
                continue;
            }

            if (entry.bytes < smallest)
            {
                smallest = entry.bytes;
                victimUnit = entry.unit;
                victim = slot;
            }

            if (entry.age > static_cast<u32>(this->config.cacheSize / 4))
            {
                smallest = entry.bytes;
                victimUnit = entry.unit;
                victim = slot;
                break;
            }
        }

        full = false;

        if (victim < kTableSlots)
        {
            slots[victim] = {0xFFFFFFFF, 0, 0, victim};
        }

        File cached;

        if (cached.Open(
                WinFormat(kUnitFormat, {this->config.installDirectory, static_cast<i32>(victimUnit)}),
                File::Disposition::OpenAlways,
                File::Access::ReadWrite
            ))
        {
            cached.SeekBegin(static_cast<i32>(kKeptHeader));
            cached.Truncate();
        }

        cached.Close();
        used -= smallest;
    }

    u32 free = 0;

    while (free < kept && slots[free].map != 0xFFFFFFFF)
    {
        ++free;
    }

    if (free < kTableSlots)
    {
        slots[free] = {map, needed, 0, free};
    }

    table.SeekBegin(0);
    u32 written = 0x10 * kept;
    table.Write(slots.data(), written);
    table.Truncate();
    table.Close();
    this->DecompressMap(static_cast<int>(free), base, size, bitsPerPixel, needed);
    return this->OpenUnit(static_cast<int>(free));
}

}  // namespace swd3
