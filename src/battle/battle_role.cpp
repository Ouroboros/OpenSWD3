#include "battle/battle_role.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <string_view>

#include "audio/audio_manager.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "config/config.h"
#include "core/assertion.h"
#include "core/words.h"
#include "draw/painter.h"
#include "org/database.h"
#include "org/fame.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"
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
constexpr i32 kMaxDamage = 0x270F;
constexpr u16 kStruckEvasion = 8;

// AttackFrame.
constexpr i32 kAttackAction = 0x28;
constexpr i32 kCriticalAction = 0x29;
constexpr i32 kForcedAttackAction = 0x30;   // unk2A8C 1 of a forced unit
constexpr i32 kForcedAttackAction2 = 0x31;  // unk2A8C 2
constexpr u16 kSecondForcedAttack = 2;
constexpr i16 kForcedX = 0x140;  // a unit with unk2B04 attacks from the middle of the field
constexpr i16 kForcedY = 0x136;
constexpr std::size_t kFlightAct = 1;  // the slot act a flying follow-up uses
constexpr i16 kSlotFadeEnd = -0x20;
constexpr i16 kSlotFadeStep = 4;
constexpr u32 kRepeatAttack = 1;  // unk26CC bit 0: the attack repeats extraAttacks times (skill 0x15)

// StartAbsorb: the particles fly into a point beside the role; a picture of kTallPicture rows or more bursts in half
// its height, a shorter one in its height less kShortBurstCut.
constexpr u16 kAbsorbLife = 0x14;
constexpr u16 kAbsorbSpeed = 0x1E;
constexpr int kAbsorbLeft = 0x32;
constexpr int kAbsorbDown = 0x28;
constexpr u16 kTallPicture = 100;
constexpr u16 kShortBurstCut = 10;
constexpr u16 kAbsorbDivisor = 0x28;
constexpr i32 kAbsorbTint = 5;
constexpr u16 kAbsorbFlags = ParticleEffect::WithinRange | ParticleEffect::Mixed;

// AbsorbFrame: the sparks (extra acts) beside the absorb; the first flies every frame, each other from its frame on.
struct Spark
{
    i16 fromFrame = 0;
    i32 action = 0;
    int offsetY = 0;  // from the top of the picture still left
    int steps = 0;
};

constexpr i32 kSparkAct = 0x186A;
constexpr std::size_t kSparkColumnShift = 2;  // the sparks start a quarter across the picture
constexpr std::array<Spark, BattleRole::ExtraActs> kSparks = {{
    {0, 1, -5, 0xE},
    {10, 2, -10, 0xA},
    {20, 3, -15, 0xC},
    {30, 1, 0, 8},
    {40, 1, 5, 0x10},
}};

// The palette of a picture passed as the draw argument (the original passes the palette pointer).
DrawArgument PaletteArgument(const Picture& picture)
{
    DrawArgument argument;

    if (!picture.palette.empty())
    {
        argument.alphaMap = reinterpret_cast<const u8*>(picture.palette.data());
    }

    if (picture.palette.size() >= 2)
    {
        argument.colorPair = MakeWords(picture.palette[0], picture.palette[1]);
    }

    return argument;
}

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
    this->extraActs.fill(ActState{});
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

