#include "party/party.h"
#include "core/byte_reader.h"
#include "org/database.h"
#include "org/object_record.h"
#include "text/big5.h"

#include <algorithm>
#include <string>
#include <string_view>

namespace swd3
{

// swd32.exe 0x0040DD60: a role of the starting state in maps.dat (0x34 bytes): the maxima come before the current
// values, the experience is a word
void RoleRecord::Load(std::span<const u8> record)
{
    ByteReader reader(record);
    this->totalExperience = static_cast<i32>(reader.Word32());
    this->maxHitPoints = static_cast<i16>(reader.Word16());
    this->maxSpirit = static_cast<i16>(reader.Word16());
    this->maxStamina = static_cast<i16>(reader.Word16());
    this->hitPoints = static_cast<i16>(reader.Word16());
    this->spirit = static_cast<i16>(reader.Word16());
    this->stamina = static_cast<i16>(reader.Word16());
    this->attack = static_cast<i16>(reader.Word16());
    this->defense = static_cast<i16>(reader.Word16());
    this->wisdom = static_cast<i16>(reader.Word16());
    this->agility = static_cast<i16>(reader.Word16());
    this->rapport = static_cast<i16>(reader.Word16());
    this->reaction = static_cast<i16>(reader.Word16());
    this->ailmentChance = static_cast<i16>(reader.Word16());
    this->evasion = static_cast<i16>(reader.Word16());
    this->unk2A = reader.Word16();
    this->experience = static_cast<i32>(reader.Word16());
    this->condition = reader.Word16();
    this->attackBonus = reader.Word16();
    this->defenseBonus = reader.Word16();
    this->level = reader.Byte();

    for (u8& value : this->resistances)
    {
        value = reader.Byte();
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
constexpr u16 kPlainObjectLimit = 0x1B;  // ApplyObject: ObjectData::category up to it applies by useKind
constexpr u16 kVitalsObject = 0;         // ObjectData::useKind
constexpr u16 kMaximaObject = 0x100;
constexpr u16 kPercentObject = 0x200;
constexpr int kPercent = 100;
constexpr std::u8string_view kDaMi = u8"大米";           // 0x004A711C
constexpr std::u8string_view kZhenMeng = u8"真夢";       // 0x004A7114
constexpr std::u8string_view kHongShanHu = u8"紅珊瑚";   // 0x004A710C
constexpr std::u8string_view kNingCaiChen = u8"寧采臣";  // 0x004A7104
constexpr std::u8string_view kYanChiXia = u8"燕赤霞";    // 0x004A70FC
constexpr std::u8string_view kXiaoQian = u8"小倩";       // 0x004A70F4

// The values the name codes set.
constexpr i16 kHeRanAttributes = 0x19;
constexpr i16 kJiangRuHongWisdom = 0x64;
constexpr i16 kJiangRuHongSpirit = 0x12C;
constexpr i16 kYangKunShuoAttack = 0x32;
constexpr i16 kYangKunShuoHitPoints = 0xC8;
constexpr u8 kGuYueShengResistance = 0xFB;
constexpr i16 kFuZiCheAttributes = 0x1E;
constexpr u8 kWenJinResistance = 0xF9;
constexpr i16 kZhuShiZiDefense = 0x64;
constexpr i16 kZhuShiZiStamina = 0x12C;
constexpr i16 kLeLeWisdom = 0x46;
constexpr i16 kLeLeSpirit = 0xFA;
constexpr i16 kDaMiWisdom = 0x64;
constexpr u8 kDaMiResistance = 2;
constexpr i16 kZhenMengDefense = 0x64;
constexpr i16 kZhenMengStamina = 0x12C;
constexpr i16 kHongShanHuAttack = 0x32;
constexpr i16 kHongShanHuHitPoints = 0xC8;
constexpr i16 kNingCaiChenLoss = 5;
constexpr i16 kNingCaiChenAttackLoss = 0xA;
constexpr i32 kNingCaiChenGold = 0x1F4;
constexpr i16 kXiaoQianValue = 5;

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
        role.wisdom = value;
        role.agility = value;
    };

    if (Named(firstName, kHeRan))
    {
        attributes(first, kHeRanAttributes);
    }

    if (Named(secondName, kJiangRuHong))
    {
        second.wisdom = kJiangRuHongWisdom;
        second.spirit = kJiangRuHongSpirit;
        second.maxSpirit = kJiangRuHongSpirit;
    }

    if (Named(firstName, kYangKunShuo))
    {
        first.attack = kYangKunShuoAttack;
        first.hitPoints = kYangKunShuoHitPoints;
        first.maxHitPoints = kYangKunShuoHitPoints;
    }

    if (Named(firstName, kGuYueSheng))
    {
        first.resistances.fill(kGuYueShengResistance);
    }

    if (Named(firstName, kFuZiChe))
    {
        attributes(first, kFuZiCheAttributes);
    }

    if (Named(secondName, kWenJin))
    {
        second.resistances.fill(kWenJinResistance);
    }

    if (Named(firstName, kZhuShiZi))
    {
        first.defense = kZhuShiZiDefense;
        first.stamina = kZhuShiZiStamina;
        first.maxStamina = kZhuShiZiStamina;
    }

    if (Named(firstName, kLeLe))
    {
        first.wisdom = kLeLeWisdom;
        first.spirit = kLeLeSpirit;
        first.maxSpirit = kLeLeSpirit;
    }

    if (Named(firstName, kDaMi))
    {
        first.wisdom = kDaMiWisdom;
        first.resistances.fill(kDaMiResistance);
    }

    if (Named(secondName, kZhenMeng))
    {
        second.defense = kZhenMengDefense;
        second.stamina = kZhenMengStamina;
        second.maxStamina = kZhenMengStamina;
    }

    if (Named(secondName, kHongShanHu))
    {
        second.attack = kHongShanHuAttack;
        second.hitPoints = kHongShanHuHitPoints;
        second.maxHitPoints = kHongShanHuHitPoints;
    }

    if (Named(firstName, kNingCaiChen))
    {
        first.defense = static_cast<i16>(first.defense - kNingCaiChenLoss);
        first.wisdom = static_cast<i16>(first.wisdom - kNingCaiChenLoss);
        first.agility = static_cast<i16>(first.agility - kNingCaiChenLoss);
        this->variables[Party::Gold] = kNingCaiChenGold;
        first.attack = static_cast<i16>(first.attack - kNingCaiChenAttackLoss);
    }

    if (Named(firstName, kYanChiXia))
    {
        first.defense = 0;
        first.stamina = 0;
        first.maxStamina = 0;
    }

    if (Named(secondName, kXiaoQian))
    {
        second.wisdom = kXiaoQianValue;
        second.spirit = kXiaoQianValue;
        second.maxSpirit = kXiaoQianValue;
    }
}

Party::Name Party::MakeName(std::string_view text)
{
    Name name = {};
    std::copy_n(text.begin(), std::min(text.size(), name.size() - 1), name.begin());
    return name;
}

std::array<Party::Name, Party::Roles> Party::DefaultNames()
{
    std::array<Name, Roles> names = {};

    for (std::size_t role = 0; role < names.size(); ++role)
    {
        names[role] = Party::MakeName(Big5(Party::RoleNames[role]));
    }

    return names;
}

// swd32.exe 0x00411030: a field of role `role` set by number (0..0xD the words from +4, 0xE the experience, 0xF the
// first dword, 0x10 the level with the experience of that level)
void Party::SetRoleStat(int field, int value, int role, Database& database)
{
    RoleRecord& record = this->roles[static_cast<std::size_t>(role)];
    const std::array<i16*, Party::WordStats> words = {
        &record.hitPoints,
        &record.spirit,
        &record.stamina,
        &record.maxHitPoints,
        &record.maxSpirit,
        &record.maxStamina,
        &record.attack,
        &record.defense,
        &record.wisdom,
        &record.agility,
        &record.rapport,
        &record.reaction,
        &record.ailmentChance,
        &record.evasion,
    };

    if (field >= 0 && field < Party::WordStats)
    {
        *words[static_cast<std::size_t>(field)] = static_cast<i16>(value);
    }
    else if (field == Party::ExperienceStat)
    {
        record.experience = value;
    }
    else if (field == Party::TotalExperienceStat)
    {
        record.totalExperience = value;
    }
    else if (field == Party::LevelStat)
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
    const std::array<i16, Party::WordStats> words = {
        record.hitPoints,
        record.spirit,
        record.stamina,
        record.maxHitPoints,
        record.maxSpirit,
        record.maxStamina,
        record.attack,
        record.defense,
        record.wisdom,
        record.agility,
        record.rapport,
        record.reaction,
        record.ailmentChance,
        record.evasion,
    };

    if (field >= 0 && field < Party::SignedStats)
    {
        return words[static_cast<std::size_t>(field)];
    }

    if (field >= Party::SignedStats && field < Party::WordStats)
    {
        return static_cast<u16>(words[static_cast<std::size_t>(field)]);
    }

    if (field == Party::ExperienceStat)
    {
        return record.experience;
    }

    if (field == Party::TotalExperienceStat)
    {
        return record.totalExperience;
    }

    if (field == Party::LevelStat)
    {
        return record.level;
    }

    return 0;
}

// swd32.exe 0x0044D6E0: object `object` used on (or worn by) a party role: its skill's ailment with bit 15 revives a
// defeated role. A standing role, or any role when the object's unk52 is at most 0x1B, gains by unk3C: 0 the vitals
// (up to the maxima) and the attributes, 0x100 the maxima and the attributes, 0x200 the vitals in percent of the
// maxima. Otherwise (a defeated role and unk52 above 0x1B) only the vitals rise.
void ApplyObject(RoleRecord& record, const ObjectData& object, Database& database)
{
    SkillScript skill;
    database.LoadSkill(skill, object.skillScript);

    if (static_cast<i16>(skill.ailment) < 0)
    {
        record.condition &= static_cast<u16>(~RoleRecord::Defeated);
    }

    auto add = [](i16& field, u16 amount)
    {
        field = static_cast<i16>(field + amount);
    };

    auto addBonus = [](u16& field, u16 amount)
    {
        field = static_cast<u16>(field + amount);
    };

    auto clampVitals = [&record]
    {
        record.hitPoints = std::min(record.hitPoints, record.maxHitPoints);
        record.spirit = std::min(record.spirit, record.maxSpirit);
        record.stamina = std::min(record.stamina, record.maxStamina);
    };

    if (object.category > kPlainObjectLimit && (record.condition & RoleRecord::Defeated) == 0)
    {
        add(record.hitPoints, object.hitPointsUp);
        add(record.spirit, object.spiritUp);
        add(record.stamina, object.staminaUp);
        clampVitals();
        return;
    }

    if ((record.condition & RoleRecord::Defeated) != 0)
    {
        return;
    }

    if (object.useKind == kVitalsObject)
    {
        add(record.hitPoints, object.hitPointsUp);
        add(record.spirit, object.spiritUp);
        add(record.stamina, object.staminaUp);
        clampVitals();
        add(record.attack, object.attackUp);
        add(record.defense, object.defenseUp);
        add(record.wisdom, object.wisdomUp);
        add(record.agility, object.agilityUp);
        add(record.rapport, object.rapportUp);
        add(record.evasion, object.evasionUp);
        addBonus(record.attackBonus, object.attackBonusUp);
        addBonus(record.defenseBonus, object.defenseBonusUp);
    }

    if (object.useKind == kMaximaObject)
    {
        add(record.maxHitPoints, object.hitPointsUp);
        add(record.maxSpirit, object.spiritUp);
        add(record.maxStamina, object.staminaUp);
        add(record.attack, object.attackUp);
        add(record.defense, object.defenseUp);
        add(record.wisdom, object.wisdomUp);
        add(record.rapport, object.rapportUp);
        add(record.agility, object.agilityUp);
        add(record.evasion, object.evasionUp);
        addBonus(record.attackBonus, object.attackBonusUp);
        addBonus(record.defenseBonus, object.defenseBonusUp);
    }

    if (object.useKind == kPercentObject)
    {
        const i16 maxHitPoints = record.maxHitPoints;
        const i16 maxSpirit = record.maxSpirit;
        const i16 maxStamina = record.maxStamina;
        // 0x0044D887..0x0044D8E4: each a 32-bit product of the two signed words, then / 100
        add(record.hitPoints, static_cast<u16>(maxHitPoints * static_cast<i16>(object.hitPointsUp) / kPercent));
        add(record.spirit, static_cast<u16>(maxSpirit * static_cast<i16>(object.spiritUp) / kPercent));
        add(record.stamina, static_cast<u16>(maxStamina * static_cast<i16>(object.staminaUp) / kPercent));
        clampVitals();
    }
}
}  // namespace swd3
