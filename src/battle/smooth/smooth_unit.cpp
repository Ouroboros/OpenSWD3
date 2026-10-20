#include "battle/battle_unit.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <utility>

#include "audio/audio_manager.h"
#include "battle/battle_enemy.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_role.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "battle/battle_unit_constants.h"
#include "battle/smooth/smooth_projector.h"
#include "battle/smooth/smooth_unit.h"
#include "config/config.h"
#include "core/format.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/draw_placer.h"
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
constexpr i32 kGuardHurtAction = 0x27;
constexpr i32 kHurtAction = 0x25;
constexpr u16 kHurtFrames = 2;  // frames a stagger step lasts
constexpr int kHurtRed = 0xF;   // the red flash an enemy leaves behind
constexpr u16 kLastHurtStep = 5;
constexpr u32 kNudgeChance = 2;    // a nudged unit shakes in one frame of two
constexpr int kNudgeShift = 0x28;  // the shake moves the act by nudge - 0x28
constexpr int kUnkStatus1000Tint = -12;
constexpr i16 kRecoilFadeStep = 2;
constexpr i16 kRecoilFadeEnd = -0x20;  // DrawRecoil: the tint at which a waiting recoil act is over
constexpr i8 kAdditiveTrailFade = 3;   // an additive act's images fade by 3 a frame, the others by 1
constexpr i16 kAdditiveTrailLevel = 0x18;
constexpr i32 kQueuedMaxDamage = 0x270F;
constexpr int kTrailBlue = -12;
constexpr int kTrailCentre = 0xC;
constexpr u8 kTrailDelay = 2;  // a new image every third frame
constexpr i16 kTrailLevel = 0xC;
constexpr int kTrailSpacing = 0x1E;
constexpr int kTrailTintBase = 0x1F;
constexpr i16 kBlueFillStep = 4;
constexpr int kBlueFillTint = 0x10;
constexpr i32 kCommand1Action = 0x28;  // the fade in action after command 1
constexpr i16 kDarkFillStep = 0xE;
constexpr int kDarkFillTint = -0x18;
constexpr u16 kEffectIcons = 0x241D;
constexpr int kFadeInSound = 0x30;
constexpr int kFadeOutSound = 0x2F;
constexpr i16 kFadeOutStep = 2;
constexpr i16 kHalvesFast = 4;
constexpr i16 kHalvesPause = 8;
constexpr i16 kHalvesSlow = 9;
constexpr u16 kPose2 = 2;
constexpr i32 kPose2Action = 0x31;  // of pose 2
constexpr i32 kPoseAction = 0x30;   // the fade in action of a posed unit
constexpr int kQuarterDrop = 0x10;
constexpr i16 kRisingEnd = 0x1E0;
constexpr i16 kRisingStep = 0x1F;
constexpr int kRisingTintDivisor = -31;
constexpr int kRisingTintLift = 4;
constexpr u16 kRunSound = 0x2F;
constexpr int kShadowLeftShift = 5;
constexpr int kSinkAlphaBase = 0x1F;
constexpr i16 kSinkFadeFrom = -0x10;
constexpr int kSinkStep = 10;
constexpr i16 kSlideFadeFrom = -15;
constexpr i16 kSplitEnd = 0x20;
constexpr i16 kSplitStep = 2;
constexpr u16 kStepPicture = 1;  // the picture of the act that plays the step sound
constexpr u16 kStepSound = 0x10F;
constexpr i16 kWaveEnd = 0x2D;
constexpr int kWaveShare = 3;
constexpr i16 kZoomEnd = 0x1E;
constexpr i16 kZoomStep = 2;
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
constexpr i32 kProjectedAngle = 0x5A;
constexpr i16 kProjectedStep = 2;
constexpr i32 kProjectedTint = 5;
constexpr i32 kProjectedGrowth = 4;
constexpr i32 kWobbles = 6;
constexpr i16 kWobbleTop = 0x18;
constexpr i16 kWobbleBottom = -8;
constexpr i16 kWobbleRise = 4;
constexpr i32 kStripRight = 0x154;
constexpr i32 kStripWidth = 0x140;
constexpr u16 kStripSpeed = 0x18;
constexpr u16 kStripLife = 0x64;
constexpr u16 kStripDivisor = 0x28;
constexpr i32 kStripTint = 0xF;
constexpr int kShakeSpan = 3;
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

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.flags = invisible ? unit.frameFlags | Painter::SmearUnder : unit.frameFlags;

    if (stride.arrived)
    {
        view.x = line.toX;
        view.y = line.toY;
        view.place = {view.x + static_cast<i16>(unit.actOffsetX), view.y + act.y};
        view.who = &unit;
        unit.throwSteps = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        unit.unk26CC &= ~kWalksHome;
        return 1;
    }

    view.x = line.fromX + line.x;
    view.y = line.fromY + line.y;
    view.place = {view.x + static_cast<i16>(unit.actOffsetX), view.y + act.y};
    view.who = &unit;

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

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.flags = unit.frameFlags;

    if (stride.arrived)
    {
        view.x = invisible ? line.toX - static_cast<i16>(unit.actOffsetX) : line.toX;
        view.y = line.toY;
        view.place = {view.x + static_cast<i16>(unit.actOffsetX), view.y + act.y};
        view.who = &unit;

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
    view.place = {view.x + static_cast<i16>(unit.actOffsetX), view.y + act.y};
    view.who = &unit;

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
    const DrawPlacer::Holding holding(view.place[0], view.place[1], view.who);
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
    view.place = {unit.unk26A8 + unit.placement.x, unit.placement.y - unit.unk26AC};
    view.who = &unit;
    view.mirrored = unit.drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
    view.flags = unit.drawFlags;
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
    const DrawPlacer::Holding holding(view.place[0], view.place[1], view.who);
    Painter& painter = services.painter;
    const Picture* body = view.body ? view.picture.Get(services.archive) : nullptr;

    if (body != nullptr)
    {
        const Picture& picture = *body;
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

    auto digitStep = [&view](i16 font, int value, int x, int y)
    {
        view.steps.push_back(NumberStep{.kind = NumberStep::Kind::Digit, .font = font, .value = value, .x = x, .y = y});
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

            auto drawHop = [&](std::size_t digit, int at)
            {
                digitStep(font, digits[digit], at, hopY(digit));
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
        fadeStep(fade);

        if (digits[0] != 0)
        {
            digitStep(font, digits[0], x - kDigitWidth, risenY);
        }

        if (digits[0] != 0 || digits[1] != 0)
        {
            fadeStep(fade);
            digitStep(font, digits[1], x, risenY);
        }

        fadeStep(fade);

        if (digits[0] != 0 || digits[1] != 0 || digits[2] != 0)
        {
            digitStep(font, digits[2], x + kDigitWidth, risenY);
        }

        digitStep(font, digits[3], x + 2 * kDigitWidth, risenY);
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

        view.fading = true;
        view.fade = unit.guardRise;
        view.letters = static_cast<int>(phases.size());

        for (std::size_t letter = 0; letter < phases.size(); ++letter)
        {
            view.y[letter] = top - unit.guardRise;
        }

        return;
    }

    view.clearMode = true;
    view.letters = 1;
    view.y[0] = top - BattleUnit::PopOffsets[static_cast<std::size_t>(phases[0])];

    for (std::size_t letter = 1; letter < phases.size() && phases[0] > 0; ++letter)
    {
        view.letters = static_cast<int>(letter) + 1;
        view.y[letter] = top - BattleUnit::PopOffsets[static_cast<std::size_t>(phases[letter])];

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

// What HurtFrame decides before its draws: the hurt act stepped, its sound, the places of the stagger step; false when
// the frame ends with `result` before anything is drawn.
bool UnitFrame::HurtAdvance(BattleServices& services, HurtView& view, int& result)
{
    BattleUnit& unit = this->unit;
    ActState& act = unit.attackAct;
    act.id = unit.actId;

    if (unit.kind != 1 || unit.unk2B00 != 0 || unit.unk2B04 != 0)
    {
        act.action = kEnemyHomeAction;
    }
    else
    {
        act.action = kHurtAction;

        if ((unit.GetGuard() == 1 || (unit.marks & BattleUnit::UnkMark1) != 0) && (unit.marks & BattleUnit::Held) == 0)
        {
            act.action = kGuardHurtAction;
        }

        if ((unit.status & BattleUnit::UnkStatus1000) != 0)
        {
            act.action = kHurtAction;
        }
    }

    act.hold = 0;

    if (unit.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        result = 1;
        return false;
    }

    if (unit.unk2AF4 == 1 && unit.kind == 0)
    {
        act.picture = unit.mainAct.picture;
    }

    if (unit.guarding == 1)
    {
        result = 1;
        return false;
    }

    const bool still = (unit.status & BattleUnit::UnkStatus4) != 0;

    if (still)
    {
        act = unit.mainAct;
    }

    unit.shadowPicture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.shadowPicture == nullptr)
    {
        result = 0;
        return false;
    }

    const Picture& picture = *unit.shadowPicture;
    services.painter.SetSource(picture.data.data());
    services.audio.PlaySound(act.view, services.config.soundVolume);

    if (act.view != 0)
    {
        services.audio.SetSoundPan(act.view, unit.facing == 1 ? -kFadePan : kFadePan);
    }

    const std::size_t step = unit.hurtStep;
    const int placeX = unit.placement.x;
    unit.unk2670 = act.x;
    act.view = 0;
    int y = unit.placement.y - act.y;
    const int shake = BattleUnit::StaggerShake[step];
    int x = shake + placeX - act.x;

    if (unit.unk2B00 == 1)
    {
        if (unit.facing != 0)
        {
            x = placeX - shake - act.x;
        }
        else
        {
            unit.unk2670 = picture.width - act.x;
            x = placeX + BattleUnit::StaggerShake[step] - unit.unk2670;
        }
    }

    if (unit.facing == 1 && unit.unk2B00 == 0)
    {
        unit.unk2670 = picture.width - act.x;
        x = placeX - BattleUnit::StaggerShake[step] - unit.unk2670;
    }

    if (still)
    {
        x = placeX - unit.unk2670;
    }

    view.picture = {act.frame, act.picture, true};
    view.bodyWidth = picture.width;
    const u32 mirrored = unit.drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);

    if (unit.kind == 0 && !still)
    {
        view.flash = true;
        view.flashFlags = mirrored | Painter::TintUnder;

        if (unit.unk2B10 != 0)
        {
            view.flashX = placeX - unit.unk2670;
            view.flashY = unit.placement.y - BattleUnit::StaggerRise[step] - act.y;
            x = placeX - unit.unk2670;
            y = unit.placement.y - BattleUnit::StaggerLift[step] - act.y;
        }
        else if (unit.facing != 0)
        {
            view.flashX = placeX + BattleUnit::StaggerRise[step] - unit.unk2670;
            view.flashY = unit.placement.y - act.y;
            x = placeX + BattleUnit::StaggerLift[step] - unit.unk2670;
        }
        else
        {
            view.flashX = placeX - BattleUnit::StaggerRise[step] - unit.unk2670;
            view.flashY = unit.placement.y - act.y;
            x = placeX - BattleUnit::StaggerLift[step] - unit.unk2670;
        }
    }

    view.x = x;
    view.y = y;
    view.shadowY = unit.placement.y;
    view.shadowTint = unit.unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
    view.shadowFlags = mirrored | Painter::Shadow;
    // What the draws leave in the unit: the last mode drawn with, the draw flags without the effects.
    unit.unk26A4 = view.shadowFlags;
    unit.drawFlags &= Painter::RleSource | Painter::Mirror | Painter::Flip;
    view.flags = unit.drawFlags;
    view.red = !still && act.action != kGuardHurtAction;
    return true;
}

// What HurtFrame draws before the unit's effects: the red flash an enemy leaves, the shadow, the unit, its red copy.
void UnitFrame::HurtDrawBody(BattleServices& services, const HurtView& view)
{
    Painter& painter = services.painter;
    const Picture* body = view.picture.Get(services.archive);

    if (body == nullptr)
    {
        return;
    }

    const Picture& picture = *body;
    painter.SetSource(picture.data.data());
    DrawOptions& options = painter.Options();

    if (view.flash)
    {
        options.tintRed = kHurtRed;
        options.tintGreen = 0;
        options.tintBlue = 0;
        painter.DrawPicture(view.flashX, view.flashY, picture.width, picture.height, view.flashFlags);
    }

    const int shadowHeight = picture.height / kShadowShare;
    options.scaleHeight = shadowHeight;
    options.shear = picture.height >> kShadowShearShift;
    options.tintRed = view.shadowTint;
    options.tintGreen = view.shadowTint;
    options.tintBlue = view.shadowTint;

    {
        painter.DrawPicture(view.x, view.shadowY - shadowHeight, picture.width, picture.height, view.shadowFlags);
    }

    painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.flags);

    if (view.red)
    {
        painter.Options().tintRed = kHurtFlash;
        painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.flags | Painter::TintUnder);
    }
}

