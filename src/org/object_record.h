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
    u16 unk66 = 0;          // +0x66 0x66..0x70: set together by 0x004769A0
    u16 unk68 = 0;
    u16 unk6A = 0;
    u16 unk6C = 0;
    u16 unk6E = 0;
    u16 unk70 = 0;
    u16 unk72 = 0;  // +0x72 0x72..0x76: set together by 0x00476A10
    u16 unk74 = 0;
    u16 unk76 = 0;
    u16 unk78 = 0;
    u16 unk7A = 0;
    u16 healA = 0;  // +0x7C an enemy heals itself (choice 1) when low
    u16 unk7E = 0;
    u16 healB = 0;  // +0x80 (choice 2)
    u16 unk82 = 0;
    u16 unk84 = 0;
    u16 unk86 = 0;
    u16 unk88 = 0;
    u16 unk8A = 0;
    u16 unk8C = 0;
    u8 canHeal = 0;  // +0x8E
    u8 unk8F = 0;
    u8 battleLevel = 0;                  // +0x90 a unit's level byte in battle
    u8 unk91 = 0;                        // +0x91
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

// A skill script of mon.dat (Database::LoadSkill): flags and parameters set by the opcodes.
struct SkillScript
{
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

}  // namespace swd3
