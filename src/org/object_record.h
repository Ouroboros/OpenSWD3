#pragma once

#include <array>
#include <optional>
#include <string>

#include "core/types.h"

namespace swd3
{

#pragma pack(push, 1)

// The fixed part of a mon.dat object (an item, a skill or a monster; 0xA0 of the 0xA4 bytes of the original record),
// filled tag by tag by Database::LoadObject. Offsets are given for reference; names follow the bestiary and battle
// screens, the rest are placeholders.
struct ObjectData
{
    static constexpr u16 FirstRoleBit = 0x8000;  // in `usableBy`: the bit of the first role, the others below it
    static constexpr u32 UnkKind20 = 0x20;       // in `kind`: scripted acts and absorbs do not affect it
    static constexpr u32 Absorbable = 0x800;     // in `kind`: an ordinary monster (蟹精魔機, 黏怪, 吸血殭屍, ...)
    static constexpr u32 UnkKind8000 = 0x8000;   // in `kind`: cleared on a skill learnt by a talk command

    std::array<char, 0x20> name = {};  // +0x00 tag 0x3E8 (a longer name runs into the next fields)
    u32 kind = 0;                      // +0x20 tag 0x19
    u16 unk24 = 0;                     // +0x24 tag 7
    u16 unk26 = 0;                     // +0x26 tag 8
    u16 unk28 = 0;                     // +0x28 tag 0xF
    u16 unk2A = 0;                     // +0x2A tag 0x10
    u16 unk2C = 0;                     // +0x2C tag 9
    u16 unk2E = 0;                     // +0x2E tag 0x11
    u16 unk30 = 0;                     // +0x30 tag 0x12
    u16 unk32 = 0;                     // +0x32 tag 0xA
    u16 unk34 = 0;                     // +0x34 tag 0x13
    u16 unk36 = 0;                     // +0x36 tag 0x14
    u16 unk38 = 0;                     // +0x38 tag 0x15
    u16 usableBy = 0;                  // +0x3A tag 0x1A the roles that can learn or use it (FirstRoleBit >> role)
    u16 unk3C = 0;                     // +0x3C tag 0x1B
    u16 unk3E = 0;                     // +0x3E tag 0x64
    u16 unk40 = 0;                     // +0x40 tag 6
    u16 unk42 = 0;                     // +0x42 tag 0xC
    u16 unk44 = 0;                     // +0x44 tag 0xD
    u16 price = 0;                     // +0x46 tag 0xB (the shop value)
    u16 unk48 = 0;                     // +0x48 tag 0x16
    u16 unk4A = 0;                     // +0x4A
    i32 maxHitPoints = 0;              // +0x4C an enemy's maximum (set in battle)
    // +0x50..+0x9C: the 0x4D-byte block of tag 1; tags 0xE, 0x7D0, 0x1C and 0x1D set parts of it.
    u16 unk50 = 0;          // +0x50 tag 0xE
    u16 unk52 = 0;          // +0x52 tag 0x7D0
    u16 level = 0;          // +0x54 tag 0x7D0 (shown by the bestiary)
    u16 attack = 0;         // +0x56
    u16 defense = 0;        // +0x58
    u16 agility = 0;        // +0x5A the battle speed
    u16 unk5C = 0;          // +0x5C
    u16 ailmentChance = 0;  // +0x5E an ailment lands on a roll of 1..10 up to it
    u16 skill = 0;          // +0x60 an enemy's skill script (mon.dat)
    u16 unk62 = 0;          // +0x62
    i16 hitPoints = 0;      // +0x64
    u16 itemA = 0;          // +0x66 the objects an enemy uses (inferred) and their counts; 0x66..0x70: set together
                            // by 0x004769A0 (BattleUnit::ChooseCommand)
    u16 itemACount = 0;
    u16 itemB = 0;
    u16 itemBCount = 0;
    u16 itemC = 0;
    u16 itemCCount = 0;
    u16 skillA = 0;  // +0x72 the skills of an enemy and their counts; 0x72..0x76: set together by 0x00476A10
    u16 skillACount = 0;
    u16 skillB = 0;
    u16 skillBCount = 0;
    u16 healItemA = 0;   // +0x7A the object an enemy heals itself with (choice 1)
    u16 healCountA = 0;  // +0x7C its uses left
    u16 healItemB = 0;
    u16 healCountB = 0;       // +0x80 (choice 2)
    u16 dropItem = 0;         // +0x82 the object an enemy may drop (RollDrop)
    u16 dropChance = 0;       // +0x84 a roll of 0..19 below it drops it
    u16 stealItem = 0;        // +0x86 the item an enemy can be robbed of (BattleRole::Steal), 0: none
    u16 stealDifficulty = 0;  // +0x88 2..5: a steal roll below it succeeds
    u16 unk8A = 0;
    u16 unk8C = 0;
    u8 itemChance = 0;   // +0x8E 0: the enemy uses no objects, else added to the turns without one for the roll
    u8 skillChance = 0;  // +0x8F the same for the skills
    u8 battleLevel = 0;  // +0x90 a unit's level byte in battle
    u8 unk91 = 0;        // +0x91
    std::array<u8, 9> resistances = {};  // +0x92 tag 0x7D0: per element
    u8 unk9B = 0;                        // +0x9B tag 0x1C, 0x7D0
    u8 unk9C = 0;                        // +0x9C tag 0x1D, 0x7D0
    std::array<u8, 3> unk9D = {};        // +0x9D
};

#pragma pack(pop)

static_assert(sizeof(ObjectData) == 0xA0);

// A mon.dat object: the fixed part and the description (+0xA0, tag 0x1E; none when the record has no such tag).
struct ObjectRecord
{
    ObjectData data;
    std::optional<std::string> description;
};

// The name of `record` up to its terminator, within its 0x20 bytes (the original copies with lstrcpyA, on into the
// next fields when no terminator is there).
inline std::string ObjectName(const ObjectRecord& record)
{
    const auto& name = record.data.name;
    std::size_t length = 0;

    while (length < name.size() && name[length] != '\0')
    {
        ++length;
    }

    return std::string(name.data(), length);
}

// A skill script of mon.dat (Database::LoadSkill): flags and parameters set by the opcodes.
struct SkillScript
{
    // The values of `ailment` (the status bit it sets on the unit); names are given where the battle shows them.
    enum Ailment : u16
    {
        UnkAilment1 = 1,
        UnkAilment2 = 2,
        UnkAilment4 = 4,
        UnkAilment8 = 8,
        UnkAilment10 = 0x10,
        UnkAilment20 = 0x20,
        SlowedAilment = 0x40,
        UnkAilment80 = 0x80,
        UnkAilment100 = 0x100,
        StatsScaledAilment = 0x200,
        UnkAilment1000 = 0x1000,
        MissedAilment = 0x2000,  // the skill marks the unit missed instead
        RefreshOnly = 0x8000,    // an effect of the same kind only gets new turns and value
    };

