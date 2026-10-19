#include "battle/battle_unit.h"

#include <algorithm>
#include <cstdlib>
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
#include "battle/battle_unit_constants.h"
#include "battle/smooth/smooth_unit.h"
#include "config/config.h"
#include "core/format.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/paint_record.h"
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

namespace swd3::smooth
{

namespace
{

constexpr i32 kRoleHomeAction = 0x32;
constexpr i32 kHomePose = 0x28;
constexpr i32 kHomePose1 = 0x30;  // an enemy with unk2A8C 1
constexpr i32 kHomePose2 = 0x31;  // and with kSecondHomePose
constexpr i32 kRoleWalkAction = 0x2A;
constexpr i16 kWeakShare = 4;  // a role at a quarter of its hit points or less looks weak
constexpr i32 kWeakAction = 0x26;
constexpr i16 kForcedPlaceX = 0x140;
constexpr i16 kForcedPlaceY = 0x136;
constexpr i32 kReadyAction = 0x2C;
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

constexpr int kNumberAbove = 0x1E;
constexpr int kDigitWidth = 0xC;
constexpr i16 kLastHop = 9;
constexpr i16 kNumberFadeStep = 2;
constexpr int kNumberScaleBase = 0x1A;
constexpr int kGuardTextOffset = 0x14;  // the guard text starts left of and above the unit
constexpr i16 kGuardPhaseEnd = 9;       // the last phase of a guard text letter
constexpr i16 kGuardTextRise = 0x18;    // pixels the guard text rises before it is gone
constexpr i16 kGuardTextStep = 4;       // pixels the guard text rises a frame
constexpr int kGuardTextHeight = 0x1A;
}  // namespace

UnitFrame::UnitFrame(BattleUnit& unit)
    : unit(unit)
{
}

// swd32.exe 0x0047CC60: a red and green tint pulse rising 2..24 and falling back over the shadow picture. The
// rebuild's continuous motion keeps the step here, the tint it gives back (0 at the turn), and the draw in IdleDraw.
int UnitFrame::FlashStep()
{
    BattleUnit& unit = this->unit;
    const u16 phase = unit.unk2A68;

    if (phase == BattleUnit::FlashPeak)
    {
        const u16 fall = unit.unk2A6A;

        if (fall == 0)
        {
            unit.unk2A68 = BattleUnit::FlashStart;
            unit.unk2A6A = BattleUnit::FlashPeak;
            return 0;
        }

        if (unit.held == 0)
        {
            unit.unk2A6A = static_cast<u16>(fall - 1);
        }

        return unit.unk2A6A;
    }

    if (unit.held == 0)
    {
        unit.unk2A68 = static_cast<u16>(phase + 1);
    }

    return unit.unk2A68;
}

// swd32.exe 0x00483700: the overlay picture (`frame`, `picture` - 1; nothing when either is 0) drawn additively at
// (x + offsetX, y + offsetY), mirrored like the unit: a facing unit measures offsetX (when not 0) from the right edge
// of its current picture, less the overlay's width. The rebuild's continuous motion keeps the choice here (at the
// walk's place in `view`), the draw in DrawWalk.
void UnitFrame::ChooseOverlay(
    BattleServices& services,
    u16 frame,
    u16 picture,
    int offsetX,
    int offsetY,
    WalkView& view
)
{
    BattleUnit& unit = this->unit;

    if (frame == 0 || picture == 0)
    {
        return;
    }

    unit.overlayX = static_cast<u16>(offsetX);
    unit.unk2554 = services.archive.GetPicture(frame, static_cast<u16>(picture - 1));

    // A missing picture is read through a null pointer in the original.
    if (unit.unk2554 == nullptr)
    {
        return;
    }

    const bool forced = unit.unk2B00 == 1 || unit.unk2B04 == 1;

    auto mirror = [&unit, offsetX]
    {
        if (offsetX != 0 && unit.picture != nullptr)
        {
            unit.overlayX = static_cast<u16>(unit.picture->width - offsetX - unit.unk2554->width);
        }
    };

    if (forced && unit.facing == 1)
    {
        mirror();
    }

    if (unit.facing == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        mirror();
    }

    view.overlay = {frame, static_cast<u16>(picture - 1), true};
    view.overlayX = view.x + static_cast<i16>(unit.overlayX);
    view.overlayY = view.y + offsetY;
    view.overlayFlags = unit.frameFlags | Painter::Additive;
}

// What ReturnHome decides of the frame: the acts stepped, the line walked, the sound, the arrival and the overlay
// chosen; the view keeps no picture when it draws nothing.
int UnitFrame::HomeAdvance(BattleServices& services, const BattleUnit& target, i32 id, i32 action, WalkView& view)
{
    BattleUnit& unit = this->unit;

    if (unit.kind != 0)
    {
        if ((unit.unk26CC & kWalksHome) == 0 &&
            (unit.command.flags & (UnitCommand::UnkFlag8 | UnitCommand::UnkFlag20)) == 0)
        {
            return 1;
        }
    }
    else if ((unit.command.flags & UnitCommand::UnkFlag10) == 0)
    {
        return 1;
    }

    if (unit.unk2B00 == 1 || unit.unk2B04 == 1)
    {
        return 1;
    }

    unit.unk2AAC = 1;
    ActState& act = unit.attackAct;
    ActState& pose = unit.recoilAct;
    act.hold = 0;

    if (unit.held == 1)
    {
        act.hold = 1;
    }

    act.id = unit.actId;
    act.action = unit.kind != 0 ? kRoleHomeAction : kEnemyHomeAction;

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

    unit.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr)
    {
        return 0;
    }

