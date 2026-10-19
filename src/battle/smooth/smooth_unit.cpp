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

namespace swd3
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

}  // namespace

// swd32.exe 0x0047CC60: a red and green tint pulse rising 2..24 and falling back over the shadow picture. The
// rebuild's continuous motion keeps the step here, the tint it gives back (0 at the turn), and the draw in IdleDraw.
int BattleUnit::FlashStep()
{
    const u16 phase = this->unk2A68;

    if (phase == BattleUnit::FlashPeak)
    {
        const u16 fall = this->unk2A6A;

        if (fall == 0)
        {
            this->unk2A68 = BattleUnit::FlashStart;
            this->unk2A6A = BattleUnit::FlashPeak;
            return 0;
        }

        if (this->held == 0)
        {
            this->unk2A6A = static_cast<u16>(fall - 1);
        }

        return this->unk2A6A;
    }

    if (this->held == 0)
    {
        this->unk2A68 = static_cast<u16>(phase + 1);
    }

    return this->unk2A68;
}

// swd32.exe 0x00483700: the overlay picture (`frame`, `picture` - 1; nothing when either is 0) drawn additively at
// (x + offsetX, y + offsetY), mirrored like the unit: a facing unit measures offsetX (when not 0) from the right edge
// of its current picture, less the overlay's width. The rebuild's continuous motion keeps the choice here (at the
// walk's place in `view`), the draw in DrawWalk.
void BattleUnit::ChooseOverlay(
    BattleServices& services,
    u16 frame,
    u16 picture,
    int offsetX,
    int offsetY,
    WalkView& view
)
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

    view.overlay = {frame, static_cast<u16>(picture - 1), true};
    view.overlayX = view.x + static_cast<i16>(this->overlayX);
    view.overlayY = view.y + offsetY;
    view.overlayFlags = this->frameFlags | Painter::Additive;
}