    // The bits of `flags`, each set by an opcode of the skill script.
    enum Flag : u32
    {
        UnkFlag1 = 1,      // opcode 3
        UnkFlag2 = 2,      // opcode 9
        UnkFlag4 = 4,      // opcode 0xB
        ElementBuff = 8,   // opcode 0xE: the buff raises the resistance to `element`
        UnkFlag10 = 0x10,  // opcodes 0xF..0x15 set 0x10..0x800 in turn (0x80 excepted)
        UnkFlag20 = 0x20,
        UnkFlag40 = 0x40,
        NoAilmentFlag = 0x80,  // opcode 6: the skill brings no ailment
        UnkFlag100 = 0x100,
        UnkFlag200 = 0x200,
        UnkFlag400 = 0x400,
        UnkFlag800 = 0x800,
        UnkFlag1000 = 0x1000,  // opcode 0x16, with unk20 and turns
        UnkFlag2000 = 0x2000,  // opcode 0x18
        UnkFlag4000 = 0x4000,  // opcode 0x19
    };

    static constexpr u16 Unk1EMark = 0x8000;  // set in unk1E by opcode 0xA

    // The values of `element` (names from the battle messages of each resistance).
    enum Element : u16
    {
        Fire = 1,
        Water = 2,  // its attack buff is named ice
        UnkElement3 = 3,
        UnkElement4 = 4,
        Poison = 5,
        Light = 6,
        Dark = 7,
        UnkElement8 = 8,
        Physical = 9,
    };