    services.painter.SetSource(unit.picture->data.data());
    pose.id = unit.actId;
    pose.action = kHomePose;

    if (unit.kind == 0)
    {
        if (unit.unk2A8C == 1)
        {
            pose.action = kHomePose1;
        }

        if (unit.unk2A8C == kSecondHomePose)
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
    const BattleUnit::Stride stride = unit.HomeStride(services, target, act, pose, unit.throwSteps + 1);
    unit.actOffsetX = stride.offsetX;
    unit.frameFlags = stride.flags;
    unit.unk2670 = stride.poseX;
    LineWalker& line = unit.flight;
    line = stride.line;
    unit.throwSteps = stride.steps;

    if (!stride.arrived && PaintRecord::Active() != nullptr)
    {
        const ActState nextAct = services.acts.Peek(act);
        const BattleUnit::Stride next =
            unit.HomeStride(services, target, nextAct, services.acts.Peek(pose), unit.throwSteps + 1);
        view.motion = BattleUnit::StrideMotion(stride, act, next, nextAct);
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.flags = invisible ? unit.frameFlags | Painter::SmearUnder : unit.frameFlags;

    if (stride.arrived)
    {
        view.x = line.toX;
        view.y = line.toY;
        unit.throwSteps = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        unit.unk26CC &= ~kWalksHome;
        return 1;
    }

    view.x = line.fromX + line.x;
    view.y = line.fromY + line.y;

    if (!invisible)
    {
        view.shadow = true;
        view.shadowY = view.y + act.y;
        view.shadowTint = unit.unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
        unit.unk26A4 = (unit.frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    }

    this->ChooseOverlay(
        services,
        act.rotation,
        act.actionCode,
        static_cast<i16>(act.offsetX),
        static_cast<i16>(act.offsetY),
        view
    );
    return 0;
}

// What WalkToTarget decides of the frame: the acts stepped, the line walked, the sound, the arrival; false when it
// draws nothing.
bool UnitFrame::WalkAdvance(BattleServices& services, const BattleUnit& target, WalkView& view)
{
    BattleUnit& unit = this->unit;

    if (unit.unk2AD8 == 1)
    {
        return false;
    }

    if (unit.unk2B00 == 1 && (unit.command.flags & UnitCommand::FadesIn) != 0)
    {
        unit.commandA = unit.commandB;
        return false;
    }

    if ((unit.status & BattleUnit::Invisible) != 0 && unit.FadeInvisible(services, 0) == 1)
    {
        return false;
    }

    unit.unk2AAC = 1;
    ActState& act = unit.attackAct;
    ActState& pose = unit.recoilAct;
    act.id = unit.actId;
    act.action = unit.kind != 0 ? kRoleWalkAction : kEnemyHomeAction;
    act.hold = 0;

    if (unit.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return false;
    }

    unit.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr)
    {
        return false;
    }

    services.painter.SetSource(unit.picture->data.data());
    pose.id = unit.actId;
    pose.action = kHomePose;

    if (unit.kind == 0)
    {
        if (unit.unk2A8C == 1)
        {
            pose.action = kHomePose1;
        }

        if (unit.unk2A8C == kSecondHomePose)
        {
            pose.action = kHomePose2;
        }
    }

    if (services.acts.Run(pose) == 0)
    {
        return false;
    }

    const BattleUnit::Stride stride = unit.WalkStride(services, target, act, pose, unit.throwSteps + 1);
    unit.actOffsetX = stride.offsetX;
    unit.frameFlags = stride.flags;
    unit.unk2670 = stride.poseX;

    if (unit.kind == 1 && unit.unk2A8E != act.picture)
    {
        unit.unk2A8E = act.picture;
    }

    unit.throwSteps = stride.steps;
    LineWalker& line = unit.flight;
    line = stride.line;

    if (!stride.arrived && PaintRecord::Active() != nullptr)
    {
        const ActState nextAct = services.acts.Peek(act);
        const BattleUnit::Stride next =
            unit.WalkStride(services, target, nextAct, services.acts.Peek(pose), unit.throwSteps + 1);
        view.motion = BattleUnit::StrideMotion(stride, act, next, nextAct);
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.flags = unit.frameFlags;

    if (stride.arrived)
    {
        view.x = invisible ? line.toX - static_cast<i16>(unit.actOffsetX) : line.toX;
        view.y = line.toY;

        if (invisible)
        {
            view.flags |= Painter::SmearUnder;
        }

        unit.throwSteps = 0;
        unit.unk2A8E = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        unit.commandA = unit.commandB;
        return true;
    }

    view.x = line.fromX + line.x;
    view.y = line.fromY + line.y;

    if (invisible)
    {
        view.flags |= Painter::SmearUnder;
        return true;
    }

    view.shadow = true;
    view.shadowY = view.y + act.y;
    view.shadowTint = unit.unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
    unit.unk26A4 = (unit.frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    return true;
}

// A walk as WalkAdvance or HomeAdvance decided it: the shadow under the picture, the picture, the overlay.
void UnitFrame::DrawWalk(BattleServices& services, const WalkView& view)
{
    Painter& painter = services.painter;
    const Picture* walking = view.picture.Get(services.archive);

    if (walking == nullptr)
    {
        return;
    }

    const Picture& picture = *walking;
    painter.SetSource(picture.data.data());

    if (view.shadow)
    {
        const int shadowHeight = picture.height / kShadowShare;
        DrawOptions& options = painter.Options();
        options.scaleHeight = shadowHeight;
        options.shear = picture.height >> kShadowShearShift;
        options.tintRed = view.shadowTint;
        options.tintGreen = view.shadowTint;
        options.tintBlue = view.shadowTint;
        painter.DrawPicture(
            view.x,
            view.shadowY - shadowHeight,
            picture.width,
            picture.height,
            (view.flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow
        );
    }

    painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.flags);

    if (const Picture* overlay = view.overlay.Get(services.archive); overlay != nullptr)
    {
        painter.SetSource(overlay->data.data());
        painter.DrawPicture(view.overlayX, view.overlayY, overlay->width, overlay->height, view.overlayFlags);
    }
}

const Picture* PictureRef::Get(TswArchive& archive) const
{
    return this->shown ? archive.GetPicture(this->id, this->frame) : nullptr;
}

// What IdleFrame decides of the frame: the move, the pose, the act stepped, the barrier flash stepped, the sounds,
// the thrown act stepped and the overlay chosen; false when the unit is not drawn (hidden, staggering, busy, fading,
// or its act or picture missing).
bool UnitFrame::IdleAdvance(BattleServices& services, int paused, IdleView& view)
{
    BattleUnit& unit = this->unit;
    ActState& act = unit.mainAct;
    unit.unk2AF4 = paused;
    unit.MoveIdle(services);
    act.hold = 0;

    if (paused == 1 || (unit.marks & BattleUnit::Held) != 0)
    {
        act.hold = 1;
    }

    if ((act.target & ActState::Holds) != 0)
    {
        act.hold = 1;
    }

    const bool forced = unit.unk2B00 == 1 || unit.unk2B04 == 1;
    const bool shown = unit.defeated != 1 || (unit.kind != 0 && !forced) || unit.unk2AF8 != 0;

    if (!shown)
    {
        return false;
    }

    if ((unit.unk26B8 & kFlag26B8) != 0)
    {
        if (unit.HurtFrame(services) == 1)
        {
            unit.charging = 0;
            unit.ClearFlag26B8();
        }

        return false;
    }

    if (unit.unk2AAC == 1)
    {
        return false;
    }

    act.id = unit.actId;
    act.action = kShadowAction;
    unit.unk2A88 = static_cast<u16>(act.action);

    // 0x00478D13 calls nullsub_1 without arguments for a party role: nothing to log.
    if (unit.kind != 1)
    {
        act.action += unit.unk2684;
    }

    const RoleRecord* record = unit.roleRecord;

    if (unit.kind == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0 &&
        record->hitPoints <= record->maxHitPoints / kWeakShare)
    {
        act.action = kWeakAction;
    }

    if (unit.unk2A86 != 0 && unit.unk2AB4 == 1 && unit.unk2AC8 == 1)
    {
        if (unit.unk2B00 == 1)
        {
            unit.guarding = 0;
        }

        if (unit.unk2B04 == 1)
        {
            unit.placement.x = kForcedPlaceX;
            unit.placement.y = kForcedPlaceY;
        }

        unit.unk2670 = 0;

        if (unit.kind == 1)
        {
            unit.unk2674 = 0;
        }

        act.action = kReadyAction;

        if (unit.actId == kReadyAct)
        {
            act.action = unit.unk2690;
        }

        unit.unk2A88 = static_cast<u16>(act.action);

        if ((unit.unk2A86 & BattleUnit::UnkCommandFlag4000) != 0 ||
            (unit.unk2A86 & BattleUnit::UnkCommandFlag2000) != 0)
        {
            act.action = kItemReadyAction;
        }

        if ((unit.unk2A86 & BattleUnit::UnkCommandFlag8000) != 0 && act.waiting == 1 && unit.kind == 1)
        {
            act.waiting = 0;
            act.position = static_cast<u16>(act.position - kReadyRewind);
        }

        if ((unit.unk2A86 & BattleUnit::UnkCommandFlag400) != 0)
        {
            act.action = kItemReadyAction;
        }

        if ((unit.unk2A86 & BattleUnit::UnkCommandFlag800) != 0)
        {
            act.action = kAbsorbReadyAction;
        }

        if ((unit.unk2A86 & BattleUnit::UnkCommandFlag200) != 0)
        {
            return false;
        }
    }

    if (unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        if (unit.unk26D2 == kGuardPose1)
        {
            unit.unk2AD4 = 0;
            act.action = kGuardAction1;
        }

        if (unit.unk26D2 == kGuardPose2)
        {
            act.action = kGuardAction2;

            if (act.waiting == 1)
            {
                act.waiting = 0;
                act.position = static_cast<u16>(act.position - kReadyRewind);
                act.view = 0;
            }
        }

        if (unit.unk26D2 == kGuardPose4)
        {
            act.action = kGuardAction4;
        }

        if (unit.unk26D2 == kGuardPose8)
        {
            act.action = kGuardAction8;
        }
    }

    if (unit.kind == 1 && unit.defeated == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        act.action = kDefeatedShadowAction;
    }

    if (unit.action != 0)
    {
        act.action = unit.action;
    }

    if ((act.target & ActState::FollowsUp) != 0)
    {
        unit.commandA = unit.commandB;
        act.target = 0;

        if ((unit.unk2A86 & BattleUnit::UnkCommandFlag8000) == 0 ||
            (unit.unk2A86 & BattleUnit::UnkCommandFlag4000) != 0)
        {
            act.hold = 0;
        }

        if ((unit.command.actionFlags & UnitCommand::DelaysFadeIn) != 0)
        {
            unit.marks = static_cast<u16>(unit.marks | BattleUnit::UnkMark8);
            unit.recoilAct = act;
        }
    }

    if (services.acts.Run(act) == 0)
    {
        return false;
    }

    unit.shadowPicture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.shadowPicture == nullptr)
    {
        return false;
    }

    const Picture& picture = *unit.shadowPicture;
    services.painter.SetSource(picture.data.data());
    unit.originX = static_cast<u16>(act.x);
    unit.drawFlags = act.flags;
    unit.actAnchorX = act.anchorX;

    auto mirror = [&unit, &act, &picture]
    {
        unit.drawFlags = act.flags ^ Painter::Mirror;

        if (act.x != 0)
        {
            unit.originX = static_cast<u16>(picture.width - static_cast<u16>(act.x));
        }

        if (act.anchorX != 0)
        {
            unit.actAnchorX = static_cast<u16>(picture.width - act.anchorX);
        }
    };

    if (forced)
    {
        if (unit.facing == 1)
        {
            mirror();
        }

        if ((unit.drawFlags & ActState::NoBlend) == 0)
        {
            unit.drawFlags |= Painter::Additive;
        }
    }

    if (unit.facing == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        mirror();
    }

    const u16 marks = unit.marks;

    if ((marks & BattleUnit::UnkMark8) != 0)
    {
        unit.FadeMainOut(services);
        return false;
    }

    if ((marks & BattleUnit::UnkMark200) != 0)
    {
        unit.FadeMainIn(services);
        return false;
    }

    if (unit.commandA == 0 || unit.commandA == BattleUnit::UnkCommand1C)
    {
        if ((marks & BattleUnit::InvisibleMark) != 0)
        {
            unit.FadeInvisible(services, 0);
            return false;
        }

        if ((marks & BattleUnit::UnkMark400) != 0)
        {
            unit.FadeInvisible(services, 1);
            return false;
        }
    }

    view.picture = {act.frame, act.picture, true};
    view.x = unit.unk26A8 + unit.placement.x - static_cast<i16>(unit.originX);
    view.y = unit.placement.y - unit.unk26AC - act.y;
    view.mirrored = unit.drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
    view.flags = unit.drawFlags;
    view.motion =
        PaintRecord::Active() != nullptr ? unit.IdleMotion(services, act, view.x, view.y) : std::array<int, 3>{0, 0, 0};
    view.body = unit.guarding == 0 && (marks & BattleUnit::GuardingMark) == 0;

    if (view.body)
    {
        const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
        view.shadow = unit.drawAtSaved != kRising && unit.drawAtSaved != kDropping && !invisible;
        view.shadowX = unit.placement.x - static_cast<i16>(unit.originX);
        view.shadowY = unit.placement.y;
        view.shadowTint = unit.unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
        view.whiteOutline = (unit.status & kOutlinedStatus) != 0;
        view.outline = unit.outline == 1;
        view.smear = invisible;
        view.alpha = (unit.drawFlags & ActState::Signal) == ActState::Signal ? act.signal : 0;
        view.signal = (unit.drawFlags & ActState::Signal) == ActState::Signal;
        view.wave = act.wait;
        view.held = (marks & BattleUnit::Held) != 0;
        view.flash = !invisible && (unit.status & BattleUnit::Barrier) != 0;
        view.gray = (marks & BattleUnit::UnkMark4) != 0;
        view.red = unit.commandA == BattleUnit::UnkCommand2;
        view.gold = unit.goldGuard == 1 && unit.unk26D2 == 0;
        view.goldValue = services.party.variables[kGoldVariable];
        view.goldX = unit.placement.x + kGoldLeft;
        view.goldY = unit.placement.y - act.y - kGoldAbove;

        if (view.flash)
        {
            view.flashTint = this->FlashStep();
            view.flashX = unit.placement.x - static_cast<i16>(unit.originX);
            view.flashY = unit.placement.y - unit.mainAct.y;
        }

        // What the draws leave in the unit (the outline colour, the last mode drawn with).
        if (view.whiteOutline)
        {
            unit.colorPair = kWhiteColorPair;
        }

        if (view.outline)
        {
            unit.colorPair = BattleUnit::OutlineColorPair;
        }

        if (view.shadow)
        {
            unit.unk26A4 = view.mirrored | Painter::Shadow;
        }

        if (view.smear || view.flash || view.gray || view.red)
        {
            unit.unk26A4 = view.mirrored;
        }

        if (act.view != 0)
        {
            services.audio.PlaySound(act.view, services.config.soundVolume);
            services.audio.SetSoundPan(act.view, unit.placement.x >= kPanCentre ? kFadePan : -kFadePan);
        }

        act.view = 0;
    }

    if (unit.commandA == BattleUnit::UnkCommand6 && unit.kind == 1)
    {
        ActState& thrown = unit.recoilAct;
        thrown.action = 0;
        thrown.id = kThrownSpark;

        if (services.acts.Run(thrown) == 0)
        {
            view.thrownLost = true;
            return true;
        }

        unit.picture = services.archive.GetPicture(thrown.frame, thrown.picture);

        if (unit.picture != nullptr)
        {
            view.thrown = {thrown.frame, thrown.picture, true};
            view.thrownX = static_cast<i16>(act.anchorX) + unit.placement.x - thrown.x - static_cast<i16>(unit.originX);
            view.thrownY = static_cast<i16>(act.anchorY) + unit.placement.y - thrown.y - act.y;
            view.thrownFlags = thrown.flags;
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

    if (unit.guarding == 0 && (unit.marks & BattleUnit::GuardingMark) == 0 &&
        (unit.status & BattleUnit::UnkStatus4) == 0 && act.rotation != 0 && act.actionCode != 0)
    {
        unit.unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

        if (unit.unk2554 != nullptr)
        {
            unit.unk2698 = static_cast<i16>(act.offsetX);
            view.overlay = {act.rotation, static_cast<u16>(act.actionCode - 1), true};
            view.overlayX = unit.unk2698 + unit.placement.x - static_cast<i16>(unit.originX);
            view.overlayY = static_cast<i16>(act.offsetY) + unit.placement.y - unit.unk26AC - act.y;
            view.overlayFlags = unit.drawFlags | Painter::Additive;
        }
    }

    return true;
}

// What IdleFrame draws of the unit as IdleAdvance decided it: its shadow, outlines, body, barrier flash, grey and red
// copies, the gold, the thrown act and the overlay.
void UnitFrame::IdleDraw(BattleServices& services, const IdleView& view)
{
    Painter& painter = services.painter;
    const Picture* body = view.body ? view.picture.Get(services.archive) : nullptr;

    if (body != nullptr)
    {
        const Picture& picture = *body;
        painter.SetSource(picture.data.data());

        if (view.shadow)
        {
            const PaintRecord::Motion shadowMoving({view.motion[0], view.motion[2]});
            const int shadowHeight = picture.height / kShadowShare;
            DrawOptions& options = painter.Options();
            options.scaleHeight = shadowHeight;
            options.shear = picture.height >> kShadowShearShift;
            options.tintRed = view.shadowTint;
            options.tintGreen = view.shadowTint;
            options.tintBlue = view.shadowTint;
            painter.DrawPicture(
                view.shadowX,
                view.shadowY - shadowHeight,
                picture.width,
                picture.height,
                view.mirrored | Painter::Shadow
            );
        }

        if (view.whiteOutline)
        {
            painter.DrawOutline(
                view.x,
                view.y,
                picture.width,
                picture.height,
                view.mirrored,
                DrawArgument{.colorPair = kWhiteColorPair}
            );
        }

        if (view.outline)
        {
            painter.DrawOutline(
                view.x,
                view.y,
                picture.width,
                picture.height,
                view.mirrored,
                DrawArgument{.colorPair = BattleUnit::OutlineColorPair}
            );
        }

        painter.SetSource(picture.data.data());

        if (view.smear)
        {
            painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.mirrored | Painter::SmearUnder);
        }
        else
        {
            if (view.signal)
            {
                painter.Options().alpha = view.alpha;
            }

            if (view.wave != 0)
            {
                painter.SetWave(view.wave);
            }

            if (view.held)
            {
                painter.SetWave(0);
            }

            painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.flags);
            painter.SetWave(0);

            if (view.flash)
            {
                DrawOptions& options = painter.Options();
                options.tintRed = view.flashTint;
                options.tintGreen = view.flashTint;
                painter.SetSource(picture.data.data());
                painter.DrawPicture(
                    view.flashX,
                    view.flashY,
                    picture.width,
                    picture.height,
                    view.mirrored | Painter::TintUnder
                );
            }
        }

        if (view.gray)
        {
            painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.mirrored | Painter::GrayUnder);
        }

        if (view.red)
        {
            painter.Options().tintRed = kHurtFlash;
            painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.mirrored | Painter::TintUnder);
        }

        if (view.gold)
        {
            services.hud.DrawNumber(kGoldFont, view.goldValue, view.goldX, view.goldY);
        }
    }

    if (const Picture* thrown = view.thrown.Get(services.archive); thrown != nullptr)
    {
        painter.SetSource(thrown->data.data());
        painter.DrawPicture(view.thrownX, view.thrownY, thrown->width, thrown->height, view.thrownFlags);
    }

    if (const Picture* overlay = view.overlay.Get(services.archive); overlay != nullptr)
    {
        painter.SetSource(overlay->data.data());
        painter.DrawPicture(view.overlayX, view.overlayY, overlay->width, overlay->height, view.overlayFlags);
    }
}

// What DrawDamageNumbers decides of the frame: the delays counted down, the hops stepped, the extra number taken, the
// fades risen and the slots cleared; the draws in their order into the view.
int UnitFrame::NumbersAdvance(BattleServices& services, NumbersView& view)
{
    BattleUnit& unit = this->unit;

    if (unit.unk2AD8 == 0)
    {
        return 0;
    }

    auto clearMode = [&view]
    {
        view.steps.push_back(NumberStep{NumberStep::Kind::ClearMode});
    };

    auto fadeStep = [&view](int fade)
    {
        view.steps.push_back(NumberStep{.kind = NumberStep::Kind::Fade, .fade = fade});
    };

    auto digitStep = [&view](i16 font, int value, int x, int y, std::array<int, 2> motion)
    {
        view.steps.push_back(
            NumberStep{.kind = NumberStep::Kind::Digit, .font = font, .value = value, .x = x, .y = y, .motion = motion}
        );
    };

    const int x = unit.placement.x;
    const int y = unit.placement.y - kNumberAbove;

    for (u16 slot = unit.damageSlot; slot < unit.targetCount; ++slot)
    {
        std::array<i16, BattleUnit::DamageDigits>& digits = unit.damageDigits[slot];
        std::array<i16, BattleUnit::DamageDigits>& hops = unit.unk283C[slot];
        i16& fade = unit.numberFades[slot];
        const auto font = static_cast<i16>(unit.pushedTargets[slot]);

        if (unit.damageDelays[slot] != 0)
        {
            --unit.damageDelays[slot];
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
            clearMode();

            auto hopY = [y, &hops](std::size_t digit)
            {
                return y - BattleUnit::DigitHops[static_cast<std::size_t>(hops[digit])];
            };

            // The rebuild's smooth battle: each digit moves to its next hop, or to the first rise once all land.
            std::array<i16, BattleUnit::DamageDigits> nextHops = hops;

            for (std::size_t digit = 0; unit.held == 0 && digit < nextHops.size(); ++digit)
            {
                if (digit > 0 && nextHops[digit - 1] <= 0)
                {
                    break;
                }

                nextHops[digit] = static_cast<i16>(std::min<int>(nextHops[digit] + 1, kLastHop));
            }

            const bool landsNext = std::all_of(
                nextHops.begin(),
                nextHops.end(),
                [](i16 hop)
                {
                    return hop == kLastHop;
                }
            );

            auto drawHop = [&](std::size_t digit, int at)
            {
                const int risesNext = fade + (unit.held == 0 ? kNumberFadeStep : 0);
                const int heightNext =
                    landsNext ? risesNext : BattleUnit::DigitHops[static_cast<std::size_t>(nextHops[digit])];
                const int height = BattleUnit::DigitHops[static_cast<std::size_t>(hops[digit])];
                digitStep(font, digits[digit], at, hopY(digit), {0, height - heightNext});
            };

            if (digits[0] != 0)
            {
                drawHop(0, x - kDigitWidth);
            }

            if ((digits[1] != 0 || digits[0] != 0) && hops[0] > 0)
            {
                drawHop(1, x);
            }

            if (hops[1] > 0)
            {
                if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0)
                {
                    drawHop(2, x + kDigitWidth);
                }

                drawHop(3, x + 2 * kDigitWidth);
            }

            if (unit.held == 0)
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
        if (unit.extraNumber == 1)
        {
            unit.extraNumber = 0;

            if (unit.targetCount < BattleUnit::MaxTargets)
            {
                std::array<i16, BattleUnit::DamageDigits>& extra = unit.damageDigits[unit.targetCount];
                extra[0] = static_cast<i16>(services.state.unk53AEDC);
                extra[1] = static_cast<i16>(services.state.unk53AEE0);
                extra[2] = static_cast<i16>(services.state.unk53AEE4);
                ++unit.targetCount;
            }
        }

        if (fade >= kNumberFadeEnd)
        {
            hops.fill(0);
            fade = 0;
            clearMode();
            ++unit.damageSlot;
            digits.fill(0);
            continue;
        }

        if (unit.held == 0)
        {
            fade = static_cast<i16>(fade + kNumberFadeStep);
        }

        const int risenY = y - fade;
        const bool clearedNext = fade >= kNumberFadeEnd;
        const std::array<int, 2> rising = {0, clearedNext || unit.held != 0 ? 0 : -kNumberFadeStep};
        fadeStep(fade);

        if (digits[0] != 0)
        {
            digitStep(font, digits[0], x - kDigitWidth, risenY, rising);
        }

        if (digits[0] != 0 || digits[1] != 0)
        {
            fadeStep(fade);
            digitStep(font, digits[1], x, risenY, rising);
        }

        fadeStep(fade);

        if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0)
        {
            digitStep(font, digits[2], x + kDigitWidth, risenY, rising);
        }

        digitStep(font, digits[3], x + 2 * kDigitWidth, risenY, rising);
        clearMode();
    }

