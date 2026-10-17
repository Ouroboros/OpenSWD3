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
#include "crt/msvc.h"
#include "draw/painter.h"
#include "org/database.h"
#include "org/fame.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "random/random.h"
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
    this->inventoryNode = nullptr;
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

// ThrowAct.
constexpr i32 kThrownAct = 0x186B;
constexpr int kThrowSteps = 8;    // line steps a frame per frame flown
constexpr i32 kCaughtAction = 1;  // CatchAct: the thrown act flying back

// GlowAct.
constexpr i32 kGlowAction = 0x24;
constexpr u16 kGlowRising = 0;
constexpr u16 kGlowFalling = 1;
constexpr u16 kGlowDone = 2;
constexpr i16 kGlowStep = 2;
constexpr i16 kGlowPeak = 0x3E;
constexpr int kGlowTintBase = 0x1F;  // the tint is glowLevel / 2 - kGlowTintBase
constexpr u16 kGlowEndSound = 0x6A;

// PoseAtTarget.
constexpr i32 kTargetPose = 0x2B;
constexpr u32 kMirroredOnce = 0x8000;  // ActState::unknown1C: the act was mirrored already
constexpr int kShadowShare = 3;        // the shadow is a third of the picture height high
constexpr std::size_t kShadowShearShift = 2;
constexpr int kShadowTint = -6;
constexpr int kFaintShadowTint = -1;
constexpr int kShadowLeft = 5;
constexpr int kPanCentre = 0x140;  // the sound pans right of the middle of the 640-pixel screen
constexpr int kTargetPan = 0x10;

// Steal and AttackPose.
constexpr u16 kNothingToSteal = 25000;  // Steal: the target has no item
constexpr u16 kStealRoll = 10;          // the roll is 0..9 less one per kStealShare percent of proficiency
constexpr u16 kStealShare = 25;
constexpr u16 kSureSteal = 100;   // a proficiency of 100 or more always steals
constexpr u16 kHitCast = 0x8000;  // AttackPose: the cast act with bit 15 tells that the attack hits
constexpr u16 kPoseDone = 2;
constexpr i32 kCastPose = 0x2D;

// BeamSpellFrame: the stages in unk267C.
constexpr u32 kBeamGlow = 0x1;
constexpr u32 kBeamRise = 0x2;
constexpr u32 kBeamFly = 0x4;
constexpr u32 kBeamImpact = 0x4000;
constexpr u32 kBeamFadeIn = 0x2000;
constexpr i16 kBeamRiseStart = -0x1F;
constexpr i16 kBeamGlowEnd = -0x1E;
constexpr i16 kBeamFadeStep = 2;
constexpr i32 kRiseAct = 0x17FE;
constexpr i32 kImpactAct = 0x1F88;
constexpr int kSpeedingFlight = -1;  // FlyAct: the steps double each frame

// ChargeAttackFrame.
constexpr i32 kChargeAction = 0x28;  // past unk2A8A
constexpr u16 kChargePicture = 2;    // the charge act is run on to its third picture
constexpr int kChargeAlpha = 8;
constexpr int kChargeBlue = 10;
constexpr int kDashStep = 10;
constexpr int kDashLength = 150;
constexpr u16 kNoUnk2A8E = 0xFFFF;
constexpr i32 kTrailBlueTint = 0xC;
constexpr i8 kChargeTrailImages = 3;

// SwarmSpellFrame.
constexpr i16 kSwarmInterval = 3;  // a new act every third frame
constexpr u16 kSwarmActs = 8;
constexpr int kSwarmAbove = 0x28;
constexpr u16 kSwarmShake = 0x34;
constexpr u16 kSwarmSound = 0x3E;

// BoomerangFrame: the stages in unk267C.
constexpr u32 kBoomerangGoing = 0;
constexpr u32 kBoomerangReturning = 1;
constexpr u32 kBoomerangBackHome = 2;
constexpr i32 kBoomerangAct = 0x1BF3;
constexpr i16 kBoomerangOut = 0x1C;
constexpr i16 kBoomerangBack = 0x20;
constexpr u16 kBoomerangShake = 7;
constexpr i32 kNoImpact = -1;  // a queued act without an impact act that lands no hit
constexpr u16 kBoomerangSound = 0x114;

// LeapSpellFrame: the stages in unk267C.
constexpr u32 kLeapCasting = 0;
constexpr u32 kLeapAtTarget = 1;
constexpr u32 kLeapReturning = 2;
constexpr u32 kLeapHome = 3;
constexpr i32 kLeapReturnAction = 2;  // the walk home takes the action two after the magic act's

// UseSelectedItem: bits of the item's object.
constexpr u16 kMultiUse = 0x400;  // ObjectData::unk3C: the item has uses in the low byte
constexpr u16 kUsesMask = 0xFF;
constexpr u32 kKeptKind = 0x80;  // ObjectData::kind: not spent by a use
constexpr u32 kUnkKind8000000 = 0x8000000;
constexpr u16 kNoUseAct = 0x2000;  // ObjectData::unk40: the use returns no item

// CancelCommand and AddFury.
constexpr std::size_t kCancelledNameBytes = 0x10;  // only the start of the skill name is cleared
constexpr int kFillFury = -1;
constexpr i32 kFuryFull = 0x38;
constexpr double kFuryScale = 47.0;  // swd32.exe 0x00499D40
constexpr u16 kFurySound = 0xBC;

// Regenerate: the gains show 6 frames apart.
constexpr u16 kRegenerateDelay = 6;

// UseAccessorySkill: the unk26CC bit of each accessory kind (RepeatAttack is kRepeatAttack).
constexpr u32 kAccessoryBit16 = 0x2;
constexpr u32 kAccessoryBit17 = 0x4;
constexpr u32 kAccessoryBit18 = 0x8;
constexpr u32 kAccessoryBit19 = 0x10;
constexpr u32 kAccessoryBit1A = 0x20;
constexpr u32 kAccessoryBit1B = 0x40;
constexpr u32 kAccessoryBit1D = 0x100;
constexpr u32 kAccessoryBit1E = 0x200;
constexpr u16 kAccessoryStep = 5;        // the proficiency steps of an accessory command
constexpr u16 kDrainStep = 0xC;          // the proficiency the life-drain accessory gains from a drain
constexpr u32 kCountedKind = 0x8000000;  // ObjectData::kind bit of the objects CountUsableObjects counts
constexpr u16 kThrownObject = 0x300;
constexpr u8 kCountedUsers = 5;        // usableBy (low byte) bits 0 and 2
constexpr u16 kListedObject = 0x2000;  // ObjectData::unk40 bit of the objects ListedObject counts
constexpr u32 kCategory0 = 0x10;       // the kind bits of the list categories (BattleRole::CategoryKinds)
constexpr u32 kCategory1 = 0xC;
constexpr u32 kCategory2 = 0x1001;
constexpr u32 kCategory3 = 0x800;
constexpr u32 kCategory4 = 0x2000;
constexpr u32 kCategory5 = 0x8000000;
constexpr int kSkillCategories = 3;
constexpr int kListCategories = 4;
constexpr int kObjectCategories = 6;
constexpr int kSkillType0 = 0x1C;  // ObjectData::unk52 of the skill types
constexpr int kSkillType1 = 0x1F;
constexpr u16 kFirstSkillType = 0x1B;
constexpr u16 kLastSkillType = 0x1E;
constexpr u16 kShownObject = 0x8000;  // ListObject flags
constexpr u16 kCostlyObject = 0x4000;
constexpr u16 kCostsStamina = 0x4000;  // ObjectData::unk40: the low 14 bits are a stamina cost
constexpr u16 kCostMask = 0x3FFF;
constexpr int kUnk1FScale = 4;           // Unk1F: unk29A4 is 4 percent of the proficiency and 1
constexpr int kAccessoryBaseShare = 10;  // the heals add a tenth of their base
constexpr int kHalfShare = 5;            // Unk22 / Unk24: the proficiency is halved, less a fifth of that
constexpr int kSpiritShare = 5;          // Unk23: a fifth of the proficiency

// RollCritical and PrepareCommand.
constexpr u32 kCriticalAttacks = 8;  // a critical may be rolled after 8 attacks, is sure after roleCount more
constexpr u32 kCriticalRoll = 10;
constexpr u16 kCriticalChance = 5;  // a roll above it is critical
constexpr u16 kWalkCommand = 0xC8;  // command A of a walking attack (its command in command B)

// BeatsLevel: chances out of kLevelRoll.
constexpr u32 kLevelRoll = 100;
constexpr u32 kAboveChance = 35;
constexpr u32 kCloseChance = 70;  // 1..kNearBelow-1 levels below
constexpr u32 kNearChance = 90;   // kNearBelow..kSureBelow-1 levels below
constexpr int kNearBelow = 8;
constexpr int kSureBelow = 13;

// StepBack.
constexpr i32 kStepBackAction = 0x2A;
constexpr i16 kStandBackEnd = 2;
constexpr i16 kStepBackEnd = 6;
constexpr u16 kStepBackSound = 0x2F;
constexpr int kStepBackShift = 0x10;

// CanAbsorb.
constexpr u16 kMaxAbsorbClass = 0x15;   // ObjectData::unk52
constexpr int kAbsorbLevelsAbove = 12;  // a target this many levels above the role is never absorbed
constexpr int kSureAbsorbBelow = 10;
constexpr int kQuarterAbove = 7;
constexpr int kFarBelow = 5;
constexpr int kQuarter = 4;
constexpr int kThird = 3;
constexpr u32 kAbsorbRoll = 100;
constexpr int kFarBelowChance = 80;
constexpr int kAbsorbChanceBase = 20;
constexpr int kAbsorbChanceStep = 5;  // less per level above
constexpr int kBelowChanceStep = 10;  // more per level below
constexpr std::array<Spark, BattleRole::ExtraActs> kSparks = {{
    {0, 1, -5, 0xE},
    {10, 2, -10, 0xA},
    {20, 3, -15, 0xC},
    {30, 1, 0, 8},
    {40, 1, 5, 0x10},
}};

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
    this->usedItem = 0;
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
void BattleRole::Setup(RoleRecord& record, RoleBattleBlock& block, const UnitPlacement& at, ErrorReporter& reporter)
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
    this->inventoryNode = nullptr;
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
    this->throwSteps = 0;
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

