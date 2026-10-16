#pragma once

#include <array>
#include <memory>
#include <vector>

#include "core/types.h"
#include "game/game_state.h"
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

// An area of the map with its encounters (0x18 bytes; the list starts with a node of its own at 0x004A9A18).
struct MapTrigger
{
    i32 left = 0;                      // +0x00 tiles
    i32 top = 0;                       // +0x04
    i32 right = 0;                     // +0x08
    i32 bottom = 0;                    // +0x0C
    u32 table = 0;                     // +0x10 an offset into maps.dat
    std::unique_ptr<MapTrigger> next;  // +0x14
};

// A list hung on a role's place (0x10 bytes, four at 0x004CACE0).
struct RoleMarkNode
{
    std::unique_ptr<RoleMarkNode> next;
};

struct RoleMark
{
    std::unique_ptr<RoleMarkNode> nodes;  // +0x00
    u16 x = 0;                            // +0x04
    u16 y = 0;                            // +0x06
    u16 unk08 = 0;                        // +0x08
    u16 unk0A = 0;                        // +0x0A
    u16 role = 0;                         // +0x0C
    u16 unk0E = 0;                        // +0x0E
};

// The lists the story scripts fill (0x00427920); their contents are restored with the scripts.
struct ScriptEffect
{
    std::array<i32, 3> unk00 = {};
    std::vector<u8> unk0C;               // +0x0C
    std::vector<u8> unk10;               // +0x10
    std::unique_ptr<ScriptEffect> next;  // +0x14
};

struct ScriptSprite
{
    std::array<u8, 0xB0> unk00 = {};
    std::unique_ptr<ScriptSprite> next;  // +0xB0
};

struct ScriptAnimation
{
    std::array<u8, 0xA0> unk00 = {};
    std::unique_ptr<ScriptAnimation> next;  // +0xA0
};

struct ScriptObject
{
    std::array<u8, 0x28> unk00 = {};
    std::unique_ptr<ScriptObject> next;  // +0x28
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
    u16 unk4C8BD4 = 0;                                 // the bytes of maps.dat's table 1
    Role pointerButtonRole;                            // 0x004C96C8 (action 0xC)
    std::array<std::array<i32, 4>, 4> unk4C97A0 = {};  // 0x004C97A0 the weather particles: x, y, dx, dy
    u32 unk4C97E4 = 0;                                 // an offset into maps.dat (its table 0x18)
    std::array<ActState, 4> unk4CAA78;
    i32 unk4CAD20 = 0;
    Role pointerPageRole;        // 0x004CADA0 (action 0xE)
    std::vector<u16> unk4CAE78;  // records of 8 words from maps.dat's table 0x12, their count first
    i32 unk4CAE88 = 0;

    // Set by LoadCity (0x0040C130).
    i32 unk49E0BC = 0;
    i32 unk49E0C0 = 0;
    i32 unk4A94A8 = 0;
    i32 unk4AB97C = 0;
    i32 unk4AB994 = 0;
    i32 unk4ACB98 = 0;
    i32 unk4ACDB4 = 0;
    std::array<std::array<i32, 4>, 4> unk4ACDC8 = {};  // 0x004ACDC8
    std::array<std::array<i32, 4>, 4> unk4ACE04 = {};  // 0x004ACE04
    i32 unk4ACE48 = 0;
    i32 unk4ACE4C = 0;
    i32 unk4ACE50 = 0;
    i32 unk4ACE54 = 0;
    i32 unk4ACE58 = 0;
    i32 unk4ACE5C = 0;
    u16 unk4ACEF0 = 0;
    u16 unk4AAEC8 = 0;
    u16 unk4B737C = 0;
    std::array<u8, 0x300> unk4B7520 = {};
    i32 unk4B7944 = 0;
    std::array<i32, 0x20> unk4B7998 = {};  // the trail of the leader: y
    std::array<i32, 0x20> unk4B7A18 = {};  // the trail of the leader: x
    i32 unk4B7A98 = 0;
    std::array<i32, 8> unk4B7C8C = {};  // the roles following the leader
    i32 unk4B7EB4 = 0;
    std::array<std::array<i32, 4>, 4> unk4B86F8 = {};
    i32 unk4B873C = 0;
    i32 unk4B88C0 = 0;
    u16 unk4B88BC = 0;
    i32 unk4B896C = 0;
    std::array<MoveRecord, 8> followerMoves;  // 0x004B8E18
    i32 unk4B9EF8 = 0;
    i32 unk4B9EFC = 0;
    std::array<u8, 0x200> unk4BA430 = {};
    u16 unk4BAB80 = 0;
    i32 unk4BABA0 = 0;  // followers in unk4B7C8C
    i32 unk4C8BD8 = 0;
    u16 unk4C97F6 = 0;
    i32 unk4C9A18 = 0;
    i32 unk4C9A20 = 0;
    i32 unk4CAE80 = 0;
    i32 unk4CC2DC = 0;
    i32 unk4A9920 = 0;                             // 0x004A9920 bit 15 kept across maps
    MapTrigger triggers;                           // 0x004A9A18 (the list from +0x14)
    std::array<RoleMark, 4> marks;                 // 0x004CACE0
    std::unique_ptr<ScriptEffect> effects;         // 0x004BAB9C
    std::unique_ptr<ScriptSprite> spritesA;        // 0x004AD3E8
    std::unique_ptr<ScriptSprite> spritesB;        // 0x004BA6E0
    std::unique_ptr<ScriptAnimation> animationsA;  // 0x004B7C70
    std::unique_ptr<ScriptAnimation> animationsB;  // 0x004B8968
    std::unique_ptr<ScriptObject> objects;         // 0x004AC9B8

    void FreeEffects();
    void FreeSpritesA();
    void FreeSpritesB();
    void FreeNodes();
    void FreeAnimations();
    void FreeMarks();
    void ResetUnk4B86F8();
    void FreeObjects();
};

}  // namespace swd3
