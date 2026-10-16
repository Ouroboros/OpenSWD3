#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

// A character on the map (0xD8 bytes in the original, 256 of them at 0x004BABA8): its position and state, followed by
// the playback of its action script. Offsets of the original are given for reference.
struct Role
{
    static constexpr u16 NoId = 0xFFFF;  // in `id`: an unused role
    // `talkState`: the talk window of the role.
    static constexpr u16 NoTalk = 0;
    static constexpr u16 TalkWaiting = 1;  // its window is waited for (the role stands still)
    static constexpr u16 TalkOpen = 2;
    // `soundTimer`: the low word counts the frames to the next play, the high word is the period.
    static constexpr u32 SoundCountMask = 0xFFFF;
    static constexpr u32 SoundPeriodMask = 0xFFFF0000;
    static constexpr u32 SoundPeriodShift = 16;
    static constexpr u32 SoundPlaysOnce = 0xFFFF0000;  // period 0xFFFF: the sound plays once
    static constexpr u32 CellShadeShift = 20;          // CellShades to a shade index (0..15)

    // Role::flags bits, named by what tests and sets them.
    enum Flag : u32
    {
        LayerMask = 3,   // the drawing layer
        FrontLayer = 1,  // the layers in LayerMask (0: the middle one)
        BackLayer = 2,
        Unk10 = 0x10,          // its cells get bit 23 too (Field::MarkRoleCells)
        NoTalkPointer = 0x20,  // the pointer does not offer to talk to it
        Examine = 0x40,        // the pointer offers to examine it instead
        Follower = 0x80,       // it follows the leader
        CellEffects = 0x100,   // it takes the shadow, shade and reflection of its cell
        Bubbles = 0x200,       // bubbles rise from it
        Hidden = 0x400,        // not drawn although visible
        FacesTalker = 0x800,   // it turns to the controlled role that talks to it
        StepsAct = 0x1000,     // its act steps every frame (an act or action was set by an event)
        TouchEvent = 0x2000,   // its event runs when the controlled role touches it
        Solid = 0x4000,        // its cells get bit 29 too
        Visible = 0x8000,
        BelowVisible = 0x7FFF,   // the flags below Visible
        PassWalls = 0x40000,     // its scripted moves ignore walls
        InEvent = 0x80000,       // the role of the running event
        CellShades = 0xF00000,   // bits 12..15 of its cell
        SoundOnce = 0x1000000,   // its sound with period 0xFFFF has played
        Arrived = 0x2000000,     // its scripted move has ended
        FastMove = 0x4000000,    // it moves at double speed
        Unk8000000 = 0x8000000,  // skipped by SaveRoleStates like a removed role
        Removed = 0x10000000,    // taken off the map by an event (FindRole skips it)
        Reflects = 0x20000000,   // it stands on a reflecting cell
        Stepping = 0x40000000,   // a scripted step is under way
        Held = 0x80000000,       // held by an event
    };

    Role* next = nullptr;          // +0x00 the next role in its row of the drawing order
    i32 x = 0;                     // +0x04 pixels (16 per tile)
    i32 y = 0;                     // +0x08
    u32* cell = nullptr;           // +0x0C its cell of the map's block grid
    u32 flags = 0;                 // +0x10
    i32 unk14 = 0;                 // +0x14
    i32 unk18 = 0;                 // +0x18
    u16 unk1C = 0;                 // +0x1C
    u16 event = 0;                 // +0x1E the event run when the role is triggered
    u16 unk20 = 0;                 // +0x20
    u16 unk22 = 0;                 // +0x22
    u16 id = 0;                    // +0x24 NoId: unused
    u16 talkState = Role::NoTalk;  // +0x26 NoTalk, TalkWaiting or TalkOpen
    u16 unk28 = 0;                 // +0x28 bits 12..15 of the placement word
    u16 unk2A = 0;                 // +0x2A bits 8..11 of the placement word
    u16 sound = 0;                 // +0x2C its sound (maps.dat table 0x15)
    u16 unk2E = 0;                 // +0x2E
    u32 soundTimer = 0;  // +0x30 low word: frames to the next play, high word: the period (0xFFFF: plays once)
    i32 labelColor = 0;  // +0x34 the text colour of the label
    std::string label;   // +0x38 a name drawn over the role while no event runs (freed when the map is left)
    ActState* attached = nullptr;  // +0x3C an act drawn with the role
    ActState act;                  // +0x40
    i32 unkD4 = 0;                 // +0xD4 0: the steps of a move count twice

    void FillUnused();
};

}  // namespace swd3