// What HurtFrame decides after the unit's effects: the overlay chosen, the recoil effect stepped with its sound;
// false when the frame ends there (the recoil effect's act missing).
bool UnitFrame::HurtAdvanceTail(BattleServices& services, HurtView& view)
{
    BattleUnit& unit = this->unit;
    const ActState& act = unit.attackAct;
    const bool still = (unit.status & BattleUnit::UnkStatus4) != 0;
    const int placeX = unit.placement.x;

    if (!still && act.rotation != 0 && act.actionCode != 0)
    {
        unit.unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

        if (unit.unk2554 != nullptr)
        {
            view.overlay = {act.rotation, static_cast<u16>(act.actionCode - 1), true};
            view.overlayX = placeX + static_cast<i16>(act.offsetX) - unit.unk2670;
            view.overlayY = unit.placement.y + static_cast<i16>(act.offsetY) - act.y;
            view.overlayFlags = unit.drawFlags | Painter::Additive;
        }
    }

    if ((act.target & ActState::Recoils) == 0)
    {
        return true;
    }

    ActState& effect = unit.castAct;
    effect.action = act.effectQ;
    effect.id = act.effectA;

    if (services.acts.Run(effect) == 0)
    {
        return false;
    }

    services.audio.PlaySound(effect.view, services.config.soundVolume);
    effect.view = 0;
    unit.picture = services.archive.GetPicture(effect.frame, effect.picture);

    if (unit.picture == nullptr)
    {
        return true;
    }

    u32 flags = effect.flags;
    int offsetX = effect.x;

    if (act.anchorX != 0)
    {
        unit.actAnchorX = act.anchorX;
    }

    if (unit.facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = unit.picture->width - offsetX;
        unit.actAnchorX = static_cast<u16>(view.bodyWidth - act.anchorX);
    }

    view.recoil = {effect.frame, effect.picture, true};
    view.recoilX = view.x + static_cast<i16>(unit.actAnchorX) - offsetX;
    view.recoilY = view.y + static_cast<i16>(act.anchorY) - effect.y;
    view.recoilFlags = flags;
    return true;
}

// What HurtFrame draws after the unit's effects: the overlay, the recoil effect.
void UnitFrame::HurtDrawTail(BattleServices& services, const HurtView& view)
{
    Painter& painter = services.painter;

    if (const Picture* overlay = view.overlay.Get(services.archive); overlay != nullptr)
    {
        painter.SetSource(overlay->data.data());
        painter.DrawPicture(view.overlayX, view.overlayY, overlay->width, overlay->height, view.overlayFlags);
    }

    if (const Picture* recoil = view.recoil.Get(services.archive); recoil != nullptr)
    {
        painter.SetSource(recoil->data.data());
        painter.DrawPicture(view.recoilX, view.recoilY, recoil->width, recoil->height, view.recoilFlags);
    }
}

// What HurtFrame decides at the end of the frame: the stagger stepped (not held); 1 once it is over.
int UnitFrame::HurtStep(BattleServices& services)
{
    BattleUnit& unit = this->unit;

    if (unit.held == 0)
    {
        ++unit.glowPhase;

        if (unit.glowPhase == kHurtFrames)
        {
            ++unit.hurtStep;
            unit.glowPhase = 0;
        }
    }

    unit.unk2AAC = 1;

    if (unit.hurtStep <= kLastHurtStep)
    {
        return 0;
    }

    services.state.unk53BF46 = 0;
    services.state.unk53BF48 = 0;
    unit.hurtStep = 1;
    services.painter.Options().shear = 0;
    unit.unk2AAC = 0;
    unit.charging = 1;
    unit.unk2B10 = 0;
    unit.attackAct = ActState{};
    unit.targets.fill(BattleUnit::NoTarget);
    return 1;
}

// What DrawAction decides of the frame: the act stepped, its place on the target or the unit (shaken by the nudge),
// its sound, the overlay chosen and cleared once the act waits; false when it draws nothing.
bool UnitFrame::ActionAdvance(BattleServices& services, const BattleUnit& target, ActState& act, ActionView& view)
{
    BattleUnit& unit = this->unit;
    u16 targetX = 0;
    u16 targetY = 0;

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

    const Picture& picture = *unit.picture;
    services.painter.SetSource(picture.data.data());
    unit.frameFlags = act.flags;
    unit.actOffsetX = static_cast<u16>(act.x);
    unit.actAnchorX = act.anchorX;
    unit.overlayX = act.offsetX;
    const bool forced = unit.unk2B00 == 1 || unit.unk2B04 == 1;
    const bool still = unit.unk2B00 == 0 && unit.unk2B04 == 0;

    auto mirror = [&unit, &act, &picture]
    {
        unit.frameFlags ^= Painter::Mirror;
        unit.actOffsetX = static_cast<u16>(picture.width - static_cast<u16>(act.x));

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

        if (unit.unk2B00 == 1 && (unit.frameFlags & ActState::NoBlend) == 0)
        {
            unit.frameFlags |= Painter::Additive;
        }
    }

    if (unit.facing == 1 && still)
    {
        mirror();
    }

    target.GetDrawPosition(targetX, targetY);
    u16 x = targetX;

    if (unit.nudge > 0 && services.random.Below(kNudgeChance) == 1)
    {
        x = static_cast<u16>(x + unit.nudge - kNudgeShift);
        unit.nudge = static_cast<u16>(unit.nudge >> 1);

        if (unit.nudge <= 1)
        {
            unit.nudge = 1;
        }
    }

    const u32 mirrored = unit.frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);

    if ((unit.command.flags & UnitCommand::AtTarget) != 0)
    {
        unit.actX = static_cast<u16>(x - unit.actOffsetX);
        unit.actY = static_cast<u16>(targetY - static_cast<u16>(act.y));
    }
    else
    {
        unit.actX = static_cast<u16>(unit.placement.x - unit.actOffsetX);
        unit.actY = static_cast<u16>(unit.placement.y - static_cast<u16>(act.y));

        if (unit.outline == 1)
        {
            unit.colorPair = BattleUnit::OutlineColorPair;
            view.outline = true;
        }
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int centre = static_cast<i16>(unit.actX) + static_cast<i16>(unit.actOffsetX);
    services.audio.SetSoundPan(act.view, centre >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;
    const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.x = static_cast<i16>(unit.actX);
    view.y = static_cast<i16>(unit.actY);
    view.mirrored = mirrored;
    view.flags = unit.frameFlags;
    view.shadowY = act.y + static_cast<i16>(unit.actY);
    view.shadowTint = invisible ? kInvisibleShadowTint : kShadowTint;
    view.smear = invisible;
    view.signal = (unit.frameFlags & ActState::Signal) != 0;
    view.alpha = act.signal;
    view.wave = unit.mainAct.wait;
    // What the draws leave in the unit: the last mode drawn with.
    unit.unk26A4 = invisible ? mirrored : mirrored | Painter::Shadow;

    if (act.rotation == 0 || act.actionCode == 0)
    {
        return true;
    }

    unit.unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

    if (unit.unk2554 == nullptr)
    {
        return true;
    }

    if (forced && unit.facing == 1 && act.offsetX != 0)
    {
        unit.overlayX = static_cast<u16>(picture.width - act.offsetX - unit.unk2554->width);
    }

    if (unit.facing == 1 && still && act.offsetX != 0)
    {
        unit.overlayX = static_cast<u16>(picture.width - act.offsetX - unit.unk2554->width);
    }

    view.overlay = {act.rotation, static_cast<u16>(act.actionCode - 1), true};
    view.overlayX = static_cast<i16>(unit.actX) + static_cast<i16>(unit.overlayX);
    view.overlayY = static_cast<i16>(unit.actY) + static_cast<i16>(act.offsetY);
    view.overlayFlags = unit.frameFlags | Painter::Additive;

    if (act.waiting == 1)
    {
        act.rotation = 0;
        act.actionCode = 0;
        act.offsetX = 0;
        act.offsetY = 0;
    }

    return true;
}

// What DrawAction draws, as ActionAdvance decided it: the outline, the shadow, the act (smeared when invisible), the
// overlay.
void UnitFrame::DrawActionView(BattleServices& services, const ActionView& view)
{
    Painter& painter = services.painter;
    const Picture* drawn = view.picture.Get(services.archive);

    if (drawn == nullptr)
    {
        return;
    }

    const Picture& picture = *drawn;
    painter.SetSource(picture.data.data());

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

    const int shadowHeight = picture.height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = picture.height >> kShadowShearShift;
    options.tintRed = view.shadowTint;
    options.tintGreen = view.shadowTint;
    options.tintBlue = view.shadowTint;
    painter.SetSource(picture.data.data());
    painter.DrawPicture(
        view.x,
        view.shadowY - shadowHeight,
        picture.width,
        picture.height,
        view.mirrored | Painter::Shadow
    );
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

        painter.DrawPicture(view.x, view.y, picture.width, picture.height, view.flags);
        painter.SetWave(0);
    }

    if (const Picture* overlay = view.overlay.Get(services.archive); overlay != nullptr)
    {
        painter.SetSource(overlay->data.data());
        painter.DrawPicture(view.overlayX, view.overlayY, overlay->width, overlay->height, view.overlayFlags);
    }
}

