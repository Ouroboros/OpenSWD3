#pragma once

#include <array>
#include <span>
#include <string_view>

#include "core/types.h"

namespace swd3
{

// A battle of battle.ffd (0x10C bytes, read to 0x004FF1E0): the background and its animation, and up to eight
// enemies with their places.
struct BattleFormation
{
    static constexpr std::size_t UnkSlots = 7;
    static constexpr std::size_t MaxEnemies = 8;
    static constexpr std::size_t Unk2ABytes = 0x2E;

    i32 unk00 = 0;                              // +0x00
    i32 background = 0;                         // +0x04 the picture of all_map2.tsw
    std::array<i32, UnkSlots> unk08 = {};       // +0x08
    u16 animated = 0;                           // +0x24 the background has an animation
    u16 unk26 = 0;                              // +0x26
    u16 animation = 0;                          // +0x28 its number
    std::array<u8, Unk2ABytes> unk2A = {};      // +0x2A
    i32 animationX = 0;                         // +0x58
    std::array<i32, UnkSlots> unk5C = {};       // +0x5C
    i32 animationY = 0;                         // +0x78
    std::array<i32, UnkSlots> unk7C = {};       // +0x7C
    u16 enemyCount = 0;                         // +0x98
    u16 unk9A = 0;                              // +0x9A
    std::array<u32, MaxEnemies> enemies = {};   // +0x9C the low words: the mon.dat objects
    std::array<u16, MaxEnemies> guarding = {};  // +0xBC 1: the enemy starts guarding
    std::array<u32, MaxEnemies> enemyX = {};    // +0xCC the low words
    std::array<u32, MaxEnemies> enemyY = {};    // +0xEC

    void Load(std::span<const u8> bytes);
};

// battle.ffd (the object at 0x004FF5B8): a header of 0x2714 bytes (a table of record numbers from +4 and, from
// +0x1F44, the number of formations of each battle) followed by the 0x10C-byte formations.
class BattleFile
{
public:
    static constexpr u32 HeaderBytes = 0x2714;
    static constexpr u32 RecordBytes = 0x10C;
    static constexpr u32 RecordsAt = 4;  // the table of record numbers
    static constexpr u32 CountsAt = 0x1F44;

    bool ReadHeader(std::string_view path);
    bool ReadFormation(std::string_view path, BattleFormation& formation, u16 battle, i8 choice);
    std::span<const u8> Counts() const;

    std::array<u8, HeaderBytes> header = {};  // +0x04
};

}  // namespace swd3
