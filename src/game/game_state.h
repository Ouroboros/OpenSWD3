#pragma once

#include <array>
#include <list>
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

// A clickable rectangle (0x10 bytes in the original list at 0x004C8BEC).
struct ClickRegion
{
    i32 unk00 = 0;   // +0x00
    u16 left = 0;    // +0x08
    u16 top = 0;     // +0x0A
    u16 right = 0;   // +0x0C
    u16 bottom = 0;  // +0x0E
};

// The clickable rectangles of the current screen and their state (0x004C8BE8..0x004C8BF8).
class ClickRegions
{
public:
    int Count() const;
    int Find(int pointerX, int pointerY, ClickRegion*& found);
    void Free();

    i32 flags = 0;                   // 0x004C8BE8
    std::list<ClickRegion> regions;  // 0x004C8BEC, the head first
    std::array<i32, 3> state = {};   // 0x004C8BF0
};

// The shared game state: the story flags, the pointer, the map characters and the player's control.
class GameState
{
public:
    static constexpr int kRoles = 256;
    static constexpr int kMoves = 72;
    static constexpr int kFlagBytes = 0x400;
    // 1 << n (swd32.exe 0x004994EC).
    static constexpr std::array<u8, 8> kBitMasks = {1, 2, 4, 8, 16, 32, 64, 128};

    bool GameFlag(int flag) const;
    u8 SetGameFlag(int flag);
    u8 ClearGameFlag(int flag);
    u8 SetGameFlagTo(int flag, int value);
    void SetWalkSpeed(int speed);
    int SetFrameInterval(int value);
    void ClearFrameInterval();
    int DirectionToPlayer(int x, int y) const;
    Role& Player();
    const Role& Player() const;
    void ClearWorld();

    std::array<u8, kFlagBytes> flags = {};  // 0x004AB384 story flags, saved with the game
    i32 pointerX = 0;                       // 0x004A9924 pointer position on the screen
    i32 pointerY = 0;                       // 0x004A9928
    i32 playerRole = 0;                     // 0x004AB378 the controlled role
    std::array<Role, kRoles> roles;         // 0x004BABA8
    Role leader;                            // 0x004B72A0 the leading role (filled with 0xFF when unused)
    i32 roleCount = 1;                      // 0x0049E0C4 roles in use
    MapHeader map;                          // 0x004B7930
    std::array<MoveRecord, kMoves> moves;   // 0x004AD490
    i32 walkSpeed = 0;                      // 0x004AB2D0
    i32 frameInterval = 0;                  // 0x004B7BCC milliseconds between the frames of the main loop
    ClickRegions clickRegions;
};

}  // namespace swd3
