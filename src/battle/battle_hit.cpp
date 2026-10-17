#include "game/game.h"
#include <algorithm>

#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr int kHitPanCentre = 0x140;  // the sound pans right from the middle of the field
constexpr int kHitPan = 0x10;
constexpr i8 kHitFadeEnd = -0x20;
constexpr i8 kHitFadeStep = 4;
constexpr i16 kPushStart = 0x1A4;     // the push of a hit, halved each call
constexpr i16 kSlideStep = 0x1E;      // the units slide back by at most this a call
constexpr i32 kRippleWalked = 0x3E8;  // HitTargetFrame: the ripple frames once the walk reached the target
constexpr i32 kRippleHold = 0x28;
constexpr u16 kRippleSound = 0x142;
constexpr int kRippleShiftX = 0x50;
constexpr int kRippleStartShiftY = 0x96;
constexpr int kRippleShiftY = 0x78;
constexpr int kRippleBackX = 0x14;
constexpr int kRippleSplashX = 0x64;
constexpr int kRippleSplashY = 0x32;
constexpr int kRippleSplashRadius = 0xC;
constexpr int kRippleSplashDepth = 0xC8;
constexpr int kSplashShiftX = 0x64;
constexpr i32 kSplashEvery = 9;
constexpr i32 kSplashLast = 0x1E;
constexpr u32 kSplashSpreadY = 100;
constexpr u32 kSplashSpreadX = 80;
constexpr int kSplashFromX = 0x3C;
constexpr i32 kSplashFrames = 0x64;
constexpr u16 kHitNudgeFrames = 0x1E;
constexpr std::array<std::pair<u16, u16>, 3> kHitNudges = {{{0x1000, 1}, {0x2000, 2}, {0x4000, 3}}};
constexpr i32 kMaxHitDamage = 0x270F;
constexpr u16 kDigitsDelay = 8;
constexpr int kAllHitLeftX = 0xA0;  // HitAllFrame: the place of a picture without an anchor
constexpr int kAllHitRightX = 0x1E0;
constexpr int kAllHitY = 0xEB;
constexpr u16 kAllHitNudge = 0x1000;

}  // namespace

// swd32.exe 0x004599B0: one frame of act `act` on `unit` (unit `index`, its hit act unk5202A8); 1 once the act waits
// (cleared) or ends (the follow act unk4FE600 cleared instead). A quaking act quakes the screen. The picture is read
// straight from its file each frame, mirrored with the sides, at the unit's offset from its origin (or its draw
// place), with the act's sound panned by the side.
int Game::PlayHitAct(BattleUnit& unit, u32 act, int index)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];

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

        if (this->acts.Run(hit) == 0)
        {
            battle.unk4FE600[at] = ActState{};
            battle.unk4FF0BC[at] = 0;
            return 1;
        }

        const std::unique_ptr<Picture> picture = this->archive.LoadPictureDirect(hit.frame, hit.picture);
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
        this->audio.PlaySound(hit.view, this->config.soundVolume);
        this->audio.SetSoundPan(hit.view, originX + static_cast<i16>(x) >= kHitPanCentre ? kHitPan : -kHitPan);
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

// swd32.exe 0x00459BF0: one frame of act `act` (action `action`) fading on `unit` (unit `index`, its act
// unk524980): 1 at once without an act, or once the fade (unk521374, darker by kHitFadeStep a frame) is past
// kHitFadeEnd (reset to 0); 0 when the act ends.
int Game::FadeHitAct(BattleUnit& unit, u32 act, i32 action, int index)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);

    if (act == 0)
    {
        return 1;
    }

    i8& fade = battle.unk521374[at];

    if (fade <= kHitFadeEnd)
    {
        fade = 0;
        return 1;
    }

    u16 x = 0;
    u16 y = 0;
    unit.GetDrawPosition(x, y);
    ActState& effect = battle.unk524980[at];
    effect.id = act;
    effect.action = action;
    effect.hold = 0;

    if (battle.paused == 1)
    {
        effect.hold = 1;
    }

    if (this->acts.Run(effect) == 0)
    {
        return 0;
    }

    const Picture* picture = this->archive.GetPicture(effect.frame, effect.picture);

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

