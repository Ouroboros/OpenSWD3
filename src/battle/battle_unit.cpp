#include "battle/battle_unit.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <string>

#include "audio/audio_manager.h"
#include "battle/battle_enemy.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_role.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "config/config.h"
#include "core/format.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "org/database.h"
#include "org/fame.h"
#include "party/party.h"
#include "pic/picture.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "random/random.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"

namespace swd3
{

namespace
{

constexpr double kGaugePixels = 62.0;
constexpr u16 kShadowAction = 0x24;
constexpr u16 kDefeatedShadowAction = 0x33;
constexpr u32 kRaiseBlocked = 0x2000000;
constexpr u32 kFlag26B8 = 0x80000000u;
constexpr int kSlotActsPerTurn = 8;

constexpr int kGaugeBase = 0x12C;  // a random gauge: kGaugeBase + kGaugeSpread / (1..kGaugeDivisors)
constexpr int kGaugeSpread = 0x96;
constexpr u32 kGaugeDivisors = 9;
constexpr std::size_t kClearedNameBytes = 0x10;  // ResetTurn clears only the start of the skill name
constexpr int kRoleGaugeBase = 0x32;             // a party role starts a turn at kRoleGaugeBase + 0..kRoleGaugeSpread-1
constexpr u32 kRoleGaugeSpread = 0x8C;
constexpr int kFadeOutSound = 0x2F;
constexpr int kFadeInSound = 0x30;
constexpr int kFadePan = 0x10;
constexpr i32 kCommand1Action = 0x28;   // the fade in action after command 1
constexpr i32 kPoseAction = 0x30;       // the fade in action of a posed unit
constexpr i32 kPose2Action = 0x31;      // of pose 2
constexpr i16 kSpeedMask = 0x7FFF;      // the speed of a command with bit 15 set
constexpr int kGuardTextOffset = 0x14;  // the guard text starts left of and above the unit
constexpr i16 kGuardTextRise = 0x18;    // pixels the guard text rises before it is gone
constexpr int kGuardTextHeight = 0x1A;
constexpr i32 kUnkBlock38 = 0x38;
constexpr u32 kCommandStatusBit = 0x10000000;  // command +0x08 bit 28
constexpr u16 kAccessoryValueMask = 0x3FFF;
constexpr u32 kHealRoll = 0xA;         // ChooseHeal: a roll of 0..4 picks the first way, 5..9 the second
constexpr u8 kDefeatPoses = 10;        // a defeated unit takes a pose of 1..kDefeatPoses
constexpr u8 kFallbackDefeatPose = 6;  // or this one when none was taken
constexpr u16 kTurnFlashFall = 8;      // the tint pulse falls from here after ResetTurn
constexpr int kResetClearsGuard = 8;   // ResetTurn mode bit
constexpr u16 kPose2 = 2;
constexpr u32 kEvadeRoll = 10;       // evade chances are in tenths
constexpr u16 kInvisibleEvades = 9;  // an invisible unit evades 9 times in kEvadeRoll
constexpr u32 kLikelyEvadeOdds = 3;
constexpr u8 kLowEvadeSkill = 6;  // up to this evade skill a hit also needs a roll of kLowEvadeGate or more
constexpr u16 kLowEvadeGate = 8;
constexpr int kTenths = 10;
constexpr int kThousands = 1000;  // the places of the four damage digits
constexpr int kHundreds = 100;
constexpr int kTens = 10;
constexpr std::size_t kThousandsDigit = 0;
constexpr std::size_t kHundredsDigit = 1;
constexpr std::size_t kTensDigit = 2;
constexpr std::size_t kOnesDigit = 3;
constexpr int kStatScalePercent = 10;  // StatsScaled moves the stats of a party role by this percent
constexpr i16 kGuardPhaseEnd = 9;      // the last phase of a guard text letter
constexpr i16 kGuardTextStep = 4;      // pixels the guard text rises a frame
constexpr int kUnkStatus1000Tint = -12;
constexpr int kAgilityShift = 2;  // the gauge step is agility / 4, agility / 8 when slowed
constexpr int kSlowedAgilityShift = 3;
constexpr int kBoostDivisor = 4;         // a boost adds a quarter of the step plus one
constexpr u16 kHealBelowShare = 3;       // an enemy heals below a third of its hit points
constexpr u8 kFirstHeal = 1;             // healChoice: ObjectData::healCountA
constexpr u8 kSecondHeal = 2;            // ObjectData::healCountB
constexpr u8 kSureEvadeStreak = 0x18;    // evades in a row from which the next hit lands
constexpr u8 kLikelyEvadeStreak = 0x14;  // from which it lands one time in three
constexpr int kPercent = 100;
constexpr int kSpeedChangePercent = 0x1E;  // the gauge step a speed buff adds or takes
constexpr int kInvisibleSlowPercent = 0xA;
constexpr int kChanceScale = 0x46;  // RollChance: kChanceScale percent of the chance plus kChanceBase
constexpr int kChanceBase = 0xA;
}  // namespace

// swd32.exe 0x00478250
BattleUnit::BattleUnit() = default;

// swd32.exe 0x00478300: frees the description of the unit's own object (+0xB0)
BattleUnit::~BattleUnit() = default;

// swd32.exe 0x00478330
int BattleUnit::SetTarget(int value)
{
    this->target = value;
    return value;
}

// swd32.exe 0x00478340: the action gauge scaled to 62 pixels of the full gauge
int BattleUnit::GaugeLength(const BattleServices& services) const
{
    return static_cast<i32>(
        crt::Ftol(static_cast<double>(this->gauge) / static_cast<double>(services.state.gaugeFull) * kGaugePixels)
    );
}

// swd32.exe 0x00478370
i16 BattleUnit::ResetGauge(const BattleServices& services)
{
    this->gauge = static_cast<u16>(services.state.gaugeFull);
    return static_cast<i16>(services.state.gaugeFull);
}

// swd32.exe 0x00478380
int BattleUnit::RandomizeGauge(BattleServices& services)
{
    const int value = kGaugeSpread / static_cast<int>(services.random.Below(kGaugeDivisors) + 1) + kGaugeBase;
    this->gauge = static_cast<u16>(value);
    return value;
}

// swd32.exe 0x004783B0: the saved placement while drawAtSaved is set
void BattleUnit::GetDrawPosition(u16& x, u16& y) const
{
    const UnitPlacement& at = this->drawAtSaved != 0 ? this->savedPlacement : this->placement;
    x = static_cast<u16>(at.x);
    y = static_cast<u16>(at.y);
}

// swd32.exe 0x00478400: the anchor of the main action (of the slot act 8 with flag 2 of +0x2A87); a mirrored unit
// measures a non-zero anchor x from the right edge of its shadow picture
void BattleUnit::GetOffset(u16& x, u16& y) const
{
    x = this->mainAct.anchorX;
    y = this->mainAct.anchorY;

    if ((this->unk2A86 & BattleUnit::UnkCommandFlag200) != 0)
    {
        x = this->slotActs[BattleUnit::UnkSlot8].anchorX;
        y = this->slotActs[BattleUnit::UnkSlot8].anchorY;
    }

    if (this->facing == 1)
    {
        const u16 offset = this->mainAct.anchorX;

        if (offset != 0 && this->shadowPicture != nullptr)
        {
            x = static_cast<u16>(this->shadowPicture->width - offset);
        }
    }
}

// swd32.exe 0x00478470
i16 BattleUnit::GetOrigin(u16& x, u16& y) const
{
    x = static_cast<u16>(this->placement.x - this->originX);
    const auto value = static_cast<u16>(this->placement.y - static_cast<u16>(this->mainAct.y));
    y = value;
    return static_cast<i16>(value);
}

// swd32.exe 0x004784A0: the rectangle of the shadow picture (action 0x24 of the unit's act, or the main action's,
// 0x33 while defeated) under the unit
void BattleUnit::GetShadowRect(BattleServices& services, UnitRect& rect)
{
    if (this->defeated == 1 && this->kind == 0)
    {
        return;
    }

    if (this->actId == 0)
    {
        return;
    }

    ActState state;
    state.action = kShadowAction;
    state.id = this->actId;

    if (this->mainAct.action != 0)
    {
        state.action = this->mainAct.action;
    }

    if (this->defeated == 1)
    {
        state.action = kDefeatedShadowAction;
    }

    services.acts.Run(state);
    this->picture = services.archive.GetPicture(state.frame, state.picture);

    if (this->picture == nullptr)
    {
        return;
    }

    if (this->facing == 1)
    {
        state.x = this->picture->width - state.x;
    }

    rect.x = this->placement.x - state.x;
    rect.y = this->placement.y - state.y;
    rect.width = this->picture->width;
    rect.height = this->picture->height;
}

// swd32.exe 0x004785A0
i16 BattleUnit::Move(i16 dx, i16 dy)
{
    this->placement.x = static_cast<i16>(this->placement.x + dx);
    this->placement.y = static_cast<i16>(this->placement.y + dy);
    return dx;
}

// swd32.exe 0x004785C0: also saves the placement
i16 BattleUnit::SetPosition(i16 x, i16 y)
{
    this->placement.x = x;
    this->placement.y = y;
    this->savedPlacement = this->placement;
    return x;
}

// swd32.exe 0x00478600
i16 BattleUnit::GetPosition(u16& x, u16& y) const
{
    x = static_cast<u16>(this->placement.x);
    const auto value = static_cast<u16>(this->placement.y);
    y = value;
    return static_cast<i16>(value);
}

// swd32.exe 0x00478620: runs a copy of the main action in the step action and fetches its picture
const Picture* BattleUnit::StepIdleAction(BattleServices& services)
{
    this->stepAct = this->mainAct;

    if (services.acts.Run(this->stepAct) == 0)
    {
        return nullptr;
    }

    this->picture = services.archive.GetPicture(this->stepAct.frame, this->stepAct.picture);
    return this->picture;
}

// swd32.exe 0x00478670: the battle attributes (second) or the party record of a party role
const void* BattleUnit::SelectEntry(int second) const
{
    if (second != 0)
    {
        return this->battleRecord;
    }

    return this->roleRecord;
}

// swd32.exe 0x00478690
int BattleUnit::GetValueAAC() const
{
    return this->unk2AAC;
}

// swd32.exe 0x004786A0
int BattleUnit::GetValueAB4() const
{
    return this->unk2AB4;
}

// swd32.exe 0x004786B0
u16 BattleUnit::GetCommandA() const
{
    return this->commandA;
}

// swd32.exe 0x004786C0
u16 BattleUnit::GetCommandB() const
{
    return this->commandB;
}

// swd32.exe 0x004786D0
u16 BattleUnit::GetWord2A74() const
{
    return this->unk2A74;
}

// swd32.exe 0x004786E0
u16 BattleUnit::GetWord29A2() const
{
    return this->unk29A2;
}

// swd32.exe 0x004786F0
int BattleUnit::ClearIdleAction()
{
    this->mainAct = ActState{};
    return 0;
}

// swd32.exe 0x00478710: commands 2, 3, 6, 13, 14 and 15 go to commandB (6 and 14 / 15 also set flags of +0x2A87), any
// other to commandA
int BattleUnit::SetCommand(int command)
{
    if (command == BattleUnit::UnkCommand14 || command == BattleUnit::UnkCommand15)
    {
        this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag4000);
    }
    else if (command == BattleUnit::UnkCommand6)
    {
        this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag800);
    }
    else if (
        command != BattleUnit::UnkCommand2 && command != BattleUnit::UnkCommand13 && command != BattleUnit::UnkCommand3
    )
    {
        this->commandA = static_cast<u16>(command);
        return 1;
    }

    this->commandB = static_cast<u16>(command);
    this->commandA = 0;
    return 1;
}

// swd32.exe 0x00478770
void BattleUnit::ClearFlag26B8()
{
    this->unk26B8 &= ~kFlag26B8;
}

// swd32.exe 0x00478780: not while status bit 25 is set; raising it again restarts the stagger (hurtStep, +0x2A78)
// and the attack act
void BattleUnit::RaiseFlag26B8()
{
    if ((this->status & kRaiseBlocked) != 0)
    {
        return;
    }

    if (static_cast<i32>(this->unk26B8) < 0)
    {
        this->hurtStep = 0;
        this->attackAct.position = 0;
    }

    this->unk26B8 |= kFlag26B8;
}

// swd32.exe 0x004787C0
int BattleUnit::Flag26B8() const
{
    return (this->unk26B8 & kFlag26B8) != 0 ? 1 : 0;
}

// swd32.exe 0x004787D0: stores at the current count without advancing it
int BattleUnit::PushTarget(u16 value)
{
    const u16 count = this->targetCount;
    this->pushedTargets[count] = value;
    return count;
}

// swd32.exe 0x004787F0: a defeated role has its hit points set to 0
int BattleUnit::StartDefeat(int defeat)
{
    this->defeated = defeat;
    this->dying = 1;

    if (this->defeatPose == 0)
    {
        this->defeatPose = kFallbackDefeatPose;
    }

    if (this->kind == BattleUnit::PartyRole && this->roleRecord != nullptr)
    {
        this->roleRecord->hitPoints = 0;
    }

    return 1;
}

// swd32.exe 0x00478830
int BattleUnit::ToggleAF8()
{
    const int value = this->unk2AF8 == 0 ? 1 : 0;
    this->unk2AF8 = value;
    return value;
}

// swd32.exe 0x00478850: clears the state of a turn; roles get a random gauge of 50..189. The slot actions cleared
// are the first eight.
int BattleUnit::ResetTurn(BattleServices& services)
{
    this->gauge = 0;
    this->unk2AAC = 0;
    const u16 mode = this->command.flags;
    this->unk2AB0 = 0;
    this->unk2AB4 = 0;
    this->unk2AA8 = 0;
    this->unk2AE0 = 0;
    this->unk2B14 = 0;
    this->unk2A8C = 0;
    this->unk2A86 = 0;
    this->unk2AC8 = 0;
    this->unk2AD4 = 0;
    this->unk2ACC = 0;
    this->unk2B0C = 0;
    this->unk2AF4 = 0;
    this->commandA = 0;
    this->commandB = 0;
    this->unk2A72 = 0;
    this->unk2A7A = 0;
    this->unk29A4 = 0;
    this->fadeLevel = BattleUnit::Opaque;
    this->brightLevel = 1;
    this->slotTint = 0;
    this->unk2A68 = BattleUnit::FlashStart;
    this->unk2A6A = kTurnFlashFall;
    this->unk2674 = 0;
    this->action = 0;
    this->unk2690 = 0;
    this->unk2670 = 0;
    this->unk2A8A = 0;
    this->unk29A6 = 0;
    this->unk29A8 = 0;
    this->unk29AA = 0;
    this->mainAct.anchorX = 0;
    this->mainAct.anchorY = 0;
    this->unk26CC = 0;

    if ((mode & kResetClearsGuard) != 0)
    {
        this->guarding = 0;
    }

    if ((this->command.actionFlags & UnitCommand::DelaysFadeIn) != 0)
    {
        this->guarding = 0;
        this->brightLevel = BattleUnit::DelayedFadeIn;
        this->marks = static_cast<u16>((this->marks & ~BattleUnit::UnkMark8) | BattleUnit::UnkMark200);
    }

    if (this->unk2B00 == 1)
    {
        this->guarding = 1;
    }

    if (this->unk2B04 == 1)
    {
        this->placement = this->savedPlacement;
    }

    this->standAct = ActState{};
    this->castAct = ActState{};
    this->recoilAct = ActState{};
    this->attackAct = ActState{};

    for (int i = 0; i < kSlotActsPerTurn; ++i)
    {
        this->slotActs[static_cast<std::size_t>(i)] = ActState{};
    }

    std::fill_n(this->skillName.begin(), kClearedNameBytes, '\0');
    this->command = UnitCommand{};
    this->targets = {BattleUnit::NoTarget, BattleUnit::NoTarget, BattleUnit::NoTarget, BattleUnit::NoTarget};
    int result = this->kind;

    if (result == BattleUnit::PartyRole)
    {
        result = static_cast<int>(services.random.Below(kRoleGaugeSpread)) + kRoleGaugeBase;
        this->gauge = static_cast<u16>(result);
    }

    return result;
}

// swd32.exe 0x00478A70
i16 BattleUnit::Retarget(i16 value)
{
    this->unk2AB4 = 1;
    this->unk29A2 = static_cast<u16>(value);
    this->gauge = 0;
    return value;
}

// swd32.exe 0x00478AA0
void BattleUnit::IncreaseCounter2A76()
{
    ++this->unk2A76;
}

// swd32.exe 0x00478AB0
u16 BattleUnit::Counter2A76() const
{
    return this->unk2A76;
}

// swd32.exe 0x00478AC0
void BattleUnit::IncreaseHits()
{
    ++this->unk2A74;
    this->unk2AE0 = 1;
}

// swd32.exe 0x00478AE0
i16 BattleUnit::DecreaseHits()
{
    if (this->unk2A74 != 0)
    {
        --this->unk2A74;
    }

    auto result = static_cast<i16>(this->unk2A76);

    if (result != 0)
    {
        --result;
        this->unk2A76 = static_cast<u16>(result);
    }

    this->unk2AE0 = 0;
    return result;
}

// swd32.exe 0x00478B20
void BattleUnit::ClearWord29A2()
{
    this->unk29A2 = BattleUnit::NoUnk29A2;
}

// swd32.exe 0x00478B30
void BattleUnit::SetAA8()
{
    this->unk2AA8 = 1;
}

// swd32.exe 0x00478B40
int BattleUnit::GetAA8() const
{
    return this->unk2AA8;
}

// swd32.exe 0x00478B50
int BattleUnit::GetAE0() const
{
    return this->unk2AE0;
}

// swd32.exe 0x0047C660
int BattleUnit::SetAC0(int value)
{
    this->outline = value;
    return value;
}

// swd32.exe 0x0047C670
bool BattleUnit::IsAE4Clear() const
{
    return this->target == 0;
}

// swd32.exe 0x0047C680
u16 BattleUnit::GetWordDA6() const
{
    return this->command.unk16;
}

// swd32.exe 0x0047C690: unless flag 2 of +0x2A87 is set
void BattleUnit::MarkAC8()
{
    if ((this->unk2A86 & BattleUnit::UnkCommandFlag200) == 0)
    {
        this->unk2AC8 = 1;
    }
}

// The draw flags of the recoil picture shared by the fades: a forced mirror (blend mode) or a facing unit flips bit 0
// and measures the origin from the right edge of the picture.
void BattleUnit::MirrorRecoil(u32 actFlags, bool forced)
{
    const u16 width = this->picture != nullptr ? this->picture->width : 0;

    if (forced)
    {
        if (this->facing == 1)
        {
            this->drawFlags =
                (actFlags & Painter::Mirror) != 0 ? actFlags & ~Painter::Mirror : actFlags | Painter::Mirror;
            this->originX = static_cast<u16>(width - static_cast<u16>(this->recoilAct.x));
        }

        this->drawFlags |= Painter::Additive;
    }

    if (this->facing == 1 && !forced)
    {
        this->drawFlags = (actFlags & 1) != 0 ? actFlags & ~1u : this->drawFlags | 1;
        this->originX = static_cast<u16>(width - static_cast<u16>(this->recoilAct.x));
    }
}

// swd32.exe 0x0047C6B0: fades the unit out over fadeLevel (sound 0x2F on the first frame): translucent, or tinted
// darker in blend mode; 1 once it is gone
int BattleUnit::DrawFadeOut(BattleServices& services)
{
    if (static_cast<i32>(this->status) < 0 || this->fadeLevel <= 0)
    {
        return 1;
    }

    this->recoilAct.id = this->actId;
    const i32 paused = this->held;
    this->unk2AAC = 1;
    this->recoilAct.action = kShadowAction;
    this->recoilAct.hold = paused == 1 ? 1 : 0;

    if (services.acts.Run(this->recoilAct) == 0)
    {
        return 1;
    }

    this->picture = services.archive.GetPicture(this->recoilAct.frame, this->recoilAct.picture);

    if ((this->recoilAct.flags & ActState::Signal) != 0)
    {
        this->fadeLevel = 0;
        return 0;
    }

    if (this->fadeLevel == BattleUnit::Opaque)
    {
        this->recoilAct.view = kFadeOutSound;
        services.audio.PlaySound(kFadeOutSound, services.config.soundVolume);
        services.audio.SetSoundPan(this->recoilAct.view, this->facing == 1 ? kFadePan : -kFadePan);
        this->recoilAct.view = 0;
    }

    if (this->picture == nullptr)
    {
        return 0;
    }

    services.painter.SetSource(this->picture->data.data());
    const i32 blend = this->unk2B00;
    const u32 actFlags = this->recoilAct.flags;
    this->drawFlags = actFlags;
    this->MirrorRecoil(actFlags, blend == 1);
    const i32 level = this->fadeLevel;
    const int x = this->placement.x - static_cast<i16>(this->originX);
    const int y = this->placement.y - this->recoilAct.y;

    if (blend == 1)
    {
        DrawOptions& options = services.painter.Options();
        options.tintRed = level - BattleUnit::Opaque;
        options.tintGreen = level - BattleUnit::Opaque;
        options.tintBlue = level - BattleUnit::Opaque;
        services.painter.DrawPicture(
            x,
            y,
            this->picture->width,
            this->picture->height,
            this->drawFlags,
            PaletteArgument(*this->picture)
        );

        if (this->held == 0)
        {
            --this->fadeLevel;
        }

        return 0;
    }

    services.painter.Options().alpha = level;
    services.painter.DrawPicture(
        x,
        y,
        this->picture->width,
        this->picture->height,
        this->drawFlags | Painter::Translucent,
        PaletteArgument(*this->picture)
    );

    if (this->held == 0)
    {
        this->fadeLevel -= this->unk2A0A;
    }

    return 0;
}

// swd32.exe 0x0047C950: fades the unit in at the draw position of `anchor` (sound 0x30 on the first frame); 1 once it
// is fully visible
int BattleUnit::DrawFadeIn(BattleServices& services, const BattleUnit& anchor)
{
    if (static_cast<i32>(this->status) < 0)
    {
        return 1;
    }

    if (this->brightLevel >= BattleUnit::Opaque)
    {
        this->recoilAct = ActState{};
        this->unk2AAC = 0;
        return 1;
    }

    this->unk2AAC = 1;
    this->recoilAct.action = this->commandA == BattleUnit::UnkCommand1 ? kCommand1Action : kShadowAction;
    this->recoilAct.id = this->actId;
    const u16 pose = this->unk2A8C;

    if (pose != 0)
    {
        this->recoilAct.action = pose == kPose2 ? kPose2Action : kPoseAction;
    }

    if (services.acts.Run(this->recoilAct) == 0)
    {
        return 0;
    }

    if ((this->recoilAct.flags & ActState::Signal) != 0)
    {
        this->brightLevel = BattleUnit::FadedIn;
        return 0;
    }

    this->picture = services.archive.GetPicture(this->recoilAct.frame, 0);

    if (this->brightLevel == 1)
    {
        this->recoilAct.view = kFadeInSound;
        services.audio.PlaySound(kFadeInSound, services.config.soundVolume);
        services.audio.SetSoundPan(this->recoilAct.view, this->facing == 1 ? -kFadePan : kFadePan);
        this->recoilAct.view = 0;
    }

    if (this->picture == nullptr)
    {
        return 0;
    }

    services.painter.SetSource(this->picture->data.data());
    const u32 actFlags = this->recoilAct.flags;
    this->originX = static_cast<u16>(this->recoilAct.x);
    const i32 blend = this->unk2B00;
    this->drawFlags = actFlags;
    this->recoilAct.hold = 1;
    this->MirrorRecoil(actFlags, blend == 1);
    u16 anchorX = 0;
    u16 anchorY = 0;
    anchor.GetDrawPosition(anchorX, anchorY);
    const int x = static_cast<i16>(anchorX) - static_cast<i16>(this->originX);
    const int y = static_cast<i16>(anchorY) - this->recoilAct.y;
    const i32 level = this->brightLevel;

    if (this->unk2B00 == 1)
    {
        DrawOptions& options = services.painter.Options();
        options.tintRed = level - BattleUnit::FadedIn;
        options.tintGreen = level - BattleUnit::FadedIn;
        options.tintBlue = level - BattleUnit::FadedIn;
        services.painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->drawFlags);

        if (this->held == 0)
        {
            ++this->brightLevel;
        }

        return 0;
    }

    services.painter.Options().alpha = level;
    services.painter.DrawPicture(
        x,
        y,
        this->picture->width,
        this->picture->height,
        this->drawFlags | Painter::Translucent,
        PaletteArgument(*this->picture)
    );

    if (this->held == 0)
    {
        this->brightLevel += this->unk2A0A;
    }

    return 0;
}

// swd32.exe 0x0047CC40
int BattleUnit::SetHeld(int value)
{
    this->held = value;
    return value;
}

// swd32.exe 0x0047CC50
int BattleUnit::SetAD4(int value)
{
    this->unk2AD4 = value;
    return value;
}

// swd32.exe 0x0047CC60: a red and green tint pulse rising 2..24 and falling back over the shadow picture
int BattleUnit::DrawFlash(BattleServices& services)
{
    DrawOptions& options = services.painter.Options();
    const u16 phase = this->unk2A68;

    if (phase == BattleUnit::FlashPeak)
    {
        const u16 fall = this->unk2A6A;

        if (fall != 0)
        {
            if (this->held == 0)
            {
                this->unk2A6A = static_cast<u16>(fall - 1);
            }

            options.tintRed = this->unk2A6A;
            options.tintGreen = this->unk2A6A;
        }
        else
        {
            this->unk2A68 = BattleUnit::FlashStart;
            this->unk2A6A = BattleUnit::FlashPeak;
        }
    }
    else
    {
        if (this->held == 0)
        {
            this->unk2A68 = static_cast<u16>(phase + 1);
        }

        options.tintRed = this->unk2A68;
        options.tintGreen = this->unk2A68;
    }

    if (this->shadowPicture == nullptr)
    {
        return 1;
    }

    services.painter.SetSource(this->shadowPicture->data.data());
    u32 flags = this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
    this->unk26A4 = flags;
    flags |= Painter::TintUnder;
    services.painter.DrawPicture(
        this->placement.x - static_cast<i16>(this->originX),
        this->placement.y - this->mainAct.y,
        this->shadowPicture->width,
        this->shadowPicture->height,
        flags
    );
    return 1;
}

// swd32.exe 0x0047CD60: the evasion roll. Invisible units (status bit 31) evade 9 in 10; a given chance is out of 10;
// otherwise the role's evasion (+0x2A93) is rolled, never right after a success, forced after 24 failures in a row and
// one in three after 20 (a low evasion first passes an 80% no).
int BattleUnit::RollEvade(BattleServices& services, u16 chance)
{
    BattleState& state = services.state;

    auto success = [&]
    {
        state.evadeStreak = 0;
        state.evadeJustSucceeded = 1;
        this->marks = static_cast<u16>(this->marks | 1);
        return 1;
    };

    if (state.criticalAttack == 1)
    {
        return 0;
    }

    if (static_cast<i32>(this->status) < 0)
    {
        return static_cast<u16>(services.random.Below(kEvadeRoll) + 1) <= kInvisibleEvades ? success() : 0;
    }

    const u8 skill = this->unk2A93;

    if (skill == 0)
    {
        return 0;
    }

    if (chance != 0)
    {
        return static_cast<u16>(services.random.Below(kEvadeRoll)) < chance ? success() : 0;
    }

    if (state.evadeJustSucceeded == 1)
    {
        state.evadeJustSucceeded = 0;
        return 0;
    }

    const u8 streak = state.evadeStreak;
    bool hit = false;

    if (streak >= kSureEvadeStreak)
    {
        hit = true;
    }
    else if (streak >= kLikelyEvadeStreak)
    {
        hit = static_cast<u16>(services.random.Below(kLikelyEvadeOdds)) == kLikelyEvadeOdds - 1;
    }
    else if (skill <= kLowEvadeSkill)
    {
        hit = static_cast<u16>(services.random.Below(kEvadeRoll)) >= kLowEvadeGate &&
              static_cast<u16>(services.random.Below(kEvadeRoll)) < this->unk2A93;
    }
    else
    {
        hit = static_cast<u16>(services.random.Below(kEvadeRoll)) < this->unk2A93;
    }

    if (hit)
    {
        return success();
    }

    ++state.evadeStreak;
    return 0;
}

// swd32.exe 0x0047CE70
int BattleUnit::Mirrored() const
{
    return static_cast<int>(this->drawFlags & 1);
}

// swd32.exe 0x0047CE80
bool BattleUnit::IsDefeated() const
{
    return this->defeated == 1 || (this->marks & BattleUnit::DefeatedMark) != 0;
}

// swd32.exe 0x0047CEA0
int BattleUnit::StatusBit13() const
{
    return (this->marks & BattleUnit::DefeatedMark) != 0 ? 1 : 0;
}

// swd32.exe 0x0047CEB0
bool BattleUnit::IsAB8() const
{
    return this->defeated == 1;
}

// swd32.exe 0x0047CEC0: +1 (capped at 0x22) or -1 on the target count
int BattleUnit::StepTargetCount(int increase)
{
    if (increase == 1)
    {
        const bool below = ++this->targetCount < BattleUnit::MaxTargets;
        this->unk2AD8 = 1;

        if (!below)
        {
            this->targetCount = BattleUnit::MaxTargets - 1;
        }
    }
    else
    {
        --this->targetCount;
        this->unk2AD8 = increase;
    }

    return increase;
}

// swd32.exe 0x0047CF00
int BattleUnit::PushTargetAlt(u16 value)
{
    const u16 count = this->targetCount;
    this->damageDelays[count] = value;
    return count;
}

// swd32.exe 0x0047D2E0: value plus `tenths` tenths for -10..127, value * (tenths + 10) / 10 (an 8-bit sum) below
int ScaleByTenths(i8 tenths, i16& value)
{
    if (tenths >= -kTenths)
    {
        value = static_cast<i16>(value + tenths * value / kTenths);
        return 0;
    }

    value = static_cast<i16>(value * static_cast<i8>(tenths + kTenths) / kTenths);
    return 1;
}

// swd32.exe 0x0047D7D0
int BattleUnit::ResetAfterGuard()
{
    this->unk2AEC = 1;
    this->gauge = 0;
    this->unk2670 = 0;
    return 0;
}

// swd32.exe 0x0047D7F0
int BattleUnit::GetGuard() const
{
    if ((this->marks & (BattleUnit::UnkMark2 | BattleUnit::Held)) != 0)
    {
        return 0;
    }

    return this->unk2AEC;
}

// swd32.exe 0x0047D810: mark 0x200 also darkens the unit (brightLevel -32)
int BattleUnit::SetStatus(int bits)
{
    if (static_cast<u16>(bits) == BattleUnit::StatsScaled)
    {
        this->brightLevel = BattleUnit::DelayedFadeIn;
    }

    this->marks = static_cast<u16>(this->marks | static_cast<u16>(bits));
    return bits;
}

