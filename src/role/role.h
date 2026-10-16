#pragma once

#include <array>
#include <vector>

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

// A character on the map (0xD8 bytes in the original, 256 of them at 0x004BABA8): its position and state, followed by
// the playback of its action script. Offsets of the original are given for reference.
struct Role
{
    Role* next = nullptr;   // +0x00 the next role in its row of the drawing order
    i32 x = 0;              // +0x04 pixels (16 per tile)
    i32 y = 0;              // +0x08
    u32* cell = nullptr;    // +0x0C its cell of the map's block grid
    u32 flags = 0;          // +0x10
    i32 unk14 = 0;          // +0x14
    i32 unk18 = 0;          // +0x18
    u16 unk1C = 0;          // +0x1C
    u16 event = 0;          // +0x1E the event run when the role is triggered
    u16 unk20 = 0;          // +0x20
    u16 unk22 = 0;          // +0x22
    u16 id = 0;             // +0x24 0xFFFF: unused
    u16 unk26 = 0;          // +0x26
    u16 unk28 = 0;          // +0x28 bits 12..15 of the placement word
    u16 unk2A = 0;          // +0x2A bits 8..11 of the placement word
    u16 unk2C = 0;          // +0x2C
    u16 unk2E = 0;          // +0x2E
    u32 colorPair = 0;      // +0x30 a colour in both halves
    i32 unk34 = 0;          // +0x34
    std::vector<u8> unk38;  // +0x38 a buffer of the role (freed when the map is left)
    i32 unk3C = 0;          // +0x3C
    ActState act;           // +0x40

    void FillUnused();
};

}  // namespace swd3