// swd32.exe 0x0045D810: the point unit `index` reaches walking the line from (fromX, fromY) to (toX, toY), `speed`
// steps more each call (unk4FDF94 counts the calls); 1 once it reaches toX (the count reset).
int Game::WalkLine(i32& x, i32& y, int fromX, int fromY, int toX, int toY, int speed, int index)
{
    BattleState& battle = this->battleState;
    u16& calls = battle.unk4FDF94[static_cast<std::size_t>(index)];
    LineWalker walker;
    walker.fromX = fromX;
    walker.fromY = fromY;
    walker.toX = toX;
    walker.toY = toY;
    ++calls;
    const int steps = speed * calls;
    bool reached = false;

    for (int step = 0; step < steps; ++step)
    {
        LineStep(walker);

        if (walker.fromX + walker.x == walker.toX)
        {
            reached = true;
            break;
        }
    }

    x = walker.fromX + walker.x;
    y = walker.fromY + walker.y;

    if (!reached)
    {
        return 0;
    }

    calls = 0;
    return 1;
}

// swd32.exe 0x0045BD90: the push of a hit across the field, every unit moved by `shake` (the background too): while
// the push (unk4A7638) lasts it halves each call, the half added to the pushed distance (unk4FDF78) and moved away
// from the side (left unless mirrored); 0 while the units move. Once it is spent and the hit act is past picture
// unk53AE7A, the units slide back by up to kSlideStep a call and the field darkens by tint steps; 1 then (`restart`
// 1 sets the push again).
int Game::PushUnits(u16 picture, int restart)
{
    BattleState& battle = this->battleState;
    ++battle.unk53BDE8;

    if (battle.unk4A7638 > 0)
    {
        battle.unk53BDE8 = 0;
        const auto half = static_cast<i16>(battle.unk4A7638 / 2);
        battle.unk4FDF78 = static_cast<u16>(battle.unk4FDF78 + half);
        battle.unk4A7638 = half;
        battle.shake = battle.mirrored != 0 ? half : -half;
    }

    if (battle.shake != 0)
    {
        this->ShiftUnits(battle.shake);
        return 0;
    }

    if (static_cast<int>(picture) > battle.unk53AE7A)
    {
        const auto pushed = static_cast<i16>(battle.unk4FDF78);

        if (pushed > 0)
        {
            const i16 step = pushed < kSlideStep ? pushed : kSlideStep;
            battle.unk4FDF78 = static_cast<u16>(pushed - step);
            battle.shake = battle.mirrored != 0 ? -step : step;
            this->ShiftUnits(battle.shake);
        }

        battle.darkenByTint = 1;
    }

    if (restart == 1)
    {
        battle.unk4A7638 = kPushStart;
    }

    return 1;
}

// The moves of PushUnits (inline twice in the original): the roles, then the enemies, moved across by `shift`.
void Game::ShiftUnits(int shift)
{
    BattleState& battle = this->battleState;

    auto move = [shift](BattleUnit& unit)
    {
        u16 x = 0;
        u16 y = 0;
        unit.GetPosition(x, y);
        unit.SetPosition(static_cast<i16>(x + shift), static_cast<i16>(y));
    };

    for (i16 index = 0; index < static_cast<i32>(battle.roleCount); ++index)
    {
        move(this->battleRoles[static_cast<std::size_t>(index)]);
    }

    for (i16 index = 0; index < static_cast<i32>(battle.enemyCount); ++index)
    {
        move(this->battleEnemies[static_cast<std::size_t>(index)]);
    }
}

