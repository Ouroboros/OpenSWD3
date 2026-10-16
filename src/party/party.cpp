#include "party/party.h"

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
constexpr char kHeRan[] = "\246\363\265M";                   // 0x004A715C 何然
constexpr char kJiangRuHong[] = "\246\277\246p\254\365";     // 0x004A7154 江如紅
constexpr char kYangKunShuo[] = "\267\250\251[\272\323";     // 0x004A714C 楊坤碩
constexpr char kGuYueSheng[] = "\245j\244\353\270t";         // 0x004A7144 古月聖
constexpr char kFuZiChe[] = "\273\262\244l\271\375";         // 0x004A713C 輔子徹
constexpr char kWenJin[] = "\257\276\300A";                  // 0x004A7134 紋錦
constexpr char kZhuShiZi[] = "\305\261\245\333\244l";        // 0x004A712C 鑄石子
constexpr char kLeLe[] = "\274\326\274\326";                 // 0x004A7124 樂樂
constexpr char kDaMi[] = "\244j\246\314";                    // 0x004A711C 大米
constexpr char kZhenMeng[] = "\257u\271\332";                // 0x004A7114 真夢
constexpr char kHongShanHu[] = "\254\365\254\300\267\344";   // 0x004A710C 紅珊瑚
constexpr char kNingCaiChen[] = "\271\347\252\366\246\332";  // 0x004A7104 寧采臣
constexpr char kYanChiXia[] = "\277P\250\252\301\370";       // 0x004A70FC 燕赤霞
constexpr char kXiaoQian[] = "\244p\255\305";                // 0x004A70F4 小倩

bool Named(const Party::Name& name, const char* text)
{
    return std::string_view(name.data()) == text;
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

}  // namespace swd3
