#include "map/map_cache.h"

#include <array>

#include "config/config.h"
#include "core/format.h"
#include "draw/painter.h"
#include "file/file.h"

namespace swd3
{

namespace
{

constexpr char kEnvFile[] = "env.dat";
constexpr char kUnitFormat[] = "%sdata\\%d.cm";
constexpr char kTableFile[] = "mcache.dat";
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

    if (file.Open(this->config.installDirectory + kEnvFile, File::Disposition::OpenExisting, File::Access::ReadWrite))
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
                WinFormat(kUnitFormat, {this->config.installDirectory.c_str(), unit}),
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
            WinFormat("%sdata\\%s", {this->config.installDirectory.c_str(), kTableFile}),
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

    if (file.Open(this->config.installDirectory + kEnvFile, File::Disposition::OpenExisting, File::Access::Write))
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

    if (file.Open(this->config.installDirectory + kEnvFile, File::Disposition::OpenExisting, File::Access::Write))
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
                WinFormat(kUnitFormat, {this->config.installDirectory.c_str(), unit}),
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

}  // namespace swd3