// swd32.exe 0x0047E650: animates the first effect with an act (the head node itself when it has one) above the unit,
// then the stone (status bit 2) or freeze (bit 12) overlay of the shadow picture (`body`, the one the caller just
// drew) at (x, y). The rebuild's smooth battle: the decisions here, the draws into `steps`.
void UnitFrame::EffectsAdvance(BattleServices& services, int x, int y, const PictureRef& body, PictureSteps& steps)
{
    BattleUnit& unit = this->unit;

    if (unit.effects.next == nullptr)
    {
        return;
    }

    EffectNode* node = &unit.effects;

    if (unit.effects.act.id <= 0)
    {
        do
        {
            node = node->next.get();
        } while (node != nullptr && node->act.id <= 0);
    }

    if (node != nullptr)
    {
        node->act.hold = unit.held == 1 ? 1 : 0;

        if (services.acts.Run(node->act) == 0)
        {
            return;
        }

        unit.picture = services.archive.GetPicture(node->act.frame, node->act.picture);

        if (unit.picture != nullptr && static_cast<i32>(unit.unk26B8) >= 0)
        {
            steps.push_back(
                PictureStep{
                    .picture = {node->act.frame, node->act.picture, true},
                    .x = unit.placement.x - node->act.x,
                    .y = unit.placement.y - (unit.picture->height >> 1) - unit.mainAct.y,
                    .mode = node->act.flags,
                }
            );
        }
    }

    const u32 mirrored = unit.drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);

    if ((unit.status & BattleUnit::UnkStatus4) != 0)
    {
        unit.unk26A4 = mirrored | Painter::GrayUnder;

        if (body.shown)
        {
            steps.push_back(PictureStep{.picture = body, .x = x, .y = y, .mode = unit.unk26A4});
        }

        return;
    }

    if ((unit.status & BattleUnit::UnkStatus1000) != 0)
    {
        unit.unk26A4 = mirrored;

        if (body.shown)
        {
            steps.push_back(
                PictureStep{
                    .picture = body,
                    .x = x,
                    .y = y,
                    .mode = unit.unk26A4 | Painter::TintUnder,
                    .tinted = true,
                    .tintRed = kUnkStatus1000Tint,
                    .tintGreen = 0,
                    .tintBlue = kUnkStatus1000Tint,
                }
            );
        }
    }
}

// swd32.exe 0x00480AE0: the main act's effect while the unit idles: when the main act recoils (Recoils), its effectA
// plays in the cast act (holding the main act with Holds; the command's follow-up act waits for it) above the main
// act's anchor, the sound panned to the unit's side; when it waits, the effect and the holds are cleared. The rebuild's
// smooth battle: the decisions here, the draw into `steps`.
void UnitFrame::IdleEffectAdvance(BattleServices& services, PictureSteps& steps)
{
    BattleUnit& unit = this->unit;
    ActState& main = unit.mainAct;
    ActState& effect = unit.castAct;
    const u16 bits = main.target;

    if ((bits & ActState::Recoils) != 0)
    {
        if (main.effectA != 0)
        {
            unit.unk267C |= BattleUnit::Recoiling;
        }

        if (unit.command.followAct != 0)
        {
            unit.unk267C |= BattleUnit::FollowingUp;
        }

        if ((bits & ActState::Holds) != 0)
        {
            main.hold = 1;
        }

        main.target = static_cast<u16>(bits & ~ActState::Recoils);
        effect = ActState{};
    }

    if ((unit.unk267C & BattleUnit::Recoiling) == 0)
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
        services.audio.SetSoundPan(effect.view, unit.placement.x >= kPanCentre ? kFadePan : -kFadePan);
    }

    effect.view = 0;
    unit.picture = services.archive.GetPicture(effect.frame, effect.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr)
    {
        return;
    }

    u32 flags = effect.flags;
    int offsetX = effect.x;

    if (unit.facing == 1)
    {
        flags ^= Painter::Mirror;

        if (offsetX != 0)
        {
            offsetX = unit.picture->width - offsetX;
        }
    }

    steps.push_back(
        PictureStep{
            .picture = {effect.frame, effect.picture, true},
            .x = static_cast<i16>(unit.actAnchorX) + unit.placement.x - static_cast<i16>(unit.originX) - offsetX,
            .y = static_cast<i16>(main.anchorY) + unit.placement.y - effect.y - main.y,
            .mode = flags,
        }
    );

    if (effect.waiting != 1)
    {
        return;
    }

    main.target = static_cast<u16>(main.target & ~ActState::Holds);
    unit.unk267C &= ~BattleUnit::Recoiling;
    unit.slotTint = 0;
    unit.actAnchorX = 0;
    unit.unk2ACC = 0;
    main.hold = 0;
    main.effectA = 0;
    effect = ActState{};
}

// swd32.exe 0x00480D40: the command's follow-up act (in the attack act) while the unit idles and FollowingUp is set,
// at the unit's place, the sound panned to its side; when it waits, every stage and the main act's effect are cleared.
// The rebuild's smooth battle: the decisions here, the draw into `steps`.
void UnitFrame::FollowUpAdvance(BattleServices& services, PictureSteps& steps)
{
    BattleUnit& unit = this->unit;

    if ((unit.unk267C & BattleUnit::FollowingUp) == 0)
    {
        return;
    }

    ActState& act = unit.attackAct;
    act.id = unit.command.followAct;
    act.action = 0;

    if (services.acts.Run(act) == 0)
    {
        return;
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);

    if (act.view != 0)
    {
        services.audio.SetSoundPan(act.view, unit.placement.x >= kPanCentre ? kFadePan : -kFadePan);
    }

    act.view = 0;
    unit.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr)
    {
        return;
    }

    u32 flags = act.flags;
    int offsetX = act.x;

    if (unit.facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = unit.picture->width - offsetX;
    }

    steps.push_back(
        PictureStep{
            .picture = {act.frame, act.picture, true},
            .x = unit.placement.x - offsetX,
            .y = unit.placement.y - act.y,
            .mode = flags,
        }
    );

    if (act.waiting != 1)
    {
        return;
    }

    unit.unk267C = 0;
    unit.slotTint = 0;
    unit.mainAct.target = 0;
    unit.actAnchorX = 0;
    unit.unk2ACC = 0;
    unit.mainAct.hold = 0;
    unit.mainAct.effectA = 0;
    act = ActState{};
}

// What DrawRecoil decides of the frame: the recoil act stepped with its sound, its place, the fade once it waits; its
// result (1 once the recoil is over), the draw into `steps`.
int UnitFrame::RecoilAdvance(BattleServices& services, ActState& act, i16 y, PictureSteps& steps)
{
    BattleUnit& unit = this->unit;

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    unit.picture = services.archive.GetPicture(act.frame, act.picture);
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, static_cast<i16>(unit.actX) >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr)
    {
        return 0;
    }

    const Picture& picture = *unit.picture;
    unit.unk26A4 = act.flags;
    unit.aimOffsetX = static_cast<u16>(act.x);

    auto mirror = [&unit, &act, &picture]
    {
        unit.unk26A4 ^= Painter::Mirror;

        if (act.x != 0)
        {
            unit.aimOffsetX = static_cast<u16>(picture.width - static_cast<u16>(act.x));
        }
    };

    if ((unit.unk2B00 == 1 || unit.unk2B04 == 1) && unit.facing == 1)
    {
        mirror();
    }

    if (unit.facing == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        mirror();
    }

    services.painter.SetSource(picture.data.data());
    PictureStep step{
        .picture = {act.frame, act.picture, true},
        .x = static_cast<i16>(unit.actAnchorX) - static_cast<i16>(unit.aimOffsetX) + static_cast<i16>(unit.actX),
        .y = static_cast<i16>(unit.actY) - act.y + y,
        .mode = unit.unk26A4,
    };

    if (act.waiting != 1)
    {
        step.blended = (unit.unk26A4 & ActState::Signal) != 0;
        step.alpha = act.signal;
        steps.push_back(step);
        return 0;
    }

    if (unit.recoilTint <= kRecoilFadeEnd)
    {
        unit.recoilTint = 0;
        return 1;
    }

    step.tinted = true;
    step.tintRed = unit.recoilTint;
    step.tintGreen = unit.recoilTint;
    step.tintBlue = unit.recoilTint;
    steps.push_back(step);
    unit.recoilTint = static_cast<i16>(unit.recoilTint - kRecoilFadeStep);

    if (unit.held == 1)
    {
        unit.recoilTint = static_cast<i16>(unit.recoilTint + kRecoilFadeStep);
    }

    return 0;
}