// swd32.exe 0x004717F0: one frame of the thrown act (kThrownAct in the attack act) flying from the role's main act
// anchor to the target's anchor (its draw position when the anchor has a zero coordinate), mirrored by the facing.
// The flight takes 8 line steps a frame more each frame; once at the target's x it is drawn there, its steps, the line
// and the attack act are cleared and 1 is returned.
int BattleRole::ThrowAct(BattleServices& services, const BattleUnit& target)
{
    u16 anchorX = 0;
    u16 anchorY = 0;
    ActState& act = this->attackAct;
    act.id = kThrownAct;
    act.action = 0;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    this->actOffsetX = static_cast<u16>(act.x);
    this->frameFlags = act.flags;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
    }

    target.GetOffset(anchorX, anchorY);
    i16 toX = 0;
    i16 toY = 0;

    if (anchorX != 0 && anchorY != 0)
    {
        u16 originX = 0;
        u16 originY = 0;
        target.GetOrigin(originX, originY);
        toX = static_cast<i16>(originX + anchorX - this->actOffsetX);
        toY = static_cast<i16>(originY + anchorY - static_cast<u16>(act.y));
    }
    else
    {
        u16 x = 0;
        u16 y = 0;
        target.GetDrawPosition(x, y);
        toX = static_cast<i16>(x - this->actOffsetX);
        toY = static_cast<i16>(y - static_cast<u16>(act.y));
    }

    LineWalker& line = this->flight;
    line = LineWalker{};
    line.fromX = static_cast<i16>(this->actAnchorX) - static_cast<i16>(this->originX) + this->placement.x -
                 static_cast<i16>(this->actOffsetX);
    line.fromY = this->placement.y + static_cast<i16>(this->mainAct.anchorY) - this->mainAct.y - act.y;
    line.toX = toX;
    line.toY = toY;
    bool arrived = false;

    for (int taken = 0; taken < kThrowSteps * this->throwSteps; ++taken)
    {
        LineStep(line);

        if (line.fromX + line.x == line.toX)
        {
            arrived = true;
            break;
        }
    }

    ++this->throwSteps;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;

    if (arrived)
    {
        painter.DrawPicture(toX, toY, this->picture->width, this->picture->height, this->frameFlags);
        this->throwSteps = 0;
        line = LineWalker{};
        act = ActState{};
        return 1;
    }

    painter.DrawPicture(
        line.fromX + line.x,
        line.fromY + line.y,
        this->picture->width,
        this->picture->height,
        this->frameFlags
    );
    return 0;
}

// swd32.exe 0x00471AD0: one frame of the thrown act flying back (action kCaughtAction, unmirrored, with its own flags)
// from the target's anchor (its draw position when the anchor has a zero coordinate) into the role's main act anchor,
// 8 line steps a frame more each frame; once at the role's x it is drawn there, its steps, the line and the attack act
// are cleared and 1 is returned.
int BattleRole::CatchAct(BattleServices& services, const BattleUnit& target)
{
    u16 anchorX = 0;
    u16 anchorY = 0;
    ActState& act = this->attackAct;
    act.id = kThrownAct;
    act.action = kCaughtAction;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    target.GetOffset(anchorX, anchorY);
    i16 fromX = 0;
    i16 fromY = 0;

    if (anchorX != 0 && anchorY != 0)
    {
        u16 originX = 0;
        u16 originY = 0;
        target.GetOrigin(originX, originY);
        fromX = static_cast<i16>(originX + anchorX - static_cast<u16>(act.x));
        fromY = static_cast<i16>(originY + anchorY - static_cast<u16>(act.y));
    }
    else
    {
        u16 x = 0;
        u16 y = 0;
        target.GetDrawPosition(x, y);
        fromX = static_cast<i16>(x - static_cast<u16>(act.x));
        fromY = static_cast<i16>(y - static_cast<u16>(act.y));
    }

    LineWalker& line = this->flight;
    line = LineWalker{};
    line.fromX = fromX;
    line.fromY = fromY;
    line.toX = this->placement.x - static_cast<i16>(this->originX) + static_cast<i16>(this->mainAct.anchorX) - act.x;
    line.toY = this->placement.y + static_cast<i16>(this->mainAct.anchorY) - this->mainAct.y - act.y;
    bool arrived = false;

    for (int taken = 0; taken < kThrowSteps * this->throwSteps; ++taken)
    {
        LineStep(line);

        if (line.fromX + line.x == line.toX)
        {
            arrived = true;
            break;
        }
    }

    ++this->throwSteps;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;

    if (arrived)
    {
        painter.DrawPicture(line.toX, line.toY, this->picture->width, this->picture->height, act.flags);
        this->throwSteps = 0;
        line = LineWalker{};
        act = ActState{};
        return 1;
    }

    painter
        .DrawPicture(line.fromX + line.x, line.fromY + line.y, this->picture->width, this->picture->height, act.flags);
    return 0;
}

// swd32.exe 0x00471D60: one frame of the role's glow act (usedItem, action kGlowAction) drawn additively at (x, y) less
// its offset, mirrored when the role does not face (the original mirrors on facing 0); 1 once it is over. It brightens
// by 2 a frame up past kGlowPeak, then dims back to 0 (counting absorbFrames), then sound kGlowEndSound plays and the
// glow, the attack act and the extra acts are cleared. The original clears the sound of the recoil act instead of this
// act's.
int BattleRole::GlowAct(BattleServices& services, int x, int y)
{
    ActState& act = this->attackAct;
    act.id = this->usedItem;
    act.action = kGlowAction;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);
    services.audio.PlaySound(act.view, services.config.soundVolume);
    this->recoilAct.view = 0;
    this->glowFlags = act.flags;
    this->glowOffsetX = act.x;

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    if (this->facing == 0)
    {
        this->glowFlags ^= Painter::Mirror;
        this->glowOffsetX = this->picture->width - act.x;
    }

    DrawOptions& options = services.painter.Options();

    auto tint = [this, &options]
    {
        const int level = this->glowLevel / 2 - kGlowTintBase;
        options.tintRed = level;
        options.tintGreen = level;
        options.tintBlue = level;
    };

    if (this->glowPhase == kGlowRising)
    {
        tint();
        this->glowLevel = static_cast<i16>(this->glowLevel + kGlowStep);

        if (this->glowLevel > kGlowPeak)
        {
            this->glowPhase = kGlowFalling;
        }
    }

    if (this->glowPhase == kGlowFalling)
    {
        tint();
        this->glowLevel = static_cast<i16>(this->glowLevel - kGlowStep);
        ++this->absorbFrames;

        if (this->glowLevel <= 0)
        {
            this->glowPhase = kGlowDone;
        }
    }

    services.painter.SetSource(this->picture->data.data());
    services.painter.DrawPicture(
        x - this->glowOffsetX,
        y - act.y,
        this->picture->width,
        this->picture->height,
        this->glowFlags | Painter::Additive
    );

    if (this->glowPhase != kGlowDone)
    {
        return 0;
    }

    services.audio.PlaySound(kGlowEndSound, services.config.soundVolume);
    this->glowLevel = 0;
    this->glowFlags = 0;
    this->glowOffsetX = 0;
    this->glowPhase = kGlowRising;
    this->absorbFrames = 0;
    this->hurtStep = 0;
    act = ActState{};
    this->extraActs.fill(ActState{});
    return 1;
}

// swd32.exe 0x004721F0: one frame of the role's pose kTargetPose (in the stand act) at the target's draw position over
// its shadow; 1 (the stand act cleared, nothing drawn) once the act waits. A facing role mirrors the act once: the
// mirrored flags with kMirroredOnce go to unknown1C, which keeps the next frames from mirroring the x again. The
// shadow is fainter with unk2B00; the sound pans right of the middle of the screen.
int BattleRole::PoseAtTarget(BattleServices& services, const BattleUnit& target)
{
    u16 targetX = 0;
    u16 targetY = 0;
    ActState& act = this->standAct;
    act.action = kTargetPose;
    act.hold = 0;
    act.id = this->actId;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());

    if (act.waiting != 0)
    {
        act = ActState{};
        return 1;
    }

    if (this->facing == 1 && act.unknown1C == 0)
    {
        act.flags ^= Painter::Mirror;
        act.unknown1C = static_cast<i32>(act.flags | kMirroredOnce);
        act.x = this->picture->width - act.x;
    }

    target.GetDrawPosition(targetX, targetY);
    const auto x = static_cast<i16>(targetX - static_cast<u16>(act.x));
    const auto y = static_cast<i16>(targetY - static_cast<u16>(act.y));
    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const int tint = this->unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, x > kPanCentre ? kTargetPan : -kTargetPan);
    this->unk26A4 = (act.flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter.DrawPicture(
        x - kShadowLeft,
        act.y + y - shadowHeight,
        this->picture->width,
        this->picture->height,
        this->unk26A4
    );
    painter.DrawPicture(x, y, this->picture->width, this->picture->height, act.flags);
    return 0;
}

// swd32.exe 0x00472430: the item stolen from the target (taken from it), 0 when the roll fails, kNothingToSteal when it
// has none. Below 100 the steal proficiency (of the worn steal accessory, ProficiencyList) lowers a 0..9 roll by one
// per 25 percent; the roll must stay below the target's steal difficulty.
u16 BattleRole::Steal(BattleServices& services, const BattleUnit& target)
{
    ObjectData& data = target.Data()->data;

    if (data.stealItem == 0)
    {
        return kNothingToSteal;
    }

    this->AccessoryProficiency(
        static_cast<u16>(AccessoryKind::Steal),
        services.party.fame.lists[Fame::ProficiencyList]
    );

    if (this->unk26DC < kSureSteal)
    {
        const auto roll = static_cast<u16>(services.random.Below(kStealRoll));
        const int cut = this->unk26DC / kStealShare;
        const u16 value = cut > roll ? u16{0} : static_cast<u16>(roll - cut);

        if (value >= data.stealDifficulty)
        {
            return 0;
        }
    }

    const u16 item = data.stealItem;
    data.stealItem = 0;
    return item;
}

// swd32.exe 0x00472710: always 1 (the target's object is read and not used)
int BattleRole::CanTarget(const BattleUnit& target)
{
    target.Data();
    return 1;
}

