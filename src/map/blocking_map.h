#pragma once

#include <span>

#include "core/types.h"

namespace swd3
{

// The blocking word of every tile of the current map (0x004B7948, one dword per tile, mapWidth tiles per row).
struct BlockingMap
{
    // A tile outside the map (the original reads the memory around the array) is blocked for every mask.
    static constexpr u32 Outside = 0xFFFFFFFF;

    // The sides around a footprint that Field::ProbeCells reports blocked, clockwise from the upper left.
    enum Probe : u32
    {
        ProbeUpLeft = 1,
        ProbeUp = 2,
        ProbeUpRight = 4,
        ProbeRight = 8,
        ProbeDownRight = 0x10,
        ProbeDown = 0x20,
        ProbeDownLeft = 0x40,
        ProbeLeft = 0x80,
    };

    u32 Cell(int index) const
    {
        if (index < 0 || static_cast<std::size_t>(index) >= this->cells.size())
        {
            return BlockingMap::Outside;
        }

        return this->cells[static_cast<std::size_t>(index)];
    }

    i32 width = 0;  // 0x004B7930 tiles
    i32 height = 0;
    std::span<const u32> cells;  // the grid of the map (MapHeader::blocks)
};

// The cells along one side of a footprint (Field::ProbeCells, PathFinder::UpdateBlocked): the footprint plus a corner
// at each end. Footprints of up to SmallFootprint tiles read SmallCells cells by index.
struct FootprintEdge
{
    static constexpr int Corners = 2;
    static constexpr int SmallFootprint = 2;
    static constexpr int SmallCells = SmallFootprint + Corners;
    static constexpr std::size_t Before = 0;    // the corner before the footprint
    static constexpr std::size_t First = 1;     // its first tile
    static constexpr std::size_t AfterOne = 2;  // the corner after a one-tile footprint, the second tile of two
    static constexpr std::size_t AfterTwo = 3;  // the corner after a two-tile footprint
};

}  // namespace swd3