// What DrawTrail decides of the frame: every third frame the images move back and the current picture comes in front,
// each image fades; the draws into `steps`.
void UnitFrame::TrailAdvance(
    BattleServices& services,
    AfterimageTrail& trail,
    u16 frame,
    u16 picture,
    u32 mode,
    i16 x,
    i16 y,
    PictureSteps& steps
)
{
    BattleUnit& unit = this->unit;
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

        unit.unk2550 = services.archive.GetPicture(frame, image.picture);
        const PictureRef shown = {frame, image.picture, true};

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
        if (unit.unk2550 == nullptr)
        {
            continue;
        }

        const int drawX = image.x + spacing * place + offset;

        if ((mode & Painter::Additive) != 0)
        {
            steps.push_back(
                PictureStep{
                    .picture = shown,
                    .x = drawX,
                    .y = image.y,
                    .mode = mode,
                    .tinted = true,
                    .tintRed = image.level - kTrailTintBase,
                    .tintGreen = image.level - kTrailTintBase,
                    .tintBlue = kTrailBlue,
                }
            );
            continue;
        }

        steps.push_back(
            PictureStep{
                .picture = shown,
                .x = drawX,
                .y = image.y,
                .mode = mode | Painter::Translucent,
                .blended = true,
                .alpha = image.level,
            }
        );

        if (image.level > 1)
        {
            steps.push_back(
                PictureStep{
                    .picture = shown,
                    .x = drawX,
                    .y = image.y,
                    .mode = mode | Painter::TintUnder,
                    .tinted = true,
                    .tintRed = trail.tintRed,
                    .tintGreen = trail.tintGreen,
                    .tintBlue = trail.tintBlue,
                }
            );
        }
    }
}

