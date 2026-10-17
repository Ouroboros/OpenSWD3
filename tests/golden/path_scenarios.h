#pragma once

// Path finder scenarios shared by the legacy golden generator and the PathFinder test: random maps with a blocked
// border four tiles wide, so that no search reads outside the map.

#include <functional>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

// The blocking cells (MapHeader::Cell) and the masks of the path scenarios.
constexpr u32 kWall = 0x40000000;
constexpr u32 kUnderRole = 0x00800000;
constexpr u32 kWallOrRole = kWall | kUnderRole;
constexpr u32 kRegion1 = 0x00000001;
constexpr u32 kUnkBit16 = 0x00010000;
constexpr u32 kLowBits = kRegion1 | kUnkBit16;
constexpr u32 kOutside = 0xFFFFFFFF;
constexpr std::array<u32, 4> kCellBits = {kWall, kUnderRole, kRegion1, kUnkBit16};
constexpr u32 kPathScenarios = 160;
constexpr u32 kMinDensity = 4;  // of kDensityRange cells, kMinDensity .. kMinDensity + kDensities - 1 are set
constexpr u32 kDensities = 9;
constexpr u32 kDensityRange = 32;
constexpr u32 kCellSeed = 7919;
constexpr int kBitsShift = 28;
constexpr u32 kBitsMask = 3;
constexpr u32 kFromSeed = 31;
constexpr u32 kFromOffset = 7;
constexpr u32 kToSeed = 131;
constexpr u32 kToOffset = 3;
constexpr u32 kSizes = 3;  // footprints of 1..kSizes cells a side
constexpr int kTile = 16;
constexpr int kPixelShift = 28;  // the top bits of a draw give a pixel offset in the tile
constexpr int kRowShift = 8;
constexpr u32 kWallOnlyEvery = 5;
constexpr u32 kLowBitsEvery = 7;
constexpr u32 kInPlaceEvery = 11;  // a search to the start tile

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
    u32 mask = kWallOrRole;
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

    for (u32 seed = 1; seed <= kPathScenarios; ++seed)
    {
        PathScenario scenario;
        scenario.mapWidth = kWidth;
        scenario.cells.assign(static_cast<std::size_t>(kWidth) * kHeight, 0);
        const u32 density = kMinDensity + seed % kDensities;

        for (int y = 0; y < kHeight; ++y)
        {
            for (int x = 0; x < kWidth; ++x)
            {
                u32& cell = scenario.cells[static_cast<std::size_t>(y) * kWidth + x];
                const u32 noise = Mix32(seed * kCellSeed + static_cast<u32>(y * kWidth + x));

                if (x < kBorder || y < kBorder || x >= kWidth - kBorder || y >= kHeight - kBorder)
                {
                    cell = kOutside;
                }
                else if (noise % kDensityRange < density)
                {
                    cell = kCellBits[noise >> kBitsShift & kBitsMask];
                }
            }
        }

        const u32 a = Mix32(seed * kFromSeed + kFromOffset);
        const u32 b = Mix32(seed * kToSeed + kToOffset);
        scenario.width = 1 + static_cast<int>(seed % kSizes);
        scenario.height = 1 + static_cast<int>((seed / kSizes) % kSizes);
        const int spanX = kWidth - 2 * kBorder - scenario.width;
        const int spanY = kHeight - 2 * kBorder - scenario.height;
        scenario.fromX =
            kTile * (kBorder + static_cast<int>(a % static_cast<u32>(spanX))) + static_cast<int>(a >> kPixelShift);
        scenario.fromY = kTile * (kBorder + static_cast<int>((a >> kRowShift) % static_cast<u32>(spanY)));
        scenario.toX = kTile * (kBorder + static_cast<int>(b % static_cast<u32>(spanX)));
        scenario.toY = kTile * (kBorder + static_cast<int>((b >> kRowShift) % static_cast<u32>(spanY))) +
                       static_cast<int>(b >> kPixelShift);
        scenario.mask = seed % kWallOnlyEvery == 0 ? kWall : (seed % kLowBitsEvery == 0 ? kLowBits : kWallOrRole);

        if (seed % kInPlaceEvery == 0)
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