// swd32.exe 0x00474FC0: a strike of the role on `target`: the weapon's proficiency takes a step (its bonus adds to
// a hit that does damage), an evadable strike may be evaded (8 in 10 under UnkStatus80) unless the role struck
// already this turn, and the damage (at most kMaxDamage) adds to the hit's damage and shows on the target
void BattleRole::Strike(BattleServices& services, BattleUnit& target, int evadable)
{
    BattleState& battle = services.state;
    this->slotTint = 0;
    battle.proficiencyBonus = 0;
    battle.proficiencyBonus = static_cast<i16>(StepCounterProgress(
        services.party.fame.lists[Fame::UnkList0],
        static_cast<i16>(this->weaponId),
        this->weapon.data.unk44,
        this->weapon.data.unk42
    ));
    const u16 evasion = (this->status & BattleUnit::UnkStatus80) != 0 ? kStruckEvasion : u16{0};

    if (evadable == 1 && this->unk2B14 == 0 && target.RollEvade(services, evasion) != 0)
    {
        this->unk2B14 = 1;
        return;
    }

    target.RaiseFlag26B8();
    battle.lastDamage = 0;
    i16 spirit = 0;
    i16 stamina = 0;
    i32 damage = this->HitDamage(services, target, spirit, stamina);
    battle.lastDamage = damage;

    if (damage != 0)
    {
        damage += battle.proficiencyBonus;
        battle.lastDamage = damage;
    }

    if (damage >= kMaxDamage)
    {
        damage = kMaxDamage;
        battle.lastDamage = damage;
    }

    battle.hitDamage += damage;

    if (damage != -1)
    {
        target.SetDamageDigits(services, damage);
        target.StepTargetCount(1);
    }

    this->unk2B14 = 1;
}

