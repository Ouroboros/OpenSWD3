#pragma once

#include "core/types.h"

namespace swd3
{

class Config;
class Painter;

// The cache of decompressed map units in <install directory>data\0.cm .. 23.cm with their table in mcache.dat. The
// last byte of env.dat marks a session that did not end cleanly: the next start empties the cache.
class MapCache
{
public:
    static constexpr int kUnits = 0x18;
    static constexpr u32 kTableBytes = 0x180;

    // One entry of mcache.dat (0x10 bytes).
    struct Slot
    {
        u32 map = 0xFFFFFFFF;  // the cached map, 0xFFFFFFFF: free
        u32 bytes = 0;
        u32 age = 0;
        u32 unit = 0;  // the .cm file
    };

    MapCache(const Config& config, const Painter& painter);

    u32 Check(int mode);
    void MarkClean();
    void MarkDirty();
    u32 CacheBytes() const;

private:
    void Clear();

    const Config& config;
    const Painter& painter;
};

}  // namespace swd3