// swd32.exe 0x0047D830: clearing mark 0x1000 also ends the guard
int BattleUnit::ClearStatus(int bits)
{
    this->marks = static_cast<u16>(this->marks & static_cast<u16>(~bits));

    if ((bits & BattleUnit::UnkStatus1000) != 0)
    {
        this->guarding = 0;
    }

    return bits;
}

// swd32.exe 0x0047D860
i16 BattleUnit::SetWord2A0A(i16 value)
{
    this->unk2A0A = static_cast<u16>(value);
    return value;
}

// swd32.exe 0x0047D870
int BattleUnit::SetGuarding(int value)
{
    this->guarding = value;
    return value;
}

// swd32.exe 0x0047D880
int BattleUnit::ActFlag1() const
{
    return static_cast<int>((this->command.actionFlags >> 1) & 1);
}

// swd32.exe 0x0047D8A0
int BattleUnit::ActFlag11() const
{
    return (this->command.actionFlags & UnitCommand::UnkAction800) != 0 ? 1 : 0;
}

// swd32.exe 0x0047D8B0: the speed of the command when its bit 15 is set
i16 BattleUnit::ActSpeed() const
{
    const i16 speed = this->command.unk1E;
    return speed >= 0 ? i16{0} : static_cast<i16>(speed & kSpeedMask);
}

// swd32.exe 0x0047D8D0
int BattleUnit::ActFlag0() const
{
    return static_cast<int>(this->command.actionFlags & UnitCommand::UnkAction1);
}

// swd32.exe 0x0047D8E0
int BattleUnit::ActFlag13() const
{
    return (this->command.actionFlags & UnitCommand::UnkAction2000) != 0 ? 1 : 0;
}

// swd32.exe 0x0047D8F0
int BattleUnit::ActFlag14() const
{
    return (this->command.actionFlags & UnitCommand::UnkAction4000) != 0 ? 1 : 0;
}

// swd32.exe 0x0047D900: also restarts the main action
int BattleUnit::SetAction(i16 value)
{
    this->action = static_cast<u16>(value);
    this->mainAct = ActState{};
    return 0;
}

// swd32.exe 0x0047D930
bool BattleUnit::IsGuarding() const
{
    return this->guarding == 1 || (this->marks & BattleUnit::GuardingMark) != 0;
}

// swd32.exe 0x0047D950: roles set the current and maximum hit points, spirit and stamina of their party record
// (positive values; -1 refills the current value); enemies take the hit points as their maximum and current value
void BattleUnit::SetVitals(i16 hitPoints, i16 spirit, i16 stamina)
{
    if (this->kind == BattleUnit::PartyRole)
    {
        RoleRecord* record = this->roleRecord;

        if (record == nullptr)
        {
            return;
        }

        if (hitPoints > 0)
        {
            record->maxHitPoints = hitPoints;
            record->hitPoints = hitPoints;
        }

        if (hitPoints == -1)
        {
            record->hitPoints = record->maxHitPoints;
        }

        if (spirit > 0)
        {
            record->maxSpirit = spirit;
            record->spirit = spirit;
        }

        if (spirit == -1)
        {
            record->spirit = record->maxSpirit;
        }

        if (stamina > 0)
        {
            record->maxStamina = stamina;
            record->stamina = stamina;
        }

        if (stamina == -1)
        {
            record->stamina = record->maxStamina;
        }

        return;
    }

    if (this->data != nullptr)
    {
        this->data->data.maxHitPoints = hitPoints;
        this->data->data.hitPoints = hitPoints;
    }
}

// swd32.exe 0x0047DA10: positive values replace the attack, defence and agility (a party role's battle copy, not
// checked for null in the original; an enemy's data)
i16 BattleUnit::SetAttributes(i16 attack, i16 defense, i16 agility)
{
    if (this->kind == BattleUnit::PartyRole)
    {
        RoleRecord* record = this->battleRecord;

        if (record != nullptr)
        {
            if (attack > 0)
            {
                record->attackBonus = static_cast<u16>(attack);
            }

            if (defense > 0)
            {
                record->defenseBonus = static_cast<u16>(defense);
            }

            if (agility > 0)
            {
                record->agility = agility;
            }
        }

        return agility;
    }

    if (this->data == nullptr)
    {
        return 0;
    }

    if (attack > 0)
    {
        this->data->data.attack = static_cast<u16>(attack);
    }

    if (defense > 0)
    {
        this->data->data.defense = static_cast<u16>(defense);
    }

    if (agility > 0)
    {
        this->data->data.agility = static_cast<u16>(agility);
    }

    return agility;
}

// swd32.exe 0x0047DA90: an enemy's level
void BattleUnit::SetLevel(i16 level)
{
    if (this->kind != BattleUnit::PartyRole && this->data != nullptr)
    {
        this->data->data.level = static_cast<u16>(level);
    }
}

// swd32.exe 0x0047DAB0
int BattleUnit::SetB00(int value)
{
    this->unk2B00 = value;
    return value;
}

// swd32.exe 0x0047DAC0
int BattleUnit::SetB04(int value)
{
    this->unk2B04 = value;
    return value;
}

namespace
{

constexpr int kBattleTextLimit = 63;
constexpr int kAnnounceX = 0x118;
constexpr int kAnnounceY = 0xA;
constexpr i16 kAnnounceFrames = 0x28;
constexpr int kAnnounceFlags = BattleMessage::BlackBar | BattleMessage::Centered;
constexpr u32 kAilmentRoll = 10;  // an ailment lands on a roll of 1..kAilmentRoll up to the ailment chance
constexpr int kFramedFlags = BattleMessage::Frame | BattleMessage::Centered;
constexpr int kAnnounceSound = 0x117;
constexpr i32 kEffectAct = 0x2AF8;

// The damage of a hit (HitDamage).
constexpr std::size_t kResistances = 9;
constexpr u32 kAttackFromBonus = 9;        // unk26CC bits: the attack is the bonus of the battle record (enraged)
constexpr u32 kAttackDownIgnored = 0x100;  // unk26CC bit: AttackDown does not cut the attack
constexpr u32 kLevelShare = 3;             // the attack roll is 1..level / kLevelShare + kLevelRollBase
constexpr u32 kLevelRollBase = 3;
constexpr int kAttackBuffPercent = 30;
constexpr int kEnragedPercent = 60;  // an enraged attack: 60% plus unk26DC percent of 40%
constexpr int kEnragedSharePercent = 40;
constexpr double kBladeScale = 2.5;  // swd32.exe 0x00499D38
constexpr int kSkillScaleShare = 3;
constexpr u32 kSkillRollDivisors = 5;  // a skill rolls 1..(level + unk14) / (5..9)
constexpr u16 kSkillRollBase = 5;
constexpr int kHalf = 2;
constexpr int kGuardShare = 3;  // a guard takes a third off
// The acts of a scripted battle with a special effect, and the poses they leave.
constexpr u16 kFallAct = 1;
constexpr u8 kFallPose = 7;
constexpr u16 kDrainAct = 2;
constexpr u8 kDrainPose = 2;
constexpr u8 kDrainPoseAfter = 8;
constexpr u16 kWeakAct = 8;
constexpr int kWeakPercent = 85;
constexpr u8 kWeakPose = 0xD;
constexpr u16 kStrongAct = 0x11;
constexpr int kStrongPercent = 150;
constexpr u32 kStrongAilments = 3;
constexpr u32 kThirdAilment = 2;
constexpr u8 kStrongTurns = 3;
constexpr u8 kStrongPose = 1;

// FlyAct: the steps per frame of a speeding flight double up to kFlightDoubling, then grow by kFlightGrowth.
constexpr int kSpeeding = -1;
constexpr i32 kFlightDoubling = 0x20;
constexpr i32 kFlightGrowth = 0x30;
constexpr int kArrived = -1;

// DrawAction.
constexpr u32 kNudgeChance = 2;    // a nudged unit shakes in one frame of two
constexpr int kNudgeShift = 0x28;  // the shake moves the act by nudge - 0x28
constexpr int kPanCentre = 0x140;  // the sound pans right from the middle of the 640-pixel screen
constexpr int kShadowShare = 3;    // the shadow is a third of the picture height high
constexpr std::size_t kShadowShearShift = 2;
constexpr int kShadowTint = -6;
constexpr int kInvisibleShadowTint = -1;
constexpr i16 kRecoilFadeEnd = -0x20;  // DrawRecoil: the tint at which a waiting recoil act is over
constexpr i16 kRecoilFadeStep = 2;

// DrawTrail.
constexpr u8 kTrailDelay = 2;  // a new image every third frame
constexpr i16 kTrailLevel = 0xC;
constexpr i16 kAdditiveTrailLevel = 0x18;
constexpr int kTrailSpacing = 0x1E;
constexpr int kTrailCentre = 0xC;
constexpr int kTrailBlue = -12;
constexpr int kTrailTintBase = 0x1F;

// RunQueuedActions and TrailLastQueuedAction.
constexpr i8 kAdditiveTrailFade = 3;  // an additive act's images fade by 3 a frame, the others by 1
constexpr i32 kQueuedMaxDamage = 0x270F;
constexpr i8 kQueuedTrailImages = 3;

// ReturnHome.
constexpr u32 kWalksHome = 4;  // unk26CC bit 2: a party role walks home after its attack
constexpr i32 kRoleHomeAction = 0x32;
constexpr i32 kEnemyHomeAction = 0x24;
constexpr i32 kHomePose = 0x28;
constexpr i32 kHomePose1 = 0x30;  // an enemy with unk2A8C 1
constexpr i32 kHomePose2 = 0x31;  // and with kSecondHomePose
constexpr u16 kSecondHomePose = 2;
constexpr int kHomeSteps = 0x23;
constexpr int kHomeShare = 6;

// WalkToTarget.
constexpr i32 kRoleWalkAction = 0x2A;
constexpr int kWalkSteps = 0x20;
constexpr int kWalkShare = 16;

// TickEffects, CanUseAccessory, the drain effects and CatchRoll.
constexpr u16 kEffectTick = 0xF;
constexpr std::size_t kPoisonResistance = 4;  // RoleRecord::unk2D: the resistance of element 5
constexpr i16 kPoisonKind = 5;
constexpr u32 kStaminaDrainBits = 0x2000000;
constexpr i16 kStaminaDrain = 10;
constexpr u16 kAccessory19 = 0x19;
constexpr u32 kAccessory19Bit = 0x10;
constexpr u16 kAccessory1C = 0x1C;
constexpr u16 kAccessory1D = 0x1D;
constexpr u32 kAccessory1DBit = 0x100;
constexpr u16 kAccessory20 = 0x20;
constexpr u16 kSpiritFanKind = 0x39;
constexpr int kDrainLeft = 0x32;
constexpr int kDrainDown = 0x28;
constexpr u16 kDrainSpeed = 0x28;
constexpr u16 kDrainLife = 0x14;
constexpr u16 kDrainDivisor = 0x3C;
constexpr i32 kDrainTint = 5;
constexpr u32 kUnusedCatchRoll = 12;
constexpr u32 kCatchRoll = 10;
constexpr int kFarAbove = 10;
constexpr int kNearAbove = 5;
constexpr u16 kFarCatch = 8;
constexpr u16 kNearCatch = 5;

// HurtFrame.
constexpr i32 kHurtAction = 0x25;
constexpr i32 kGuardHurtAction = 0x27;
constexpr int kHurtRed = 0xF;     // the red flash an enemy leaves behind
constexpr int kHurtFlash = 0x10;  // the red flash over the picture
constexpr u16 kHurtFrames = 2;    // frames a stagger step lasts
constexpr u16 kLastHurtStep = 5;

// IdleFrame.
constexpr u16 kShaking = 1;   // drawAtSaved: the unit shakes while nudge lasts
constexpr u16 kRising = 2;    // lifts to the top
constexpr u16 kDropping = 3;  // drops back to its saved place
constexpr u32 kShakeChance = 2;
constexpr i16 kRiseStep = 10;
constexpr i16 kDropStep = 0x1E;
constexpr i16 kWeakShare = 4;  // a role at a quarter of its hit points or less looks weak
constexpr i32 kWeakAction = 0x26;
constexpr i16 kForcedPlaceX = 0x140;
constexpr i16 kForcedPlaceY = 0x136;
constexpr i32 kReadyAction = 0x2C;
constexpr u16 kReadyAct = 0x1C;  // an act whose ready pose is unk2690
constexpr i32 kItemReadyAction = 0x2D;
constexpr i32 kAbsorbReadyAction = 0x31;
constexpr u16 kReadyRewind = 5;
constexpr i16 kGuardPose1 = 1;
constexpr i16 kGuardPose2 = 2;
constexpr i16 kGuardPose4 = 4;
constexpr i16 kGuardPose8 = 8;
constexpr i32 kGuardAction1 = 0x48;
constexpr i32 kGuardAction2 = 0x49;
constexpr i32 kGuardAction4 = 0x4A;
constexpr i32 kGuardAction8 = 0x4B;
constexpr u32 kOutlinedStatus = 0x2000000;
constexpr u32 kWhiteColorPair = 0xFFFFFFFF;
constexpr i32 kThrownSpark = 0x186A;
constexpr i16 kGoldFont = 0x2354;
constexpr std::size_t kGoldVariable = 0;
constexpr int kGoldLeft = 10;
constexpr int kGoldAbove = 0xF;

// ChooseCommand.
constexpr int kNoChoice = 0xFFFF;
constexpr int kItemChoice = 0;
constexpr int kSkillChoice = 1;
constexpr int kHealChoice = 2;
constexpr int kStatusChoice = 4;  // a status of bit 5 or 8: the attack
constexpr int kIdleChoice = 5;    // no idle turn yet: the attack
constexpr u8 kItemsChosen = 1;    // choicePhase
constexpr u8 kSkillsChosen = 2;
constexpr u32 kUnusedChoiceRoll = 2;
constexpr u32 kChoiceRoll = 10;
constexpr u32 kFirstItemRoll = 4;   // a roll up to it takes the first object
constexpr u32 kSecondItemRoll = 7;  // the second
constexpr u32 kFirstSkillRoll = 6;
constexpr u8 kItemWaitTurns = 7;
constexpr u8 kSkillWaitTurns = 5;
constexpr u32 kNoForcedStatus = 0x10000000;  // in UnitCommand::unk08: a forced unit (unk2B00) does not choose it
constexpr i32 kFirstItemAction = 0x2C;       // the ready poses of an act kReadyAct by the object used
constexpr i32 kSecondItemAction = 0x2D;
constexpr i32 kThirdItemAction = 0x2E;
constexpr i32 kHealAction = 0x2F;
constexpr u16 kDelayedCommand = 0xC8;

// EnemyHitDamage.
constexpr u16 kSacrificeAilment = 0x8000;  // the ailment landed, the attacker falls (inferred)
constexpr int kSacrificeDamage = 0x270F;
constexpr u32 kEnemySpreadRoll = 5;  // the bonus of other commands rolls below (level + unk5C) / (5..9)
constexpr u32 kEnemySpreadBase = 5;
constexpr u32 kGuardCutRoll = 0x1E;  // a guard cuts 10..39%
constexpr u32 kGuardCutBase = 10;
constexpr u16 kFanStep = 5;
constexpr u16 kFanShare = 0x14;  // the spirit fan guards more by each 20% of proficiency
constexpr std::array<u16, 5> kFanGuards = {0x1080, 0x10C8, 0x10E9, 0x10EF, 0x12EF};

// RunAway.
constexpr i32 kRunEnd = 6;       // fadeLevel at which the run is over
constexpr u16 kStepPicture = 1;  // the picture of the act that plays the step sound
constexpr u16 kStepSound = 0x10F;
constexpr i32 kRunSoundAt = 0xF;
constexpr u16 kRunSound = 0x2F;
constexpr int kRunStep = 0x19;

// StepOrbCounters.
constexpr u16 kOrbStep = 0xC;  // the proficiency an element orb gains from a hit of its element

// DrawDamageNumbers.
constexpr int kNumberAbove = 0x1E;
constexpr int kDigitWidth = 0xC;
constexpr i16 kLastHop = 9;
constexpr i16 kNumberFadeEnd = 0x30;
constexpr i16 kNumberFadeStep = 2;
constexpr int kNumberScaleBase = 0x1A;

// RollDrop.
constexpr u32 kDropRoll = 20;

// RefineTalisman.
constexpr u16 kTwoChoices = 0x32;
constexpr u16 kTwoChoicesEnd = 0x4F;
constexpr u16 kThreeChoices = 0x50;
constexpr u16 kThreeChoicesEnd = 0x64;
constexpr u32 kRefineObjects = 3;
constexpr u32 kRefineRoll = 100;
constexpr u16 kRefineChance = 0x3C;
constexpr u16 kSureRefine = 0x5A;
constexpr u32 kRefinedKind = 0x8000000;

// FadeInvisible and StepAccessoryCounters.
constexpr i16 kInvisibleFadeFrames = 0x18;
constexpr int kShadowLeftShift = 5;
constexpr double kCounterPercent = 100.0;  // swd32.exe 0x00499D28

// StepGauge.
constexpr std::size_t kAgilityShare = 2;  // the gauge fills by a quarter of the agility a tick
constexpr int kBoostShare = 4;
constexpr int kInvisibleSlowdown = 4;

// FadeMainOut and Defense.
constexpr i16 kFadeOutStep = 2;
constexpr i16 kFadedOut = -0x20;
constexpr int kDefenseChangePercent = 30;
constexpr int kBarrierPercent = 70;

// DefeatFrame: the defeat poses (defeatPose) and their timing.
constexpr i32 kDefeatAction = 0x24;
constexpr i16 kDefeatedAtOnce = 0x3E8;
constexpr u8 kRisingPose = 1;
constexpr u8 kBurstPose = 2;
constexpr u8 kSplitRowsPose = 3;
constexpr u8 kSplitColumnsPose = 4;
constexpr u8 kFillPose = 5;
constexpr u8 kFadePose = 6;
constexpr u8 kQuartersPose = 7;
constexpr u8 kSweepPose = 8;
constexpr u8 kWavePose = 9;
constexpr u8 kBlueFillPose = 0xA;
constexpr u8 kSinkPose = 0xB;
constexpr u8 kZoomPose = 0xC;
constexpr u8 kStripsPose = 0xD;
constexpr u8 kHalvesPose = 0xE;
constexpr u8 kSlidePose = 0xF;
constexpr u8 kHopPose = 0x32;
constexpr u8 kProjectedPose = 0x33;
constexpr u8 kParticlePose = 0x64;
constexpr u16 kDefeatSound = 0x31;
constexpr i16 kRisingEnd = 0x1E0;
constexpr int kRisingTintDivisor = -31;
constexpr int kRisingTintLift = 4;
constexpr i16 kRisingStep = 0x1F;
constexpr i16 kParticlesDone = 100;
constexpr i16 kParticlesStarted = 0x65;
constexpr i32 kBurstTarget = 2;
constexpr u16 kBurstSpeed = 0x32;
constexpr u16 kBurstLife = 0xFA;
constexpr u16 kBurstDivisor = 0xA;
constexpr i32 kSweepLeft = -0x1E;
constexpr i32 kSweepRight = 0x29E;
constexpr i32 kSweepAbove = 0xA;
constexpr u16 kSweepSpeed = 0x64;
constexpr u16 kSweepLife = 0x64;
constexpr i16 kSplitEnd = 0x20;
constexpr i16 kSplitStep = 2;
constexpr int kDarkFillTint = -0x18;
constexpr int kBlueFillTint = 0x10;
constexpr i16 kDarkFillStep = 0xE;
constexpr i16 kBlueFillStep = 4;
constexpr int kQuarterDrop = 0x10;
constexpr i16 kWaveEnd = 0x2D;
constexpr int kWaveShare = 3;
constexpr i16 kSinkFadeFrom = -0x10;
constexpr int kSinkAlphaBase = 0x1F;
constexpr int kSinkStep = 10;
constexpr i16 kZoomEnd = 0x1E;
constexpr int kZoomUnit = 0x400;
constexpr i16 kZoomStep = 2;
constexpr i16 kHalvesSlow = 9;
constexpr i16 kHalvesPause = 8;
constexpr i16 kHalvesFast = 4;
constexpr i16 kSlideFadeFrom = -15;
constexpr i32 kProjectedAngle = 0x5A;
constexpr i16 kProjectedStep = 2;
constexpr i32 kProjectedTint = 5;
constexpr i32 kProjectedGrowth = 4;
constexpr i32 kWobbles = 6;
constexpr i16 kWobbleTop = 0x18;
constexpr i16 kWobbleBottom = -8;
constexpr i16 kWobbleRise = 4;
constexpr u16 kWobbleSound = 0xEB;
constexpr i32 kStripRight = 0x154;
constexpr i32 kStripWidth = 0x140;
constexpr u16 kStripSpeed = 0x18;
constexpr u16 kStripLife = 0x64;
constexpr u16 kStripDivisor = 0x28;
constexpr u16 kStripFlags = ParticleEffect::WithinRange | ParticleEffect::Mixed;
constexpr i32 kStripTint = 0xF;
constexpr int kShakeSpan = 3;
constexpr u16 kEffectIcons = 0x241D;

// The value of a skill script: the word at +0xE.
u16 SkillValue(const SkillScript& skill)
{
    return HighWord(skill.unk0C);
}

void SetEffectIcon(EffectNode& node, int frame, u16 icon)
{
    node.act.id = kEffectAct;
    node.act.action = frame;
    node.icon = icon;
}

// value +/- value / 10 on the word of a stat
template <typename Field>
void ScaleStat(Field& field, int sign)
{
    const auto value = static_cast<u16>(field);
    field = static_cast<Field>(static_cast<u16>(value + sign * (kStatScalePercent * value / kPercent)));
}

void ScaleRoleStats(RoleRecord& stats, int sign)
{
    ScaleStat(stats.attackBonus, sign);
    ScaleStat(stats.defenseBonus, sign);
    ScaleStat(stats.agility, sign);
    ScaleStat(stats.unk14, sign);
}

}  // namespace

// The message of an effect: the format with the unit's name in the battle text, with sound 0x117.
void BattleUnit::Announce(BattleServices& services, std::u8string_view format, int flags)
{
    const std::string_view name(this->displayName.data(), this->displayName.size());
    const std::string text = WinFormat(format, {name.substr(0, name.find('\0'))});
    auto& buffer = services.state.text;
    const std::size_t length = std::min<std::size_t>(text.size(), kBattleTextLimit);
    std::memcpy(buffer.data(), text.data(), length);
    buffer[length] = '\0';
    services.messages.Add(kAnnounceX, kAnnounceY, kAnnounceFrames, buffer, flags);
    services.audio.PlaySound(kAnnounceSound, services.config.soundVolume);
}

// swd32.exe 0x0047CF20: while mark 1 is set, the four letters (frames 0..3 of digit font 0) pop in one after another
// (the later ones only advance once the first is past phase 0), then rise and fade out by 4 per frame
void BattleUnit::DrawGuardText(BattleServices& services)
{
    if ((this->marks & 1) == 0)
    {
        return;
    }

    BattleHud& hud = services.hud;
    const int top = this->placement.y - kGuardTextOffset;
    const int left = this->placement.x - kGuardTextOffset;
    auto& phases = this->guardPhases;

    if (std::all_of(
            phases.begin(),
            phases.end(),
            [](i16 phase)
            {
                return phase == kGuardPhaseEnd;
            }
        ))
    {
        const i16 rise = this->guardRise;

        if (rise >= kGuardTextRise)
        {
            phases = {};
            this->guardRise = 0;
            hud.digitMode = 0;
            this->marks = static_cast<u16>(this->marks & ~BattleUnit::UnkMark1);
            this->action = 0;
            this->unk26A8 = 0;
            this->unk26AC = 0;
            return;
        }

        if (this->held == 0)
        {
            this->guardRise = static_cast<i16>(rise + kGuardTextStep);
        }

        u32 offset = 0;

        for (int letter = 0; letter < static_cast<int>(phases.size()); ++letter)
        {
            const i16 current = this->guardRise;
            DrawOptions& options = services.painter.Options();
            options.tintRed = -current;
            options.tintGreen = -current;
            options.tintBlue = -current;
            options.scaleHeight = current + kGuardTextHeight;
            offset += hud.DrawDigit(0, letter, static_cast<u16>(offset) + left, top - current);
        }

        return;
    }

    hud.digitMode = 0;
    u32 offset = hud.DrawDigit(0, 0, left, top - BattleUnit::PopOffsets[static_cast<std::size_t>(phases[0])]);

    for (std::size_t letter = 1; letter < phases.size() && phases[0] > 0; ++letter)
    {
        offset += hud.DrawDigit(
            0,
            static_cast<int>(letter),
            static_cast<u16>(offset) + left,
            top - BattleUnit::PopOffsets[static_cast<std::size_t>(phases[letter])]
        );

        if (this->held == 0)
        {
            ++phases[letter];
        }

        if (phases[letter] > kGuardPhaseEnd)
        {
            phases[letter] = kGuardPhaseEnd;
        }
    }

    if (++phases[0] > kGuardPhaseEnd)
    {
        phases[0] = kGuardPhaseEnd;
    }
}

// swd32.exe 0x0047D640: splits `amount` into the four digits of the current target slot (a set bit 15 negates it).
// While the barrier is up against a damage number, a hit only counts the barrier down and ends it with
// "抵抗所有攻擊解除".
void BattleUnit::SetDamageDigits(BattleServices& services, int amount)
{
    if ((this->marks & BattleUnit::UnkMark8000) != 0)
    {
        return;
    }

    const u16 slot = this->targetCount;

    if (this->pushedTargets[slot] == BattleUnit::DamageTarget && (this->status & kRaiseBlocked) != 0 &&
        (amount & BattleUnit::Healing) == 0)
    {
        if (--this->barrierTurns != 0)
        {
            return;
        }

        this->Announce(services, BattleMessages::ResistAllEnd, kAnnounceFlags);
        this->status &= ~kRaiseBlocked;
        return;
    }

    if ((amount & BattleUnit::Healing) != 0)
    {
        amount = -amount;
    }

    auto& digits = this->damageDigits[slot];
    digits[kThousandsDigit] = static_cast<i16>(amount / kThousands);
    amount -= kThousands * digits[kThousandsDigit];
    digits[kHundredsDigit] = static_cast<i16>(amount / kHundreds);
    amount -= kHundreds * digits[kHundredsDigit];
    digits[kTensDigit] = static_cast<i16>(amount / kTens);
    digits[kOnesDigit] = static_cast<i16>(amount - static_cast<u16>(static_cast<u16>(digits[kTensDigit]) * kTens));
}

// swd32.exe 0x0047DBD0: the ailment of `skill` lands unless it is bit 13 (mark 0x20 instead) or the skill has flag 7,
// or a roll of 1..10 exceeds the ailment chance. An effect of the same kind (the search starts with the head node of
// the unit) only gets the new turns and value; kind bit 15 adds nothing new. 1 when it landed.
int BattleUnit::AddAilment(BattleServices& services, const SkillScript& skill)
{
    const u16 kind = skill.ailment;

    if ((kind & SkillScript::MissedAilment) != 0)
    {
        this->marks = static_cast<u16>(this->marks | BattleUnit::MissedMark);
        return 0;
    }

    if (kind == 0 || (skill.flags & SkillScript::NoAilmentFlag) != 0)
    {
        return 0;
    }

    const int roll = static_cast<int>(services.random.Below(kAilmentRoll)) + 1;

    if (this->kind == BattleUnit::PartyRole)
    {
        if (roll > static_cast<u16>(this->battleRecord->ailmentChance))
        {
            return 0;
        }
    }
    else if (roll > this->data->data.ailmentChance)
    {
        return 0;
    }

    bool found = false;

    for (EffectNode* node = &this->effects; node != nullptr; node = node->next.get())
    {
        if (node->bits == skill.ailment)
        {
            node->turns = skill.turns;
            node->value = SkillValue(skill);
            found = true;
            break;
        }
    }

    if ((skill.ailment & SkillScript::RefreshOnly) != 0 || found)
    {
        return 1;
    }

    auto added = std::make_unique<EffectNode>();
    added->next = std::move(this->effects.next);
    added->turns = skill.turns;
    added->bits = skill.ailment;
    added->value = SkillValue(skill);
    EffectNode& node = *added;
    this->effects.next = std::move(added);

    switch (skill.ailment)
    {
        case SkillScript::UnkAilment1:
            SetEffectIcon(node, EffectNode::Ailment1Frame, EffectNode::Ailment1Icon);
            this->status |= skill.ailment;
            this->marks = static_cast<u16>(this->marks | BattleUnit::Held);
            break;

        case SkillScript::UnkAilment2:
            SetEffectIcon(node, EffectNode::Ailment2Frame, EffectNode::Ailment2Icon);
            this->status |= skill.ailment;
            this->marks = static_cast<u16>(this->marks | BattleUnit::UnkMark2);
            break;

        case SkillScript::UnkAilment4:
            this->status |= skill.ailment;
            node.icon = EffectNode::Ailment4Icon;
            this->marks = static_cast<u16>(this->marks | BattleUnit::Held);
            break;

        case SkillScript::UnkAilment8:
            SetEffectIcon(node, EffectNode::Ailment8Frame, EffectNode::Ailment8Icon);
            this->status |= skill.ailment;
            this->marks = static_cast<u16>(this->marks | BattleUnit::Held);
            break;

        case SkillScript::UnkAilment10:
            SetEffectIcon(node, EffectNode::Ailment10Frame, EffectNode::Ailment10Icon);
            this->status |= skill.ailment;
            this->marks = static_cast<u16>(this->marks | BattleUnit::UnkMark100);
            break;

        case SkillScript::UnkAilment20:
            SetEffectIcon(node, EffectNode::Ailment20Frame, EffectNode::Ailment20Icon);
            this->status |= skill.ailment;
            break;

        case SkillScript::SlowedAilment:
            SetEffectIcon(node, EffectNode::SlowedFrame, EffectNode::SlowedIcon);
            this->status |= skill.ailment;
            break;

        case SkillScript::UnkAilment80:
            SetEffectIcon(node, EffectNode::Ailment80Frame, EffectNode::Ailment80Icon);
            this->status |= skill.ailment;
            break;

        case SkillScript::UnkAilment100:
            SetEffectIcon(node, EffectNode::Ailment100Frame, EffectNode::Ailment100Icon);
            this->status |= skill.ailment;
            break;

        case SkillScript::StatsScaledAilment:
            SetEffectIcon(node, EffectNode::StatsScaledFrame, EffectNode::StatsScaledIcon);
            this->status |= skill.ailment;

            if (this->kind == BattleUnit::PartyRole)
            {
                ScaleRoleStats(*this->battleRecord, -1);
            }

            break;

        case SkillScript::UnkAilment1000:
            this->status |= skill.ailment;
            node.icon = EffectNode::UnkIcon1;
            break;

        default:
            break;
    }

    return 1;
}

