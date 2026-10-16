#pragma once

// tsw archive scenarios shared by the legacy golden generator and the TswArchive test (see draw_scenarios.h).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

struct PictureResult
{
    bool present = false;
    std::vector<u8> data;
    int width = 0;
    int height = 0;
    std::size_t paletteEntries = 0;
};

class TswBackend
{
public:
    virtual ~TswBackend() = default;
    virtual void SetCacheLimit(int bytes) = 0;
    virtual PictureResult Get(u32 id, u32 frame) = 0;
    virtual PictureResult Find(u32 id, u32 frame) = 0;
    virtual PictureResult Direct(u32 id, u32 frame) = 0;
    virtual int CacheBytes() = 0;
    virtual void CloseAll() = 0;
};

inline u64 HashPicture(const PictureResult& result)
{
    std::vector<u8> bytes = result.data;
    Append32(bytes, result.present ? 1 : 0);
    Append32(bytes, static_cast<u32>(result.width));
    Append32(bytes, static_cast<u32>(result.height));
    Append32(bytes, static_cast<u32>(result.paletteEntries));
    return Hash(bytes.data(), bytes.size());
}

inline void RunTsw(TswBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    // Existing frames only (a frame beyond the entry reads picture data as frame records): character 7, 23, 1 (8-bit),
    // item 3001, system 9014 and 9001 (8-bit), map 12001 and 15001, magic 6001 and 6010.
    const u32 requests[][2] = {
        {7, 0},
        {23, 0},
        {23, 1},
        {23, 2},
        {1, 0},
        {1, 1},
        {1, 2},
        {3001, 0},
        {9014, 0},
        {9001, 0},
        {12001, 0},
        {12001, 1},
        {12001, 2},
        {15001, 0},
        {6001, 0},
        {6001, 2},
        {6010, 0},
        {6010, 30},
    };

    const u32 ids[] = {7, 23, 1, 3001, 9014, 9001, 12001, 15001, 6001, 6010};
    backend.SetCacheLimit(300000);
    int step = 0;

    for (int round = 0; round < 3; ++round)
    {
        for (const auto& request : requests)
        {
            const std::string name =
                "get/" + std::to_string(step++) + "/" + std::to_string(request[0]) + "/" + std::to_string(request[1]);
            report(name, HashPicture(backend.Get(request[0], request[1])));
            report(name + "/bytes", static_cast<u64>(backend.CacheBytes()));
        }

        for (u32 id : ids)
        {
            report("find/" + std::to_string(round) + "/" + std::to_string(id), HashPicture(backend.Find(id, 0)));
        }
    }

    for (const auto& request : requests)
    {
        // Directly loaded magic pictures carry the palette pointer that 0x00431AA0 leaves uninitialised for 16-bit
        // entries; the palette is left out of the comparison.
        PictureResult result = backend.Direct(request[0], request[1]);
        result.paletteEntries = 0;
        report("direct/" + std::to_string(request[0]) + "/" + std::to_string(request[1]), HashPicture(result));
    }

    backend.CloseAll();
    report("closed/bytes", static_cast<u64>(backend.CacheBytes()));
    report("closed/find", HashPicture(backend.Find(7, 0)));
    report("reopened", HashPicture(backend.Get(23, 1)));
}

}  // namespace golden
