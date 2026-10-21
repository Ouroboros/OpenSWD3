#include "battle/smooth/smooth_hit.h"

#include <algorithm>

#include "battle/battle_hit_constants.h"
#include "battle/smooth/smooth_unit.h"
#include "core/fault.h"
#include "game/game.h"
#include "platform/display.h"

namespace swd3::smooth
{

namespace
{

constexpr u16 kAllHitNudge = 0x1000;
constexpr int kHitPan = 0x10;
constexpr int kHitPanCentre = 0x140;  // the sound pans right from the middle of the field
constexpr int kRippleBackX = 0x14;
constexpr int kRippleShiftX = 0x50;
constexpr int kRippleShiftY = 0x78;
constexpr int kRippleSplashDepth = 0xC8;
constexpr int kRippleSplashRadius = 0xC;
constexpr int kRippleSplashX = 0x64;
constexpr int kRippleSplashY = 0x32;
constexpr int kRippleStartShiftY = 0x96;
constexpr i32 kRippleWalked = 0x3E8;  // HitTargetFrame: the ripple frames once the walk reached the target
constexpr i32 kSplashEvery = 9;
constexpr i32 kSplashFrames = 0x64;
constexpr int kSplashFromX = 0x3C;
constexpr i32 kSplashLast = 0x1E;
constexpr int kSplashShiftX = 0x64;
constexpr u32 kSplashSpreadX = 80;
constexpr u32 kSplashSpreadY = 100;
// 烈火術's hit act: its script strikes on its first step, so the damage shows before the fire is drawn.
constexpr i32 kFirstStepStrikeAct = 6001;
}  // namespace

HitFrame::HitFrame(Game& game)
    : game(game)
{
}

// What Game::PlayHitAct decides of the frame; the draws into `scene`.
int HitFrame::PlayHitActAdvance(BattleUnit& unit, u32 act, int index, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    ScenePainter painter(game.battleServices, scene);
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.hitActs[at];

    if ((hit.target & ActState::Quakes) != 0)
    {
        battle.quakePhase = 0;
        battle.quaking = 1;
    }

    if (hit.waiting == 0)
    {
        hit.id = act;
        hit.action = 0;
        hit.hold = 0;

        if (battle.paused == 1)
        {
            hit.hold = 1;
        }

        if (game.acts.Run(hit) == 0)
        {
            battle.followActs[at] = ActState{};
            battle.unk4FF0BC[at] = 0;
            return 1;
        }

        const Picture* picture = game.archive.LoadPictureKept(hit.frame, hit.picture);
        u32 flags = hit.flags;
        int originX = hit.x;

        // A picture that does not load is read through a null pointer in the original.
        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());

            if (battle.mirrored == 1)
            {
                flags ^= Painter::Mirror;
                originX = picture->width - originX;
            }
        }

        u16 offsetX = 0;
        u16 offsetY = 0;
        u16 x = 0;
        u16 y = 0;
        unit.GetOffset(offsetX, offsetY);

        if (offsetX != 0 && offsetY != 0)
        {
            unit.GetOrigin(x, y);
            x = static_cast<u16>(x + offsetX);
            y = static_cast<u16>(y + offsetY);
        }
        else
        {
            unit.GetDrawPosition(x, y);
        }

        y = static_cast<u16>(y - static_cast<u16>(hit.y));
        x = static_cast<u16>(x - originX);
        game.audio.PlaySound(hit.view, game.config.soundVolume);
        game.audio.SetSoundPan(hit.view, originX + static_cast<i16>(x) >= kHitPanCentre ? kHitPan : -kHitPan);
        hit.view = 0;

        if (picture != nullptr)
        {
            painter.DrawPicture(
                static_cast<i16>(x),
                static_cast<i16>(y),
                picture->width,
                picture->height,
                flags,
                PaletteArgument(*picture)
            );
        }
    }

    if (hit.waiting != 1)
    {
        return 0;
    }

    hit = ActState{};
    return 1;
}