    // The bits of `statusBits` (kept by the effect nodes of a unit, EffectNode::bits).
    enum Buff : u32
    {
        DefenseDown = 0x80000,
        SpeedDown = 0x100000,
        DarkAttackUp = 0x200000,
        LightAttackUp = 0x400000,
        IceAttackUp = 0x800000,
        FireAttackUp = 0x1000000,
        Barrier = 0x2000000,
        AttackDown = 0x8000000,
        Invisible = 0x10000000,
        DefenseUp = 0x20000000,
        SpeedUp = 0x40000000,
        AttackUp = 0x80000000,
    };

    u32 unk00 = 0;
    u32 flags = 0;       // +0x04 opcodes 3, 6, 9, 0xB, 0xE..0x16, 0x18, 0x19
    u32 statusBits = 0;  // +0x08 opcode 0xC: the status bits of a buff
    u32 unk0C = 0;       // +0x0C opcode 0
    u32 unk10 = 0;       // +0x10 opcode 0
    u16 unk14 = 0;       // +0x14 opcode 8
    u16 unk16 = 0;       // +0x16 opcode 4
    u16 ailment = 0;     // +0x18 opcodes 6, 7 (bit 13: none, bit 15: refresh only)
    u16 element = 0;     // +0x1A opcode 0xE: the element of a resistance buff
    u16 unk1C = 0;       // +0x1C opcode 0xD
    u16 unk1E = 0;       // +0x1E opcode 0xA (bit 15 set with it)
    u16 unk20 = 0;       // +0x20 opcode 0x16
    u16 unk22 = 0;       // +0x22 opcode 0x17
    u8 turns = 0;        // +0x24 opcodes 2, 0x16: the turns of an effect
};

// The opcodes of a skill script of mon.dat (Database::LoadSkill); 5 ends the script.
enum class SkillOpcode : u16
{
    Values = 0,  // unk0C and unk10
    Turns = 2,
    SetUnkFlag1 = 3,
    Unk16 = 4,
    End = 5,
    NoAilment = 6,
    Ailment = 7,
    Unk14 = 8,
    SetUnkFlag2 = 9,
    Unk1E = 0xA,
    SetUnkFlag4 = 0xB,
    StatusBits = 0xC,
    Unk1C = 0xD,
    Element = 0xE,
    SetUnkFlag10 = 0xF,
    SetUnkFlag20 = 0x10,
    SetUnkFlag40 = 0x11,
    SetUnkFlag100 = 0x12,
    SetUnkFlag200 = 0x13,
    SetUnkFlag400 = 0x14,
    SetUnkFlag800 = 0x15,
    Unk20 = 0x16,
    Unk22 = 0x17,
    SetUnkFlag2000 = 0x18,
    SetUnkFlag4000 = 0x19,
};

// The tags of an object script of mon.dat (Database::LoadObject); 5 ends the script.
enum class ObjectTag : u16
{
    Unk50Block = 1,  // 0x4D bytes from unk50
    End = 5,
    Unk40 = 6,
    Unk24 = 7,
    Unk26 = 8,
    Unk2C = 9,
    Unk32 = 0xA,
    Price = 0xB,
    Unk42 = 0xC,
    Unk44 = 0xD,
    Unk50 = 0xE,
    Unk28 = 0xF,
    Unk2A = 0x10,
    Unk2E = 0x11,
    Unk30 = 0x12,
    Unk34 = 0x13,
    Unk36 = 0x14,
    Unk38 = 0x15,
    Unk48 = 0x16,
    Kind = 0x19,
    UsableBy = 0x1A,
    Unk3C = 0x1B,
    Unk9B = 0x1C,
    Unk9C = 0x1D,
    Description = 0x1E,
    Unk3E = 0x64,
    Name = 0x3E8,
    Resistances = 0x7D0,  // the resistances, unk9B, unk9C, the level and unk52
};

}  // namespace swd3