// swd32.exe 0x0045E7D0: the frame shown shaken by the quake step of quakePhase (QuakeSteps): odd steps move it down,
// even ones up, the band left uncovered black (the frame's top rows are cleared for it); the original blits to the
// primary surface instead of the plain copy. Every unk53BD68 frames (counted in glowFrames, shared with the panel
// glow) the phase steps on (between 0 and 1 for UnkPanel100); after 10 the quake ends.
void Game::QuakeFrame()
{
    BattleState& battle = this->battleState;
    const auto phase = static_cast<std::size_t>(battle.quakePhase);
    const int step = phase < BattleState::QuakeSteps.size() ? BattleState::QuakeSteps[phase] : 0;
    const bool down = battle.quakePhase % 2 != 0;
    Surface* primary = this->graphics.GetSurface(Graphics::SurfaceId::Primary);

    auto copyRows = [this, primary](int from, int to, int rows)
    {
        if (primary == nullptr || this->offscreen == nullptr)
        {
            return;
        }

        for (int row = 0; row < rows; ++row)
        {
            std::copy_n(this->offscreen->Row(from + row), Display::Width, primary->Row(to + row));
        }
    };

    if (down)
    {
        copyRows(0, step, Display::Height - step);
    }
    else
    {
        copyRows(step, 0, Display::Height - step);
    }

    std::fill_n(this->drawSurface, static_cast<std::size_t>(Display::Width * step), u16{0});
    copyRows(0, down ? 0 : Display::Height - step, step);
    this->graphics.Present();

    if (++battle.glowFrames > battle.unk53BD68)
    {
        battle.glowFrames = 0;

        if ((battle.panelFlags & BattleState::UnkPanel100) != 0)
        {
            battle.quakePhase = (battle.quakePhase + 1) % 2;
        }
        else
        {
            ++battle.quakePhase;
        }
    }

    if (battle.quakePhase == static_cast<i32>(BattleState::QuakeSteps.size()))
    {
        battle.quaking = 0;
        battle.quakePhase = 0;
    }
}

// swd32.exe 0x004582B0: one frame of the hit act `act` of `attacker` on `target` (unit slot `index`, its hit act
// unk5202A8 and follow act unk4FE600); 1 once the hit act waits with no fade or follow act left (the follow act and
// the ripple frames cleared), 1 too when an act ends. In the ripple battle (unk53C03C) a ripple runs instead: walking
// from the attacker to the target (then held for kRippleHold frames) for a moving attack, splashes over the target
// otherwise. Else the picture is read straight from its file and drawn, walking from the attacker to the target for a
// moving attack. The follow act plays its effects over it. The target bits: Recoils starts the follow act, Quakes
// the quake, FollowsUp the nudges and the field fade, UnkTarget4 raises flag 26B8, Strikes (and UnkTarget10) take
// the damage of the hit with its digits (and the spirit and stamina digits of a role's hit).
int Game::HitTargetFrame(BattleUnit& target, BattleUnit& attacker, u32 act, int index)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];
    ActState& follow = battle.unk4FE600[at];
    u16 targetX = 0;
    u16 targetY = 0;
    int originX = 0;

    if (hit.waiting == 0)
    {
        if (battle.unk53C03C == 1)
        {
            this->RippleHitFrame(target, attacker, index, targetX);
        }
        else if (!this->DrawHitPicture(target, attacker, act, index, targetX, targetY, originX))
        {
            return 1;
        }

        battle.darkRed = static_cast<i16>(hit.red);
        battle.darkGreen = static_cast<i16>(hit.green);
        battle.darkBlue = static_cast<i16>(hit.blue);
        this->PrepareDarkenedCopies();
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

        if (this->acts.Run(follow) == 0)
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

        const std::unique_ptr<Picture> picture = this->archive.LoadPictureDirect(follow.frame, follow.picture);
        this->audio.PlaySound(follow.view, this->config.soundVolume);
        this->audio.SetSoundPan(follow.view, originX + static_cast<i16>(targetX) >= kHitPanCentre ? kHitPan : -kHitPan);
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
            painter.DrawPicture(battle.unk53BDE0, battle.unk53BDE4, picture->width, picture->height, flags);
        }

        if (follow.waiting == 1)
        {
            battle.unk4FF0BC[at] = 0;
        }
    }

    this->TakeHitBits(target, attacker, index);

    if (battle.unk52411C[at] == 1 && this->FadeHitAct(target, hit.effectA, hit.effectQ, index) == 1)
    {
        hit.target = 0;
        battle.unk52411C[at] = 0;
    }

    if (battle.unk53AE7A > 0 && this->PushUnits(hit.picture, hit.waiting) == 0)
    {
        return 0;
    }

    if (hit.waiting != 1 || battle.unk52411C[at] != 0 || battle.unk4FF0BC[at] != 0)
    {
        return 0;
    }

    battle.unk53BDE4 = 0;
    follow = ActState{};
    battle.unk53BDE0 = 0;
    battle.unk53AEE8[at] = 0;
    return 1;
}