// swd32.exe 0x004724D0: one frame of the role's attack pose (kAttackAction in the stand act) at its place over its
// shadow, the sound panned left. The draw takes the act's own x and flags (the mirrored offset and flags it computes
// for a facing role are kept but not used). It returns the cast act with kHitCast once the act strikes or follows up
// (the bits cleared), kPoseDone (the act cleared) once it waits, otherwise 0.
u16 BattleRole::AttackPose(BattleServices& services)
{
    ActState& act = this->standAct;
    act.id = this->actId;
    this->unk2AAC = 1;
    act.action = kAttackAction;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    this->actOffsetX = static_cast<u16>(act.x);
    this->frameFlags = act.flags;
    this->actAnchorX = act.anchorX;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    }

    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.tintRed = kShadowTint;
    options.tintGreen = kShadowTint;
    options.tintBlue = kShadowTint;
    options.shear = this->picture->height >> kShadowShearShift;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, -kTargetPan);
    act.view = 0;
    this->unk26A4 = (act.flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    const int x = this->placement.x - act.x;
    painter
        .DrawPicture(x, this->placement.y - shadowHeight, this->picture->width, this->picture->height, this->unk26A4);
    painter.DrawPicture(x, this->placement.y - act.y, this->picture->width, this->picture->height, act.flags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) != 0)
    {
        act.target = 0;
        return static_cast<u16>(this->command.castAct | kHitCast);
    }

    if (act.waiting != 1)
    {
        return 0;
    }

    this->unk2AAC = 0;
    act = ActState{};
    return kPoseDone;
}

// swd32.exe 0x00472730: whether the role absorbs the target: only an Absorbable monster (not UnkKind20) of class up to
// kMaxAbsorbClass, at most 11 levels above the role. Ten levels or more below it always; 7..11 above it at a quarter of
// its hit points or less; otherwise at a third or less, or by a 0..99 roll: up to 80 for 5..9 levels below, 20 less 5
// per level above, 20 plus 10 per level below.
int BattleRole::CanAbsorb(BattleServices& services, const BattleUnit& target) const
{
    const ObjectData& data = target.Data()->data;
    i32 hitPoints = 0;
    i32 maximum = 0;
    target.GetHitPoints(hitPoints, maximum);

    if ((data.kind & ObjectData::UnkKind20) != 0 || (data.kind & ObjectData::Absorbable) == 0 ||
        data.unk52 > kMaxAbsorbClass)
    {
        return 0;
    }

    const int roleLevel = this->battleRecord->level;
    const int above = data.level - roleLevel;

    if (above >= kAbsorbLevelsAbove)
    {
        return 0;
    }

    const int below = roleLevel - data.level;

    if (below >= kSureAbsorbBelow)
    {
        return 1;
    }

    if (above >= kQuarterAbove && above <= kAbsorbLevelsAbove - 1)
    {
        return hitPoints <= maximum / kQuarter ? 1 : 0;
    }

    auto weakOrRoll = [&](int chance)
    {
        if (hitPoints <= maximum / kThird)
        {
            return 1;
        }

        return static_cast<int>(services.random.Below(kAbsorbRoll)) <= chance ? 1 : 0;
    };

    if (below >= kFarBelow && below < kSureAbsorbBelow)
    {
        return weakOrRoll(kFarBelowChance);
    }

    if (above < kQuarterAbove && above >= 1)
    {
        return weakOrRoll(kAbsorbChanceBase - kAbsorbChanceStep * above);
    }

    if (below >= kFarBelow || below < 0)
    {
        return 0;
    }

    return weakOrRoll(kBelowChanceStep * below + kAbsorbChanceBase);
}

// swd32.exe 0x004728E0: one frame of the role's direct attack (pose kForcedAttackAction in the stand act) at its
// place, the shadow at the target's height. When the act strikes or follows up, the hit lands at once: its damage
// (at most kMaxDamage, without the proficiency bonus and counted even when no number shows) adds to the hit's damage
// and shows on the target, and the follow-up act (the cast act) is placed on the target and drawn each frame; 1 (the
// acts cleared) once the stand act waits after it.
int BattleRole::DirectAttack(BattleServices& services, BattleUnit& target)
{
    BattleState& battle = services.state;
    u16 targetX = 0;
    u16 targetY = 0;
    ActState& act = this->standAct;
    act.id = this->actId;
    this->unk2AAC = 1;
    act.action = kForcedAttackAction;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    this->frameFlags = act.flags;
    this->actOffsetX = static_cast<u16>(act.x);
    this->actAnchorX = act.anchorX;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    }

    target.GetDrawPosition(targetX, targetY);
    const auto shadowY = static_cast<i16>(targetY - static_cast<u16>(act.y));
    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const int tint = this->unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int x = this->placement.x - static_cast<i16>(this->actOffsetX);
    services.audio.SetSoundPan(act.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
    this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter.DrawPicture(x, act.y + shadowY - shadowHeight, this->picture->width, this->picture->height, this->unk26A4);
    painter.DrawPicture(x, this->placement.y - act.y, this->picture->width, this->picture->height, this->frameFlags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) != 0)
    {
        act.target = 0;
        this->unk267C = BattleUnit::FollowingUp;
        target.RaiseFlag26B8();
        i16 spirit = 0;
        i16 stamina = 0;
        i32 damage = this->HitDamage(services, target, spirit, stamina);
        battle.lastDamage = damage;

        if (damage >= kMaxDamage)
        {
            damage = kMaxDamage;
            battle.lastDamage = damage;
        }

        battle.hitDamage += damage;
        target.SetDamageDigits(services, damage);
        target.StepTargetCount(1);
    }

    if (this->unk267C != BattleUnit::FollowingUp)
    {
        return 0;
    }

    ActState& follow = this->slotActs[0];
    follow.id = this->command.castAct;
    follow.action = 0;
    this->AimAction(services, target, follow, 0, 0);

    if (this->picture != nullptr)
    {
        painter.SetSource(this->picture->data.data());
        painter.DrawPicture(
            static_cast<i16>(this->aimX),
            static_cast<i16>(this->aimY),
            this->picture->width,
            this->picture->height,
            this->unk26A4
        );
    }

    if (act.waiting != 1)
    {
        return 0;
    }

    this->unk267C = 0;
    follow = ActState{};
    act = ActState{};
    return 1;
}

