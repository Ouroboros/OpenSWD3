#pragma once

// Path finder scenarios shared by the legacy golden generator and the PathFinder test: random maps with a blocked
// border four tiles wide, so that no search reads outside the map.

#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

struct PathScenario
{
    std::vector<u32> cells;
    int mapWidth = 0;
    int fromX = 0;
    int fromY = 0;
    int toX = 0;
    int toY = 0;
    int width = 1;
    int height = 1;
    u32 mask = 0x40800000;
};

class PathBackend
{
public:
    virtual ~PathBackend() = default;
    // The result, the directions up to the end mark, then the pool's free and peak counts; `again` runs a second
    // search with the same finder.
    virtual std::vector<u8> Find(const PathScenario& scenario, bool again) = 0;
};

inline std::vector<PathScenario> MakePathScenarios()
{
    std::vector<PathScenario> scenarios;
    constexpr int kWidth = 48;
    constexpr int kHeight = 36;
    constexpr int kBorder = 4;
    const u32 bits[] = {0x40000000, 0x00800000, 0x00000001, 0x00010000};

    for (u32 seed = 1; seed <= 160; ++seed)
    {
        PathScenario scenario;
        scenario.mapWidth = kWidth;
        scenario.cells.assign(static_cast<std::size_t>(kWidth) * kHeight, 0);
        const u32 density = 4 + seed % 9;

        for (int y = 0; y < kHeight; ++y)
        {
            for (int x = 0; x < kWidth; ++x)
            {
                u32& cell = scenario.cells[static_cast<std::size_t>(y) * kWidth + x];
                const u32 noise = Mix32(seed * 7919u + static_cast<u32>(y * kWidth + x));

                if (x < kBorder || y < kBorder || x >= kWidth - kBorder || y >= kHeight - kBorder)
                {
                    cell = 0xFFFFFFFF;
                }
                else if (noise % 32 < density)
                {
                    cell = bits[noise >> 28 & 3];
                }
            }
        }

        const u32 a = Mix32(seed * 31u + 7u);
        const u32 b = Mix32(seed * 131u + 3u);
        scenario.width = 1 + static_cast<int>(seed % 3);
        scenario.height = 1 + static_cast<int>((seed / 3) % 3);
        const int spanX = kWidth - 2 * kBorder - scenario.width;
        const int spanY = kHeight - 2 * kBorder - scenario.height;
        scenario.fromX = 16 * (kBorder + static_cast<int>(a % static_cast<u32>(spanX))) + static_cast<int>(a >> 28);
        scenario.fromY = 16 * (kBorder + static_cast<int>((a >> 8) % static_cast<u32>(spanY)));
        scenario.toX = 16 * (kBorder + static_cast<int>(b % static_cast<u32>(spanX)));
        scenario.toY =
            16 * (kBorder + static_cast<int>((b >> 8) % static_cast<u32>(spanY))) + static_cast<int>(b >> 28);
        scenario.mask = seed % 5 == 0 ? 0x40000000u : (seed % 7 == 0 ? 0x00010001u : 0x40800000u);

        if (seed % 11 == 0)
        {
            scenario.toX = scenario.fromX;
            scenario.toY = scenario.fromY;
        }

        scenarios.push_back(std::move(scenario));
    }

    return scenarios;
}

inline void RunPaths(PathBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    const std::vector<PathScenario> scenarios = MakePathScenarios();

    for (std::size_t i = 0; i < scenarios.size(); ++i)
    {
        for (const bool again : {false, true})
        {
            const std::vector<u8> result = backend.Find(scenarios[i], again);
            report("path/" + std::to_string(i) + (again ? "/again" : ""), Hash(result.data(), result.size()));
        }
    }
}

}  // namespace golden
