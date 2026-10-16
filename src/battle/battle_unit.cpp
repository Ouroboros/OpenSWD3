#include "battle/battle_unit.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "audio/audio_manager.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "battle/battle_text.h"
#include "config/config.h"
#include "core/format.h"
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

// swd32.exe 0x004787F0: a defeated role has its hit points set to 0
int BattleUnit::StartDefeat(int defeat)
{
    this->defeated = defeat;
    this->unk2ABC = 1;

    if (this->defeatPose == 0)
    {
        this->defeatPose = 6;
    }

    if (this->kind == BattleUnit::kPartyRole && this->roleRecord != nullptr)
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

    if (result == BattleUnit::kPartyRole)
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

// swd32.exe 0x0047D950: roles set the current and maximum hit points, spirit and stamina of their party record
// (positive values; -1 refills the current value); enemies take the hit points as their maximum and current value
void BattleUnit::SetVitals(i16 hitPoints, i16 spirit, i16 stamina)
{
    if (this->kind == BattleUnit::kPartyRole)
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
    if (this->kind == BattleUnit::kPartyRole)
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
    if (this->kind != BattleUnit::kPartyRole && this->data != nullptr)
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
constexpr int kAnnounceFlags = BattleMessage::kBlackBar | BattleMessage::kCentered;
constexpr int kFramedFlags = BattleMessage::kFrame | BattleMessage::kCentered;
constexpr int kAnnounceSound = 0x117;
constexpr i32 kEffectAct = 0x2AF8;
constexpr u16 kEffectIcons = 0x241D;
constexpr u16 kDamageTarget = 0x235E;
constexpr u16 kHealTarget = 0x246F;

// The value of a skill script: the word at +0xE.
u16 SkillValue(const SkillScript& skill)
{
    return static_cast<u16>(skill.unk0C >> 16);
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
    field = static_cast<Field>(static_cast<u16>(value + sign * (10 * value / 100)));
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
void BattleUnit::Announce(BattleServices& services, const char* format, int flags)
{
    const std::string text = WinFormat(format, {this->displayName != nullptr ? this->displayName : ""});
    std::array<char, 64>& buffer = services.state.text;
    const std::size_t length = std::min<std::size_t>(text.size(), kBattleTextLimit);
    std::memcpy(buffer.data(), text.data(), length);
    buffer[length] = '\0';
    services.messages.Add(kAnnounceX, kAnnounceY, kAnnounceFrames, buffer.data(), flags);
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
    const int top = this->placement.y - 0x14;
    const int left = this->placement.x - 0x14;
    std::array<i16, 4>& phases = this->guardPhases;

    if (phases[0] == 9 && phases[1] == 9 && phases[2] == 9 && phases[3] == 9)
    {
        const i16 rise = this->guardRise;

        if (rise >= 0x18)
        {
            phases = {};
            this->guardRise = 0;
            hud.digitMode = 0;
            this->marks = static_cast<u16>(this->marks & 0xFFFE);
            this->action = 0;
            this->unk26A8 = 0;
            this->unk26AC = 0;
            return;
        }

        if (this->held == 0)
        {
            this->guardRise = static_cast<i16>(rise + 4);
        }

        u32 offset = 0;

        for (int letter = 0; letter < 4; ++letter)
        {
            const i16 current = this->guardRise;
            DrawOptions& options = services.painter.Options();
            options.tintRed = -current;
            options.tintGreen = -current;
            options.tintBlue = -current;
            options.scaleHeight = current + 0x1A;
            offset += hud.DrawDigit(0, letter, static_cast<u16>(offset) + left, top - current);
        }

        return;
    }

    hud.digitMode = 0;
    u32 offset = hud.DrawDigit(0, 0, left, top - BattleUnit::kPopOffsets[static_cast<std::size_t>(phases[0])]);

    for (std::size_t letter = 1; letter < 4 && phases[0] > 0; ++letter)
    {
        offset += hud.DrawDigit(
            0,
            static_cast<int>(letter),
            static_cast<u16>(offset) + left,
            top - BattleUnit::kPopOffsets[static_cast<std::size_t>(phases[letter])]
        );

        if (this->held == 0)
        {
            ++phases[letter];
        }

        if (phases[letter] > 9)
        {
            phases[letter] = 9;
        }
    }

    if (++phases[0] > 9)
    {
        phases[0] = 9;
    }
}

// swd32.exe 0x0047D640: splits `amount` into the four digits of the current target slot (a set bit 15 negates it).
// While the barrier is up against a damage number, a hit only counts the barrier down and ends it with
// "抵抗所有攻擊解除".
void BattleUnit::SetDamageDigits(BattleServices& services, int amount)
{
    if ((this->marks & 0x8000) != 0)
    {
        return;
    }

    const u16 slot = this->targetCount;

    if (this->pushedTargets[slot] == kDamageTarget && (this->status & kRaiseBlocked) != 0 && (amount & 0x8000) == 0)
    {
        if (--this->barrierTurns != 0)
        {
            return;
        }

        this->Announce(services, battle_text::kResistAllEnd, kAnnounceFlags);
        this->status &= ~kRaiseBlocked;
        return;
    }

    if ((amount & 0x8000) != 0)
    {
        amount = -amount;
    }

    std::array<i16, 4>& digits = this->damageDigits[slot];
    digits[0] = static_cast<i16>(amount / 1000);
    amount -= 1000 * digits[0];
    digits[1] = static_cast<i16>(amount / 100);
    amount -= 100 * digits[1];
    digits[2] = static_cast<i16>(amount / 10);
    digits[3] = static_cast<i16>(amount - static_cast<u16>(static_cast<u16>(digits[2]) * 10));
}

// swd32.exe 0x0047DBD0: the ailment of `skill` lands unless it is bit 13 (mark 0x20 instead) or the skill has flag 7,
// or a roll of 1..10 exceeds the ailment chance. An effect of the same kind (the search starts with the head node of
// the unit) only gets the new turns and value; kind bit 15 adds nothing new. 1 when it landed.
int BattleUnit::AddAilment(BattleServices& services, const SkillScript& skill)
{
    const u16 kind = skill.ailment;

    if ((kind & 0x2000) != 0)
    {
        this->marks = static_cast<u16>(this->marks | 0x20);
        return 0;
    }

    if (kind == 0 || (skill.flags & 0x80) != 0)
    {
        return 0;
    }

    const int roll = static_cast<int>(services.random.Below(10)) + 1;

    if (this->kind == BattleUnit::kPartyRole)
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

    if ((skill.ailment & 0x8000) != 0 || found)
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
        case 1:
            SetEffectIcon(node, 1, 2);
            this->status |= 1;
            this->marks = static_cast<u16>(this->marks | 0x40);
            break;

        case 2:
            SetEffectIcon(node, 7, 8);
            this->status |= 2;
            this->marks = static_cast<u16>(this->marks | 2);
            break;

        case 4:
            this->status |= 4;
            node.icon = 3;
            this->marks = static_cast<u16>(this->marks | 0x40);
            break;

        case 8:
            SetEffectIcon(node, 0, 4);
            this->status |= 8;
            this->marks = static_cast<u16>(this->marks | 0x40);
            break;

        case 0x10:
            SetEffectIcon(node, 5, 5);
            this->status |= 0x10;
            this->marks = static_cast<u16>(this->marks | 0x100);
            break;

        case 0x20:
            SetEffectIcon(node, 4, 9);
            this->status |= 0x20;
            break;

        case 0x40:
            SetEffectIcon(node, 3, 6);
            this->status |= 0x40;
            break;

        case 0x80:
            SetEffectIcon(node, 2, 7);
            this->status |= 0x80;
            break;

        case 0x100:
            SetEffectIcon(node, 8, 0xA);
            this->status |= 0x100;
            break;

        case 0x200:
            SetEffectIcon(node, 6, 0xB);
            this->status |= 0x200;

            if (this->kind == BattleUnit::kPartyRole)
            {
                ScaleRoleStats(*this->battleRecord, -1);
            }

            break;

        case 0x1000:
            this->status |= 0x1000;
            node.icon = 1;
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

    if (bits == 0 && (skill.flags & 8) == 0)
    {
        return 0;
    }

    if ((bits & 0x4000000) != 0)
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

        if ((skill.flags & 8) != 0)
        {
            node->element = skill.element;
        }
    }

    const char* resistUp = nullptr;

    switch (skill.element)
    {
        case 1:
            resistUp = battle_text::kFireResistUp;
            break;

        case 2:
            resistUp = battle_text::kWaterResistUp;
            break;

        case 5:
            resistUp = battle_text::kPoisonResistUp;
            break;

        case 6:
            resistUp = battle_text::kLightResistUp;
            break;

        case 7:
            resistUp = battle_text::kDarkResistUp;
            break;

        case 9:
            resistUp = battle_text::kPhysicalResistUp;
            break;

        default:
            break;
    }

    if (resistUp != nullptr)
    {
        this->Announce(services, resistUp, kAnnounceFlags);
    }

    const char* text = nullptr;

    switch (skill.statusBits)
    {
        case 0x80000:
            node->act.action = 0;
            this->status |= 0x100000;
            text = battle_text::kDefenseDown;
            break;

        case 0x100000:
            node->act.action = 0;
            this->status |= 0x8000000;
            text = battle_text::kSpeedDown;
            break;

        case 0x2000000:
            node->act.action = 0;
            this->status |= 0x4000000;

            if (this->kind != 0)
            {
                text = battle_text::kBarrierStart;
            }

            break;

        case 0x8000000:
            node->act.action = 0;
            this->status |= 0x1000000;
            text = battle_text::kAttackDown;
            break;

        case 0x10000000:
            node->act.action = 0;
            this->marks = static_cast<u16>(this->marks | 0x80);

            if (this->kind != 0)
            {
                text = battle_text::kInvisibleStart;
            }

            break;

        case 0x20000000:
            node->act.action = 0;
            this->status |= 0x40000000;
            text = battle_text::kDefenseUp;
            break;

        case 0x40000000:
            node->act.action = 0;
            this->status |= 0x20000000;
            text = battle_text::kSpeedUp;
            break;

        case 0x80000000:
            node->act.action = 0;
            this->status |= 0x10000000;
            text = battle_text::kAttackUp;
            break;

        default:
            break;
    }

    if (text != nullptr)
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
        case 1:
            bits = 0x1000000;
            break;

        case 2:
            bits = 0x800000;
            break;

        case 6:
            bits = 0x400000;
            break;

        case 7:
            bits = 0x200000;
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

    this->PushTarget(kDamageTarget);

    if ((this->pendingDamage & 0x8000) != 0)
    {
        this->PushTarget(kHealTarget);
    }

    this->SetDamageDigits(services, static_cast<i16>(this->pendingDamage));
    this->StepTargetCount(1);

    if ((this->pendingDamage & 0x8000) == 0)
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

    if ((this->status & 4) != 0)
    {
        const u32 flags = (this->drawFlags & 0x80000000u) | (this->drawFlags & 3) | 0x28;
        this->unk26A4 = flags;

        if (body != nullptr)
        {
            services.painter.SetSource(body->data.data());
            services.painter.DrawPicture(x, y, body->width, body->height, flags);
        }

        return;
    }

    if ((this->status & 0x1000) != 0)
    {
        this->unk26A4 = this->drawFlags & 0x80000003u;
        DrawOptions& options = services.painter.Options();
        options.tintRed = -12;
        options.tintGreen = 0;
        options.tintBlue = -12;

        if (body != nullptr)
        {
            services.painter.SetSource(body->data.data());
            services.painter.DrawPicture(x, y, body->width, body->height, this->unk26A4 | 0x10);
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

// swd32.exe 0x0047E950: with `clearAll` 0, every effect whose turns ran out loses its status and is announced, then
// freed; the effect following a freed one is not looked at in this pass. Otherwise all effects are freed and the
// status cleared.
void BattleUnit::ExpireEffects(BattleServices& services, int clearAll)
{
    if (clearAll != 0)
    {
        this->marks = static_cast<u16>(this->marks & 0xFEBD);
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
                case 1:
                    this->status &= ~1u;
                    clearHeld = (this->status & 0xC) == 0;
                    this->held = 0;
                    break;

                case 2:
                    this->marks = static_cast<u16>(this->marks & ~2);
                    this->status &= ~2u;
                    break;

                case 4:
                    this->status &= ~4u;
                    clearHeld = (this->status & 9) == 0;
                    this->held = 0;
                    break;

                case 8:
                    this->status &= ~8u;
                    clearHeld = (this->status & 5) == 0;
                    this->held = 0;
                    break;

                case 0x10:
                    this->marks = static_cast<u16>(this->marks & ~0x100);
                    this->status &= ~0x10u;
                    break;

                case 0x20:
                case 0x40:
                case 0x80:
                case 0x100:
                case 0x1000:
                    this->status &= ~node->bits;
                    break;

                case 0x200:
                    this->status &= ~0x200u;

                    if (this->kind == BattleUnit::kPartyRole)
                    {
                        ScaleRoleStats(*this->battleRecord, 1);
                    }

                    break;

                case 0x80000:
                    this->Announce(services, battle_text::kDefenseDownEnd, kAnnounceFlags);
                    this->status &= ~0x100000u;
                    break;

                case 0x100000:
                    this->Announce(services, battle_text::kSpeedDownEnd, kAnnounceFlags);
                    this->status &= ~0x8000000u;
                    break;

                case 0x200000:
                    this->Announce(services, battle_text::kDarkAttackUpEnd, kAnnounceFlags);
                    break;

                case 0x400000:
                    this->Announce(services, battle_text::kLightAttackUpEnd, kAnnounceFlags);
                    break;

                case 0x800000:
                    this->Announce(services, battle_text::kIceAttackUpEnd, kFramedFlags);
                    break;

                case 0x1000000:
                    this->Announce(services, battle_text::kFireAttackUpEnd, kFramedFlags);
                    break;

                case 0x2000000:
                    this->Announce(services, battle_text::kBarrierEnd, kAnnounceFlags);
                    this->status &= ~0x4000000u;
                    break;

                case 0x8000000:
                    this->Announce(services, battle_text::kAttackDownEnd, kAnnounceFlags);
                    this->status &= ~0x1000000u;
                    this->unk26CC &= ~0x100u;
                    break;

                case 0x10000000:
                    this->marks = static_cast<u16>(this->marks | 0x400);
                    this->status &= 0x7FFFFFFF;
                    this->unk2958 = 0;

                    if (this->kind != 0)
                    {
                        this->Announce(services, battle_text::kInvisibleEnd, kAnnounceFlags);
                    }

                    break;

                case 0x20000000:
                    this->Announce(services, battle_text::kDefenseUpEnd, kAnnounceFlags);
                    this->status &= ~0x40000000u;
                    break;

                case 0x40000000:
                    this->Announce(services, battle_text::kSpeedUpEnd, kAnnounceFlags);
                    this->status &= ~0x20000000u;
                    break;

                case 0x80000000:
                    this->Announce(services, battle_text::kAttackUpEnd, kAnnounceFlags);
                    this->status &= ~0x10000000u;
                    break;

                default:
                    break;
            }

            if (clearHeld)
            {
                this->marks = static_cast<u16>(this->marks & ~0x40);
            }

            static constexpr std::array<const char*, 10> kResistEnd = {
                nullptr,
                battle_text::kResistFireEnd,
                battle_text::kResistIceEnd,
                battle_text::kResistWindEnd,
                battle_text::kResistEarthEnd,
                battle_text::kResistPoisonEnd,
                battle_text::kResistLightEnd,
                battle_text::kResistDarkEnd,
                battle_text::kResistThunderEnd,
                battle_text::kResistPhysicalEnd,
            };

            if (node->element < kResistEnd.size() && kResistEnd[node->element] != nullptr)
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
    if (hitPoints > 0 && ((this->marks & 0x8000) != 0 || (this->status & kRaiseBlocked) != 0))
    {
        return 0;
    }

    if (this->defeated == 1 || this->EffectFlag3() == 1)
    {
        return 0;
    }

    if (this->kind != BattleUnit::kPartyRole)
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
        this->unk2ABC = 1;
        this->unk2958 = 0;

        if (pose == 0)
        {
            this->defeatPose = static_cast<u8>(services.random.Below(10) + 1);
        }

        return 1;
    }

    RoleRecord& record = *this->roleRecord;
    i32& gold = services.party.variables[0];

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
    this->unk2ABC = 1;

    if (this->unk2B00 == 1)
    {
        this->defeatPose = 6;
    }

    return 1;
}

// swd32.exe 0x0047F340
int BattleUnit::EffectFlag2() const
{
    return static_cast<int>((this->equipmentFlags >> 2) & 1);
}

// swd32.exe 0x0047F360
int BattleUnit::EffectFlag3() const
{
    return static_cast<int>((this->equipmentFlags >> 3) & 1);
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

}  // namespace swd3