// swd32.exe 0x00472CE0: one frame of the role's cast pose (kCastPose in the stand act, its hold left as it is) at its
// place over its shadow, mirrored by the facing; once the act strikes or follows up (the bits stay set) the spell act
// `spell` (slot act 0) is drawn on the target each frame, and 1 (both acts cleared) tells that it waits.
int BattleRole::CastAt(BattleServices& services, const BattleUnit& target, i32 spell)
{
    ActState& act = this->standAct;
    act.id = this->actId;
    this->unk2AAC = 1;
    act.action = kCastPose;

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    u32 flags = act.flags;
    auto offsetX = static_cast<i16>(act.x);
    this->actAnchorX = act.anchorX;

    if (this->facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = static_cast<i16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    }

    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const int tint = this->unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int x = this->placement.x - offsetX;
    services.audio.SetSoundPan(act.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
    this->unk26A4 = (flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter
        .DrawPicture(x, this->placement.y - shadowHeight, this->picture->width, this->picture->height, this->unk26A4);
    painter.DrawPicture(x, this->placement.y - act.y, this->picture->width, this->picture->height, flags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) == 0)
    {
        return 0;
    }

    ActState& effect = this->slotActs[0];
    effect.id = spell;
    effect.action = 0;

    if (services.acts.Run(effect) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(effect.frame, effect.picture);

    if (this->picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(this->picture->data.data());
    u32 effectFlags = effect.flags;
    auto effectOffsetX = static_cast<i16>(effect.x);
    this->actAnchorX = effect.anchorX;

    if (this->facing == 1)
    {
        effectFlags ^= Painter::Mirror;
        effectOffsetX = static_cast<i16>(this->picture->width - static_cast<u16>(effect.x));

        if (effect.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - effect.anchorX);
        }
    }

    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    painter.DrawPicture(
        static_cast<i16>(targetX - effectOffsetX),
        static_cast<i16>(targetY - static_cast<u16>(effect.y)),
        this->picture->width,
        this->picture->height,
        effectFlags
    );

    if (effect.waiting != 1)
    {
        return 0;
    }

    effect = ActState{};
    act = ActState{};
    return 1;
}

// swd32.exe 0x00473010: one frame of the role's magic act (its act + MagicActs, action unk2A8A, slot act 8) drawn by
// DrawAction; its target bits play the effect in the recoil act (holding the act) and fade the field by its lights;
// a follow-up marks the spell cast in BattleState::spellStage. 1 (slot act 8 and the stages cleared) once the battle
// marks the spell over.
int BattleRole::MagicFrame(BattleServices& services, const BattleUnit& target)
{
    BattleState& battle = services.state;
    ActState& act = this->slotActs[BattleUnit::UnkSlot8];
    act.action = this->unk2A8A;
    this->unk2AAC = 1;
    act.id = this->actId + BattleRole::MagicActs;
    this->DrawAction(services, target, act);

    if ((act.target & ActState::Recoils) != 0)
    {
        if (act.effectA != 0)
        {
            this->unk267C |= BattleUnit::Recoiling;
            this->recoilAct.id = act.effectA;
            this->recoilAct.action = act.effectQ;
        }

        if ((act.target & ActState::Holds) != 0)
        {
            act.hold = 1;
        }

        act.effectA = 0;
        act.target = static_cast<u16>(act.target & ~ActState::Recoils);
        act.effectQ = 0;
    }

    if ((this->unk267C & BattleUnit::Recoiling) != 0 &&
        this->DrawRecoil(services, this->recoilAct, static_cast<i16>(act.anchorY)) == 1)
    {
        this->unk267C &= ~BattleUnit::Recoiling;
        act.target = 0;
        act.hold = 0;
        this->recoilAct = ActState{};
    }

    if ((act.target & ActState::FollowsUp) != 0)
    {
        if ((act.target & ActState::FadesField) != 0)
        {
            battle.fadingField = 1;
            battle.StartFieldFade(
                static_cast<i16>(act.lights[0]),
                static_cast<i16>(act.lights[1]),
                static_cast<i16>(act.lights[2]),
                static_cast<i16>(act.lights[3]),
                static_cast<i16>(act.lights[4]),
                static_cast<i16>(act.lights[5]),
                static_cast<i16>(act.lights[6])
            );
            act.target = static_cast<u16>(act.target & ~ActState::FadesField);
        }

        if ((battle.spellStage & BattleState::SpellCast) == 0)
        {
            battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellCast);
        }
    }

    if ((battle.spellStage & BattleState::SpellOver) == 0)
    {
        return 0;
    }

    this->unk267C = 0;
    act = ActState{};
    return 1;
}

// swd32.exe 0x004735B0: one frame of the role's beam spell; 1 once it is over (the four acts cleared). Stage 0 draws
// the magic act (slot act 8) over its shadow until it follows up; then it glows away (stage bit 1: additive, glowLevel
// down to kBeamGlowEnd) while the rising act kRiseAct (slot act 9) brightens from slotTint -0x1F above the role
// (stage bit 2). Stage 4 sends kRiseAct flying (speeding) into the target; on arrival the hit lands (at most
// kMaxDamage, counted even when no number shows), and stage 0x4000 draws the impact act kImpactAct on the target until
// it waits. Stage 0x2000 fades the last picture back in with the magic act's flags, 2 a frame up to 0.
int BattleRole::BeamSpellFrame(BattleServices& services, BattleUnit& target)
{
    BattleState& battle = services.state;
    Painter& painter = services.painter;
    ActState& magic = this->slotActs[BattleUnit::UnkSlot8];
    ActState& rise = this->slotActs[BattleUnit::UnkSlot9];
    this->unk2AAC = 1;
    magic.id = this->actId + BattleRole::MagicActs;
    magic.action = this->unk2A8A;

    if (services.acts.Run(magic) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(magic.frame, magic.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(this->picture->data.data());
    auto offsetX = static_cast<i16>(magic.x);
    u32 flags = magic.flags;
    this->actAnchorX = magic.anchorX;

    if (this->facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = static_cast<i16>(this->picture->width - static_cast<u16>(magic.x));

        if (magic.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - magic.anchorX);
        }
    }

    const int x = this->placement.x - offsetX;
    const int magicY = this->placement.y - magic.y;

    auto tint = [&painter](int level)
    {
        DrawOptions& options = painter.Options();
        options.tintRed = level;
        options.tintGreen = level;
        options.tintBlue = level;
    };

    if ((magic.target & ActState::FollowsUp) != 0)
    {
        this->slotTint = kBeamRiseStart;
        this->unk267C |= kBeamGlow | kBeamRise;
        magic.target = 0;
        this->glowLevel = 0;
    }

    if (this->unk267C == 0)
    {
        services.audio.PlaySound(magic.view, services.config.soundVolume);
        services.audio.SetSoundPan(magic.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
        magic.view = 0;
        const int shadowHeight = this->picture->height / kShadowShare;
        DrawOptions& options = painter.Options();
        options.scaleHeight = shadowHeight;
        options.tintRed = kShadowTint;
        options.tintGreen = kShadowTint;
        options.tintBlue = kShadowTint;
        options.shear = this->picture->height >> kShadowShearShift;
        this->unk26A4 = (flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
        painter.DrawPicture(
            x - kShadowLeft,
            this->placement.y - shadowHeight,
            this->picture->width,
            this->picture->height,
            this->unk26A4
        );
        painter.DrawPicture(x, magicY, this->picture->width, this->picture->height, flags);
    }

    if ((this->unk267C & kBeamGlow) != 0)
    {
        if (this->glowLevel > kBeamGlowEnd)
        {
            tint(this->glowLevel);
            painter.DrawPicture(x, magicY, this->picture->width, this->picture->height, flags | Painter::Additive);
            --this->glowLevel;
        }
        else
        {
            this->unk267C &= ~kBeamGlow;
        }
    }

    if ((this->unk267C & kBeamRise) != 0)
    {
        rise.action = 0;
        rise.id = kRiseAct;

        if (services.acts.Run(rise) == 0)
        {
            return 0;
        }

        this->picture = services.archive.GetPicture(rise.frame, rise.picture);

        if (this->picture == nullptr)
        {
            return 0;
        }

        painter.SetSource(this->picture->data.data());
        flags = rise.flags;
        this->aimOffsetX = static_cast<u16>(rise.x);
        this->aimAnchorX = rise.anchorX;

        if (this->facing == 1)
        {
            flags ^= Painter::Mirror;
            this->aimOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(rise.x));

            if (rise.anchorX != 0)
            {
                this->aimAnchorX = static_cast<u16>(this->picture->width - rise.anchorX);
            }
        }

        services.audio.PlaySound(rise.view, services.config.soundVolume);
        services.audio.SetSoundPan(rise.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
        rise.view = 0;

        if (this->slotTint > 0)
        {
            this->unk267C = kBeamFly;
            this->glowLevel = 0;
            this->slotTint = 0;
        }
        else
        {
            tint(this->slotTint);
            const int riseX =
                static_cast<i16>(this->actAnchorX) - static_cast<i16>(this->aimAnchorX) - offsetX + this->placement.x;
            const int riseY =
                this->placement.y - static_cast<i16>(rise.anchorY) + static_cast<i16>(magic.anchorY) - magic.y;
            painter.DrawPicture(riseX, riseY, this->picture->width, this->picture->height, flags | Painter::Additive);
            ++this->slotTint;
        }
    }

    if (this->unk267C == kBeamFly)
    {
        const int fromX = this->placement.x - offsetX + static_cast<i16>(this->actAnchorX);
        const int fromY = this->placement.y + static_cast<i16>(magic.anchorY) - magic.y;

        if (this->FlyAct(services, target, this->slotActs[0], 0, kRiseAct, fromX, fromY, kSpeedingFlight, 0) == 1)
        {
            target.RaiseFlag26B8();
            i16 spirit = 0;
            i16 stamina = 0;
            i32 damage = this->HitDamage(services, target, spirit, stamina);
            battle.lastDamage = damage;

            if (damage >= kMaxDamage)
            {
                damage = kMaxDamage;
                battle.lastDamage = damage;
            }

            battle.hitDamage += damage;
            target.SetDamageDigits(services, damage);
            target.StepTargetCount(1);
            this->unk267C = kBeamImpact;
            this->glowLevel = kBeamRiseStart;
        }
    }

    if ((this->unk267C & kBeamImpact) != 0)
    {
        ActState& impact = this->slotActs[1];
        impact.id = kImpactAct;
        impact.action = 0;

        if (this->DrawAimed(services, target, impact, 0, 0) == 1)
        {
            this->unk267C = kBeamFadeIn;
        }
    }

    if ((this->unk267C & kBeamFadeIn) == 0)
    {
        return 0;
    }

    if (this->glowLevel <= 0)
    {
        tint(this->glowLevel);

        if (this->picture != nullptr)
        {
            painter.DrawPicture(x, magicY, this->picture->width, this->picture->height, flags | Painter::Additive);
        }

        this->glowLevel = static_cast<i16>(this->glowLevel + kBeamFadeStep);
        return 0;
    }

    this->glowLevel = 0;
    this->unk267C = 0;
    magic = ActState{};
    rise = ActState{};
    this->slotActs[0] = ActState{};
    this->slotActs[1] = ActState{};
    return 1;
}

// swd32.exe 0x00473C10: one frame of the role's charge attack (magic act in slot act 8 + `slot`, drawn by DrawAction);
// 1 once it is over (the acts, the trails and the targets cleared, the critical attack ended). When the magic act
// follows up with bit 8 the role dashes: its picture slides translucent (blue under it) by 10 a frame until the dash
// is a multiple of kDashLength, the charge act (kChargeAction past unk2A8A) holding, and a charge hit raises flag 26B8
// of the target. Charging, the charge act runs on the target with a trail (trail 1) and its follow-up (the cast act or
// effectA, slot act 1) mirrored twice; once the charge act waits the role slides back to 0. A strike of the magic act
// strikes; a follow-up runs the cast act (slot act 0) on the target, drawn with the shadow's palette.
int BattleRole::ChargeAttackFrame(BattleServices& services, BattleUnit& target, int slot)
{
    BattleState& battle = services.state;
    Painter& painter = services.painter;

    if (this->unk2A74 != 0 || this->unk2AD8 == 1)
    {
        return 0;
    }

    this->unk2AAC = 1;
    ActState& magic = this->slotActs[BattleUnit::UnkSlot8 + static_cast<std::size_t>(slot)];
    ActState& charge = this->attackAct;
    magic.id = this->actId + BattleRole::MagicActs;
    magic.action = this->unk2A8A;
    this->DrawAction(services, target, magic);

    auto slide = [this, &painter]
    {
        if (this->picture == nullptr)
        {
            return;
        }

        const int x = static_cast<i16>(this->actX) - this->fadeLevel;
        const int y = static_cast<i16>(this->actY);
        painter.Options().alpha = kChargeAlpha;
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->frameFlags | Painter::Translucent);
        painter.Options().tintBlue = kChargeBlue;
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->frameFlags | Painter::TintUnder);
    };

    auto endDash = [this, &magic, &charge]
    {
        magic.target = 0;
        this->glowLevel = 0;
        this->unk2678 = 0;
        this->unk2A8E = kNoUnk2A8E;
        magic.hold = 0;
        charge.hold = 0;
    };

    if ((magic.target & ActState::FollowsUp) != 0 && (magic.target & ActState::Dashes) != 0)
    {
        AfterimageTrail& dashTrail = this->trails[0];

        if (dashTrail.started == 0)
        {
            dashTrail = AfterimageTrail{};
            dashTrail.started = 1;

            for (AfterimageTrail::Image& image : dashTrail.images)
            {
                image.picture = AfterimageTrail::NoPicture;
            }

            dashTrail.tintBlue = kTrailBlueTint;
            this->fadeLevel = 0;
        }

        magic.hold = 1;
        charge.id = this->actId + BattleRole::MagicActs;
        charge.action = this->unk2A8A + kChargeAction;

        while (charge.picture < kChargePicture)
        {
            if (services.acts.Run(charge) == 0)
            {
                return 0;
            }
        }

        slide();
        this->fadeLevel += this->facing == 0 ? kDashStep : -kDashStep;
        charge.hold = 1;

        if ((charge.target & ActState::Strikes) != 0)
        {
            target.RaiseFlag26B8();
            charge.target = 0;
        }

        if (this->fadeLevel % kDashLength != 0)
        {
            return 0;
        }

        endDash();
        this->charging = 1;
    }

    if (this->charging == 1)
    {
        if (charge.waiting != 0)
        {
            slide();
            this->fadeLevel += this->facing == 0 ? -kDashStep : kDashStep;
            charge.hold = 1;

            if (this->fadeLevel == 0)
            {
                endDash();
                this->fadeLevel = 0;
                this->charging = 0;
            }
        }
        else if (this->ChargeOnTarget(services, target) == 0)
        {
            return 0;
        }
    }

    if ((magic.target & ActState::FollowsUp) != 0)
    {
        this->unk267C |= BattleUnit::FollowingUp;
        magic.target = 0;
    }

    if ((magic.target & ActState::Strikes) != 0)
    {
        this->Strike(services, target, 0);
        magic.target = 0;
        this->unk267C |= BattleUnit::FollowingUp;
        this->slotTint = 0;
        battle.proficiencyBonus = 0;
        this->slotActs[0] = ActState{};
    }

    if ((this->unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    ActState& follow = this->slotActs[0];
    follow.action = 0;
    follow.id = this->command.castAct;
    this->AimAction(services, target, follow, static_cast<i16>(magic.anchorX), static_cast<i16>(magic.anchorY));

    if ((follow.target & ActState::Strikes) != 0)
    {
        this->slotTint = 0;
        follow.target = 0;
        this->Strike(services, target, 0);
    }

    if (this->picture != nullptr)
    {
        painter.SetSource(this->picture->data.data());
    }

    if (follow.waiting == 1)
    {
        magic.effectA = 0;
        charge.effectA = 0;

        if (magic.waiting != 1 || this->charging != 0)
        {
            return 0;
        }

        magic = ActState{};
        charge = ActState{};
        this->recoilAct = ActState{};
        follow = ActState{};
        this->slotActs[1] = ActState{};
        this->trails = {};
        this->targets.fill(BattleUnit::NoTarget);
        this->slotTint = 0;
        this->unk2AB0 = 0;
        this->unk267C = 0;
        this->unk26D4 = 0;
        battle.criticalAttack = 0;
        ++battle.attacksSinceCritical;
        this->charging = 0;
        return 1;
    }

    if ((this->unk267C & BattleUnit::Recoiling) == 0 && this->picture != nullptr)
    {
        const DrawArgument argument =
            this->shadowPicture != nullptr ? PaletteArgument(*this->shadowPicture) : DrawArgument{};
        painter.DrawPicture(
            static_cast<i16>(this->aimX),
            static_cast<i16>(this->aimY),
            this->picture->width,
            this->picture->height,
            this->unk26A4,
            argument
        );
    }

    return 0;
}

// The charge part of ChargeAttackFrame (inline in the original): the charge act on the target with trail 1, and its
// follow-up mirrored twice beside it; 0 when an act ends the frame.
int BattleRole::ChargeOnTarget(BattleServices& services, BattleUnit& target)
{
    Painter& painter = services.painter;
    ActState& charge = this->attackAct;
    charge.id = this->actId + BattleRole::MagicActs;
    charge.action = this->unk2A8A + kChargeAction;

    if (services.acts.Run(charge) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(charge.frame, charge.picture);
    services.audio.PlaySound(charge.view, services.config.soundVolume);
    charge.view = 0;
    this->unk26A4 = charge.flags;
    this->actOffsetX = static_cast<u16>(charge.x);
    const u16 chargeAnchorX = charge.anchorX;

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    if (this->facing == 1)
    {
        this->unk26A4 ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(charge.x));
    }

    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    const auto dashX = static_cast<i16>(targetX - this->actOffsetX);
    const auto dashY = static_cast<i16>(targetY - static_cast<u16>(charge.y));
    AfterimageTrail& trail = this->trails[1];

    if (trail.started == 0)
    {
        trail = AfterimageTrail{};
        trail.started = 1;

        for (AfterimageTrail::Image& image : trail.images)
        {
            image.picture = AfterimageTrail::NoPicture;
        }

        trail.tintBlue = kTrailBlueTint;
    }

    trail.direction = AfterimageTrail::InPlace;
    trail.count = kChargeTrailImages;
    trail.fadeStep = 1;
    this->DrawTrail(services, trail, charge.frame, charge.picture, this->unk26A4, dashX, dashY);
    this->picture = services.archive.GetPicture(charge.frame, charge.picture);

    if (this->picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(this->picture->data.data());
    painter.DrawPicture(dashX, dashY, this->picture->width, this->picture->height, this->unk26A4);

    if ((charge.target & ActState::FollowsUp) != 0)
    {
        this->unk267C |= BattleUnit::ChargeFollowing;
        charge.target = 0;
        this->slotActs[1] = ActState{};
    }

    if ((this->unk267C & BattleUnit::ChargeFollowing) == 0)
    {
        return 1;
    }

    ActState& follow = this->slotActs[1];
    follow.action = 0;
    follow.id = this->command.castAct;

    if (charge.effectA != 0)
    {
        follow.id = charge.effectA;
    }

    if (follow.waiting != 0)
    {
        return 1;
    }

    if (services.acts.Run(follow) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(follow.frame, follow.picture);

    if (this->picture == nullptr)
    {
        return 0;
    }

    u16 followAnchorX = follow.anchorX;
    this->unk26A4 = follow.flags;
    this->aimOffsetX = static_cast<u16>(follow.x);

    if (this->facing == 1)
    {
        this->unk26A4 ^= Painter::Mirror;
        this->aimOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(follow.x));
    }

    services.audio.PlaySound(follow.view, services.config.soundVolume);
    services.audio.SetSoundPan(follow.view, this->facing == 1 ? kTargetPan : -kTargetPan);
    follow.view = 0;
    painter.SetSource(this->picture->data.data());
    this->unk26A4 ^= Painter::Mirror;
    this->aimOffsetX = static_cast<u16>(this->picture->width - this->aimOffsetX);

    if (followAnchorX != 0)
    {
        followAnchorX = static_cast<u16>(this->picture->width - follow.anchorX);
    }

    i16 x = 0;
    i16 y = 0;

    if ((follow.anchorX != 0 || follow.anchorY != 0) && (charge.anchorX != 0 || charge.anchorY != 0))
    {
        x = static_cast<i16>(dashX - followAnchorX + chargeAnchorX);
        y = static_cast<i16>(charge.anchorY - follow.anchorY + dashY);
    }

    painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->unk26A4);

    if ((follow.target & ActState::Strikes) != 0)
    {
        this->slotTint = 0;
        follow.target = 0;
        this->Strike(services, target, 0);
    }

    return 1;
}

// swd32.exe 0x00474BA0: one frame of the role's swarm spell (magic act in slot act 8); 1 once it is over. The magic
// act's target bits play the effect in the recoil act; once it strikes or follows up, every third frame (glowLevel
// counting) one more act (the cast act, action unk2F0C; at most 8 queued) flies from a point around the role
// (SwarmOffsetX / Y) to 0x28 above the target (a UnkAction2 command aims at 0, 0), shaking 0x34 frames, speeding, with
// a trail, and sound kSwarmSound plays. The queued acts run each frame; when they are all gone and the magic act waits,
// it ends.
int BattleRole::SwarmSpellFrame(BattleServices& services, BattleUnit& target)
{
    ActState& magic = this->slotActs[BattleUnit::UnkSlot8];
    u16 targetX = 0;
    u16 targetY = 0;
    magic.action = this->unk2A8A;
    this->unk2AAC = 1;
    magic.id = this->actId + BattleRole::MagicActs;
    this->DrawAction(services, target, magic);

    if ((magic.target & ActState::Recoils) != 0)
    {
        if (magic.effectA != 0)
        {
            this->unk267C |= BattleUnit::Recoiling;
            this->recoilAct.id = magic.effectA;
            this->recoilAct.action = magic.effectQ;
        }

        if ((magic.target & ActState::Holds) != 0)
        {
            magic.hold = 1;
        }

        magic.effectA = 0;
        magic.target = static_cast<u16>(magic.target & ~ActState::Recoils);
        magic.effectQ = 0;
    }

    if ((this->unk267C & BattleUnit::Recoiling) != 0 &&
        this->DrawRecoil(services, this->recoilAct, static_cast<i16>(magic.anchorY)) == 1)
    {
        magic.target = 0;
        this->unk267C &= ~BattleUnit::Recoiling;
        magic.hold = 0;
    }

    auto aim = [this, &target, &targetX, &targetY]
    {
        target.GetDrawPosition(targetX, targetY);

        if ((this->command.actionFlags & UnitCommand::UnkAction2) != 0)
        {
            targetX = 0;
            targetY = 0;
        }

        this->ClearMoveAxes(targetX, targetY);
    };

    if ((magic.target & (ActState::Strikes | ActState::FollowsUp)) != 0)
    {
        this->unk267C |= BattleUnit::FollowingUp;
        magic.target = 0;
        aim();
        this->unk2F0C = this->unk2F24;
        ++this->unk2F24;
    }

    if ((this->unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    ++this->glowLevel;

    if (this->glowLevel % kSwarmInterval == 1 && this->queuedActions < kSwarmActs)
    {
        aim();
        const std::size_t place = this->queuedActions & (kSwarmActs - 1);
        this->AddQueuedAction(
            this->command.castAct,
            this->unk2F0C,
            static_cast<i16>(this->placement.x + BattleRole::SwarmOffsetX[place]),
            static_cast<i16>(this->placement.y + BattleRole::SwarmOffsetY[place]),
            static_cast<i16>(targetX),
            static_cast<i16>(targetY - kSwarmAbove),
            1,
            kSwarmShake,
            0
        );
        this->TrailLastQueuedAction(0, 0, kTrailBlueTint);
        services.audio.PlaySound(kSwarmSound, services.config.soundVolume);
    }

    if (this->RunQueuedActions(services, target, this->unk2F0C) != 1)
    {
        return 0;
    }

    this->unk267C = 0;

    if (magic.waiting != 1)
    {
        return 0;
    }

    this->glowLevel = 0;
    this->unk2F0C = 0;
    this->unk2F24 = 0;
    this->slotActs[0] = ActState{};
    magic = ActState{};
    return 1;
}

// swd32.exe 0x004751C0: one frame of the role's boomerang throw (pose kForcedAttackAction in the stand act, its hold
// left as it is, at its place over its shadow); 1 once it is over. Once the pose strikes or follows up, kBoomerangAct
// is queued flying (kBoomerangOut steps a frame, no hit) to the target, raising flag 26B8 of the target when it
// arrives, then back (kBoomerangBack steps after kBoomerangShake frames of shaking), and sound kBoomerangSound plays.
int BattleRole::BoomerangFrame(BattleServices& services, BattleUnit& target)
{
    Painter& painter = services.painter;
    ActState& act = this->standAct;
    act.id = this->actId;
    this->unk2AAC = 1;
    act.action = kForcedAttackAction;

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(this->picture->data.data());
    this->frameFlags = act.flags;
    this->actOffsetX = static_cast<u16>(act.x);
    this->actAnchorX = act.anchorX;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    }

    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const int tint = this->unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int x = this->placement.x - static_cast<i16>(this->actOffsetX);
    services.audio.SetSoundPan(act.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
    this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter
        .DrawPicture(x, this->placement.y - shadowHeight, this->picture->width, this->picture->height, this->unk26A4);
    painter.DrawPicture(x, this->placement.y - act.y, this->picture->width, this->picture->height, this->frameFlags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) == 0)
    {
        return 0;
    }

    ActState& thrown = this->slotActs[0];
    thrown.action = 0;
    thrown.id = kBoomerangAct;
    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);

    if (this->unk267C == kBoomerangGoing)
    {
        if (this->unk2F0C == 0)
        {
            this->AddQueuedAction(
                thrown.id,
                0,
                this->placement.x,
                this->placement.y,
                static_cast<i16>(targetX),
                static_cast<i16>(targetY),
                kBoomerangOut,
                0,
                kNoImpact
            );
            this->TrailLastQueuedAction(0, 0, kTrailBlueTint);
            this->unk2F0C = 1;
        }

        if (this->RunQueuedActions(services, target, 0) == 1)
        {
            this->unk2F0C = 0;
            this->unk267C = kBoomerangReturning;
            target.RaiseFlag26B8();
        }
    }

    if (this->unk267C == kBoomerangReturning)
    {
        if (this->unk2F0C == 0)
        {
            this->AddQueuedAction(
                thrown.id,
                0,
                static_cast<i16>(targetX),
                static_cast<i16>(targetY),
                this->placement.x,
                this->placement.y,
                kBoomerangBack,
                kBoomerangShake,
                kNoImpact
            );
            this->TrailLastQueuedAction(0, 0, kTrailBlueTint);
            this->unk2F0C = 1;
        }

        if (this->RunQueuedActions(services, target, 0) == 1)
        {
            this->unk2F0C = 0;
            this->unk267C = kBoomerangBackHome;
            services.audio.PlaySound(kBoomerangSound, services.config.soundVolume);
        }
    }

    if (this->unk267C != kBoomerangBackHome)
    {
        return 0;
    }

    this->unk267C = 0;
    thrown = ActState{};
    act = ActState{};
    return 1;
}

// swd32.exe 0x00474E60: one frame of the role's leap spell (magic act in slot act 8, stages in unk267C); 1 once the
// battle marks the spell over after stage 3 (the act cleared). Stage 0 draws the magic act; stage 1 the next action on
// the target; stage 2 the action after that walking home (UnkFlag8 set), stage 3 once home. A waiting act moves on a
// stage; a follow-up marks the spell cast.
int BattleRole::LeapSpellFrame(BattleServices& services, BattleUnit& target)
{
    BattleState& battle = services.state;
    ActState& magic = this->slotActs[BattleUnit::UnkSlot8];
    magic.id = this->actId + BattleRole::MagicActs;
    this->unk2AAC = 1;
    magic.action = this->unk2A8A;

    if (this->unk267C == kLeapCasting)
    {
        this->DrawAction(services, target, magic);
    }

    if (this->unk267C == kLeapAtTarget)
    {
        u16 targetX = 0;
        u16 targetY = 0;
        ++magic.action;
        target.GetDrawPosition(targetX, targetY);
        this->DrawActionAt(services, static_cast<i16>(targetX), static_cast<i16>(targetY), magic);
    }

    if (this->unk267C == kLeapReturning)
    {
        this->command.flags = static_cast<u16>(this->command.flags | UnitCommand::UnkFlag8);
        magic.action += kLeapReturnAction;

        if (this->ReturnHome(services, target, magic.id, magic.action) == 1)
        {
            this->unk267C = kLeapHome;
        }
    }

    if (magic.waiting == 1)
    {
        magic = ActState{};
        ++this->unk267C;
    }

    if ((magic.target & ActState::FollowsUp) != 0)
    {
        if ((battle.spellStage & BattleState::SpellCast) == 0)
        {
            battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellCast);
        }

        magic.target = 0;
    }

    if ((battle.spellStage & BattleState::SpellOver) == 0 ||
        static_cast<i32>(this->unk267C) < static_cast<i32>(kLeapHome))
    {
        return 0;
    }

    this->unk267C = 0;
    magic = ActState{};
    return 1;
}

// swd32.exe 0x00470E20: the selected item used up: 0 when none is selected or it is not in the list, otherwise its id
// (usedItem; 0 for an item with kNoUseAct). A multi-use item (kMultiUse, uses in the low byte) loses a use and is spent
// only with its last; a spent item that is not kept (kKeptKind without kUnkKind8000000) loses a count (else a pending
// count) and unk06, and the selection is cleared; an item left with no count is freed from the list. The original
// then reads the flag of the item before it (of the list head for the first item, past it).
u16 BattleRole::UseSelectedItem()
{
    int place = 0;
    this->ShowAllItems();
    ItemNode* const selected = this->selectedItem;

    if (selected == nullptr)
    {
        return 0;
    }

    while (true)
    {
        ++place;
        ItemNode* node = this->inventory->get();

        // The original leaves the shown list at no item when the selected one is not in it.
        if (node == nullptr)
        {
            this->inventory = nullptr;
            this->inventoryNode = nullptr;
            return 0;
        }

        this->inventory = &node->next;
        this->inventoryNode = node;

        if (node->id == selected->id)
        {
            break;
        }
    }

    this->inventory = &selected->next;
    this->inventoryNode = selected;
    this->usedItem = selected->id;
    ObjectData& data = selected->record.data;
    const u16 uses = data.unk3C;
    bool spent = true;

    if ((uses & kMultiUse) != 0 && (uses & kUsesMask) != 0)
    {
        data.unk3C = static_cast<u16>((uses & ~kUsesMask) | ((uses & kUsesMask) - 1));
        spent = (uses & kUsesMask) == 1;
    }

    if (spent && ((data.kind & kKeptKind) == 0 || (data.kind & kUnkKind8000000) != 0))
    {
        if (selected->count != 0)
        {
            --selected->count;
        }
        else if (selected->pending != 0)
        {
            --selected->pending;
        }

        if (selected->unk06 != 0)
        {
            --selected->unk06;
        }

        this->selectedItem = nullptr;
    }

    ItemNode* flagged = selected;

    if (selected->count == 0 && selected->pending == 0)
    {
        std::unique_ptr<ItemNode> rest = std::move(selected->next);
        this->ShowAllItems();
        ItemLink* before = this->inventory;

        for (int step = place - 1; step > 0; --step)
        {
            before = &(*before)->next;
        }

        // `before` holds the freed item; the item before it is the one whose flag is read next.
        *before = std::move(rest);
        flagged = nullptr;

        if (place - 1 > 0)
        {
            ItemLink* previous = this->inventory;

            for (int step = place - 2; step > 0; --step)
            {
                previous = &(*previous)->next;
            }

            flagged = previous->get();
        }

        this->inventory = before;
        this->inventoryNode = flagged;
    }

    // The list head read as an item has no flag here.
    if (flagged != nullptr && (flagged->record.data.unk40 & kNoUseAct) != 0)
    {
        return 0;
    }

    return this->usedItem;
}

// swd32.exe 0x004750C0: the role's chosen command dropped: the command, the start of the skill name, the stamina
// owed, the item and the heals cleared; a selected item gives back its reservation (unk06 of the item at the list
// cursor, which the original reads in the list head when the cursor is there)
void BattleRole::CancelCommand()
{
    this->unk2A86 = 0;
    this->command = UnitCommand{};
    std::fill_n(this->skillName.begin(), kCancelledNameBytes, '\0');
    this->unk2F10 = 0;
    this->unk2F12 = 0;
    this->unk2A8A = 0;
    this->unk2F16 = 0;
    this->unk2F18 = 0;
    this->unk29A6 = 0;
    this->unk29A8 = 0;
    this->unk29AA = 0;
    this->unk29A4 = 0;
    this->usedItem = 0;

    if (this->selectedItem == nullptr)
    {
        return;
    }

    if (this->inventoryNode != nullptr && this->inventoryNode->unk06 != 0)
    {
        --this->inventoryNode->unk06;
    }

    this->selectedItem = nullptr;
}

// swd32.exe 0x00482DA0: the fury gauge (roleBlock[0], full at kFuryFull) raised by `damage` taken: -1 fills it, 0
// empties it; otherwise, below full, by damage / maximum hit points * 47 (a whole hit point or more fills it), the
// damage carried while that is 0 (with the original's double compare it never is). Full plays kFurySound. Not for a
// forced unit.
void BattleRole::AddFury(BattleServices& services, int damage)
{
    if (this->unk2B00 == 1 || this->unk2B04 == 1)
    {
        return;
    }

    int carried = damage;
    i32& fury = this->roleBlock[0];

    if (damage == kFillFury)
    {
        fury = kFuryFull;
    }
    else if (damage == 0)
    {
        fury = 0;
    }

    const i32 before = fury;

    if (before >= kFuryFull || damage <= 0)
    {
        return;
    }

    const int maximum = this->roleRecord->maxHitPoints;

    if (damage < maximum)
    {
        if (this->damageCarry > 0)
        {
            carried = this->damageCarry + damage;
            damage += this->damageCarry;
        }

        const double share = static_cast<double>(damage) / static_cast<double>(maximum) * kFuryScale;
        fury = before + crt::Ftol(share);

        if (share == 0.0)
        {
            this->damageCarry += carried;
        }
        else
        {
            this->damageCarry = 0;
        }
    }
    else
    {
        fury = kFuryFull;
    }

    if (fury >= kFuryFull)
    {
        fury = kFuryFull;
        services.audio.PlaySound(kFurySound, services.config.soundVolume);
    }
}

// swd32.exe 0x0046EE60: the turn's regeneration of a role still up: its armour bonuses (percent of its maxima, at
// least 1) heal its hit points, spirit and stamina, each shown 6 frames after the one before
void BattleRole::Regenerate(BattleServices& services)
{
    const RoleRecord& record = *this->roleRecord;

    if ((record.unk24 & RoleRecord::Defeated) != 0)
    {
        return;
    }

    u16 delay = 0;

    auto share = [](u16 bonus, i16 maximum)
    {
        const auto value = static_cast<u16>(bonus * maximum / kPercent);
        return value == 0 ? u16{1} : value;
    };

    if (this->bonusA != 0)
    {
        this->unk29A6 = static_cast<u16>(-static_cast<i16>(share(this->bonusA, record.maxHitPoints)));
        this->Damage(services, static_cast<i16>(this->unk29A6), 0, 0);
        this->PushTarget(BattleUnit::HealTarget);
        this->SetDamageDigits(services, static_cast<i16>(this->unk29A6));
        this->PushTargetAlt(0);
        this->StepTargetCount(1);
        delay = kRegenerateDelay;
        this->unk29A6 = 0;
    }

    if (this->bonusB != 0)
    {
        this->unk29A8 = share(this->bonusB, record.maxSpirit);
        this->Damage(services, 0, static_cast<i16>(this->unk29A8), 0);
        this->PushTarget(BattleUnit::SpiritGainTarget);
        this->SetDamageDigits(services, static_cast<i16>(this->unk29A8));
        this->PushTargetAlt(delay);
        this->StepTargetCount(1);
        delay = static_cast<u16>(delay + kRegenerateDelay);
        this->unk29A8 = 0;
    }

    if (this->bonusC != 0)
    {
        this->unk29AA = share(this->bonusC, record.maxStamina);
        this->Damage(services, 0, 0, static_cast<i16>(this->unk29AA));
        this->PushTarget(BattleUnit::StaminaGainTarget);
        this->SetDamageDigits(services, static_cast<i16>(this->unk29AA));
        this->PushTargetAlt(delay);
        this->StepTargetCount(1);
        this->unk29AA = 0;
    }
}

// swd32.exe 0x0046F1F0: the command of an accessory of `kind` (the stamina it costs taken first when none is owed):
// the kind sets its unk26CC bit and becomes command A (some take command B with UnkCommandFlag400, command A 0) and
// most read the accessory's proficiency into unk26DC; Unk1F scales unk29A4, Unk22 / Unk23 / Unk24 a heal of the
// hit points / spirit / stamina of the battle record. The proficiencies then step by kAccessoryStep.
void BattleRole::UseAccessorySkill(BattleServices& services, u16 kind)
{
    CounterList& proficiency = services.party.fame.lists[Fame::ProficiencyList];

    if (this->unk2F12 == 0)
    {
        this->unk2F12 = this->AccessoryValue(kind);
    }

    const RoleRecord& record = *this->battleRecord;

    auto share = [](i16 base, u16 percent)
    {
        return static_cast<u16>(base * percent / kPercent + base / kAccessoryBaseShare);
    };

    switch (static_cast<AccessoryKind>(kind))
    {
        case AccessoryKind::RepeatAttack:
            this->unk26CC |= kRepeatAttack;
            this->commandA = 1;
            this->AccessoryProficiency(kind, proficiency);
            this->StepAccessoryCounters(proficiency, kAccessoryStep);
            this->extraAttacks = 1;
            return;

        case AccessoryKind::Unk16:
            this->commandA = 0;
            this->unk26CC |= kAccessoryBit16;
            this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag400);
            this->commandB = kind;
            break;

        case AccessoryKind::Steal:
            this->commandA = kind;
            this->unk26CC |= kAccessoryBit17;
            break;

        case AccessoryKind::Unk18:
            this->unk26CC |= kAccessoryBit18;
            this->commandA = kind;
            this->AccessoryProficiency(kind, proficiency);
            break;

        case AccessoryKind::Unk19:
            this->commandA = 0;
            this->unk26CC |= kAccessoryBit19;
            this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag400);
            this->commandB = kind;
            break;

        case AccessoryKind::Unk1A:
            this->commandA = kind;
            this->unk26CC |= kAccessoryBit1A;
            break;

        case AccessoryKind::Unk1B:
            this->commandA = kind;
            this->unk26CC |= kAccessoryBit1B;
            break;

        case AccessoryKind::Unk1C:
            this->commandA = kind;
            this->AccessoryProficiency(kind, proficiency);
            break;

        case AccessoryKind::Unk1D:
            this->commandA = kind;
            this->unk26CC |= kAccessoryBit1D;
            break;

        case AccessoryKind::Unk1E:
            this->commandA = kind;
            this->unk26CC |= kAccessoryBit1E;
            break;

        case AccessoryKind::Unk1F:
            this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag400);
            this->commandA = 0;
            this->commandB = kind;
            this->AccessoryProficiency(kind, proficiency);
            this->unk29A4 = static_cast<u16>(kUnk1FScale * this->unk26DC / kPercent + 1);
            break;

        case AccessoryKind::Unk20:
        case AccessoryKind::Unk21:
            this->commandA = kind;
            this->AccessoryProficiency(kind, proficiency);
            break;

        case AccessoryKind::Unk22:
            this->AccessoryProficiency(kind, proficiency);
            this->unk26DC = static_cast<u16>((this->unk26DC >> 1) - (this->unk26DC >> 1) / kHalfShare);
            this->unk29A6 = share(record.maxHitPoints, this->unk26DC);
            break;

        case AccessoryKind::Unk23:
            this->AccessoryProficiency(kind, proficiency);
            this->unk26DC = static_cast<u16>(this->unk26DC / kSpiritShare);
            this->unk29A8 = share(record.maxSpirit, this->unk26DC);
            break;

        case AccessoryKind::Unk24:
            this->AccessoryProficiency(kind, proficiency);
            this->unk26DC = static_cast<u16>((this->unk26DC >> 1) - (this->unk26DC >> 1) / kHalfShare);
            this->unk29AA = share(record.stamina, this->unk26DC);
            break;

        default:
            return;
    }

    this->StepAccessoryCounters(proficiency, kAccessoryStep);
}