// The ripple of HitTargetFrame (inline in the original); see there. The ripple (battleRipple) is refracted over the
// frame at the walked point (kRippleSound once it reaches the target) or over the target.
void Game::RippleHitFrame(BattleUnit& target, BattleUnit& attacker, int index, u16& targetX)
{
    BattleState& battle = this->battleState;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];
    i32& frames = battle.unk53AEE8[at];
    u16 speed = 0;
    u16 targetY = 0;
    const std::span<u16> screen(
        this->drawSurface,
        static_cast<std::size_t>(this->painter.ScreenPitch()) * Display::Height
    );

    // Without a ripple the original calls through a null pointer: the ripple is skipped.
    ScriptObject* ripple = this->battleRipple.get();

    auto refract = [ripple, &screen](int x, int y)
    {
        if (ripple != nullptr)
        {
            ripple->MoveTo(x, y);
            ripple->Refract(screen);
            ripple->Step();
        }
    };

    if (attacker.CommandTarget(speed) == 1)
    {
        u16 fromX = 0;
        u16 fromY = 0;
        target.GetDrawPosition(targetX, targetY);
        attacker.GetDrawPosition(fromX, fromY);

        if (frames < kRippleWalked && this->WalkLine(
                                          battle.unk53BDE0,
                                          battle.unk53BDE4,
                                          static_cast<i16>(fromX),
                                          static_cast<i16>(fromY),
                                          static_cast<i16>(targetX),
                                          static_cast<i16>(targetY),
                                          speed,
                                          index
                                      ) == 1)
        {
            this->audio.PlaySound(kRippleSound, this->config.soundVolume);
            hit.target |= ActState::Strikes;
            frames = kRippleWalked;
        }

        if (ripple != nullptr)
        {
            ripple->CopyScreen(screen);

            if (frames == 0)
            {
                ripple->MoveTo(battle.unk53BDE0 - kRippleShiftX, battle.unk53BDE4 - kRippleStartShiftY);
                ripple
                    ->Splash(kRippleSplashX, kRippleSplashY, kRippleSplashRadius, kRippleSplashDepth, this->crtRandom);
            }
        }

        if (++frames >= kRippleWalked)
        {
            battle.unk53BDE0 = static_cast<i16>(targetX) - kRippleBackX;
            battle.unk53BDE4 = static_cast<i16>(targetY);
        }

        refract(battle.unk53BDE0 - kRippleShiftX, battle.unk53BDE4 - kRippleShiftY);

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

    if (ripple != nullptr)
    {
        ripple->CopyScreen(screen);
    }

    if (frames % kSplashEvery == 0 && frames <= kSplashLast)
    {
        if (ripple != nullptr)
        {
            ripple->MoveTo(x, y);
        }

        if (frames == 0)
        {
            this->audio.PlaySound(kRippleSound, this->config.soundVolume);
        }

        const int splashY = static_cast<int>(this->random.Below(kSplashSpreadY)) + kRippleSplashY;
        const int splashX = static_cast<int>(this->random.Below(kSplashSpreadX)) + kSplashFromX;

        if (ripple != nullptr)
        {
            ripple->Splash(splashX, splashY, kRippleSplashRadius, kRippleSplashDepth, this->crtRandom);
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

// The picture of HitTargetFrame (inline in the original); see there. The act runs a step (false once it ends: the
// follow act cleared); the picture is mirrored for an enemy attacker and with the sides, placed at the attacker's
// offset from its origin less the act's anchor, or at the target less the act's place, its sound panned by the side.
// For a moving attack it walks the line from the attacker; reaching the target strikes and ends the act.
bool Game::DrawHitPicture(BattleUnit& target, BattleUnit& attacker, u32 act, int index, u16& x, u16& y, int& originX)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];
    hit.id = static_cast<i32>(act);
    hit.action = 0;
    hit.hold = 0;

    if (battle.paused == 1)
    {
        hit.hold = 1;
    }

    if (this->acts.Run(hit) == 0)
    {
        battle.unk4FE600[at] = ActState{};
        battle.unk4FF0BC[at] = 0;
        return false;
    }

    const std::unique_ptr<Picture> picture = this->archive.LoadPictureDirect(hit.frame, hit.picture);
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

    this->audio.PlaySound(hit.view, this->config.soundVolume);
    this->audio.SetSoundPan(hit.view, originX + static_cast<i16>(x) >= kHitPanCentre ? kHitPan : -kHitPan);
    hit.view = 0;
    attacker.ClearMoveAxes(x, y);
    battle.unk53BDE0 = static_cast<i16>(x);
    battle.unk53BDE4 = static_cast<i16>(y);
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

        if (this->WalkLine(
                battle.unk53BDE0,
                battle.unk53BDE4,
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
            battle.unk53BDE0,
            battle.unk53BDE4,
            picture->width,
            picture->height,
            flags,
            PaletteArgument(*picture)
        );
    }

    return true;
}

// The target bits of HitTargetFrame (inline in the original); see there. A strike's damage (capped at kMaxHitDamage,
// none for -1) adds to unk5242B0 of the slot and becomes hitDamage; resisted (ActFlag14) it shows the target's
// resistance panel and ends the menu.
void Game::TakeHitBits(BattleUnit& target, BattleUnit& attacker, int index)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];

    if ((hit.target & ActState::Recoils) != 0)
    {
        hit.target = 0;
        battle.unk4FF0BC[at] = 1;
    }

    if ((hit.target & ActState::Quakes) != 0)
    {
        battle.quakePhase = 0;
        battle.quaking = 1;
    }

    if ((hit.target & ActState::FollowsUp) != 0)
    {
        for (const auto& [bit, nudge] : kHitNudges)
        {
            if ((hit.target & bit) != 0)
            {
                hit.target = static_cast<u16>(hit.target & ~bit);
                target.SetNudge(kHitNudgeFrames, nudge);
            }
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
        target.RaiseFlag26B8();
        hit.target = 0;
    }

    if ((hit.target & ActState::Strikes) != 0)
    {
        target.RaiseFlag26B8();
    }
    else if ((hit.target & ActState::UnkTarget10) == 0)
    {
        return;
    }

    if (attacker.ActFlag14() == 1)
    {
        battle.panelFlags |= BattleState::ResistancePanel;
        const ObjectRecord* record = target.Data();
        messages.panelUnit = record != nullptr ? &record->data : nullptr;
        battle.result = 0;
    }

    const i16 damage = attacker.kind == 1
                           ? attacker.HitDamage(this->battleServices, target, battle.spiritDamage, battle.staminaDamage)
                           : attacker.EnemyHitDamage(this->battleServices, target);
    battle.lastDamage = damage;

    if (battle.lastDamage >= kMaxHitDamage)
    {
        battle.lastDamage = kMaxHitDamage;
    }

    u16 delay = 0;

    if (battle.lastDamage == -1)
    {
        battle.lastDamage = 0;
    }
    else
    {
        target.SetDamageDigits(this->battleServices, battle.lastDamage);
        target.StepTargetCount(1);
        delay = static_cast<u16>(delay + kDigitsDelay);
    }

    if (battle.spiritDamage != 0)
    {
        target.PushTarget(BattleUnit::SpiritGainTarget);
        target.SetDamageDigits(this->battleServices, battle.spiritDamage);
        target.PushTargetAlt(delay);
        target.StepTargetCount(1);
        delay = static_cast<u16>(delay + kDigitsDelay);
    }

    if (battle.staminaDamage != 0)
    {
        target.PushTarget(BattleUnit::StaminaGainTarget);
        target.SetDamageDigits(this->battleServices, battle.staminaDamage);
        target.PushTargetAlt(delay);
        target.StepTargetCount(1);
    }

    battle.unk5241B4[at] = battle.spiritDamage;
    battle.unk5242B0[at] += battle.lastDamage;
    battle.unk524220[at] = battle.staminaDamage;
    battle.hitDamage = battle.unk5242B0[at];
    battle.unk52411C[at] = 0;
    hit.target = 0;
}

