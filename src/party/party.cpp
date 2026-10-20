#include "party/party.h"
#include "core/byte_reader.h"
#include "org/database.h"
#include "org/object_record.h"
#include "text/big5.h"

#include <algorithm>
#include <array>
#include <span>
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
    this->strength = static_cast<i16>(reader.Word16());
    this->endurance = static_cast<i16>(reader.Word16());
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

using Variables = std::array<i32, Party::Variables>;

// A name code: the name typed and what it does to the role so named (and to the party's variables).
struct NameCode
{
    std::u8string_view name;
    void (*apply)(RoleRecord& role, Variables& variables);
};

void SetAttributes(RoleRecord& role, i16 value)
{
    role.strength = value;
    role.endurance = value;
    role.wisdom = value;
    role.agility = value;
}

void SetHitPoints(RoleRecord& role, i16 value)
{
    role.hitPoints = value;
    role.maxHitPoints = value;
}

void SetSpirit(RoleRecord& role, i16 value)
{
    role.spirit = value;
    role.maxSpirit = value;
}

void SetStamina(RoleRecord& role, i16 value)
{
    role.stamina = value;
    role.maxStamina = value;
}

// The codes of the first role, then of the second; a name is one of them at most.
constexpr std::array<NameCode, 10> kFirstRoleCodes = {{
    {kHeRan,
     [](RoleRecord& role, Variables&)
     {
         SetAttributes(role, kHeRanAttributes);
     }},
    {kYangKunShuo,
     [](RoleRecord& role, Variables&)
     {
         role.strength = kYangKunShuoAttack;
         SetHitPoints(role, kYangKunShuoHitPoints);
     }},
    {kGuYueSheng,
     [](RoleRecord& role, Variables&)
     {
         role.resistances.fill(kGuYueShengResistance);
     }},
    {kFuZiChe,
     [](RoleRecord& role, Variables&)
     {
         SetAttributes(role, kFuZiCheAttributes);
     }},
    {kZhuShiZi,
     [](RoleRecord& role, Variables&)
     {
         role.endurance = kZhuShiZiDefense;
         SetStamina(role, kZhuShiZiStamina);
     }},
    {kLeLe,
     [](RoleRecord& role, Variables&)
     {
         role.wisdom = kLeLeWisdom;
         SetSpirit(role, kLeLeSpirit);
     }},
    {kDaMi,
     [](RoleRecord& role, Variables&)
     {
         role.wisdom = kDaMiWisdom;
         role.resistances.fill(kDaMiResistance);
     }},
    {kNingCaiChen,
     [](RoleRecord& role, Variables& variables)
     {
         role.endurance = static_cast<i16>(role.endurance - kNingCaiChenLoss);
         role.wisdom = static_cast<i16>(role.wisdom - kNingCaiChenLoss);
         role.agility = static_cast<i16>(role.agility - kNingCaiChenLoss);
         variables[Party::Gold] = kNingCaiChenGold;
         role.strength = static_cast<i16>(role.strength - kNingCaiChenAttackLoss);
     }},
    {kYanChiXia,
     [](RoleRecord& role, Variables&)
     {
         role.endurance = 0;
         SetStamina(role, 0);
     }},
}};

constexpr std::array<NameCode, 5> kSecondRoleCodes = {{
    {kJiangRuHong,
     [](RoleRecord& role, Variables&)
     {
         role.wisdom = kJiangRuHongWisdom;
         SetSpirit(role, kJiangRuHongSpirit);
     }},
    {kWenJin,
     [](RoleRecord& role, Variables&)
     {
         role.resistances.fill(kWenJinResistance);
     }},
    {kZhenMeng,
     [](RoleRecord& role, Variables&)
     {
         role.endurance = kZhenMengDefense;
         SetStamina(role, kZhenMengStamina);
     }},
    {kHongShanHu,
     [](RoleRecord& role, Variables&)
     {
         role.strength = kHongShanHuAttack;
         SetHitPoints(role, kHongShanHuHitPoints);
     }},
    {kXiaoQian,
     [](RoleRecord& role, Variables&)
     {
         role.wisdom = kXiaoQianValue;
         SetSpirit(role, kXiaoQianValue);
     }},
}};

// The code `name` is, applied to `role`.
void ApplyNameCode(std::span<const NameCode> codes, const Party::Name& name, RoleRecord& role, Variables& variables)
{
    const std::string_view all(name.data(), name.size());
    const std::string_view typed = all.substr(0, all.find('\0'));

    for (const NameCode& code : codes)
    {
        if (typed == Big5(code.name))
        {
            code.apply(role, variables);
            return;
        }
    }
}

}  // namespace

// swd32.exe 0x00449D80: the codes typed as the names of the first two roles: each sets attributes or hit points,
// spirit and stamina of that role, one the gold. The rebuild looks each name up in its role's table (the original
// compares it with every code in turn; the names differ, so at most one applies either way).
void Party::ApplyNameCheats()
{
    ApplyNameCode(kFirstRoleCodes, this->names[0], this->roles[0], this->variables);
    ApplyNameCode(kSecondRoleCodes, this->names[1], this->roles[1], this->variables);
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
        &record.strength,
        &record.endurance,
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
        record.strength,
        record.endurance,
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
        add(record.strength, object.strengthUp);
        add(record.endurance, object.enduranceUp);
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
        add(record.strength, object.strengthUp);
        add(record.endurance, object.enduranceUp);
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