// What FlyAct decides of the frame: the act stepped with its sound, the line walked; its result (1 once the target's
// x is reached), the draw into `steps`.
int UnitFrame::FlyAdvance(
    BattleServices& services,
    const BattleUnit& target,
    ActState& act,
    int slot,
    i32 id,
    int fromX,
    int fromY,
    int steps,
    int backwards,
    PictureSteps& draws
)
{
    BattleUnit& unit = this->unit;
    u16 targetX = 0;
    u16 targetY = 0;
    act.id = id;
    act.action = 0;
    act.hold = 0;

    if (unit.held == 1)
    {
        act.hold = 1;
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

    const Picture& picture = *unit.picture;
    services.painter.SetSource(picture.data.data());
    const u16 anchorX = act.anchorX;
    u32 flags = act.flags;
    auto offsetX = static_cast<u16>(act.x);
    i32 anchorShift = static_cast<i16>(anchorX);
    const bool forced = unit.unk2B00 == 1 || unit.unk2B04 == 1;

    if (forced && unit.facing == 1)
    {
        flags ^= Painter::Mirror;

        if (act.x != 0)
        {
            offsetX = static_cast<u16>(picture.width - static_cast<u16>(act.x));
        }

        if (anchorX != 0)
        {
            anchorShift = picture.width - static_cast<i16>(act.anchorX);
        }
    }

    if (unit.facing == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        flags ^= Painter::Mirror;
        offsetX = static_cast<u16>(picture.width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            anchorShift = picture.width - static_cast<i16>(anchorX);
        }
    }

    target.GetDrawPosition(targetX, targetY);
    const auto toY = static_cast<i16>(targetY - static_cast<u16>(act.y));
    const auto toX = static_cast<i16>(targetX - offsetX);
    LineWalker& line = unit.flight;
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

    i32& count = unit.flightSteps[static_cast<std::size_t>(slot)];

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
    // The flown act stands at its line's point less the act's anchor: a new picture keeps the point.
    draws.push_back(
        PictureStep{
            .picture = {act.frame, act.picture, true},
            .x = line.fromX + line.x,
            .y = line.fromY + line.y,
            .mode = flags,
            .held = true,
            .stand = {line.fromX + line.x + offsetX, line.fromY + line.y + act.y},
            .who = &count,
        }
    );

    if (taken != kArrived)
    {
        return 0;
    }

    count = 0;
    line = LineWalker{};
    return 1;
}

// What RunQueuedActions decides of the frame: each queued act stepped, shaken or flown with its trail, its impact
// played to its end, the hits landed and the done nodes freed; its result (1 when there are none), the draws into
// `steps`.
int UnitFrame::QueuedAdvance(BattleServices& services, BattleUnit& target, i32 action, PictureSteps& steps)
{
    BattleUnit& unit = this->unit;
    BattleState& battle = services.state;
    LineWalker& line = unit.flight;
    QueuedAction* node = unit.actions.get();
    int taken = 0;

    if (node == nullptr)
    {
        unit.queuedActions = 0;
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

        unit.picture = services.archive.GetPicture(act.frame, act.picture);

        // A missing picture is read through a null pointer in the original.
        if (unit.picture == nullptr)
        {
            return 0;
        }

        services.painter.SetSource(unit.picture->data.data());
        const BattleUnit::Stride stride = unit.FlightStride(services, *node, act, taken);
        const u32 flags = stride.flags;
        line = stride.line;
        node->shakeFrames = stride.shakeFrames;
        node->flown = stride.flown;
        taken = stride.taken;
        services.audio.PlaySound(act.view, services.config.soundVolume);
        act.view = 0;
        const int x = stride.x;
        const int y = stride.y;

        if (node->trailed == 1)
        {
            node->trail.fadeStep = (flags & Painter::Additive) != 0 ? kAdditiveTrailFade : 1;
            const std::size_t trailFrom = steps.size();
            this->TrailAdvance(
                services,
                node->trail,
                act.frame,
                act.picture,
                flags,
                static_cast<i16>(x),
                static_cast<i16>(y),
                steps
            );

            // The afterimages are the node's: each moves by where it is drawn.
            for (std::size_t at = trailFrom; at < steps.size(); ++at)
            {
                steps[at].held = true;
                steps[at].stand = {steps[at].x, steps[at].y};
                steps[at].who = node;
            }
        }

        steps.push_back(
            PictureStep{
                .picture = {act.frame, act.picture, true},
                .x = x,
                .y = y,
                .mode = flags,
                .held = true,
                .stand = {x + static_cast<i16>(stride.offsetX), y + act.y},
                .who = node,
            }
        );

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

            unit.picture = services.archive.GetPicture(impact.frame, impact.picture);

            if (unit.picture == nullptr)
            {
                return 0;
            }

            services.painter.SetSource(unit.picture->data.data());
            u32 impactFlags = impact.flags;

            if (unit.facing == 1)
            {
                impactFlags ^= Painter::Mirror;
            }

            services.audio.PlaySound(impact.view, services.config.soundVolume);
            services.audio.SetSoundPan(impact.view, line.toX >= kPanCentre ? kFadePan : -kFadePan);
            impact.view = 0;
            steps.push_back(
                PictureStep{
                    .picture = {impact.frame, impact.picture, true},
                    .x = line.fromX + line.x,
                    .y = line.fromY + line.y,
                    .mode = impactFlags,
                }
            );

            if (impact.waiting == 0)
            {
                continue;
            }
        }

        node->done = 1;
        ++unit.landedActions;
        line = LineWalker{};

        if ((unit.command.actionFlags & UnitCommand::UnkAction1) == 0 && impact.id >= 0)
        {
            target.RaiseFlag26B8();
            i16 spirit = 0;
            i16 stamina = 0;
            i32 damage = unit.HitDamage(services, target, spirit, stamina);
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

    std::unique_ptr<QueuedAction>* link = &unit.actions;

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

void UnitFrame::Action(BattleServices& services, const BattleUnit& target, ActState& act, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    ActionView view;

    if (this->ActionAdvance(services, target, act, view))
    {
        scene.Add(
            [view](BattleServices& drawing)
            {
                UnitFrame::DrawActionView(drawing, view);
            }
        );
    }
}

int UnitFrame::Recoil(BattleServices& services, ActState& act, i16 y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    PictureSteps steps;
    const int result = this->RecoilAdvance(services, act, y, steps);
    scene.Add(std::move(steps));
    return result;
}

int UnitFrame::Fly(
    BattleServices& services,
    const BattleUnit& target,
    ActState& act,
    int slot,
    i32 id,
    int fromX,
    int fromY,
    int steps,
    int backwards,
    Scene& scene
)
{
    const Scene::Owner owner(scene, &this->unit);
    PictureSteps draws;
    const int result = this->FlyAdvance(services, target, act, slot, id, fromX, fromY, steps, backwards, draws);
    scene.Add(std::move(draws));
    return result;
}

void UnitFrame::Trail(
    BattleServices& services,
    AfterimageTrail& trail,
    u16 frame,
    u16 picture,
    u32 mode,
    i16 x,
    i16 y,
    Scene& scene
)
{
    const Scene::Owner owner(scene, &this->unit);
    PictureSteps steps;
    this->TrailAdvance(services, trail, frame, picture, mode, x, y, steps);
    scene.Add(std::move(steps));
}

int UnitFrame::Queued(BattleServices& services, BattleUnit& target, i32 action, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    PictureSteps steps;
    const int result = this->QueuedAdvance(services, target, action, steps);
    scene.Add(std::move(steps));
    return result;
}

int UnitFrame::Home(BattleServices& services, const BattleUnit& target, i32 id, i32 action, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    WalkView view;
    const int result = this->HomeAdvance(services, target, id, action, view);

    if (view.picture.shown)
    {
        scene.Add(
            [view](BattleServices& drawing)
            {
                UnitFrame::DrawWalk(drawing, view);
            }
        );
    }

    return result;
}

void UnitFrame::Idle(BattleServices& services, int paused, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    IdleView view;

    if (!this->IdleAdvance(services, paused, view))
    {
        return;
    }

    scene.Add(
        [view](BattleServices& drawing)
        {
            UnitFrame::IdleDraw(drawing, view);
        }
    );

    if (view.thrownLost)
    {
        return;
    }

    PictureSteps steps;
    this->IdleEffectAdvance(services, steps);
    scene.Add(std::move(steps));
    steps = PictureSteps{};
    this->FollowUpAdvance(services, steps);
    scene.Add(std::move(steps));
    steps = PictureSteps{};
    this->EffectsAdvance(services, view.x, view.y, view.picture, steps);
    scene.Add(std::move(steps));
}

void UnitFrame::Walk(BattleServices& services, const BattleUnit& target, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    WalkView view;

    if (!this->WalkAdvance(services, target, view))
    {
        return;
    }

    scene.Add(
        [view](BattleServices& drawing)
        {
            UnitFrame::DrawWalk(drawing, view);
        }
    );
}

int UnitFrame::Hurt(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    HurtView view;
    int result = 0;

    if (!this->HurtAdvance(services, view, result))
    {
        return result;
    }

    scene.Add(
        [view](BattleServices& drawing)
        {
            UnitFrame::HurtDrawBody(drawing, view);
        }
    );
    PictureSteps steps;
    this->EffectsAdvance(services, view.x, view.y, view.picture, steps);

    scene.Add(std::move(steps));

    const bool finished = this->HurtAdvanceTail(services, view);
    scene.Add(
        [view](BattleServices& drawing)
        {
            UnitFrame::HurtDrawTail(drawing, view);
        }
    );

    if (!finished)
    {
        return 0;
    }

    return this->HurtStep(services);
}

int UnitFrame::Numbers(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    NumbersView view;
    const int result = this->NumbersAdvance(services, view);
    scene.Add(
        [view = std::move(view)](BattleServices& drawing)
        {
            UnitFrame::DrawNumbers(drawing, view);
        }
    );
    return result;
}

void UnitFrame::Guard(Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    GuardView view;
    this->GuardAdvance(view);
    scene.Add(
        [view](BattleServices& drawing)
        {
            UnitFrame::DrawGuard(drawing, view);
        }
    );
}

// What BattleUnit::DrawActionAt decides of the frame; the draws into `scene`.
void UnitFrame::DrawActionAtAdvance(BattleServices& services, i16 x, i16 y, ActState& act, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (services.acts.Run(act) == 0)
    {
        return;
    }

    unit.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr)
    {
        return;
    }

    ScenePainter painter(services, scene);
    unit.frameFlags = act.flags;
    unit.actOffsetX = static_cast<u16>(act.x);
    unit.actAnchorX = act.anchorX;

    auto mirror = [&unit, &act]
    {
        unit.frameFlags ^= Painter::Mirror;
        unit.actOffsetX = static_cast<u16>(unit.picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            unit.actAnchorX = static_cast<u16>(unit.picture->width - act.anchorX);
        }
    };

    if (unit.unk2B00 == 1 || unit.unk2B04 == 1)
    {
        if (unit.facing == 1)
        {
            mirror();
        }

        if (unit.unk2B00 == 1)
        {
            unit.frameFlags |= Painter::Additive;
        }
    }

    if (unit.facing == 1 && unit.unk2B00 == 0 && unit.unk2B04 == 0)
    {
        mirror();
    }

    unit.actX = static_cast<u16>(x - unit.actOffsetX);
    unit.actY = static_cast<u16>(y - static_cast<u16>(act.y));
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int centre = static_cast<i16>(unit.actX) + static_cast<i16>(unit.actOffsetX);
    services.audio.SetSoundPan(act.view, centre >= kPanCentre ? kFadePan : -kFadePan);
    act.view = 0;
    const int shadowHeight = unit.picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = unit.picture->height >> kShadowShearShift;
    const bool invisible = (unit.status & BattleUnit::Invisible) != 0;
    const int tint = invisible ? kInvisibleShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    unit.unk26A4 = (unit.frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.SetSource(unit.picture->data.data());
    painter.DrawPicture(
        static_cast<i16>(unit.actX),
        act.y + static_cast<i16>(unit.actY) - shadowHeight,
        unit.picture->width,
        unit.picture->height,
        unit.unk26A4
    );
    painter.SetSource(unit.picture->data.data());

    if (invisible)
    {
        painter.DrawPicture(
            static_cast<i16>(unit.actX),
            static_cast<i16>(unit.actY),
            unit.picture->width,
            unit.picture->height,
            unit.frameFlags | Painter::SmearUnder
        );
        return;
    }

    if ((unit.frameFlags & ActState::Signal) == ActState::Signal)
    {
        painter.Options().alpha = unit.mainAct.signal;
    }

    painter.DrawPicture(
        static_cast<i16>(unit.actX),
        static_cast<i16>(unit.actY),
        unit.picture->width,
        unit.picture->height,
        unit.frameFlags
    );
}

// What BattleUnit::DrawAimed decides of the frame; the draws into `scene`.
int UnitFrame::DrawAimedAdvance(
    BattleServices& services,
    BattleUnit& target,
    ActState& act,
    i16 shiftX,
    i16 shiftY,
    Scene& scene
)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);
    u16 targetX = 0;
    u16 targetY = 0;
    act.hold = 0;

    if (unit.held == 1)
    {
        act.hold = 1;
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

    unit.unk26A4 = act.flags;
    unit.aimOffsetX = static_cast<u16>(act.x);
    unit.aimAnchorX = act.anchorX;

    if (unit.facing == 1)
    {
        unit.unk26A4 ^= Painter::Mirror;
        unit.aimOffsetX = static_cast<u16>(unit.picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            unit.aimAnchorX = static_cast<u16>(unit.picture->width - act.anchorX);
        }
    }

    target.GetDrawPosition(targetX, targetY);
    unit.aimX = static_cast<u16>(targetX - unit.aimOffsetX);
    unit.aimY = static_cast<u16>(targetY - static_cast<u16>(act.y));

    if ((act.anchorX != 0 || act.anchorY != 0) && (shiftX != 0 || shiftY != 0))
    {
        unit.aimX = static_cast<u16>(unit.actX + unit.actAnchorX - unit.aimAnchorX);
        unit.aimY = static_cast<u16>(unit.actY - act.anchorY + shiftY);
    }

    unit.ClearMoveAxes(unit.aimX, unit.aimY);
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

    painter.SetSource(unit.picture->data.data());

    // The original reads the palette of the shadow picture through a null pointer when there is none.
    const DrawArgument argument = unit.shadowPicture != nullptr ? PaletteArgument(*unit.shadowPicture) : DrawArgument{};
    painter.DrawPicture(
        static_cast<i16>(unit.aimX),
        static_cast<i16>(unit.aimY),
        unit.picture->width,
        unit.picture->height,
        unit.unk26A4,
        argument
    );
    return act.waiting == 1 ? 1 : 0;
}

// What BattleUnit::DrawMainFaded decides of the frame; the draws into `scene`.
void UnitFrame::DrawMainFadedAdvance(BattleServices& services, int tint, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);
    ActState& act = unit.mainAct;
    act.hold = 0;

    if (unit.held == 1)
    {
        act.hold = 1;
    }

    unit.picture = services.archive.GetPicture(act.frame, act.picture);

    auto tintAll = [&painter, tint]
    {
        DrawOptions& options = painter.Options();
        options.tintRed = tint;
        options.tintGreen = tint;
        options.tintBlue = tint;
    };

    const int x = unit.placement.x - static_cast<i16>(unit.originX);
    const int y = unit.placement.y - act.y;

    // A missing picture is read through a null pointer in the original.
    if (unit.picture != nullptr)
    {
        painter.SetSource(unit.picture->data.data());
        tintAll();
        painter.DrawPicture(x, y, unit.picture->width, unit.picture->height, unit.drawFlags | Painter::Additive);
    }

    if (act.rotation == 0 || act.actionCode == 0)
    {
        return;
    }

    tintAll();
    unit.unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

    if (unit.unk2554 == nullptr)
    {
        return;
    }

    painter.SetSource(unit.unk2554->data.data());
    painter.DrawPicture(
        x + static_cast<i16>(act.offsetX),
        y + static_cast<i16>(act.offsetY),
        unit.unk2554->width,
        unit.unk2554->height,
        unit.drawFlags | Painter::Additive
    );
}

// What BattleUnit::FadeMainIn decides of the frame; the draws into `scene`.
int UnitFrame::FadeMainInAdvance(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    this->DrawMainFadedAdvance(services, unit.brightLevel, scene);

    if (unit.held == 0)
    {
        ++unit.brightLevel;
    }

    if (unit.brightLevel < 0)
    {
        return 0;
    }

    unit.marks = static_cast<u16>(unit.marks & ~BattleUnit::UnkMark200);
    unit.brightLevel = 1;
    return 1;
}

// What BattleUnit::FadeMainOut decides of the frame; the draws into `scene`.
int UnitFrame::FadeMainOutAdvance(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    this->DrawMainFadedAdvance(services, unit.glowLevel, scene);

    if (unit.held == 0)
    {
        unit.glowLevel = static_cast<i16>(unit.glowLevel - kFadeOutStep);
    }

    if (unit.glowLevel > kFadedOut)
    {
        return 0;
    }

    unit.glowLevel = kFadedOut;
    unit.recoilAct = ActState{};
    return 1;
}

// What BattleUnit::FadeInvisible decides of the frame; the draws into `scene`.
int UnitFrame::FadeInvisibleAdvance(BattleServices& services, int mode, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);
    unit.mainAct.hold = 0;

    if (unit.held == 1)
    {
        unit.mainAct.hold = 1;
    }

    unit.picture = services.archive.GetPicture(unit.mainAct.frame, unit.mainAct.picture);
    ++unit.glowLevel;

    if (unit.glowLevel > kInvisibleFadeFrames)
    {
        unit.glowLevel = kInvisibleFadeFrames;

        if (mode == 1)
        {
            unit.marks = static_cast<u16>(unit.marks & ~(BattleUnit::InvisibleMark | BattleUnit::UnkMark400));
            unit.glowLevel = 0;
            return 0;
        }

        if (mode == 0)
        {
            unit.status |= BattleUnit::Invisible;
        }

        unit.marks = static_cast<u16>(unit.marks & ~(BattleUnit::InvisibleMark | BattleUnit::UnkMark400));
        return 0;
    }

    // A missing picture is read through a null pointer in the original.
    if (unit.picture == nullptr || unit.shadowPicture == nullptr)
    {
        return 1;
    }

    painter.SetSource(unit.picture->data.data());
    const int shadowHeight = unit.shadowPicture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = unit.shadowPicture->height >> kShadowShearShift;
    options.tintRed = kInvisibleShadowTint;
    options.tintGreen = kInvisibleShadowTint;
    options.tintBlue = kInvisibleShadowTint;
    unit.unk26A4 = (unit.drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    const int x = unit.placement.x - static_cast<i16>(unit.originX);
    painter.DrawPicture(
        x - kShadowLeftShift,
        unit.placement.y - shadowHeight,
        unit.shadowPicture->width,
        unit.shadowPicture->height,
        unit.unk26A4
    );

    if (mode == 0)
    {
        painter.Options().alpha = Painter::MaxAlpha - unit.glowLevel / 2;
    }
    else if (mode == 1)
    {
        painter.Options().alpha = unit.glowLevel / 2;
    }

    unit.unk26A4 = unit.drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
    painter.DrawPicture(
        x,
        unit.placement.y - unit.mainAct.y,
        unit.picture->width,
        unit.picture->height,
        unit.unk26A4 | Painter::Translucent
    );
    return 1;
}

// What BattleUnit::DrawFadeIn decides of the frame; the draws into `scene`.
int UnitFrame::DrawFadeInAdvance(BattleServices& services, const BattleUnit& anchor, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (static_cast<i32>(unit.status) < 0)
    {
        return 1;
    }

    if (unit.brightLevel >= BattleUnit::Opaque)
    {
        unit.recoilAct = ActState{};
        unit.unk2AAC = 0;
        return 1;
    }

    unit.unk2AAC = 1;
    unit.recoilAct.action = unit.commandA == BattleUnit::UnkCommand1 ? kCommand1Action : kShadowAction;
    unit.recoilAct.id = unit.actId;
    const u16 pose = unit.unk2A8C;

    if (pose != 0)
    {
        unit.recoilAct.action = pose == kPose2 ? kPose2Action : kPoseAction;
    }

    if (services.acts.Run(unit.recoilAct) == 0)
    {
        return 0;
    }

    if ((unit.recoilAct.flags & ActState::Signal) != 0)
    {
        unit.brightLevel = BattleUnit::FadedIn;
        return 0;
    }

    unit.picture = services.archive.GetPicture(unit.recoilAct.frame, 0);

    if (unit.brightLevel == 1)
    {
        unit.recoilAct.view = kFadeInSound;
        services.audio.PlaySound(kFadeInSound, services.config.soundVolume);
        services.audio.SetSoundPan(unit.recoilAct.view, unit.facing == 1 ? -kFadePan : kFadePan);
        unit.recoilAct.view = 0;
    }

    if (unit.picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(unit.picture->data.data());
    const u32 actFlags = unit.recoilAct.flags;
    unit.originX = static_cast<u16>(unit.recoilAct.x);
    const i32 blend = unit.unk2B00;
    unit.drawFlags = actFlags;
    unit.recoilAct.hold = 1;
    unit.MirrorRecoil(actFlags, blend == 1);
    u16 anchorX = 0;
    u16 anchorY = 0;
    anchor.GetDrawPosition(anchorX, anchorY);
    const int x = static_cast<i16>(anchorX) - static_cast<i16>(unit.originX);
    const int y = static_cast<i16>(anchorY) - unit.recoilAct.y;
    const i32 level = unit.brightLevel;

    if (unit.unk2B00 == 1)
    {
        DrawOptions& options = painter.Options();
        options.tintRed = level - BattleUnit::FadedIn;
        options.tintGreen = level - BattleUnit::FadedIn;
        options.tintBlue = level - BattleUnit::FadedIn;
        painter.DrawPicture(x, y, unit.picture->width, unit.picture->height, unit.drawFlags);

        if (unit.held == 0)
        {
            ++unit.brightLevel;
        }

        return 0;
    }

    painter.Options().alpha = level;
    painter.DrawPicture(
        x,
        y,
        unit.picture->width,
        unit.picture->height,
        unit.drawFlags | Painter::Translucent,
        PaletteArgument(*unit.picture)
    );

    if (unit.held == 0)
    {
        unit.brightLevel += unit.unk2A0A;
    }

    return 0;
}

// What BattleUnit::DrawFadeOut decides of the frame; the draws into `scene`.
int UnitFrame::DrawFadeOutAdvance(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (static_cast<i32>(unit.status) < 0 || unit.fadeLevel <= 0)
    {
        return 1;
    }

    unit.recoilAct.id = unit.actId;
    const i32 paused = unit.held;
    unit.unk2AAC = 1;
    unit.recoilAct.action = kShadowAction;
    unit.recoilAct.hold = paused == 1 ? 1 : 0;

    if (services.acts.Run(unit.recoilAct) == 0)
    {
        return 1;
    }

    unit.picture = services.archive.GetPicture(unit.recoilAct.frame, unit.recoilAct.picture);

    if ((unit.recoilAct.flags & ActState::Signal) != 0)
    {
        unit.fadeLevel = 0;
        return 0;
    }

    if (unit.fadeLevel == BattleUnit::Opaque)
    {
        unit.recoilAct.view = kFadeOutSound;
        services.audio.PlaySound(kFadeOutSound, services.config.soundVolume);
        services.audio.SetSoundPan(unit.recoilAct.view, unit.facing == 1 ? kFadePan : -kFadePan);
        unit.recoilAct.view = 0;
    }

    if (unit.picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(unit.picture->data.data());
    const i32 blend = unit.unk2B00;
    const u32 actFlags = unit.recoilAct.flags;
    unit.drawFlags = actFlags;
    unit.MirrorRecoil(actFlags, blend == 1);
    const i32 level = unit.fadeLevel;
    const int x = unit.placement.x - static_cast<i16>(unit.originX);
    const int y = unit.placement.y - unit.recoilAct.y;

    if (blend == 1)
    {
        DrawOptions& options = painter.Options();
        options.tintRed = level - BattleUnit::Opaque;
        options.tintGreen = level - BattleUnit::Opaque;
        options.tintBlue = level - BattleUnit::Opaque;
        painter.DrawPicture(
            x,
            y,
            unit.picture->width,
            unit.picture->height,
            unit.drawFlags,
            PaletteArgument(*unit.picture)
        );

        if (unit.held == 0)
        {
            --unit.fadeLevel;
        }

        return 0;
    }

    painter.Options().alpha = level;
    painter.DrawPicture(
        x,
        y,
        unit.picture->width,
        unit.picture->height,
        unit.drawFlags | Painter::Translucent,
        PaletteArgument(*unit.picture)
    );

    if (unit.held == 0)
    {
        unit.fadeLevel -= unit.unk2A0A;
    }

    return 0;
}

// What BattleUnit::DrawEffectIcons decides of the frame; the draws into `scene`.
void UnitFrame::DrawEffectIconsAdvance(BattleServices& services, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (unit.effects.next == nullptr)
    {
        return;
    }

    int count = 0;

    for (const EffectNode* node = &unit.effects; node != nullptr; node = node->next.get())
    {
        if (node->icon != 0 && static_cast<i32>(node->bits) >= 0)
        {
            unit.picture = services.archive.GetPicture(kEffectIcons, static_cast<u16>(node->icon - 1u));

            if (unit.picture != nullptr)
            {
                painter.SetSource(unit.picture->data.data());
                painter.DrawPicture(x + count * unit.picture->width, y, unit.picture->width, unit.picture->height, 0);
            }

            ++count;
        }
    }
}

// What BattleUnit::RunAway decides of the frame; the draws into `scene`.
int UnitFrame::RunAwayAdvance(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);
    ActState& run = unit.recoilAct;

    if (unit.fadeLevel <= kRunEnd)
    {
        run = ActState{};
        return 1;
    }

    if (run.picture == kStepPicture)
    {
        services.audio.PlaySound(kStepSound, services.config.soundVolume);
    }

    run.id = unit.actId;
    unit.unk2AAC = 1;
    run.action = kShadowAction;
    run.hold = 0;

    if (unit.held == 1)
    {
        run.hold = 1;
    }

    if (services.acts.Run(run) == 0)
    {
        return 1;
    }

    unit.picture = services.archive.GetPicture(run.frame, run.picture);
    unit.frameFlags = run.flags;
    unit.actOffsetX = static_cast<u16>(run.x);

    if (unit.fadeLevel == kRunSoundAt)
    {
        run.view = kRunSound;
        services.audio.PlaySound(run.view, services.config.soundVolume);
        services.audio.SetSoundPan(run.view, unit.facing == 1 ? kFadePan : -kFadePan);
        run.view = 0;
    }

    unit.frameFlags ^= Painter::Mirror;

    if (unit.facing == 1)
    {
        unit.frameFlags ^= Painter::Mirror;

        // A missing picture is read through a null pointer in the original.
        if (run.x != 0 && unit.picture != nullptr)
        {
            unit.actOffsetX = static_cast<u16>(unit.picture->width - static_cast<u16>(run.x));
        }
    }

    u16 x = 0;
    u16 y = 0;
    unit.GetPosition(x, y);
    const int stepped = unit.facing == 1 ? x + kRunStep : x - kRunStep;
    unit.SetPosition(static_cast<i16>(stepped), static_cast<i16>(y));

    if (unit.picture != nullptr)
    {
        painter.SetSource(unit.picture->data.data());
        painter.DrawPicture(
            unit.placement.x - static_cast<i16>(unit.actOffsetX),
            unit.placement.y - run.y,
            unit.picture->width,
            unit.picture->height,
            unit.frameFlags,
            PaletteArgument(*unit.picture)
        );
    }

    --unit.fadeLevel;
    return 0;
}

// What BattleUnit::DrawDefeatPiece decides of the frame; the draws into `scene`.
void UnitFrame::DrawDefeatPieceAdvance(
    BattleServices& services,
    const Picture& picture,
    int left,
    int top,
    int right,
    int bottom,
    int x,
    int y,
    int height,
    Scene& scene
)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);
    painter.SetClipClamped(left, top, right, bottom);
    DrawOptions& options = painter.Options();
    options.tintRed = -unit.glowLevel;
    options.tintGreen = -unit.glowLevel;
    options.tintBlue = -unit.glowLevel;
    painter.DrawPicture(x, y, picture.width, height, unit.drawFlags | Painter::Additive);
}