// swd32.exe 0x00458DE0: one frame of the hit act `act` of `attacker` on `target` or, with `all` 1, on its whole side
// (unit slot `index`); 1 once the hit act waits with no follow act left (the follow act cleared), 0 when an act ends.
// The picture is read straight from its file (counting unk4FE5C8), placed at the attacker's offset from its origin less
// the act's anchor, or on a fixed place by the side (kAllHitLeftX, kAllHitRightX, kAllHitY: an enemy attacker
// unmirrored keeps y 0), walking along y 0 for a moving attack. The follow act clears the hit act's sound instead of
// its own (kept). The target bits work as in HitTargetFrame, on every unit of the side with `all`: the roles hit by
// a role while unk53BF64 and unk53AE7C take the damage at once (Damage), as do all roles while unk53BF68; the spirit
// and stamina of a hit are cleared after their digits (so the slots record 0 for them), and the digit delay of the
// enemies runs on from the walk speed without a reset (kept).
int Game::HitAllFrame(BattleUnit& target, BattleUnit& attacker, u32 act, int index, int all)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.unk5202A8[at];
    ActState& follow = battle.unk4FE600[at];
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

        if (this->acts.Run(hit) == 0)
        {
            return 0;
        }

        const std::unique_ptr<Picture> picture = this->archive.LoadPictureDirect(hit.frame, hit.picture);
        this->audio.PlaySound(hit.view, this->config.soundVolume);
        hit.view = 0;

        // A picture that does not load is read through a null pointer in the original.
        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
        }

        this->PlaceAllHit(attacker, picture != nullptr ? picture->width : 0, index, delay);
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
            painter.DrawPicture(battle.unk53BDE0, battle.unk53BDE4, picture->width, picture->height, flags);
        }

        ++battle.unk4FE5C8;
        battle.darkBlue = static_cast<i16>(hit.blue);
        battle.darkRed = static_cast<i16>(hit.red);
        battle.darkGreen = static_cast<i16>(hit.green);
        battle.unk53BF94 = 1;
        battle.unk53BF98 = 1;
        this->PrepareDarkenedCopies();
    }

    if (battle.unk4FF0BC[at] == 1)
    {
        follow.id = hit.effectA;
        follow.action = hit.effectQ;

        if (this->acts.Run(follow) == 0)
        {
            return 0;
        }

        if (follow.target != 0)
        {
            hit.target = follow.target;
            follow.target = 0;
        }

        const std::unique_ptr<Picture> picture = this->archive.LoadPictureDirect(follow.frame, follow.picture);
        this->audio.PlaySound(follow.view, this->config.soundVolume);
        u32 flags = follow.flags;
        hit.view = 0;

        if (attacker.kind == 0)
        {
            flags ^= Painter::Mirror;
        }

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(battle.unk53BDE0, battle.unk53BDE4, picture->width, picture->height, flags);
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
            this->NudgeAllHit(target, attacker, all);
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
        this->RaiseAllHit(target, all);
        hit.target = 0;
    }

    if ((hit.target & (ActState::Strikes | ActState::UnkTarget10)) != 0)
    {
        this->StrikeAllHit(target, attacker, index, all, delay);
        hit.target = 0;
    }

    if (battle.unk53AE7A > 0)
    {
        battle.darkenByTint = 1;

        if (this->PushUnits(hit.picture, hit.waiting) == 0)
        {
            return 0;
        }
    }

    if (hit.waiting != 1 || battle.unk4FF0BC[at] != 0)
    {
        return 0;
    }

    if (battle.unk53AE7A > 0)
    {
        this->PushUnits(hit.picture, 1);
    }

    battle.unk4FE5C8 = 0;
    follow = ActState{};
    battle.unk4FF0BC[at] = 0;
    return 1;
}

