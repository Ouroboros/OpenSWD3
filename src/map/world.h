#pragma once

#include <array>
#include <vector>

#include "core/types.h"
#include "role/role.h"

namespace swd3
{

// The header of the current map (0x64 bytes at 0x004B7930); its two buffers are freed when the map is left.
struct MapHeader
{
    i32 unk00 = 0;
    i32 unk04 = 0;
    i32 bitsPerPixel = 0;  // +0x08 16: the map units hold 16-bit pixels
    std::array<i32, 3> unk0C = {};
    std::vector<u8> unk18;  // +0x18
    std::vector<u8> unk1C;  // +0x1C
    std::array<i32, 17> unk20 = {};
};

// The movement of a role along a path (0x21C bytes, 72 of them at 0x004AD490); filled with 0xFF when unused.
struct MoveRecord
{
    std::array<u8, 0x21C> unk000 = {};

    void Clear();
};

// The characters and the map they stand on.
class World
{
public:
    static constexpr int kRoles = 256;
    static constexpr int kMoves = 72;

    void Clear();

    MapHeader map;                         // 0x004B7930
    std::array<Role, kRoles> roles;        // 0x004BABA8
    Role leader;                           // 0x004B72A0 the leading role (filled with 0xFF when unused)
    i32 roleCount = 1;                     // 0x0049E0C4 roles in use, at least the leader
    std::array<MoveRecord, kMoves> moves;  // 0x004AD490
};

}  // namespace swd3
