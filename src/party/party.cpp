#include "party/party.h"

namespace swd3
{

namespace
{

u16 Word(std::span<const u8> bytes, std::size_t offset)
{
    return static_cast<u16>(bytes[offset] | bytes[offset + 1] << 8);
}

}  // namespace

// swd32.exe 0x0040DD60: a role of the starting state in maps.dat (0x34 bytes): the maxima come before the current
// values, the experience is a word
void RoleRecord::Load(std::span<const u8> record)
{
    this->unk00 = static_cast<i32>(Word(record, 0) | static_cast<u32>(Word(record, 2)) << 16);
    this->maxHitPoints = static_cast<i16>(Word(record, 4));
    this->maxSpirit = static_cast<i16>(Word(record, 6));
    this->maxStamina = static_cast<i16>(Word(record, 8));
    this->hitPoints = static_cast<i16>(Word(record, 0xA));
    this->spirit = static_cast<i16>(Word(record, 0xC));
    this->stamina = static_cast<i16>(Word(record, 0xE));
    this->attack = static_cast<i16>(Word(record, 0x10));
    this->defense = static_cast<i16>(Word(record, 0x12));
    this->unk14 = static_cast<i16>(Word(record, 0x14));
    this->agility = static_cast<i16>(Word(record, 0x16));
    this->unk18 = static_cast<i16>(Word(record, 0x18));
    this->unk1A = static_cast<i16>(Word(record, 0x1A));
    this->ailmentChance = static_cast<i16>(Word(record, 0x1C));
    this->unk1E = static_cast<i16>(Word(record, 0x1E));
    this->unk2A = Word(record, 0x20);
    this->experience = static_cast<i32>(Word(record, 0x22));
    this->unk24 = Word(record, 0x24);
    this->attackBonus = Word(record, 0x26);
    this->defenseBonus = Word(record, 0x28);
    this->level = record[0x2A];

    for (std::size_t index = 0; index < this->unk2D.size(); ++index)
    {
        this->unk2D[index] = record[0x2B + index];
    }
}

}  // namespace swd3