// swd32.exe 0x0046FF00: whether the role's next attack is critical (it sets `critical`): never for a forced unit, with
// an accessory command or for the role that had the last one; always with the Unk32 equipment; after roleCount + 8
// attacks since the last one; after 8, on a 0..9 roll above 5. A rolled one restarts the count for this role.
void BattleRole::RollCritical(BattleServices& services)
{
    BattleState& battle = services.state;

    if (this->unk2B00 == 1 || this->unk2B04 == 1 || this->unk26CC != 0 ||
        battle.lastCriticalRole == this->placement.actId)
    {
        return;
    }

    if ((this->equipmentFlags & BattleUnit::Unk32) != 0)
    {
        this->critical = 1;
        return;
    }

    const u32 since = battle.attacksSinceCritical;

    if (since < battle.roleCount + kCriticalAttacks &&
        (since < kCriticalAttacks || static_cast<u16>(services.random.Below(kCriticalRoll)) <= kCriticalChance))
    {
        return;
    }

    this->critical = 1;
    battle.attacksSinceCritical = 0;
    battle.lastCriticalRole = this->placement.actId;
}

// swd32.exe 0x0046FFF0: the role's chosen command prepared. A UnkCommandFlag200 command takes command unk2F18 (a
// walking one, UnkFlag8 / UnkFlag20, goes to command B with command A kWalkCommand). Otherwise the skill name is
// cleared, the attack set from the battle record (not for Unk1B), the accessory command applied, the critical rolled,
// the command loaded from skill 0 (a LifeDrain equipment drains life) and then from the weapon's skill: a walking
// attack (command 1, Steal or Unk1A, not forced) goes to command B with command A kWalkCommand.
void BattleRole::PrepareCommand(BattleServices& services, Database& database)
{
    if (this->commandA == 0)
    {
        return;
    }

    auto walk = [this]
    {
        this->commandB = this->commandA;
        this->commandA = kWalkCommand;
    };

    auto walksForward = [this]
    {
        const u16 kind = this->commandA;
        const bool attack = kind == 1 || kind == static_cast<u16>(AccessoryKind::Steal) ||
                            kind == static_cast<u16>(AccessoryKind::Unk1A);
        return attack && this->unk2B00 == 0 && this->unk2B04 == 0;
    };

    const u16 walking = UnitCommand::UnkFlag8 | UnitCommand::UnkFlag20;

    if ((this->unk2A86 & BattleUnit::UnkCommandFlag200) != 0)
    {
        this->unk2B0C = 1;

        if (this->unk2F18 != 0)
        {
            this->commandA = this->unk2F18;
        }

        if ((this->command.flags & walking) != 0)
        {
            walk();
        }

        return;
    }

    std::fill_n(this->skillName.begin(), kCancelledNameBytes, '\0');
    const u16 kind = this->commandA;

    if (kind != static_cast<u16>(AccessoryKind::Unk1B))
    {
        this->unk29A4 = this->battleRecord->attackBonus;
    }

    this->UseAccessorySkill(services, kind);
    this->RollCritical(services);
    this->command = UnitCommand{};
    this->LoadCommand(database, 0);

    if ((this->equipmentFlags & BattleUnit::LifeDrain) != 0)
    {
        this->command.actionFlags |= UnitCommand::DrainsLife;
    }

    const u16 skill = this->weapon.data.unk3E;

    if (skill != 0)
    {
        this->LoadCommand(database, skill);

        if ((this->command.flags & walking) == 0 && this->commandA != static_cast<u16>(AccessoryKind::Steal))
        {
            return;
        }

        if (walksForward())
        {
            walk();
        }

        return;
    }

    if ((this->command.flags & walking) != 0 && walksForward())
    {
        walk();
    }
}