    if (unit.damageSlot < unit.targetCount)
    {
        return 0;
    }

    clearMode();
    unit.targetCount = 0;
    unit.damageSlot = 0;
    unit.unk2AD8 = 0;
    return 1;
}

// What DrawDamageNumbers draws, as NumbersAdvance decided it.
void UnitFrame::DrawNumbers(BattleServices& services, const NumbersView& view)
{
    BattleHud& hud = services.hud;
    DrawOptions& options = services.painter.Options();

    for (const NumberStep& step : view.steps)
    {
        switch (step.kind)
        {
            case NumberStep::Kind::ClearMode:
                hud.digitMode = 0;
                break;

            case NumberStep::Kind::Fade:
                options.tintRed = -step.fade;
                options.tintGreen = -step.fade;
                options.tintBlue = -step.fade;
                options.scaleHeight = step.fade + kNumberScaleBase;
                break;

            case NumberStep::Kind::Digit:
            {
                const PaintRecord::Motion moving(step.motion);
                hud.DrawNumber(step.font, step.value, step.x, step.y);
                break;
            }
        }
    }
}

// What DrawGuardText decides of the frame: the letters popped in or risen, the mark cleared at the top.
void UnitFrame::GuardAdvance(GuardView& view)
{
    BattleUnit& unit = this->unit;

    if ((unit.marks & 1) == 0)
    {
        return;
    }

    const int top = unit.placement.y - kGuardTextOffset;
    view.left = unit.placement.x - kGuardTextOffset;
    auto& phases = unit.guardPhases;

    if (std::all_of(
            phases.begin(),
            phases.end(),
            [](i16 phase)
            {
                return phase == kGuardPhaseEnd;
            }
        ))
    {
        const i16 rise = unit.guardRise;

        if (rise >= kGuardTextRise)
        {
            phases = {};
            unit.guardRise = 0;
            view.clearMode = true;
            unit.marks = static_cast<u16>(unit.marks & ~BattleUnit::UnkMark1);
            unit.action = 0;
            unit.unk26A8 = 0;
            unit.unk26AC = 0;
            return;
        }

        if (unit.held == 0)
        {
            unit.guardRise = static_cast<i16>(rise + kGuardTextStep);
        }

        // The rebuild's smooth battle: the letters rise on next frame, unless they are gone then.
        const bool goneNext = unit.guardRise >= kGuardTextRise;
        view.fading = true;
        view.fade = unit.guardRise;
        view.letters = static_cast<int>(phases.size());

        for (std::size_t letter = 0; letter < phases.size(); ++letter)
        {
            view.y[letter] = top - unit.guardRise;
            view.motion[letter] = {0, goneNext || unit.held != 0 ? 0 : -kGuardTextStep};
        }

        return;
    }

    view.clearMode = true;
    // The rebuild's smooth battle: each letter moves to its next phase, or to the first rise once all have popped.
    auto nextPhases = phases;
    nextPhases[0] = std::min<i16>(static_cast<i16>(nextPhases[0] + 1), kGuardPhaseEnd);

    for (std::size_t letter = 1; letter < nextPhases.size() && phases[0] > 0; ++letter)
    {
        nextPhases[letter] =
            std::min<i16>(static_cast<i16>(nextPhases[letter] + (unit.held == 0 ? 1 : 0)), kGuardPhaseEnd);
    }

    const bool risesNext = std::all_of(
        nextPhases.begin(),
        nextPhases.end(),
        [](i16 phase)
        {
            return phase == kGuardPhaseEnd;
        }
    );

    auto popMotion = [&](std::size_t letter) -> std::array<int, 2>
    {
        const int height = BattleUnit::PopOffsets[static_cast<std::size_t>(phases[letter])];
        const int heightNext = risesNext ? unit.guardRise + (unit.held == 0 ? kGuardTextStep : 0)
                                         : BattleUnit::PopOffsets[static_cast<std::size_t>(nextPhases[letter])];
        return {0, height - heightNext};
    };

    view.letters = 1;
    view.y[0] = top - BattleUnit::PopOffsets[static_cast<std::size_t>(phases[0])];
    view.motion[0] = popMotion(0);

    for (std::size_t letter = 1; letter < phases.size() && phases[0] > 0; ++letter)
    {
        view.letters = static_cast<int>(letter) + 1;
        view.y[letter] = top - BattleUnit::PopOffsets[static_cast<std::size_t>(phases[letter])];
        view.motion[letter] = popMotion(letter);

        if (unit.held == 0)
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

// What DrawGuardText draws, as GuardAdvance decided it: the letters side by side from `left`, each placed after the
// width of the ones before it.
void UnitFrame::DrawGuard(BattleServices& services, const GuardView& view)
{
    BattleHud& hud = services.hud;

    if (view.clearMode)
    {
        hud.digitMode = 0;
    }

    u32 offset = 0;

    for (int letter = 0; letter < view.letters; ++letter)
    {
        const auto at = static_cast<std::size_t>(letter);
        const PaintRecord::Motion moving(view.motion[at]);

        if (view.fading)
        {
            DrawOptions& options = services.painter.Options();
            options.tintRed = -view.fade;
            options.tintGreen = -view.fade;
            options.tintBlue = -view.fade;
            options.scaleHeight = view.fade + kGuardTextHeight;
        }

        offset += hud.DrawDigit(0, letter, static_cast<u16>(offset) + view.left, view.y[at]);
    }
}

}  // namespace swd3::smooth