// swd32.exe 0x0047E070: the buff of `skill` (its status bits, or its element with flag 3): bit 26 is the hit barrier
// (turns in barrierTurns); otherwise an effect with the same bits, or whose element equals the whole flags word of the
// skill, is refreshed (the search starts with the head node of the unit) or a new one is appended, then announced
int BattleUnit::AddBuff(BattleServices& services, const SkillScript& skill)
{
    const u32 bits = skill.statusBits;

    if (bits == 0 && (skill.flags & SkillScript::ElementBuff) == 0)
    {
        return 0;
    }

    if ((bits & BattleUnit::Barrier) != 0)
    {
        this->status |= kRaiseBlocked;
        this->barrierTurns = skill.turns;
        return 1;
    }

    EffectNode* node = &this->effects;
    bool refreshed = false;

    for (;;)
    {
        if (node->bits != 0 && node->bits == bits)
        {
            refreshed = true;
            break;
        }

        if (node->element != 0 && node->element == skill.flags)
        {
            refreshed = true;
            break;
        }

        if (node->next == nullptr)
        {
            break;
        }

        node = node->next.get();
    }

    if (refreshed)
    {
        node->turns = skill.turns;
    }
    else
    {
        node->next = std::make_unique<EffectNode>();
        node = node->next.get();
        node->turns = skill.turns;
        node->value = SkillValue(skill);

        if (skill.statusBits != 0)
        {
            node->bits = skill.statusBits;
        }

        if ((skill.flags & SkillScript::ElementBuff) != 0)
        {
            node->element = skill.element;
        }
    }

    std::u8string_view resistUp;

    switch (skill.element)
    {
        case SkillScript::Fire:
            resistUp = BattleMessages::FireResistUp;
            break;

        case SkillScript::Water:
            resistUp = BattleMessages::WaterResistUp;
            break;

        case SkillScript::Poison:
            resistUp = BattleMessages::PoisonResistUp;
            break;

        case SkillScript::Light:
            resistUp = BattleMessages::LightResistUp;
            break;

        case SkillScript::Dark:
            resistUp = BattleMessages::DarkResistUp;
            break;

        case SkillScript::Physical:
            resistUp = BattleMessages::PhysicalResistUp;
            break;

        default:
            break;
    }

    if (!resistUp.empty())
    {
        this->Announce(services, resistUp, kAnnounceFlags);
    }

    std::u8string_view text;

    switch (skill.statusBits)
    {
        case SkillScript::DefenseDown:
            node->act.action = 0;
            this->status |= BattleUnit::DefenseDown;
            text = BattleMessages::DefenseDown;
            break;

        case SkillScript::SpeedDown:
            node->act.action = 0;
            this->status |= BattleUnit::SpeedDown;
            text = BattleMessages::SpeedDown;
            break;

        case SkillScript::Barrier:
            node->act.action = 0;
            this->status |= BattleUnit::Barrier;

            if (this->kind != 0)
            {
                text = BattleMessages::BarrierStart;
            }

            break;

        case SkillScript::AttackDown:
            node->act.action = 0;
            this->status |= BattleUnit::AttackDown;
            text = BattleMessages::AttackDown;
            break;

        case SkillScript::Invisible:
            node->act.action = 0;
            this->marks = static_cast<u16>(this->marks | BattleUnit::InvisibleMark);

            if (this->kind != 0)
            {
                text = BattleMessages::InvisibleStart;
            }

            break;

        case SkillScript::DefenseUp:
            node->act.action = 0;
            this->status |= BattleUnit::DefenseUp;
            text = BattleMessages::DefenseUp;
            break;

        case SkillScript::SpeedUp:
            node->act.action = 0;
            this->status |= BattleUnit::SpeedUp;
            text = BattleMessages::SpeedUp;
            break;

        case SkillScript::AttackUp:
            node->act.action = 0;
            this->status |= BattleUnit::AttackUp;
            text = BattleMessages::AttackUp;
            break;

        default:
            break;
    }

    if (!text.empty())
    {
        this->Announce(services, text, kAnnounceFlags);
    }

    return 1;
}

// swd32.exe 0x0047E4C0: the effects after the head node
int BattleUnit::HasEffectElement(i16 element) const
{
    if (element == 0)
    {
        return 0;
    }

    for (const EffectNode* node = this->effects.next.get(); node != nullptr; node = node->next.get())
    {
        if (element == static_cast<i16>(node->element))
        {
            return 1;
        }
    }

    return 0;
}

// swd32.exe 0x0047E500: kinds 1, 2, 6 and 7 are the attack buffs of fire, ice, light and dark (other kinds look for
// bits 0); the effects after the head node
int BattleUnit::HasEffectKind(i16 kind) const
{
    const EffectNode* node = this->effects.next.get();

    if (node == nullptr)
    {
        return 0;
    }

    u32 bits = 0;

    switch (kind)
    {
        case SkillScript::Fire:
            bits = SkillScript::FireAttackUp;
            break;

        case SkillScript::Water:
            bits = SkillScript::IceAttackUp;
            break;

        case SkillScript::Light:
            bits = SkillScript::LightAttackUp;
            break;

        case SkillScript::Dark:
            bits = SkillScript::DarkAttackUp;
            break;

        default:
            break;
    }

    for (; node != nullptr; node = node->next.get())
    {
        if (bits == node->bits)
        {
            return 1;
        }
    }

    return 0;
}

// swd32.exe 0x0047E580: the effects after the head node
int BattleUnit::HasEffectBits(u16 bits) const
{
    for (const EffectNode* node = this->effects.next.get(); node != nullptr; node = node->next.get())
    {
        if ((node->bits & bits) != 0)
        {
            return 1;
        }
    }

    return 0;
}

// swd32.exe 0x0047E5C0: applies the pending damage (bit 15: healing) with its number; true when the unit fell
bool BattleUnit::ApplyPendingDamage(BattleServices& services)
{
    if (this->pendingDamage == 0)
    {
        return false;
    }

    this->PushTarget(BattleUnit::DamageTarget);

    if ((this->pendingDamage & BattleUnit::Healing) != 0)
    {
        this->PushTarget(BattleUnit::HealTarget);
    }

    this->SetDamageDigits(services, static_cast<i16>(this->pendingDamage));
    this->StepTargetCount(1);

    if ((this->pendingDamage & BattleUnit::Healing) == 0)
    {
        this->RaiseFlag26B8();
    }

    const int result = this->Damage(services, static_cast<i16>(this->pendingDamage), 0, 0);
    this->pendingDamage = 0;
    return result == 1;
}