// swd32.exe 0x00470820: the cost of the role's command paid: with UnkCommandFlag8000 the selected item is used up, or
// the spirit owed (unk2F10) taken (not below 0); then the stamina owed
void BattleRole::FinishCommand()
{
    if ((this->unk2A86 & BattleUnit::UnkCommandFlag8000) != 0)
    {
        if (this->selectedItem != nullptr)
        {
            this->UseSelectedItem();
            this->selectedItem = nullptr;
        }
        else
        {
            RoleRecord& record = *this->roleRecord;
            record.spirit = static_cast<i16>(record.spirit - this->unk2F10);

            if (record.spirit < 0)
            {
                record.spirit = 0;
            }
        }

        this->unk2F10 = 0;
    }

    this->PayStamina();
}

// swd32.exe 0x004714B0: whether the role wins a roll against `level` (0 never): above the role's level 35 in 100;
// 1..7 levels below it 70 in 100, 8..12 below 90 in 100, 13 or more always; the same level never
int BattleRole::BeatsLevel(BattleServices& services, u16 level) const
{
    if (level == 0)
    {
        return 0;
    }

    const int own = this->battleRecord->level;

    if (own < level)
    {
        return services.random.Below(kLevelRoll) <= kAboveChance ? 1 : 0;
    }

    const int below = own - level;

    if (below >= kNearBelow && below < kSureBelow)
    {
        return services.random.Below(kLevelRoll) <= kNearChance ? 1 : 0;
    }

    if (below >= 1 && below < kNearBelow)
    {
        return services.random.Below(kLevelRoll) <= kCloseChance ? 1 : 0;
    }

    return below >= kSureBelow ? 1 : 0;
}