// What Game::FadeHitAct decides of the frame; the draws into `scene`.
int HitFrame::FadeHitActAdvance(BattleUnit& unit, u32 act, i32 action, int index, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    ScenePainter painter(game.battleServices, scene);
    const auto at = static_cast<std::size_t>(index);

    if (act == 0)
    {
        return 1;
    }

    i8& fade = battle.fadeActLevels[at];

    if (fade <= kHitFadeEnd)
    {
        fade = 0;
        return 1;
    }

    u16 x = 0;
    u16 y = 0;
    unit.GetDrawPosition(x, y);
    ActState& effect = battle.fadeActs[at];
    effect.id = act;
    effect.action = action;
    effect.hold = 0;

    if (battle.paused == 1)
    {
        effect.hold = 1;
    }

    if (game.acts.Run(effect) == 0)
    {
        return 0;
    }

    const Picture* picture = game.archive.GetPicture(effect.frame, effect.picture);

    // A missing picture is read through a null pointer in the original.
    if (picture != nullptr)
    {
        painter.SetSource(picture->data.data());
        DrawOptions& options = painter.Options();
        options.tintRed = fade;
        options.tintGreen = fade;
        options.tintBlue = fade;
        painter.DrawPicture(
            static_cast<i16>(x) - effect.x,
            static_cast<i16>(y) - effect.y,
            picture->width,
            picture->height,
            effect.flags,
            PaletteArgument(*picture)
        );
    }

    fade = static_cast<i8>(fade - kHitFadeStep);
    return 0;
}

// What Game::DrawHitPicture decides of the frame; the draws into `scene`.
bool HitFrame::DrawHitPictureAdvance(
    BattleUnit& target,
    BattleUnit& attacker,
    u32 act,
    int index,
    u16& x,
    u16& y,
    int& originX,
    Scene& scene
)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    ScenePainter painter(game.battleServices, scene);
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.hitActs[at];
    hit.id = static_cast<i32>(act);
    hit.action = 0;
    hit.hold = 0;

    if (battle.paused == 1)
    {
        hit.hold = 1;
    }

    if (game.acts.Run(hit) == 0)
    {
        battle.followActs[at] = ActState{};
        battle.unk4FF0BC[at] = 0;
        return false;
    }

    // The rebuild moves the strike of kFirstStepStrikeAct (its only one) to the step that ends the act.
    if (hit.id == kFirstStepStrikeAct)
    {
        hit.target = hit.waiting == 1 ? static_cast<u16>(hit.target | ActState::Strikes)
                                      : static_cast<u16>(hit.target & ~ActState::Strikes);
    }

    const Picture* picture = game.archive.LoadPictureKept(hit.frame, hit.picture);
    const int width = picture != nullptr ? picture->width : 0;
    u32 flags = hit.flags;
    u16 anchorX = hit.anchorX;
    originX = hit.x;

    // A picture that does not load is read through a null pointer in the original.
    if (picture != nullptr)
    {
        painter.SetSource(picture->data.data());
    }

    if (attacker.kind == 0)
    {
        flags ^= Painter::Mirror;
        originX = width - hit.x;

        if (hit.anchorX != 0)
        {
            anchorX = static_cast<u16>(width - hit.anchorX);
        }
    }

    if (battle.mirrored == 1)
    {
        flags ^= Painter::Mirror;
        originX = width - hit.x;

        if (hit.anchorX != 0)
        {
            anchorX = hit.anchorX;
        }
    }

    u16 offsetX = 0;
    u16 offsetY = 0;
    attacker.GetOffset(offsetX, offsetY);

    if (offsetX != 0 || offsetY != 0)
    {
        attacker.GetOrigin(x, y);
        x = static_cast<u16>(x + offsetX);
        y = static_cast<u16>(y + offsetY);
    }

    if (anchorX != 0 || hit.anchorY != 0)
    {
        if (x != 0 || y != 0)
        {
            y = static_cast<u16>(y - hit.anchorY);
            x = static_cast<u16>(x - anchorX);
        }
    }
    else
    {
        target.GetDrawPosition(x, y);
        x = static_cast<u16>(x - originX);
        y = static_cast<u16>(y - static_cast<u16>(hit.y));
    }

    game.audio.PlaySound(hit.view, game.config.soundVolume);
    game.audio.SetSoundPan(hit.view, originX + static_cast<i16>(x) >= kHitPanCentre ? kHitPan : -kHitPan);
    hit.view = 0;
    attacker.ClearMoveAxes(x, y);
    battle.hitPictureX = static_cast<i16>(x);
    battle.hitPictureY = static_cast<i16>(y);
    u16 speed = 0;

    if (attacker.CommandTarget(speed) == 1)
    {
        u16 fromX = 0;
        u16 fromY = 0;
        attacker.GetDrawPosition(fromX, fromY);
        fromX = static_cast<u16>(fromX - originX);
        fromY = static_cast<u16>(fromY - static_cast<u16>(hit.y));

        if (x == 0)
        {
            x = static_cast<u16>(-originX);
        }

        if (game.WalkLine(
                battle.hitPictureX,
                battle.hitPictureY,
                static_cast<i16>(fromX),
                static_cast<i16>(fromY),
                static_cast<i16>(x),
                static_cast<i16>(y),
                speed,
                index
            ) == 1)
        {
            hit.target |= ActState::Strikes;
            hit.waiting = 1;
        }
    }

    if (picture != nullptr)
    {
        painter.DrawPicture(
            battle.hitPictureX,
            battle.hitPictureY,
            picture->width,
            picture->height,
            flags,
            PaletteArgument(*picture)
        );
    }

    return true;
}