// swd32.exe 0x0047E650: animates the first effect with an act (the head node itself when it has one) above the unit,
// then the stone (status bit 2) or freeze (bit 12) overlay of the shadow picture at (x, y)
void BattleUnit::DrawEffects(BattleServices& services, int x, int y)
{
    if (this->effects.next == nullptr)
    {
        return;
    }

    EffectNode* node = &this->effects;

    if (this->effects.act.id <= 0)
    {
        do
        {
            node = node->next.get();
        } while (node != nullptr && node->act.id <= 0);
    }

    if (node != nullptr)
    {
        node->act.hold = this->held == 1 ? 1 : 0;

        if (services.acts.Run(node->act) == 0)
        {
            return;
        }

        this->picture = services.archive.GetPicture(node->act.frame, node->act.picture);

        if (this->picture != nullptr && static_cast<i32>(this->unk26B8) >= 0)
        {
            services.painter.SetSource(this->picture->data.data());
            services.painter.DrawPicture(
                this->placement.x - node->act.x,
                this->placement.y - (this->picture->height >> 1) - this->mainAct.y,
                this->picture->width,
                this->picture->height,
                node->act.flags
            );
        }
    }

    const Picture* body = this->shadowPicture;

    if ((this->status & BattleUnit::UnkStatus4) != 0)
    {
        const u32 flags =
            (this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::GrayUnder;
        this->unk26A4 = flags;

        if (body != nullptr)
        {
            services.painter.SetSource(body->data.data());
            services.painter.DrawPicture(x, y, body->width, body->height, flags);
        }

        return;
    }

    if ((this->status & BattleUnit::UnkStatus1000) != 0)
    {
        this->unk26A4 = this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
        DrawOptions& options = services.painter.Options();
        options.tintRed = kUnkStatus1000Tint;
        options.tintGreen = 0;
        options.tintBlue = kUnkStatus1000Tint;

        if (body != nullptr)
        {
            services.painter.SetSource(body->data.data());
            services.painter.DrawPicture(x, y, body->width, body->height, this->unk26A4 | Painter::TintUnder);
        }
    }
}

// swd32.exe 0x0047E7F0: the icons (picture 0x241D, frame icon - 1) of the effects in a row, the head node first
void BattleUnit::DrawEffectIcons(BattleServices& services, int x, int y)
{
    if (this->effects.next == nullptr)
    {
        return;
    }

    int count = 0;

    for (const EffectNode* node = &this->effects; node != nullptr; node = node->next.get())
    {
        if (node->icon != 0 && static_cast<i32>(node->bits) >= 0)
        {
            this->picture = services.archive.GetPicture(kEffectIcons, static_cast<u16>(node->icon - 1u));

            if (this->picture != nullptr)
            {
                services.painter.SetSource(this->picture->data.data());
                services.painter
                    .DrawPicture(x + count * this->picture->width, y, this->picture->width, this->picture->height, 0);
            }

            ++count;
        }
    }
}

// swd32.exe 0x0047E880: ReviveBit revives a defeated party role (its record no longer marked, its turn reset);
// otherwise every effect sharing a bit with `bits` ends at once, the bits it covers (its low word) taken off `bits`,
// the effects expired, and the search starting again from the first effect until no bit is left
void BattleUnit::CureEffects(BattleServices& services, u16 bits)
{
    if (bits == BattleUnit::ReviveBit && this->kind == BattleUnit::PartyRole && this->defeated == 1)
    {
        this->roleRecord->unk24 = static_cast<u16>(this->roleRecord->unk24 & ~RoleRecord::Defeated);
        this->ResetTurn(services);
        this->defeated = 0;
        --services.state.defeatedRoles;
        return;
    }

    EffectNode* node = this->effects.next.get();

    while (node != nullptr)
    {
        if ((bits & node->bits) != 0)
        {
            bits = static_cast<u16>(bits & ~LowWord(node->bits));
            node->turns = 0;
            node->timer = 0;
            this->ExpireEffects(services, 0);

            if (bits == 0)
            {
                break;
            }

            node = this->effects.next.get();
            continue;
        }

        node = node->next.get();
    }
}

// swd32.exe 0x0047E950: with `clearAll` 0, every effect whose turns ran out loses its status and is announced, then
// freed; the effect following a freed one is not looked at in this pass. Otherwise all effects are freed and the
// status cleared.
void BattleUnit::ExpireEffects(BattleServices& services, int clearAll)
{
    if (clearAll != 0)
    {
        this->marks =
            static_cast<u16>(this->marks & ~(BattleUnit::UnkMark2 | BattleUnit::Held | BattleUnit::UnkMark100));
        this->status = 0;
        this->effects.next.reset();
        return;
    }

    std::unique_ptr<EffectNode>* link = &this->effects.next;

    for (;;)
    {
        EffectNode* node = link->get();

        if (node == nullptr)
        {
            return;
        }

        if (node->turns == 0)
        {
            bool clearHeld = false;

            switch (node->bits)
            {
                case SkillScript::UnkAilment1:
                    this->status &= ~BattleUnit::UnkStatus1;
                    clearHeld = (this->status & (BattleUnit::UnkStatus4 | BattleUnit::UnkStatus8)) == 0;
                    this->held = 0;
                    break;

                case SkillScript::UnkAilment2:
                    this->marks = static_cast<u16>(this->marks & ~BattleUnit::UnkMark2);
                    this->status &= ~BattleUnit::UnkStatus2;
                    break;

                case SkillScript::UnkAilment4:
                    this->status &= ~BattleUnit::UnkStatus4;
                    clearHeld = (this->status & (BattleUnit::UnkStatus1 | BattleUnit::UnkStatus8)) == 0;
                    this->held = 0;
                    break;

                case SkillScript::UnkAilment8:
                    this->status &= ~BattleUnit::UnkStatus8;
                    clearHeld = (this->status & (BattleUnit::UnkStatus1 | BattleUnit::UnkStatus4)) == 0;
                    this->held = 0;
                    break;

                case SkillScript::UnkAilment10:
                    this->marks = static_cast<u16>(this->marks & ~BattleUnit::UnkMark100);
                    this->status &= ~BattleUnit::UnkStatus10;
                    break;

                case SkillScript::UnkAilment20:
                case SkillScript::SlowedAilment:
                case SkillScript::UnkAilment80:
                case SkillScript::UnkAilment100:
                case SkillScript::UnkAilment1000:
                    this->status &= ~node->bits;
                    break;

                case SkillScript::StatsScaledAilment:
                    this->status &= ~BattleUnit::StatsScaled;

                    if (this->kind == BattleUnit::PartyRole)
                    {
                        ScaleRoleStats(*this->battleRecord, 1);
                    }

                    break;

                case SkillScript::DefenseDown:
                    this->Announce(services, BattleMessages::DefenseDownEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::DefenseDown;
                    break;

                case SkillScript::SpeedDown:
                    this->Announce(services, BattleMessages::SpeedDownEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::SpeedDown;
                    break;

                case SkillScript::DarkAttackUp:
                    this->Announce(services, BattleMessages::DarkAttackUpEnd, kAnnounceFlags);
                    break;

                case SkillScript::LightAttackUp:
                    this->Announce(services, BattleMessages::LightAttackUpEnd, kAnnounceFlags);
                    break;

                case SkillScript::IceAttackUp:
                    this->Announce(services, BattleMessages::IceAttackUpEnd, kFramedFlags);
                    break;

                case SkillScript::FireAttackUp:
                    this->Announce(services, BattleMessages::FireAttackUpEnd, kFramedFlags);
                    break;

                case SkillScript::Barrier:
                    this->Announce(services, BattleMessages::BarrierEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::Barrier;
                    break;

                case SkillScript::AttackDown:
                    this->Announce(services, BattleMessages::AttackDownEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::AttackDown;
                    this->unk26CC &= ~kAttackDownIgnored;
                    break;

                case SkillScript::Invisible:
                    this->marks = static_cast<u16>(this->marks | BattleUnit::UnkMark400);
                    this->status &= ~BattleUnit::Invisible;
                    this->glowLevel = 0;

                    if (this->kind != 0)
                    {
                        this->Announce(services, BattleMessages::InvisibleEnd, kAnnounceFlags);
                    }

                    break;

                case SkillScript::DefenseUp:
                    this->Announce(services, BattleMessages::DefenseUpEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::DefenseUp;
                    break;

                case SkillScript::SpeedUp:
                    this->Announce(services, BattleMessages::SpeedUpEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::SpeedUp;
                    break;

                case SkillScript::AttackUp:
                    this->Announce(services, BattleMessages::AttackUpEnd, kAnnounceFlags);
                    this->status &= ~BattleUnit::AttackUp;
                    break;

                default:
                    break;
            }

            if (clearHeld)
            {
                this->marks = static_cast<u16>(this->marks & ~BattleUnit::Held);
            }

            static constexpr std::array<std::u8string_view, 10> kResistEnd = {
                u8"",
                BattleMessages::ResistFireEnd,
                BattleMessages::ResistIceEnd,
                BattleMessages::ResistWindEnd,
                BattleMessages::ResistEarthEnd,
                BattleMessages::ResistPoisonEnd,
                BattleMessages::ResistLightEnd,
                BattleMessages::ResistDarkEnd,
                BattleMessages::ResistThunderEnd,
                BattleMessages::ResistPhysicalEnd,
            };

            if (node->element < kResistEnd.size() && !kResistEnd[node->element].empty())
            {
                this->Announce(services, kResistEnd[node->element], kAnnounceFlags);
            }

            std::unique_ptr<EffectNode> next = std::move(node->next);

            if (next == nullptr)
            {
                link->reset();
                return;
            }

            *link = std::move(next);
        }

        EffectNode* current = link->get();

        if (current->next == nullptr)
        {
            return;
        }

        link = &current->next;
    }
}

// swd32.exe 0x0047F150: applies hit point damage and spirit and stamina changes; 1 when the unit falls. A party role
// with the gold guard pays the damage from the gold while it is not zero (the gold can go below zero); an enemy with an
// override loses that first.
int BattleUnit::Damage(BattleServices& services, int hitPoints, i16 spirit, i16 stamina)
{
    if (hitPoints > 0 && ((this->marks & BattleUnit::UnkMark8000) != 0 || (this->status & kRaiseBlocked) != 0))
    {
        return 0;
    }

    if (this->defeated == 1 || this->EffectFlag3() == 1)
    {
        return 0;
    }

    if (this->kind != BattleUnit::PartyRole)
    {
        ObjectData& block = this->data->data;
        const i32 shield = this->hitPointsOverride;

        if (shield != 0)
        {
            this->hitPointsOverride = hitPoints < shield ? shield - hitPoints : 0;

            if (this->hitPointsOverride > block.maxHitPoints)
            {
                this->hitPointsOverride = block.maxHitPoints;
            }

            if (this->hitPointsOverride > 0)
            {
                return 0;
            }

            block.hitPoints = 0;
            this->hitPointsOverride = 0;
        }
        else
        {
            const i16 life = block.hitPoints;
            block.hitPoints = static_cast<i16>(hitPoints < life ? life - hitPoints : 0);

            if (block.hitPoints < 0)
            {
                block.hitPoints = static_cast<i16>(block.maxHitPoints);
            }

            if (block.hitPoints > block.maxHitPoints)
            {
                block.hitPoints = static_cast<i16>(block.maxHitPoints);
            }

            if (block.hitPoints > 0)
            {
                return 0;
            }

            block.hitPoints = 0;
        }

        const u8 pose = this->defeatPose;
        this->dying = 1;
        this->glowLevel = 0;

        if (pose == 0)
        {
            this->defeatPose = static_cast<u8>(services.random.Below(kDefeatPoses) + 1);
        }

        return 1;
    }

    RoleRecord& record = *this->roleRecord;
    i32& gold = services.party.variables[Party::Gold];

    if (this->goldGuard == 1 && gold != 0)
    {
        gold -= hitPoints;
    }
    else
    {
        record.hitPoints = static_cast<i16>(record.hitPoints - hitPoints);

        if (record.hitPoints > record.maxHitPoints)
        {
            record.hitPoints = record.maxHitPoints;
        }
    }

    record.spirit = static_cast<i16>(record.spirit + spirit);

    if (record.spirit > record.maxSpirit)
    {
        record.spirit = record.maxSpirit;
    }

    record.stamina = static_cast<i16>(record.stamina + stamina);

    if (record.stamina > record.maxStamina)
    {
        record.stamina = record.maxStamina;
    }

    if (record.spirit <= 0)
    {
        record.spirit = 0;
    }

    if (record.stamina <= 0)
    {
        record.stamina = 0;
    }

    if (record.hitPoints > 0)
    {
        return 0;
    }

    record.hitPoints = 0;
    this->dying = 1;

    if (this->unk2B00 == 1)
    {
        this->defeatPose = kFallbackDefeatPose;
    }

    return 1;
}

// swd32.exe 0x0047F340
int BattleUnit::EffectFlag2() const
{
    return (this->equipmentFlags & BattleUnit::BladeReturn) != 0 ? 1 : 0;
}

// swd32.exe 0x0047F360
int BattleUnit::EffectFlag3() const
{
    return (this->equipmentFlags & BattleUnit::Unk36) != 0 ? 1 : 0;
}

// swd32.exe 0x0047F380: a fixed pose overrides the requested one
void BattleUnit::SetDefeatPose(u8 pose)
{
    this->defeatPose = pose;

    if (this->fixedPose != 0)
    {
        this->defeatPose = this->fixedPose;
    }
}

// swd32.exe 0x0047F3A0
void BattleUnit::SetDefeatPoseFixed(u8 pose)
{
    this->fixedPose = pose;
    this->defeatPose = pose;
}

// swd32.exe 0x0047D350: clears the unit between battles: its actions (the slot actions 0..7 and 10), placement,
// command and target lists, effects (all freed), a party role's block, and its state words; the targets back to -1
void BattleUnit::Reset(BattleServices& services)
{
    this->mainAct = ActState{};
    this->standAct = ActState{};
    this->castAct = ActState{};
    this->recoilAct = ActState{};
    this->attackAct = ActState{};
    this->unk598 = ActState{};
    this->slotActs[BattleUnit::SlotActs - 1] = ActState{};

    for (int slot = 0; slot < kSlotActsPerTurn; ++slot)
    {
        this->slotActs[static_cast<std::size_t>(slot)] = ActState{};
    }

    this->placement = UnitPlacement{};
    this->command = UnitCommand{};
    this->numberFades = {};
    this->pushedTargets = {};
    this->trails = {};
    this->unk283C = {};
    this->ExpireEffects(services, 1);

    if (this->kind == BattleUnit::PartyRole && this->data != nullptr)
    {
        this->ownedData.reset();
        this->data = nullptr;
    }

    this->targets = {BattleUnit::NoTarget, BattleUnit::NoTarget, BattleUnit::NoTarget, BattleUnit::NoTarget};
    this->unk29A2 = BattleUnit::NoUnk29A2;
    this->fadeLevel = BattleUnit::Opaque;
    this->brightLevel = 1;
    this->unk2A68 = BattleUnit::FlashStart;
    this->unk2A6A = BattleUnit::FlashPeak;
    this->slotTint = 0;
    this->unk2660 = 0;
    this->unk2664 = 0;
    this->marks = 0;
    this->status = 0;
    this->unk26D2 = 0;
    this->glowLevel = 0;
    this->action = 0;
    this->unk29A4 = 0;
    this->actId = 0;
    this->gauge = 0;
    this->unk2A14 = 0;
    this->glowPhase = 0;
    this->commandA = 0;
    this->unk2A6E = 0;
    this->unk2A72 = 0;
    this->unk2A74 = 0;
    this->hurtStep = 0;
    this->unk2A7A = 0;
    this->targetCount = 0;
    this->queuedActions = 0;
    this->damageSlot = 0;
    this->landedActions = 0;
    this->unk26B8 = 0;
    this->healChoice = 0;
    this->unk2A92 = 0;
    this->unk2A93 = 0;
    this->defeatPose = 0;
    this->fixedPose = 0;
    this->unk2A86 = 0;
    this->idleTurns = 0;
    this->itemWait = 0;
    this->skillWait = 0;
    this->choicePhase = 0;
    this->pendingDamage = 0;
    this->equipmentFlags = 0;
    this->unk26CC = 0;
    this->kind = 0;
    this->unk2AA4 = 0;
    this->unk2AA8 = 0;
    this->unk2AAC = 0;
    this->unk2AB0 = 0;
    this->unk2AB4 = 0;
    this->defeated = 0;
    this->outline = 0;
    this->charging = 0;
    this->unk2AC8 = 0;
    this->unk267C = 0;
    this->held = 0;
    this->unk2AD4 = 0;
    this->unk2AD8 = 0;
    this->unk2ADC = 0;
    this->unk2AE0 = 0;
    this->unk2A8C = 0;
    this->target = 0;
    this->extraNumber = 0;
    this->unk2AEC = 0;
    this->guarding = 0;
    this->unk2AF4 = 0;
    this->dying = 0;
    this->unk2ACC = 0;
    this->unk2AF8 = 0;
    this->absorbing = 0;
    this->unk2B00 = 0;
    this->unk2B04 = 0;
    this->facing = 0;
    this->unk2B0C = 0;
    this->unk2B20 = 0;
}

// swd32.exe 0x0047F900
int BattleUnit::SetFacing(int value)
{
    this->facing = value;
    return value;
}

// swd32.exe 0x00483FE0
void BattleUnit::SetDisplayName(std::span<const char> name)
{
    this->displayName = name;
}

// swd32.exe 0x00483870: a party role's maximum stamina and its stamina (nothing for an enemy)
void BattleUnit::GetStamina(u16& maximum, u16& value) const
{
    if (this->kind == BattleUnit::PartyRole)
    {
        maximum = static_cast<u16>(this->roleRecord->maxStamina);
        value = static_cast<u16>(this->roleRecord->stamina);
    }
}

// swd32.exe 0x004838A0: a party role's maximum spirit and its spirit (nothing for an enemy)
void BattleUnit::GetSpirit(u16& maximum, u16& value) const
{
    if (this->kind == BattleUnit::PartyRole)
    {
        maximum = static_cast<u16>(this->roleRecord->maxSpirit);
        value = static_cast<u16>(this->roleRecord->spirit);
    }
}

// swd32.exe 0x00484500: the hit points and their maximum: a party role's from its record, an enemy's from its object
// (its current hit points when overridden)
void BattleUnit::GetHitPoints(i32& value, i32& maximum) const
{
    if (this->kind == BattleUnit::PartyRole)
    {
        maximum = this->roleRecord->maxHitPoints;
        value = this->roleRecord->hitPoints;
        return;
    }

    value = this->data->data.hitPoints;

    if (this->hitPointsOverride != 0)
    {
        value = this->hitPointsOverride;
    }

    maximum = this->data->data.maxHitPoints;
}

// swd32.exe 0x004755E0: the action gauge filled by a quarter of the agility (an eighth while slowed, with a quarter
// more and one when boosted; speed up and speed down add and take 30%, invisibility takes 10% more) while below full;
// a full gauge marks the unit ready (true). Held units do not move.
int BattleUnit::AdvanceGauge(const BattleServices& services, int boost)
{
    if ((this->marks & BattleUnit::Held) != 0 || (this->marks & BattleUnit::Unk4000) != 0)
    {
        return 0;
    }

    const u16 gauge = this->gauge;

    // 0x0047561B: the zero-extended gauge against the dword 0x004A74CC, signed (jl)
    if (static_cast<i32>(gauge) < services.state.gaugeFull)
    {
        int step = this->data->data.agility >> kAgilityShift;

        if ((this->status & BattleUnit::Slowed) != 0)
        {
            step = this->data->data.agility >> kSlowedAgilityShift;
        }

        if (boost == 1)
        {
            step += step / kBoostDivisor + 1;
        }

        int gain = 0;
        int loss = 0;

        if ((this->status & BattleUnit::SpeedUp) != 0)
        {
            gain = kSpeedChangePercent * step / kPercent;
        }

        if ((this->status & BattleUnit::SpeedDown) != 0)
        {
            loss = kSpeedChangePercent * step / kPercent;
        }

        if ((this->status & BattleUnit::Invisible) != 0)
        {
            loss = static_cast<u16>(kInvisibleSlowPercent * step / kPercent + loss);
        }

        this->unk2AB0 = 0;
        this->gauge = static_cast<u16>(step + gain + gauge - loss);
        return 0;
    }

    this->unk2AB0 = 1;
    this->unk2AEC = 0;

    if (this->unk2B20 == 1)
    {
        this->unk2B20 = 0;
        this->facing = 0;
    }

    return 1;
}

// The command block read as the skill script it was loaded from (the same 0x28-byte record in the original).
SkillScript BattleUnit::CommandSkill() const
{
    SkillScript skill;
    skill.unk00 = static_cast<u32>(this->command.unk00);
    skill.flags = this->command.actionFlags;
    skill.statusBits = static_cast<u32>(this->command.unk08);
    skill.unk0C = MakeWords(static_cast<u16>(this->command.value), this->command.flags);
    skill.unk10 = MakeWords(this->command.followAct, this->command.resistance);
    skill.unk14 = this->command.castAct;
    skill.unk16 = this->command.unk16;
    skill.ailment = this->command.ailment;
    skill.element = this->command.element;
    skill.unk1C = this->command.pose;
    skill.unk1E = static_cast<u16>(this->command.unk1E);
    skill.unk20 = this->command.unk20;
    skill.unk22 = this->command.target;
    skill.turns = this->command.unk24;
    return skill;
}

// The skill script `id` read into the command (0x00476A80 on the record at +0xD90; it sets only the fields its
// opcodes name).
void BattleUnit::LoadCommand(Database& database, u16 id)
{
    SkillScript skill = this->CommandSkill();
    database.LoadSkill(skill, id);
    this->command.unk00 = static_cast<i32>(skill.unk00);
    this->command.actionFlags = skill.flags;
    this->command.unk08 = static_cast<i32>(skill.statusBits);
    this->command.flags = static_cast<u16>(skill.unk0C);
    this->command.value = static_cast<i16>(HighWord(skill.unk0C));
    this->command.resistance = LowWord(skill.unk10);
    this->command.followAct = HighWord(skill.unk10);
    this->command.castAct = skill.unk14;
    this->command.unk16 = skill.unk16;
    this->command.ailment = skill.ailment;
    this->command.element = skill.element;
    this->command.pose = skill.unk1C;
    this->command.unk1E = static_cast<i16>(skill.unk1E);
    this->command.unk20 = skill.unk20;
    this->command.target = skill.unk22;
    this->command.unk24 = skill.turns;
}

// swd32.exe 0x00480AD0
ObjectRecord* BattleUnit::Data() const
{
    return this->data;
}

// swd32.exe 0x004728D0
bool BattleUnit::MarkBit11Clear() const
{
    return (this->marks & BattleUnit::UnkMark800) == 0;
}

// swd32.exe 0x0047F920: held (by a mark) or holding
int BattleUnit::IsHeldOrHolding() const
{
    if ((this->marks & BattleUnit::Held) != 0)
    {
        return 1;
    }

    return this->held;
}

// swd32.exe 0x0047F910
u16 BattleUnit::ActId() const
{
    return this->actId;
}

// swd32.exe 0x00482E90: the drain of the command: an attack drain (bit 8) drains life with a vampire ring (8) and hit
// points otherwise (1); bit 9 drains spirit (2); bit 10 stamina (4)
u16 BattleUnit::DrainKind() const
{
    const u32 flags = this->command.actionFlags;

    if ((flags & UnitCommand::DrainsLife) != 0)
    {
        return (this->equipmentFlags & BattleUnit::LifeDrain) != 0 ? u16{BattleUnit::DrainedLife}
                                                                   : u16{BattleUnit::DrainedHitPoints};
    }

    if ((flags & UnitCommand::DrainsSpirit) != 0)
    {
        return BattleUnit::DrainedSpirit;
    }

    return (flags & UnitCommand::DrainsStamina) != 0 ? u16{BattleUnit::DrainedStamina} : u16{0};
}

// swd32.exe 0x00483820
int BattleUnit::StatusBit5Or8() const
{
    if ((this->status & BattleUnit::UnkStatus20) != 0)
    {
        return 1;
    }

    return (this->status & BattleUnit::UnkStatus100) != 0 ? 1 : 0;
}

// swd32.exe 0x00483FD0
int BattleUnit::SetValueAAC(int value)
{
    this->unk2AAC = value;
    return value;
}

// swd32.exe 0x00483FF0: the facing (and +0x2B20) set, unless the unit is a defeated party role
int BattleUnit::SetFacingIfAble(int value)
{
    if (this->kind != BattleUnit::PartyRole || this->defeated != 1)
    {
        this->unk2B20 = value;
        this->facing = value;
        return value;
    }

    return 1;
}

// swd32.exe 0x00482080
void BattleUnit::SetNudge(u16 nudge, u16 saved)
{
    this->nudge = nudge;
    this->drawAtSaved = saved;
}

// swd32.exe 0x00483840: the target of a command that names one (flag 1)
bool BattleUnit::CommandTarget(u16& target) const
{
    if ((this->command.flags & 1) == 0)
    {
        return false;
    }

    target = this->command.target;
    return true;
}

// swd32.exe 0x00481FC0
int BattleUnit::MarkBit1() const
{
    return (this->marks >> 1) & 1;
}

// swd32.exe 0x00481FB0
bool BattleUnit::MarkBit8Clear() const
{
    return (this->marks & BattleUnit::UnkMark100) == 0;
}

// swd32.exe 0x00482E80: the role's battle block starts with 0x38
bool BattleUnit::HasBlock38() const
{
    return *this->roleBlock == kUnkBlock38;
}

// swd32.exe 0x004717D0
int BattleUnit::Word2A86Bit13() const
{
    return (this->unk2A86 & BattleUnit::UnkCommandFlag2000) != 0 ? 1 : 0;
}

// swd32.exe 0x004717E0
int BattleUnit::Word2A86Bit12() const
{
    return (this->unk2A86 & BattleUnit::UnkCommandFlag1000) != 0 ? 1 : 0;
}

// swd32.exe 0x00482030: the main act started over (cleared, the cast act marked done and +0x26D2 set) when it waits or
// when forced
bool BattleUnit::RestartMainAct(i16 value, int force)
{
    if (this->mainAct.waiting != 1 && force != 1)
    {
        return false;
    }

    this->mainAct = ActState{};
    this->castAct.waiting = 1;
    this->unk26D2 = value;
    return true;
}

// swd32.exe 0x00476140: the command sets status bit 28, or its flag 12
int BattleUnit::CommandStatusBit() const
{
    if ((static_cast<u32>(this->command.unk08) & kCommandStatusBit) != 0)
    {
        return 1;
    }

    return (this->command.actionFlags & UnitCommand::UnkAction1000) != 0 ? 1 : 0;
}

// swd32.exe 0x00476900
void BattleUnit::CommandValues(u16& value, u16& turns) const
{
    value = this->command.unk20;
    turns = this->command.unk24;
}

// swd32.exe 0x004768D0: a machine enemy (kind 0x800, like 蟹精魔機) of a type up to 0x15, never a named one
bool BattleUnit::IsSmallMachine() const
{
    const u32 kind = this->data->data.kind;

    if ((kind & BattleEnemy::NamedKind) != 0)
    {
        return false;
    }

    if ((kind & BattleEnemy::MachineKind) != 0)
    {
        return this->data->data.unk52 <= BattleEnemy::SmallMachineTypes;
    }

    return false;
}

// swd32.exe 0x00475870
void BattleUnit::Unk62And8A(u16& first, u16& second) const
{
    first = this->data->data.unk62;
    second = this->data->data.unk8A;
}

// swd32.exe 0x00482F70: the value (+0x40, low 14 bits) of the worn accessory of `kind`, 0 without one
u16 BattleUnit::AccessoryValue(u16 kind) const
{
    for (const ObjectRecord& accessory : this->accessories)
    {
        if (accessory.data.unk48 == kind)
        {
            return static_cast<u16>(accessory.data.unk40 & kAccessoryValueMask);
        }
    }

    return 0;
}

// swd32.exe 0x00482F10: the proficiency in percent of the worn accessory of `kind` (kept at +0x26DC; the last one
// kept without such an accessory)
i16 BattleUnit::AccessoryProficiency(u16 kind, CounterList& proficiency)
{
    for (const ObjectRecord& accessory : this->accessories)
    {
        if (accessory.data.unk48 == kind)
        {
            this->unk26DC = static_cast<u16>(FindCounterPercent(proficiency, static_cast<i16>(accessory.data.unk50)));
            return static_cast<i16>(this->unk26DC);
        }
    }

    return static_cast<i16>(this->unk26DC);
}

// swd32.exe 0x00476A10: the object's words +0x72..+0x76 set where non-zero (the fourth value goes to +0x74 as well)
void BattleUnit::SetWords72(i16 first, i16 second, i16 third, i16 fourth)
{
    ObjectData& object = this->data->data;

    if (first != 0)
    {
        object.skillA = static_cast<u16>(first);
    }

    if (second != 0)
    {
        object.skillACount = static_cast<u16>(second);
    }

    if (third != 0)
    {
        object.skillB = static_cast<u16>(third);
    }

    if (fourth != 0)
    {
        object.skillACount = static_cast<u16>(fourth);
    }
}

// swd32.exe 0x004769A0: the object's words +0x66..+0x70 set where non-zero
void BattleUnit::SetWords66(i16 a, i16 b, i16 c, i16 d, i16 e, i16 f)
{
    ObjectData& object = this->data->data;
    const std::array<std::pair<i16, u16*>, 6> words = {{
        {a, &object.itemA},
        {b, &object.itemACount},
        {c, &object.itemB},
        {d, &object.itemBCount},
        {e, &object.itemC},
        {f, &object.itemCCount},
    }};

    for (const auto& [value, word] : words)
    {
        if (value != 0)
        {
            *word = static_cast<u16>(value);
        }
    }
}

// swd32.exe 0x00476920: the object's resistances from the low bytes of nine words
void BattleUnit::SetResistances(std::span<const i16, 9> values)
{
    for (std::size_t element = 0; element < values.size(); ++element)
    {
        this->data->data.resistances[element] = static_cast<u8>(values[element]);
    }
}

// swd32.exe 0x00482EC0: the unit has an action and its main act shows picture `picture` - 1 (-1: the main act waits)
bool BattleUnit::ShowsPicture(i16 picture) const
{
    if (this->action == 0)
    {
        return false;
    }

    if (picture != -1)
    {
        return this->mainAct.picture == static_cast<u16>(picture - 1);
    }

    return this->mainAct.waiting == 1;
}

// swd32.exe 0x00476160: skill `id` of mon.dat taken as the unit's action: its act, its name, its command script (the
// command's +0x0E adding to +0x29A4) and command 2
void BattleUnit::LoadSkillAction(Database& database, u16 id, u16& act)
{
    database.LoadObject(this->object, id);
    act = this->object.data.unk50;
    const std::span<const char> name(this->object.data.name);
    const std::size_t length = std::min<std::size_t>(strnlen(name.data(), name.size()), this->skillName.size() - 1);
    std::copy_n(name.begin(), length, this->skillName.begin());
    this->skillName[length] = '\0';
    this->LoadCommand(database, this->object.data.unk3E);
    this->unk29A4 = static_cast<u16>(this->unk29A4 + static_cast<u16>(this->command.value));
    this->SetCommand(BattleUnit::UnkCommand2);
    this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
}

// swd32.exe 0x004761D0: an enemy's command: with +0x2A8C a command of flag 2 runs (command 2, its cast act returned);
// otherwise the enemy's own skill script is read afresh, +0x29A4 starts from its attack and command 1 follows
u16 BattleUnit::PrepareCommand(Database& database)
{
    if (this->unk2A8C != 0)
    {
        if ((this->command.flags & UnitCommand::UnkFlag2) == 0)
        {
            return 0;
        }

        this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
        this->SetCommand(BattleUnit::UnkCommand2);
        return this->command.castAct;
    }

    this->unk29A4 = 0;
    this->command = UnitCommand{};
    this->LoadCommand(database, this->data->data.skill);
    this->unk29A4 = static_cast<u16>(this->unk29A4 + this->data->data.attack);
    this->SetCommand(1);
    return 0;
}

// swd32.exe 0x00476080: an enemy below a third of its hit points heals itself half the time (on a roll of 0..4 its
// first way, 5..9 its second) when it has that way, may heal and is not marked otherwise
bool BattleUnit::ChooseHeal(BattleServices& services)
{
    u16 roll = static_cast<u16>(services.random.Below(kHealRoll));

    if (roll < kHealRoll / 2)
    {
        roll = 0;
    }
    else if (roll < kHealRoll)
    {
        roll = 1;
    }

    const ObjectData& object = this->data->data;
    i32 hitPoints = object.hitPoints;

    if (this->hitPointsOverride != 0)
    {
        hitPoints = this->hitPointsOverride;
    }

    if (hitPoints >= object.maxHitPoints / kHealBelowShare)
    {
        return false;
    }

    const bool free = (this->marks & BattleUnit::UnkMark100) == 0 && object.itemChance != 0;

    if (roll == 0)
    {
        if (object.healCountA != 0 && free)
        {
            this->healChoice = kFirstHeal;
            return true;
        }
    }
    else if (roll == 1 && object.healCountB != 0 && free)
    {
        this->healChoice = kSecondHeal;
        return true;
    }

    return false;
}

// swd32.exe 0x00481FD0: a command that does not move across (flag 4) starts at x 0, or 0x280 for a target command of
// an enemy or of a role facing left; one that does not move down (flag 5) at y 0
void BattleUnit::ClearMoveAxes(u16& x, u16& y) const
{
    if ((this->command.actionFlags & UnitCommand::StaysAcross) != 0)
    {
        x = 0;

        if ((this->command.flags & UnitCommand::TargetsUnit) != 0)
        {
            if (this->kind == BattleUnit::PartyRole && this->facing == 1)
            {
                x = Display::Width;
            }

            if (this->kind == 0)
            {
                x = Display::Width;
            }
        }
    }

    if ((this->command.actionFlags & UnitCommand::StaysDown) != 0)
    {
        y = 0;
    }
}

// swd32.exe 0x00474B60: a roll of 0..99 at most 70% of `percent` plus 10 (16-bit compare)
bool RollChance(Random& random, int percent)
{
    return static_cast<u16>(kChanceScale * percent / kPercent + kChanceBase) >=
           static_cast<u16>(random.Below(kPercent));
}

// swd32.exe 0x00481010: the damage of this unit's hit on `target` (negative: healing; -1 shows no number), with the
// spirit and stamina changes it makes. A barrier stops a hit that does not restore. The buff and ailment of the command
// land first (a held target is freed by a hit that restores nothing). The target's defence and resistances are those
// of its party record or its mon.dat object. A restoring command (actionFlags Restores) cures and heals at once; an
// attack takes the attack (or the battle record's bonus) plus a roll on the level, raised and cut by the attack buffs,
// less the defence, scaled when enraged and by the equipment, then by the resistance of the command's element. In a
// scripted battle the attacker's act adds a special effect. Skills take their value instead. The original computes
// the damage in 16 bits.
i16 BattleUnit::HitDamage(BattleServices& services, BattleUnit& target, i16& spiritDamage, i16& staminaDamage)
{
    i16 attack = static_cast<i16>(this->unk29A4);
    i32 healHitPoints = 0;
    i32 healSpirit = 0;
    i32 healStamina = 0;
    i32 raise = 0;
    i32 cut = 0;
    bool resisted = false;
    const RoleRecord* targetRecord = nullptr;
    const ObjectRecord* targetData = nullptr;
    u16 defense = 0;
    std::array<u8, kResistances> resistances = {};
    spiritDamage = 0;
    staminaDamage = 0;
    target.PushTarget(BattleUnit::DamageTarget);
    const bool restores = (this->command.actionFlags & UnitCommand::Restores) != 0;

    if (!restores && target.HasEffectBits(BattleUnit::UnkStatus4) == 1)
    {
        return 0;
    }

    const bool anyHeal = this->unk29A6 != 0 || this->unk29A8 != 0 || this->unk29AA != 0;

    if (target.AddBuff(services, this->CommandSkill()) == 1 && this->unk29A6 == 0 && !anyHeal)
    {
        return -1;
    }

    if (!anyHeal && target.IsHeldOrHolding() == 1)
    {
        target.CureEffects(services, BattleUnit::UnkStatus8);
    }

    auto resistanceOf = [&resistances](u16 element)
    {
        return static_cast<i8>(resistances[static_cast<std::size_t>(element - 1)]);
    };

    auto elemental = [](u16 element)
    {
        return element >= 1 && element <= kResistances;
    };

    if (target.kind == BattleUnit::PartyRole)
    {
        targetRecord = static_cast<const RoleRecord*>(target.SelectEntry(0));
        defense = targetRecord->defenseBonus;
        std::copy_n(targetRecord->unk2D.begin(), resistances.size(), resistances.begin());

        if (static_cast<u16>(services.random.Below(kAilmentRoll)) > static_cast<u16>(targetRecord->ailmentChance))
        {
            target.AddAilment(services, this->CommandSkill());
        }

        target.SetDefeatPose(LowByte(this->command.pose));
    }
    else
    {
        targetData = target.Data();
        defense = targetData->data.defense;
        std::copy_n(targetData->data.resistances.begin(), resistances.size(), resistances.begin());

        if (this->command.ailment != 0)
        {
            if (this->command.value != 0 && elemental(this->command.resistance))
            {
                ScaleByTenths(resistanceOf(this->command.resistance), this->command.value);
            }

            if (target.AddAilment(services, this->CommandSkill()) == 1)
            {
                target.RaiseFlag26B8();
            }
            else if (this->command.value == 0)
            {
                target.RollEvade(services, kEvadeRoll);
            }

            if (this->command.ailment != 0 && this->command.value == 0)
            {
                return -1;
            }
        }

        target.SetDefeatPose(LowByte(this->command.pose));
    }

    if (this->healPercent == 1)
    {
        if (targetRecord != nullptr)
        {
            healHitPoints = static_cast<i16>(this->unk29A6) * targetRecord->maxHitPoints / kPercent;
            healSpirit = targetRecord->maxSpirit * static_cast<i16>(this->unk29A8) / kPercent;
            healStamina = targetRecord->maxStamina * static_cast<i16>(this->unk29AA) / kPercent;
        }
    }
    else
    {
        healHitPoints = this->unk29A6;
        healSpirit = this->unk29A8;
        healStamina = this->unk29AA;
    }

    if (restores)
    {
        target.CureEffects(services, this->command.ailment);
        spiritDamage = static_cast<i16>(healSpirit);
        staminaDamage = static_cast<i16>(healStamina);

        if (LowWord(static_cast<u32>(healHitPoints)) == 0)
        {
            return -1;
        }

        target.PushTarget(BattleUnit::HealTarget);
        return static_cast<i16>(-healHitPoints);
    }

    const u16 kind = this->commandA;

    if (kind == BattleUnit::UnkCommand1 || kind == BattleUnit::UnkCommand1A || kind == BattleUnit::UnkCommand18)
    {
        if ((this->unk26CC & kAttackFromBonus) != 0)
        {
            attack = static_cast<i16>(this->battleRecord->attackBonus);
        }

        if ((this->unk2B00 == 1 || this->unk2B04 == 1) && this->command.value == 0)
        {
            attack = static_cast<i16>(this->battleRecord->attackBonus);
        }

        auto levelRoll = [this, &services]
        {
            const auto limit = static_cast<u32>(this->battleRecord->level / kLevelShare + kLevelRollBase);
            return static_cast<i32>(services.random.Below(limit)) + 1;
        };

        attack = static_cast<i16>(attack + levelRoll());

        if (this->HasEffectKind(static_cast<i16>(this->command.resistance)) == 1)
        {
            raise = kAttackBuffPercent * attack / kPercent;
        }

        if ((this->status & BattleUnit::AttackUp) != 0)
        {
            raise += kAttackBuffPercent * attack / kPercent;
        }

        if ((this->status & BattleUnit::AttackDown) != 0 && (this->unk26CC & kAttackDownIgnored) == 0)
        {
            cut = kAttackBuffPercent * attack / kPercent;
        }

        attack = static_cast<i16>(attack + (raise - cut));

        if (services.state.criticalAttack == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
        {
            attack = static_cast<i16>(static_cast<i16>(this->battleRecord->attackBonus) + levelRoll());
            u8 pose = 0;

            switch (this->placement.actId)
            {
                case kFallAct:
                    pose = kFallPose;
                    break;

                case kDrainAct:
                {
                    // A party role as the target has no object: the original reads a null pointer here.
                    if (targetData == nullptr || (targetData->data.kind & ObjectData::UnkKind20) != 0)
                    {
                        break;
                    }

                    i32 hitPoints = 0;
                    i32 maximum = 0;
                    target.GetHitPoints(hitPoints, maximum);

                    if (static_cast<i32>(defense) + hitPoints >= attack)
                    {
                        attack = static_cast<i16>(hitPoints + defense);
                    }

                    target.SetVitals(0, 0, 0);
                    target.SetDefeatPose(kDrainPose);
                    pose = kDrainPoseAfter;
                    break;
                }

                case kWeakAct:
                    attack = static_cast<i16>(kWeakPercent * attack / kPercent);
                    pose = kWeakPose;
                    break;

                case kStrongAct:
                {
                    attack = static_cast<i16>(kStrongPercent * attack / kPercent);

                    if (targetData == nullptr || (targetData->data.kind & ObjectData::UnkKind20) != 0)
                    {
                        break;
                    }

                    const u32 roll = services.random.Below(kStrongAilments);

                    if (roll == 0)
                    {
                        this->command.ailment = SkillScript::UnkAilment100;
                    }
                    else if (roll == 1)
                    {
                        this->command.ailment = SkillScript::SlowedAilment;
                    }
                    else if (roll == kThirdAilment)
                    {
                        this->command.ailment = SkillScript::UnkAilment1;
                    }

                    this->command.unk24 = kStrongTurns;
                    target.AddAilment(services, this->CommandSkill());
                    pose = kStrongPose;
                    break;
                }

                default:
                    break;
            }

            if (pose != 0)
            {
                target.SetDefeatPose(pose);
            }
        }

        attack = attack > static_cast<i32>(defense) ? static_cast<i16>(attack - defense) : i16{1};

        if ((this->unk26CC & kAttackFromBonus) != 0)
        {
            const i32 value = attack;
            const auto share = static_cast<u16>(kEnragedSharePercent * value / kPercent);
            attack = static_cast<i16>(kEnragedPercent * value / kPercent + share * this->unk26DC / kPercent);
        }

        if ((this->equipmentFlags & BattleUnit::BladeReturn) != 0)
        {
            attack = static_cast<i16>(crt::Ftol(static_cast<double>(attack) * kBladeScale));
        }

        if (elemental(this->command.resistance))
        {
            if (this->command.value == 0)
            {
                resisted = ScaleByTenths(resistanceOf(this->command.resistance), attack) == 1;
            }
            else if (this->unk2B00 == 0)
            {
                ScaleByTenths(resistanceOf(this->command.resistance), this->command.value);
                attack = static_cast<i16>(attack + static_cast<u16>(this->command.value));
                resisted = ScaleByTenths(static_cast<i8>(resistances.back()), attack) == 1;
            }
        }
    }

    if (kind == BattleUnit::UnkCommand1B)
    {
        const i32 scale = kSkillScaleShare * this->unk26DC / kPercent + 1;
        attack = static_cast<i16>(
            attack * scale + static_cast<u16>(this->battleRecord->attack * scale) + this->battleRecord->level
        );
    }

    if (kind == BattleUnit::UnkCommand1F)
    {
        return (target.marks & BattleUnit::UnkMark8000) != 0 ? i16{0} : attack;
    }

    if (kind == BattleUnit::UnkCommand2 || (this->unk2A86 & BattleUnit::UnkCommandFlag200) != 0 ||
        kind == BattleUnit::UnkCommand16 || kind == BattleUnit::UnkCommand3)
    {
        attack = this->command.value;

        if (this->HasEffectKind(static_cast<i16>(this->command.resistance)) == 1)
        {
            attack = static_cast<i16>(attack + kAttackBuffPercent * attack / kPercent);
        }

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag4000) == 0)
        {
            const auto divisor = static_cast<u16>(services.random.Below(kSkillRollDivisors) + kSkillRollBase);
            const i32 spread = this->battleRecord->level + static_cast<u16>(this->battleRecord->unk14);
            const auto limit = static_cast<u16>(spread / divisor);
            const i32 roll = static_cast<i32>(services.random.Below(limit)) + 1;

            if (attack > 0)
            {
                attack = static_cast<i16>(attack + roll);
            }
        }

        spiritDamage = static_cast<i16>(healSpirit);
        staminaDamage = static_cast<i16>(healStamina);

        if (elemental(this->command.resistance) && ScaleByTenths(resistanceOf(this->command.resistance), attack) == 1)
        {
            resisted = true;
        }
    }

    if (target.HasEffectElement(static_cast<i16>(this->command.element)) == 1)
    {
        attack = static_cast<i16>(attack / kHalf);
    }

    const bool healsHitPoints = static_cast<i16>(healHitPoints) != 0;

    if (healsHitPoints || kind == BattleUnit::UnkCommand22)
    {
        target.PushTarget(BattleUnit::HealTarget);
        attack = static_cast<i16>(-healHitPoints);
    }

    if (kind == BattleUnit::UnkCommand23)
    {
        spiritDamage = static_cast<i16>(healSpirit);
        target.PushTarget(BattleUnit::SpiritGainTarget);

        if (static_cast<i16>(healSpirit) == 0)
        {
            attack = -1;
        }
    }

    if (kind == BattleUnit::UnkCommand24)
    {
        staminaDamage = static_cast<i16>(healStamina);
        target.PushTarget(BattleUnit::StaminaGainTarget);
    }

    if ((static_cast<i16>(healSpirit) != 0 || static_cast<i16>(healStamina) != 0) && !healsHitPoints)
    {
        attack = -1;
    }

    if (resisted)
    {
        target.PushTarget(BattleUnit::HealTarget);
    }

    if (target.GetGuard() == 1 && !healsHitPoints && this->command.resistance != 0)
    {
        attack = static_cast<i16>(attack - attack / kGuardShare);
    }

    if (target.kind == BattleUnit::PartyRole && (targetRecord->unk24 & RoleRecord::Defeated) != 0)
    {
        spiritDamage = 0;
        attack = 0;
        staminaDamage = 0;

        if (static_cast<i16>(healSpirit) != 0)
        {
            target.PushTarget(BattleUnit::SpiritGainTarget);
        }

        if (static_cast<i16>(healStamina) != 0)
        {
            target.PushTarget(BattleUnit::StaminaGainTarget);
        }
    }

    return (target.marks & BattleUnit::UnkMark8000) != 0 ? i16{0} : attack;
}

// swd32.exe 0x0047F940: one frame of `act` (`id`) flying from (fromX, fromY) to the target's draw position (from the
// target back with `backwards`; a zero fromY keeps it level). The act is mirrored by the facing like the unit, its
// anchor measured from the picture's right edge. Each frame the flight takes flightSteps[slot] line steps, one more
// than the frame before, or with steps kSpeeding a doubling count (from 1, past 0x20 growing by 0x30). The act's sound
// plays, the picture is drawn, and 1 tells that the target's x was reached (the steps and the line are cleared).
int BattleUnit::FlyAct(
    BattleServices& services,
    const BattleUnit& target,
    ActState& act,
    int slot,
    i32 id,
    int fromX,
    int fromY,
    int steps,
    int backwards
)
{
    u16 targetX = 0;
    u16 targetY = 0;
    act.id = id;
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

    services.painter.SetSource(this->picture->data.data());
    const u16 anchorX = act.anchorX;
    u32 flags = act.flags;
    auto offsetX = static_cast<u16>(act.x);
    i32 anchorShift = static_cast<i16>(anchorX);
    const bool forced = this->unk2B00 == 1 || this->unk2B04 == 1;

    if (forced && this->facing == 1)
    {
        flags ^= Painter::Mirror;

        if (act.x != 0)
        {
            offsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
        }

        if (anchorX != 0)
        {
            anchorShift = this->picture->width - static_cast<i16>(act.anchorX);
        }
    }

    if (this->facing == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        flags ^= Painter::Mirror;
        offsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            anchorShift = this->picture->width - static_cast<i16>(anchorX);
        }
    }

    target.GetDrawPosition(targetX, targetY);
    const auto toY = static_cast<i16>(targetY - static_cast<u16>(act.y));
    const auto toX = static_cast<i16>(targetX - offsetX);
    LineWalker& line = this->flight;
    line = LineWalker{};
    const i32 startX = fromX - anchorShift;
    line.fromX = startX;
    line.fromY = fromY - static_cast<i16>(act.anchorY);
    line.toX = toX;
    line.toY = toY;

    if (backwards == 1)
    {
        line.fromX = toX;
        line.fromY = toY;
        line.toX = startX;
        line.toY = fromY - static_cast<i16>(act.anchorY);
    }

    if (fromY == 0)
    {
        line.fromY = 0;
        line.toY = 0;
    }

    i32& count = this->flightSteps[static_cast<std::size_t>(slot)];

    if (steps == kSpeeding)
    {
        steps = 1;

        if (count == 0)
        {
            count = 1;
        }

        count = count <= kFlightDoubling ? 2 * count : count + kFlightGrowth;
    }
    else
    {
        ++count;
    }

    int taken = 0;

    if (count * steps > 0)
    {
        while (true)
        {
            LineStep(line);

            if (line.fromX + line.x == line.toX)
            {
                taken = kArrived;
                break;
            }

            ++taken;

            if (taken >= count * steps)
            {
                break;
            }
        }
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    services.painter
        .DrawPicture(line.fromX + line.x, line.fromY + line.y, this->picture->width, this->picture->height, flags);

    if (taken != kArrived)
    {
        return 0;
    }

    count = 0;
    line = LineWalker{};
    return 1;
}

// swd32.exe 0x004831C0: one frame of `act` drawn over its shadow, on the target (a command with AtTarget) or at the
// unit's place with its outline. Mirrored by the facing like the unit (a forced draw also adds Additive); a nudged
// unit shakes, the nudge halving to 1 and staying there. The act's sound pans to the side of the screen it is on. An
// invisible unit draws a fainter shadow and smears the destination instead of the picture. The act's overlay picture
// follows, cleared once the act waits.
void BattleUnit::DrawAction(BattleServices& services, const BattleUnit& target, ActState& act)
{
    u16 targetX = 0;
    u16 targetY = 0;

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
    this->frameFlags = act.flags;
    this->actOffsetX = static_cast<u16>(act.x);
    this->actAnchorX = act.anchorX;
    this->overlayX = act.offsetX;
    const bool forced = this->unk2B00 == 1 || this->unk2B04 == 1;
    const bool still = this->unk2B00 == 0 && this->unk2B04 == 0;

    auto mirror = [this, &act]
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    };

    if (forced)
    {
        if (this->facing == 1)
        {
            mirror();
        }

        if (this->unk2B00 == 1 && (this->frameFlags & ActState::NoBlend) == 0)
        {
            this->frameFlags |= Painter::Additive;
        }
    }

    if (this->facing == 1 && still)
    {
        mirror();
    }

    target.GetDrawPosition(targetX, targetY);
    u16 x = targetX;

    if (this->nudge > 0 && services.random.Below(kNudgeChance) == 1)
    {
        x = static_cast<u16>(x + this->nudge - kNudgeShift);
        this->nudge = static_cast<u16>(this->nudge >> 1);

        if (this->nudge <= 1)
        {
            this->nudge = 1;
        }
    }

    if ((this->command.flags & UnitCommand::AtTarget) != 0)
    {
        this->actX = static_cast<u16>(x - this->actOffsetX);
        this->actY = static_cast<u16>(targetY - static_cast<u16>(act.y));
    }
    else
    {
        this->actX = static_cast<u16>(this->placement.x - this->actOffsetX);
        this->actY = static_cast<u16>(this->placement.y - static_cast<u16>(act.y));

        if (this->outline == 1)
        {
            this->colorPair = BattleUnit::OutlineColorPair;
            painter.DrawOutline(
                static_cast<i16>(this->actX),
                static_cast<i16>(this->actY),
                this->picture->width,
                this->picture->height,
                this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip),
                DrawArgument{.colorPair = this->colorPair}
            );
            this->colorPair = BattleUnit::OutlineColorPair;
        }
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int centre = static_cast<i16>(this->actX) + static_cast<i16>(this->actOffsetX);
    services.audio.SetSoundPan(act.view, centre >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;
    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const bool invisible = (this->status & BattleUnit::Invisible) != 0;
    const int tint = invisible ? kInvisibleShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.SetSource(this->picture->data.data());
    painter.DrawPicture(
        static_cast<i16>(this->actX),
        act.y + static_cast<i16>(this->actY) - shadowHeight,
        this->picture->width,
        this->picture->height,
        this->unk26A4
    );
    painter.SetSource(this->picture->data.data());

    if (invisible)
    {
        this->unk26A4 = this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
        painter.DrawPicture(
            static_cast<i16>(this->actX),
            static_cast<i16>(this->actY),
            this->picture->width,
            this->picture->height,
            this->unk26A4 | Painter::SmearUnder
        );
    }
    else
    {
        if ((this->frameFlags & ActState::Signal) != 0)
        {
            painter.Options().alpha = act.signal;
        }

        if (this->mainAct.wait != 0)
        {
            painter.SetWave(this->mainAct.wait);
        }

        painter.DrawPicture(
            static_cast<i16>(this->actX),
            static_cast<i16>(this->actY),
            this->picture->width,
            this->picture->height,
            this->frameFlags
        );
        painter.SetWave(0);
    }

    if (act.rotation == 0 || act.actionCode == 0)
    {
        return;
    }

    this->unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

    if (this->unk2554 == nullptr)
    {
        return;
    }

    painter.SetSource(this->unk2554->data.data());

    if (forced && this->facing == 1 && act.offsetX != 0)
    {
        this->overlayX = static_cast<u16>(this->picture->width - act.offsetX - this->unk2554->width);
    }

    if (this->facing == 1 && still && act.offsetX != 0)
    {
        this->overlayX = static_cast<u16>(this->picture->width - act.offsetX - this->unk2554->width);
    }

    painter.DrawPicture(
        static_cast<i16>(this->actX) + static_cast<i16>(this->overlayX),
        static_cast<i16>(this->actY) + static_cast<i16>(act.offsetY),
        this->unk2554->width,
        this->unk2554->height,
        this->frameFlags | Painter::Additive
    );

    if (act.waiting == 1)
    {
        act.rotation = 0;
        act.actionCode = 0;
        act.offsetX = 0;
        act.offsetY = 0;
    }
}

// swd32.exe 0x004838D0: one frame of `act` placed on the target (mirrored by the facing like the unit), or with a
// shift and an anchored act at the act DrawAction placed, the act's anchor apart (shiftX only decides, shiftY adds).
// The act's sound pans to the target's side of the screen; the act's target bits start the quake and raise flag 26B8
// of the target.
void BattleUnit::AimAction(BattleServices& services, BattleUnit& target, ActState& act, i16 shiftX, i16 shiftY)
{
    u16 targetX = 0;
    u16 targetY = 0;
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

    services.painter.SetSource(this->picture->data.data());
    this->unk26A4 = act.flags;
    this->aimOffsetX = static_cast<u16>(act.x);
    this->aimAnchorX = act.anchorX;

    auto mirror = [this, &act]
    {
        this->unk26A4 ^= Painter::Mirror;

        if (act.x != 0)
        {
            this->aimOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
        }

        if (act.anchorX != 0)
        {
            this->aimAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    };

    if ((this->unk2B00 == 1 || this->unk2B04 == 1) && this->facing == 1)
    {
        mirror();
    }

    if (this->facing == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        mirror();
    }

    target.GetDrawPosition(targetX, targetY);
    this->aimX = static_cast<u16>(targetX - this->aimOffsetX);
    this->aimY = static_cast<u16>(targetY - static_cast<u16>(act.y));

    if ((act.anchorX != 0 || act.anchorY != 0) && (shiftX != 0 || shiftY != 0))
    {
        this->aimX = static_cast<u16>(this->actX + this->actAnchorX - this->aimAnchorX);
        this->aimY = static_cast<u16>(this->actY - act.anchorY + shiftY);
    }

    this->ClearMoveAxes(this->aimX, this->aimY);
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, static_cast<i16>(targetX) >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;

    if ((act.target & ActState::Quakes) != 0)
    {
        services.state.quaking = 1;
        services.state.quakePhase = 0;
    }

    if ((act.target & ActState::UnkTarget4) != 0)
    {
        act.target = 0;
        target.RaiseFlag26B8();
    }
}

// swd32.exe 0x00483B30: one frame of the recoil act `act` at the place DrawAction drew the unit's act, `y` lower (the
// act measured from its anchor, mirrored by the facing like the unit). Its sound pans to the unit's side of the
// screen. Once the act waits, it is drawn tinted by recoilTint, darkening by 2 a frame (not while held) until it
// reaches kRecoilFadeEnd: then recoilTint is cleared and 1 tells that the recoil is over.
int BattleUnit::DrawRecoil(BattleServices& services, ActState& act, i16 y)
{
    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, static_cast<i16>(this->actX) >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return 0;
    }

    this->unk26A4 = act.flags;
    this->aimOffsetX = static_cast<u16>(act.x);

    auto mirror = [this, &act]
    {
        this->unk26A4 ^= Painter::Mirror;

        if (act.x != 0)
        {
            this->aimOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
        }
    };

    if ((this->unk2B00 == 1 || this->unk2B04 == 1) && this->facing == 1)
    {
        mirror();
    }

    if (this->facing == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        mirror();
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    const int drawX =
        static_cast<i16>(this->actAnchorX) - static_cast<i16>(this->aimOffsetX) + static_cast<i16>(this->actX);
    const int drawY = static_cast<i16>(this->actY) - act.y + y;

    if (act.waiting != 1)
    {
        if ((this->unk26A4 & ActState::Signal) != 0)
        {
            painter.Options().alpha = act.signal;
        }

        painter.DrawPicture(drawX, drawY, this->picture->width, this->picture->height, this->unk26A4);
        painter.Options().alpha = 0;
        return 0;
    }

    if (this->recoilTint <= kRecoilFadeEnd)
    {
        this->recoilTint = 0;
        return 1;
    }

    DrawOptions& options = painter.Options();
    options.tintRed = this->recoilTint;
    options.tintGreen = this->recoilTint;
    options.tintBlue = this->recoilTint;
    painter.DrawPicture(drawX, drawY, this->picture->width, this->picture->height, this->unk26A4);
    this->recoilTint = static_cast<i16>(this->recoilTint - kRecoilFadeStep);

    if (this->held == 1)
    {
        this->recoilTint = static_cast<i16>(this->recoilTint + kRecoilFadeStep);
    }

    return 0;
}

// swd32.exe 0x00483DB0: AimAction for a facing unit only (its x always measured from the right edge), drawn at once
// with the shadow picture's palette as the draw argument; 1 once the act waits.
int BattleUnit::DrawAimed(BattleServices& services, BattleUnit& target, ActState& act, i16 shiftX, i16 shiftY)
{
    u16 targetX = 0;
    u16 targetY = 0;
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

    this->unk26A4 = act.flags;
    this->aimOffsetX = static_cast<u16>(act.x);
    this->aimAnchorX = act.anchorX;

    if (this->facing == 1)
    {
        this->unk26A4 ^= Painter::Mirror;
        this->aimOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->aimAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    }

    target.GetDrawPosition(targetX, targetY);
    this->aimX = static_cast<u16>(targetX - this->aimOffsetX);
    this->aimY = static_cast<u16>(targetY - static_cast<u16>(act.y));

    if ((act.anchorX != 0 || act.anchorY != 0) && (shiftX != 0 || shiftY != 0))
    {
        this->aimX = static_cast<u16>(this->actX + this->actAnchorX - this->aimAnchorX);
        this->aimY = static_cast<u16>(this->actY - act.anchorY + shiftY);
    }

    this->ClearMoveAxes(this->aimX, this->aimY);
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, static_cast<i16>(targetX) >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;

    if ((act.target & ActState::Quakes) != 0)
    {
        services.state.quaking = 1;
        services.state.quakePhase = 0;
    }

    if ((act.target & ActState::UnkTarget4) != 0)
    {
        act.target = 0;
        target.RaiseFlag26B8();
    }

    services.painter.SetSource(this->picture->data.data());

    // The original reads the palette of the shadow picture through a null pointer when there is none.
    const DrawArgument argument =
        this->shadowPicture != nullptr ? PaletteArgument(*this->shadowPicture) : DrawArgument{};
    services.painter.DrawPicture(
        static_cast<i16>(this->aimX),
        static_cast<i16>(this->aimY),
        this->picture->width,
        this->picture->height,
        this->unk26A4,
        argument
    );
    return act.waiting == 1 ? 1 : 0;
}

// swd32.exe 0x004820A0: one frame of the trail: every third frame the images move back one place and the current
// picture (`frame`, `picture`, `mode` at x, y) is pushed in front at level 12 (24 when additive). Each image is drawn
// at its own y and fades by fadeStep (gone at 0): additive ones tinted by their level (blue fixed at -12), the others
// translucent at their level with the trail's tint under them while above 1. The original places the image at
// 12 + 30 per place for direction 0 (the image's x cancels), at x + y - 12 - 30 per place for Left, at x for InPlace.
void BattleUnit::DrawTrail(
    BattleServices& services,
    AfterimageTrail& trail,
    u16 frame,
    u16 picture,
    u32 mode,
    i16 x,
    i16 y
)
{
    Painter& painter = services.painter;
    int spacing = trail.direction == AfterimageTrail::Left ? -kTrailSpacing : kTrailSpacing;

    if (trail.delay == 0)
    {
        for (int place = trail.count - 1; place > 0; --place)
        {
            trail.images[static_cast<std::size_t>(place)] = trail.images[static_cast<std::size_t>(place - 1)];
        }

        AfterimageTrail::Image& front = trail.images[0];
        front.level = kTrailLevel;
        front.picture = picture;

        if ((mode & Painter::Additive) != 0)
        {
            front.level = kAdditiveTrailLevel;
        }

        front.mode = mode;
        front.x = x;
        front.y = y;
        trail.delay = kTrailDelay;
    }
    else
    {
        --trail.delay;
    }

    for (int place = 0; place < trail.count; ++place)
    {
        AfterimageTrail::Image& image = trail.images[static_cast<std::size_t>(place)];

        if (image.picture == AfterimageTrail::NoPicture)
        {
            continue;
        }

        int offset = kTrailCentre - image.x;

        if (trail.direction == AfterimageTrail::Left)
        {
            offset = image.y - kTrailCentre;
        }

        if (trail.direction == AfterimageTrail::InPlace)
        {
            offset = 0;
            spacing = 0;
        }

        this->unk2550 = services.archive.GetPicture(frame, image.picture);

        if (image.level > 0)
        {
            image.level = static_cast<i16>(image.level - trail.fadeStep);

            if (image.level <= 0)
            {
                image.picture = AfterimageTrail::NoPicture;
                image.level = 0;
            }
        }

        // A missing picture is read through a null pointer in the original.
        if (this->unk2550 == nullptr)
        {
            continue;
        }

        painter.SetSource(this->unk2550->data.data());
        const int drawX = image.x + spacing * place + offset;
        const int width = this->unk2550->width;
        const int height = this->unk2550->height;

        if ((mode & Painter::Additive) != 0)
        {
            DrawOptions& options = painter.Options();
            options.tintRed = image.level - kTrailTintBase;
            options.tintBlue = kTrailBlue;
            options.tintGreen = image.level - kTrailTintBase;
            painter.DrawPicture(drawX, image.y, width, height, mode);
            continue;
        }

        painter.Options().alpha = image.level;
        painter.DrawPicture(drawX, image.y, width, height, mode | Painter::Translucent);

        if (image.level > 1)
        {
            DrawOptions& options = painter.Options();
            options.tintRed = trail.tintRed;
            options.tintGreen = trail.tintGreen;
            options.tintBlue = trail.tintBlue;
            painter.DrawPicture(drawX, image.y, width, height, mode | Painter::TintUnder);
        }
    }
}

// swd32.exe 0x0047FC40: one frame of the queued acts (`action`, when not 0, given to each first); 1 when there are none
// (queuedActions cleared). Each act, mirrored by the facing, shakes on its line for its shake frames (its y from
// kShakeOffsets), then flies it (one step more a frame, doubling while speeding) with its trail; at the line's end its
// impact act (if any) plays to its end within the frame (the original runs the whole node again until it waits), the
// node is done and, unless the command is UnkAction1 or the impact is negative, the hit lands on `target`. Done nodes
// are freed. A shaking node takes the arrival of the node before it (the original's first is uninitialised).
int BattleUnit::RunQueuedActions(BattleServices& services, BattleUnit& target, i32 action)
{
    BattleState& battle = services.state;
    Painter& painter = services.painter;
    LineWalker& line = this->flight;
    QueuedAction* node = this->actions.get();
    int taken = 0;

    if (node == nullptr)
    {
        this->queuedActions = 0;
        return 1;
    }

    while (node != nullptr)
    {
        ActState& act = node->act;

        if (action != 0)
        {
            act.action = action;
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

        painter.SetSource(this->picture->data.data());
        u32 flags = act.flags;
        auto offsetX = static_cast<i16>(act.x);

        if (this->facing == 1)
        {
            flags ^= Painter::Mirror;
            offsetX = static_cast<i16>(this->picture->width - static_cast<u16>(act.x));
        }

        line = LineWalker{};
        line.fromX = node->fromX - offsetX;
        line.fromY = node->fromY - act.y;
        line.toX = node->toX - offsetX;
        line.toY = node->toY - act.y;

        if (node->fromY == 0)
        {
            line.fromY = 0;
        }

        if (node->toY == 0)
        {
            line.toY = 0;
        }

        if (node->shakeFrames != 0)
        {
            // A negative phase (shake frames past 0x7FFF) is read before the table in the original.
            const int phase = static_cast<i16>(node->shakeFrames) % static_cast<int>(BattleUnit::ShakeOffsets.size());
            line.y = phase >= 0 ? BattleUnit::ShakeOffsets[static_cast<std::size_t>(phase)] : 0;
            --node->shakeFrames;
        }
        else
        {
            if (node->steps == 1)
            {
                if (node->flown == 0)
                {
                    node->flown = 1;
                }

                node->flown =
                    static_cast<i16>(node->flown <= kFlightDoubling ? 2 * node->flown : node->flown + kFlightGrowth);
            }
            else
            {
                ++node->flown;
            }

            taken = 0;

            while (taken < node->flown * node->steps)
            {
                LineStep(line);

                if (line.fromX + line.x == line.toX && line.fromY + line.y == line.toY)
                {
                    taken = kArrived;
                    break;
                }

                ++taken;
            }
        }

        services.audio.PlaySound(act.view, services.config.soundVolume);
        act.view = 0;
        const int x = line.fromX + line.x;
        const int y = line.fromY + line.y;

        if (node->trailed == 1)
        {
            node->trail.fadeStep = (flags & Painter::Additive) != 0 ? kAdditiveTrailFade : 1;
            this->DrawTrail(
                services,
                node->trail,
                act.frame,
                act.picture,
                flags,
                static_cast<i16>(x),
                static_cast<i16>(y)
            );
        }

        painter.SetSource(this->picture->data.data());
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, flags);

        if (taken != kArrived)
        {
            node = node->next.get();
            continue;
        }

        ActState& impact = node->impact;

        if (impact.id > 0)
        {
            if (services.acts.Run(impact) == 0)
            {
                return 0;
            }

            this->picture = services.archive.GetPicture(impact.frame, impact.picture);

            if (this->picture == nullptr)
            {
                return 0;
            }

            painter.SetSource(this->picture->data.data());
            u32 impactFlags = impact.flags;

            if (this->facing == 1)
            {
                impactFlags ^= Painter::Mirror;
            }

            services.audio.PlaySound(impact.view, services.config.soundVolume);
            services.audio.SetSoundPan(impact.view, line.toX >= kPanCentre ? kFadePan : -kFadePan);
            impact.view = 0;
            painter.DrawPicture(
                line.fromX + line.x,
                line.fromY + line.y,
                this->picture->width,
                this->picture->height,
                impactFlags
            );

            if (impact.waiting == 0)
            {
                continue;
            }
        }

        node->done = 1;
        ++this->landedActions;
        line = LineWalker{};

        if ((this->command.actionFlags & UnitCommand::UnkAction1) == 0 && impact.id >= 0)
        {
            target.RaiseFlag26B8();
            i16 spirit = 0;
            i16 stamina = 0;
            i32 damage = this->HitDamage(services, target, spirit, stamina);
            battle.lastDamage = damage;

            if (damage >= kQueuedMaxDamage)
            {
                damage = kQueuedMaxDamage;
                battle.lastDamage = damage;
            }

            battle.hitDamage += damage;
            target.SetDamageDigits(services, damage);
            target.StepTargetCount(1);
        }

        node = node->next.get();
    }

    std::unique_ptr<QueuedAction>* link = &this->actions;

    while (*link != nullptr)
    {
        if ((*link)->done == 1)
        {
            *link = std::move((*link)->next);
        }
        else
        {
            link = &(*link)->next;
        }
    }

    return 0;
}

// swd32.exe 0x004800F0: appends an act (`id`, `action`) flying from (fromX, fromY) to (toX, toY) with `steps` line
// steps a frame flown (1: speeding) after `shakeFrames` frames of shaking, and its impact act; counts it in
// queuedActions
void BattleUnit::AddQueuedAction(
    i32 id,
    i32 action,
    i16 fromX,
    i16 fromY,
    i16 toX,
    i16 toY,
    i16 steps,
    u16 shakeFrames,
    i32 impact
)
{
    std::unique_ptr<QueuedAction>* link = &this->actions;

    while (*link != nullptr)
    {
        link = &(*link)->next;
    }

    *link = std::make_unique<QueuedAction>();
    QueuedAction& node = **link;
    node.act.id = id;
    node.act.action = action;
    node.impact.id = impact;
    node.fromX = fromX;
    node.fromY = fromY;
    node.toX = toX;
    node.toY = toY;
    node.shakeFrames = shakeFrames;
    node.steps = steps;
    node.done = 0;
    ++this->queuedActions;
}

// swd32.exe 0x004801A0: the last queued act leaves a trail (three images in place, tinted under them); its first call
// clears the images. The original reads the list head as a node when nothing is queued.
void BattleUnit::TrailLastQueuedAction(i32 tintRed, i32 tintGreen, i32 tintBlue)
{
    QueuedAction* last = this->actions.get();

    if (last == nullptr)
    {
        return;
    }

    while (last->next != nullptr)
    {
        last = last->next.get();
    }

    last->trailed = 1;
    AfterimageTrail& trail = last->trail;

    if (trail.started != 0)
    {
        return;
    }

    trail.started = 1;

    for (AfterimageTrail::Image& image : trail.images)
    {
        image.picture = AfterimageTrail::NoPicture;
    }

    trail.tintRed = tintRed;
    trail.tintGreen = tintGreen;
    trail.tintBlue = tintBlue;
    trail.direction = AfterimageTrail::InPlace;
    trail.count = kQueuedTrailImages;
}

// swd32.exe 0x00483700: the overlay picture (`frame`, `picture` - 1; nothing when either is 0) drawn additively at
// (x + offsetX, y + offsetY), mirrored like the unit: a facing unit measures offsetX (when not 0) from the right edge
// of its current picture, less the overlay's width
void BattleUnit::DrawOverlay(BattleServices& services, u16 frame, u16 picture, int x, int y, int offsetX, int offsetY)
{
    if (frame == 0 || picture == 0)
    {
        return;
    }

    this->overlayX = static_cast<u16>(offsetX);
    this->unk2554 = services.archive.GetPicture(frame, static_cast<u16>(picture - 1));

    // A missing picture is read through a null pointer in the original.
    if (this->unk2554 == nullptr)
    {
        return;
    }

    services.painter.SetSource(this->unk2554->data.data());
    const bool forced = this->unk2B00 == 1 || this->unk2B04 == 1;

    auto mirror = [this, offsetX]
    {
        if (offsetX != 0 && this->picture != nullptr)
        {
            this->overlayX = static_cast<u16>(this->picture->width - offsetX - this->unk2554->width);
        }
    };

    if (forced && this->facing == 1)
    {
        mirror();
    }

    if (this->facing == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        mirror();
    }

    services.painter.DrawPicture(
        x + static_cast<i16>(this->overlayX),
        y + offsetY,
        this->unk2554->width,
        this->unk2554->height,
        this->frameFlags | Painter::Additive
    );
}

// swd32.exe 0x00484230: DrawAction at (x, y) without the outline, the shake and the overlay: `act` over its shadow,
// mirrored like the unit (a forced draw adds Additive), the sound panned to its side. An invisible unit smears the
// destination with the act's own flags; a blending act (both bits of ActState::Signal) takes the main act's level.
void BattleUnit::DrawActionAt(BattleServices& services, i16 x, i16 y, ActState& act)
{
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
    this->frameFlags = act.flags;
    this->actOffsetX = static_cast<u16>(act.x);
    this->actAnchorX = act.anchorX;

    auto mirror = [this, &act]
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(this->picture->width - act.anchorX);
        }
    };

    if (this->unk2B00 == 1 || this->unk2B04 == 1)
    {
        if (this->facing == 1)
        {
            mirror();
        }

        if (this->unk2B00 == 1)
        {
            this->frameFlags |= Painter::Additive;
        }
    }

    if (this->facing == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        mirror();
    }

    this->actX = static_cast<u16>(x - this->actOffsetX);
    this->actY = static_cast<u16>(y - static_cast<u16>(act.y));
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int centre = static_cast<i16>(this->actX) + static_cast<i16>(this->actOffsetX);
    services.audio.SetSoundPan(act.view, centre >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;
    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const bool invisible = (this->status & BattleUnit::Invisible) != 0;
    const int tint = invisible ? kInvisibleShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.SetSource(this->picture->data.data());
    painter.DrawPicture(
        static_cast<i16>(this->actX),
        act.y + static_cast<i16>(this->actY) - shadowHeight,
        this->picture->width,
        this->picture->height,
        this->unk26A4
    );
    painter.SetSource(this->picture->data.data());

    if (invisible)
    {
        painter.DrawPicture(
            static_cast<i16>(this->actX),
            static_cast<i16>(this->actY),
            this->picture->width,
            this->picture->height,
            this->frameFlags | Painter::SmearUnder
        );
        return;
    }

    if ((this->frameFlags & ActState::Signal) == ActState::Signal)
    {
        painter.Options().alpha = this->mainAct.signal;
    }

    painter.DrawPicture(
        static_cast<i16>(this->actX),
        static_cast<i16>(this->actY),
        this->picture->width,
        this->picture->height,
        this->frameFlags
    );
}

// swd32.exe 0x00482840: one frame of the unit walking back from the target to its place (the attack act, or `id` /
// `action` when given; the stand pose in the recoil act gives its x); 1 when there is no walk (a party role without
// kWalksHome or UnkFlag8 / UnkFlag20, an enemy without UnkFlag10, a forced unit) or it arrived (the acts and the line
// cleared, kWalksHome cleared). The walk takes 0x23 line steps a frame more each frame; with UnkFlag20 none until the
// act follows up, then a sixth of the distance a frame more. Drawn over its shadow (an invisible unit smears) with
// the act's overlay.
int BattleUnit::ReturnHome(BattleServices& services, const BattleUnit& target, i32 id, i32 action)
{
    if (this->kind != 0)
    {
        if ((this->unk26CC & kWalksHome) == 0 &&
            (this->command.flags & (UnitCommand::UnkFlag8 | UnitCommand::UnkFlag20)) == 0)
        {
            return 1;
        }
    }
    else if ((this->command.flags & UnitCommand::UnkFlag10) == 0)
    {
        return 1;
    }

    if (this->unk2B00 == 1 || this->unk2B04 == 1)
    {
        return 1;
    }

    this->unk2AAC = 1;
    ActState& act = this->attackAct;
    ActState& pose = this->recoilAct;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    act.id = this->actId;
    act.action = this->kind != 0 ? kRoleHomeAction : kEnemyHomeAction;

    if (id != -1)
    {
        act.id = id;
    }

    if (action != 0)
    {
        act.action = action;
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
    pose.id = this->actId;
    pose.action = kHomePose;

    if (this->kind == 0)
    {
        if (this->unk2A8C == 1)
        {
            pose.action = kHomePose1;
        }

        if (this->unk2A8C == kSecondHomePose)
        {
            pose.action = kHomePose2;
        }
    }

    if (services.acts.Run(pose) == 0)
    {
        return 0;
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    this->actOffsetX = static_cast<u16>(pose.x);
    this->frameFlags = act.flags;
    this->unk2670 = pose.x;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
        this->unk2670 = this->picture->width - pose.x;
    }

    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    LineWalker& line = this->flight;
    line = LineWalker{};
    line.fromX = static_cast<i16>(targetX) - this->unk2670;
    line.fromY = static_cast<i16>(targetY) - act.y;
    line.toX = this->placement.x - static_cast<i16>(this->originX);
    line.toY = this->placement.y - act.y;
    ++this->throwSteps;
    int steps = kHomeSteps;

    if ((this->command.flags & UnitCommand::UnkFlag20) != 0)
    {
        steps = 0;

        if ((act.target & ActState::FollowsUp) != 0)
        {
            steps = (line.toX >= line.fromX ? line.toX - line.fromX : line.fromX - line.toX) / kHomeShare;
        }
        else
        {
            --this->throwSteps;
        }
    }

    bool arrived = false;

    for (int taken = 0; taken < this->throwSteps * steps; ++taken)
    {
        LineStep(line);

        if (line.fromX + line.x == line.toX)
        {
            arrived = true;
            break;
        }
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    painter.SetSource(this->picture->data.data());
    const bool invisible = (this->status & BattleUnit::Invisible) != 0;

    if (arrived)
    {
        painter.DrawPicture(
            line.toX,
            line.toY,
            this->picture->width,
            this->picture->height,
            invisible ? this->frameFlags | Painter::SmearUnder : this->frameFlags
        );
        this->throwSteps = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        this->unk26CC &= ~kWalksHome;
        return 1;
    }

    const int x = line.fromX + line.x;
    const int y = line.fromY + line.y;

    if (invisible)
    {
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->frameFlags | Painter::SmearUnder);
    }
    else
    {
        const int shadowHeight = this->picture->height / kShadowShare;
        DrawOptions& options = painter.Options();
        options.scaleHeight = shadowHeight;
        options.shear = this->picture->height >> kShadowShearShift;
        const int tint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
        options.tintRed = tint;
        options.tintGreen = tint;
        options.tintBlue = tint;
        this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
        painter.DrawPicture(x, y - shadowHeight + act.y, this->picture->width, this->picture->height, this->unk26A4);
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->frameFlags);
    }

    this->DrawOverlay(
        services,
        act.rotation,
        act.actionCode,
        x,
        y,
        static_cast<i16>(act.offsetX),
        static_cast<i16>(act.offsetY)
    );
    return 0;
}

// swd32.exe 0x00476250: an enemy's command taken from its object (skill skillA for choice 1, skillB for another): the
// command script and its value; a UnkFlag2 command gives its cast act, sets UnkCommandFlag8000 and command 2 (0);
// another sets unk2A8C to the choice and command 1 (1).
int BattleUnit::LoadEnemyCommand(Database& database, int choice, u32& act)
{
    this->command = UnitCommand{};
    this->unk29A4 = 0;
    const ObjectData& data = this->data->data;
    this->LoadCommand(database, choice == 1 ? data.skillA : data.skillB);
    this->unk29A4 = static_cast<u16>(this->command.value);

    if ((this->command.flags & UnitCommand::UnkFlag2) != 0)
    {
        act = this->command.castAct;
        this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
        this->SetCommand(BattleUnit::UnkCommand2);
        return 0;
    }

    this->unk2A8C = static_cast<u16>(choice);
    this->SetCommand(BattleUnit::UnkCommand1);
    return 1;
}

// swd32.exe 0x0047F710: one frame of the unit fading out to invisible (mode 0) or back in (mode 1) at its place, over
// the shadow picture's shadow, glowLevel counting up to kInvisibleFadeFrames; 0 when done: mode 0 makes the unit
// Invisible, both clear InvisibleMark and UnkMark400 (mode 1 also glowLevel)
int BattleUnit::FadeInvisible(BattleServices& services, int mode)
{
    Painter& painter = services.painter;
    this->mainAct.hold = 0;

    if (this->held == 1)
    {
        this->mainAct.hold = 1;
    }

    this->picture = services.archive.GetPicture(this->mainAct.frame, this->mainAct.picture);
    ++this->glowLevel;

    if (this->glowLevel > kInvisibleFadeFrames)
    {
        this->glowLevel = kInvisibleFadeFrames;

        if (mode == 1)
        {
            this->marks = static_cast<u16>(this->marks & ~(BattleUnit::InvisibleMark | BattleUnit::UnkMark400));
            this->glowLevel = 0;
            return 0;
        }

        if (mode == 0)
        {
            this->status |= BattleUnit::Invisible;
        }

        this->marks = static_cast<u16>(this->marks & ~(BattleUnit::InvisibleMark | BattleUnit::UnkMark400));
        return 0;
    }

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr || this->shadowPicture == nullptr)
    {
        return 1;
    }

    painter.SetSource(this->picture->data.data());
    const int shadowHeight = this->shadowPicture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->shadowPicture->height >> kShadowShearShift;
    options.tintRed = kInvisibleShadowTint;
    options.tintGreen = kInvisibleShadowTint;
    options.tintBlue = kInvisibleShadowTint;
    this->unk26A4 = (this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    const int x = this->placement.x - static_cast<i16>(this->originX);
    painter.DrawPicture(
        x - kShadowLeftShift,
        this->placement.y - shadowHeight,
        this->shadowPicture->width,
        this->shadowPicture->height,
        this->unk26A4
    );

    if (mode == 0)
    {
        painter.Options().alpha = Painter::MaxAlpha - this->glowLevel / 2;
    }
    else if (mode == 1)
    {
        painter.Options().alpha = this->glowLevel / 2;
    }

    this->unk26A4 = this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
    painter.DrawPicture(
        x,
        this->placement.y - this->mainAct.y,
        this->picture->width,
        this->picture->height,
        this->unk26A4 | Painter::Translucent
    );
    return 1;
}

// swd32.exe 0x004830A0: the proficiency counters of the two worn accessories (their unk50) stepped by `amount` up to
// their unk44 in `list`, a missing one appended; 1 when either accessory counts. The search goes on from where the
// first accessory's ended (the original keeps its node), so the second may be appended again.
int BattleUnit::StepAccessoryCounters(CounterList& list, u16 amount)
{
    int counts = 0;
    CounterNode* at = list.id != 0 ? list.next.get() : &list;

    for (const ObjectRecord& accessory : this->accessories)
    {
        const ObjectData& data = accessory.data;
        const auto id = static_cast<i16>(data.unk50);

        if (id == 0)
        {
            continue;
        }

        counts = 1;

        while (at->id != id && at->next != nullptr)
        {
            at = at->next.get();
        }

        if (at->id == id)
        {
            if (at->done != 0)
            {
                continue;
            }

            at->value = static_cast<u16>(at->value + amount);

            if (at->value >= data.unk44)
            {
                at->value = data.unk44;
            }

            at->percent = static_cast<u16>(
                crt::Ftol(static_cast<double>(at->value) / static_cast<double>(data.unk44) * kCounterPercent)
            );
            continue;
        }

        at->next = std::make_unique<CounterNode>();
        at = at->next.get();
        at->id = id;
        at->value = amount;
        at->percent = static_cast<u16>(
            crt::Ftol(static_cast<double>(amount) / static_cast<double>(data.unk44) * kCounterPercent)
        );
        ++list.id;
    }

    return counts;
}

// swd32.exe 0x0046E520: one tick of the action gauge: nothing while held, stopped (Unk4000), defeated or full
// (unk2AB0). Below gaugeFull it fills by a quarter of the agility (an eighth while Slowed; a quarter more and 1 with
// `boost`), 30% more with SpeedUp, 30% less with SpeedDown, and less by half the unk26DC percent of it and 4 while
// Invisible (0). Full, it is marked full, the main act's effect cleared, the cast act marked waiting and the facing
// of a unit with unk2B20 turned back (1).
int BattleUnit::StepGauge(BattleServices& services, int boost)
{
    if ((this->marks & BattleUnit::Held) != 0 || (this->marks & BattleUnit::Unk4000) != 0)
    {
        return 0;
    }

    if (this->defeated == 1 || this->unk2AB0 == 1)
    {
        return 0;
    }

    const u16 gauge = this->gauge;

    if (static_cast<i32>(gauge) >= services.state.gaugeFull)
    {
        this->unk2AB0 = 1;
        this->unk2AEC = 0;

        if (this->unk2B20 == 1)
        {
            this->unk2B20 = 0;

            if (this->unk2B04 == 0)
            {
                this->facing = 0;
            }
        }

        this->mainAct.effectA = 0;
        this->mainAct.effectQ = 0;
        this->castAct.waiting = 1;
        return 1;
    }

    const u32 status = this->status;
    int speed = static_cast<u16>(this->battleRecord->agility) >> kAgilityShare;

    if ((status & BattleUnit::Slowed) != 0)
    {
        speed >>= 1;
    }

    if (boost == 1)
    {
        speed += speed / kBoostShare + 1;
    }

    int extra = 0;
    int cut = 0;

    if ((status & BattleUnit::SpeedUp) != 0)
    {
        extra = kSpeedChangePercent * speed / kPercent;
    }

    if ((status & BattleUnit::SpeedDown) != 0)
    {
        cut = kSpeedChangePercent * speed / kPercent;
    }

    if ((status & BattleUnit::Invisible) != 0)
    {
        cut += this->unk26DC * speed / kPercent / 2 + kInvisibleSlowdown;
    }

    this->unk2AB0 = 0;
    this->gauge = static_cast<u16>(gauge - cut + extra + speed);
    return 0;
}

// swd32.exe 0x0047F3C0: one frame of the unit's main act and overlay fading out (tinted by glowLevel, falling by 2 a
// frame while not held); 1 once at kFadedOut (the recoil act cleared)
int BattleUnit::FadeMainOut(BattleServices& services)
{
    this->DrawMainFaded(services, this->glowLevel);

    if (this->held == 0)
    {
        this->glowLevel = static_cast<i16>(this->glowLevel - kFadeOutStep);
    }

    if (this->glowLevel > kFadedOut)
    {
        return 0;
    }

    this->glowLevel = kFadedOut;
    this->recoilAct = ActState{};
    return 1;
}

// swd32.exe 0x0047F580: one frame of the unit's main act and overlay brightening (tinted by brightLevel, rising by 1
// a frame while not held); 1 once at 0 (UnkMark200 cleared, brightLevel 1)
int BattleUnit::FadeMainIn(BattleServices& services)
{
    this->DrawMainFaded(services, this->brightLevel);

    if (this->held == 0)
    {
        ++this->brightLevel;
    }

    if (this->brightLevel < 0)
    {
        return 0;
    }

    this->marks = static_cast<u16>(this->marks & ~BattleUnit::UnkMark200);
    this->brightLevel = 1;
    return 1;
}

// The draw of FadeMainOut and FadeMainIn (inline in both): the main act's picture and its overlay additively at the
// unit's place, tinted by `tint`.
void BattleUnit::DrawMainFaded(BattleServices& services, int tint)
{
    Painter& painter = services.painter;
    ActState& act = this->mainAct;
    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    auto tintAll = [&painter, tint]
    {
        DrawOptions& options = painter.Options();
        options.tintRed = tint;
        options.tintGreen = tint;
        options.tintBlue = tint;
    };

    const int x = this->placement.x - static_cast<i16>(this->originX);
    const int y = this->placement.y - act.y;

    // A missing picture is read through a null pointer in the original.
    if (this->picture != nullptr)
    {
        painter.SetSource(this->picture->data.data());
        tintAll();
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->drawFlags | Painter::Additive);
    }

    if (act.rotation == 0 || act.actionCode == 0)
    {
        return;
    }

    tintAll();
    this->unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

    if (this->unk2554 == nullptr)
    {
        return;
    }

    painter.SetSource(this->unk2554->data.data());
    painter.DrawPicture(
        x + static_cast<i16>(act.offsetX),
        y + static_cast<i16>(act.offsetY),
        this->unk2554->width,
        this->unk2554->height,
        this->drawFlags | Painter::Additive
    );
}

// swd32.exe 0x00480EE0: the unit's defence: a party role's from its battle record (+30% DefenseUp, -30% DefenseDown,
// +70% Barrier), an enemy's from its object (no barrier)
int BattleUnit::Defense() const
{
    const u32 status = this->status;
    const int base = this->kind == 1 ? this->battleRecord->defenseBonus : this->data->data.defense;
    int up = 0;
    int down = 0;
    int barrier = 0;

    if ((status & BattleUnit::DefenseUp) != 0)
    {
        up = kDefenseChangePercent * base / kPercent;
    }

    if ((status & BattleUnit::DefenseDown) != 0)
    {
        down = kDefenseChangePercent * base / kPercent;
    }

    if (this->kind == 1 && (status & BattleUnit::Barrier) != 0)
    {
        barrier = kBarrierPercent * base / kPercent;
    }

    return up + barrier + base - down;
}

// swd32.exe 0x00479850: one frame of the unit's defeat while `dying`; 1 once it is over (the cast act cleared, the
// turn reset, the unit defeated). A party role or a unit with unk2AF8 (not forced) ends at once: a role is marked
// Defeated in its record and its effects expire. Others play the defeat pose (the fixed pose, or kParticlePose for
// an UnkKind20 object) on the picture of the cast act's action kDefeatAction, glowLevel counting the frames.
int BattleUnit::DefeatFrame(BattleServices& services)
{
    if (this->dying == 0)
    {
        return 0;
    }

    this->unk2AAC = 1;
    this->defeated = 1;
    ActState& act = this->castAct;
    act.id = this->actId;
    act.action = kDefeatAction;

    if ((this->kind == 1 || this->unk2AF8 == 1) && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        this->glowLevel = kDefeatedAtOnce;
        this->defeatPose = 0;

        if (this->kind == 1)
        {
            this->roleRecord->unk24 = static_cast<u16>(this->roleRecord->unk24 | RoleRecord::Defeated);
        }

        this->mainAct.effectA = 0;
        this->mainAct.effectQ = 0;
        this->glowLevel = 0;
        this->gauge = 0;
        act = ActState{};
        this->ResetTurn(services);
        this->unk2AAC = 0;
        this->dying = 0;
        this->defeated = 1;
        this->ExpireEffects(services, 1);
        return 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 1;
    }

    this->shadowPicture = services.archive.GetPicture(act.frame, 0);
    int offsetX = act.x;

    if (this->unk2B20 == 1)
    {
        this->facing = 0;
    }

    // A missing picture is read through a null pointer in the original.
    if (this->shadowPicture == nullptr)
    {
        return 1;
    }

    const Picture& picture = *this->shadowPicture;

    if (this->facing == 1 && act.x != 0)
    {
        offsetX = picture.width - act.x;
    }

    this->drawFlags &= Painter::RleSource | Painter::Mirror | Painter::Flip;

    if ((this->data->data.kind & ObjectData::UnkKind20) != 0)
    {
        this->defeatPose = kParticlePose;
    }

    if (this->fixedPose != 0)
    {
        this->defeatPose = this->fixedPose;
    }

    const int x = this->placement.x - offsetX;
    const int y = this->placement.y - act.y;

    switch (this->defeatPose)
    {
        case kRisingPose:
            return this->DefeatRising(services, picture, x, y);

        case kBurstPose:
        case kSweepPose:
            return this->DefeatBurst(services, picture, x, y);

        case kSplitRowsPose:
            return this->DefeatSplitRows(services, picture, x, y);

        case kSplitColumnsPose:
            return this->DefeatSplitColumns(services, picture, x, y);

        case kFillPose:
        case kBlueFillPose:
            return this->DefeatFill(services, picture, x, y);

        case kFadePose:
            return this->DefeatFadeOut(services, picture);

        case kQuartersPose:
            return this->DefeatQuarters(services, picture, x, y);

        case kWavePose:
            return this->DefeatWave(services, picture);

        case kSinkPose:
            return this->DefeatSink(services, picture);

        case kZoomPose:
            return this->DefeatZoom(services, picture);

        case kStripsPose:
            return this->DefeatStrips(services, picture, x, y);

        case kHalvesPose:
            return this->DefeatHalves(services, picture, x, y);

        case kSlidePose:
            return this->DefeatSlide(services, picture);

        case kHopPose:
            return this->DefeatHop(services, picture, x, y);

        case kProjectedPose:
            return this->DefeatProjected(services, picture);

        case kParticlePose:
            return this->DefeatParticles(services, picture, x, y);

        default:
            return 0;
    }
}

// The end of a defeat pose (inline in DefeatFrame): the gauge and the cast act cleared, the turn reset, the unit
// defeated.
int BattleUnit::EndDefeat(BattleServices& services)
{
    this->gauge = 0;
    this->castAct = ActState{};
    this->ResetTurn(services);
    this->unk2AAC = 0;
    this->defeated = 1;
    this->dying = 0;
    return 1;
}

// The end of a particle pose: the decoded pixels freed and the effect cleared as well.
int BattleUnit::EndParticleDefeat(BattleServices& services)
{
    this->glowLevel = 0;
    this->absorbPixels.clear();
    this->absorb = ParticleEffect{};
    return this->EndDefeat(services);
}

// The defeat sound at the first frame of a pose, then the picture as the source.
void BattleUnit::StartDefeatDraw(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel == 0)
    {
        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
    }

    services.painter.SetSource(picture.data.data());
}

// One of the split halves of a defeat pose drawn additively within (left, top, right, bottom), tinted by -glowLevel.
void BattleUnit::DrawDefeatPiece(
    BattleServices& services,
    const Picture& picture,
    int left,
    int top,
    int right,
    int bottom,
    int x,
    int y,
    int height
)
{
    Painter& painter = services.painter;
    painter.SetClipClamped(left, top, right, bottom);
    DrawOptions& options = painter.Options();
    options.tintRed = -this->glowLevel;
    options.tintGreen = -this->glowLevel;
    options.tintBlue = -this->glowLevel;
    painter.DrawPicture(x, y, picture.width, height, this->drawFlags | Painter::Additive);
}

// Pose 1: the picture rises by glowLevel (0x1F more a frame up to 0x1E0), glowing, scaled up by glowLevel rows.
int BattleUnit::DefeatRising(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel > kRisingEnd)
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    Painter& painter = services.painter;
    const int tint = 2 * (this->glowLevel / kRisingTintDivisor);
    DrawOptions& options = painter.Options();
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    options.scaleHeight = this->glowLevel + picture.height;
    this->drawFlags |= Painter::ScaledGlow;
    painter
        .DrawPicture(x, y - kRisingTintLift * tint - this->glowLevel, picture.width, picture.height, this->drawFlags);
    this->glowLevel = static_cast<i16>(this->glowLevel + kRisingStep);
    return 0;
}

// Poses 2 and 8: the picture breaks into particles, into its middle (2, with the cast act's sound) or off the side
// the unit faces (8); 1 once they are gone.
int BattleUnit::DefeatBurst(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel == kParticlesDone)
    {
        return this->EndParticleDefeat(services);
    }

    if (this->absorbPixels.empty())
    {
        ParticleEffect& effect = this->absorb;
        effect = ParticleEffect{};
        this->DecodeForParticles(services, picture);
        effect.width = picture.width;
        effect.height = picture.height;
        effect.x = x;
        effect.y = y;
        effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mixed);
        effect.bursts = 1;
        effect.divisor = kBurstDivisor;

        if (this->defeatPose == kBurstPose)
        {
            effect.targetLeft = x + (picture.width >> 1);
            effect.targetTop = y + (picture.height >> 1);
            effect.targetWidth = kBurstTarget;
            effect.targetHeight = kBurstTarget;
            effect.speed = kBurstSpeed;
            effect.lifeBase = kBurstLife;
        }
        else
        {
            effect.targetLeft = this->facing == 1 ? kSweepRight : kSweepLeft;
            effect.targetTop = y - kSweepAbove;
            effect.targetWidth = kBurstTarget;
            effect.targetHeight = picture.height + kSweepAbove;
            effect.speed = kSweepSpeed;
            effect.lifeBase = kSweepLife;
        }

        if (this->Mirrored() == 1)
        {
            effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mirrored);
        }

        services.state.projector.SetScreen(Display::Width, Display::Height);

        if (this->defeatPose == kBurstPose)
        {
            this->castAct.view = kDefeatSound;
        }

        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
        this->glowLevel = 1;
    }

    return this->RunDefeatParticles(services);
}