// What BattleUnit::DefeatFadeOut decides of the frame; the draws into `scene`.
int UnitFrame::DefeatFadeOutAdvance(BattleServices& services, const Picture& picture, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel <= kFadedOut)
    {
        unit.glowLevel = 0;
        unit.slotTint = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    ScenePainter painter(services, scene);
    DrawOptions& options = painter.Options();
    options.tintRed = unit.glowLevel;
    options.tintGreen = unit.glowLevel;
    options.tintBlue = unit.glowLevel;
    painter.DrawPicture(
        unit.placement.x - static_cast<i16>(unit.originX),
        unit.placement.y - unit.mainAct.y,
        picture.width,
        picture.height,
        unit.drawFlags | Painter::Additive
    );
    --unit.glowLevel;
    return 0;
}

// What BattleUnit::DefeatFill decides of the frame; the draws into `scene`.
int UnitFrame::DefeatFillAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel >= picture.height)
    {
        unit.defeatPose = kBurstPose;
        unit.glowLevel = 0;
        return 0;
    }

    unit.StartDefeatDraw(services, picture);
    ScenePainter painter(services, scene);
    painter.DrawPicture(x, y, picture.width, picture.height, unit.drawFlags);

    if (unit.glowLevel > picture.height)
    {
        unit.glowLevel = static_cast<i16>(picture.height - 1);
    }

    DrawOptions& options = painter.Options();
    const bool blue = unit.defeatPose == kBlueFillPose;
    options.tintRed = blue ? 0 : kDarkFillTint;
    options.tintGreen = blue ? 0 : kDarkFillTint;
    options.tintBlue = blue ? kBlueFillTint : kDarkFillTint;
    painter.DrawPicture(
        x,
        y,
        picture.width,
        blue ? unit.glowLevel + 1 : unit.glowLevel,
        unit.drawFlags | Painter::TintUnder
    );
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    unit.glowLevel = static_cast<i16>(unit.glowLevel + (blue ? kBlueFillStep : kDarkFillStep));
    return 0;
}

