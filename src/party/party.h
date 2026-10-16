#pragma once

#include <array>
#include <memory>

#include "core/types.h"
#include "item/item_list.h"
#include "org/fame.h"

namespace swd3
{

// A party member (0x38 bytes in the original, four of them at 0x004AB790). The attribute names follow the status
// screen (attack and defence shown with their equipment bonus) and the battle checks of spirit and stamina costs;
// those marked inferred are not yet confirmed by a label.
struct HeroRecord
{
    i32 unk00 = 0;                 // +0x00
    i16 hitPoints = 0;             // +0x04 生命
    i16 spirit = 0;                // +0x06 靈力
    i16 stamina = 0;               // +0x08 體力
    i16 maxHitPoints = 0;          // +0x0A
    i16 maxSpirit = 0;             // +0x0C
    i16 maxStamina = 0;            // +0x0E
    i16 attack = 0;                // +0x10 (inferred)
    i16 defense = 0;               // +0x12 (inferred)
    i16 unk14 = 0;                 // +0x14
    i16 agility = 0;               // +0x16 (inferred)
    i16 unk18 = 0;                 // +0x18
    i16 unk1A = 0;                 // +0x1A
    i16 ailmentChance = 0;         // +0x1C an ailment lands on a roll of 1..10 up to it
    i16 unk1E = 0;                 // +0x1E
    i32 experience = 0;            // +0x20 (inferred: the level.dat dword)
    u16 unk24 = 0;                 // +0x24
    u16 attackBonus = 0;           // +0x26 from the equipment, cleared in the save
    u16 defenseBonus = 0;          // +0x28 from the equipment, cleared in the save
    u16 unk2A = 0;                 // +0x2A
    u8 level = 0;                  // +0x2C
    std::array<u8, 9> unk2D = {};  // +0x2D
    std::array<u8, 2> unk36 = {};  // +0x36
};

static_assert(sizeof(HeroRecord) == 0x38);

// The data of the party saved with the game.
class Party
{
public:
    static constexpr int kHeroes = 4;
    static constexpr int kSlotsPerHero = 16;
    static constexpr int kVariables = 64;

    std::array<HeroRecord, kHeroes> heroes;                                    // 0x004AB790
    ItemLink inventory;                                                        // 0x004A9940
    std::array<std::unique_ptr<ItemNode>, kHeroes> skills;                     // 0x004A9490 each headed by "無"
    std::array<std::unique_ptr<ItemNode>, kHeroes * kSlotsPerHero> equipment;  // 0x004C8AD0
    i32 unk4ACDB0 = 0;                                                         // 0x004ACDB0
    std::array<i32, kVariables> variables = {};                                // 0x004ACBD0 [0]: the gold
    Fame fame;
};

}  // namespace swd3