// swd32.exe 0x0046F8C0: one frame of the role's attack on `target`; 1 once it is over. A unit with unk2B00 and a
// FadesIn command fades in first; one with unk2B04 attacks from the middle of the field. The attack act (the critical
// one, with the weapon's act, while critical) is drawn, and its target bits play the effect in the recoil act, quake
// the screen, start the follow-up in slotActs[slot] (fading the field by the act's lights), or strike. The follow-up
// flies to the target (TargetsUnit) or is placed on it and may strike again; once it waits it darkens by 4 a frame.
// When the attack act waits too, the acts are cleared; without a critical attack an extra attack repeats it, and the
// recoil tint is left at 1.
int BattleRole::AttackFrame(BattleServices& services, BattleUnit& target, int slot)
{
    BattleState& battle = services.state;

    if (this->unk2A74 != 0 || this->unk2AD8 == 1)
    {
        return 0;
    }

    if (this->unk2B00 == 1 && (this->command.flags & UnitCommand::FadesIn) != 0 && this->fadedIn == 0 &&
        this->DrawFadeIn(services, target) == 0)
    {
        this->fadedIn = 1;
        return 0;
    }

    if (this->unk2B04 == 1)
    {
        this->placement.x = kForcedX;
        this->placement.y = kForcedY;
    }

    this->unk2AAC = 1;
    ActState& attack = this->standAct;
    attack.hold = 0;

    if (this->held == 1)
    {
        attack.hold = 1;
    }

    if ((attack.target & ActState::Holds) != 0)
    {
        attack.hold = 1;
    }

    const bool forced = this->unk2B00 == 1 || this->unk2B04 == 1;

    if (this->critical == 1)
    {
        battle.criticalAttack = 1;
        attack.id = this->actId;
        attack.action = kCriticalAction;

        if (forced)
        {
            attack.action = kAttackAction;
        }
        else
        {
            this->command.castAct = this->weapon.data.unk48;
        }
    }
    else
    {
        attack.id = this->actId;
        attack.action = kAttackAction;

        if (forced && this->unk2A8C != 0)
        {
            attack.action = this->unk2A8C == kSecondForcedAttack ? kForcedAttackAction2 : kForcedAttackAction;
        }
    }

    this->DrawAction(services, target, attack);

    if ((attack.target & ActState::Recoils) != 0)
    {
        if (attack.effectA != 0)
        {
            this->recoilAct.id = attack.effectA;
            this->unk267C |= BattleUnit::Recoiling;
            this->recoilAct.action = attack.effectQ;
        }

        if ((attack.target & ActState::Holds) != 0)
        {
            attack.hold = 1;
        }

        attack.target = static_cast<u16>(attack.target & ~ActState::Recoils);
        attack.effectA = 0;
        attack.effectQ = 0;
    }

    if ((this->unk267C & BattleUnit::Recoiling) != 0 &&
        this->DrawRecoil(services, this->recoilAct, static_cast<i16>(attack.anchorY)) == 1)
    {
        if ((attack.target & ActState::Holds) != 0)
        {
            attack.target = static_cast<u16>(attack.target & ~ActState::Holds);
        }

        attack.hold = 0;
        this->unk267C &= ~BattleUnit::Recoiling;
    }

    if ((attack.target & ActState::Quakes) != 0)
    {
        battle.quaking = 1;
        battle.quakePhase = 0;
    }

    ActState& follow = this->slotActs[static_cast<std::size_t>(slot)];

    if ((attack.target & ActState::FollowsUp) != 0)
    {
        if ((attack.target & ActState::FadesField) != 0)
        {
            battle.fadingField = 1;
            battle.StartFieldFade(
                static_cast<i16>(attack.lights[0]),
                static_cast<i16>(attack.lights[1]),
                static_cast<i16>(attack.lights[2]),
                static_cast<i16>(attack.lights[3]),
                static_cast<i16>(attack.lights[4]),
                static_cast<i16>(attack.lights[5]),
                static_cast<i16>(attack.lights[6])
            );
            attack.target = static_cast<u16>(attack.target & ~ActState::FadesField);
        }

        attack.target = static_cast<u16>(attack.target & ~ActState::FollowsUp);
        this->unk267C |= BattleUnit::FollowingUp;
        follow = ActState{};
    }

    if ((attack.target & ActState::UnkTarget4) != 0)
    {
        target.RaiseFlag26B8();
        attack.target = 0;
        this->unk267C |= BattleUnit::FollowingUp;
    }

    if ((attack.target & ActState::Strikes) != 0)
    {
        this->unk267C |= BattleUnit::FollowingUp;
        this->slotTint = 0;
        battle.proficiencyBonus = 0;
        follow = ActState{};
        this->Strike(services, target, 1);
        attack.target = 0;
    }

    if ((this->unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    follow.id = this->command.castAct;
    follow.action = 0;

    if (attack.effectA != 0)
    {
        follow.id = attack.effectA;
    }

    if (follow.id != 0)
    {
        if ((this->command.flags & UnitCommand::TargetsUnit) != 0)
        {
            const int fromX =
                static_cast<i16>(this->actAnchorX) - static_cast<i16>(this->actOffsetX) + this->placement.x;
            const int fromY = this->placement.y + static_cast<i16>(attack.anchorY) - attack.y;

            if (this->FlyAct(
                    services,
                    target,
                    this->slotActs[kFlightAct],
                    0,
                    follow.id,
                    fromX,
                    fromY,
                    static_cast<i16>(this->command.target),
                    0
                ) != 1)
            {
                return 0;
            }

            attack.waiting = 1;
            follow.waiting = 1;
            follow.target |= ActState::Strikes;
        }
        else
        {
            this->AimAction(
                services,
                target,
                follow,
                static_cast<i16>(attack.anchorX),
                static_cast<i16>(attack.anchorY)
            );
        }

        if ((follow.target & ActState::Strikes) != 0)
        {
            this->Strike(services, target, 1);
            follow.target = 0;
        }
    }
    else
    {
        follow.waiting = 1;
    }

    Painter& painter = services.painter;

    if (follow.waiting != 1)
    {
        // A missing picture is read through a null pointer in the original.
        if (this->picture != nullptr)
        {
            painter.DrawPicture(
                static_cast<i16>(this->aimX),
                static_cast<i16>(this->aimY),
                this->picture->width,
                this->picture->height,
                this->unk26A4
            );
        }

        return 0;
    }

    if (follow.id != 0 && (this->command.flags & UnitCommand::TargetsUnit) == 0 && this->slotTint > kSlotFadeEnd)
    {
        DrawOptions& options = painter.Options();
        options.tintRed = this->slotTint;
        options.tintGreen = this->slotTint;
        options.tintBlue = this->slotTint;

        if ((this->unk26A4 & ActState::NoBlend) != 0)
        {
            options.tintRed = 0;
            options.tintGreen = 0;
            options.tintBlue = 0;
            this->slotTint = kSlotFadeEnd;
        }

        if (this->picture != nullptr)
        {
            painter.DrawPicture(
                static_cast<i16>(this->aimX),
                static_cast<i16>(this->aimY),
                this->picture->width,
                this->picture->height,
                this->unk26A4,
                PaletteArgument(*this->picture)
            );
        }

        this->slotTint = static_cast<i16>(this->slotTint - kSlotFadeStep);

        if (this->held == 1)
        {
            this->slotTint = static_cast<i16>(this->slotTint + kSlotFadeStep);
        }

        return 0;
    }

    if (attack.waiting != 1)
    {
        return 0;
    }

    this->standAct = ActState{};
    this->recoilAct = ActState{};
    follow = ActState{};
    this->slotActs[kFlightAct] = ActState{};
    this->slotActs[kFlightAct + 1] = ActState{};
    this->targets.fill(BattleUnit::NoTarget);
    this->slotTint = 0;
    this->recoilTint = 1;
    this->unk2AB0 = 0;
    this->fadedIn = 0;
    this->unk267C = 0;

    if (battle.criticalAttack == 0)
    {
        if (this->extraAttacks == 0)
        {
            this->unk26CC &= ~kRepeatAttack;
        }

        if (this->extraAttacks != 0)
        {
            --this->extraAttacks;
            return 0;
        }
    }

    ++battle.attacksSinceCritical;
    this->critical = 0;
    battle.criticalAttack = 0;
    return 1;
}

// swd32.exe 0x004707B0: item `id` of mon.dat taken as the role's action: its act, its command script (an item without
// a resistance element takes its +0x34 word as the value) and UnkCommandFlag8000. The original frees the description
// of the unit's own object (+0x0C) instead of the item's, which it leaks.
void BattleRole::LoadItemAction(Database& database, u16 id, u32& act)
{
    ObjectRecord item;
    database.LoadObject(item, id);

    // A unit without its object is read through a null pointer in the original.
    if (this->data != nullptr)
    {
        Database::FreeDescription(*this->data);
    }

    act = item.data.unk50;
    this->LoadCommand(database, item.data.unk3E);

    if (this->command.resistance == 0)
    {
        this->command.value = static_cast<i16>(item.data.unk34);
    }

    this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
}

// swd32.exe 0x004710D0: the source unit's idle picture breaks into particles flying into the role (a point 0x32 left
// of and 0x28 below its act's anchor): the picture is decoded to raw pixels at the source's origin, a tall one (100
// rows or more) bursting in half its height, a shorter one in its height less 10, mirrored like the source. The
// projector clips to the screen; the follow-up line, the attack act and the five extra acts are cleared.
void BattleRole::StartAbsorb(BattleServices& services, BattleUnit& source)
{
    u16 sourceX = 0;
    u16 sourceY = 0;
    this->absorbSource = source.StepIdleAction(services);
    source.GetOrigin(sourceX, sourceY);
    ParticleEffect& effect = this->absorb;
    effect = ParticleEffect{};
    this->absorbPixels.clear();

    // A source without its picture is read through a null pointer in the original.
    if (this->absorbSource == nullptr)
    {
        return;
    }

    const std::optional<RawPicture> raw = RleCodec(services.painter).Decode(this->absorbSource->data);

    if (raw.has_value())
    {
        this->absorbPixels.resize(raw->pixels.size() / sizeof(u16));
        std::memcpy(this->absorbPixels.data(), raw->pixels.data(), this->absorbPixels.size() * sizeof(u16));
    }

    effect.pixels = this->absorbPixels;
    effect.width = this->absorbSource->width;
    effect.height = this->absorbSource->height;
    effect.x = static_cast<i16>(sourceX) - 1;
    effect.y = static_cast<i16>(sourceY);
    effect.lifeBase = kAbsorbLife;
    effect.speed = kAbsorbSpeed;
    effect.targetLeft =
        static_cast<i16>(this->actAnchorX) - static_cast<i16>(this->originX) + this->placement.x - kAbsorbLeft;
    effect.targetTop = this->placement.y - this->mainAct.y + kAbsorbDown;
    effect.targetWidth = 1;
    effect.targetHeight = 1;
    const u16 height = this->absorbSource->height;
    effect.bursts = height >= kTallPicture ? static_cast<u16>(height >> 1) : static_cast<u16>(height - kShortBurstCut);
    effect.divisor = kAbsorbDivisor;
    effect.tintRed = kAbsorbTint;
    effect.tintGreen = kAbsorbTint;
    effect.tintBlue = kAbsorbTint;
    effect.flags = kAbsorbFlags;

    if (source.Mirrored() == 1)
    {
        effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mirrored);
    }

    this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag800);
    services.state.projector.SetScreen(Display::Width, Display::Height);
    this->unk2680 = 0;
    this->flight = LineWalker{};
    this->attackAct = ActState{};
    this->extraActs.fill(ActState{});
}