// swd32.exe 0x00471540: one frame of the role stepping back (mode 1, 0x10 a frame, with kStepBackSound) or standing
// back (mode 0) in the recoil act's walk action, mirrored against its facing, fadeLevel counting down from 0xF to 6
// (mode 1) or 2; 1 once there (the recoil act cleared), or for a defeated role. A held or holding role only counts.
int BattleRole::StepBack(BattleServices& services, int mode)
{
    if (this->defeated == 1)
    {
        this->unk2AAC = 0;
        return 1;
    }

    this->glowLevel = mode == 0 ? kStandBackEnd : kStepBackEnd;

    if (this->IsHeldOrHolding() == 1)
    {
        if (this->fadeLevel <= this->glowLevel)
        {
            this->fadeLevel = BattleUnit::Opaque;
            return 1;
        }

        --this->fadeLevel;
        return 0;
    }

    ActState& act = this->recoilAct;

    if (this->fadeLevel <= this->glowLevel)
    {
        act = ActState{};
        this->fadeLevel = BattleUnit::Opaque;

        if (mode == 1)
        {
            this->unk2AAC = 1;
        }

        return 1;
    }

    this->unk2AAC = 1;
    act.id = this->actId;
    act.action = kStepBackAction;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 1;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    this->actOffsetX = static_cast<u16>(act.x);
    this->frameFlags = act.flags ^ Painter::Mirror;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
    }

    if (this->fadeLevel == BattleUnit::Opaque && mode == 1)
    {
        act.view = kStepBackSound;
        services.audio.PlaySound(kStepBackSound, services.config.soundVolume);
        services.audio.SetSoundPan(act.view, this->facing == 1 ? -kTargetPan : kTargetPan);
        act.view = 0;
    }

    u16 x = 0;
    u16 y = 0;
    this->GetPosition(x, y);

    if (mode == 1)
    {
        x = static_cast<u16>(this->facing == 1 ? x - kStepBackShift : x + kStepBackShift);
    }

    this->SetPosition(static_cast<i16>(x), static_cast<i16>(y));
    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    painter.DrawPicture(
        this->placement.x - static_cast<i16>(this->actOffsetX),
        this->placement.y - act.y,
        this->picture->width,
        this->picture->height,
        this->frameFlags,
        PaletteArgument(*this->picture)
    );
    --this->fadeLevel;
    return 0;
}