// What BattleUnit::DefeatHalves decides of the frame; the draws into `scene`.
int UnitFrame::DefeatHalvesAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (unit.glowLevel > kSplitEnd)
    {
        unit.glowLevel = 0;
        unit.slotTint = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    const int shift = 2 * unit.glowLevel;
    const int half = picture.height >> 1;
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x - shift,
        y,
        x + picture.width - shift,
        y + half,
        x - shift,
        y,
        picture.height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + shift,
        y + half,
        x + shift + picture.width,
        y + picture.height,
        x + shift,
        y,
        picture.height,
        scene
    );

    if (unit.glowLevel >= kHalvesSlow)
    {
        ++unit.glowLevel;
    }
    else if (unit.glowLevel == kHalvesPause)
    {
        ++unit.slotTint;

        if (unit.slotTint == kHalvesPause)
        {
            unit.glowLevel = kHalvesSlow;
        }
    }
    else
    {
        unit.glowLevel = static_cast<i16>(unit.glowLevel + kHalvesFast);
    }

    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// What BattleUnit::DefeatHop decides of the frame; the draws into `scene`.
int UnitFrame::DefeatHopAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel >= static_cast<i16>(BattleUnit::HopHeights.size()))
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    if (unit.glowLevel == 0)
    {
        const u16 sound = unit.castAct.view;
        unit.standAct.view = kDefeatSound;
        services.audio.PlaySound(sound, services.config.soundVolume);
    }

    ScenePainter painter(services, scene);
    painter.SetSource(picture.data.data());
    painter.Options().alpha = Painter::MaxAlpha - unit.glowLevel;
    painter.DrawPicture(
        x,
        y - BattleUnit::HopHeights[static_cast<std::size_t>(unit.glowLevel)],
        picture.width,
        picture.height,
        unit.drawFlags | Painter::Translucent | Painter::Flip
    );
    ++unit.glowLevel;
    return 0;
}

// What BattleUnit::DefeatQuarters decides of the frame; the draws into `scene`.
int UnitFrame::DefeatQuartersAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (unit.glowLevel >= kSplitEnd)
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    const int shift = unit.glowLevel;
    const int halfWidth = picture.width >> 1;
    const int halfHeight = picture.height >> 1;
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x - shift,
        y - shift,
        x + halfWidth - shift,
        y + halfHeight - shift,
        x - shift,
        y - shift,
        halfHeight,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x - shift,
        y + halfHeight + shift,
        x + halfWidth - shift,
        y + picture.height + shift,
        x - shift,
        y + shift,
        picture.height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + halfWidth + shift,
        y - shift,
        x + picture.width + shift,
        y + halfHeight - shift,
        x + shift,
        y - shift,
        picture.height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + halfWidth + shift,
        y + halfHeight + shift,
        x + picture.width + shift,
        y + picture.height + shift,
        x + shift,
        y + shift + kQuarterDrop,
        picture.height,
        scene
    );
    ++unit.glowLevel;
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// What BattleUnit::DefeatRising decides of the frame; the draws into `scene`.
int UnitFrame::DefeatRisingAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel > kRisingEnd)
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    ScenePainter painter(services, scene);
    const int tint = 2 * (unit.glowLevel / kRisingTintDivisor);
    DrawOptions& options = painter.Options();
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    options.scaleHeight = unit.glowLevel + picture.height;
    unit.drawFlags |= Painter::ScaledGlow;
    painter.DrawPicture(x, y - kRisingTintLift * tint - unit.glowLevel, picture.width, picture.height, unit.drawFlags);
    unit.glowLevel = static_cast<i16>(unit.glowLevel + kRisingStep);
    return 0;
}

// What BattleUnit::DefeatSink decides of the frame; the draws into `scene`.
int UnitFrame::DefeatSinkAdvance(BattleServices& services, const Picture& picture, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel <= kFadedOut)
    {
        unit.glowLevel = 0;
        unit.slotTint = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    ScenePainter painter(services, scene);
    const int x = unit.placement.x - static_cast<i16>(unit.originX);
    const int y = unit.placement.y - unit.mainAct.y;
    painter.Options().alpha = Painter::MaxAlpha;

    if (unit.glowLevel <= kSinkFadeFrom)
    {
        painter.Options().alpha = unit.glowLevel + kSinkAlphaBase;
    }

    painter.DrawPicture(x, y, picture.width, picture.height, unit.drawFlags | Painter::Translucent);
    painter.Options().alpha = 0;
    DrawOptions& options = painter.Options();
    options.tintRed = unit.glowLevel;
    options.tintGreen = unit.glowLevel;
    options.tintBlue = unit.glowLevel;
    painter.DrawPicture(
        x,
        y + kSinkStep * unit.glowLevel,
        picture.width,
        picture.height,
        unit.drawFlags | Painter::Additive
    );
    --unit.glowLevel;
    return 0;
}

// What BattleUnit::DefeatSlide decides of the frame; the draws into `scene`.
int UnitFrame::DefeatSlideAdvance(BattleServices& services, const Picture& picture, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel <= kFadedOut)
    {
        unit.glowLevel = 0;
        unit.slotTint = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    ScenePainter painter(services, scene);
    const int x = unit.placement.x - static_cast<i16>(unit.originX);
    const int y = unit.placement.y - unit.mainAct.y;

    if (unit.glowLevel >= kSlideFadeFrom)
    {
        painter.Options().alpha = unit.glowLevel + Painter::MaxAlpha;
    }

    painter.DrawPicture(x, y, picture.width, picture.height, unit.drawFlags | Painter::Translucent);
    painter.Options().alpha = 0;
    DrawOptions& options = painter.Options();
    options.tintRed = unit.glowLevel;
    options.tintGreen = unit.glowLevel;
    options.tintBlue = unit.glowLevel;
    painter.DrawPicture(x + 2 * unit.glowLevel, y, picture.width, picture.height, unit.drawFlags | Painter::Additive);

    --unit.glowLevel;
    return 0;
}

// What BattleUnit::DefeatSplitColumns decides of the frame; the draws into `scene`.
int UnitFrame::DefeatSplitColumnsAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (unit.glowLevel > kSplitEnd)
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    const int shift = 2 * unit.glowLevel;
    const int half = picture.height >> 1;
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x - shift,
        y,
        x + picture.width - shift,
        y + half,
        x - shift,
        y,
        picture.height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + shift,
        y + half,
        x + shift + picture.width,
        y + picture.height,
        x + shift,
        y,
        picture.height,
        scene
    );
    unit.glowLevel = static_cast<i16>(unit.glowLevel + kSplitStep);
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// What BattleUnit::DefeatSplitRows decides of the frame; the draws into `scene`.
int UnitFrame::DefeatSplitRowsAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (unit.glowLevel > kSplitEnd)
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    const int shift = 2 * unit.glowLevel;
    const int half = picture.width >> 1;
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x,
        y - shift,
        x + half,
        y + picture.height - shift,
        x,
        y - shift,
        picture.height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + half,
        y + shift,
        x + picture.width,
        y + shift + picture.height,
        x,
        y + shift,
        picture.height,
        scene
    );
    unit.glowLevel = static_cast<i16>(unit.glowLevel + kSplitStep);
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// What BattleUnit::DefeatStrips decides of the frame; the draws into `scene`.
int UnitFrame::DefeatStripsAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ScenePainter painter(services, scene);

    if (unit.glowLevel > kSplitEnd)
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    const int shift = 2 * unit.glowLevel;
    const int strip = picture.height >> 2;
    const int height = picture.height;
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x - shift,
        y,
        x + picture.width - shift,
        y + strip,
        x - shift,
        y,
        height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + shift,
        y + strip,
        x + shift + picture.width,
        y + 2 * strip,
        x + shift,
        y,
        height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x - shift,
        y + 2 * strip,
        x + picture.width - shift,
        y + 3 * strip,
        x - shift,
        y,
        height,
        scene
    );
    this->DrawDefeatPieceAdvance(
        services,
        picture,
        x + shift,
        y + 3 * strip,
        x + shift + picture.width,
        y + height,
        x + shift,
        y,
        height,
        scene
    );
    unit.glowLevel = static_cast<i16>(unit.glowLevel + kSplitStep);
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    return 0;
}