// The explosion of a particle pose; glowLevel kParticlesDone once it is over.
int BattleUnit::RunDefeatParticles(BattleServices& services)
{
    if (services.state.projector.DrawExplosion(services.screen, services.painter, services.crtRandom, this->absorb) ==
        1)
    {
        this->glowLevel = kParticlesDone;
    }

    return 0;
}

// The picture decoded into absorbPixels for a particle pose.
void BattleUnit::DecodeForParticles(BattleServices& services, const Picture& picture)
{
    this->absorbPixels.clear();
    const std::optional<RawPicture> raw = RleCodec(services.painter).Decode(picture.data);

    if (raw.has_value())
    {
        this->absorbPixels.resize(raw->pixels.size() / sizeof(u16));
        std::memcpy(this->absorbPixels.data(), raw->pixels.data(), this->absorbPixels.size() * sizeof(u16));
    }

    this->absorb.pixels = this->absorbPixels;
}

// Pose 3: the left half rises and the right half sinks by 2 a frame, darkening, up to 0x20.
int BattleUnit::DefeatSplitRows(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel > kSplitEnd)
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    const int shift = 2 * this->glowLevel;
    const int half = picture.width >> 1;
    this->DrawDefeatPiece(
        services,
        picture,
        x,
        y - shift,
        x + half,
        y + picture.height - shift,
        x,
        y - shift,
        picture.height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x + half,
        y + shift,
        x + picture.width,
        y + shift + picture.height,
        x,
        y + shift,
        picture.height
    );
    this->glowLevel = static_cast<i16>(this->glowLevel + kSplitStep);
    services.painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// Pose 4: the top half slides left and the bottom half right by 2 a frame, darkening, up to 0x20.
