#include "battle/battle_role.h"

#include <algorithm>
#include <string_view>

#include "core/assertion.h"
#include "org/database.h"
#include "text/big5.h"

namespace swd3
{

// swd32.exe 0x0046E490
BattleRole::BattleRole()
    : ownedRecord(std::make_unique<RoleRecord>())
{
    this->battleRecord = this->ownedRecord.get();
}

// swd32.exe 0x0046E870
void BattleRole::SetSkills(ItemNode* list)
{
    this->skills = list;
    this->skillsHead = list;
}

// swd32.exe 0x0046E850
void BattleRole::SetInventory(ItemLink* list)
{
    this->inventory = list;
    this->inventoryHead = list;
}

namespace
{

constexpr std::string_view kManroleFile = "C:\\Project\\swd102aDVD\\manrole.cpp";
// The source lines the assertions name.
constexpr u32 kNoActLine = 0xDE;
constexpr u32 kNoEscortActLine = 0x123;
constexpr u32 kNoGuestActLine = 0x14E;
constexpr u32 kNoWeaponActLine = 0x182;
constexpr std::u8string_view kNoAct = u8"戰鬥角色actNumber為0!!";           // swd32.exe 0x004A7C2C
constexpr std::u8string_view kNoEscortAct = u8"護駕actNumber為0!!";         // swd32.exe 0x004A7C68
constexpr std::u8string_view kNoGuestAct = u8"Npc角色actNumber為0!!";       // swd32.exe 0x004A7C7C
constexpr std::u8string_view kNoWeaponAct = u8"武器%d號的強攻擊act沒設!!";  // swd32.exe 0x004A7C94
constexpr i16 kStatLimit = 0x270F;
constexpr u16 kCoinArmour = 0x39D;  // 投幣式鎧甲: the gold pays for the hits
constexpr int kTenths = 10;
constexpr int kPercent = 100;
constexpr u32 kAttackRingPercent = 10;  // an attack ring adds 10% of the attack bonus besides its proficiency share

// The value grown by a tenth per point of `scale` (the product kept to 16 bits as the original does).
i16 Grown(i16 value, i16 scale)
{
    return static_cast<i16>(value + static_cast<i16>(scale * value) / kTenths);
}

}  // namespace

// swd32.exe 0x0046E6A0: the battle extras cleared: the words past the unit, the five acts at +0x2BC8, slot acts 8 and
// 9 and the record at +0x2B24
void BattleRole::ClearExtras()
{
    this->bonusA = 0;
    this->bonusB = 0;
    this->bonusC = 0;
    this->unk2F0C = 0;
    this->unk2F10 = 0;
    this->unk2F12 = 0;
    this->unk2F1C = 0;
    this->weaponId = 0;
    this->unk2F24 = 0;
    this->unk2F14 = 0;
    this->unk2F18 = 0;
    this->unk2F16 = 0;
    this->unk2BC8.fill(ActState{});
    this->slotActs[BattleUnit::UnkSlot8] = ActState{};
    this->slotActs[BattleUnit::UnkSlot9] = ActState{};
    this->unk2B24 = ObjectRecord{};
}

// swd32.exe 0x0046E730: a party role entering battle: its record copied into the battle copy (the attack and defence
// bonuses starting from the attack and defence), its party record kept with its current and maximum values held to
// 9999, the act from its placement (asserted non-zero), and bit 15 of +0x24 marking it defeated
void BattleRole::Setup(
    RoleRecord& record,
    const RoleBattleBlock& block,
    const UnitPlacement& at,
    ErrorReporter& reporter
)
{
    this->ClearExtras();
    this->placement = at;
    this->savedPlacement = this->placement;
    this->kind = at.kind;
    *this->battleRecord = record;
    this->roleRecord = &record;
    this->roleBlock = block.unk00.data();
    this->unk2A93 = static_cast<u8>(this->battleRecord->unk1E);
    this->actId = at.actId;

    if (this->actId == 0)
    {
        MaintenanceAssert(reporter, kNoAct, 0, kManroleFile, kNoActLine);
    }

    this->battleRecord->attackBonus = static_cast<u16>(this->battleRecord->attack);
    this->battleRecord->defenseBonus = static_cast<u16>(this->battleRecord->defense);

    for (i16* value :
         {&record.maxHitPoints,
          &record.hitPoints,
          &record.maxSpirit,
          &record.spirit,
          &record.maxStamina,
          &record.stamina})
    {
        if (*value > kStatLimit)
        {
            *value = kStatLimit;
        }
    }

    this->defeated = (this->battleRecord->unk24 & RoleRecord::Defeated) != 0 ? 1 : 0;
}

// swd32.exe 0x0046E890: a summoned escort: its mon.dat object (by the act of its placement, asserted non-zero) gives
// the attack and defence bonuses, the agility, +0x14, the level byte, the resistances and the hit points of the battle
// record, which serves as both party and battle record
void BattleRole::SetupEscort(const UnitPlacement& at, Database& database, ErrorReporter& reporter)
{
    this->ownedData = std::make_unique<ObjectRecord>();
    this->data = this->ownedData.get();
    database.LoadObject(*this->data, at.actId);
    Database::FreeDescription(*this->data);
    this->placement = at;
    this->savedPlacement = at;
    this->kind = at.kind;
    this->actId = at.actId;

    if (this->actId == 0)
    {
        MaintenanceAssert(reporter, kNoEscortAct, 0, kManroleFile, kNoEscortActLine);
    }

    RoleRecord& record = *this->battleRecord;
    const ObjectData& object = this->data->data;
    record.attackBonus = object.attack;
    record.defenseBonus = object.defense;
    record.agility = static_cast<i16>(object.agility);
    record.unk14 = static_cast<i16>(object.unk5C);
    record.unk1E = object.battleLevel;
    std::copy_n(object.resistances.begin(), record.unk2D.size(), record.unk2D.begin());
    this->unk2A93 = static_cast<u8>(record.unk1E);
    record.hitPoints = object.hitPoints;
    record.maxHitPoints = record.hitPoints;
    this->roleRecord = this->battleRecord;
    this->weapon.data.unk3E = object.skill;
}

// swd32.exe 0x0046E9C0: a guest fighting with the party: its mon.dat object (by the act of its placement, asserted
// non-zero) scales the hit points, the bonuses, +0x14, the level and the agility of `base` by tenths, gives its level
// byte and resistances, and fills the hit points; the record serves as both party and battle record
void BattleRole::SetupGuest(
    const UnitPlacement& at,
    const RoleRecord& base,
    Database& database,
    ErrorReporter& reporter
)
{
    this->ownedData = std::make_unique<ObjectRecord>();
    this->data = this->ownedData.get();
    database.LoadObject(*this->data, at.actId);
    Database::FreeDescription(*this->data);
    this->placement = at;
    this->savedPlacement = at;
    this->kind = at.kind;
    this->actId = at.actId;

    if (this->actId == 0)
    {
        MaintenanceAssert(reporter, kNoGuestAct, 0, kManroleFile, kNoGuestActLine);
    }

    RoleRecord& record = *this->battleRecord;
    const ObjectData& object = this->data->data;
    const auto scale = static_cast<i16>(object.unk24);
    record.maxHitPoints = Grown(base.maxHitPoints, scale);
    record.attackBonus = static_cast<u16>(Grown(static_cast<i16>(base.attackBonus), scale));
    record.defenseBonus = static_cast<u16>(Grown(static_cast<i16>(base.defenseBonus), scale));
    record.unk14 = Grown(base.unk14, scale);
    record.level = static_cast<u8>(base.level + static_cast<i8>(scale * base.level) / kTenths);
    record.unk1E = object.battleLevel;
    record.agility = static_cast<i16>(base.agility + static_cast<i16>(base.agility * object.unk2C) / kTenths);
    std::copy_n(object.resistances.begin(), record.unk2D.size(), record.unk2D.begin());
    this->unk2A93 = static_cast<u8>(record.unk1E);
    record.hitPoints = record.maxHitPoints;
    this->weapon.data.unk3E = object.skill;
    this->roleRecord = this->battleRecord;
}

// swd32.exe 0x0046EBB0: the sixteen equipment slots add to the battle record: the attack and defence bonuses, the
// agility, +0x14, +0x18, +0x1E, raised attack and defence, the resistances (not slots 7..10) and, for slots 1..6 of
// kind 0, the three bonus words. Slot 0 is the weapon (its strong attack act asserted), slots 7 and 8 the
// accessories; 投幣式鎧甲 makes the gold guard.
void BattleRole::ApplyEquipment(
    std::span<const std::unique_ptr<ItemNode>> slots,
    CounterList& proficiency,
    ErrorReporter& reporter
)
{
    this->accessories.fill(ObjectRecord{});
    RoleRecord& record = *this->battleRecord;

    for (std::size_t slot = 0; slot < static_cast<std::size_t>(Party::SlotsPerRole) && slot < slots.size(); ++slot)
    {
        if (slots[slot] == nullptr)
        {
            continue;
        }

        const ItemNode& item = *slots[slot];
        const ObjectData& stats = item.record.data;

        if (slot == BattleRole::WeaponSlot)
        {
            this->weapon = item.record;
            this->weaponId = item.id;

            if (this->weapon.data.unk48 == 0)
            {
                MaintenanceAssert(reporter, kNoWeaponAct, item.id, kManroleFile, kNoWeaponActLine);
            }
        }

        record.attackBonus = static_cast<u16>(record.attackBonus + stats.unk24);
        record.defenseBonus = static_cast<u16>(record.defenseBonus + stats.unk26);
        record.agility = static_cast<i16>(record.agility + stats.unk2C);
        record.unk14 = static_cast<i16>(record.unk14 + stats.unk2E);
        record.unk18 = static_cast<i16>(record.unk18 + stats.unk30);
        record.unk1E = static_cast<i16>(record.unk1E + stats.unk32);

        if (stats.unk28 != 0)
        {
            record.attack = static_cast<i16>(record.attack + stats.unk28);
            record.attackBonus = static_cast<u16>(record.attackBonus + stats.unk28);
        }

        if (stats.unk2A != 0)
        {
            record.defense = static_cast<i16>(record.defense + stats.unk2A);
            record.defenseBonus = static_cast<u16>(record.defenseBonus + stats.unk2A);
        }

        if (slot == BattleRole::FirstAccessorySlot || slot == BattleRole::SecondAccessorySlot)
        {
            ObjectRecord& accessory = this->accessories[slot - BattleRole::FirstAccessorySlot];
            accessory = item.record;

            if (item.id != ItemNode::Empty)
            {
                accessory.data.unk50 = item.id;
            }

            this->ApplyAccessory(accessory, proficiency);
        }
        else if (slot != BattleRole::NoResistanceSlotA && slot != BattleRole::NoResistanceSlotB)
        {
            for (std::size_t element = 0; element < record.unk2D.size(); ++element)
            {
                record.unk2D[element] = static_cast<u8>(record.unk2D[element] + stats.resistances[element]);
            }

            if (slot < BattleRole::ArmourSlots && stats.unk3C == 0)
            {
                this->bonusA = static_cast<u16>(this->bonusA + stats.unk34);
                this->bonusB = static_cast<u16>(this->bonusB + stats.unk36);
                this->bonusC = static_cast<u16>(this->bonusC + stats.unk38);
            }

            if (item.id == kCoinArmour)
            {
                this->goldGuard = 1;
            }
        }
    }
}

// swd32.exe 0x0046F030: an accessory by its kind: most set their equipment flag; an elemental orb lowers its
// first resistance by the proficiency of the accessory, an attack ring raises the attack bonus by a tenth and half the
// proficiency in percent
void BattleRole::ApplyAccessory(const ObjectRecord& accessory, CounterList& proficiency)
{
    RoleRecord& record = *this->battleRecord;

    switch (static_cast<AccessoryKind>(accessory.data.unk48))
    {
        case AccessoryKind::Unk32:
            this->equipmentFlags |= BattleUnit::Unk32;
            break;

        case AccessoryKind::ElementOrb:
        {
            const i16 percent = FindCounterPercent(proficiency, static_cast<i16>(accessory.data.unk50));
            std::size_t element = 0;

            while (element < accessory.data.resistances.size() && accessory.data.resistances[element] == 0)
            {
                ++element;
            }

            if (element == accessory.data.resistances.size())
            {
                break;
            }

            const auto scaled = static_cast<i8>(static_cast<u8>(-kTenths * accessory.data.resistances[element]));
            const auto product = static_cast<i16>(percent * scaled);
            const auto share = static_cast<u16>(product / kPercent);
            record.unk2D[element] = static_cast<u8>(record.unk2D[element] + static_cast<int>(share) / -kTenths);
            break;
        }

        case AccessoryKind::AttackRing:
        {
            const auto half = static_cast<u16>(
                static_cast<u16>(FindCounterPercent(proficiency, static_cast<i16>(accessory.data.unk50))) >> 1
            );
            const u16 extra =
                half != 0 ? static_cast<u16>(half * static_cast<u32>(record.attackBonus) / kPercent) : u16{0};
            record.attackBonus = static_cast<u16>(
                record.attackBonus + extra + kAttackRingPercent * static_cast<u32>(record.attackBonus) / kPercent
            );
            break;
        }

        case AccessoryKind::BladeReturn:
            this->equipmentFlags |= BattleUnit::BladeReturn;
            break;

        case AccessoryKind::Unk36:
            this->equipmentFlags |= BattleUnit::Unk36;
            break;

        case AccessoryKind::SpiritDrain:
            this->equipmentFlags |= BattleUnit::SpiritDrain;
            break;

        case AccessoryKind::LifeDrain:
            this->equipmentFlags |= BattleUnit::LifeDrain;
            break;

        case AccessoryKind::SpiritFan:
            this->equipmentFlags |= BattleUnit::SpiritFan;
            break;

        default:
            break;
    }
}

// swd32.exe 0x0046FFE0: the skill list shown from its start
void BattleRole::ShowAllSkills()
{
    this->skills = this->skillsHead;
}

// swd32.exe 0x00470900: the item list shown from its start
void BattleRole::ShowAllItems()
{
    this->inventory = this->inventoryHead;
}

// swd32.exe 0x00470890: the stamina owed (+0x2F12) taken from the party record, not below 0
void BattleRole::PayStamina()
{
    if (this->unk2F12 == 0)
    {
        return;
    }

    this->roleRecord->stamina = static_cast<i16>(this->roleRecord->stamina - this->unk2F12);

    if (this->roleRecord->stamina < 0)
    {
        this->roleRecord->stamina = 0;
    }

    this->unk2F12 = 0;
}

// swd32.exe 0x004708C0: flag 9 of +0x2A86 and +0x2B0C set, command A from +0x2F18 when set, slot acts 8 and 9 cleared
void BattleRole::TakeCommand2F18()
{
    this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag200);
    this->unk2B0C = 1;

    if (this->unk2F18 != 0)
    {
        this->commandA = this->unk2F18;
    }

    this->slotActs[BattleUnit::UnkSlot8] = ActState{};
    this->slotActs[BattleUnit::UnkSlot9] = ActState{};
}

}  // namespace swd3
