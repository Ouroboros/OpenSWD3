#include "party/party.h"
#include "org/database.h"
#include "text/big5.h"

#include <algorithm>
#include <string>
#include <string_view>

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

namespace
{

// The names that change the starting roles (swd32.exe 0x004A70F4..0x004A715C).
constexpr std::u8string_view kHeRan = u8"何然";          // 0x004A715C
constexpr std::u8string_view kJiangRuHong = u8"江如紅";  // 0x004A7154
constexpr std::u8string_view kYangKunShuo = u8"楊坤碩";  // 0x004A714C
constexpr std::u8string_view kGuYueSheng = u8"古月聖";   // 0x004A7144
constexpr std::u8string_view kFuZiChe = u8"輔子徹";      // 0x004A713C
constexpr std::u8string_view kWenJin = u8"紋錦";         // 0x004A7134
constexpr std::u8string_view kZhuShiZi = u8"鑄石子";     // 0x004A712C
constexpr std::u8string_view kLeLe = u8"樂樂";           // 0x004A7124
constexpr std::u8string_view kDaMi = u8"大米";           // 0x004A711C
constexpr std::u8string_view kZhenMeng = u8"真夢";       // 0x004A7114
constexpr std::u8string_view kHongShanHu = u8"紅珊瑚";   // 0x004A710C
constexpr std::u8string_view kNingCaiChen = u8"寧采臣";  // 0x004A7104
constexpr std::u8string_view kYanChiXia = u8"燕赤霞";    // 0x004A70FC
constexpr std::u8string_view kXiaoQian = u8"小倩";       // 0x004A70F4

bool Named(const Party::Name& name, std::u8string_view text)
{
    const std::string_view all(name.data(), name.size());
    return all.substr(0, all.find('\0')) == Big5(text);
}

}  // namespace

// swd32.exe 0x00449D80: the codes typed as the names of the first two roles: each sets attributes or hit points,
// spirit and stamina of that role, one the gold
void Party::ApplyNameCheats()
{
    RoleRecord& first = this->roles[0];
    RoleRecord& second = this->roles[1];
    const Name& firstName = this->names[0];
    const Name& secondName = this->names[1];

    auto attributes = [](RoleRecord& role, i16 value)
    {
        role.attack = value;
        role.defense = value;
        role.unk14 = value;
        role.agility = value;
    };

    if (Named(firstName, kHeRan))
    {
        attributes(first, 0x19);
    }

    if (Named(secondName, kJiangRuHong))
    {
        second.unk14 = 0x64;
        second.spirit = 0x12C;
        second.maxSpirit = 0x12C;
    }

    if (Named(firstName, kYangKunShuo))
    {
        first.attack = 0x32;
        first.hitPoints = 0xC8;
        first.maxHitPoints = 0xC8;
    }

    if (Named(firstName, kGuYueSheng))
    {
        first.unk2D.fill(0xFB);
    }

    if (Named(firstName, kFuZiChe))
    {
        attributes(first, 0x1E);
    }

    if (Named(secondName, kWenJin))
    {
        second.unk2D.fill(0xF9);
    }

    if (Named(firstName, kZhuShiZi))
    {
        first.defense = 0x64;
        first.stamina = 0x12C;
        first.maxStamina = 0x12C;
    }

    if (Named(firstName, kLeLe))
    {
        first.unk14 = 0x46;
        first.spirit = 0xFA;
        first.maxSpirit = 0xFA;
    }

    if (Named(firstName, kDaMi))
    {
        first.unk14 = 0x64;
        first.unk2D.fill(2);
    }

    if (Named(secondName, kZhenMeng))
    {
        second.defense = 0x64;
        second.stamina = 0x12C;
        second.maxStamina = 0x12C;
    }

    if (Named(secondName, kHongShanHu))
    {
        second.attack = 0x32;
        second.hitPoints = 0xC8;
        second.maxHitPoints = 0xC8;
    }

    if (Named(firstName, kNingCaiChen))
    {
        first.defense = static_cast<i16>(first.defense - 5);
        first.unk14 = static_cast<i16>(first.unk14 - 5);
        first.agility = static_cast<i16>(first.agility - 5);
        this->variables[0] = 0x1F4;
        first.attack = static_cast<i16>(first.attack - 0xA);
    }

    if (Named(firstName, kYanChiXia))
    {
        first.defense = 0;
        first.stamina = 0;
        first.maxStamina = 0;
    }

    if (Named(secondName, kXiaoQian))
    {
        second.unk14 = 5;
        second.spirit = 5;
        second.maxSpirit = 5;
    }
}

Party::Name Party::MakeName(std::string_view text)
{
    Name name = {};
    std::copy_n(text.begin(), std::min(text.size(), name.size() - 1), name.begin());
    return name;
}

std::array<Party::Name, Party::kRoles> Party::DefaultNames()
{
    std::array<Name, kRoles> names = {};

    for (std::size_t role = 0; role < names.size(); ++role)
    {
        names[role] = Party::MakeName(Big5(Party::kDefaultNames[role]));
    }

    return names;
}

// swd32.exe 0x00411030: a field of role `role` set by number (0..0xD the words from +4, 0xE the experience, 0xF the
// first dword, 0x10 the level with the experience of that level)
void Party::SetRoleStat(int field, int value, int role, Database& database)
{
    RoleRecord& record = this->roles[static_cast<std::size_t>(role)];
    const std::array<i16*, 14> words = {
        &record.hitPoints,
        &record.spirit,
        &record.stamina,
        &record.maxHitPoints,
        &record.maxSpirit,
        &record.maxStamina,
        &record.attack,
        &record.defense,
        &record.unk14,
        &record.agility,
        &record.unk18,
        &record.unk1A,
        &record.ailmentChance,
        &record.unk1E,
    };

    if (field >= 0 && field < 0xE)
    {
        *words[static_cast<std::size_t>(field)] = static_cast<i16>(value);
    }
    else if (field == 0xE)
    {
        record.experience = value;
    }
    else if (field == 0xF)
    {
        record.unk00 = value;
    }
    else if (field == 0x10)
    {
        record.level = static_cast<u8>(value);
        u32 experience = 0;

        if (database.LoadLevelEntry(role + 1, value + 1, experience))
        {
            record.experience = static_cast<i32>(experience);
        }
    }
}

// swd32.exe 0x004112B0: a field of role `role` by number (the first six words signed, the others unsigned)
int Party::RoleStat(int field, int role) const
{
    const RoleRecord& record = this->roles[static_cast<std::size_t>(role)];
    const std::array<i16, 14> words = {
        record.hitPoints,
        record.spirit,
        record.stamina,
        record.maxHitPoints,
        record.maxSpirit,
        record.maxStamina,
        record.attack,
        record.defense,
        record.unk14,
        record.agility,
        record.unk18,
        record.unk1A,
        record.ailmentChance,
        record.unk1E,
    };

    if (field >= 0 && field < 6)
    {
        return words[static_cast<std::size_t>(field)];
    }

    if (field >= 6 && field < 0xE)
    {
        return static_cast<u16>(words[static_cast<std::size_t>(field)]);
    }

    if (field == 0xE)
    {
        return record.experience;
    }

    if (field == 0xF)
    {
        return record.unk00;
    }

    if (field == 0x10)
    {
        return record.level;
    }

    return 0;
}

}  // namespace swd3