int BattleUnit::DefeatSplitColumns(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel > kSplitEnd)
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    const int shift = 2 * this->glowLevel;
    const int half = picture.height >> 1;
    this->DrawDefeatPiece(
        services,
        picture,
        x - shift,
        y,
        x + picture.width - shift,
        y + half,
        x - shift,
        y,
        picture.height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x + shift,
        y + half,
        x + shift + picture.width,
        y + picture.height,
        x + shift,
        y,
        picture.height
    );
    this->glowLevel = static_cast<i16>(this->glowLevel + kSplitStep);
    services.painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// Poses 5 and 10: a tint fills the picture from the top (5: dark, 0xE rows a frame; 10: blue, 4 rows a frame); once
// full the unit goes on with pose 2.
int BattleUnit::DefeatFill(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel >= picture.height)
    {
        this->defeatPose = kBurstPose;
        this->glowLevel = 0;
        return 0;
    }

    this->StartDefeatDraw(services, picture);
    Painter& painter = services.painter;
    painter.DrawPicture(x, y, picture.width, picture.height, this->drawFlags);

    if (this->glowLevel > picture.height)
    {
        this->glowLevel = static_cast<i16>(picture.height - 1);
    }

    DrawOptions& options = painter.Options();
    const bool blue = this->defeatPose == kBlueFillPose;
    options.tintRed = blue ? 0 : kDarkFillTint;
    options.tintGreen = blue ? 0 : kDarkFillTint;
    options.tintBlue = blue ? kBlueFillTint : kDarkFillTint;
    painter.DrawPicture(
        x,
        y,
        picture.width,
        blue ? this->glowLevel + 1 : this->glowLevel,
        this->drawFlags | Painter::TintUnder
    );
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->glowLevel = static_cast<i16>(this->glowLevel + (blue ? kBlueFillStep : kDarkFillStep));
    return 0;
}

// Pose 6: the picture darkens by one tint step a frame at the unit's place down to -0x20.
int BattleUnit::DefeatFadeOut(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel <= kFadedOut)
    {
        this->glowLevel = 0;
        this->slotTint = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    Painter& painter = services.painter;
    DrawOptions& options = painter.Options();
    options.tintRed = this->glowLevel;
    options.tintGreen = this->glowLevel;
    options.tintBlue = this->glowLevel;
    painter.DrawPicture(
        this->placement.x - static_cast<i16>(this->originX),
        this->placement.y - this->mainAct.y,
        picture.width,
        picture.height,
        this->drawFlags | Painter::Additive
    );
    --this->glowLevel;
    return 0;
}

// Pose 7: the four quarters fly apart diagonally by 1 a frame, darkening, up to 0x20 (the top left one drawn half
// high, the bottom right one 0x10 lower: as in the original).
int BattleUnit::DefeatQuarters(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel >= kSplitEnd)
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    const int shift = this->glowLevel;
    const int halfWidth = picture.width >> 1;
    const int halfHeight = picture.height >> 1;
    this->DrawDefeatPiece(
        services,
        picture,
        x - shift,
        y - shift,
        x + halfWidth - shift,
        y + halfHeight - shift,
        x - shift,
        y - shift,
        halfHeight
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x - shift,
        y + halfHeight + shift,
        x + halfWidth - shift,
        y + picture.height + shift,
        x - shift,
        y + shift,
        picture.height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x + halfWidth + shift,
        y - shift,
        x + picture.width + shift,
        y + halfHeight - shift,
        x + shift,
        y - shift,
        picture.height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x + halfWidth + shift,
        y + halfHeight + shift,
        x + picture.width + shift,
        y + picture.height + shift,
        x + shift,
        y + shift + kQuarterDrop,
        picture.height
    );
    ++this->glowLevel;
    services.painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// Pose 9: the picture waves away (wave row glowLevel / 3 + 1), more transparent each frame, up to 0x2D frames.
int BattleUnit::DefeatWave(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel >= kWaveEnd)
    {
        this->glowLevel = 0;
        this->slotTint = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    Painter& painter = services.painter;
    const int wave = this->glowLevel / kWaveShare + 1;
    painter.SetWave(wave);
    painter.Options().alpha = Painter::MaxAlpha - wave;
    painter.DrawPicture(
        this->placement.x - static_cast<i16>(this->originX),
        this->placement.y - this->mainAct.y,
        picture.width,
        picture.height,
        this->drawFlags | Painter::Translucent
    );
    painter.Options().alpha = 0;
    painter.SetWave(0);
    ++this->glowLevel;
    return 0;
}

// Pose 11: the picture sinks by 10 a frame, darkening, under its fading translucent copy, down to -0x20.
int BattleUnit::DefeatSink(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel <= kFadedOut)
    {
        this->glowLevel = 0;
        this->slotTint = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    Painter& painter = services.painter;
    const int x = this->placement.x - static_cast<i16>(this->originX);
    const int y = this->placement.y - this->mainAct.y;
    painter.Options().alpha = Painter::MaxAlpha;

    if (this->glowLevel <= kSinkFadeFrom)
    {
        painter.Options().alpha = this->glowLevel + kSinkAlphaBase;
    }

    painter.DrawPicture(x, y, picture.width, picture.height, this->drawFlags | Painter::Translucent);
    painter.Options().alpha = 0;
    DrawOptions& options = painter.Options();
    options.tintRed = this->glowLevel;
    options.tintGreen = this->glowLevel;
    options.tintBlue = this->glowLevel;
    painter.DrawPicture(
        x,
        y + kSinkStep * this->glowLevel,
        picture.width,
        picture.height,
        this->drawFlags | Painter::Additive
    );
    --this->glowLevel;
    return 0;
}

// Pose 12: the picture zooms about the main act's position, wider and flatter each frame, for 0x1E frames.
int BattleUnit::DefeatZoom(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel >= kZoomEnd)
    {
        this->glowLevel = 0;
        this->slotTint = 0;
        return this->EndDefeat(services);
    }

    if (this->glowLevel == 0)
    {
        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
    }

    Painter& painter = services.painter;
    painter.SetZoom(
        this->mainAct.x,
        this->mainAct.y,
        this->glowLevel * this->slotTint + kZoomUnit,
        kZoomUnit - this->glowLevel * (this->slotTint + 1)
    );
    painter.SetSource(picture.data.data());
    const int x = this->placement.x - static_cast<i16>(this->originX);
    const int y = this->placement.y - this->mainAct.y;

    if ((this->drawFlags & Painter::Mirror) != 0)
    {
        painter.DrawZoomedMirrored(x, y, picture.width, picture.height);
    }
    else
    {
        painter.DrawZoomed(x, y, picture.width, picture.height);
    }

    this->slotTint = static_cast<i16>(this->slotTint + kZoomStep);
    ++this->glowLevel;
    return 0;
}

// Pose 13: four strips of a quarter of the height slide apart, alternately left and right, by 2 a frame, darkening.
int BattleUnit::DefeatStrips(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel > kSplitEnd)
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    const int shift = 2 * this->glowLevel;
    const int strip = picture.height >> 2;
    const int height = picture.height;
    this->DrawDefeatPiece(services, picture, x - shift, y, x + picture.width - shift, y + strip, x - shift, y, height);
    this->DrawDefeatPiece(
        services,
        picture,
        x + shift,
        y + strip,
        x + shift + picture.width,
        y + 2 * strip,
        x + shift,
        y,
        height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x - shift,
        y + 2 * strip,
        x + picture.width - shift,
        y + 3 * strip,
        x - shift,
        y,
        height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x + shift,
        y + 3 * strip,
        x + shift + picture.width,
        y + height,
        x + shift,
        y,
        height
    );
    this->glowLevel = static_cast<i16>(this->glowLevel + kSplitStep);
    services.painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// Pose 14: the halves slide apart like pose 4, fast (4 a frame) to 8, then waiting 8 frames, then 1 a frame to 0x20.
int BattleUnit::DefeatHalves(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel > kSplitEnd)
    {
        this->glowLevel = 0;
        this->slotTint = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    const int shift = 2 * this->glowLevel;
    const int half = picture.height >> 1;
    this->DrawDefeatPiece(
        services,
        picture,
        x - shift,
        y,
        x + picture.width - shift,
        y + half,
        x - shift,
        y,
        picture.height
    );
    this->DrawDefeatPiece(
        services,
        picture,
        x + shift,
        y + half,
        x + shift + picture.width,
        y + picture.height,
        x + shift,
        y,
        picture.height
    );

    if (this->glowLevel >= kHalvesSlow)
    {
        ++this->glowLevel;
    }
    else if (this->glowLevel == kHalvesPause)
    {
        ++this->slotTint;

        if (this->slotTint == kHalvesPause)
        {
            this->glowLevel = kHalvesSlow;
        }
    }
    else
    {
        this->glowLevel = static_cast<i16>(this->glowLevel + kHalvesFast);
    }

    services.painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// Pose 15: the picture slides right by 2 a frame, darkening, beside its fading translucent copy, down to -0x20.
int BattleUnit::DefeatSlide(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel <= kFadedOut)
    {
        this->glowLevel = 0;
        this->slotTint = 0;
        return this->EndDefeat(services);
    }

    this->StartDefeatDraw(services, picture);
    Painter& painter = services.painter;
    const int x = this->placement.x - static_cast<i16>(this->originX);
    const int y = this->placement.y - this->mainAct.y;

    if (this->glowLevel >= kSlideFadeFrom)
    {
        painter.Options().alpha = this->glowLevel + Painter::MaxAlpha;
    }

    painter.DrawPicture(x, y, picture.width, picture.height, this->drawFlags | Painter::Translucent);
    painter.Options().alpha = 0;
    DrawOptions& options = painter.Options();
    options.tintRed = this->glowLevel;
    options.tintGreen = this->glowLevel;
    options.tintBlue = this->glowLevel;
    painter.DrawPicture(x + 2 * this->glowLevel, y, picture.width, picture.height, this->drawFlags | Painter::Additive);
    --this->glowLevel;
    return 0;
}

// Pose 50: the picture hops (BattleUnit::HopHeights) upside down, more transparent each frame, for 15 frames; the
// cast act's sound plays first (the stand act's sound set to kDefeatSound).
int BattleUnit::DefeatHop(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel >= static_cast<i16>(BattleUnit::HopHeights.size()))
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    if (this->glowLevel == 0)
    {
        const u16 sound = this->castAct.view;
        this->standAct.view = kDefeatSound;
        services.audio.PlaySound(sound, services.config.soundVolume);
    }

    Painter& painter = services.painter;
    painter.SetSource(picture.data.data());
    painter.Options().alpha = Painter::MaxAlpha - this->glowLevel;
    painter.DrawPicture(
        x,
        y - BattleUnit::HopHeights[static_cast<std::size_t>(this->glowLevel)],
        picture.width,
        picture.height,
        this->drawFlags | Painter::Translucent | Painter::Flip
    );
    ++this->glowLevel;
    return 0;
}

// Pose 51: the picture projected (DrawScaled) at the unit's place, growing by 4 in both scales and turned, every
// other frame number up to 100; the cast act's sound plays first.
int BattleUnit::DefeatProjected(BattleServices& services, const Picture& picture)
{
    if (this->glowLevel >= kParticlesDone)
    {
        this->glowLevel = 0;
        return this->EndDefeat(services);
    }

    ProjectedPicture& projected = this->projected;

    if (this->glowLevel == 0)
    {
        projected.scaleX = kZoomUnit;
        projected.scaleY = kZoomUnit;
        projected.unk08 = picture.width >> 1;
        projected.unk0C = picture.height;
        projected.width = picture.width;
        projected.height = picture.height;
        projected.flags = ProjectedPicture::Mixed;
        projected.x = this->placement.x - (picture.width >> 1);
        projected.y = this->placement.y;

        if (this->Mirrored() == 1)
        {
            projected.flags = static_cast<u16>(projected.flags | ProjectedPicture::Mirrored);
        }

        projected.angle = kProjectedAngle;
        this->projectedPixels.clear();
        const std::optional<RawPicture> raw = RleCodec(services.painter).Decode(picture.data);

        if (raw.has_value())
        {
            this->projectedPixels.resize(raw->pixels.size() / sizeof(u16));
            std::memcpy(this->projectedPixels.data(), raw->pixels.data(), this->projectedPixels.size() * sizeof(u16));
        }

        projected.pixels = this->projectedPixels;
        const u16 sound = this->castAct.view;
        this->standAct.view = kDefeatSound;
        services.audio.PlaySound(sound, services.config.soundVolume);
    }

    this->glowLevel = static_cast<i16>(this->glowLevel + kProjectedStep);
    projected.tintRed = kProjectedTint;
    projected.tintGreen = kProjectedTint;
    projected.tintBlue = kProjectedTint;
    projected.scaleX += kProjectedGrowth;
    projected.scaleY += kProjectedGrowth;
    services.state.projector.DrawScaled(services.screen, services.painter, projected);
    return 0;
}

// Pose 100: the unit wobbles (recoilTint, flickering up by 4 and down by 1, kWobbleSound at its top) six times, then
// shakes while it breaks into particles that fly off a strip across the field.
int BattleUnit::DefeatParticles(BattleServices& services, const Picture& picture, int x, int y)
{
    if (this->glowLevel == kParticlesDone)
    {
        return this->EndParticleDefeat(services);
    }

    if (this->glowLevel == 0)
    {
        if (this->throwSteps >= kWobbles)
        {
            this->recoilTint = 0;
            this->glowLevel = 1;
            this->throwSteps = 0;
            return 0;
        }

        if (this->recoilTint == kWobbleTop)
        {
            services.audio.PlaySound(kWobbleSound, services.config.soundVolume);
        }

        Painter& painter = services.painter;
        painter.SetSource(picture.data.data());
        DrawOptions& options = painter.Options();
        options.tintRed = this->recoilTint;
        options.tintGreen = this->recoilTint;
        options.tintBlue = this->recoilTint;
        painter.DrawPicture(
            this->placement.x - static_cast<i16>(this->originX),
            this->placement.y - this->mainAct.y,
            picture.width,
            picture.height,
            this->drawFlags | Painter::Additive
        );
        const i32 wobble = this->throwSteps;

        if (wobble % 2 != 0)
        {
            --this->recoilTint;
        }
        else
        {
            this->recoilTint = static_cast<i16>(this->recoilTint + kWobbleRise);
        }

        if (this->recoilTint >= kWobbleTop || this->recoilTint <= kWobbleBottom)
        {
            this->throwSteps = wobble + 1;
        }

        return 0;
    }

    if (this->absorbPixels.empty())
    {
        ParticleEffect& effect = this->absorb;
        effect = ParticleEffect{};
        this->DecodeForParticles(services, picture);
        effect.width = picture.width;
        effect.height = picture.height;
        effect.x = x;
        effect.y = y;
        effect.targetLeft = this->facing == 1 ? kStripRight : 0;
        effect.targetTop = 1;
        effect.targetWidth = kStripWidth;
        effect.targetHeight = 1;
        effect.speed = kStripSpeed;
        effect.lifeBase = kStripLife;
        effect.bursts = static_cast<u16>(picture.height >> 2);
        effect.divisor = kStripDivisor;
        effect.flags = kStripFlags;

        if (this->Mirrored() == 1)
        {
            effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mirrored);
        }

        effect.tintRed = kStripTint;
        effect.tintGreen = kStripTint;
        effect.tintBlue = kStripTint;
        services.state.projector.SetScreen(Display::Width, Display::Height);
        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
        this->glowLevel = kParticlesStarted;
    }

    ++this->glowLevel;
    this->absorb.x = x + this->glowLevel % kShakeSpan;
    return this->RunDefeatParticles(services);
}

// swd32.exe 0x00482310: one frame of the unit walking from its place to the target (the attack act's walk action, the
// stand pose in the recoil act giving its x): kWalkSteps line steps a frame more each frame; with UnkFlag20 none
// until the act follows up, then a sixteenth of the distance a frame more. A forced fading command (FadesIn) takes its
// command B at once; an invisible unit first fades out. Drawn over its shadow (an invisible unit smears; at the end it
// is drawn actOffsetX further left, as in the original). At the target the acts and the line are cleared and command A
// becomes command B.
void BattleUnit::WalkToTarget(BattleServices& services, const BattleUnit& target)
{
    if (this->unk2AD8 == 1)
    {
        return;
    }

    if (this->unk2B00 == 1 && (this->command.flags & UnitCommand::FadesIn) != 0)
    {
        this->commandA = this->commandB;
        return;
    }

    if ((this->status & BattleUnit::Invisible) != 0 && this->FadeInvisible(services, 0) == 1)
    {
        return;
    }

    this->unk2AAC = 1;
    ActState& act = this->attackAct;
    ActState& pose = this->recoilAct;
    act.id = this->actId;
    act.action = this->kind != 0 ? kRoleWalkAction : kEnemyHomeAction;
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
    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    pose.id = this->actId;
    pose.action = kHomePose;

    if (this->kind == 0)
    {
        if (this->unk2A8C == 1)
        {
            pose.action = kHomePose1;
        }

        if (this->unk2A8C == kSecondHomePose)
        {
            pose.action = kHomePose2;
        }
    }

    if (services.acts.Run(pose) == 0)
    {
        return;
    }

    this->actOffsetX = static_cast<u16>(act.x);
    this->frameFlags = act.flags;
    this->unk2670 = pose.x;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;
        this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(act.x));
        this->unk2670 = this->picture->width - pose.x;
    }

    const int toX = targetX - this->unk2670;
    const int toY = targetY - act.y;

    if (this->kind == 1 && this->unk2A8E != act.picture)
    {
        this->unk2A8E = act.picture;
    }

    ++this->throwSteps;
    LineWalker& line = this->flight;
    line = LineWalker{};
    line.fromX = this->placement.x - static_cast<i16>(this->actOffsetX);
    line.fromY = this->placement.y - act.y;
    line.toX = toX;
    line.toY = toY;
    int steps = kWalkSteps;

    if ((this->command.flags & UnitCommand::UnkFlag20) != 0)
    {
        steps = 0;

        if ((act.target & ActState::FollowsUp) != 0)
        {
            steps = (toX >= line.fromX ? toX - line.fromX : line.fromX - toX) / kWalkShare;
        }
        else
        {
            --this->throwSteps;
        }
    }

    bool arrived = false;

    for (int taken = 0; taken < steps * this->throwSteps; ++taken)
    {
        LineStep(line);

        if (line.fromX + line.x == line.toX)
        {
            arrived = true;
            break;
        }
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (this->status & BattleUnit::Invisible) != 0;

    if (arrived)
    {
        if (invisible)
        {
            painter.DrawPicture(
                line.toX - static_cast<i16>(this->actOffsetX),
                line.toY,
                this->picture->width,
                this->picture->height,
                this->frameFlags | Painter::SmearUnder
            );
        }
        else
        {
            painter.DrawPicture(line.toX, line.toY, this->picture->width, this->picture->height, this->frameFlags);
        }

        this->throwSteps = 0;
        this->unk2A8E = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        this->commandA = this->commandB;
        return;
    }

    painter.SetSource(this->picture->data.data());
    const int x = line.fromX + line.x;
    const int y = line.fromY + line.y;

    if (invisible)
    {
        painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->frameFlags | Painter::SmearUnder);
        return;
    }

    const int shadowHeight = this->picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = this->picture->height >> kShadowShearShift;
    const int tint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.DrawPicture(x, y + act.y - shadowHeight, this->picture->width, this->picture->height, this->unk26A4);
    painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->frameFlags);
}