// What Game::RippleHitFrame decides of the frame; the draws into `scene`.
void HitFrame::RippleHitFrameAdvance(BattleUnit& target, BattleUnit& attacker, int index, u16& targetX, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.hitActs[at];
    i32& frames = battle.rippleFrames[at];
    u16 speed = 0;
    u16 targetY = 0;
    ScenePainter painter(game.battleServices, scene);

    // Without a ripple the original calls through a null pointer: the ripple is skipped. The copy of the screen the
    // original takes before the ripple moves is taken where it refracts: nothing is drawn between.
    ScriptObject* ripple = game.battleRipple.get();

    auto refract = [ripple, &painter](int x, int y)
    {
        if (ripple != nullptr)
        {
            ripple->MoveTo(x, y);
            painter.Refract(*ripple);
            ripple->Step();
        }
    };

    if (attacker.CommandTarget(speed) == 1)
    {
        u16 fromX = 0;
        u16 fromY = 0;
        target.GetDrawPosition(targetX, targetY);
        attacker.GetDrawPosition(fromX, fromY);

        if (frames < kRippleWalked && game.WalkLine(
                                          battle.hitPictureX,
                                          battle.hitPictureY,
                                          static_cast<i16>(fromX),
                                          static_cast<i16>(fromY),
                                          static_cast<i16>(targetX),
                                          static_cast<i16>(targetY),
                                          speed,
                                          index
                                      ) == 1)
        {
            game.audio.PlaySound(kRippleSound, game.config.soundVolume);
            hit.target |= ActState::Strikes;
            frames = kRippleWalked;
        }

        if (ripple != nullptr && frames == 0)
        {
            ripple->MoveTo(battle.hitPictureX - kRippleShiftX, battle.hitPictureY - kRippleStartShiftY);
            ripple->Splash(kRippleSplashX, kRippleSplashY, kRippleSplashRadius, kRippleSplashDepth, game.crtRandom);
        }

        if (++frames >= kRippleWalked)
        {
            battle.hitPictureX = static_cast<i16>(targetX) - kRippleBackX;
            battle.hitPictureY = static_cast<i16>(targetY);
        }

        refract(battle.hitPictureX - kRippleShiftX, battle.hitPictureY - kRippleShiftY);

        if (frames >= kRippleWalked + kRippleHold)
        {
            hit.waiting = 1;
            frames = 0;
        }

        return;
    }

    target.GetDrawPosition(targetX, targetY);
    const int x = static_cast<i16>(targetX) - kSplashShiftX;
    const int y = static_cast<i16>(targetY) - kRippleStartShiftY;

    if (frames % kSplashEvery == 0 && frames <= kSplashLast)
    {
        if (ripple != nullptr)
        {
            ripple->MoveTo(x, y);
        }

        if (frames == 0)
        {
            game.audio.PlaySound(kRippleSound, game.config.soundVolume);
        }

        const int splashY = static_cast<int>(game.random.Below(kSplashSpreadY)) + kRippleSplashY;
        const int splashX = static_cast<int>(game.random.Below(kSplashSpreadX)) + kSplashFromX;

        if (ripple != nullptr)
        {
            ripple->Splash(splashX, splashY, kRippleSplashRadius, kRippleSplashDepth, game.crtRandom);
        }
    }

    ++frames;
    refract(x, y);

    if (frames >= kSplashFrames)
    {
        frames = 0;
        hit.target |= ActState::Strikes;
        hit.waiting = 1;
    }
}