// swd32.exe 0x00472C70: with a SpiritDrain accessory worn (its proficiency stepped by kDrainStep), `amount` becomes
// the share drained: half the proficiency in percent of it, plus 1; 0 without one.
int BattleRole::DrainShare(BattleServices& services, i32& amount)
{
    if ((this->equipmentFlags & BattleUnit::SpiritDrain) == 0)
    {
        return 0;
    }

    CounterList& proficiency = services.party.fame.lists[Fame::ProficiencyList];
    this->AccessoryProficiency(static_cast<u16>(AccessoryKind::SpiritDrain), proficiency);
    this->StepAccessoryCounters(proficiency, kDrainStep);
    this->unk26DC = static_cast<u16>(this->unk26DC >> 1);
    amount = amount * this->unk26DC / kPercent + 1;
    return 1;
}

// swd32.exe 0x00475160: the proficiency in percent of the Unk1E accessory (硃砂神筆), kept in unk26DC
u16 BattleRole::Unk1EProficiency(BattleServices& services)
{
    this->AccessoryProficiency(
        static_cast<u16>(AccessoryKind::Unk1E),
        services.party.fame.lists[Fame::ProficiencyList]
    );
    return this->unk26DC;
}

// swd32.exe 0x00471080: `count` set to the objects (count and pending) in the inventory with kind bit kCountedKind,
// not kThrownObject, usable by roles bit 0 or 2 (usableBy low byte & 5); the walk leaves the shown list at its end
void BattleRole::CountUsableObjects(u16& count)
{
    this->ShowAllItems();
    count = 0;

    while (ItemNode* const node = this->StepItem())
    {
        const ObjectData& data = node->record.data;

        if ((data.kind & kCountedKind) != 0 && node->id != kThrownObject &&
            (LowByte(data.usableBy) & kCountedUsers) != 0)
        {
            count = static_cast<u16>(count + node->count + node->pending);
        }
    }
}

// swd32.exe 0x00470F70: the `index`th object of the inventory with unk40 bit kListedObject (the first one at all for
// 0): its name and its count with the pending ones; 0 past the end
int BattleRole::ListedObject(int index, std::string& name, u16& count)
{
    int listed = 0;
    ItemNode* node = nullptr;
    this->ShowAllItems();

    do
    {
        node = this->StepItem();

        if (node == nullptr)
        {
            return 0;
        }

        if ((node->record.data.unk40 & kListedObject) != 0)
        {
            ++listed;
        }
    } while (listed != index);

    name = ObjectName(node->record);
    count = static_cast<u16>(node->count + node->pending);
    return 1;
}

// The step of the shown item list to its next item (inline in the original, which keeps the node at +0x2EC8): at
// the end both are cleared.
ItemNode* BattleRole::StepItem()
{
    ItemNode* const node = this->inventory != nullptr ? this->inventory->get() : nullptr;

    if (node == nullptr)
    {
        this->inventory = nullptr;
        this->inventoryNode = nullptr;
        return nullptr;
    }

    this->inventory = &node->next;
    this->inventoryNode = node;
    return node;
}

// The kind bits of the item and skill categories of the battle lists (inline in the original: 0 kCategory0, 1
// kCategory1, 2 kCategory2, 3 kCategory3, 4 kCategory4, 5 kCategory5; another category is taken as the bits);
// `categories` limits the switch to its first entries as each function has it.
u32 BattleRole::CategoryKinds(int category, int categories)
{
    static constexpr std::array<u32, 6> kinds = {
        kCategory0,
        kCategory1,
        kCategory2,
        kCategory3,
        kCategory4,
        kCategory5,
    };

    if (category >= 0 && category < categories)
    {
        return kinds[static_cast<std::size_t>(category)];
    }

    return static_cast<u32>(category);
}

// The step of the shown skill list to its next skill (inline in the original); nullptr at the end.
ItemNode* BattleRole::StepSkill()
{
    this->skills = this->skills != nullptr ? this->skills->next.get() : nullptr;
    return this->skills;
}

// swd32.exe 0x004702E0: `count` raised by the skills of `category` usable by roles bit 0 or 2: for `type` 0 those
// with unk52 kFirstSkillType..kLastSkillType, otherwise those with unk52 kSkillType1 (type 1 is that kind; others
// are taken as themselves, but only kSkillType0 is told apart)
void BattleRole::CountSkills(int category, int type, u8& count)
{
    const u32 kinds = BattleRole::CategoryKinds(category, kSkillCategories);
    int wanted = type;

    if (type == 0)
    {
        wanted = kSkillType0;
    }
    else if (type == 1)
    {
        wanted = kSkillType1;
    }

    this->ShowAllSkills();

    while (ItemNode* const node = this->StepSkill())
    {
        const ObjectData& data = node->record.data;

        if ((LowByte(data.usableBy) & kCountedUsers) == 0 || (kinds & data.kind) == 0)
        {
            continue;
        }

        if (wanted == kSkillType0)
        {
            if (data.unk52 >= kFirstSkillType && data.unk52 <= kLastSkillType)
            {
                ++count;
            }
        }
        else if (data.unk52 == kSkillType1)
        {
            ++count;
        }
    }
}

// The objects of a node left to use: its count and pending ones less those already taken (unk06).
int BattleRole::ObjectsLeft(const ItemNode& node)
{
    return node.count + node.pending - static_cast<u16>(node.unk06);
}

// swd32.exe 0x00470A10: `count` raised by the objects of `category` usable by roles bit 0 or 2 with some left, and
// for category 4 by every object with unk40 bit kListedObject as well
void BattleRole::CountObjects(int category, u16& count)
{
    const u32 kinds = BattleRole::CategoryKinds(category, kObjectCategories);
    this->ShowAllItems();

    while (ItemNode* const node = this->StepItem())
    {
        const ObjectData& data = node->record.data;

        if ((kinds & data.kind) != 0 && (LowByte(data.usableBy) & kCountedUsers) != 0 && ObjectsLeft(*node) > 0)
        {
            ++count;
        }

        if (kinds == kCategory4 && (data.unk40 & kListedObject) != 0)
        {
            ++count;
        }
    }
}

// swd32.exe 0x00470910: the `index`th object of `category` (0..3) usable by roles bit 0 or 2 with some left: its
// name, the count left, and `flags` kShownObject, with kCostlyObject when its unk40 cost (with kCostsStamina) is
// above the role's stamina; 0 (flags 0) past the end. Index 0 takes the first object at all, shown only when it
// qualifies.
int BattleRole::ListObject(int category, int index, std::string& name, u16& flags, u16& count)
{
    int listed = 0;
    ItemNode* node = nullptr;
    this->ShowAllItems();
    flags = 0;
    const u32 kinds = BattleRole::CategoryKinds(category, kListCategories);

    do
    {
        node = this->StepItem();

        if (node == nullptr)
        {
            flags = 0;
            return 0;
        }

        const ObjectData& data = node->record.data;

        if ((kinds & data.kind) != 0 && (LowByte(data.usableBy) & kCountedUsers) != 0 && ObjectsLeft(*node) > 0)
        {
            ++listed;
        }
    } while (listed != index);

    const ObjectData& data = node->record.data;

    if ((kinds & data.kind) != 0 && (LowByte(data.usableBy) & kCountedUsers) != 0)
    {
        name = ObjectName(node->record);
        count = static_cast<u16>(ObjectsLeft(*node));
        flags = static_cast<u16>(flags | kShownObject);

        if ((data.unk40 & kCostsStamina) != 0 && (data.unk40 & kCostMask) > this->roleRecord->stamina)
        {
            flags = static_cast<u16>(flags | kCostlyObject);
        }
    }

    return 1;
}

// swd32.exe 0x00470FE0: with `throwable`, the `index`th object of kind bit kCountedKind that is not kThrownObject
// (it becomes the selected item); without, the first kThrownObject (the index ignored, the selection kept). Its name
// and count (with the pending ones) when usable by roles bit 0 or 2; 0 past the end. Index 0 takes the first object.
int BattleRole::ThrowableObject(int throwable, int index, std::string& name, u16& count)
{
    int listed = 0;
    ItemNode* node = nullptr;
    bool selected = true;
    this->ShowAllItems();

    do
    {
        node = this->StepItem();

        if (node == nullptr)
        {
            return 0;
        }

        if (throwable != 0)
        {
            if ((node->record.data.kind & kCountedKind) != 0 && node->id != kThrownObject)
            {
                ++listed;
            }
        }
        else if (node->id == kThrownObject)
        {
            selected = false;
            break;
        }
    } while (listed != index);

    if (selected)
    {
        this->selectedItem = node;
    }

    if ((LowByte(node->record.data.usableBy) & kCountedUsers) != 0)
    {
        name = ObjectName(node->record);
        count = static_cast<u16>(node->count + node->pending);
    }

    return 1;
}
}  // namespace swd3