// What ReturnHome decides of the frame: the acts stepped, the line walked, the sound, the arrival and the overlay
// chosen; the view keeps no picture when it draws nothing.
int BattleUnit::HomeAdvance(BattleServices& services, const BattleUnit& target, i32 id, i32 action, WalkView& view)
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

    services.painter.SetSource(this->picture->data.data());
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
    const Stride stride = this->HomeStride(services, target, act, pose, this->throwSteps + 1);
    this->actOffsetX = stride.offsetX;
    this->frameFlags = stride.flags;
    this->unk2670 = stride.poseX;
    LineWalker& line = this->flight;
    line = stride.line;
    this->throwSteps = stride.steps;

    if (!stride.arrived && PaintRecord::Active() != nullptr)
    {
        const ActState nextAct = services.acts.Peek(act);
        const Stride next = this->HomeStride(services, target, nextAct, services.acts.Peek(pose), this->throwSteps + 1);
        view.motion = BattleUnit::StrideMotion(stride, act, next, nextAct);
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (this->status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.flags = invisible ? this->frameFlags | Painter::SmearUnder : this->frameFlags;

    if (stride.arrived)
    {
        view.x = line.toX;
        view.y = line.toY;
        this->throwSteps = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        this->unk26CC &= ~kWalksHome;
        return 1;
    }

    view.x = line.fromX + line.x;
    view.y = line.fromY + line.y;

    if (!invisible)
    {
        view.shadow = true;
        view.shadowY = view.y + act.y;
        view.shadowTint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
        this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
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
bool BattleUnit::WalkAdvance(BattleServices& services, const BattleUnit& target, WalkView& view)
{
    if (this->unk2AD8 == 1)
    {
        return false;
    }

    if (this->unk2B00 == 1 && (this->command.flags & UnitCommand::FadesIn) != 0)
    {
        this->commandA = this->commandB;
        return false;
    }

    if ((this->status & BattleUnit::Invisible) != 0 && this->FadeInvisible(services, 0) == 1)
    {
        return false;
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
        return false;
    }

    this->picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->picture == nullptr)
    {
        return false;
    }

    services.painter.SetSource(this->picture->data.data());
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
        return false;
    }

    const Stride stride = this->WalkStride(services, target, act, pose, this->throwSteps + 1);
    this->actOffsetX = stride.offsetX;
    this->frameFlags = stride.flags;
    this->unk2670 = stride.poseX;

    if (this->kind == 1 && this->unk2A8E != act.picture)
    {
        this->unk2A8E = act.picture;
    }

    this->throwSteps = stride.steps;
    LineWalker& line = this->flight;
    line = stride.line;

    if (!stride.arrived && PaintRecord::Active() != nullptr)
    {
        const ActState nextAct = services.acts.Peek(act);
        const Stride next = this->WalkStride(services, target, nextAct, services.acts.Peek(pose), this->throwSteps + 1);
        view.motion = BattleUnit::StrideMotion(stride, act, next, nextAct);
    }

    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const bool invisible = (this->status & BattleUnit::Invisible) != 0;
    view.picture = {act.frame, act.picture, true};
    view.flags = this->frameFlags;

    if (stride.arrived)
    {
        view.x = invisible ? line.toX - static_cast<i16>(this->actOffsetX) : line.toX;
        view.y = line.toY;

        if (invisible)
        {
            view.flags |= Painter::SmearUnder;
        }

        this->throwSteps = 0;
        this->unk2A8E = 0;
        line = LineWalker{};
        pose = ActState{};
        act = ActState{};
        this->commandA = this->commandB;
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
    view.shadowTint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
    this->unk26A4 = (this->frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    return true;
}

// A walk as WalkAdvance or HomeAdvance decided it: the shadow under the picture, the picture, the overlay.
void BattleUnit::DrawWalk(BattleServices& services, const WalkView& view)
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
bool BattleUnit::IdleAdvance(BattleServices& services, int paused, IdleView& view)
{
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
        return false;
    }

    if ((this->unk26B8 & kFlag26B8) != 0)
    {
        if (this->HurtFrame(services) == 1)
        {
            this->charging = 0;
            this->ClearFlag26B8();
        }

        return false;
    }

    if (this->unk2AAC == 1)
    {
        return false;
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
            return false;
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
        return false;
    }

    this->shadowPicture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (this->shadowPicture == nullptr)
    {
        return false;
    }

    const Picture& picture = *this->shadowPicture;
    services.painter.SetSource(picture.data.data());
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
        return false;
    }

    if ((marks & BattleUnit::UnkMark200) != 0)
    {
        this->FadeMainIn(services);
        return false;
    }

    if (this->commandA == 0 || this->commandA == BattleUnit::UnkCommand1C)
    {
        if ((marks & BattleUnit::InvisibleMark) != 0)
        {
            this->FadeInvisible(services, 0);
            return false;
        }

        if ((marks & BattleUnit::UnkMark400) != 0)
        {
            this->FadeInvisible(services, 1);
            return false;
        }
    }

    view.picture = {act.frame, act.picture, true};
    view.x = this->unk26A8 + this->placement.x - static_cast<i16>(this->originX);
    view.y = this->placement.y - this->unk26AC - act.y;
    view.mirrored = this->drawFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip);
    view.flags = this->drawFlags;
    view.motion = PaintRecord::Active() != nullptr ? this->IdleMotion(services, act, view.x, view.y)
                                                   : std::array<int, 3>{0, 0, 0};
    view.body = this->guarding == 0 && (marks & BattleUnit::GuardingMark) == 0;

    if (view.body)
    {
        const bool invisible = (this->status & BattleUnit::Invisible) != 0;
        view.shadow = this->drawAtSaved != kRising && this->drawAtSaved != kDropping && !invisible;
        view.shadowX = this->placement.x - static_cast<i16>(this->originX);
        view.shadowY = this->placement.y;
        view.shadowTint = this->unk2B00 == 1 ? kInvisibleShadowTint : kShadowTint;
        view.whiteOutline = (this->status & kOutlinedStatus) != 0;
        view.outline = this->outline == 1;
        view.smear = invisible;
        view.alpha = (this->drawFlags & ActState::Signal) == ActState::Signal ? act.signal : 0;
        view.signal = (this->drawFlags & ActState::Signal) == ActState::Signal;
        view.wave = act.wait;
        view.held = (marks & BattleUnit::Held) != 0;
        view.flash = !invisible && (this->status & BattleUnit::Barrier) != 0;
        view.gray = (marks & BattleUnit::UnkMark4) != 0;
        view.red = this->commandA == BattleUnit::UnkCommand2;
        view.gold = this->goldGuard == 1 && this->unk26D2 == 0;
        view.goldValue = services.party.variables[kGoldVariable];
        view.goldX = this->placement.x + kGoldLeft;
        view.goldY = this->placement.y - act.y - kGoldAbove;

        if (view.flash)
        {
            view.flashTint = this->FlashStep();
            view.flashX = this->placement.x - static_cast<i16>(this->originX);
            view.flashY = this->placement.y - this->mainAct.y;
        }

        // What the draws leave in the unit (the outline colour, the last mode drawn with).
        if (view.whiteOutline)
        {
            this->colorPair = kWhiteColorPair;
        }

        if (view.outline)
        {
            this->colorPair = BattleUnit::OutlineColorPair;
        }

        if (view.shadow)
        {
            this->unk26A4 = view.mirrored | Painter::Shadow;
        }

        if (view.smear || view.flash || view.gray || view.red)
        {
            this->unk26A4 = view.mirrored;
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
            view.thrownLost = true;
            return true;
        }

        this->picture = services.archive.GetPicture(thrown.frame, thrown.picture);

        if (this->picture != nullptr)
        {
            view.thrown = {thrown.frame, thrown.picture, true};
            view.thrownX =
                static_cast<i16>(act.anchorX) + this->placement.x - thrown.x - static_cast<i16>(this->originX);
            view.thrownY = static_cast<i16>(act.anchorY) + this->placement.y - thrown.y - act.y;
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

    if (this->guarding == 0 && (this->marks & BattleUnit::GuardingMark) == 0 &&
        (this->status & BattleUnit::UnkStatus4) == 0 && act.rotation != 0 && act.actionCode != 0)
    {
        this->unk2554 = services.archive.GetPicture(act.rotation, static_cast<u16>(act.actionCode - 1));

        if (this->unk2554 != nullptr)
        {
            this->unk2698 = static_cast<i16>(act.offsetX);
            view.overlay = {act.rotation, static_cast<u16>(act.actionCode - 1), true};
            view.overlayX = this->unk2698 + this->placement.x - static_cast<i16>(this->originX);
            view.overlayY = static_cast<i16>(act.offsetY) + this->placement.y - this->unk26AC - act.y;
            view.overlayFlags = this->drawFlags | Painter::Additive;
        }
    }

    return true;
}

// What IdleFrame draws of the unit as IdleAdvance decided it: its shadow, outlines, body, barrier flash, grey and red
// copies, the gold, the thrown act and the overlay.
void BattleUnit::IdleDraw(BattleServices& services, const IdleView& view) const
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

}  // namespace swd3
