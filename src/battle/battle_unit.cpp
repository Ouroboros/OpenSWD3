#include "battle/battle_unit.h"

#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "crt/msvc.h"
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

}  // namespace swd3
