#pragma once

#include <array>
#include <memory>
#include <span>

#include "core/types.h"
#include "item/item_list.h"
#include "org/fame.h"
#include "text/big5.h"

namespace swd3
{

class Database;

// A party member (0x38 bytes in the original, four of them at 0x004AB790). The attribute names follow the status
// screen (attack and defence shown with their equipment bonus) and the battle checks of spirit and stamina costs;
// those marked inferred are not yet confirmed by a label.
struct RoleRecord
{
    static constexpr u16 Defeated = 0x8000;  // in unk24: the role starts the battle defeated

    i32 unk00 = 0;                 // +0x00 raised by the experience of a won battle (DrawVictoryPanel)
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

    void Load(std::span<const u8> record);
};

static_assert(sizeof(RoleRecord) == 0x38);

// The 0x60-byte battle block of a party role (four of them at 0x004ACF50).
struct RoleBattleBlock
{
    std::array<i32, 0x18> unk00 = {};
};

// The data of the party saved with the game.
class Party
{
public:
    static constexpr int Roles = 4;
    static constexpr std::size_t NameBytes = 0x10;
    using Name = std::array<char, NameBytes>;
    // swd32.exe 0x0049E188: the names a new game offers (the first two can be changed).
    static constexpr std::array<std::u8string_view, Roles> RoleNames = {u8"賽特", u8"妮可", u8"卡瑪", u8"李靖"};

    // A name buffer holding a Big5 text (cut to keep its NUL).
    static Name MakeName(std::string_view text);
    // The default names as the Big5 name buffers.
    static std::array<Name, Roles> DefaultNames();

    static constexpr int SlotsPerRole = 16;
    static constexpr int Variables = 64;
    // Party::SetRoleStat and RoleStat fields: the words of RoleRecord from hitPoints (the first SignedStats
    // signed), then the experience, unk00 and the level.
    static constexpr int SignedStats = 6;
    static constexpr int WordStats = 0xE;
    static constexpr int ExperienceStat = 0xE;
    static constexpr int Unk00Stat = 0xF;
    static constexpr int LevelStat = 0x10;
    static constexpr std::size_t Gold = 0;      // the variable that holds the gold
    static constexpr int FirstRoleFlag = 0x1E;  // story flag FirstRoleFlag + n: party role n has joined
    // The variables talk command VariableNamePlate may name a plate by.
    static constexpr u16 FirstNameVariable = 0xB;
    static constexpr u16 SecondNameVariable = 0xC;

    std::array<RoleRecord, Roles> roles;                                    // 0x004AB790
    ItemLink inventory;                                                     // 0x004A9940
    std::array<std::unique_ptr<ItemNode>, Roles> skills;                    // 0x004A9490 each headed by "無"
    std::array<std::unique_ptr<ItemNode>, Roles * SlotsPerRole> equipment;  // 0x004C8AD0
    static constexpr u32 ClockLimit = 1000;                                 // the clock goes back to 0 past it
    i32 clock = 0;  // 0x004ACDB0 the game clock: a tick every Game::ClockFrames + 1 frames on the map (talk commands
                    // 0x22..0x25)
    std::array<i32, Variables> variables = {};  // 0x004ACBD0 [0]: the gold
    Fame fame;
    std::array<Name, Roles> names = Party::DefaultNames();  // 0x0049E148

    void ApplyNameCheats();
    static constexpr int LastStat = 0x10;  // the attributes SetRoleStat and RoleStat know
    void SetRoleStat(int field, int value, int role, Database& database);
    int RoleStat(int field, int role) const;
    std::array<RoleBattleBlock, Roles> battleBlocks;  // 0x004ACF50
};

}  // namespace swd3
