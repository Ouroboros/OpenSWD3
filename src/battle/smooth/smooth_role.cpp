#include "battle/smooth/smooth_role.h"

#include <algorithm>
#include <array>

#include "audio/audio_manager.h"
#include "battle/battle_message.h"
#include "battle/battle_role.h"
#include "battle/battle_role_constants.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "battle/smooth/smooth_unit.h"
#include "config/config.h"
#include "core/words.h"
#include "draw/painter.h"
#include "org/database.h"
#include "pic/rle.h"
#include "platform/display.h"
#include "random/random.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"

namespace swd3::smooth
{

namespace
{

constexpr i32 kCriticalAction = 0x29;
constexpr i32 kForcedAttackAction2 = 0x31;  // unk2A8C 2
constexpr i16 kForcedX = 0x140;             // a unit with unk2B04 attacks from the middle of the field
constexpr i16 kForcedY = 0x136;
constexpr u16 kSecondForcedAttack = 2;
constexpr u32 kBoomerangBackHome = 2;
constexpr u32 kBoomerangGoing = 0;
constexpr u32 kBoomerangReturning = 1;
constexpr int kChargeAlpha = 8;
constexpr int kChargeBlue = 10;
constexpr u16 kChargePicture = 2;  // the charge act is run on to its third picture
constexpr i8 kChargeTrailImages = 3;
constexpr int kDashStep = 10;
constexpr int kFaintShadowTint = -1;
constexpr u16 kGlowDone = 2;
constexpr u16 kGlowFalling = 1;
constexpr u16 kGlowRising = 0;
constexpr i16 kGlowStep = 2;
constexpr int kGlowTintBase = 0x1F;  // the tint is glowLevel / 2 - kGlowTintBase
constexpr i32 kNoImpact = -1;        // a queued act without an impact act that lands no hit
constexpr u16 kNoUnk2A8E = 0xFFFF;
constexpr i16 kStandBackEnd = 2;
constexpr i32 kStepBackAction = 0x2A;
constexpr i16 kStepBackEnd = 6;
constexpr int kStepBackShift = 0x10;
constexpr int kSwarmAbove = 0x28;
constexpr u16 kSwarmActs = 8;
constexpr i16 kSwarmInterval = 3;  // a new act every third frame
constexpr u16 kSwarmShake = 0x34;
constexpr int kThrowSteps = 8;  // line steps a frame per frame flown
constexpr i32 kTrailBlueTint = 0xC;
constexpr u32 kBeamFadeIn = 0x2000;
constexpr i16 kBeamFadeStep = 2;
constexpr u32 kBeamFly = 0x4;
constexpr u32 kBeamGlow = 0x1;
constexpr u32 kBeamImpact = 0x4000;
constexpr u32 kBeamRise = 0x2;
constexpr i16 kBeamRiseStart = -0x1F;
constexpr u32 kLeapAtTarget = 1;
constexpr u32 kLeapCasting = 0;
constexpr u32 kLeapHome = 3;
constexpr i32 kLeapReturnAction = 2;  // the walk home takes the action two after the magic act's
constexpr u32 kLeapReturning = 2;
constexpr int kSpeedingFlight = -1;  // FlyAct: the steps double each frame
}  // namespace

RoleFrame::RoleFrame(BattleRole& role)
    : role(role)
{
}

// What BattleRole::AttackFrame decides of the frame: the attack act and its follow-up stepped, the recoil, the quake,
// the field fade and the strikes; its result (1 once the attack is over), the draws into `scene`.
int RoleFrame::AttackAdvance(BattleServices& services, BattleUnit& target, int slot, Scene& scene)
{
    BattleRole& role = this->role;
    BattleState& battle = services.state;

    if (role.unk2A74 != 0 || role.unk2AD8 == 1)
    {
        return 0;
    }

    if (role.unk2B00 == 1 && (role.command.flags & UnitCommand::FadesIn) != 0 && role.fadedIn == 0 &&
        role.DrawFadeIn(services, target) == 0)
    {
        role.fadedIn = 1;
        return 0;
    }

    if (role.unk2B04 == 1)
    {
        role.placement.x = kForcedX;
        role.placement.y = kForcedY;
    }

    role.unk2AAC = 1;
    ActState& attack = role.standAct;
    attack.hold = 0;

    if (role.held == 1)
    {
        attack.hold = 1;
    }

    if ((attack.target & ActState::Holds) != 0)
    {
        attack.hold = 1;
    }

    const bool forced = role.unk2B00 == 1 || role.unk2B04 == 1;

    if (role.critical == 1)
    {
        battle.criticalAttack = 1;
        attack.id = role.actId;
        attack.action = kCriticalAction;

        if (forced)
        {
            attack.action = kAttackAction;
        }
        else
        {
            role.command.castAct = role.weapon.data.unk48;
        }
    }
    else
    {
        attack.id = role.actId;
        attack.action = kAttackAction;

        if (forced && role.unk2A8C != 0)
        {
            attack.action = role.unk2A8C == kSecondForcedAttack ? kForcedAttackAction2 : kForcedAttackAction;
        }
    }

    UnitFrame(role).Action(services, target, attack, scene);

    if ((attack.target & ActState::Recoils) != 0)
    {
        if (attack.effectA != 0)
        {
            role.recoilAct.id = attack.effectA;
            role.unk267C |= BattleUnit::Recoiling;
            role.recoilAct.action = attack.effectQ;
        }

        if ((attack.target & ActState::Holds) != 0)
        {
            attack.hold = 1;
        }

        attack.target = static_cast<u16>(attack.target & ~ActState::Recoils);
        attack.effectA = 0;
        attack.effectQ = 0;
    }

    if ((role.unk267C & BattleUnit::Recoiling) != 0 &&
        UnitFrame(role).Recoil(services, role.recoilAct, static_cast<i16>(attack.anchorY), scene) == 1)
    {
        if ((attack.target & ActState::Holds) != 0)
        {
            attack.target = static_cast<u16>(attack.target & ~ActState::Holds);
        }

        attack.hold = 0;
        role.unk267C &= ~BattleUnit::Recoiling;
    }

    if ((attack.target & ActState::Quakes) != 0)
    {
        battle.quaking = 1;
        battle.quakePhase = 0;
    }

    ActState& follow = role.slotActs[static_cast<std::size_t>(slot)];

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
        role.unk267C |= BattleUnit::FollowingUp;
        follow = ActState{};
    }

    if ((attack.target & ActState::UnkTarget4) != 0)
    {
        target.RaiseFlag26B8();
        attack.target = 0;
        role.unk267C |= BattleUnit::FollowingUp;
    }

