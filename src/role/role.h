#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

// Role::flags bits, named by what tests and sets them.
namespace role_flag
{

constexpr u32 kLayerMask = 3;         // the drawing layer
constexpr u32 kUnk10 = 0x10;          // its cells get bit 23 too (Field::MarkRoleCells)
constexpr u32 kNoTalkPointer = 0x20;  // the pointer does not offer to talk to it
constexpr u32 kExamine = 0x40;        // the pointer offers to examine it instead
constexpr u32 kFollower = 0x80;       // it follows the leader
constexpr u32 kCellEffects = 0x100;   // it takes the shadow, shade and reflection of its cell
constexpr u32 kBubbles = 0x200;       // bubbles rise from it
constexpr u32 kHidden = 0x400;        // not drawn although visible
constexpr u32 kFacesTalker = 0x800;   // it turns to the controlled role that talks to it
constexpr u32 kStepsAct = 0x1000;     // its act steps every frame (an act or action was set by an event)
constexpr u32 kTouchEvent = 0x2000;   // its event runs when the controlled role touches it
constexpr u32 kSolid = 0x4000;        // its cells get bit 29 too
constexpr u32 kVisible = 0x8000;
constexpr u32 kBelowVisible = 0x7FFF;   // the flags below kVisible
constexpr u32 kPassWalls = 0x40000;     // its scripted moves ignore walls
constexpr u32 kInEvent = 0x80000;       // the role of the running event
constexpr u32 kCellShades = 0xF00000;   // bits 12..15 of its cell
constexpr u32 kSoundOnce = 0x1000000;   // its sound with period 0xFFFF has played
constexpr u32 kArrived = 0x2000000;     // its scripted move has ended
constexpr u32 kFastMove = 0x4000000;    // it moves at double speed
constexpr u32 kUnk8000000 = 0x8000000;  // skipped by SaveRoleStates like a removed role
constexpr u32 kRemoved = 0x10000000;    // taken off the map by an event (FindRole skips it)
constexpr u32 kReflects = 0x20000000;   // it stands on a reflecting cell
constexpr u32 kStepping = 0x40000000;   // a scripted step is under way
constexpr u32 kHeld = 0x80000000;       // held by an event

}  // namespace role_flag

// A character on the map (0xD8 bytes in the original, 256 of them at 0x004BABA8): its position and state, followed by
// the playback of its action script. Offsets of the original are given for reference.
struct Role
{
    Role* next = nullptr;  // +0x00 the next role in its row of the drawing order
    i32 x = 0;             // +0x04 pixels (16 per tile)
    i32 y = 0;             // +0x08
    u32* cell = nullptr;   // +0x0C its cell of the map's block grid
    u32 flags = 0;         // +0x10
    i32 unk14 = 0;         // +0x14
    i32 unk18 = 0;         // +0x18
    u16 unk1C = 0;         // +0x1C
    u16 event = 0;         // +0x1E the event run when the role is triggered
    u16 unk20 = 0;         // +0x20
    u16 unk22 = 0;         // +0x22
    u16 id = 0;            // +0x24 0xFFFF: unused
    u16 unk26 = 0;         // +0x26
    u16 unk28 = 0;         // +0x28 bits 12..15 of the placement word
    u16 unk2A = 0;         // +0x2A bits 8..11 of the placement word
    u16 sound = 0;         // +0x2C its sound (maps.dat table 0x15)
    u16 unk2E = 0;         // +0x2E
    u32 soundTimer = 0;    // +0x30 low word: frames to the next play, high word: the period (0xFFFF: plays once)
    i32 labelColor = 0;    // +0x34 the text colour of the label
    std::string label;     // +0x38 a name drawn over the role while no event runs (freed when the map is left)
    ActState* attached = nullptr;  // +0x3C an act drawn with the role
    ActState act;                  // +0x40
    i32 unkD4 = 0;                 // +0xD4 0: the steps of a move count twice

    void FillUnused();
};

}  // namespace swd3
