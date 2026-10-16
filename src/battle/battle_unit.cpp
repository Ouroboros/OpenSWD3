#include "battle/battle_unit.h"

#include "audio/audio_manager.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "config/config.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "party/party.h"
#include "pic/picture.h"
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
    const int value = 0x96 / static_cast<int>(services.random.Below(9) + 1) + 0x12C;
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

    if (((this->unk2A86 >> 8) & 2) != 0)
    {
        x = this->slotActs[8].anchorX;
        y = this->slotActs[8].anchorY;
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
void BattleUnit::GetShadowRect(BattleServices& services, std::array<i32, 4>& rect)
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

    rect[0] = this->placement.x - state.x;
    rect[1] = this->placement.y - state.y;
    rect[2] = this->picture->width;
    rect[3] = this->picture->height;
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

// swd32.exe 0x00478670: the battle attributes (second) or the party record of a hero
const void* BattleUnit::SelectEntry(int second) const
{
    if (second != 0)
    {
        return this->heroStats;
    }

    return this->hero;
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
    if (command == 0xE || command == 0xF)
    {
        this->unk2A86 = static_cast<u16>(this->unk2A86 | 0x4000);
    }
    else if (command == 6)
    {
        this->unk2A86 = static_cast<u16>(this->unk2A86 | 0x800);
    }
    else if (command != 2 && command != 0xD && command != 3)
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

// swd32.exe 0x00478780: not while status bit 25 is set; raising it again restarts the attack
void BattleUnit::RaiseFlag26B8()
{
    if ((this->status & kRaiseBlocked) != 0)
    {
        return;
    }

    if (static_cast<i32>(this->unk26B8) < 0)
    {
        this->attackStage = 0;
        this->attackAct.position = 0;
    }

    this->unk26B8 |= kFlag26B8;
}

// swd32.exe 0x004787C0
int BattleUnit::Flag26B8() const
{
    return static_cast<int>(this->unk26B8 >> 31);
}

// swd32.exe 0x004787D0: stores at the current count without advancing it
int BattleUnit::PushTarget(u16 value)
{
    const u16 count = this->targetCount;
    this->pushedTargets[count] = value;
    return count;
}

// swd32.exe 0x004787F0: a defeated hero has its hit points set to 0
int BattleUnit::StartDefeat(int defeat)
{
    this->defeated = defeat;
    this->unk2ABC = 1;

    if (this->unk2A94 == 0)
    {
        this->unk2A94 = 6;
    }

    if (this->kind == BattleUnit::kHero && this->hero != nullptr)
    {
        this->hero->hitPoints = 0;
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

// swd32.exe 0x00478850: clears the state of a turn; heroes get a random gauge of 50..189. The slot actions cleared
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
    this->fadeLevel = 0xF;
    this->brightLevel = 1;
    this->unk2954 = 0;
    this->unk2A68 = 2;
    this->unk2A6A = 8;
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

    if ((mode & 8) != 0)
    {
        this->guarding = 0;
    }

    if ((this->command.actionFlags & 4) != 0)
    {
        this->guarding = 0;
        this->brightLevel = -0x20;
        this->marks = static_cast<u16>((this->marks & 0xFFF7) | 0x200);
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

    this->unk2630 = 0;
    this->unk2634 = 0;
    this->unk2638 = 0;
    this->unk263C = 0;
    this->command = UnitCommand{};
    this->targets = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    int result = this->kind;

    if (result == BattleUnit::kHero)
    {
        result = static_cast<int>(services.random.Below(0x8C)) + 0x32;
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
    this->unk29A2 = 0xFFFF;
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
    if (((this->unk2A86 >> 8) & 2) == 0)
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
            this->drawFlags = (actFlags & 1) != 0 ? actFlags & ~1u : actFlags | 1;
            this->originX = static_cast<u16>(width - static_cast<u16>(this->recoilAct.x));
        }

        this->drawFlags |= 4;
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

    if ((this->recoilAct.flags & 0x14) != 0)
    {
        this->fadeLevel = 0;
        return 0;
    }

    if (this->fadeLevel == 0xF)
    {
        this->recoilAct.view = 0x2F;
        services.audio.PlaySound(0x2F, services.config.soundVolume);
        services.audio.SetSoundPan(this->recoilAct.view, this->facing == 1 ? 0x10 : -0x10);
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
        options.tintRed = level - 0xF;
        options.tintGreen = level - 0xF;
        options.tintBlue = level - 0xF;
        services.painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->drawFlags);

        if (this->held == 0)
        {
            --this->fadeLevel;
        }

        return 0;
    }

    services.painter.Options().alpha = level;
    services.painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->drawFlags | 0x14);

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

    if (this->brightLevel >= 0xF)
    {
        this->recoilAct = ActState{};
        this->unk2AAC = 0;
        return 1;
    }

    this->unk2AAC = 1;
    this->recoilAct.action = this->commandA == 1 ? 0x28 : kShadowAction;
    this->recoilAct.id = this->actId;
    const u16 pose = this->unk2A8C;

    if (pose != 0)
    {
        this->recoilAct.action = pose == 2 ? 0x31 : 0x30;
    }

    if (services.acts.Run(this->recoilAct) == 0)
    {
        return 0;
    }

    if ((this->recoilAct.flags & 0x14) != 0)
    {
        this->brightLevel = 0x10;
        return 0;
    }

    this->picture = services.archive.GetPicture(this->recoilAct.frame, 0);

    if (this->brightLevel == 1)
    {
        this->recoilAct.view = 0x30;
        services.audio.PlaySound(0x30, services.config.soundVolume);
        services.audio.SetSoundPan(this->recoilAct.view, this->facing == 1 ? -0x10 : 0x10);
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
        options.tintRed = level - 0x10;
        options.tintGreen = level - 0x10;
        options.tintBlue = level - 0x10;
        services.painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->drawFlags);

        if (this->held == 0)
        {
            ++this->brightLevel;
        }

        return 0;
    }

    services.painter.Options().alpha = level;
    services.painter.DrawPicture(x, y, this->picture->width, this->picture->height, this->drawFlags | 0x14);

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

    if (phase == 0x18)
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
            this->unk2A68 = 2;
            this->unk2A6A = 0x18;
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
    u32 flags = this->drawFlags & 0x80000003u;
    this->unk26A4 = flags;
    flags |= 0x10;
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
// otherwise the hero's evasion (+0x2A93) is rolled, never right after a success, forced after 24 failures in a row and
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

    if (state.evadeDisabled == 1)
    {
        return 0;
    }

    if (static_cast<i32>(this->status) < 0)
    {
        return static_cast<u16>(services.random.Below(10) + 1) <= 9 ? success() : 0;
    }

    const u8 skill = this->unk2A93;

    if (skill == 0)
    {
        return 0;
    }

    if (chance != 0)
    {
        return static_cast<u16>(services.random.Below(10)) < chance ? success() : 0;
    }

    if (state.evadeJustSucceeded == 1)
    {
        state.evadeJustSucceeded = 0;
        return 0;
    }

    const u8 streak = state.evadeStreak;
    bool hit = false;

    if (streak >= 0x18)
    {
        hit = true;
    }
    else if (streak >= 0x14)
    {
        hit = static_cast<u16>(services.random.Below(3)) == 2;
    }
    else if (skill <= 6)
    {
        hit = static_cast<u16>(services.random.Below(10)) >= 8 &&
              static_cast<u16>(services.random.Below(10)) < this->unk2A93;
    }
    else
    {
        hit = static_cast<u16>(services.random.Below(10)) < this->unk2A93;
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
    return this->defeated == 1 || (this->marks & 0x2000) != 0;
}

// swd32.exe 0x0047CEA0
int BattleUnit::StatusBit13() const
{
    return (this->marks >> 13) & 1;
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
        const bool below = ++this->targetCount < 0x23;
        this->unk2AD8 = 1;

        if (!below)
        {
            this->targetCount = 0x22;
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
    if (tenths >= -10)
    {
        value = static_cast<i16>(value + tenths * value / 10);
        return 0;
    }

    value = static_cast<i16>(value * static_cast<i8>(tenths + 10) / 10);
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
    if ((this->marks & 0x42) != 0)
    {
        return 0;
    }

    return this->unk2AEC;
}

// swd32.exe 0x0047D810: mark 0x200 also darkens the unit (brightLevel -32)
int BattleUnit::SetStatus(int bits)
{
    if (static_cast<u16>(bits) == 0x200)
    {
        this->brightLevel = -0x20;
    }

    this->marks = static_cast<u16>(this->marks | static_cast<u16>(bits));
    return bits;
}

// swd32.exe 0x0047D830: clearing mark 0x1000 also ends the guard
int BattleUnit::ClearStatus(int bits)
{
    this->marks = static_cast<u16>(this->marks & static_cast<u16>(~bits));

    if ((bits & 0x1000) != 0)
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
    return static_cast<int>((this->command.actionFlags >> 11) & 1);
}

// swd32.exe 0x0047D8B0: the speed of the command when its bit 15 is set
i16 BattleUnit::ActSpeed() const
{
    const i16 speed = this->command.unk1E;
    return speed >= 0 ? i16{0} : static_cast<i16>(speed & 0x7FFF);
}

// swd32.exe 0x0047D8D0
int BattleUnit::ActFlag0() const
{
    return static_cast<int>(this->command.actionFlags & 1);
}

// swd32.exe 0x0047D8E0
int BattleUnit::ActFlag13() const
{
    return static_cast<int>((this->command.actionFlags >> 13) & 1);
}

// swd32.exe 0x0047D8F0
int BattleUnit::ActFlag14() const
{
    return static_cast<int>((this->command.actionFlags >> 14) & 1);
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
    return this->guarding == 1 || (this->marks & 0x1000) != 0;
}

// swd32.exe 0x0047D950: heroes set the current and maximum hit points, spirit and stamina of their party record
// (positive values; -1 refills the current value); enemies take the hit points as their maximum and current value
void BattleUnit::SetVitals(i16 hitPoints, i16 spirit, i16 stamina)
{
    if (this->kind == BattleUnit::kHero)
    {
        HeroRecord* record = this->hero;

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

// swd32.exe 0x0047DA10: positive values replace the attack, defence and agility (a hero's battle copy, not checked
// for null in the original; an enemy's data)
i16 BattleUnit::SetAttributes(i16 attack, i16 defense, i16 agility)
{
    if (this->kind == BattleUnit::kHero)
    {
        HeroRecord* record = this->heroStats;

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
    if (this->kind != BattleUnit::kHero && this->data != nullptr)
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

}  // namespace swd3
