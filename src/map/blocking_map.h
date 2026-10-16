#pragma once

#include <vector>

#include "core/types.h"

namespace swd3
{

// The blocking word of every tile of the current map (0x004B7948, one dword per tile, mapWidth tiles per row).
struct BlockingMap
{
    // A tile outside the map (the original reads the memory around the array) is blocked for every mask.
    static constexpr u32 kOutside = 0xFFFFFFFF;

    u32 Cell(int index) const
    {
        if (index < 0 || static_cast<std::size_t>(index) >= this->cells.size())
        {
            return BlockingMap::kOutside;
        }

        return this->cells[static_cast<std::size_t>(index)];
    }

    i32 width = 0;  // 0x004B7930 tiles
    i32 height = 0;
    std::vector<u32> cells;
};

}  // namespace swd3