// swd32.exe 0x00471FC0: one frame of extra act `slot` (`id`, `action`) flying from (x, y) into the role's main act
// anchor, extraSteps[slot] * `steps` line steps a frame (one more each frame). Its sound plays centred; past the
// role's x less unk2674 it is drawn there for the last time and its steps and the line are cleared.
void BattleRole::FlyExtraAct(BattleServices& services, i32 id, i32 action, int slot, i16 x, i16 y, int steps)
{
    ActState& act = this->extraActs[static_cast<std::size_t>(slot)];
    i32& count = this->extraSteps[static_cast<std::size_t>(slot)];
    act.id = id;
    act.action = action;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    LineWalker& line = this->flight;
    line = LineWalker{};
    line.fromX = static_cast<i16>(x - static_cast<u16>(act.x));
    line.fromY = static_cast<i16>(y - static_cast<u16>(act.y));
    line.toX = this->placement.x - static_cast<i16>(this->originX) + static_cast<i16>(this->mainAct.anchorX) - act.x;
    line.toY = this->placement.y + static_cast<i16>(this->mainAct.anchorY) - this->mainAct.y - act.y;

    for (int taken = 0; taken < count * steps; ++taken)
    {
        LineStep(line);
    }

    ++count;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const int endX = line.toX - this->unk2674;
    const int atX = line.fromX + line.x;

    if (atX >= endX)
    {
        painter.DrawPicture(endX, line.toY, this->picture->width, this->picture->height, act.flags);
        count = 0;
        line = LineWalker{};
        return;
    }

    painter.DrawPicture(atX, line.fromY + line.y, this->picture->width, this->picture->height, act.flags);
}