// The place of the picture of HitAllFrame (inline in the original); see there. `delay` takes the walk speed of a
// moving attack, which walks along y 0 from the attacker (slot 0 counts the walk); reaching the end strikes and ends
// the act.
void Game::PlaceAllHit(BattleUnit& attacker, int width, int index, u16& delay)
{
    BattleState& battle = this->battleState;
    ActState& hit = battle.unk5202A8[static_cast<std::size_t>(index)];
    u16 offsetX = 0;
    u16 offsetY = 0;
    u16 x = 0;
    u16 y = 0;
    attacker.GetOffset(offsetX, offsetY);

    if (offsetX != 0 && offsetY != 0)
    {
        attacker.GetOrigin(x, y);
        x = static_cast<u16>(x + offsetX);
        y = static_cast<u16>(y + offsetY);
    }

    int originX = hit.x;
    u16 anchorX = hit.anchorX;

    if (attacker.kind == 0)
    {
        if (originX != 0)
        {
            originX = width - hit.x;
        }

        if (hit.anchorX != 0)
        {
            anchorX = static_cast<u16>(width - hit.anchorX);
        }
    }

    if (battle.mirrored == 1)
    {
        if (originX != 0)
        {
            originX = width - hit.x;
        }

        if (hit.anchorX != 0)
        {
            anchorX = hit.anchorX;
        }
    }

    if (anchorX != 0 || hit.anchorY != 0)
    {
        if (x != 0 || y != 0)
        {
            x = static_cast<u16>(x - anchorX);
            y = static_cast<u16>(y - hit.anchorY);
        }
    }
    else
    {
        x = 0;
        y = 0;

        if (battle.mirrored == 1)
        {
            x = static_cast<u16>(Display::Width - width);
        }

        if (originX != 0)
        {
            const bool leftSide = (attacker.kind != 0) != (battle.mirrored == 1);
            x = static_cast<u16>((leftSide ? kAllHitLeftX : kAllHitRightX) - originX);

            if (attacker.kind != 0 || battle.mirrored == 1)
            {
                y = static_cast<u16>(kAllHitY - static_cast<u16>(hit.y));
            }

            if (attacker.kind == 0 && battle.mirrored == 1)
            {
                x = static_cast<u16>(kAllHitLeftX - static_cast<u16>(hit.x));
            }
        }
    }

    battle.unk53BDE0 = static_cast<i16>(x);
    battle.unk53BDE4 = static_cast<i16>(y);

    if (attacker.CommandTarget(delay) != 1)
    {
        return;
    }

    u16 fromX = 0;
    u16 fromY = 0;
    attacker.GetDrawPosition(fromX, fromY);
    fromX = static_cast<u16>(fromX - originX);

    if (x == 0)
    {
        x = static_cast<u16>(-originX);
    }

    if (this->WalkLine(
            battle.unk53BDE0,
            battle.unk53BDE4,
            static_cast<i16>(fromX),
            0,
            static_cast<i16>(x),
            0,
            delay,
            0
        ) == 1)
    {
        hit.target |= ActState::Strikes;
        hit.waiting = 1;
    }
}