    if ((attack.target & ActState::Strikes) != 0)
    {
        role.unk267C |= BattleUnit::FollowingUp;
        role.slotTint = 0;
        battle.proficiencyBonus = 0;
        follow = ActState{};
        role.Strike(services, target, 1);
        attack.target = 0;
    }

    if ((role.unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    follow.id = role.command.castAct;
    follow.action = 0;

    if (attack.effectA != 0)
    {
        follow.id = attack.effectA;
    }

    if (follow.id != 0)
    {
        if ((role.command.flags & UnitCommand::TargetsUnit) != 0)
        {
            const int fromX = static_cast<i16>(role.actAnchorX) - static_cast<i16>(role.actOffsetX) + role.placement.x;
            const int fromY = role.placement.y + static_cast<i16>(attack.anchorY) - attack.y;

            if (UnitFrame(role).Fly(
                    services,
                    target,
                    role.slotActs[kFlightAct],
                    0,
                    follow.id,
                    fromX,
                    fromY,
                    static_cast<i16>(role.command.target),
                    0,
                    scene
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
            role.AimAction(
                services,
                target,
                follow,
                static_cast<i16>(attack.anchorX),
                static_cast<i16>(attack.anchorY)
            );
        }

        if ((follow.target & ActState::Strikes) != 0)
        {
            role.Strike(services, target, 1);
            follow.target = 0;
        }
    }
    else
    {
        follow.waiting = 1;
    }

    if (follow.waiting != 1)
    {
        // A missing picture is read through a null pointer in the original.
        if (role.picture != nullptr)
        {
            scene.Add(
                PictureStep{
                    .picture = RefOf(services.archive, role.picture),
                    .x = static_cast<i16>(role.aimX),
                    .y = static_cast<i16>(role.aimY),
                    .mode = role.unk26A4,
                }
            );
        }

        return 0;
    }

    if (follow.id != 0 && (role.command.flags & UnitCommand::TargetsUnit) == 0 && role.slotTint > kSlotFadeEnd)
    {
        int tint = role.slotTint;

        if ((role.unk26A4 & ActState::NoBlend) != 0)
        {
            tint = 0;
            role.slotTint = kSlotFadeEnd;
        }

        if (role.picture != nullptr)
        {
            scene.Add(
                PictureStep{
                    .picture = RefOf(services.archive, role.picture),
                    .x = static_cast<i16>(role.aimX),
                    .y = static_cast<i16>(role.aimY),
                    .mode = role.unk26A4,
                    .tinted = true,
                    .tintRed = tint,
                    .tintGreen = tint,
                    .tintBlue = tint,
                    .palette = true,
                }
            );
        }
        else
        {
            // Without a picture the tint stays set for the next draw.
            scene.Add(
                [tint](BattleServices& drawing)
                {
                    DrawOptions& options = drawing.painter.Options();
                    options.tintRed = tint;
                    options.tintGreen = tint;
                    options.tintBlue = tint;
                }
            );
        }

        role.slotTint = static_cast<i16>(role.slotTint - kSlotFadeStep);

        if (role.held == 1)
        {
            role.slotTint = static_cast<i16>(role.slotTint + kSlotFadeStep);
        }

        return 0;
    }

    if (attack.waiting != 1)
    {
        return 0;
    }

    role.standAct = ActState{};
    role.recoilAct = ActState{};
    follow = ActState{};
    role.slotActs[kFlightAct] = ActState{};
    role.slotActs[kFlightAct + 1] = ActState{};
    role.targets.fill(BattleUnit::NoTarget);
    role.slotTint = 0;
    role.recoilTint = 1;
    role.unk2AB0 = 0;
    role.fadedIn = 0;
    role.unk267C = 0;

    if (battle.criticalAttack == 0)
    {
        if (role.extraAttacks == 0)
        {
            role.unk26CC &= ~kRepeatAttack;
        }

        if (role.extraAttacks != 0)
        {
            --role.extraAttacks;
            return 0;
        }
    }

    ++battle.attacksSinceCritical;
    role.critical = 0;
    battle.criticalAttack = 0;
    return 1;
}

// What BattleRole::CastAt decides of the frame; the draws into `scene`.
int RoleFrame::CastAtAdvance(BattleServices& services, const BattleUnit& target, i32 spell, Scene& scene)
{
    BattleRole& role = this->role;
    ActState& act = role.standAct;
    act.id = role.actId;
    role.unk2AAC = 1;
    act.action = kCastPose;

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
    u32 flags = act.flags;
    auto offsetX = static_cast<i16>(act.x);
    role.actAnchorX = act.anchorX;

    if (role.facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = static_cast<i16>(role.picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(role.picture->width - act.anchorX);
        }
    }

    const int shadowHeight = role.picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = role.picture->height >> kShadowShearShift;
    const int tint = role.unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int x = role.placement.x - offsetX;
    services.audio.SetSoundPan(act.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
    role.unk26A4 = (flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter.DrawPicture(x, role.placement.y - shadowHeight, role.picture->width, role.picture->height, role.unk26A4);
    painter.DrawPicture(x, role.placement.y - act.y, role.picture->width, role.picture->height, flags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) == 0)
    {
        return 0;
    }

    ActState& effect = role.slotActs[0];
    effect.id = spell;
    effect.action = 0;

    if (services.acts.Run(effect) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(effect.frame, effect.picture);

    if (role.picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(role.picture->data.data());
    u32 effectFlags = effect.flags;
    auto effectOffsetX = static_cast<i16>(effect.x);
    role.actAnchorX = effect.anchorX;

    if (role.facing == 1)
    {
        effectFlags ^= Painter::Mirror;
        effectOffsetX = static_cast<i16>(role.picture->width - static_cast<u16>(effect.x));

        if (effect.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(role.picture->width - effect.anchorX);
        }
    }

    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    painter.DrawPicture(
        static_cast<i16>(targetX - effectOffsetX),
        static_cast<i16>(targetY - static_cast<u16>(effect.y)),
        role.picture->width,
        role.picture->height,
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

// What BattleRole::ThrowAct decides of the frame; the draws into `scene`.
int RoleFrame::ThrowActAdvance(BattleServices& services, const BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    u16 anchorX = 0;
    u16 anchorY = 0;
    ActState& act = role.attackAct;
    act.id = kThrownAct;
    act.action = 0;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
    role.actOffsetX = static_cast<u16>(act.x);
    role.frameFlags = act.flags;

    if (role.facing == 1)
    {
        role.frameFlags ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(act.x));
    }

    target.GetOffset(anchorX, anchorY);
    i16 toX = 0;
    i16 toY = 0;

    if (anchorX != 0 && anchorY != 0)
    {
        u16 originX = 0;
        u16 originY = 0;
        target.GetOrigin(originX, originY);
        toX = static_cast<i16>(originX + anchorX - role.actOffsetX);
        toY = static_cast<i16>(originY + anchorY - static_cast<u16>(act.y));
    }
    else
    {
        u16 x = 0;
        u16 y = 0;
        target.GetDrawPosition(x, y);
        toX = static_cast<i16>(x - role.actOffsetX);
        toY = static_cast<i16>(y - static_cast<u16>(act.y));
    }

    LineWalker& line = role.flight;
    line = LineWalker{};
    line.fromX = static_cast<i16>(role.actAnchorX) - static_cast<i16>(role.originX) + role.placement.x -
                 static_cast<i16>(role.actOffsetX);
    line.fromY = role.placement.y + static_cast<i16>(role.mainAct.anchorY) - role.mainAct.y - act.y;
    line.toX = toX;
    line.toY = toY;
    bool arrived = false;

    for (int taken = 0; taken < kThrowSteps * role.throwSteps; ++taken)
    {
        LineStep(line);

        if (line.fromX + line.x == line.toX)
        {
            arrived = true;
            break;
        }
    }

    ++role.throwSteps;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;

    if (arrived)
    {
        painter.DrawPicture(toX, toY, role.picture->width, role.picture->height, role.frameFlags);
        role.throwSteps = 0;
        line = LineWalker{};
        act = ActState{};
        return 1;
    }

    painter.DrawPicture(
        line.fromX + line.x,
        line.fromY + line.y,
        role.picture->width,
        role.picture->height,
        role.frameFlags
    );
    return 0;
}

// What BattleRole::StepBack decides of the frame; the draws into `scene`.
int RoleFrame::StepBackAdvance(BattleServices& services, int mode, Scene& scene)
{
    BattleRole& role = this->role;

    if (role.defeated == 1)
    {
        role.unk2AAC = 0;
        return 1;
    }

    role.glowLevel = mode == 0 ? kStandBackEnd : kStepBackEnd;

    if (role.IsHeldOrHolding() == 1)
    {
        if (role.fadeLevel <= role.glowLevel)
        {
            role.fadeLevel = BattleUnit::Opaque;
            return 1;
        }

        --role.fadeLevel;
        return 0;
    }

    ActState& act = role.recoilAct;

    if (role.fadeLevel <= role.glowLevel)
    {
        act = ActState{};
        role.fadeLevel = BattleUnit::Opaque;

        if (mode == 1)
        {
            role.unk2AAC = 1;
        }

        return 1;
    }

    role.unk2AAC = 1;
    act.id = role.actId;
    act.action = kStepBackAction;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 1;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    role.actOffsetX = static_cast<u16>(act.x);
    role.frameFlags = act.flags ^ Painter::Mirror;

    if (role.facing == 1)
    {
        role.frameFlags ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(act.x));
    }

    if (role.fadeLevel == BattleUnit::Opaque && mode == 1)
    {
        act.view = kStepBackSound;
        services.audio.PlaySound(kStepBackSound, services.config.soundVolume);
        services.audio.SetSoundPan(act.view, role.facing == 1 ? -kTargetPan : kTargetPan);
        act.view = 0;
    }

    u16 x = 0;
    u16 y = 0;
    role.GetPosition(x, y);

    if (mode == 1)
    {
        x = static_cast<u16>(role.facing == 1 ? x - kStepBackShift : x + kStepBackShift);
    }

    role.SetPosition(static_cast<i16>(x), static_cast<i16>(y));
    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
    painter.DrawPicture(
        role.placement.x - static_cast<i16>(role.actOffsetX),
        role.placement.y - act.y,
        role.picture->width,
        role.picture->height,
        role.frameFlags,
        PaletteArgument(*role.picture)
    );
    --role.fadeLevel;
    return 0;
}

// What BattleRole::GlowAct decides of the frame; the draws into `scene`.
int RoleFrame::GlowActAdvance(BattleServices& services, int x, int y, Scene& scene)
{
    BattleRole& role = this->role;
    ScenePainter painter(services, scene);
    ActState& act = role.attackAct;
    act.id = role.usedItem;
    act.action = kGlowAction;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);
    services.audio.PlaySound(act.view, services.config.soundVolume);
    role.recoilAct.view = 0;
    role.glowFlags = act.flags;
    role.glowOffsetX = act.x;

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    if (role.facing == 0)
    {
        role.glowFlags ^= Painter::Mirror;
        role.glowOffsetX = role.picture->width - act.x;
    }

    DrawOptions& options = painter.Options();

    auto tint = [&role, &options]
    {
        const int level = role.glowLevel / 2 - kGlowTintBase;
        options.tintRed = level;
        options.tintGreen = level;
        options.tintBlue = level;
    };

    if (role.glowPhase == kGlowRising)
    {
        tint();
        role.glowLevel = static_cast<i16>(role.glowLevel + kGlowStep);

        if (role.glowLevel > kGlowPeak)
        {
            role.glowPhase = kGlowFalling;
        }
    }

    if (role.glowPhase == kGlowFalling)
    {
        tint();
        role.glowLevel = static_cast<i16>(role.glowLevel - kGlowStep);
        ++role.absorbFrames;

        if (role.glowLevel <= 0)
        {
            role.glowPhase = kGlowDone;
        }
    }

    painter.SetSource(role.picture->data.data());
    painter.DrawPicture(
        x - role.glowOffsetX,
        y - act.y,
        role.picture->width,
        role.picture->height,
        role.glowFlags | Painter::Additive
    );

    if (role.glowPhase != kGlowDone)
    {
        return 0;
    }

    services.audio.PlaySound(kGlowEndSound, services.config.soundVolume);
    role.glowLevel = 0;
    role.glowFlags = 0;
    role.glowOffsetX = 0;
    role.glowPhase = kGlowRising;
    role.absorbFrames = 0;
    role.hurtStep = 0;
    act = ActState{};
    role.extraActs.fill(ActState{});
    return 1;
}

// What BattleRole::CatchAct decides of the frame; the draws into `scene`.
int RoleFrame::CatchActAdvance(BattleServices& services, const BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    u16 anchorX = 0;
    u16 anchorY = 0;
    ActState& act = role.attackAct;
    act.id = kThrownAct;
    act.action = kCaughtAction;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
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

    LineWalker& line = role.flight;
    line = LineWalker{};
    line.fromX = fromX;
    line.fromY = fromY;
    line.toX = role.placement.x - static_cast<i16>(role.originX) + static_cast<i16>(role.mainAct.anchorX) - act.x;
    line.toY = role.placement.y + static_cast<i16>(role.mainAct.anchorY) - role.mainAct.y - act.y;
    bool arrived = false;

    for (int taken = 0; taken < kThrowSteps * role.throwSteps; ++taken)
    {
        LineStep(line);

        if (line.fromX + line.x == line.toX)
        {
            arrived = true;
            break;
        }
    }

    ++role.throwSteps;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;

    if (arrived)
    {
        painter.DrawPicture(line.toX, line.toY, role.picture->width, role.picture->height, act.flags);
        role.throwSteps = 0;
        line = LineWalker{};
        act = ActState{};
        return 1;
    }

    painter.DrawPicture(line.fromX + line.x, line.fromY + line.y, role.picture->width, role.picture->height, act.flags);
    return 0;
}

// What BattleRole::AttackPose decides of the frame; the draws into `scene`.
u16 RoleFrame::AttackPoseAdvance(BattleServices& services, Scene& scene)
{
    BattleRole& role = this->role;
    ActState& act = role.standAct;
    act.id = role.actId;
    role.unk2AAC = 1;
    act.action = kAttackAction;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
    role.actOffsetX = static_cast<u16>(act.x);
    role.frameFlags = act.flags;
    role.actAnchorX = act.anchorX;

    if (role.facing == 1)
    {
        role.frameFlags ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(role.picture->width - act.anchorX);
        }
    }

    const int shadowHeight = role.picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.tintRed = kShadowTint;
    options.tintGreen = kShadowTint;
    options.tintBlue = kShadowTint;
    options.shear = role.picture->height >> kShadowShearShift;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, -kTargetPan);
    act.view = 0;
    role.unk26A4 = (act.flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    const int x = role.placement.x - act.x;
    painter.DrawPicture(x, role.placement.y - shadowHeight, role.picture->width, role.picture->height, role.unk26A4);
    painter.DrawPicture(x, role.placement.y - act.y, role.picture->width, role.picture->height, act.flags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) != 0)
    {
        act.target = 0;
        return static_cast<u16>(role.command.castAct | kHitCast);
    }

    if (act.waiting != 1)
    {
        return 0;
    }

    role.unk2AAC = 0;
    act = ActState{};
    return kPoseDone;
}

// What BattleRole::PoseAtTarget decides of the frame; the draws into `scene`.
int RoleFrame::PoseAtTargetAdvance(BattleServices& services, const BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    u16 targetX = 0;
    u16 targetY = 0;
    ActState& act = role.standAct;
    act.action = kTargetPose;
    act.hold = 0;
    act.id = role.actId;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());

    if (act.waiting != 0)
    {
        act = ActState{};
        return 1;
    }

    if (role.facing == 1 && act.unknown1C == 0)
    {
        act.flags ^= Painter::Mirror;
        act.unknown1C = static_cast<i32>(act.flags | kMirroredOnce);
        act.x = role.picture->width - act.x;
    }

    target.GetDrawPosition(targetX, targetY);
    const auto x = static_cast<i16>(targetX - static_cast<u16>(act.x));
    const auto y = static_cast<i16>(targetY - static_cast<u16>(act.y));
    const int shadowHeight = role.picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = role.picture->height >> kShadowShearShift;
    const int tint = role.unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    services.audio.SetSoundPan(act.view, x > kPanCentre ? kTargetPan : -kTargetPan);
    role.unk26A4 = (act.flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter.DrawPicture(
        x - kShadowLeft,
        act.y + y - shadowHeight,
        role.picture->width,
        role.picture->height,
        role.unk26A4
    );
    painter.DrawPicture(x, y, role.picture->width, role.picture->height, act.flags);
    return 0;
}

// What BattleRole::FlyExtraAct decides of the frame; the draws into `scene`.
void RoleFrame::FlyExtraActAdvance(
    BattleServices& services,
    i32 id,
    i32 action,
    int slot,
    i16 x,
    i16 y,
    int steps,
    Scene& scene
)
{
    BattleRole& role = this->role;
    ActState& act = role.extraActs[static_cast<std::size_t>(slot)];
    i32& count = role.extraSteps[static_cast<std::size_t>(slot)];
    act.id = id;
    act.action = action;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
    LineWalker& line = role.flight;
    line = LineWalker{};
    line.fromX = static_cast<i16>(x - static_cast<u16>(act.x));
    line.fromY = static_cast<i16>(y - static_cast<u16>(act.y));
    line.toX = role.placement.x - static_cast<i16>(role.originX) + static_cast<i16>(role.mainAct.anchorX) - act.x;
    line.toY = role.placement.y + static_cast<i16>(role.mainAct.anchorY) - role.mainAct.y - act.y;

    for (int taken = 0; taken < count * steps; ++taken)
    {
        LineStep(line);
    }

    ++count;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    act.view = 0;
    const int endX = line.toX - role.unk2674;
    const int atX = line.fromX + line.x;

    if (atX >= endX)
    {
        painter.DrawPicture(endX, line.toY, role.picture->width, role.picture->height, act.flags);
        count = 0;
        line = LineWalker{};
        return;
    }

    painter.DrawPicture(atX, line.fromY + line.y, role.picture->width, role.picture->height, act.flags);
}

// What BattleRole::DirectAttack decides of the frame; the draws into `scene`.
int RoleFrame::DirectAttackAdvance(BattleServices& services, BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    BattleState& battle = services.state;
    u16 targetX = 0;
    u16 targetY = 0;
    ActState& act = role.standAct;
    act.id = role.actId;
    role.unk2AAC = 1;
    act.action = kForcedAttackAction;
    act.hold = 0;

    if (role.held == 1)
    {
        act.hold = 1;
    }

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    ScenePainter painter(services, scene);
    painter.SetSource(role.picture->data.data());
    role.frameFlags = act.flags;
    role.actOffsetX = static_cast<u16>(act.x);
    role.actAnchorX = act.anchorX;

    if (role.facing == 1)
    {
        role.frameFlags ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(role.picture->width - act.anchorX);
        }
    }

    target.GetDrawPosition(targetX, targetY);
    const auto shadowY = static_cast<i16>(targetY - static_cast<u16>(act.y));
    const int shadowHeight = role.picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = role.picture->height >> kShadowShearShift;
    const int tint = role.unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int x = role.placement.x - static_cast<i16>(role.actOffsetX);
    services.audio.SetSoundPan(act.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
    role.unk26A4 = (role.frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter.DrawPicture(x, act.y + shadowY - shadowHeight, role.picture->width, role.picture->height, role.unk26A4);
    painter.DrawPicture(x, role.placement.y - act.y, role.picture->width, role.picture->height, role.frameFlags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) != 0)
    {
        act.target = 0;
        role.unk267C = BattleUnit::FollowingUp;
        target.RaiseFlag26B8();
        i16 spirit = 0;
        i16 stamina = 0;
        i32 damage = role.HitDamage(services, target, spirit, stamina);
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

    if (role.unk267C != BattleUnit::FollowingUp)
    {
        return 0;
    }

    ActState& follow = role.slotActs[0];
    follow.id = role.command.castAct;
    follow.action = 0;
    role.AimAction(services, target, follow, 0, 0);

    if (role.picture != nullptr)
    {
        painter.SetSource(role.picture->data.data());
        painter.DrawPicture(
            static_cast<i16>(role.aimX),
            static_cast<i16>(role.aimY),
            role.picture->width,
            role.picture->height,
            role.unk26A4
        );
    }

    if (act.waiting != 1)
    {
        return 0;
    }

    role.unk267C = 0;
    follow = ActState{};
    act = ActState{};
    return 1;
}

// What BattleRole::ChargeOnTarget decides of the frame; the draws into `scene`.
int RoleFrame::ChargeOnTargetAdvance(BattleServices& services, BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    ScenePainter painter(services, scene);
    ActState& charge = role.attackAct;
    charge.id = role.actId + BattleRole::MagicActs;
    charge.action = role.unk2A8A + kChargeAction;

    if (services.acts.Run(charge) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(charge.frame, charge.picture);
    services.audio.PlaySound(charge.view, services.config.soundVolume);
    charge.view = 0;
    role.unk26A4 = charge.flags;
    role.actOffsetX = static_cast<u16>(charge.x);
    const u16 chargeAnchorX = charge.anchorX;

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    if (role.facing == 1)
    {
        role.unk26A4 ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(charge.x));
    }

    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    const auto dashX = static_cast<i16>(targetX - role.actOffsetX);
    const auto dashY = static_cast<i16>(targetY - static_cast<u16>(charge.y));
    AfterimageTrail& trail = role.trails[1];

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
    UnitFrame(role).Trail(services, trail, charge.frame, charge.picture, role.unk26A4, dashX, dashY, scene);
    role.picture = services.archive.GetPicture(charge.frame, charge.picture);

    if (role.picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(role.picture->data.data());
    painter.DrawPicture(dashX, dashY, role.picture->width, role.picture->height, role.unk26A4);

    if ((charge.target & ActState::FollowsUp) != 0)
    {
        role.unk267C |= BattleUnit::ChargeFollowing;
        charge.target = 0;
        role.slotActs[1] = ActState{};
    }

    if ((role.unk267C & BattleUnit::ChargeFollowing) == 0)
    {
        return 1;
    }

    ActState& follow = role.slotActs[1];
    follow.action = 0;
    follow.id = role.command.castAct;

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

    role.picture = services.archive.GetPicture(follow.frame, follow.picture);

    if (role.picture == nullptr)
    {
        return 0;
    }

    u16 followAnchorX = follow.anchorX;
    role.unk26A4 = follow.flags;
    role.aimOffsetX = static_cast<u16>(follow.x);

    if (role.facing == 1)
    {
        role.unk26A4 ^= Painter::Mirror;
        role.aimOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(follow.x));
    }

    services.audio.PlaySound(follow.view, services.config.soundVolume);
    services.audio.SetSoundPan(follow.view, role.facing == 1 ? kTargetPan : -kTargetPan);
    follow.view = 0;
    painter.SetSource(role.picture->data.data());
    role.unk26A4 ^= Painter::Mirror;
    role.aimOffsetX = static_cast<u16>(role.picture->width - role.aimOffsetX);

    if (followAnchorX != 0)
    {
        followAnchorX = static_cast<u16>(role.picture->width - follow.anchorX);
    }

    i16 x = 0;
    i16 y = 0;

    if ((follow.anchorX != 0 || follow.anchorY != 0) && (charge.anchorX != 0 || charge.anchorY != 0))
    {
        x = static_cast<i16>(dashX - followAnchorX + chargeAnchorX);
        y = static_cast<i16>(charge.anchorY - follow.anchorY + dashY);
    }

    painter.DrawPicture(x, y, role.picture->width, role.picture->height, role.unk26A4);

    if ((follow.target & ActState::Strikes) != 0)
    {
        role.slotTint = 0;
        follow.target = 0;
        role.Strike(services, target, 0);
    }

    return 1;
}

// What BattleRole::MagicFrame decides of the frame; the draws into `scene`.
int RoleFrame::MagicFrameAdvance(BattleServices& services, const BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    BattleState& battle = services.state;
    ActState& act = role.slotActs[BattleUnit::UnkSlot8];
    act.action = role.unk2A8A;
    role.unk2AAC = 1;
    act.id = role.actId + BattleRole::MagicActs;
    UnitFrame(role).Action(services, target, act, scene);

    if ((act.target & ActState::Recoils) != 0)
    {
        if (act.effectA != 0)
        {
            role.unk267C |= BattleUnit::Recoiling;
            role.recoilAct.id = act.effectA;
            role.recoilAct.action = act.effectQ;
        }

        if ((act.target & ActState::Holds) != 0)
        {
            act.hold = 1;
        }

        act.effectA = 0;
        act.target = static_cast<u16>(act.target & ~ActState::Recoils);
        act.effectQ = 0;
    }

    if ((role.unk267C & BattleUnit::Recoiling) != 0 &&
        UnitFrame(role).Recoil(services, role.recoilAct, static_cast<i16>(act.anchorY), scene) == 1)
    {
        role.unk267C &= ~BattleUnit::Recoiling;
        act.target = 0;
        act.hold = 0;
        role.recoilAct = ActState{};
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

    role.unk267C = 0;
    act = ActState{};
    return 1;
}

// What BattleRole::ChargeAttackFrame decides of the frame; the draws into `scene`.
int RoleFrame::ChargeAttackFrameAdvance(BattleServices& services, BattleUnit& target, int slot, Scene& scene)
{
    BattleRole& role = this->role;
    BattleState& battle = services.state;
    ScenePainter painter(services, scene);

    if (role.unk2A74 != 0 || role.unk2AD8 == 1)
    {
        return 0;
    }

    role.unk2AAC = 1;
    ActState& magic = role.slotActs[BattleUnit::UnkSlot8 + static_cast<std::size_t>(slot)];
    ActState& charge = role.attackAct;
    magic.id = role.actId + BattleRole::MagicActs;
    magic.action = role.unk2A8A;
    UnitFrame(role).Action(services, target, magic, scene);

    auto slide = [&role, &painter]
    {
        if (role.picture == nullptr)
        {
            return;
        }

        const int x = static_cast<i16>(role.actX) - role.fadeLevel;
        const int y = static_cast<i16>(role.actY);
        painter.Options().alpha = kChargeAlpha;
        painter.DrawPicture(x, y, role.picture->width, role.picture->height, role.frameFlags | Painter::Translucent);
        painter.Options().tintBlue = kChargeBlue;
        painter.DrawPicture(x, y, role.picture->width, role.picture->height, role.frameFlags | Painter::TintUnder);
    };

    auto endDash = [&role, &magic, &charge]
    {
        magic.target = 0;
        role.glowLevel = 0;
        role.unk2678 = 0;
        role.unk2A8E = kNoUnk2A8E;
        magic.hold = 0;
        charge.hold = 0;
    };

    if ((magic.target & ActState::FollowsUp) != 0 && (magic.target & ActState::Dashes) != 0)
    {
        AfterimageTrail& dashTrail = role.trails[0];

        if (dashTrail.started == 0)
        {
            dashTrail = AfterimageTrail{};
            dashTrail.started = 1;

            for (AfterimageTrail::Image& image : dashTrail.images)
            {
                image.picture = AfterimageTrail::NoPicture;
            }

            dashTrail.tintBlue = kTrailBlueTint;
            role.fadeLevel = 0;
        }

        magic.hold = 1;
        charge.id = role.actId + BattleRole::MagicActs;
        charge.action = role.unk2A8A + kChargeAction;

        while (charge.picture < kChargePicture)
        {
            if (services.acts.Run(charge) == 0)
            {
                return 0;
            }
        }

        slide();
        role.fadeLevel += role.facing == 0 ? kDashStep : -kDashStep;
        charge.hold = 1;

        if ((charge.target & ActState::Strikes) != 0)
        {
            target.RaiseFlag26B8();
            charge.target = 0;
        }

        if (role.fadeLevel % kDashLength != 0)
        {
            return 0;
        }

        endDash();
        role.charging = 1;
    }

    if (role.charging == 1)
    {
        if (charge.waiting != 0)
        {
            slide();
            role.fadeLevel += role.facing == 0 ? -kDashStep : kDashStep;
            charge.hold = 1;

            if (role.fadeLevel == 0)
            {
                endDash();
                role.fadeLevel = 0;
                role.charging = 0;
            }
        }
        else if (this->ChargeOnTargetAdvance(services, target, scene) == 0)
        {
            return 0;
        }
    }

    if ((magic.target & ActState::FollowsUp) != 0)
    {
        role.unk267C |= BattleUnit::FollowingUp;
        magic.target = 0;
    }

    if ((magic.target & ActState::Strikes) != 0)
    {
        role.Strike(services, target, 0);
        magic.target = 0;
        role.unk267C |= BattleUnit::FollowingUp;
        role.slotTint = 0;
        battle.proficiencyBonus = 0;
        role.slotActs[0] = ActState{};
    }

    if ((role.unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    ActState& follow = role.slotActs[0];
    follow.action = 0;
    follow.id = role.command.castAct;
    role.AimAction(services, target, follow, static_cast<i16>(magic.anchorX), static_cast<i16>(magic.anchorY));

    if ((follow.target & ActState::Strikes) != 0)
    {
        role.slotTint = 0;
        follow.target = 0;
        role.Strike(services, target, 0);
    }

    if (role.picture != nullptr)
    {
        painter.SetSource(role.picture->data.data());
    }

    if (follow.waiting == 1)
    {
        magic.effectA = 0;
        charge.effectA = 0;

        if (magic.waiting != 1 || role.charging != 0)
        {
            return 0;
        }

        magic = ActState{};
        charge = ActState{};
        role.recoilAct = ActState{};
        follow = ActState{};
        role.slotActs[1] = ActState{};
        role.trails = {};
        role.targets.fill(BattleUnit::NoTarget);
        role.slotTint = 0;
        role.unk2AB0 = 0;
        role.unk267C = 0;
        role.unk26D4 = 0;
        battle.criticalAttack = 0;
        ++battle.attacksSinceCritical;
        role.charging = 0;
        return 1;
    }

    if ((role.unk267C & BattleUnit::Recoiling) == 0 && role.picture != nullptr)
    {
        const DrawArgument argument =
            role.shadowPicture != nullptr ? PaletteArgument(*role.shadowPicture) : DrawArgument{};
        painter.DrawPicture(
            static_cast<i16>(role.aimX),
            static_cast<i16>(role.aimY),
            role.picture->width,
            role.picture->height,
            role.unk26A4,
            argument
        );
    }

    return 0;
}

// What BattleRole::BoomerangFrame decides of the frame; the draws into `scene`.
int RoleFrame::BoomerangFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    ScenePainter painter(services, scene);
    ActState& act = role.standAct;
    act.id = role.actId;
    role.unk2AAC = 1;
    act.action = kForcedAttackAction;

    if (services.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(role.picture->data.data());
    role.frameFlags = act.flags;
    role.actOffsetX = static_cast<u16>(act.x);
    role.actAnchorX = act.anchorX;

    if (role.facing == 1)
    {
        role.frameFlags ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(role.picture->width - act.anchorX);
        }
    }

    const int shadowHeight = role.picture->height / kShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = role.picture->height >> kShadowShearShift;
    const int tint = role.unk2B00 == 1 ? kFaintShadowTint : kShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    services.audio.PlaySound(act.view, services.config.soundVolume);
    const int x = role.placement.x - static_cast<i16>(role.actOffsetX);
    services.audio.SetSoundPan(act.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
    role.unk26A4 = (role.frameFlags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    act.view = 0;
    painter.DrawPicture(x, role.placement.y - shadowHeight, role.picture->width, role.picture->height, role.unk26A4);
    painter.DrawPicture(x, role.placement.y - act.y, role.picture->width, role.picture->height, role.frameFlags);

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) == 0)
    {
        return 0;
    }

    ActState& thrown = role.slotActs[0];
    thrown.action = 0;
    thrown.id = kBoomerangAct;
    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);

    if (role.unk267C == kBoomerangGoing)
    {
        if (role.unk2F0C == 0)
        {
            role.AddQueuedAction(
                thrown.id,
                0,
                role.placement.x,
                role.placement.y,
                static_cast<i16>(targetX),
                static_cast<i16>(targetY),
                kBoomerangOut,
                0,
                kNoImpact
            );
            role.TrailLastQueuedAction(0, 0, kTrailBlueTint);
            role.unk2F0C = 1;
        }

        if (UnitFrame(role).Queued(services, target, 0, scene) == 1)
        {
            role.unk2F0C = 0;
            role.unk267C = kBoomerangReturning;
            target.RaiseFlag26B8();
        }
    }

    if (role.unk267C == kBoomerangReturning)
    {
        if (role.unk2F0C == 0)
        {
            role.AddQueuedAction(
                thrown.id,
                0,
                static_cast<i16>(targetX),
                static_cast<i16>(targetY),
                role.placement.x,
                role.placement.y,
                kBoomerangBack,
                kBoomerangShake,
                kNoImpact
            );
            role.TrailLastQueuedAction(0, 0, kTrailBlueTint);
            role.unk2F0C = 1;
        }

        if (UnitFrame(role).Queued(services, target, 0, scene) == 1)
        {
            role.unk2F0C = 0;
            role.unk267C = kBoomerangBackHome;
            services.audio.PlaySound(kBoomerangSound, services.config.soundVolume);
        }
    }

    if (role.unk267C != kBoomerangBackHome)
    {
        return 0;
    }

    role.unk267C = 0;
    thrown = ActState{};
    act = ActState{};
    return 1;
}

// What BattleRole::SwarmSpellFrame decides of the frame; the draws into `scene`.
int RoleFrame::SwarmSpellFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    ActState& magic = role.slotActs[BattleUnit::UnkSlot8];
    u16 targetX = 0;
    u16 targetY = 0;
    magic.action = role.unk2A8A;
    role.unk2AAC = 1;
    magic.id = role.actId + BattleRole::MagicActs;
    UnitFrame(role).Action(services, target, magic, scene);

    if ((magic.target & ActState::Recoils) != 0)
    {
        if (magic.effectA != 0)
        {
            role.unk267C |= BattleUnit::Recoiling;
            role.recoilAct.id = magic.effectA;
            role.recoilAct.action = magic.effectQ;
        }

        if ((magic.target & ActState::Holds) != 0)
        {
            magic.hold = 1;
        }

        magic.effectA = 0;
        magic.target = static_cast<u16>(magic.target & ~ActState::Recoils);
        magic.effectQ = 0;
    }

    if ((role.unk267C & BattleUnit::Recoiling) != 0 &&
        UnitFrame(role).Recoil(services, role.recoilAct, static_cast<i16>(magic.anchorY), scene) == 1)
    {
        magic.target = 0;
        role.unk267C &= ~BattleUnit::Recoiling;
        magic.hold = 0;
    }

    auto aim = [&role, &target, &targetX, &targetY]
    {
        target.GetDrawPosition(targetX, targetY);

        if ((role.command.actionFlags & UnitCommand::UnkAction2) != 0)
        {
            targetX = 0;
            targetY = 0;
        }

        role.ClearMoveAxes(targetX, targetY);
    };

    if ((magic.target & (ActState::Strikes | ActState::FollowsUp)) != 0)
    {
        role.unk267C |= BattleUnit::FollowingUp;
        magic.target = 0;
        aim();
        role.unk2F0C = role.unk2F24;
        ++role.unk2F24;
    }

    if ((role.unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    ++role.glowLevel;

    if (role.glowLevel % kSwarmInterval == 1 && role.queuedActions < kSwarmActs)
    {
        aim();
        const std::size_t place = role.queuedActions & (kSwarmActs - 1);
        role.AddQueuedAction(
            role.command.castAct,
            role.unk2F0C,
            static_cast<i16>(role.placement.x + BattleRole::SwarmOffsetX[place]),
            static_cast<i16>(role.placement.y + BattleRole::SwarmOffsetY[place]),
            static_cast<i16>(targetX),
            static_cast<i16>(targetY - kSwarmAbove),
            1,
            kSwarmShake,
            0
        );
        role.TrailLastQueuedAction(0, 0, kTrailBlueTint);
        services.audio.PlaySound(kSwarmSound, services.config.soundVolume);
    }

    if (UnitFrame(role).Queued(services, target, role.unk2F0C, scene) != 1)
    {
        return 0;
    }

    role.unk267C = 0;

    if (magic.waiting != 1)
    {
        return 0;
    }

    role.glowLevel = 0;
    role.unk2F0C = 0;
    role.unk2F24 = 0;
    role.slotActs[0] = ActState{};
    magic = ActState{};
    return 1;
}

// What BattleRole::BeamSpellFrame decides of the frame; the draws into `scene`.
int RoleFrame::BeamSpellFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    BattleState& battle = services.state;
    ScenePainter painter(services, scene);
    ActState& magic = role.slotActs[BattleUnit::UnkSlot8];
    ActState& rise = role.slotActs[BattleUnit::UnkSlot9];
    role.unk2AAC = 1;
    magic.id = role.actId + BattleRole::MagicActs;
    magic.action = role.unk2A8A;

    if (services.acts.Run(magic) == 0)
    {
        return 0;
    }

    role.picture = services.archive.GetPicture(magic.frame, magic.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    painter.SetSource(role.picture->data.data());
    auto offsetX = static_cast<i16>(magic.x);
    u32 flags = magic.flags;
    role.actAnchorX = magic.anchorX;

    if (role.facing == 1)
    {
        flags ^= Painter::Mirror;
        offsetX = static_cast<i16>(role.picture->width - static_cast<u16>(magic.x));

        if (magic.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(role.picture->width - magic.anchorX);
        }
    }

    const int x = role.placement.x - offsetX;
    const int magicY = role.placement.y - magic.y;

    auto tint = [&painter](int level)
    {
        DrawOptions& options = painter.Options();
        options.tintRed = level;
        options.tintGreen = level;
        options.tintBlue = level;
    };

    if ((magic.target & ActState::FollowsUp) != 0)
    {
        role.slotTint = kBeamRiseStart;
        role.unk267C |= kBeamGlow | kBeamRise;
        magic.target = 0;
        role.glowLevel = 0;
    }

    if (role.unk267C == 0)
    {
        services.audio.PlaySound(magic.view, services.config.soundVolume);
        services.audio.SetSoundPan(magic.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
        magic.view = 0;
        const int shadowHeight = role.picture->height / kShadowShare;
        DrawOptions& options = painter.Options();
        options.scaleHeight = shadowHeight;
        options.tintRed = kShadowTint;
        options.tintGreen = kShadowTint;
        options.tintBlue = kShadowTint;
        options.shear = role.picture->height >> kShadowShearShift;
        role.unk26A4 = (flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
        painter.DrawPicture(
            x - kShadowLeft,
            role.placement.y - shadowHeight,
            role.picture->width,
            role.picture->height,
            role.unk26A4
        );
        painter.DrawPicture(x, magicY, role.picture->width, role.picture->height, flags);
    }

    if ((role.unk267C & kBeamGlow) != 0)
    {
        if (role.glowLevel > kBeamGlowEnd)
        {
            tint(role.glowLevel);
            painter.DrawPicture(x, magicY, role.picture->width, role.picture->height, flags | Painter::Additive);
            --role.glowLevel;
        }
        else
        {
            role.unk267C &= ~kBeamGlow;
        }
    }

    if ((role.unk267C & kBeamRise) != 0)
    {
        rise.action = 0;
        rise.id = kRiseAct;

        if (services.acts.Run(rise) == 0)
        {
            return 0;
        }

        role.picture = services.archive.GetPicture(rise.frame, rise.picture);

        if (role.picture == nullptr)
        {
            return 0;
        }

        painter.SetSource(role.picture->data.data());
        flags = rise.flags;
        role.aimOffsetX = static_cast<u16>(rise.x);
        role.aimAnchorX = rise.anchorX;

        if (role.facing == 1)
        {
            flags ^= Painter::Mirror;
            role.aimOffsetX = static_cast<u16>(role.picture->width - static_cast<u16>(rise.x));

            if (rise.anchorX != 0)
            {
                role.aimAnchorX = static_cast<u16>(role.picture->width - rise.anchorX);
            }
        }

        services.audio.PlaySound(rise.view, services.config.soundVolume);
        services.audio.SetSoundPan(rise.view, x >= kPanCentre ? kTargetPan : -kTargetPan);
        rise.view = 0;

        if (role.slotTint > 0)
        {
            role.unk267C = kBeamFly;
            role.glowLevel = 0;
            role.slotTint = 0;
        }
        else
        {
            tint(role.slotTint);
            const int riseX =
                static_cast<i16>(role.actAnchorX) - static_cast<i16>(role.aimAnchorX) - offsetX + role.placement.x;
            const int riseY =
                role.placement.y - static_cast<i16>(rise.anchorY) + static_cast<i16>(magic.anchorY) - magic.y;
            painter.DrawPicture(riseX, riseY, role.picture->width, role.picture->height, flags | Painter::Additive);
            ++role.slotTint;
        }
    }

    if (role.unk267C == kBeamFly)
    {
        const int fromX = role.placement.x - offsetX + static_cast<i16>(role.actAnchorX);
        const int fromY = role.placement.y + static_cast<i16>(magic.anchorY) - magic.y;

        if (UnitFrame(role)
                .Fly(services, target, role.slotActs[0], 0, kRiseAct, fromX, fromY, kSpeedingFlight, 0, scene) == 1)
        {
            target.RaiseFlag26B8();
            i16 spirit = 0;
            i16 stamina = 0;
            i32 damage = role.HitDamage(services, target, spirit, stamina);
            battle.lastDamage = damage;

            if (damage >= kMaxDamage)
            {
                damage = kMaxDamage;
                battle.lastDamage = damage;
            }

            battle.hitDamage += damage;
            target.SetDamageDigits(services, damage);
            target.StepTargetCount(1);
            role.unk267C = kBeamImpact;
            role.glowLevel = kBeamRiseStart;
        }
    }

    if ((role.unk267C & kBeamImpact) != 0)
    {
        ActState& impact = role.slotActs[1];
        impact.id = kImpactAct;
        impact.action = 0;

        if (UnitFrame(role).DrawAimedAdvance(services, target, impact, 0, 0, scene) == 1)
        {
            role.unk267C = kBeamFadeIn;
        }
    }

    if ((role.unk267C & kBeamFadeIn) == 0)
    {
        return 0;
    }

    if (role.glowLevel <= 0)
    {
        tint(role.glowLevel);

        if (role.picture != nullptr)
        {
            painter.DrawPicture(x, magicY, role.picture->width, role.picture->height, flags | Painter::Additive);
        }

        role.glowLevel = static_cast<i16>(role.glowLevel + kBeamFadeStep);
        return 0;
    }

    role.glowLevel = 0;
    role.unk267C = 0;
    magic = ActState{};
    rise = ActState{};
    role.slotActs[0] = ActState{};
    role.slotActs[1] = ActState{};
    return 1;
}

// What BattleRole::LeapSpellFrame decides of the frame; the draws into `scene`.
int RoleFrame::LeapSpellFrameAdvance(BattleServices& services, BattleUnit& target, Scene& scene)
{
    BattleRole& role = this->role;
    BattleState& battle = services.state;
    ActState& magic = role.slotActs[BattleUnit::UnkSlot8];
    magic.id = role.actId + BattleRole::MagicActs;
    role.unk2AAC = 1;
    magic.action = role.unk2A8A;

    if (role.unk267C == kLeapCasting)
    {
        UnitFrame(role).Action(services, target, magic, scene);
    }

    if (role.unk267C == kLeapAtTarget)
    {
        u16 targetX = 0;
        u16 targetY = 0;
        ++magic.action;
        target.GetDrawPosition(targetX, targetY);
        UnitFrame(role)
            .DrawActionAtAdvance(services, static_cast<i16>(targetX), static_cast<i16>(targetY), magic, scene);
    }

    if (role.unk267C == kLeapReturning)
    {
        role.command.flags = static_cast<u16>(role.command.flags | UnitCommand::UnkFlag8);
        magic.action += kLeapReturnAction;

        if (UnitFrame(role).Home(services, target, magic.id, magic.action, scene) == 1)
        {
            role.unk267C = kLeapHome;
        }
    }

    if (magic.waiting == 1)
    {
        magic = ActState{};
        ++role.unk267C;
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
        static_cast<i32>(role.unk267C) < static_cast<i32>(kLeapHome))
    {
        return 0;
    }

    role.unk267C = 0;
    magic = ActState{};
    return 1;
}

}  // namespace swd3::smooth