// swd32.exe 0x00471270: one frame of the absorb; 1 once the picture is gone (its pixels freed, the effect, the extra
// steps, the follow-up line and the attack act cleared). While bursts are left the sparks fly from a quarter across the
// picture's remaining top into the role, one more joining every ten frames.
int BattleRole::AbsorbFrame(BattleServices& services)
{
    ParticleEffect& effect = this->absorb;
    ++this->absorbFrames;
    effect.tintRed = kAbsorbTint;
    effect.tintGreen = kAbsorbTint;
    effect.tintBlue = kAbsorbTint;
    this->absorbing = 1;

    if (services.state.projector.DrawExplosion(services.screen, services.painter, services.crtRandom, effect) == 1)
    {
        this->absorbFrames = 0;
        this->absorbing = 0;
        this->absorbPixels.clear();
        effect = ParticleEffect{};
        this->extraSteps.fill(0);
        this->flight = LineWalker{};
        this->attackAct = ActState{};
        return 1;
    }

    if (effect.bursts == 0)
    {
        return 0;
    }

    for (std::size_t slot = 0; slot < kSparks.size(); ++slot)
    {
        const Spark& spark = kSparks[slot];

        if (slot != 0 && this->absorbFrames < spark.fromFrame)
        {
            continue;
        }

        const int x = effect.x + (effect.width >> kSparkColumnShift);
        const int y = effect.height - effect.bursts + effect.y + spark.offsetY;
        this->FlyExtraAct(
            services,
            kSparkAct,
            spark.action,
            static_cast<int>(slot),
            static_cast<i16>(x),
            static_cast<i16>(y),
            spark.steps
        );
    }

    return 0;
}
}  // namespace swd3