// The nudge of HitAllFrame (inline in the original): with `all` every enemy (a role attacking) or every role,
// otherwise the target.
void Game::NudgeAllHit(BattleUnit& target, BattleUnit& attacker, int all)
{
    BattleState& battle = this->battleState;

    if (all != 1)
    {
        target.SetNudge(kHitNudgeFrames, 1);
        return;
    }

    if (attacker.kind == 1)
    {
        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            this->battleEnemies[index].SetNudge(kHitNudgeFrames, 1);
        }

        return;
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        this->battleRoles[index].SetNudge(kHitNudgeFrames, 1);
    }
}

// The flag 26B8 of HitAllFrame (inline in the original): with `all` every role in battle standing (while
// unk53BF68) or every enemy standing, otherwise the target.
void Game::RaiseAllHit(BattleUnit& target, int all)
{
    BattleState& battle = this->battleState;

    if (all != 1)
    {
        target.RaiseFlag26B8();
        return;
    }

    if (battle.unk53BF68 == 1)
    {
        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];

            if (role.unk2B00 != 1 && role.unk2B04 != 1 && !role.IsDefeated())
            {
                role.RaiseFlag26B8();
            }
        }

        return;
    }

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        BattleEnemy& enemy = this->battleEnemies[index];

        if (!enemy.IsDefeated())
        {
            enemy.RaiseFlag26B8();
        }
    }
}