// What Game::HitTargetFrame decides of the frame; the draws into `scene`.
int HitFrame::HitTargetFrameAdvance(BattleUnit& target, BattleUnit& attacker, u32 act, int index, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    ScenePainter painter(game.battleServices, scene);
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.hitActs[at];
    ActState& follow = battle.followActs[at];
    u16 targetX = 0;
    u16 targetY = 0;
    int originX = 0;

    if (hit.waiting == 0)
    {
        if (battle.rippleHits == 1)
        {
            this->RippleHitFrameAdvance(target, attacker, index, targetX, scene);
        }
        else if (!this->DrawHitPictureAdvance(target, attacker, act, index, targetX, targetY, originX, scene))
        {
            return 1;
        }

        battle.darkRed = static_cast<i16>(hit.red);
        battle.darkGreen = static_cast<i16>(hit.green);
        battle.darkBlue = static_cast<i16>(hit.blue);
        game.PrepareDarkenedCopies();
        battle.unk53BF94 = 1;
        battle.unk53BF98 = 1;
    }

    if (battle.unk4FF0BC[at] == 1)
    {
        follow.id = hit.effectA;
        follow.action = hit.effectQ;
        follow.hold = 0;

        if (battle.paused == 1)
        {
            follow.hold = 1;
        }

        if (game.acts.Run(follow) == 0)
        {
            follow = ActState{};
            battle.unk4FF0BC[at] = 0;
            return 1;
        }

        if (follow.target != 0)
        {
            hit.target = follow.target;
            follow.target = 0;
        }

        const Picture* picture = game.archive.LoadPictureKept(follow.frame, follow.picture);
        game.audio.PlaySound(follow.view, game.config.soundVolume);
        game.audio.SetSoundPan(follow.view, originX + static_cast<i16>(targetX) >= kHitPanCentre ? kHitPan : -kHitPan);
        follow.view = 0;
        u32 flags = follow.flags;

        if (attacker.kind == 0)
        {
            flags ^= Painter::Mirror;
        }

        if (battle.mirrored == 1)
        {
            flags ^= Painter::Mirror;
        }

        // A picture that does not load is read through a null pointer in the original.
        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(battle.hitPictureX, battle.hitPictureY, picture->width, picture->height, flags);
        }

        if (follow.waiting == 1)
        {
            battle.unk4FF0BC[at] = 0;
        }
    }

    game.TakeHitBits(target, attacker, index);

    if (battle.hitFading[at] == 1 && this->FadeHitActAdvance(target, hit.effectA, hit.effectQ, index, scene) == 1)
    {
        hit.target = 0;
        battle.hitFading[at] = 0;
    }

    if (battle.slideBackPicture > 0 && game.PushUnits(hit.picture, hit.waiting) == 0)
    {
        return 0;
    }

    if (hit.waiting != 1 || battle.hitFading[at] != 0 || battle.unk4FF0BC[at] != 0)
    {
        return 0;
    }

    battle.hitPictureY = 0;
    follow = ActState{};
    battle.hitPictureX = 0;
    battle.rippleFrames[at] = 0;
    return 1;
}

