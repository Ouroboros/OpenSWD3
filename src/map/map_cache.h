#pragma once

#include <functional>
#include <span>
#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"

namespace swd3
{

class Config;
class Painter;

// The cache of decompressed map units in <install directory>data\0.cm .. 23.cm with their table in mcache.dat. The
// last byte of env.dat marks a session that did not end cleanly: the next start empties the cache.
class MapCache
{
public:
    static constexpr int Units = 0x18;
    static constexpr u32 TableBytes = 0x180;

    // One entry of mcache.dat (0x10 bytes).
    struct Slot
    {
        u32 map = 0xFFFFFFFF;  // the cached map, 0xFFFFFFFF: free
        u32 bytes = 0;
        u32 age = 0;
        u32 unit = 0;  // the .cm file
    };

    static constexpr u32 HeaderBytes = 0x1A8;

    MapCache(const Config& config, const Painter& painter);

    u32 Check(int mode);
    void MarkClean();
    void MarkDirty();
    u32 CacheBytes() const;
    const u8* GetCachedMap(u32 map, u32 base, u32 size, int bitsPerPixel);
    void DecompressMap(int unit, u32 base, u32 size, int bitsPerPixel, u32& bytes);
    void ReadUnitBytes(u32 base, u32 size, u32& bytes);
    const u8* OpenUnit(int unit);
    static std::vector<u8> Pack(std::span<const u8> data);
    static void Unpack(std::span<const u8> packed, std::span<u8> output);

    File lmf;                           // 0x004B8868 HUGE.lmf
    File unitFile;                      // 0x004CAA28 the mapped unit
    const u8* view = nullptr;           // the mapped view of unitFile (the map header's pixels)
    std::string unitPath;               // 0x004A9ACC
    std::function<void(int)> progress;  // the loading screen at a percentage

private:
    void Clear();

    const Config& config;
    const Painter& painter;
};

}  // namespace swd3