// The digits of one hit of HitAllFrame (inline in the original): the damage capped at kMaxHitDamage (none for -1),
// then the spirit and stamina digits each kDigitsDelay later, those two cleared once shown. The spirit and stamina
// left are returned.
void Game::ShowAllHitDigits(BattleUnit& unit, i16 damage, u16& delay, i32& spirit, i32& stamina)
{
    BattleState& battle = this->battleState;
    battle.lastDamage = damage;

    if (battle.lastDamage >= kMaxHitDamage)
    {
        battle.lastDamage = kMaxHitDamage;
    }

    if (battle.lastDamage == -1)
    {
        battle.lastDamage = 0;
    }
    else
    {
        unit.SetDamageDigits(this->battleServices, battle.lastDamage);
        unit.StepTargetCount(1);
        delay = static_cast<u16>(delay + kDigitsDelay);
    }

    if (battle.spiritDamage != 0)
    {
        unit.PushTarget(BattleUnit::SpiritGainTarget);
        unit.SetDamageDigits(this->battleServices, battle.spiritDamage);
        unit.PushTargetAlt(delay);
        unit.StepTargetCount(1);
        delay = static_cast<u16>(delay + kDigitsDelay);
        battle.spiritDamage = 0;
    }

    if (battle.staminaDamage != 0)
    {
        unit.PushTarget(BattleUnit::StaminaGainTarget);
        unit.SetDamageDigits(this->battleServices, battle.staminaDamage);
        unit.PushTargetAlt(delay);
        unit.StepTargetCount(1);
        battle.staminaDamage = 0;
    }

    spirit = battle.spiritDamage;
    stamina = battle.staminaDamage;
}

// The strike of HitAllFrame (inline in the original); see there.
void Game::StrikeAllHit(BattleUnit& target, BattleUnit& attacker, int index, int all, u16& delay)
{
    BattleState& battle = this->battleState;
    ActState& hit = battle.unk5202A8[static_cast<std::size_t>(index)];
    i32 spirit = 0;
    i32 stamina = 0;

    if (all != 1)
    {
        if ((hit.target & ActState::Strikes) != 0)
        {
            target.RaiseFlag26B8();
            battle.quakePhase = 0;
        }

        const i16 damage =
            attacker.kind == 1
                ? attacker.HitDamage(this->battleServices, target, battle.spiritDamage, battle.staminaDamage)
                : attacker.EnemyHitDamage(this->battleServices, target);
        delay = 0;
        this->ShowAllHitDigits(target, damage, delay, spirit, stamina);
        battle.hitDamage += battle.lastDamage;
        return;
    }

    if ((battle.unk53BF64 == 1 && battle.unk53AE7C == 1) || battle.unk53BF68 == 1)
    {
        for (u32 slot = 0; slot < battle.roleCount; ++slot)
        {
            BattleRole& role = this->battleRoles[slot];

            if (role.unk2B00 == 1 || role.unk2B04 == 1 || role.StatusBit13() == 1)
            {
                continue;
            }

            i16 damage = 0;

            if (battle.unk53BF68 != 1)
            {
                damage = attacker.HitDamage(this->battleServices, role, battle.spiritDamage, battle.staminaDamage);
            }
            else
            {
                if (role.IsDefeated())
                {
                    continue;
                }

                if ((hit.target & ActState::Strikes) != 0)
                {
                    role.RaiseFlag26B8();
                }

                damage = attacker.EnemyHitDamage(this->battleServices, role);
            }

            delay = 0;
            this->ShowAllHitDigits(role, damage, delay, spirit, stamina);
            battle.unk5242B0[slot] += battle.lastDamage;
            battle.unk5241B4[slot] = spirit;
            battle.hitDamage = battle.unk5242B0[slot];
            battle.unk524220[slot] = stamina;

            if (battle.unk53BF64 == 1 && battle.unk53AE7C == 1)
            {
                role.Damage(
                    this->battleServices,
                    battle.unk5242B0[slot],
                    static_cast<i16>(spirit),
                    static_cast<i16>(stamina)
                );
            }
        }

        return;
    }

    for (u32 slot = 0; slot < battle.enemyCount; ++slot)
    {
        BattleEnemy& enemy = this->battleEnemies[slot];

        if (enemy.IsDefeated())
        {
            continue;
        }

        if ((hit.target & ActState::Strikes) != 0)
        {
            enemy.RaiseFlag26B8();
        }

        const i16 damage = attacker.HitDamage(this->battleServices, enemy, battle.spiritDamage, battle.staminaDamage);
        this->ShowAllHitDigits(enemy, damage, delay, spirit, stamina);
        battle.unk5242B0[slot] += battle.lastDamage;
        battle.unk5241B4[slot] = spirit;
        battle.unk524220[slot] = stamina;
        battle.hitDamage = battle.unk5242B0[slot];
    }
}
}  // namespace swd3
