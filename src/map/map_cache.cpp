#include "map/map_cache.h"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <string>

#include "codec/lzo.h"
#include "config/config.h"
#include "core/byte_reader.h"
#include "core/format.h"
#include "core/log.h"
#include "draw/painter.h"
#include "file/file.h"
#include "game/game_state.h"

namespace swd3
{

namespace
{

constexpr std::string_view kUnitFormat = "%sdata\\%d.cm";
constexpr std::string_view kTableFile = "mcache.dat";

}  // namespace

MapCache::MapCache(const Config& config, const Painter& painter)
    : config(config),
      painter(painter)
{
}

// swd32.exe 0x00425570: mode 0 empties the cache when the session mark is set (and clears it); mode 1 empties it when
// the display's colour masks differ from those recorded (and records the new ones). The original keeps both in Env.dat
// (the mark in its last byte, the masks at 0x1E); the rebuild in [game] (session_open, red_mask ...).
// The bytes the cache holds afterwards.
// swd32.exe 0x00498190 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x0049819B (EH unwind: the new object freed (operator delete))
// swd32.exe 0x004981A6 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x004981B1 (EH handler of 0x00425570)
u32 MapCache::Check(int mode)
{
    bool clear = false;

    if (mode == MapCache::CheckSessionMark)
    {
        this->Store(
            [&clear](GameSettings& game)
            {
                clear = game.sessionOpen.value_or(false);
                game.sessionOpen = false;
            }
        );
    }
    else if (mode == MapCache::CheckColorMasks)
    {
        u32 red = 0;
        u32 green = 0;
        u32 blue = 0;
        this->painter.ColorMasks(red, green, blue);
        const std::array<u16, Config::Channels>& recorded = this->config.channelMasks;

        if (red != recorded[Config::RedChannel] || green != recorded[Config::GreenChannel] ||
            blue != recorded[Config::BlueChannel])
        {
            clear = true;
            this->Store(
                [red, green, blue](GameSettings& game)
                {
                    game.redMask = static_cast<u16>(red);
                    game.greenMask = static_cast<u16>(green);
                    game.blueMask = static_cast<u16>(blue);
                }
            );
        }
    }

    if (clear)
    {
        this->Clear();
    }

    return this->CacheBytes();
}

// Empties every unit file and resets the table.
void MapCache::Clear()
{
    for (int unit = 0; unit < MapCache::Units; ++unit)
    {
        File file;

        if (file.Open(
                PathFormat(kUnitFormat, {this->config.installDirectory, unit}),
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
            PathFormat("%sdata\\%s", {this->config.installDirectory, kTableFile}),
            File::Disposition::OpenExisting,
            File::Access::ReadWrite
        ))
    {
        std::array<Slot, MapCache::Units> slots;

        for (int unit = 0; unit < MapCache::Units; ++unit)
        {
            slots[static_cast<std::size_t>(unit)].unit = static_cast<u32>(unit);
        }

        u32 bytes = MapCache::TableBytes;
        table.SeekBegin(0);
        table.Truncate();
        table.Write(slots.data(), bytes);
    }

    table.Close();
}

// swd32.exe 0x004258E0: clears the session mark (at a clean exit)
// swd32.exe 0x004981C0 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x004981CB (EH handler of 0x004258E0)
void MapCache::MarkClean()
{
    this->Store(
        [](GameSettings& game)
        {
            game.sessionOpen = false;
        }
    );
}

// swd32.exe 0x004259B0: sets the session mark (once the game runs)
// swd32.exe 0x004981E0 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x004981EB (EH handler of 0x004259B0)
void MapCache::MarkDirty()
{
    this->Store(
        [](GameSettings& game)
        {
            game.sessionOpen = true;
        }
    );
}

// [game] of the program's settings file changed by `change` (the original writes Env.dat; a failure is logged).
void MapCache::Store(const std::function<void(GameSettings&)>& change) const
{
    std::string error;

    if (!UpdateSettingsFile(
            this->config.settingsFile,
            [&change](Settings& settings)
            {
                change(settings.game);
            },
            error
        ))
    {
        LogWarning(u8"settings: the map cache's [game] values cannot be saved: %s", AsUtf8(error));
    }
}

// swd32.exe 0x00425A70: the sizes of the unit files that exist
// swd32.exe 0x00498200 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x0049820B (EH handler of 0x00425A70)
u32 MapCache::CacheBytes() const
{
    u32 total = 0;

    for (int unit = 0; unit < MapCache::Units; ++unit)
    {
        File file;

        if (file.Open(
                PathFormat(kUnitFormat, {this->config.installDirectory, unit}),
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

// The unit header of a map in HUGE.lmf: the unpacked size, the block size, then a length per block.
constexpr std::size_t kUnpackedAt = 0x10;
constexpr std::size_t kBlockSizeAt = 0x14;
constexpr std::size_t kBlocksAt = 0x1C;
constexpr std::size_t kBlockEntryBytes = 8;
constexpr int kSlackShift = 10;     // a block buffer has 1/1024 to spare for packed data
constexpr u32 kProgressStart = 15;  // the loading screen runs from 15 to 60 percent while unpacking
constexpr u32 kProgressSpan = 45;
constexpr int kMegabyteShift = 20;
constexpr i32 kAgeShare = 4;  // a slot older than a quarter of the cache size in megabytes goes first

u32 HeaderDword(const std::array<u8, MapCache::HeaderBytes>& header, std::size_t at)
{
    return ByteReader::Word32At(header, at);
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

    this->lmf.SeekBegin(static_cast<i32>(base + size + kUnpackedAt));
    this->lmf.ReadDword(bytes);
}

// swd32.exe 0x00426DF0: unpacks the unit of a map from HUGE.lmf block by block into data\<unit>.cm (16-bit pixels
// converted to the display), the loading screen from 15 to 60
// swd32.exe 0x00498250 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498258 (EH handler of 0x00426DF0)
void MapCache::DecompressMap(int unit, u32 base, u32 size, int bitsPerPixel, u32& bytes)
{
    if (size == 0)
    {
        return;
    }

    File cached;

    if (!cached.Open(
            PathFormat(kUnitFormat, {this->config.installDirectory, unit}),
            File::Disposition::OpenAlways,
            File::Access::ReadWrite
        ))
    {
        return;
    }

    cached.SeekBegin(0);
    this->lmf.SeekBegin(static_cast<i32>(size + base));
    std::array<u8, MapCache::HeaderBytes> header = {};
    u32 read = MapCache::HeaderBytes;

    if (!this->lmf.Read(header.data(), read))
    {
        cached.Close();
        return;
    }

    const u32 total = HeaderDword(header, kUnpackedAt);
    bytes = total;
    const u32 blockSize = HeaderDword(header, kBlockSizeAt);

    if (blockSize == 0)
    {
        cached.Close();
        return;
    }

    std::vector<u8> packed((blockSize >> kSlackShift) + blockSize, 0);
    std::vector<u8> output((blockSize >> kSlackShift) + blockSize, 0);
    const u32 blocks = (blockSize + total) / blockSize;
    u32 remaining = total;
    u32 step = 0;

    for (u32 block = 0; block < blocks; ++block)
    {
        const std::size_t entry = kBlocksAt + kBlockEntryBytes * static_cast<std::size_t>(block);

        if (entry + sizeof(u32) > header.size())
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

        if (bitsPerPixel == MapHeader::TrueColorBits)
        {
            this->painter.ConvertPixels(std::span<u16>(reinterpret_cast<u16*>(output.data()), written / sizeof(u16)));
        }

        cached.Write(output.data(), written);
        remaining -= blockSize;

        if (this->progress)
        {
            this->progress(static_cast<int>(step / blocks + kProgressStart));
        }

        step += kProgressSpan;
    }

    cached.Close();
}

// swd32.exe 0x00427140: maps data\<unit>.cm as the view of the map (the previous view released)
// swd32.exe 0x00498270 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498278 (EH handler of 0x00427140)
const u8* MapCache::OpenUnit(int unit)
{
    const std::u8string path = PathFormat(kUnitFormat, {this->config.installDirectory, unit});
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
// swd32.exe 0x00498220 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x0049822B (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498233 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x0049823E (EH handler of 0x00426840)
const u8* MapCache::GetCachedMap(u32 map, u32 base, u32 size, int bitsPerPixel)
{
    File table;
    const std::u8string tablePath = PathFormat("%sdata\\%s", {this->config.installDirectory, kTableFile});

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

        for (int unit = 1; unit < MapCache::Units; ++unit)
        {
            slots[static_cast<std::size_t>(unit)].unit = static_cast<u32>(unit);
        }

        u32 written = MapCache::TableBytes;
        table.SeekBegin(0);
        table.Write(slots.data(), written);
        table.Truncate();
        table.Close();
        return this->OpenUnit(0);
    }

    table.SeekBegin(0);
    u32 read = std::min<u32>(bytes, sizeof(slots));
    table.Read(slots.data(), read);
    const u32 count = bytes / sizeof(Slot);
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
        u32 written = sizeof(Slot) * kept;
        table.Write(slots.data(), written);
        table.Close();
        return opened;
    }

    u32 needed = 0;
    this->ReadUnitBytes(base, size, needed);
    bool full = true;

    for (u32 slot = 0; slot < kept; ++slot)
    {
        if (slots[slot].map == Slot::Free)
        {
            full = false;
            break;
        }
    }

    const auto limit = static_cast<u32>(this->config.cacheSize) << kMegabyteShift;
    u32 victim = count;
    u32 victimUnit = 0;

    while ((used + needed > limit && count != 0) || full)
    {
        u32 smallest = std::numeric_limits<u32>::max();

        for (u32 slot = 0; slot < kept; ++slot)
        {
            const Slot& entry = slots[slot];

            if (entry.map == Slot::Free)
            {
                continue;
            }

            if (entry.bytes < smallest)
            {
                smallest = entry.bytes;
                victimUnit = entry.unit;
                victim = slot;
            }

            if (entry.age > static_cast<u32>(this->config.cacheSize / kAgeShare))
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
            slots[victim] = {Slot::Free, 0, 0, victim};
        }

        File cached;

        if (cached.Open(
                PathFormat(kUnitFormat, {this->config.installDirectory, static_cast<i32>(victimUnit)}),
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

    while (free < kept && slots[free].map != Slot::Free)
    {
        ++free;
    }

    if (free < kTableSlots)
    {
        slots[free] = {map, needed, 0, free};
    }

    table.SeekBegin(0);
    u32 written = sizeof(Slot) * kept;
    table.Write(slots.data(), written);
    table.Truncate();
    table.Close();
    this->DecompressMap(static_cast<int>(free), base, size, bitsPerPixel, needed);
    return this->OpenUnit(static_cast<int>(free));
}

}  // namespace swd3