// swd32.exe 0x00480AE0: the main act's effect while the unit idles: when the main act recoils (Recoils), its effectA
// plays in the cast act (holding the main act with Holds; the command's follow-up act waits for it) above the main
// act's anchor, the sound panned to the unit's side; when it waits, the effect and the holds are cleared.
void BattleUnit::PlayIdleEffect(BattleServices& services)
{
    ActState& main = this->mainAct;
    ActState& effect = this->castAct;
    const u16 bits = main.target;

    if ((bits & ActState::Recoils) != 0)
    {
        if (main.effectA != 0)
        {
            this->unk267C |= BattleUnit::Recoiling;
        }

        if (this->command.followAct != 0)
        {
            this->unk267C |= BattleUnit::FollowingUp;
        }

        if ((bits & ActState::Holds) != 0)
        {
            main.hold = 1;
        }

        main.target = static_cast<u16>(bits & ~ActState::Recoils);
        effect = ActState{};
    }

    if ((this->unk267C & BattleUnit::Recoiling) == 0)
    {
        return;
    }

    effect.action = main.effectQ;
    effect.id = main.effectA;

    if (services.acts.Run(effect) == 0)
    {
        return;
    }

    services.audio.PlaySound(effect.view, services.config.soundVolume);

    if (effect.view != 0)
    {
        services.audio.SetSoundPan(effect.view, this->placement.x >= kPanCentre ? kFadePan : -kFadePan);
    }

    effect.view = 0;
    this->picture = services.archive.GetPicture(effect.frame, effect.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    u32 flags = effect.flags;
    int offsetX = effect.x;

    if (this->facing == 1)
    {
        flags ^= Painter::Mirror;

        if (offsetX != 0)
        {
            offsetX = this->picture->width - offsetX;
        }
    }

    painter.DrawPicture(
        static_cast<i16>(this->actAnchorX) + this->placement.x - static_cast<i16>(this->originX) - offsetX,
        static_cast<i16>(main.anchorY) + this->placement.y - effect.y - main.y,
        this->picture->width,
        this->picture->height,
        flags
    );

    if (effect.waiting != 1)
    {
        return;
    }

    main.target = static_cast<u16>(main.target & ~ActState::Holds);
    this->unk267C &= ~BattleUnit::Recoiling;
    this->slotTint = 0;
    this->actAnchorX = 0;
    this->unk2ACC = 0;
    main.hold = 0;
    main.effectA = 0;
    effect = ActState{};
}

// swd32.exe 0x00480D40: the command's follow-up act (in the attack act) while the unit idles and FollowingUp is set,
// at the unit's place, the sound panned to its side; when it waits, every stage and the main act's effect are cleared.
void BattleUnit::PlayIdleFollowUp(BattleServices& services)
{
    if ((this->unk267C & BattleUnit::FollowingUp) == 0)
    {
        return;
    }

    ActState& act = this->attackAct;
    act.id = this->command.followAct;
    act.action = 0;

    if (services.acts.Run(act) == 0)
    {
        return;
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);

    if (act.view != 0)
    {
        services.audio.SetSoundPan(act.view, this->placement.x >= kPanCentre ? kFadePan : -kFadePan);
    }

    act.view = 0;
    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return;
    }

    Painter& painter = services.painter;
    painter.SetSource(this->picture->data.data());
    u32 flags = act.flags;
    int offsetX = act.x;

    if (this->facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = this->picture->width - offsetX;
    }

    painter.DrawPicture(
        this->placement.x - offsetX,
        this->placement.y - act.y,
        this->picture->width,
        this->picture->height,
        flags
    );

    if (act.waiting != 1)
    {
        return;
    }

    this->unk267C = 0;
    this->slotTint = 0;
    this->mainAct.target = 0;
    this->actAnchorX = 0;
    this->unk2ACC = 0;
    this->mainAct.hold = 0;
    this->mainAct.effectA = 0;
    act = ActState{};
}

// swd32.exe 0x0047DAD0: one tick of the unit's effects (not while held): each timer rises by kEffectTick; a full one
// (gaugeFull) restarts and takes a turn off. A poison (UnkStatus1000) sets the pending damage when none is pending,
// scaled by the element 5 resistance of the battle record and halved by an effect of kind 5; a stamina drain
// (kStaminaDrainBits) takes 10 stamina a turn, ending when less is left. Then the finished effects expire.
void BattleUnit::TickEffects(BattleServices& services)
{
    if (this->effects.next == nullptr || this->held != 0)
    {
        return;
    }

    for (EffectNode* node = this->effects.next.get(); node != nullptr; node = node->next.get())
    {
        node->timer = static_cast<u16>(node->timer + kEffectTick);

        if (static_cast<i32>(node->timer) < services.state.gaugeFull)
        {
            continue;
        }

        if ((node->bits & BattleUnit::UnkStatus1000) != 0 && this->pendingDamage == 0)
        {
            auto damage = static_cast<i16>(node->value);
            ScaleByTenths(static_cast<i8>(this->battleRecord->unk2D[kPoisonResistance]), damage);

            if (this->HasEffectKind(kPoisonKind) == 1)
            {
                damage = static_cast<i16>(damage / 2);
            }

            this->pendingDamage = static_cast<u16>(damage);
        }

        node->timer = 0;

        if (node->turns == 0)
        {
            continue;
        }

        --node->turns;

        if (node->bits != kStaminaDrainBits)
        {
            continue;
        }

        if (this->roleRecord->stamina < kStaminaDrain)
        {
            node->turns = 0;
        }
        else
        {
            this->Damage(services, 0, 0, -kStaminaDrain);
        }
    }

    this->ExpireEffects(services, 0);
}

// swd32.exe 0x00482FC0: whether the accessory command of `kind` can be taken: not when it costs more stamina than the
// role has; Unk1C not while Invisible, Unk1D not with its bit, Unk20 not under a Barrier, SpiritFan only with the
// equipment; Unk19 sets its unk26CC bit.
int BattleUnit::CanUseAccessory(u16 kind)
{
    if (static_cast<i32>(this->AccessoryValue(kind)) > this->roleRecord->stamina)
    {
        return 0;
    }

    switch (kind)
    {
        case kAccessory19:
            this->unk26CC |= kAccessory19Bit;
            return 1;

        case kAccessory1C:
            return (this->status & BattleUnit::Invisible) != 0 ? 0 : 1;

        case kAccessory1D:
            return (this->unk26CC & kAccessory1DBit) != 0 ? 0 : 1;

        case kAccessory20:
            return (this->status & BattleUnit::Barrier) != 0 ? 0 : 1;

        case kSpiritFanKind:
            return (this->equipmentFlags & BattleUnit::SpiritFan) != 0 ? 1 : 0;

        default:
            return 1;
    }
}

// swd32.exe 0x00484020: drain effect `slot` started: the source unit's idle picture breaks into particles flying into
// this unit (a point 0x32 left of and 0x28 below its act's anchor), bursting in half the picture's height
void BattleUnit::StartDrainEffect(BattleServices& services, int slot, BattleUnit& source)
{
    u16 sourceX = 0;
    u16 sourceY = 0;
    this->absorbSource = source.StepIdleAction(services);
    source.GetOrigin(sourceX, sourceY);
    ParticleEffect& effect = this->drains[static_cast<std::size_t>(slot)];
    std::vector<u16>& pixels = this->drainPixels[static_cast<std::size_t>(slot)];
    effect = ParticleEffect{};
    pixels.clear();

    // A source without its picture is read through a null pointer in the original.
    if (this->absorbSource == nullptr)
    {
        return;
    }

    const std::optional<RawPicture> raw = RleCodec(services.painter).Decode(this->absorbSource->data);

    if (raw.has_value())
    {
        pixels.resize(raw->pixels.size() / sizeof(u16));
        std::memcpy(pixels.data(), raw->pixels.data(), pixels.size() * sizeof(u16));
    }

    effect.pixels = pixels;
    effect.width = this->absorbSource->width;
    effect.height = this->absorbSource->height;
    effect.x = static_cast<i16>(sourceX) - 1;
    effect.y = static_cast<i16>(sourceY);
    effect.targetLeft =
        static_cast<i16>(this->actAnchorX) - static_cast<i16>(this->originX) + this->placement.x - kDrainLeft;
    effect.targetTop = this->placement.y - this->mainAct.y + kDrainDown;
    effect.targetWidth = 1;
    effect.targetHeight = 1;
    effect.speed = kDrainSpeed;
    effect.lifeBase = kDrainLife;
    effect.bursts = static_cast<u16>(this->absorbSource->height >> 1);
    effect.divisor = kDrainDivisor;
    effect.tintRed = kDrainTint;
    effect.tintGreen = kDrainTint;
    effect.tintBlue = kDrainTint;
    effect.flags = kStripFlags;

    if (source.Mirrored() == 1)
    {
        effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mirrored);
    }

    this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag800);
    services.state.projector.SetScreen(Display::Width, Display::Height);
}

// swd32.exe 0x004841B0: one frame of drain effect `slot` (absorbing set); 1 once it is gone (its pixels freed and the
// effect cleared)
int BattleUnit::DrainEffectFrame(BattleServices& services, int slot)
{
    ParticleEffect& effect = this->drains[static_cast<std::size_t>(slot)];
    effect.tintRed = kDrainTint;
    effect.tintGreen = kDrainTint;
    effect.tintBlue = kDrainTint;
    this->absorbing = 1;

    if (services.state.projector.DrawExplosion(services.screen, services.painter, services.crtRandom, effect) != 1)
    {
        return 0;
    }

    this->drainPixels[static_cast<std::size_t>(slot)].clear();
    effect = ParticleEffect{};
    return 1;
}

// swd32.exe 0x00476330: whether `level` catches the enemy (never with UnkMark800 or when its object's unk91 is 0; a
// 0..11 roll is drawn and not used): more than 10 levels above its level 8 in 10, 6..10 above 5 in 10, exactly 5
// above unk91 in 10, otherwise never
int BattleUnit::CatchRoll(BattleServices& services, u8 level)
{
    if ((this->marks & BattleUnit::UnkMark800) != 0)
    {
        return 0;
    }

    services.random.Below(kUnusedCatchRoll);
    const ObjectData& data = this->data->data;

    if (data.unk91 == 0)
    {
        return 0;
    }

    const int above = level - data.level;

    if (above > kFarAbove)
    {
        return static_cast<u16>(services.random.Below(kCatchRoll)) < kFarCatch ? 1 : 0;
    }

    if (above > kNearAbove)
    {
        return static_cast<u16>(services.random.Below(kCatchRoll)) < kNearCatch ? 1 : 0;
    }

    return above >= kNearAbove && static_cast<u16>(services.random.Below(kCatchRoll)) < data.unk91 ? 1 : 0;
}

// swd32.exe 0x0047BA80: one frame of the unit staggering from a hit (hurtStep 0..5, one step every second frame not
// held); 1 once over (hurtStep back to 1, the attack act and the targets cleared, `charging` set) or when its act
// ends. A party role takes its hurt pose (its guard pose while guarding or marked, not held; a poisoned one the
// hurt pose), others the shadow action. The picture shakes back by kStaggerShake (UnkStatus4 copies the main act and
// stays), over its shadow, with a flash; an enemy also leaves a red flash behind, lifted by kStaggerRise (up with
// unk2B10, else to the side). Its effects, overlay and a recoil effect follow.
int BattleUnit::HurtFrame(BattleServices& services)
{
    Painter& painter = services.painter;
    ActState& act = this->attackAct;
    act.id = this->actId;

    if (this->kind != 1 || this->unk2B00 != 0 || this->unk2B04 != 0)
    {
        act.action = kEnemyHomeAction;
    }
    else
    {
        act.action = kHurtAction;

        if ((this->GetGuard() == 1 || (this->marks & BattleUnit::UnkMark1) != 0) &&
            (this->marks & BattleUnit::Held) == 0)
        {
            act.action = kGuardHurtAction;
        }

        if ((this->status & BattleUnit::UnkStatus1000) != 0)
        {
            act.action = kHurtAction;
        }
    }

    act.hold = 0;

    if (this->held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 1;
    }

    if (this->unk2AF4 == 1 && this->kind == 0)
    {
        act.picture = this->mainAct.picture;
    }

    if (this->guarding == 1)
    {
        return 1;
    }

    const bool still = (this->status & BattleUnit::UnkStatus4) != 0;

    if (still)
    {
        act = this->mainAct;
    }

    this->shadowPicture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->shadowPicture == nullptr)
    {
        return 0;
    }

    const Picture& picture = *this->shadowPicture;
    painter.SetSource(picture.data.data());
    services.audio.PlaySound(act.view, services.config.soundVolume);

    if (act.view != 0)
    {
        services.audio.SetSoundPan(act.view, this->facing == 1 ? -kFadePan : kFadePan);
    }

    const std::size_t step = this->hurtStep;
    const int placeX = this->placement.x;
    this->unk2670 = act.x;
    act.view = 0;
    int y = this->placement.y - act.y;
    const int shake = BattleUnit::StaggerShake[step];
    int x = shake + placeX - act.x;

    if (this->unk2B00 == 1)
    {
        if (this->facing != 0)
        {
            x = placeX - shake - act.x;
        }
        else
        {
            this->unk2670 = picture.width - act.x;
            x = placeX + BattleUnit::StaggerShake[step] - this->unk2670;
        }
    }

    if (this->facing == 1 && this->unk2B00 == 0)
    {
        this->unk2670 = picture.width - act.x;
        x = placeX - BattleUnit::StaggerShake[step] - this->unk2670;
    }

    if (still)
    {
        x = placeX - this->unk2670;
    }

    DrawOptions& options = painter.Options();

    if (this->kind == 0 && !still)
    {
        options.tintRed = kHurtRed;
        options.tintGreen = 0;
        options.tintBlue = 0;
        this->unk26A4 = this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
        const u32 flash = this->unk26A4 | Painter::TintUnder;

        if (this->unk2B10 != 0)
        {
            painter.DrawPicture(
                placeX - this->unk2670,
                this->placement.y - BattleUnit::StaggerRise[step] - act.y,
                picture.width,
                picture.height,
                flash
            );
            x = placeX - this->unk2670;
            y = this->placement.y - BattleUnit::StaggerLift[step] - act.y;
        }
        else if (this->facing != 0)
        {
            painter.DrawPicture(
                placeX + BattleUnit::StaggerRise[step] - this->unk2670,
                this->placement.y - act.y,
                picture.width,
                picture.height,
                flash
            );
            x = placeX + BattleUnit::StaggerLift[step] - this->unk2670;
        }
        else
        {
            painter.DrawPicture(
                placeX - BattleUnit::StaggerRise[step] - this->unk2670,
                this->placement.y - act.y,
                picture.width,
                picture.height,
                flash
            );
            x = placeX - BattleUnit::StaggerLift[step] - this->unk2670;
        }
    }

    const int shadowHeight = picture.height / kShadowShare;
    options.scaleHeight = shadowHeight;
    options.shear = picture.height >> kShadowShearShift;
    const int tint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    this->unk26A4 = (this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.DrawPicture(x, this->placement.y - shadowHeight, picture.width, picture.height, this->unk26A4);
    this->drawFlags &= Painter::RleSource | Painter::Mirror | Painter::Flip;
    painter.DrawPicture(x, y, picture.width, picture.height, this->drawFlags);

    if (!still && act.action != kGuardHurtAction)
    {
        painter.Options().tintRed = kHurtFlash;
        painter.DrawPicture(x, y, picture.width, picture.height, this->drawFlags | Painter::TintUnder);
    }

    this->DrawEffects(services, x, y);

    if (!still && act.rotation != 0 && act.actionCode != 0)
    {
        this->unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

        if (this->unk2554 != nullptr)
        {
            painter.SetSource(this->unk2554->data.data());
            painter.DrawPicture(
                placeX + static_cast<i16>(act.offsetX) - this->unk2670,
                this->placement.y + static_cast<i16>(act.offsetY) - act.y,
                this->unk2554->width,
                this->unk2554->height,
                this->drawFlags | Painter::Additive
            );
        }
    }

    if ((act.target & ActState::Recoils) != 0)
    {
        ActState& effect = this->castAct;
        effect.action = act.effectQ;
        effect.id = act.effectA;

        if (services.acts.Run(effect) == 0)
        {
            return 0;
        }

        services.audio.PlaySound(effect.view, services.config.soundVolume);
        effect.view = 0;
        this->picture = services.archive.GetPicture(effect.frame, effect.picture);

        if (this->picture != nullptr)
        {
            painter.SetSource(this->picture->data.data());
            u32 flags = effect.flags;
            int offsetX = effect.x;

            if (act.anchorX != 0)
            {
                this->actAnchorX = act.anchorX;
            }

            if (this->facing == 1)
            {
                flags ^= Painter::Mirror;
                offsetX = this->picture->width - offsetX;
                this->actAnchorX = static_cast<u16>(picture.width - act.anchorX);
            }

            painter.DrawPicture(
                x + static_cast<i16>(this->actAnchorX) - offsetX,
                y + static_cast<i16>(act.anchorY) - effect.y,
                this->picture->width,
                this->picture->height,
                flags
            );
        }
    }

    if (this->held == 0)
    {
        ++this->glowPhase;

        if (this->glowPhase == kHurtFrames)
        {
            ++this->hurtStep;
            this->glowPhase = 0;
        }
    }

    this->unk2AAC = 1;

    if (this->hurtStep <= kLastHurtStep)
    {
        return 0;
    }

    services.state.unk53BF46 = 0;
    services.state.unk53BF48 = 0;
    this->hurtStep = 1;
    painter.Options().shear = 0;
    this->unk2AAC = 0;
    this->charging = 1;
    this->unk2B10 = 0;
    act = ActState{};
    this->targets.fill(BattleUnit::NoTarget);
    return 1;
}

// swd32.exe 0x00478B60: one frame of the unit at rest (`paused`: its acts hold). drawAtSaved moves it first (1 shakes
// it a pixel at random, 2 lifts it 10 a frame to the top, 3 drops it 0x1E a frame back to its saved place, marking
// flag 26B8). A staggering unit (flag 26B8) runs HurtFrame. Otherwise, unless busy, the main act takes its pose: the
// stand pose (an enemy's plus unk2684; a party role's weak pose at a quarter of its hit points or less), the pose of
// a chosen command, a guard pose by unk26D2, the defeated pose of a role, or unk2A0E. It is drawn over its shadow
// (mirrored by the facing; a forced draw adds Additive), with its outlines, its blend or wave, the barrier flash, a
// grey copy (UnkMark4), a red copy for command 2, the gold over a gold guard, its sound; command 6 of a
// role shows the thrown act; its overlay, idle effect, follow-up and effects follow. A fading or invisible unit runs
// its fade instead.
void BattleUnit::IdleFrame(BattleServices& services, int paused)
{
    Painter& painter = services.painter;
    ActState& act = this->mainAct;
    this->unk2AF4 = paused;
    this->MoveIdle(services);
    act.hold = 0;

    if (paused == 1 || (this->marks & BattleUnit::Held) != 0)
    {
        act.hold = 1;
    }

    if ((act.target & ActState::Holds) != 0)
    {
        act.hold = 1;
    }

    const bool forced = this->unk2B00 == 1 || this->unk2B04 == 1;
    const bool shown = this->defeated != 1 || (this->kind != 0 && !forced) || this->unk2AF8 != 0;

    if (!shown)
    {
        return;
    }

    if ((this->unk26B8 & kFlag26B8) != 0)
    {
        if (this->HurtFrame(services) == 1)
        {
            this->charging = 0;
            this->ClearFlag26B8();
        }

        return;
    }

    if (this->unk2AAC == 1)
    {
        return;
    }

    act.id = this->actId;
    act.action = kShadowAction;
    this->unk2A88 = static_cast<u16>(act.action);

    // 0x00478D13 calls nullsub_1 without arguments for a party role: nothing to log.
    if (this->kind != 1)
    {
        act.action += this->unk2684;
    }

    const RoleRecord* record = this->roleRecord;

    if (this->kind == 1 && this->unk2B00 == 0 && this->unk2B04 == 0 &&
        record->hitPoints <= record->maxHitPoints / kWeakShare)
    {
        act.action = kWeakAction;
    }

    if (this->unk2A86 != 0 && this->unk2AB4 == 1 && this->unk2AC8 == 1)
    {
        if (this->unk2B00 == 1)
        {
            this->guarding = 0;
        }

        if (this->unk2B04 == 1)
        {
            this->placement.x = kForcedPlaceX;
            this->placement.y = kForcedPlaceY;
        }

        this->unk2670 = 0;

        if (this->kind == 1)
        {
            this->unk2674 = 0;
        }

        act.action = kReadyAction;

        if (this->actId == kReadyAct)
        {
            act.action = this->unk2690;
        }

        this->unk2A88 = static_cast<u16>(act.action);

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag4000) != 0 ||
            (this->unk2A86 & BattleUnit::UnkCommandFlag2000) != 0)
        {
            act.action = kItemReadyAction;
        }

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag8000) != 0 && act.waiting == 1 && this->kind == 1)
        {
            act.waiting = 0;
            act.position = static_cast<u16>(act.position - kReadyRewind);
        }

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag400) != 0)
        {
            act.action = kItemReadyAction;
        }

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag800) != 0)
        {
            act.action = kAbsorbReadyAction;
        }

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag200) != 0)
        {
            return;
        }
    }

    if (this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        if (this->unk26D2 == kGuardPose1)
        {
            this->unk2AD4 = 0;
            act.action = kGuardAction1;
        }

        if (this->unk26D2 == kGuardPose2)
        {
            act.action = kGuardAction2;

            if (act.waiting == 1)
            {
                act.waiting = 0;
                act.position = static_cast<u16>(act.position - kReadyRewind);
                act.view = 0;
            }
        }

        if (this->unk26D2 == kGuardPose4)
        {
            act.action = kGuardAction4;
        }

        if (this->unk26D2 == kGuardPose8)
        {
            act.action = kGuardAction8;
        }
    }

    if (this->kind == 1 && this->defeated == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        act.action = kDefeatedShadowAction;
    }

    if (this->action != 0)
    {
        act.action = this->action;
    }

    if ((act.target & ActState::FollowsUp) != 0)
    {
        this->commandA = this->commandB;
        act.target = 0;

        if ((this->unk2A86 & BattleUnit::UnkCommandFlag8000) == 0 ||
            (this->unk2A86 & BattleUnit::UnkCommandFlag4000) != 0)
        {
            act.hold = 0;
        }

        if ((this->command.actionFlags & UnitCommand::DelaysFadeIn) != 0)
        {
            this->marks = static_cast<u16>(this->marks | BattleUnit::UnkMark8);
            this->recoilAct = act;
        }
    }

    if (services.acts.Run(act) == 0)
    {
        return;
    }

    this->shadowPicture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->shadowPicture == nullptr)
    {
        return;
    }

    const Picture& picture = *this->shadowPicture;
    painter.SetSource(picture.data.data());
    this->originX = static_cast<u16>(act.x);
    this->drawFlags = act.flags;
    this->actAnchorX = act.anchorX;

    auto mirror = [this, &act, &picture]
    {
        this->drawFlags = act.flags ^ Painter::Mirror;

        if (act.x != 0)
        {
            this->originX = static_cast<u16>(picture.width - static_cast<u16>(act.x));
        }

        if (act.anchorX != 0)
        {
            this->actAnchorX = static_cast<u16>(picture.width - act.anchorX);
        }
    };

    if (forced)
    {
        if (this->facing == 1)
        {
            mirror();
        }

        if ((this->drawFlags & ActState::NoBlend) == 0)
        {
            this->drawFlags |= Painter::Additive;
        }
    }

    if (this->facing == 1 && this->unk2B00 == 0 && this->unk2B04 == 0)
    {
        mirror();
    }

    const u16 marks = this->marks;

    if ((marks & BattleUnit::UnkMark8) != 0)
    {
        this->FadeMainOut(services);
        return;
    }

    if ((marks & BattleUnit::UnkMark200) != 0)
    {
        this->FadeMainIn(services);
        return;
    }

    if (this->commandA == 0 || this->commandA == BattleUnit::UnkCommand1C)
    {
        if ((marks & BattleUnit::InvisibleMark) != 0)
        {
            this->FadeInvisible(services, 0);
            return;
        }

        if ((marks & BattleUnit::UnkMark400) != 0)
        {
            this->FadeInvisible(services, 1);
            return;
        }
    }

    const int x = this->unk26A8 + this->placement.x - static_cast<i16>(this->originX);
    const int y = this->placement.y - this->unk26AC - act.y;
    const u32 mirrored = this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);

    if (this->guarding == 0 && (marks & BattleUnit::GuardingMark) == 0)
    {
        if (this->drawAtSaved != kRising && this->drawAtSaved != kDropping &&
            (this->status & BattleUnit::Invisible) == 0)
        {
            const int shadowHeight = picture.height / kShadowShare;
            DrawOptions& options = painter.Options();
            options.scaleHeight = shadowHeight;
            options.shear = picture.height >> kShadowShearShift;
            const int tint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
            options.tintRed = tint;
            options.tintGreen = tint;
            options.tintBlue = tint;
            this->unk26A4 = mirrored | Painter::Shadow;
            painter.DrawPicture(
                this->placement.x - static_cast<i16>(this->originX),
                this->placement.y - shadowHeight,
                picture.width,
                picture.height,
                this->unk26A4
            );
        }

        if ((this->status & kOutlinedStatus) != 0)
        {
            this->colorPair = kWhiteColorPair;
            painter
                .DrawOutline(x, y, picture.width, picture.height, mirrored, DrawArgument{.colorPair = this->colorPair});
        }

        if (this->outline == 1)
        {
            this->colorPair = BattleUnit::OutlineColorPair;
            painter
                .DrawOutline(x, y, picture.width, picture.height, mirrored, DrawArgument{.colorPair = this->colorPair});
            this->colorPair = BattleUnit::OutlineColorPair;
        }

        painter.SetSource(picture.data.data());

        if ((this->status & BattleUnit::Invisible) != 0)
        {
            this->unk26A4 = mirrored;
            painter.DrawPicture(x, y, picture.width, picture.height, this->unk26A4 | Painter::SmearUnder);
        }
        else
        {
            if ((this->drawFlags & ActState::Signal) == ActState::Signal)
            {
                painter.Options().alpha = act.signal;
            }

            if (act.wait != 0)
            {
                painter.SetWave(act.wait);
            }

            if ((marks & BattleUnit::Held) != 0)
            {
                painter.SetWave(0);
            }

            painter.DrawPicture(x, y, picture.width, picture.height, this->drawFlags);
            painter.SetWave(0);

            if ((this->status & BattleUnit::Barrier) != 0)
            {
                this->DrawFlash(services);
            }
        }

        if ((marks & BattleUnit::UnkMark4) != 0)
        {
            this->unk26A4 = mirrored;
            painter.DrawPicture(x, y, picture.width, picture.height, this->unk26A4 | Painter::GrayUnder);
        }

        if (this->commandA == BattleUnit::UnkCommand2)
        {
            this->unk26A4 = mirrored;
            painter.Options().tintRed = kHurtFlash;
            painter.DrawPicture(x, y, picture.width, picture.height, this->unk26A4 | Painter::TintUnder);
        }

        if (this->goldGuard == 1 && this->unk26D2 == 0)
        {
            services.hud.DrawNumber(
                kGoldFont,
                services.party.variables[kGoldVariable],
                this->placement.x + kGoldLeft,
                this->placement.y - act.y - kGoldAbove
            );
        }

        if (act.view != 0)
        {
            services.audio.PlaySound(act.view, services.config.soundVolume);
            services.audio.SetSoundPan(act.view, this->placement.x >= kPanCentre ? kFadePan : -kFadePan);
        }

        act.view = 0;
    }

    if (this->commandA == BattleUnit::UnkCommand6 && this->kind == 1)
    {
        ActState& thrown = this->recoilAct;
        thrown.action = 0;
        thrown.id = kThrownSpark;

        if (services.acts.Run(thrown) == 0)
        {
            return;
        }

        this->picture = services.archive.GetPicture(thrown.frame, thrown.picture);

        if (this->picture != nullptr)
        {
            painter.SetSource(this->picture->data.data());
            painter.DrawPicture(
                static_cast<i16>(act.anchorX) + this->placement.x - thrown.x - static_cast<i16>(this->originX),
                static_cast<i16>(act.anchorY) + this->placement.y - thrown.y - act.y,
                this->picture->width,
                this->picture->height,
                thrown.flags
            );
        }

        services.audio.PlaySound(thrown.view, services.config.soundVolume);
        const i32 waiting = thrown.waiting;
        thrown.view = 0;

        if (waiting == 1)
        {
            thrown.waiting = 0;
            thrown.position = 0;
        }
    }

    if (this->guarding == 0 && (this->marks & BattleUnit::GuardingMark) == 0 &&
        (this->status & BattleUnit::UnkStatus4) == 0 && act.rotation != 0 && act.actionCode != 0)
    {
        this->unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

        if (this->unk2554 != nullptr)
        {
            painter.SetSource(this->unk2554->data.data());
            this->unk2698 = static_cast<i16>(act.offsetX);
            painter.DrawPicture(
                this->unk2698 + this->placement.x - static_cast<i16>(this->originX),
                static_cast<i16>(act.offsetY) + this->placement.y - this->unk26AC - act.y,
                this->unk2554->width,
                this->unk2554->height,
                this->drawFlags | Painter::Additive
            );
        }
    }

    this->PlayIdleEffect(services);
    this->PlayIdleFollowUp(services);
    this->DrawEffects(services, x, y);
}