// What BattleUnit::DefeatWave decides of the frame; the draws into `scene`.
int UnitFrame::DefeatWaveAdvance(BattleServices& services, const Picture& picture, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel >= kWaveEnd)
    {
        unit.glowLevel = 0;
        unit.slotTint = 0;
        return unit.EndDefeat(services);
    }

    unit.StartDefeatDraw(services, picture);
    ScenePainter painter(services, scene);
    const int wave = unit.glowLevel / kWaveShare + 1;
    painter.SetWave(wave);
    painter.Options().alpha = Painter::MaxAlpha - wave;
    painter.DrawPicture(
        unit.placement.x - static_cast<i16>(unit.originX),
        unit.placement.y - unit.mainAct.y,
        picture.width,
        picture.height,
        unit.drawFlags | Painter::Translucent
    );
    painter.Options().alpha = 0;
    painter.SetWave(0);
    ++unit.glowLevel;
    return 0;
}

// What BattleUnit::DefeatZoom decides of the frame; the draws into `scene`.
int UnitFrame::DefeatZoomAdvance(BattleServices& services, const Picture& picture, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel >= kZoomEnd)
    {
        unit.glowLevel = 0;
        unit.slotTint = 0;
        return unit.EndDefeat(services);
    }

    if (unit.glowLevel == 0)
    {
        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
    }

    ScenePainter painter(services, scene);
    painter.SetZoom(
        unit.mainAct.x,
        unit.mainAct.y,
        unit.glowLevel * unit.slotTint + kZoomUnit,
        kZoomUnit - unit.glowLevel * (unit.slotTint + 1)
    );
    painter.SetSource(picture.data.data());
    const int x = unit.placement.x - static_cast<i16>(unit.originX);
    const int y = unit.placement.y - unit.mainAct.y;

    if ((unit.drawFlags & Painter::Mirror) != 0)
    {
        painter.DrawZoomedMirrored(x, y, picture.width, picture.height);
    }
    else
    {
        painter.DrawZoomed(x, y, picture.width, picture.height);
    }

    unit.slotTint = static_cast<i16>(unit.slotTint + kZoomStep);
    ++unit.glowLevel;
    return 0;
}

// What BattleUnit::RunDefeatParticles decides of the frame; the draws into `scene`.
int UnitFrame::RunDefeatParticlesAdvance(BattleServices& services, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (ExplosionFrame(services.state.projector)
            .Explosion(services.screen, services.painter, services.crtRandom, unit.absorb, scene) == 1)
    {
        unit.glowLevel = kParticlesDone;
    }

    return 0;
}

// What BattleUnit::DefeatBurst decides of the frame; the draws into `scene`.
int UnitFrame::DefeatBurstAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel == kParticlesDone)
    {
        return unit.EndParticleDefeat(services);
    }

    if (unit.absorbPixels.empty())
    {
        ParticleEffect& effect = unit.absorb;
        effect = ParticleEffect{};
        unit.DecodeForParticles(services, picture);
        effect.width = picture.width;
        effect.height = picture.height;
        effect.x = x;
        effect.y = y;
        effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mixed);
        effect.bursts = 1;
        effect.divisor = kBurstDivisor;

        if (unit.defeatPose == kBurstPose)
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
            effect.targetLeft = unit.facing == 1 ? kSweepRight : kSweepLeft;
            effect.targetTop = y - kSweepAbove;
            effect.targetWidth = kBurstTarget;
            effect.targetHeight = picture.height + kSweepAbove;
            effect.speed = kSweepSpeed;
            effect.lifeBase = kSweepLife;
        }

        if (unit.Mirrored() == 1)
        {
            effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mirrored);
        }

        services.state.projector.SetScreen(Display::Width, Display::Height);

        if (unit.defeatPose == kBurstPose)
        {
            unit.castAct.view = kDefeatSound;
        }

        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
        unit.glowLevel = 1;
    }

    return this->RunDefeatParticlesAdvance(services, scene);
}

// What BattleUnit::DefeatParticles decides of the frame; the draws into `scene`.
int UnitFrame::DefeatParticlesAdvance(BattleServices& services, const Picture& picture, int x, int y, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel == kParticlesDone)
    {
        return unit.EndParticleDefeat(services);
    }

    if (unit.glowLevel == 0)
    {
        if (unit.throwSteps >= kWobbles)
        {
            unit.recoilTint = 0;
            unit.glowLevel = 1;
            unit.throwSteps = 0;
            return 0;
        }

        if (unit.recoilTint == kWobbleTop)
        {
            services.audio.PlaySound(kWobbleSound, services.config.soundVolume);
        }

        ScenePainter painter(services, scene);
        painter.SetSource(picture.data.data());
        DrawOptions& options = painter.Options();
        options.tintRed = unit.recoilTint;
        options.tintGreen = unit.recoilTint;
        options.tintBlue = unit.recoilTint;
        painter.DrawPicture(
            unit.placement.x - static_cast<i16>(unit.originX),
            unit.placement.y - unit.mainAct.y,
            picture.width,
            picture.height,
            unit.drawFlags | Painter::Additive
        );
        const i32 wobble = unit.throwSteps;

        if (wobble % 2 != 0)
        {
            --unit.recoilTint;
        }
        else
        {
            unit.recoilTint = static_cast<i16>(unit.recoilTint + kWobbleRise);
        }

        if (unit.recoilTint >= kWobbleTop || unit.recoilTint <= kWobbleBottom)
        {
            unit.throwSteps = wobble + 1;
        }

        return 0;
    }

    if (unit.absorbPixels.empty())
    {
        ParticleEffect& effect = unit.absorb;
        effect = ParticleEffect{};
        unit.DecodeForParticles(services, picture);
        effect.width = picture.width;
        effect.height = picture.height;
        effect.x = x;
        effect.y = y;
        effect.targetLeft = unit.facing == 1 ? kStripRight : 0;
        effect.targetTop = 1;
        effect.targetWidth = kStripWidth;
        effect.targetHeight = 1;
        effect.speed = kStripSpeed;
        effect.lifeBase = kStripLife;
        effect.bursts = static_cast<u16>(picture.height >> 2);
        effect.divisor = kStripDivisor;
        effect.flags = kStripFlags;

        if (unit.Mirrored() == 1)
        {
            effect.flags = static_cast<u16>(effect.flags | ParticleEffect::Mirrored);
        }

        effect.tintRed = kStripTint;
        effect.tintGreen = kStripTint;
        effect.tintBlue = kStripTint;
        services.state.projector.SetScreen(Display::Width, Display::Height);
        services.audio.PlaySound(kDefeatSound, services.config.soundVolume);
        unit.glowLevel = kParticlesStarted;
    }

    ++unit.glowLevel;
    unit.absorb.x = x + unit.glowLevel % kShakeSpan;
    return this->RunDefeatParticlesAdvance(services, scene);
}

// What BattleUnit::DefeatProjected decides of the frame; the draws into `scene`.
int UnitFrame::DefeatProjectedAdvance(BattleServices& services, const Picture& picture, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;

    if (unit.glowLevel >= kParticlesDone)
    {
        unit.glowLevel = 0;
        return unit.EndDefeat(services);
    }

    ProjectedPicture& projected = unit.projected;

    if (unit.glowLevel == 0)
    {
        projected.scaleX = kZoomUnit;
        projected.scaleY = kZoomUnit;
        projected.unk08 = picture.width >> 1;
        projected.unk0C = picture.height;
        projected.width = picture.width;
        projected.height = picture.height;
        projected.flags = ProjectedPicture::Mixed;
        projected.x = unit.placement.x - (picture.width >> 1);
        projected.y = unit.placement.y;

        if (unit.Mirrored() == 1)
        {
            projected.flags = static_cast<u16>(projected.flags | ProjectedPicture::Mirrored);
        }

        projected.angle = kProjectedAngle;
        unit.projectedPixels.clear();
        const std::optional<RawPicture> raw = RleCodec(services.painter).Decode(picture.data);

        if (raw.has_value())
        {
            unit.projectedPixels.resize(raw->pixels.size() / sizeof(u16));
            std::memcpy(unit.projectedPixels.data(), raw->pixels.data(), unit.projectedPixels.size() * sizeof(u16));
        }

        projected.pixels = unit.projectedPixels;
        const u16 sound = unit.castAct.view;
        unit.standAct.view = kDefeatSound;
        services.audio.PlaySound(sound, services.config.soundVolume);
    }

    unit.glowLevel = static_cast<i16>(unit.glowLevel + kProjectedStep);
    projected.tintRed = kProjectedTint;
    projected.tintGreen = kProjectedTint;
    projected.tintBlue = kProjectedTint;
    projected.scaleX += kProjectedGrowth;
    projected.scaleY += kProjectedGrowth;
    ExplosionFrame(services.state.projector).Scaled(services.screen, services.painter, projected, scene);
    return 0;
}

// What BattleUnit::DrainEffectFrame decides of the frame; the draws into `scene`.
int UnitFrame::DrainEffectFrameAdvance(BattleServices& services, int slot, Scene& scene)
{
    const Scene::Owner owner(scene, &this->unit);
    BattleUnit& unit = this->unit;
    ParticleEffect& effect = unit.drains[static_cast<std::size_t>(slot)];
    effect.tintRed = kDrainTint;
    effect.tintGreen = kDrainTint;
    effect.tintBlue = kDrainTint;
    unit.absorbing = 1;

    if (ExplosionFrame(services.state.projector)
            .Explosion(services.screen, services.painter, services.crtRandom, effect, scene) != 1)
    {
        return 0;
    }

    unit.drainPixels[static_cast<std::size_t>(slot)].clear();
    effect = ParticleEffect{};
    return 1;
}

}  // namespace swd3::smooth