// What Game::HitAllFrame decides of the frame; the draws into `scene`.
int HitFrame::HitAllFrameAdvance(BattleUnit& target, BattleUnit& attacker, u32 act, int index, int all, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    ScenePainter painter(game.battleServices, scene);
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.hitActs[at];
    ActState& follow = battle.followActs[at];
    u16 delay = 0;
    battle.lastDamage = 0;

    if (hit.waiting == 0)
    {
        hit.id = static_cast<i32>(act);
        hit.action = 0;
        hit.hold = 0;

        if (battle.paused == 1)
        {
            hit.hold = 1;
        }

        if (game.acts.Run(hit) == 0)
        {
            return 0;
        }

        const Picture* picture = game.archive.LoadPictureKept(hit.frame, hit.picture);
        game.audio.PlaySound(hit.view, game.config.soundVolume);
        hit.view = 0;

        // A picture that does not load is read through a null pointer in the original.
        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
        }

        game.PlaceAllHit(attacker, picture != nullptr ? picture->width : 0, index, delay);
        u32 flags = hit.flags;

        if (attacker.kind == 0)
        {
            flags ^= Painter::Mirror;
        }

        if (battle.mirrored == 1)
        {
            flags ^= Painter::Mirror;
        }

        if (picture != nullptr)
        {
            painter.DrawPicture(battle.hitPictureX, battle.hitPictureY, picture->width, picture->height, flags);
        }

        ++battle.hitAllFrames;
        battle.darkBlue = static_cast<i16>(hit.blue);
        battle.darkRed = static_cast<i16>(hit.red);
        battle.darkGreen = static_cast<i16>(hit.green);
        battle.unk53BF94 = 1;
        battle.unk53BF98 = 1;
        game.PrepareDarkenedCopies();
    }

    if (battle.unk4FF0BC[at] == 1)
    {
        follow.id = hit.effectA;
        follow.action = hit.effectQ;

        if (game.acts.Run(follow) == 0)
        {
            return 0;
        }

        if (follow.target != 0)
        {
            hit.target = follow.target;
            follow.target = 0;
        }

        const Picture* picture = game.archive.LoadPictureKept(follow.frame, follow.picture);
        game.audio.PlaySound(follow.view, game.config.soundVolume);
        u32 flags = follow.flags;
        hit.view = 0;

        if (attacker.kind == 0)
        {
            flags ^= Painter::Mirror;
        }

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(battle.hitPictureX, battle.hitPictureY, picture->width, picture->height, flags);
        }

        if (follow.waiting == 1)
        {
            battle.unk4FF0BC[at] = 0;
        }
    }

    if ((hit.target & ActState::Recoils) != 0)
    {
        battle.unk4FF0BC[at] = 1;
        hit.target = 0;
    }

    if ((hit.target & ActState::Quakes) != 0)
    {
        battle.quaking = 1;
        battle.quakePhase = 0;
    }

    if ((hit.target & ActState::FollowsUp) != 0)
    {
        if ((hit.target & kAllHitNudge) != 0)
        {
            hit.target = static_cast<u16>(hit.target & ~kAllHitNudge);
            game.NudgeAllHit(target, attacker, all);
        }

        if ((hit.target & ActState::FadesField) != 0)
        {
            battle.StartFieldFade(
                static_cast<i16>(hit.lights[0]),
                static_cast<i16>(hit.lights[1]),
                static_cast<i16>(hit.lights[2]),
                static_cast<i16>(hit.lights[3]),
                static_cast<i16>(hit.lights[4]),
                static_cast<i16>(hit.lights[5]),
                static_cast<i16>(hit.lights[6])
            );
            hit.target = static_cast<u16>(hit.target & ~ActState::FadesField);
            battle.fadingField = 1;
        }
    }

    if ((hit.target & ActState::UnkTarget4) != 0)
    {
        game.RaiseAllHit(target, all);
        hit.target = 0;
    }

    if ((hit.target & (ActState::Strikes | ActState::UnkTarget10)) != 0)
    {
        game.StrikeAllHit(target, attacker, index, all, delay);
        hit.target = 0;
    }

    if (battle.slideBackPicture > 0)
    {
        battle.darkenByTint = 1;

        if (game.PushUnits(hit.picture, hit.waiting) == 0)
        {
            return 0;
        }
    }

    if (hit.waiting != 1 || battle.unk4FF0BC[at] != 0)
    {
        return 0;
    }

    if (battle.slideBackPicture > 0)
    {
        game.PushUnits(hit.picture, 1);
    }

    battle.hitAllFrames = 0;
    follow = ActState{};
    battle.unk4FF0BC[at] = 0;
    return 1;
}

}  // namespace swd3::smooth
