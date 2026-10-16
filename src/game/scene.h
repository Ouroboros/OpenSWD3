#pragma once

#include <array>
#include <memory>
#include <vector>

#include "core/types.h"
#include "res/act_state.h"
#include "role/role.h"

namespace swd3
{

// A node of the list headed at 0x004ACF00 (0x4C bytes; two buffers and the next node at +0x48).
struct SceneNode
{
    std::array<i32, 14> unk00 = {};
    std::vector<u8> unk38;  // +0x38
    std::array<i32, 2> unk3C = {};
    std::vector<u8> unk44;            // +0x44
    std::unique_ptr<SceneNode> next;  // +0x48
};

// The state of the story scenes that a new game starts from (named by the original's addresses until their readers
// are restored): the pointer roles, the animations of the screen elements and tables of maps.dat.
struct Scene
{
    static constexpr int kPointerAct = 0x2329;
    static constexpr u32 kPointerFlags = 0x9000;

    i32 unk4A93D4 = 0;
    i32 unk4A9918 = 0;
    i32 unk4A9930 = 0;
    i32 unk4A9938 = 0;
    std::vector<u8> unk4A9A04;
    std::vector<u8> unk4A9A08;
    std::vector<u8> unk4A9A0C;
    ActState unk4A9A30;
    std::array<ActState, 4> unk4A9120;
    ActState unk4A93E8;
    u32 unk4A94B0 = 0;
    i32 unk4AB788 = 0;
    ActState unk4ACAF8;
    i32 viewWidth = 0;   // 0x004ACBC0
    i32 viewHeight = 0;  // 0x004ACBC4
    i32 unk4ACBC8 = 0;
    i32 unk4ACBCC = 0;
    Role pointerRole;  // 0x004ACCD8 the mouse pointer (act 0x2329, action 0)
    i32 unk4ACDB8 = 0;
    i32 unk4ACDBC = 0;
    u32 unk4ACE40 = 0;  // an offset into maps.dat (its table 1)
    std::array<u8, 0x80> unk4ACE70 = {};
    SceneNode unk4ACF00;
    std::array<ActState, 4> unk4AD0D8;
    ActState unk4AD3F0;
    i32 unk4B74F4 = 0;
    i32 unk4B751C = 0;
    std::array<i32, 6> unk4B7C74 = {};
    std::array<ActState, 8> unk4B9F68;
    ActState unk4C83A8;
    i32 unk4C8BE0 = 0;
    u16 unk4C8BD4 = 0;       // the bytes of maps.dat's table 1
    Role pointerButtonRole;  // 0x004C96C8 (action 0xC)
    std::array<u8, 0x40> unk4C97A0 = {};
    u32 unk4C97E4 = 0;  // an offset into maps.dat (its table 0x18)
    std::array<ActState, 4> unk4CAA78;
    i32 unk4CAD20 = 0;
    Role pointerPageRole;        // 0x004CADA0 (action 0xE)
    std::vector<u16> unk4CAE78;  // records of 8 words from maps.dat's table 0x12, their count first
    i32 unk4CAE88 = 0;
};

}  // namespace swd3
