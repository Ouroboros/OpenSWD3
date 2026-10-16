#pragma once

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

// A character on the map (0xD8 bytes in the original, 256 of them at 0x004BABA8): its position and state, followed by
// the playback of its action script. Offsets of the original are given for reference.
struct Role
{
    i32 unk00 = 0;      // +0x00
    i32 x = 0;          // +0x04 pixels (16 per tile)
    i32 y = 0;          // +0x08
    i32 layer = 0;      // +0x0C
    u32 flags = 0;      // +0x10
    i32 unk14 = 0;      // +0x14
    i32 unk18 = 0;      // +0x18
    u16 unk1C = 0;      // +0x1C
    u16 event = 0;      // +0x1E the event run when the role is triggered
    u16 unk20 = 0;      // +0x20
    u16 unk22 = 0;      // +0x22
    u16 id = 0;         // +0x24 0xFFFF: unused
    u16 unk26 = 0;      // +0x26
    i32 unk28[6] = {};  // +0x28
    ActState act;       // +0x40
};

}  // namespace swd3