// The movement of IdleFrame by drawAtSaved (inline in the original): 1 shakes the unit a pixel at random while nudge
// lasts (towards its side), 2 lifts it, 3 drops it back to its saved place.
void BattleUnit::MoveIdle(BattleServices& services)
{
    if (this->nudge == 0)
    {
        return;
    }

    if (this->drawAtSaved == kShaking)
    {
        if (static_cast<u16>(services.random.Below(kShakeChance)) == 1)
        {
            const bool back = this->kind == 1 ? this->facing != 0 : this->facing == 0;
            this->placement.x = static_cast<i16>(this->placement.x + (back ? -1 : 1));
        }

        --this->nudge;
        return;
    }

    if (this->drawAtSaved == kRising)
    {
        this->placement.y = static_cast<i16>(this->placement.y - kRiseStep);

        if (this->placement.y <= 0)
        {
            this->placement.y = 0;
            this->nudge = 0;
            this->drawAtSaved = 0;
        }

        return;
    }

    if (this->drawAtSaved == kDropping)
    {
        this->placement.y = static_cast<i16>(this->placement.y + kDropStep);
        const i16 home = this->savedPlacement.y;

        if (this->placement.y >= home)
        {
            this->placement.y = home;
            this->nudge = 0;
            this->drawAtSaved = 0;
            this->unk2B10 = 1;
            this->RaiseFlag26B8();
        }
    }
}

// swd32.exe 0x00480220: an enemy chooses its command for the turn; `act` takes the act of an object or a cast; 1 when
// the command is chosen with its act (UnkCommandFlag8000 and command 2, or 0xF for UnkAction1000), 0 for an attack
// or a skill of command 1, 1 at once while UnkCommandFlag8000 is set. A heal (healChoice) comes first; else objects
// and skills take turns (choicePhase), and a status of bit 5 or 8 or no idle turn yet fall back to the attack after an
// unused roll. An object or skill is tried with a roll of 0..9 below idleTurns plus its chance; the third object
// comes every 7 turns, the second skill every 5. The attack is the object's skill script with its attack added.
int BattleUnit::ChooseCommand(BattleServices& services, Database& database, u32& act)
{
    Random& random = services.random;
    ObjectData& data = this->data->data;
    int choice = kNoChoice;
    int chosen = 0;

    if ((this->unk2A86 & BattleUnit::UnkCommandFlag8000) != 0)
    {
        return 1;
    }

    this->command = UnitCommand{};

    if (this->healChoice != 0)
    {
        choice = kHealChoice;
    }
    else if (data.itemACount != 0)
    {
        if (data.skillACount != 0 && this->choicePhase == kItemsChosen)
        {
            choice = kSkillChoice;
            this->choicePhase = kSkillsChosen;
        }
        else
        {
            choice = kItemChoice;
            this->choicePhase = kItemsChosen;
        }
    }
    else if (data.itemBCount != 0 || data.itemCCount != 0)
    {
        choice = kItemChoice;
        this->choicePhase = kItemsChosen;
    }
    else if (data.skillACount != 0 || data.skillBCount != 0)
    {
        choice = kSkillChoice;
        this->choicePhase = kSkillsChosen;
    }

    bool rollIdle = true;

    if (this->StatusBit5Or8() == 1)
    {
        choice = kStatusChoice;
    }
    else if (choice == kHealChoice)
    {
        rollIdle = false;
    }

    if (rollIdle && this->idleTurns == 0)
    {
        random.Below(kUnusedChoiceRoll);
        choice = kIdleChoice;
        this->choicePhase = static_cast<u8>(this->choicePhase != kItemsChosen ? 1 : 0);
    }

    // A command the unit may not use: a forced unit's skill with kNoForcedStatus, or an UnkAction1000 one while
    // unk53BF28 is set.
    auto barred = [this, &services]
    {
        return (this->unk2B00 == 1 && (static_cast<u32>(this->command.unk08) & kNoForcedStatus) != 0) ||
               ((this->command.actionFlags & UnitCommand::UnkAction1000) != 0 && services.state.unk53BF28 != 0);
    };

    if (choice == kItemChoice)
    {
        if ((this->marks & BattleUnit::UnkMark100) != 0 || data.itemChance == 0)
        {
            return this->ChooseAttack(database);
        }

        const u32 value = random.Below(kChoiceRoll);
        int item = static_cast<int>(value);

        if (value <= kFirstItemRoll)
        {
            item = 0;
        }
        else if (value <= kSecondItemRoll)
        {
            item = 1;
        }
        else if (value <= kChoiceRoll)
        {
            item = 2;
        }

        if (this->itemWait >= kItemWaitTurns)
        {
            if (data.itemC != 0 && data.itemCCount != 0)
            {
                item = 2;
            }

            this->itemWait = 0;
        }

        // An object used: its skill script is the command (inline in the original).
        auto useItem = [&](u16 id, u16& count, i32 readyAction)
        {
            if (id == 0 || count == 0)
            {
                return false;
            }

            if (random.Below(kChoiceRoll) >= static_cast<u32>(this->idleTurns + data.itemChance))
            {
                return false;
            }

            database.LoadObject(this->object, id);
            this->LoadCommand(database, this->object.data.unk3E);

            if (barred())
            {
                return false;
            }

            act = this->object.data.unk50;
            --count;

            if (this->actId == kReadyAct)
            {
                this->unk2690 = readyAction;
            }

            return true;
        };

        if (item == 0)
        {
            chosen = useItem(data.itemA, data.itemACount, kFirstItemAction) ? 1 : 0;
        }
        else if (item == 1)
        {
            chosen = useItem(data.itemB, data.itemBCount, kSecondItemAction) ? 1 : 0;
        }
        else if (item == 2)
        {
            if (useItem(data.itemC, data.itemCCount, kThirdItemAction))
            {
                chosen = 1;
                this->itemWait = 0;
            }
        }

        Database::FreeDescription(this->object);
        std::fill_n(this->skillName.begin(), kClearedNameBytes, '\0');

        if (chosen != 1)
        {
            return this->ChooseAttack(database);
        }

        const std::span<const char> name(this->object.data.name);
        const std::size_t length = std::min<std::size_t>(strnlen(name.data(), name.size()), this->skillName.size() - 1);
        std::copy_n(name.begin(), length, this->skillName.begin());
        this->skillName[length] = '\0';
        this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
        this->unk29A4 = static_cast<u16>(this->unk29A4 + static_cast<u16>(this->command.value));
        const bool special = (this->command.actionFlags & UnitCommand::UnkAction1000) != 0;
        this->SetCommand(special ? BattleUnit::UnkCommand15 : BattleUnit::UnkCommand2);
        this->idleTurns = 0;
        return 1;
    }

    if (choice == kSkillChoice)
    {
        if (data.skillChance == 0)
        {
            return this->ChooseAttack(database);
        }

        const u32 value = random.Below(kChoiceRoll);
        int skill = static_cast<int>(value);

        if (value <= kFirstSkillRoll)
        {
            skill = 0;
        }
        else if (value <= kChoiceRoll)
        {
            skill = 1;
        }

        if (this->skillWait >= kSkillWaitTurns)
        {
            if (data.skillB != 0 && data.skillBCount != 0)
            {
                skill = 1;
            }

            this->skillWait = 0;
        }

        if (skill != 0 && skill != 1)
        {
            return this->ChooseAttack(database);
        }

        const u16 id = skill == 0 ? data.skillA : data.skillB;
        u16& count = skill == 0 ? data.skillACount : data.skillBCount;

        if (id == 0 || count == 0)
        {
            return this->ChooseAttack(database);
        }

        if (random.Below(kChoiceRoll) >= static_cast<u32>(this->idleTurns + data.skillChance))
        {
            return this->ChooseAttack(database);
        }

        this->LoadCommand(database, id);

        if (skill == 1)
        {
            this->skillWait = 0;
        }

        if (barred())
        {
            return this->ChooseAttack(database);
        }

        this->unk29A4 = static_cast<u16>(this->command.value);
        --count;
        this->idleTurns = 0;

        if ((this->command.actionFlags & UnitCommand::UnkAction1000) != 0)
        {
            this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
            this->SetCommand(BattleUnit::UnkCommand15);
            return 1;
        }

        if ((this->command.flags & UnitCommand::UnkFlag2) != 0)
        {
            act = this->command.castAct;
            this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
            this->SetCommand(BattleUnit::UnkCommand2);
            return 1;
        }

        this->unk2A8C = static_cast<u16>(skill + 1);
        this->SetCommand(BattleUnit::UnkCommand1);
        this->MarkDelayedAttack();
        return 0;
    }

    if (choice == kHealChoice && (this->marks & BattleUnit::UnkMark100) == 0 && data.itemChance != 0 &&
        (this->healChoice == kFirstHeal || this->healChoice == kSecondHeal))
    {
        const bool first = this->healChoice == kFirstHeal;
        database.LoadObject(this->object, first ? data.healItemA : data.healItemB);
        Database::FreeDescription(this->object);
        act = this->object.data.unk50;
        this->LoadCommand(database, this->object.data.unk3E);
        this->unk29A6 = this->object.data.unk34;
        this->healChoice = 0;
        --(first ? data.healCountA : data.healCountB);
        this->unk2A86 = static_cast<u16>(this->unk2A86 | BattleUnit::UnkCommandFlag8000);
        this->SetCommand(BattleUnit::UnkCommand2);

        if (this->actId == kReadyAct)
        {
            this->unk2690 = kHealAction;
        }

        return 1;
    }

    return this->ChooseAttack(database);
}

// The attack of ChooseCommand (inline in the original): the object's skill script as command 1, every wait counted
// up, the attack (a party role's equipment bonus) added to the value; 0.
int BattleUnit::ChooseAttack(Database& database)
{
    this->command = UnitCommand{};
    this->LoadCommand(database, this->data->data.skill);
    this->SetCommand(BattleUnit::UnkCommand1);
    ++this->idleTurns;
    ++this->itemWait;
    ++this->skillWait;

    if (this->kind != 0)
    {
        this->unk29A4 = static_cast<u16>(this->unk29A4 + this->battleRecord->attackBonus);
    }
    else
    {
        this->unk29A4 = static_cast<u16>(this->unk29A4 + this->data->data.attack);
    }

    this->MarkDelayedAttack();
    return 0;
}

// The end of a command-1 choice (inline in the original): a command with UnkFlag10 chosen as command 1 becomes command
// 0xC8 with command B 1.
void BattleUnit::MarkDelayedAttack()
{
    if ((this->command.flags & UnitCommand::UnkFlag10) != 0 && this->commandA == BattleUnit::UnkCommand1)
    {
        this->commandB = BattleUnit::UnkCommand1;
        this->commandA = kDelayedCommand;
    }
}

// swd32.exe 0x00481A40: the damage of an enemy's hit on `target` (-1 shows no number). A barrier stops it; a held
// target is freed by a hit that heals nothing; the buff lands first. A party role takes the ailment unless its spirit
// fan guards against it by proficiency (an ailment with kSacrificeAilment defeats the attacker and gives the target's
// hit points), then its defence and record resistances count; an enemy target its object's. Command 1 rolls on the
// level, raised and cut by the attack buffs, less the defence (else the bare value, at least 1, rolled again unless
// unk2A8C); other commands add a roll to a positive value. The resistance of the command's element scales it, an
// effect of its element halves it, unk29A6 heals instead, a guard cuts it by 10..39%, and a party role gains fury. The
// original computes in 16 bits.
i16 BattleUnit::EnemyHitDamage(BattleServices& services, BattleUnit& target)
{
    Random& random = services.random;
    const ObjectData& data = this->data->data;
    i16 damage = static_cast<i16>(this->unk29A4);
    i32 raise = 0;
    i32 cut = 0;
    bool resisted = false;
    int defense = 0;
    std::array<u8, kResistances> resistances = {};
    target.PushTarget(BattleUnit::DamageTarget);

    if (target.HasEffectBits(BattleUnit::UnkStatus4) == 1)
    {
        return 0;
    }

    if (this->unk29A6 == 0 && target.IsHeldOrHolding() == 1)
    {
        target.CureEffects(services, BattleUnit::UnkStatus8);
    }

    if (target.AddBuff(services, this->CommandSkill()) == 1)
    {
        return -1;
    }

    if (target.kind == BattleUnit::PartyRole)
    {
        const auto* record = static_cast<const RoleRecord*>(target.SelectEntry(1));
        defense = target.Defense();
        std::copy_n(record->unk2D.begin(), resistances.size(), resistances.begin());
        const u16 ailment = this->command.ailment;

        if (ailment != 0 && !target.FanGuards(services, ailment))
        {
            if (target.AddAilment(services, this->CommandSkill()) == 1)
            {
                target.RaiseFlag26B8();

                if ((ailment & kSacrificeAilment) != 0)
                {
                    this->Damage(services, kSacrificeDamage, 0, 0);
                    i32 hitPoints = 0;
                    i32 maximum = 0;
                    target.GetHitPoints(hitPoints, maximum);
                    return static_cast<i16>(hitPoints);
                }
            }
            else if (this->command.value != 0 || this->unk29A4 != 0)
            {
                target.RaiseFlag26B8();
            }
            else
            {
                target.RollEvade(services, kEvadeRoll);
            }

            if (this->command.value == 0 && this->unk29A4 == 0)
            {
                return -1;
            }
        }
    }
    else
    {
        const ObjectData& targetData = target.Data()->data;
        defense = targetData.defense;
        std::copy_n(targetData.resistances.begin(), resistances.size(), resistances.begin());
        target.SetDefeatPose(LowByte(this->command.pose));
    }

    const u16 element = this->command.resistance;
    const bool elemental = element >= 1 && element <= kResistances;

    auto scaleByElement = [&]
    {
        if (elemental && ScaleByTenths(static_cast<i8>(resistances[element - 1U]), damage) == 1)
        {
            resisted = true;
        }
    };

    auto levelRoll = [&random, &data]
    {
        return static_cast<i32>(random.Below(
            static_cast<u16>(
                static_cast<i16>(data.level) / static_cast<int>(kLevelShare) + static_cast<int>(kLevelRollBase)
            )
        ));
    };

    if (this->commandA != BattleUnit::UnkCommand1)
    {
        const u32 spread = random.Below(kEnemySpreadRoll);
        const auto limit = static_cast<u16>((data.level + data.unk5C) / static_cast<u16>(spread + kEnemySpreadBase));
        const i32 bonus = static_cast<i32>(random.Below(limit)) + 1;

        if (damage > 0)
        {
            damage = static_cast<i16>(damage + bonus);
        }

        if (this->commandA == BattleUnit::UnkCommand2)
        {
            scaleByElement();
        }
    }
    else
    {
        const auto attack = static_cast<i16>(levelRoll() + 1 + damage);

        if ((this->status & BattleUnit::AttackUp) != 0)
        {
            raise = kAttackBuffPercent * attack / kPercent;
        }

        if ((this->status & BattleUnit::AttackDown) != 0)
        {
            cut = kAttackBuffPercent * attack / kPercent;
        }

        const auto raised = static_cast<i16>(raise - cut + attack);

        if (raised > static_cast<i32>(static_cast<u16>(defense)))
        {
            damage = static_cast<i16>(raised - defense);
        }
        else
        {
            const auto value = static_cast<i16>(this->unk29A4);

            if ((this->status & BattleUnit::AttackUp) != 0)
            {
                raise = kAttackBuffPercent * value / kPercent;
            }

            if ((this->status & BattleUnit::AttackDown) != 0)
            {
                cut = kAttackBuffPercent * value / kPercent;
            }

            auto bare = static_cast<i16>(raise - defense - cut + value);

            if (bare < 0)
            {
                bare = 1;
            }

            damage = this->unk2A8C != 0 ? bare : static_cast<i16>(levelRoll() + 1 + bare);
        }

        scaleByElement();
    }

    if (target.HasEffectElement(static_cast<i16>(this->command.element)) == 1)
    {
        damage = static_cast<i16>(damage / 2);
    }

    if (this->unk29A6 != 0)
    {
        target.PushTarget(BattleUnit::HealTarget);
        damage = static_cast<i16>(-static_cast<i16>(this->unk29A6));
    }

    if (resisted)
    {
        target.PushTarget(BattleUnit::HealTarget);
    }

    if (target.GetGuard() == 1)
    {
        const auto share = static_cast<u16>(random.Below(kGuardCutRoll) + kGuardCutBase);
        damage = static_cast<i16>(damage * share / -kPercent + damage);
    }

    if (target.kind == BattleUnit::PartyRole && damage > 0)
    {
        static_cast<BattleRole&>(target).AddFury(services, damage);
    }

    return damage;
}

// The spirit fan of EnemyHitDamage (inline in the original): a worn one counts its proficiency up by kFanStep and, by
// each kFanShare of it, guards against more ailments (all at full proficiency).
bool BattleUnit::FanGuards(BattleServices& services, u16 ailment)
{
    if (this->CanUseAccessory(kSpiritFanKind) != 1)
    {
        return false;
    }

    CounterList& proficiency = services.party.fame.lists[Fame::ProficiencyList];
    this->StepAccessoryCounters(proficiency, kFanStep);
    const auto level = static_cast<u16>(this->AccessoryProficiency(kSpiritFanKind, proficiency)) / kFanShare;

    if (level == kFanGuards.size())
    {
        return true;
    }

    return level < kFanGuards.size() && (ailment & kFanGuards[level]) != 0;
}

// swd32.exe 0x004763D0: one frame of the unit running off the field, counting fadeLevel down; 1 once it is down to
// kRunEnd (the recoil act cleared) or the act ends. The recoil act runs the shadow action turned away from the
// facing, stepping kRunStep a frame; its first picture plays a step sound, fadeLevel kRunSoundAt the run sound.
int BattleUnit::RunAway(BattleServices& services)
{
    Painter& painter = services.painter;
    ActState& run = this->recoilAct;

    if (this->fadeLevel <= kRunEnd)
    {
        run = ActState{};
        return 1;
    }

    if (run.picture == kStepPicture)
    {
        services.audio.PlaySound(kStepSound, services.config.soundVolume);
    }

    run.id = this->actId;
    this->unk2AAC = 1;
    run.action = kShadowAction;
    run.hold = 0;

    if (this->held == 1)
    {
        run.hold = 1;
    }

    if (services.acts.Run(run) == 0)
    {
        return 1;
    }

    this->picture = services.archive.GetPicture(run.frame, run.picture);
    this->frameFlags = run.flags;
    this->actOffsetX = static_cast<u16>(run.x);

    if (this->fadeLevel == kRunSoundAt)
    {
        run.view = kRunSound;
        services.audio.PlaySound(run.view, services.config.soundVolume);
        services.audio.SetSoundPan(run.view, this->facing == 1 ? kFadePan : -kFadePan);
        run.view = 0;
    }

    this->frameFlags ^= Painter::Mirror;

    if (this->facing == 1)
    {
        this->frameFlags ^= Painter::Mirror;

        // A missing picture is read through a null pointer in the original.
        if (run.x != 0 && this->picture != nullptr)
        {
            this->actOffsetX = static_cast<u16>(this->picture->width - static_cast<u16>(run.x));
        }
    }

    u16 x = 0;
    u16 y = 0;
    this->GetPosition(x, y);
    const int stepped = this->facing == 1 ? x + kRunStep : x - kRunStep;
    this->SetPosition(static_cast<i16>(stepped), static_cast<i16>(y));

    if (this->picture != nullptr)
    {
        painter.SetSource(this->picture->data.data());
        painter.DrawPicture(
            this->placement.x - static_cast<i16>(this->actOffsetX),
            this->placement.y - run.y,
            this->picture->width,
            this->picture->height,
            this->frameFlags,
            PaletteArgument(*this->picture)
        );
    }

    --this->fadeLevel;
    return 0;
}

// swd32.exe 0x0046F6E0: the worn element orbs (AccessoryKind 0x33) step their proficiency counters (their unk50) in
// `list` by kOrbStep up to their unk44, when hit by `command` of an element 1..9 (the original also tests the address
// of the orb's resistance to it, never null); 1 when an orb counts. Each search starts at the head of the list, its
// count compared as an id; a missing counter is appended at the end and counted.
int BattleUnit::StepOrbCounters(CounterList& list, const UnitCommand& command)
{
    int counts = 0;

    for (const ObjectRecord& accessory : this->accessories)
    {
        const ObjectData& data = accessory.data;
        const auto id = static_cast<i16>(data.unk50);

        if (id == 0 || data.unk48 != static_cast<u16>(AccessoryKind::ElementOrb) || command.resistance < 1 ||
            command.resistance > kResistances)
        {
            continue;
        }

        counts = 1;
        CounterNode* at = &list;

        while (at->id != id && at->next != nullptr)
        {
            at = at->next.get();
        }

        if (at->id == id)
        {
            if (at->done != 0)
            {
                continue;
            }

            at->value = static_cast<u16>(at->value + kOrbStep);

            if (at->value >= data.unk44)
            {
                at->value = data.unk44;
            }

            at->percent = static_cast<u16>(
                crt::Ftol(static_cast<double>(at->value) / static_cast<double>(data.unk44) * kCounterPercent)
            );
            continue;
        }

        at->next = std::make_unique<CounterNode>();
        at = at->next.get();
        at->id = id;
        at->value = kOrbStep;
        at->percent = static_cast<u16>(
            crt::Ftol(static_cast<double>(kOrbStep) / static_cast<double>(data.unk44) * kCounterPercent)
        );
        ++list.id;
    }

    return counts;
}

// swd32.exe 0x0047C1F0: one frame of the unit's damage numbers (while unk2AD8): from damageSlot to targetCount, each
// slot waits out its delay, then its four digits (font pushedTargets) hop up one after another by DigitHops (not while
// held); once all have landed (step 9) a pending extra number (extraNumber: digits unk53AEDC..E4) becomes the next
// slot, and the slot rises and fades by 2 a frame up to kNumberFadeEnd before it is cleared and the next is shown. A
// leading zero digit is not drawn. 1 once every slot is done (all cleared).
int BattleUnit::DrawDamageNumbers(BattleServices& services)
{
    BattleHud& hud = services.hud;
    DrawOptions& options = services.painter.Options();

    if (this->unk2AD8 == 0)
    {
        return 0;
    }

    const int x = this->placement.x;
    const int y = this->placement.y - kNumberAbove;

    for (u16 slot = this->damageSlot; slot < this->targetCount; ++slot)
    {
        std::array<i16, DamageDigits>& digits = this->damageDigits[slot];
        std::array<i16, DamageDigits>& hops = this->unk283C[slot];
        i16& fade = this->numberFades[slot];
        const auto font = static_cast<i16>(this->pushedTargets[slot]);

        if (this->damageDelays[slot] != 0)
        {
            --this->damageDelays[slot];
            continue;
        }

        const bool landed = std::all_of(
            hops.begin(),
            hops.end(),
            [](i16 hop)
            {
                return hop == kLastHop;
            }
        );

        if (!landed)
        {
            hud.digitMode = 0;

            auto hopY = [y, &hops](std::size_t digit)
            {
                return y - BattleUnit::DigitHops[static_cast<std::size_t>(hops[digit])];
            };

            if (digits[0] != 0)
            {
                hud.DrawNumber(font, digits[0], x - kDigitWidth, hopY(0));
            }

            if ((digits[1] != 0 || digits[0] != 0) && hops[0] > 0)
            {
                hud.DrawNumber(font, digits[1], x, hopY(1));
            }

            if (hops[1] > 0)
            {
                if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0)
                {
                    hud.DrawNumber(font, digits[2], x + kDigitWidth, hopY(2));
                }

                hud.DrawNumber(font, digits[3], x + 2 * kDigitWidth, hopY(3));
            }

            if (this->held == 0)
            {
                for (std::size_t digit = 0; digit < hops.size(); ++digit)
                {
                    if (digit > 0 && hops[digit - 1] <= 0)
                    {
                        break;
                    }

                    hops[digit] = static_cast<i16>(std::min<int>(hops[digit] + 1, kLastHop));
                }
            }

            continue;
        }

        // The extra number takes the next slot (its fourth digit as it was; the original writes past the slots when
        // they are all used, here it is dropped).
        if (this->extraNumber == 1)
        {
            this->extraNumber = 0;

            if (this->targetCount < MaxTargets)
            {
                std::array<i16, DamageDigits>& extra = this->damageDigits[this->targetCount];
                extra[0] = static_cast<i16>(services.state.unk53AEDC);
                extra[1] = static_cast<i16>(services.state.unk53AEE0);
                extra[2] = static_cast<i16>(services.state.unk53AEE4);
                ++this->targetCount;
            }
        }

        if (fade >= kNumberFadeEnd)
        {
            hops.fill(0);
            fade = 0;
            hud.digitMode = 0;
            ++this->damageSlot;
            digits.fill(0);
            continue;
        }

        if (this->held == 0)
        {
            fade = static_cast<i16>(fade + kNumberFadeStep);
        }

        auto fading = [&options, &fade]
        {
            options.tintRed = -fade;
            options.tintGreen = -fade;
            options.tintBlue = -fade;
            options.scaleHeight = fade + kNumberScaleBase;
        };

        const int risenY = y - fade;
        fading();

        if (digits[0] != 0)
        {
            hud.DrawNumber(font, digits[0], x - kDigitWidth, risenY);
        }

        if (digits[0] != 0 || digits[1] != 0)
        {
            fading();
            hud.DrawNumber(font, digits[1], x, risenY);
        }

        fading();

        if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0)
        {
            hud.DrawNumber(font, digits[2], x + kDigitWidth, risenY);
        }

        hud.DrawNumber(font, digits[3], x + 2 * kDigitWidth, risenY);
        hud.digitMode = 0;
    }

    if (this->damageSlot < this->targetCount)
    {
        return 0;
    }

    hud.digitMode = 0;
    this->targetCount = 0;
    this->damageSlot = 0;
    this->unk2AD8 = 0;
    return 1;
}

// swd32.exe 0x00476780: object `choice` (0..2: itemA, itemB, itemC) of the enemy loaded into `object`, its name copied
// to `name` and `id` set to itemA whichever was chosen (as in the original); 0 without that object
int BattleUnit::ItemName(Database& database, int choice, std::string& name, u32& id)
{
    const ObjectData& data = this->data->data;
    u16 item = 0;

    if (choice == 0)
    {
        item = data.itemA;
    }
    else if (choice == 1)
    {
        item = data.itemB;
    }
    else if (choice == 2)
    {
        item = data.itemC;
    }

    if (item == 0)
    {
        return 0;
    }

    database.LoadObject(this->object, item);
    id = this->data->data.itemA;
    name = ObjectName(this->object);
    return 1;
}

// swd32.exe 0x00476860: skill `choice` (0: skillA, 1: skillB) of the enemy loaded into `object` and its name copied
// to `name`; 0 without that skill
int BattleUnit::SkillName(Database& database, int choice, std::string& name)
{
    const ObjectData& data = this->data->data;
    u16 skill = 0;

    if (choice == 0)
    {
        skill = data.skillA;
    }
    else if (choice == 1)
    {
        skill = data.skillB;
    }

    if (skill == 0)
    {
        return 0;
    }

    database.LoadObject(this->object, skill);
    name = ObjectName(this->object);
    return 1;
}

// swd32.exe 0x004762F0: the object an enemy drops (dropItem) on a roll of 0..19 below its dropChance; 0 otherwise
u16 BattleUnit::RollDrop(BattleServices& services)
{
    const ObjectData& data = this->data->data;

    if (data.dropItem == 0 || services.random.Below(kDropRoll) >= data.dropChance)
    {
        return 0;
    }

    return this->data->data.dropItem;
}

// swd32.exe 0x0046F850: the first worn accessory whose proficiency counter in `list` is full (at its unk44) and not
// yet done: the counter marked done and the accessory's counter id returned (0 when none; the second accessory is not
// looked at once the first is). The search starts at the head of the list.
u16 BattleUnit::FinishProficiency(CounterList& list)
{
    u16 finished = 0;
    bool found = false;

    for (const ObjectRecord& accessory : this->accessories)
    {
        const ObjectData& data = accessory.data;

        if (data.unk50 == 0)
        {
            continue;
        }

        if (found)
        {
            return finished;
        }

        for (CounterNode* node = &list; node != nullptr; node = node->next.get())
        {
            if (static_cast<u16>(node->id) == data.unk50 && node->done == 0 && node->value >= data.unk44)
            {
                found = true;
                node->value = data.unk44;
                node->done = 1;
                finished = data.unk50;
                break;
            }
        }
    }

    return finished;
}

// swd32.exe 0x00476600: a refinable enemy (MissedMark) turned into one of its objects by a role of `level` with the
// talisman brush proficiency `proficiency`: below 50 its first object, 50..79 one of the first two, 80..100 one of
// three at random; a roll of 0..99 below 60 (90 when `level` is at least the enemy's) loads it into `object` and ends
// the mark. The object's unk48 when it is of kind bit kRefinedKind; 0 otherwise.
u16 BattleUnit::RefineTalisman(BattleServices& services, Database& database, u16 proficiency, u8 level)
{
    if ((this->marks & BattleUnit::MissedMark) == 0)
    {
        return 0;
    }

    u32 choices = 1;

    if (proficiency >= kTwoChoices && proficiency <= kTwoChoicesEnd)
    {
        choices = 2;
    }

    if (proficiency >= kThreeChoices && proficiency <= kThreeChoicesEnd)
    {
        choices = kRefineObjects;
    }

    const auto choice = static_cast<u16>(services.random.Below(choices) + 1) - 1;
    const ObjectData& data = this->data->data;
    const u16 item = choice == 0 ? data.itemA : choice == 1 ? data.itemB : choice == 2 ? data.itemC : u16{0};

    if (item == 0)
    {
        return 0;
    }

    const u16 chance = level >= data.level ? kSureRefine : kRefineChance;

    if (static_cast<u16>(services.random.Below(kRefineRoll)) >= chance)
    {
        return 0;
    }

    database.LoadObject(this->object, item);
    this->marks = static_cast<u16>(this->marks & ~BattleUnit::MissedMark);

    if ((this->object.data.kind & kRefinedKind) == 0)
    {
        return 0;
    }

    return this->object.data.unk48;
}

// swd32.exe 0x0046E090: unk2B00 (only its address is used: battle talk command 0x3A tests it)
i32 BattleUnit::Unk2B00() const
{
    return this->unk2B00;
}

// swd32.exe 0x0046E0A0: unk2B04 (only its address is used: battle talk command 0x3A tests it)
i32 BattleUnit::Unk2B04() const
{
    return this->unk2B04;
}

}  // namespace swd3
