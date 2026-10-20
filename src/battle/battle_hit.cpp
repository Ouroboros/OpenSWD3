#include "game/game.h"
#include <algorithm>

#include "battle/battle_hit_constants.h"
#include "battle/smooth/smooth_hit.h"
#include "core/fault.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr i16 kPushStart = 0x1A4;  // the push of a hit, halved each call
constexpr i16 kSlideStep = 0x1E;   // the units slide back by at most this a call

constexpr u16 kHitNudgeFrames = 0x1E;
constexpr std::array<std::pair<u16, u16>, 3> kHitNudges = {{{0x1000, 1}, {0x2000, 2}, {0x4000, 3}}};
constexpr i32 kMaxHitDamage = 0x270F;
constexpr u16 kDigitsDelay = 8;
constexpr int kAllHitLeftX = 0xA0;  // HitAllFrame: the place of a picture without an anchor
constexpr int kAllHitRightX = 0x1E0;
constexpr int kAllHitY = 0xEB;

constexpr u16 kWhitePixel = 0xFFFF;  // HitFrame: the frame flashed white on a defeat
constexpr u16 kEnemyAttack = 1;      // EnemyActionFrame: the command A of the acting enemy
constexpr u16 kEnemySpell = 2;
constexpr u16 kEnemyDrain = 6;
constexpr u16 kEnemyDefeatRole = 7;
constexpr u16 kEnemyFadeOut = 0xA;
constexpr u16 kEnemyFadeIn = 0xB;
constexpr u16 kEnemyHide = 0xC;
constexpr u16 kEnemyCall = 0xF;
constexpr u16 kEnemyEscape = 0x11;
constexpr u16 kEnemyDone = 100;
constexpr u16 kEnemyWalk = 200;
constexpr u16 kEnemyReturn = 300;
constexpr int kRippleGrid = 0xC8;
constexpr i16 kDrainDark = -12;
constexpr i32 kDefeatedUnitShift = 4;
constexpr i16 kFadedWord = 8;
constexpr i32 kEndCheckState = 0x63;
constexpr int kAfterAttackCommand = 0x12C;
constexpr u16 kCallStarted = 0x8000;
constexpr u16 kCallCount = 0x7FFF;
constexpr u16 kCallFrames = 0x19;
constexpr i16 kCallX = 0xF0;
constexpr i16 kCallY = 0xDC;
constexpr i16 kSecondCallY = 0x15E;
constexpr i32 kBattleLostState = 0x67;  // EnemyFrame
constexpr int kEnemySlotRecord = 2;
constexpr int kHangDraws = 0x10000;   // random draws after which a search of the original is taken as never ending
constexpr int kOrderTargetShift = 8;  // BattleState::enemyOrders: the high byte holds the bits, the low the target
constexpr u16 kOrderTarget = 0xFF;
constexpr u16 kOrderPrepared = 0x8000;
constexpr u16 kOrderSpell = 0x4000;
constexpr u16 kOrderDrain = 0x2000;
constexpr int kRunCommand = 0x11;
constexpr i16 kRunWord = 2;
constexpr int kEnemySkillX = 0x118;
constexpr i16 kEnemySkillFrames = 0x28;
constexpr int kEnemySpellCommand = 2;
constexpr int kEnemyDrainCommand = 6;
constexpr i32 kLastAimedUnit = 8;
constexpr i32 kFacingPanel = 0x80;
constexpr u16 kTalkCountdown = 0x7FFF;
constexpr u16 kNoAimedEnemy = 0xFFFF;

}  // namespace

// swd32.exe 0x004599B0: one frame of act `act` on `unit` (unit `index`, its hit act hitActs); 1 once the act waits
// (cleared) or ends (the follow act followActs cleared instead). A quaking act quakes the screen. The picture is read
// straight from its file each frame, mirrored with the sides, at the unit's offset from its origin (or its draw
// place), with the act's sound panned by the side.
int Game::PlayHitAct(BattleUnit& unit, u32 act, int index)
{
    smooth::Scene scene;
    const auto result = smooth::HitFrame(*this).PlayHitActAdvance(unit, act, index, scene);
    scene.Show(this->battleServices);
    return result;
}

// swd32.exe 0x00459BF0: one frame of act `act` (action `action`) fading on `unit` (unit `index`, its act
// fadeActs): 1 at once without an act, or once the fade (fadeActLevels, darker by kHitFadeStep a frame) is past
// kHitFadeEnd (reset to 0); 0 when the act ends.
int Game::FadeHitAct(BattleUnit& unit, u32 act, i32 action, int index)
{
    smooth::Scene scene;
    const auto result = smooth::HitFrame(*this).FadeHitActAdvance(unit, act, action, index, scene);
    scene.Show(this->battleServices);
    return result;
}

// swd32.exe 0x0045D810: the point unit `index` reaches walking the line from (fromX, fromY) to (toX, toY), `speed`
// steps more each call (walkLineCalls counts the calls); 1 once it reaches toX (the count reset).
int Game::WalkLine(i32& x, i32& y, int fromX, int fromY, int toX, int toY, int speed, int index)
{
    BattleState& battle = this->battleState;
    u16& calls = battle.walkLineCalls[static_cast<std::size_t>(index)];
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
// the push (hitPush) lasts it halves each call, the half added to the pushed distance (unk4FDF78) and moved away
// from the side (left unless mirrored); 0 while the units move. Once it is spent and the hit act is past picture
// slideBackPicture, the units slide back by up to kSlideStep a call and the field darkens by tint steps; 1 then
// (`restart` 1 sets the push again).
int Game::PushUnits(u16 picture, int restart)
{
    BattleState& battle = this->battleState;
    ++battle.pushFrames;

    if (battle.hitPush > 0)
    {
        battle.pushFrames = 0;
        const auto half = static_cast<i16>(battle.hitPush / 2);
        battle.unk4FDF78 = static_cast<u16>(battle.unk4FDF78 + half);
        battle.hitPush = half;
        battle.shake = battle.mirrored != 0 ? half : -half;
    }

    if (battle.shake != 0)
    {
        this->ShiftUnits(battle.shake);
        return 0;
    }

    if (static_cast<int>(picture) > battle.slideBackPicture)
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
        battle.hitPush = kPushStart;
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
// primary surface instead of the plain copy. Every quakeStepFrames frames (counted in glowFrames, shared with the panel
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

    if (++battle.glowFrames > battle.quakeStepFrames)
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
// hitActs and follow act followActs); 1 once the hit act waits with no fade or follow act left (the follow act and
// the ripple frames cleared), 1 too when an act ends. In the ripple battle (rippleHits) a ripple runs instead: walking
// from the attacker to the target (then held for kRippleHold frames) for a moving attack, splashes over the target
// otherwise. Else the picture is read straight from its file and drawn, walking from the attacker to the target for a
// moving attack. The follow act plays its effects over it. The target bits: Recoils starts the follow act, Quakes
// the quake, FollowsUp the nudges and the field fade, UnkTarget4 raises flag 26B8, Strikes (and UnkTarget10) take
// the damage of the hit with its digits (and the spirit and stamina digits of a role's hit).
int Game::HitTargetFrame(BattleUnit& target, BattleUnit& attacker, u32 act, int index)
{
    smooth::Scene scene;
    const auto result = smooth::HitFrame(*this).HitTargetFrameAdvance(target, attacker, act, index, scene);
    scene.Show(this->battleServices);
    return result;
}

// The ripple of HitTargetFrame (inline in the original); see there. The ripple (battleRipple) is refracted over the
// frame at the walked point (kRippleSound once it reaches the target) or over the target.
void Game::RippleHitFrame(BattleUnit& target, BattleUnit& attacker, int index, u16& targetX)
{
    smooth::Scene scene;
    smooth::HitFrame(*this).RippleHitFrameAdvance(target, attacker, index, targetX, scene);
    scene.Show(this->battleServices);
}

// The picture of HitTargetFrame (inline in the original); see there. The act runs a step (false once it ends: the
// follow act cleared); the picture is mirrored for an enemy attacker and with the sides, placed at the attacker's
// offset from its origin less the act's anchor, or at the target less the act's place, its sound panned by the side.
// For a moving attack it walks the line from the attacker; reaching the target strikes and ends the act.
bool Game::DrawHitPicture(BattleUnit& target, BattleUnit& attacker, u32 act, int index, u16& x, u16& y, int& originX)
{
    smooth::Scene scene;
    const auto result =
        smooth::HitFrame(*this).DrawHitPictureAdvance(target, attacker, act, index, x, y, originX, scene);
    scene.Show(this->battleServices);
    return result;
}

// The target bits of HitTargetFrame (inline in the original); see there. A strike's damage (capped at kMaxHitDamage,
// none for -1) adds to slotDamage of the slot and becomes hitDamage; resisted (ActFlag14) it shows the target's
// resistance panel and ends the menu.
void Game::TakeHitBits(BattleUnit& target, BattleUnit& attacker, int index)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    const auto at = static_cast<std::size_t>(index);
    ActState& hit = battle.hitActs[at];

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

    battle.lastSpiritDamage[at] = battle.spiritDamage;
    battle.slotDamage[at] += battle.lastDamage;
    battle.lastStaminaDamage[at] = battle.staminaDamage;
    battle.hitDamage = battle.slotDamage[at];
    battle.hitFading[at] = 0;
    hit.target = 0;
}

// swd32.exe 0x00458DE0: one frame of the hit act `act` of `attacker` on `target` or, with `all` 1, on its whole side
// (unit slot `index`); 1 once the hit act waits with no follow act left (the follow act cleared), 0 when an act ends.
// The picture is read straight from its file (counting hitAllFrames), placed at the attacker's offset from its origin
// less the act's anchor, or on a fixed place by the side (kAllHitLeftX, kAllHitRightX, kAllHitY: an enemy attacker
// unmirrored keeps y 0), walking along y 0 for a moving attack. The follow act clears the hit act's sound instead of
// its own (kept). The target bits work as in HitTargetFrame, on every unit of the side with `all`: the roles hit by
// a role while unk53BF64 and unk53AE7C take the damage at once (Damage), as do all roles while hitsAllRoles; the spirit
// and stamina of a hit are cleared after their digits (so the slots record 0 for them), and the digit delay of the
// enemies runs on from the walk speed without a reset (kept).
int Game::HitAllFrame(BattleUnit& target, BattleUnit& attacker, u32 act, int index, int all)
{
    smooth::Scene scene;
    const auto result = smooth::HitFrame(*this).HitAllFrameAdvance(target, attacker, act, index, all, scene);
    scene.Show(this->battleServices);
    return result;
}

// The place of the picture of HitAllFrame (inline in the original); see there. `delay` takes the walk speed of a
// moving attack, which walks along y 0 from the attacker (slot 0 counts the walk); reaching the end strikes and ends
// the act.
void Game::PlaceAllHit(BattleUnit& attacker, int width, int index, u16& delay)
{
    BattleState& battle = this->battleState;
    ActState& hit = battle.hitActs[static_cast<std::size_t>(index)];
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

    battle.hitPictureX = static_cast<i16>(x);
    battle.hitPictureY = static_cast<i16>(y);

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
            battle.hitPictureX,
            battle.hitPictureY,
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
// hitsAllRoles) or every enemy standing, otherwise the target.
void Game::RaiseAllHit(BattleUnit& target, int all)
{
    BattleState& battle = this->battleState;

    if (all != 1)
    {
        target.RaiseFlag26B8();
        return;
    }

    if (battle.hitsAllRoles == 1)
    {
        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];

            if (role.autoControl != 1 && role.guestControl != 1 && !role.IsDefeated())
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
    ActState& hit = battle.hitActs[static_cast<std::size_t>(index)];
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

    if ((battle.unk53BF64 == 1 && battle.unk53AE7C == 1) || battle.hitsAllRoles == 1)
    {
        for (u32 slot = 0; slot < battle.roleCount; ++slot)
        {
            BattleRole& role = this->battleRoles[slot];

            if (role.autoControl == 1 || role.guestControl == 1 || role.StatusBit13() == 1)
            {
                continue;
            }

            i16 damage = 0;

            if (battle.hitsAllRoles != 1)
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
            battle.slotDamage[slot] += battle.lastDamage;
            battle.lastSpiritDamage[slot] = spirit;
            battle.hitDamage = battle.slotDamage[slot];
            battle.lastStaminaDamage[slot] = stamina;

            if (battle.unk53BF64 == 1 && battle.unk53AE7C == 1)
            {
                role.Damage(
                    this->battleServices,
                    battle.slotDamage[slot],
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
        battle.slotDamage[slot] += battle.lastDamage;
        battle.lastSpiritDamage[slot] = spirit;
        battle.lastStaminaDamage[slot] = stamina;
        battle.hitDamage = battle.slotDamage[slot];
    }
}

// swd32.exe 0x0045C010: the hits of the acting unit actingUnit once its spell took effect (SpellCast, not SpellOver);
// 1 once they are over (the hit acts and, after a sweep, the slot totals cleared). The target is the attacker's
// word 29A2. A defeated target (Damage 1) flashes the frame white and is recorded. See RoleHitFrame and
// EnemyHitFrame.
int Game::HitFrame()
{
    BattleState& battle = this->battleState;

    if ((battle.spellStage & BattleState::SpellCast) == 0 || (battle.spellStage & BattleState::SpellOver) != 0)
    {
        return 0;
    }

    if (battle.actingUnit >= BattleState::FirstRoleUnit)
    {
        return this->RoleHitFrame(battle.actingUnit - BattleState::FirstRoleUnit);
    }

    return this->EnemyHitFrame(battle.actingUnit);
}

// The white frame of a defeat in HitFrame (inline in the original): the frame filled with 0xFFFF, as many pixels as
// the client area holds (written from the start of the frame without its pitch).
void Game::FlashFrameWhite()
{
    smooth::Scene scene;
    smooth::FillScreen(kWhitePixel, static_cast<std::size_t>(Display::Width) * Display::Height, scene);
    scene.Show(this->battleServices);
}

// The damage of the last hit cleared (inline in 0x0045C010).
void Game::ClearHitDamage()
{
    BattleState& battle = this->battleState;
    battle.hitDamage = 0;
    battle.spiritDamage = 0;
    battle.staminaDamage = 0;
}

// The slot totals of a sweep cleared (inline in 0x0045C010).
void Game::ClearHitTotals()
{
    BattleState& battle = this->battleState;
    battle.slotDamage.fill(0);
    battle.lastSpiritDamage.fill(0);
    battle.lastStaminaDamage.fill(0);
}

// One step of a sweep (inline in 0x0045C010): while fewer than `units` are swept (sweptUnits), every
// sweepGap frames one more is.
void Game::StepSweep(u32 units)
{
    BattleState& battle = this->battleState;

    if (battle.sweptUnits < units)
    {
        ++battle.pushFrames;

        if (battle.pushFrames >= battle.sweepGap)
        {
            ++battle.sweptUnits;
            battle.pushFrames = 0;
        }
    }
}

// The spirit a role drains from its damage (inline in 0x0045C010): with DrainShare giving a positive share, the
// share shows as spirit digits and is added as spirit.
void Game::DrainToSpirit(BattleRole& role, i32& damage)
{
    if (role.DrainShare(this->battleServices, damage) != 1 || damage <= 0)
    {
        return;
    }

    role.PushTarget(BattleUnit::SpiritGainTarget);
    role.SetDamageDigits(this->battleServices, damage);
    role.StepTargetCount(1);
    role.Damage(this->battleServices, 0, static_cast<i16>(damage), 0);
}

// A role attacking in HitFrame (inline in the original): with unk53AE84 its hit sweeps a side (unk53AE88 1: the
// whole side at once, the hits taken by the enemies; otherwise one unit after another as the sweep reaches them,
// until unk53AE74 are hit), else one target is hit (unk53AE88 1: HitAllFrame on the target alone). unk53AE7C makes
// the party the target side.
int Game::RoleHitFrame(int role)
{
    BattleState& battle = this->battleState;
    BattleRole& attacker = this->battleRoles[static_cast<std::size_t>(role)];
    const auto target = static_cast<i16>(attacker.GetWord29A2());
    const auto at = static_cast<std::size_t>(target);
    const u32 act = static_cast<u32>(battle.unk4FE5D4[static_cast<std::size_t>(role)]);

    if (battle.unk53AE84 != 0)
    {
        if (battle.unk53AE88 == 1)
        {
            if (this->HitAllFrame(this->battleEnemies[at], attacker, act, 0, 1) != 1)
            {
                return 0;
            }

            if (battle.unk53AE7C == 0)
            {
                for (u32 index = 0; index < battle.enemyCount; ++index)
                {
                    BattleEnemy& enemy = this->battleEnemies[index];

                    if (enemy.IsDefeated())
                    {
                        continue;
                    }

                    battle.slotTargets[index] = static_cast<i32>(index);
                    battle.hitLanded = 1;
                    battle.sweepHit = 1;

                    if (battle.unk53AF68 == 0 && enemy.Damage(
                                                     this->battleServices,
                                                     battle.slotDamage[index],
                                                     static_cast<i16>(battle.lastSpiritDamage[index]),
                                                     static_cast<i16>(battle.lastStaminaDamage[index])
                                                 ) == 1)
                    {
                        ++battle.unk53BD88;
                        battle.flashing = 1;
                        battle.unk4A7626 = 0;
                        battle.unk502984[index] = static_cast<i32>(index);
                        this->FlashFrameWhite();
                    }

                    this->ClearHitDamage();
                    ++battle.unk53BD80;
                    battle.slotDamage[index] = 0;
                    battle.lastSpiritDamage[index] = 0;
                    battle.lastStaminaDamage[index] = 0;
                }
            }

            this->ClearHitDamage();
            battle.unk53BD80 = 0;
            battle.hitActs.fill(ActState{});
            this->ClearHitTotals();
            return 1;
        }

        if (!this->SweepFromRole(attacker, target, act))
        {
            return 0;
        }

        battle.unk53BD80 = 0;
        battle.sweptUnits = 1;
        battle.hitActs.fill(ActState{});
        this->ClearHitTotals();
        return 1;
    }

    if (battle.unk53AE88 == 1)
    {
        if (battle.unk53AE7C == 0)
        {
            BattleEnemy& enemy = this->battleEnemies[at];

            if (this->HitAllFrame(enemy, attacker, act, 0, 0) != 1)
            {
                return 0;
            }

            battle.unk4FDF7C = attacker.GetWord29A2();
            battle.lastEnemyHit = static_cast<u16>(target);
            battle.hitLanded = 1;
            battle.slotTargets[at] = target;

            if (battle.unk53AF68 == 0 &&
                enemy.Damage(this->battleServices, battle.hitDamage, battle.spiritDamage, battle.staminaDamage) == 1)
            {
                battle.unk4A7626 = static_cast<u16>(target);
                battle.flashing = 1;
                battle.unk502984[at] = target;
                this->FlashFrameWhite();
            }
        }
        else
        {
            BattleRole& aimed = this->battleRoles[at];

            if (this->HitAllFrame(aimed, attacker, act, 0, 0) != 1)
            {
                return 0;
            }

            aimed.Damage(this->battleServices, battle.hitDamage, battle.spiritDamage, battle.staminaDamage);
            battle.slotTargets[at] = target;
        }

        this->ClearHitDamage();
        battle.hitActs.fill(ActState{});
        return 1;
    }

    if (battle.unk53AE7C == 0)
    {
        BattleEnemy& enemy = this->battleEnemies[at];

        if (this->HitTargetFrame(enemy, attacker, act, 0) != 1)
        {
            return 0;
        }

        battle.unk4FDF7C = attacker.GetWord29A2();
        battle.lastEnemyHit = static_cast<u16>(target);
        battle.hitLanded = 1;
        battle.slotTargets[at] = target;
        this->ApplyDrain(attacker, enemy);

        if (battle.unk53AF68 == 0 &&
            enemy.Damage(this->battleServices, battle.hitDamage, battle.spiritDamage, battle.staminaDamage) == 1)
        {
            battle.unk4A7626 = static_cast<u16>(target);
            battle.flashing = 1;
            battle.unk502984[at] = target;
            this->FlashFrameWhite();
        }

        this->ClearHitDamage();
        battle.hitActs.fill(ActState{});
        return 1;
    }

    BattleRole& aimed = this->battleRoles[at];

    if (this->HitTargetFrame(aimed, attacker, act, 0) != 1)
    {
        return 0;
    }

    battle.unk4FDF7C = attacker.GetWord29A2();
    battle.lastRoleHit = static_cast<u16>(target);

    if (battle.unk53AF68 == 0 &&
        aimed.Damage(this->battleServices, battle.slotDamage[0], battle.spiritDamage, battle.staminaDamage) == 1)
    {
        battle.lastRoleDefeated = static_cast<u16>(target);
        this->FlashFrameWhite();
    }

    battle.slotDamage[0] = 0;
    battle.hitActs.fill(ActState{});
    this->ClearHitDamage();
    return 1;
}

// The sweep of a role's hit in RoleHitFrame (inline in the original): over the enemies (a defeated one widens the
// sweep at once, lowering unk53AE74) or the roles in battle (unk53AE7C) the sweep has reached, each hit once
// (slotTargets) takes its slot totals; true once unk53AE74 are hit.
bool Game::SweepFromRole(BattleRole& attacker, i16 target, u32 act)
{
    BattleState& battle = this->battleState;

    auto hitOne = [this, &battle](BattleUnit& unit, std::size_t slot, bool enemy, i16 aimed)
    {
        battle.slotTargets[slot] = static_cast<i32>(slot);
        battle.hitLanded = 1;
        battle.sweepHit = 1;

        if (battle.unk53AF68 == 0 && unit.Damage(
                                         this->battleServices,
                                         battle.slotDamage[slot],
                                         static_cast<i16>(battle.lastSpiritDamage[slot]),
                                         static_cast<i16>(battle.lastStaminaDamage[slot])
                                     ) == 1)
        {
            ++battle.unk53BD88;

            if (enemy)
            {
                battle.unk4A7626 = static_cast<u16>(aimed);
            }

            battle.unk502984[slot] = static_cast<i32>(slot);
            battle.flashing = 1;
            this->FlashFrameWhite();
        }

        this->ClearHitDamage();
        return ++battle.unk53BD80 >= battle.unk53AE74;
    };

    if (battle.unk53AE7C == 0)
    {
        for (u32 slot = 0; slot < battle.sweptUnits; ++slot)
        {
            BattleEnemy& enemy = this->battleEnemies[slot];

            if (enemy.IsDefeated())
            {
                if (battle.sweptUnits < battle.enemyCount)
                {
                    ++battle.sweptUnits;

                    if (battle.unk53AE74 > 1)
                    {
                        --battle.unk53AE74;
                    }
                }

                battle.pushFrames = 0;
                continue;
            }

            this->StepSweep(battle.enemyCount);

            if (this->HitTargetFrame(enemy, attacker, act, static_cast<int>(slot)) == 1 &&
                battle.slotTargets[slot] == -1 && hitOne(enemy, slot, true, target))
            {
                return true;
            }
        }

        return false;
    }

    for (u32 slot = 0; slot < battle.sweptUnits; ++slot)
    {
        this->StepSweep(battle.roleCount);
        BattleRole& role = this->battleRoles[slot];

        if (role.autoControl != 1 && role.guestControl != 1 &&
            this->HitTargetFrame(role, attacker, act, static_cast<int>(slot)) == 1 && battle.slotTargets[slot] == -1 &&
            hitOne(role, slot, false, target))
        {
            return true;
        }
    }

    return false;
}

// An enemy attacking in HitFrame (inline in the original): its act is enemyHitAct. Without enemySweeps one target is
// hit: a role (enemyHitsAll 1: HitAllFrame on it alone) steps its orb counters by the enemy's command, takes the drain
// and its DrainShare as spirit, a held aimed role ends the menu; an enemy (unk53AE80) takes the hit. With enemySweeps
// the hit sweeps: enemyHitsAll 1 all roles at once, otherwise the enemies (unk53AE80) or the roles the sweep has
// reached, until sweepTargets are hit (counting the defeats in sweepDefeated).
int Game::EnemyHitFrame(int enemy)
{
    BattleState& battle = this->battleState;
    BattleEnemy& attacker = this->battleEnemies[static_cast<std::size_t>(enemy)];
    CounterList& proficiency = this->party.fame.lists[Fame::ProficiencyList];
    const auto target = static_cast<i16>(attacker.GetWord29A2());
    const auto at = static_cast<std::size_t>(target);
    const u32 act = static_cast<u32>(battle.enemyHitAct);

    if (battle.enemySweeps == 0)
    {
        if (battle.enemyHitsAll == 1)
        {
            BattleRole& role = this->battleRoles[at];

            if (this->HitAllFrame(role, attacker, act, 0, 0) != 1)
            {
                this->DrainToSpirit(role, battle.hitDamage);
                return 0;
            }

            role.StepOrbCounters(proficiency, attacker.command);
            battle.unk4FDF7C = attacker.GetWord29A2();
            battle.lastRoleHit = static_cast<u16>(target);
            battle.hitLanded = 1;

            if (battle.unk53AF68 == 0 && role.Damage(this->battleServices, battle.hitDamage, 0, 0) == 1)
            {
                battle.hitDamage = -1;
                battle.lastRoleDefeated = static_cast<u16>(target);
                battle.flashing = 1;
                this->FlashFrameWhite();
            }

            if (role.IsHeldOrHolding() == 1 && target == battle.currentUnit - BattleState::FirstRoleUnit)
            {
                battle.result = 0;
            }

            this->ClearHitDamage();
            battle.hitActs.fill(ActState{});
            return 1;
        }

        if (battle.unk53AE80 != 0)
        {
            BattleEnemy& aimed = this->battleEnemies[at];

            if (this->HitTargetFrame(aimed, attacker, act, 0) != 1)
            {
                return 0;
            }

            battle.unk4FDF7C = attacker.GetWord29A2();
            battle.lastEnemyHit = static_cast<u16>(target);
            battle.hitLanded = 1;

            if (battle.unk53AF68 == 0 && aimed.Damage(this->battleServices, battle.slotDamage[0], 0, 0) == 1)
            {
                battle.unk4A7626 = static_cast<u16>(target);
                this->FlashFrameWhite();
            }

            this->ClearHitDamage();
            battle.slotDamage[0] = 0;
            battle.hitActs.fill(ActState{});
            return 1;
        }

        BattleRole& role = this->battleRoles[at];

        if (this->HitTargetFrame(role, attacker, act, 0) != 1)
        {
            return 0;
        }

        role.StepOrbCounters(proficiency, attacker.command);
        battle.unk4FDF7C = attacker.GetWord29A2();
        battle.lastRoleHit = static_cast<u16>(target);
        battle.hitLanded = 1;

        if (battle.unk53AF68 == 0 &&
            role.Damage(this->battleServices, battle.slotDamage[0], battle.spiritDamage, battle.staminaDamage) == 1)
        {
            battle.flashing = 1;
            battle.lastRoleDefeated = static_cast<u16>(target);
            this->FlashFrameWhite();
        }

        this->ApplyDrain(attacker, role);

        if (role.IsHeldOrHolding() == 1 && target == battle.currentUnit - BattleState::FirstRoleUnit)
        {
            battle.result = 0;
        }

        this->DrainToSpirit(role, battle.slotDamage[0]);
        this->ClearHitDamage();
        battle.slotDamage[0] = 0;
        battle.hitActs.fill(ActState{});
        return 1;
    }

    if (!this->SweepFromEnemy(attacker, target, act))
    {
        return 0;
    }

    battle.unk53BD80 = 0;

    if (battle.enemyHitsAll != 1)
    {
        battle.sweptUnits = 1;
    }

    battle.hitActs.fill(ActState{});
    battle.slotDamage.fill(0);
    return 1;
}

// The sweep of an enemy's hit in EnemyHitFrame (inline in the original); see there. True once sweepTargets are hit.
bool Game::SweepFromEnemy(BattleEnemy& attacker, i16 target, u32 act)
{
    BattleState& battle = this->battleState;
    CounterList& proficiency = this->party.fame.lists[Fame::ProficiencyList];

    auto standing = [](const BattleRole& role)
    {
        return !role.IsDefeated() && role.guestControl != 1 && role.autoControl != 1;
    };

    if (battle.enemyHitsAll == 1)
    {
        if (this->HitAllFrame(this->battleRoles[static_cast<std::size_t>(target)], attacker, act, 0, 1) != 1)
        {
            return false;
        }

        for (u32 slot = 0; slot < battle.roleCount; ++slot)
        {
            BattleRole& role = this->battleRoles[slot];

            if (!standing(role))
            {
                continue;
            }

            role.StepOrbCounters(proficiency, attacker.command);

            if (battle.slotDamage[slot] > 0)
            {
                battle.hitDamage = battle.slotDamage[slot];
            }

            this->ApplyDrain(attacker, role);
            battle.slotTargets[slot] = static_cast<i32>(slot);
            battle.hitLanded = 1;
            battle.sweepHit = 1;

            if (battle.unk53AF68 == 0 && role.Damage(this->battleServices, battle.slotDamage[slot], 0, 0) == 1)
            {
                ++battle.unk53BD88;
                battle.flashing = 1;
                this->FlashFrameWhite();
            }

            this->DrainToSpirit(role, battle.slotDamage[slot]);
            this->ClearHitDamage();
            battle.slotDamage[slot] = 0;

            if (++battle.unk53BD80 == battle.sweepTargets)
            {
                return true;
            }
        }

        return false;
    }

    auto counted = [this, &battle](BattleUnit& unit, std::size_t slot)
    {
        battle.slotTargets[slot] = static_cast<i32>(slot);
        battle.hitLanded = 1;
        battle.sweepHit = 1;

        if (battle.unk53AF68 == 0 && unit.Damage(this->battleServices, battle.slotDamage[slot], 0, 0) == 1)
        {
            ++battle.sweepDefeated;
            battle.flashing = 1;
            this->FlashFrameWhite();
        }
    };

    if (battle.unk53AE80 != 0)
    {
        for (u32 slot = 0; slot < battle.sweptUnits; ++slot)
        {
            this->StepSweep(battle.enemyCount);
            BattleEnemy& enemy = this->battleEnemies[slot];

            if (enemy.IsDefeated() || this->HitTargetFrame(enemy, attacker, act, static_cast<int>(slot)) != 1 ||
                battle.slotTargets[slot] != -1)
            {
                continue;
            }

            counted(enemy, slot);
            this->ClearHitDamage();
            battle.slotDamage[slot] = 0;

            if (++battle.unk53BD80 == battle.sweepTargets)
            {
                return true;
            }
        }

        return false;
    }

    for (u32 slot = 0; slot < battle.sweptUnits; ++slot)
    {
        this->StepSweep(battle.roleCount);
        BattleRole& role = this->battleRoles[slot];

        if (!standing(role) || this->HitTargetFrame(role, attacker, act, static_cast<int>(slot)) != 1 ||
            battle.slotTargets[slot] != -1)
        {
            continue;
        }

        role.StepOrbCounters(proficiency, attacker.command);
        counted(role, slot);
        this->DrainToSpirit(role, battle.slotDamage[slot]);
        this->ClearHitDamage();
        battle.slotDamage[slot] = 0;

        if (++battle.unk53BD80 == battle.sweepTargets)
        {
            return true;
        }
    }

    return false;
}

// swd32.exe 0x00455D60: one frame of the action of enemy `enemy` (its command A) on role `target` (or enemy `target`
// while unk53AE80); 1 once the action is over. 1: the attack (the drain and the damage, a defeat flashing white; on a
// role it goes on with command 0x12C and stays 0); 2: the spell (SpellCast set, the sweep reset, a ripple battle for
// ActFlag13) until SpellOver; 6: the drain of a role out of battle; 7: the role's defeat, with unit `target` + 4
// taken out of the slot records and the menu (kept); 0xA the fade out; 0xB and 0xC the fade in and out (0xC ends the
// battle once every enemy left); 0xF calls unk53BF2A enemies of act unk53BF28 for 0x19 frames; 0x11 the escape
// (the battle ends once every enemy left; a called enemy left alone takes the field again); 100 at once; 200 walks
// to the role; 300 returns home.
// swd32.exe 0x00498370 (EH unwind: the new ripple, freed by its unique_ptr)
// swd32.exe 0x0049837B (EH handler of 0x00455D60)
int Game::EnemyActionFrame(int enemy, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleEnemy& unit = this->battleEnemies[static_cast<std::size_t>(enemy)];
    const auto at = static_cast<std::size_t>(target);

    switch (unit.GetCommandA())
    {
        case kEnemyAttack:
            return this->EnemyAttackAction(unit, target);

        case kEnemySpell:
            if ((battle.spellStage & BattleState::SpellCast) == 0)
            {
                battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellCast);
                battle.sweepGap = unit.GetWordDA6();
                battle.slotTargets.fill(-1);
                battle.sweptUnits = 1;
                battle.slotDamage.fill(0);

                if (unit.ActFlag13() == 1)
                {
                    battle.rippleHits = 1;
                    this->battleRipple =
                        std::make_unique<ScriptObject>(Display::Width, Display::Height, 0, 0, kRippleGrid, kRippleGrid);
                }
            }

            if ((battle.spellStage & BattleState::SpellOver) == 0)
            {
                return 0;
            }

            this->ClearHitDamage();

            if (battle.rippleHits == 1)
            {
                this->battleRipple.reset();
            }

            battle.rippleHits = 0;
            battle.undarkening = 1;
            return 1;

        case kEnemyDrain:
        {
            BattleRole& role = this->battleRoles[at];

            if (battle.pushFrames == 0)
            {
                unit.StartDrainEffect(services, target, role);
                role.StartDefeat(1);
                role.SetGuarding(1);
                battle.RemoveSlotRecord(target + BattleState::FirstRoleUnit);
                battle.darkRed = kDrainDark;
                battle.darkGreen = kDrainDark;
                battle.darkBlue = kDrainDark;
                battle.unk53BF94 = 1;
                this->PrepareDarkenedCopies();
                battle.pushFrames = 1;
                battle.sweptUnits = 1;
            }

            if (unit.DrainEffectFrame(services, target) != 1)
            {
                return 0;
            }

            battle.sweptUnits = 1;
            battle.pushFrames = 0;
            battle.unk53BF94 = 0;
            battle.darkenUnit = -1;
            battle.undarkening = 1;
            return 1;
        }

        case kEnemyDefeatRole:
        {
            BattleRole& role = this->battleRoles[at];

            if (role.DefeatFrame(services) != 1)
            {
                return 0;
            }

            role.SetCommand(0);
            const i32 removed = target + kDefeatedUnitShift;
            battle.RemoveSlotRecord(removed);

            if (target >= 0 && target < static_cast<int>(Party::Roles))
            {
                ++battle.defeatedRoles;
            }

            if (battle.currentUnit == removed)
            {
                battle.currentUnit = 0;
            }

            if (battle.actingUnit == removed)
            {
                battle.actingUnit = -1;
                battle.unk53BF74 = 0;
            }

            return 1;
        }

        case kEnemyFadeOut:
            unit.FadeMainOut(services);
            return 0;

        case kEnemyFadeIn:
            if (unit.DrawFadeIn(services, unit) != 1)
            {
                return 0;
            }

            unit.ClearStatus(BattleUnit::GuardingMark);
            unit.SetGuarding(0);
            unit.SetWord2A0A(kFadedWord);
            battle.enemyFaded = 1;
            return 1;

        case kEnemyHide:
            if (unit.DrawFadeOut(services) != 1)
            {
                return 0;
            }

            unit.SetGuarding(1);
            unit.SetStatus(BattleUnit::GuardingMark);
            unit.SetWord2A0A(kFadedWord);
            battle.enemyFaded = 1;

            if (battle.enemiesGone >= battle.enemyCount)
            {
                battle.autoBattle = 0;
                battle.result = kEndCheckState;
            }

            return 1;

        case kEnemyCall:
            return this->CallEnemies(unit);

        case kEnemyEscape:
            if (unit.RunAway(services) != 1)
            {
                return 0;
            }

            unit.SetGuarding(1);
            unit.SetWord2A0A(kFadedWord);
            unit.StartDefeat(1);
            battle.enemyFaded = 1;
            ++battle.enemiesGone;

            if (battle.enemiesGone >= battle.enemyCount)
            {
                battle.unk524788.fill(BattleSlotRecord{});
                battle.unk53BFC0 = 1;
                battle.autoBattle = 0;
                battle.result = kEndCheckState;
            }

            if (battle.unk53BF28 != 0 && !this->battleEnemies[0].IsDefeated() &&
                static_cast<i32>(battle.enemyCount) - battle.enemiesGone == 1)
            {
                battle.enemyCount = 1;
                battle.enemiesGone = 0;
                battle.unk53BF28 = 0;
                battle.orderSorted = 0;
                this->MeasureUnits();
                battle.SortDrawOrder();
                battle.ListEnemyOrder();
            }

            return 1;

        case kEnemyDone:
            return 1;

        case kEnemyWalk:
            unit.WalkToTarget(services, this->battleRoles[at]);
            return 0;

        case kEnemyReturn:
            if (unit.ReturnHome(services, this->battleRoles[at], -1, 0) == 1)
            {
                battle.darkenUnit = -1;
                return 1;
            }

            return 0;

        default:
            return 0;
    }
}

// Command 1 of EnemyActionFrame (inline in the original); see there.
int Game::EnemyAttackAction(BattleEnemy& unit, int target)
{
    BattleState& battle = this->battleState;
    const auto at = static_cast<std::size_t>(target);

    if (battle.unk53AE80 == 0)
    {
        BattleRole& role = this->battleRoles[at];

        if (this->EnemyAttackFrame(unit, role) == 1)
        {
            battle.hitLanded = 1;
            battle.unk53BFC0 = 0;
            battle.lastRoleHit = static_cast<u16>(target);
            this->ApplyDrain(unit, role);

            if (battle.unk53AF68 == 0 &&
                role.Damage(this->battleServices, battle.hitDamage, battle.spiritDamage, battle.staminaDamage) == 1)
            {
                battle.lastRoleDefeated = static_cast<u16>(target);
                battle.flashing = 1;
                this->FlashFrameWhite();
            }

            this->ClearHitDamage();
            battle.darkenUnit = -1;
            unit.SetCommand(kAfterAttackCommand);
        }

        return 0;
    }

    BattleEnemy& aimed = this->battleEnemies[at];

    if (this->EnemyAttackFrame(unit, aimed) != 1)
    {
        return 0;
    }

    battle.hitLanded = 1;
    battle.lastRoleHit = static_cast<u16>(target);
    battle.unk53BFC0 = 0;

    if (battle.unk53AF68 == 0 && aimed.Damage(this->battleServices, battle.hitDamage, 0, 0) == 1)
    {
        battle.unk502984[at] = target;
        battle.lastRoleDefeated = static_cast<u16>(target);
        battle.flashing = 1;
        this->FlashFrameWhite();
    }

    battle.hitDamage = 0;
    battle.darkenUnit = -1;
    return 1;
}

// Command 0xF of EnemyActionFrame (inline in the original): the first frame (bit kCallStarted clear) places the
// called enemies after the others (at kCallX, kCallY and kSecondCallY for the second, mirrored with the sides),
// loads their act record and sets them up hidden by BattleUnit::UnkMark400; the call is over kCallFrames frames later.
int Game::CallEnemies(BattleEnemy& unit)
{
    BattleState& battle = this->battleState;

    if ((battle.pushFrames & kCallStarted) == 0)
    {
        battle.pushFrames = kCallStarted | kCallFrames;
        unit.CommandValues(battle.unk53BF28, battle.unk53BF2A);

        for (u16 called = 0; called < battle.unk53BF2A; ++called)
        {
            // The original sets up enemies past the last one with the field full.
            if (battle.enemyCount >= this->battleEnemies.size())
            {
                break;
            }

            BattleEnemy& enemy = this->battleEnemies[battle.enemyCount];
            enemy.Reset(this->battleServices);
            battle.unk5246E0 = ObjectRecord{};
            UnitPlacement& place = battle.enemyPlaces[battle.enemyCount];
            place.actId = battle.unk53BF28;
            place.x = kCallX;
            place.y = called == 1 ? kSecondCallY : kCallY;
            place.kind = 0;

            if (battle.mirrored == 1)
            {
                enemy.SetFacing(1);
                place.x = static_cast<i16>(Display::Width - place.x);
            }

            this->database.LoadObject(battle.unk5246E0, battle.unk53BF28);
            this->database.FreeDescription(battle.unk5246E0);
            enemy.Setup(place, battle.unk53BF28, this->database);
            enemy.SetStatus(BattleUnit::UnkMark400);
            ++battle.enemyCount;
            this->MeasureUnits();
            battle.SortDrawOrder();
            battle.ListEnemyOrder();
        }
    }

    --battle.pushFrames;

    if ((battle.pushFrames & kCallCount) != 0)
    {
        return 0;
    }

    battle.darkenUnit = -1;
    battle.pushFrames = 0;
    battle.unk53BF2A = 0;
    return 1;
}

// swd32.exe 0x004576A0: one frame of enemy `enemy`. While the fight talk lets the battle run (unk4A7B58): its effects
// tick and its gauge fills (a full gauge queues its slot record), and unless the battle waits (unk53BF64, unk53BF1C)
// the acting enemy (actingUnit) takes its turn: a held or defeated one ends it; a sweep (gatheringSweep) gathers its
// targets; otherwise it picks a target (EnemyPickTarget) and a command (EnemyChooseCommand), then acts
// (EnemyActionFrame) until the action is over (EnemyAfterAction). Every frame it is drawn at rest (its acts holding
// while the field is darkened with word 2A74, or while paused), its hit act plays and a defeat is taken out; the
// result of RemoveDefeated is returned (ResetTurn's when a held or defeated acting enemy ends its turn).
int Game::EnemyFrame(int enemy)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleEnemy& unit = this->battleEnemies[static_cast<std::size_t>(enemy)];

    if (this->fightTalk.unk4A7B58 == 1)
    {
        if (!unit.IsDefeated() && battle.unk53BFC0 == 0 && battle.unk53BFC4 == 0)
        {
            unit.TickEffects(services);

            if (unit.unk2AB0 == 0 && unit.AdvanceGauge(services, battle.enemyGaugeBoost) == 1 &&
                battle.result != kBattleLostState)
            {
                battle.AddSlotRecord(kEnemySlotRecord, enemy);
            }
        }

        if (battle.unk53BF64 == 0 && battle.unk53BF1C == 0)
        {
            if (battle.actingUnit == enemy)
            {
                if (unit.IsHeldOrHolding() == 1 || unit.IsDefeated())
                {
                    if (unit.IsHeldOrHolding() == 1)
                    {
                        battle.heldEnemy = enemy + 1;
                    }

                    battle.unk53BF5C = 0;
                    battle.unk53BF74 = 0;
                    battle.hitsAllRoles = 0;
                    battle.unk53BFC0 = 0;
                    battle.actingUnit = -1;
                    return unit.ResetTurn(services);
                }

                if (battle.gatheringSweep == 1)
                {
                    this->EnemyGatherSweep(unit);
                }
                else
                {
                    if (battle.enemyTargetPicked == 0)
                    {
                        this->EnemyPickTarget(unit, enemy);
                        battle.enemyTargetPicked = 1;
                    }

                    if (unit.GetValueAB4() == 0)
                    {
                        this->EnemyChooseCommand(unit, enemy);
                    }
                }
            }

            if (unit.GetValueAB4() == 1)
            {
                if (battle.actingUnit == enemy)
                {
                    battle.hitsAllRoles = 1;
                    battle.unk53BFC0 = 1;
                    unit.MarkAC8();
                }

                battle.darkenUnit = static_cast<i16>(enemy);

                if (this->EnemyActionFrame(enemy, static_cast<i16>(unit.GetWord29A2())) == 1)
                {
                    this->EnemyAfterAction(unit, enemy);
                }
                else
                {
                    battle.hitsAllRoles = 1;
                }
            }
        }
    }

    const bool holding =
        (unit.GetWord2A74() != 0 && (battle.unk53BF94 == 1 || battle.unk53BF98 == 1)) || battle.paused == 1;
    unit.IdleFrame(services, holding ? 1 : 0);
    const auto at = static_cast<std::size_t>(enemy);

    if (battle.unk53AE90[at] != -1 && this->PlayHitAct(unit, static_cast<u32>(battle.enemiesHitAct), enemy) == 1)
    {
        battle.unk53AE90[at] = -1;
    }

    const int removed = this->RemoveDefeated(battle.unk502984[at], 0);

    if (removed == 1)
    {
        battle.unk502984[at] = -1;
        battle.slotDamage[at] = 0;
        battle.slotTargets[at] = -1;
        battle.unk4A7626 = kNoAimedEnemy;
        battle.enemyFaded = 1;
        battle.unk53BFDC = 0;
    }

    return removed;
}

// A sweeping enemy gathers its targets in EnemyFrame (inline in the original): an enemy aiming at the enemies sweeps
// them all; otherwise each role standing, not a guest (guestControl), not guarding, not unk2AE0 and with unkAAC clear
// is marked (unmarked AC0, one more hit) while the enemy's unkAB4 is clear; with every role marked the sweep starts at
// the first role standing (none: the panels just close).
void Game::EnemyGatherSweep(BattleEnemy& unit)
{
    BattleState& battle = this->battleState;

    if (battle.unk53BF64 != 0 || battle.hitsAllRoles != 0)
    {
        return;
    }

    if (battle.unk53AE80 != 0)
    {
        battle.enemySweeps = 1;
        unit.Retarget(0);
        battle.sweepTargets = static_cast<i32>(battle.enemyCount) - battle.enemiesGone;
        return;
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];

        if (!role.IsDefeated() && role.guestControl != 1 && !role.IsGuarding() && role.GetAE0() != 1 &&
            role.GetValueAAC() == 0 && unit.GetValueAB4() == 0)
        {
            role.SetAC0(0);
            role.IncreaseHits();
            ++battle.sweepTargets;
        }
    }

    const auto standing =
        static_cast<i32>(battle.roleCount) - battle.escortCount - battle.guestCount - battle.defeatedRoles;

    if (battle.sweepTargets < standing)
    {
        return;
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        if (!this->battleRoles[index].IsDefeated())
        {
            battle.enemySweeps = 1;
            unit.SetAA8();
            unit.Retarget(static_cast<i16>(index));
            break;
        }
    }

    battle.unk53BFC0 = 1;
}

// The target of an acting enemy in EnemyFrame (inline in the original): a role at random (not autoControl, not a guest,
// standing), then ChooseHeal may aim it at itself; an enemy kept from acting (StatusBit5Or8) with other enemies
// standing aims at another enemy at random instead. The original draws for ever when nothing qualifies.
void Game::EnemyPickTarget(BattleEnemy& unit, int enemy)
{
    BattleState& battle = this->battleState;

    auto randomRole = [this, &battle]
    {
        for (int tries = 0; tries < kHangDraws; ++tries)
        {
            if (battle.roleCount != 0)
            {
                battle.enemyTarget = static_cast<i32>(this->random.Below(battle.roleCount));
            }

            const BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.enemyTarget)];

            if (role.autoControl != 1 && role.guestControl != 1 && !role.IsDefeated())
            {
                return;
            }
        }

        Hang();
    };

    if (unit.StatusBit5Or8() == 0)
    {
        randomRole();

        if (unit.ChooseHeal(this->battleServices))
        {
            battle.unk53AE80 = 1;
            battle.enemyTarget = enemy;
        }

        return;
    }

    if (static_cast<i32>(battle.enemyCount) - battle.enemiesGone == 1)
    {
        randomRole();
        return;
    }

    for (int tries = 0;; ++tries)
    {
        if (tries >= kHangDraws)
        {
            Hang();
        }

        if (battle.enemyCount != 0)
        {
            battle.enemyTarget = static_cast<i32>(this->random.Below(battle.enemyCount));
        }

        if (!this->battleEnemies[static_cast<std::size_t>(battle.enemyTarget)].IsDefeated() &&
            battle.enemyTarget != enemy)
        {
            break;
        }
    }

    battle.unk53AE80 = 1;
}

// A unit `index` of the aimed side is down for the target search of EnemyChooseCommand (inline in the original); the
// enemies past the last are read past the enemy array by the original: taken as down.
bool Game::AimedDown(i32 index) const
{
    if (this->battleState.unk53AE80 == 1)
    {
        return index < 0 || static_cast<std::size_t>(index) >= this->battleEnemies.size() ||
               this->battleEnemies[static_cast<std::size_t>(index)].IsDefeated();
    }

    return this->battleRoles[static_cast<std::size_t>(index)].IsDefeated();
}

// The command of an acting enemy in EnemyFrame (inline in the original): a role caught by the catch roll (the first
// party role's level) makes it run (0x11). Without an order from the fight talk (enemyOrders) it chooses its command
// (with the skill name shown); otherwise the order's bits give it: 0x8000 a prepared command, 0x4000 the spell (with
// the name shown), 0x2000 the drain (the order's low byte then is the target, also taken without the bits). ActFlag1
// aims at the party at once, ActFlag0 starts a sweep; otherwise it aims at the target (a downed target passes the aim
// on to the next standing unit, up to unit 9), a role target counting one more hit.
void Game::EnemyChooseCommand(BattleEnemy& unit, int enemy)
{
    BattleState& battle = this->battleState;
    const u16 order = battle.enemyOrders[static_cast<std::size_t>(enemy)];
    battle.gatheringSweep = 0;

    if ((order >> kOrderTargetShift) != 0)
    {
        battle.enemyTarget = order & kOrderTarget;
    }

    const u8 level = this->party.roles[static_cast<std::size_t>(battle.partyRoles[0])].level;

    if (unit.CatchRoll(this->battleServices, level) != 0)
    {
        unit.Retarget(0);
        unit.SetCommand(kRunCommand);
        unit.SetWord2A0A(kRunWord);
        return;
    }

    auto showName = [this, &unit]
    {
        if (unit.skillName[0] != '\0')
        {
            this->battleMessages.Add(kEnemySkillX, 0, kEnemySkillFrames, unit.skillName, BattleMessage::Rise);
        }
    };

    auto aim = [this, &battle, &unit]
    {
        if (battle.unk53AE80 == 1)
        {
            if (!this->AimedDown(battle.enemyTarget))
            {
                unit.Retarget(static_cast<i16>(battle.enemyTarget));
            }

            return;
        }

        if (!this->AimedDown(battle.enemyTarget))
        {
            unit.Retarget(static_cast<i16>(battle.enemyTarget));
            this->battleRoles[static_cast<std::size_t>(battle.enemyTarget)].IncreaseHits();
        }
    };

    if ((order & kOrderPrepared) == 0 && (order & (kOrderSpell | kOrderDrain)) == 0)
    {
        u32 act = static_cast<u32>(battle.enemyHitAct);
        const int chosen = unit.ChooseCommand(this->battleServices, this->database, act);
        battle.enemyHitAct = static_cast<i32>(act);

        if (chosen == 1)
        {
            battle.darkenUnit = static_cast<i16>(enemy);
            battle.unk53BD0C = 0;

            if (unit.ActFlag1() == 1)
            {
                battle.enemyHitsAll = 1;
            }

            showName();
        }

        if (unit.CommandStatusBit() == 1)
        {
            battle.unk53AE80 = 1;
            battle.enemyTarget = enemy;
        }

        battle.darkenUnit = static_cast<i16>(enemy);

        if (unit.ActFlag0() == 1)
        {
            battle.gatheringSweep = 1;
            battle.sweepTargets = 0;
            return;
        }

        aim();
        return;
    }

    if (battle.enemyAimsAtEnemies == 1)
    {
        battle.unk53AE80 = 1;
        battle.enemyAimsAtEnemies = 0;
    }

    if ((order & kOrderPrepared) != 0)
    {
        battle.enemyHitAct = unit.PrepareCommand(this->database);
        battle.darkenUnit = static_cast<i16>(enemy);
    }

    if ((order & kOrderSpell) != 0)
    {
        unit.SetCommand(kEnemySpellCommand);
        battle.darkenUnit = static_cast<i16>(enemy);
        showName();
    }

    if ((order & kOrderDrain) != 0)
    {
        unit.SetCommand(kEnemyDrainCommand);
        battle.darkenUnit = static_cast<i16>(enemy);
    }

    if (unit.ActFlag1() == 1)
    {
        battle.enemyHitsAll = 1;
    }

    if (unit.ActFlag0() == 1)
    {
        battle.gatheringSweep = 1;
        battle.sweepTargets = 0;
    }

    battle.unk53BD48 = 0;

    if (this->AimedDown(battle.enemyTarget))
    {
        while (++battle.enemyTarget <= kLastAimedUnit && this->AimedDown(battle.enemyTarget))
        {
        }
    }

    aim();
}

// A held role whose escort is out gives it back (inline in EnemyAfterAction): the escort record and order cleared,
// the turn and gauge reset and the escort's object back in the inventory.
void Game::ReturnHeldEscort(BattleRole& role, std::size_t index, bool dropCommand)
{
    BattleState& battle = this->battleState;
    u16& escort = battle.escortCalled[index + static_cast<std::size_t>(BattleState::FirstRoleUnit)];

    if (role.IsHeldOrHolding() != 1 || escort == 0)
    {
        return;
    }

    escort = 0;
    battle.callingEscort = 0;
    battle.pendingOrders[index].unk00 = 0;

    if (dropCommand)
    {
        role.CancelCommand();
    }

    role.ResetTurn(this->battleServices);
    role.ResetGauge(this->battleServices);
    const auto place = static_cast<std::size_t>(battle.roleCount - battle.escortCount);
    AddOneItem(this->party.inventory, battle.rolePlaces[place].actId, 0, this->database);
}

// The end of an enemy's action in EnemyFrame (inline in the original): the target loses a hit (a sweep over the roles:
// each role), a held role gives its escort back; the enemy's turn resets, the roles face again after a facing change
// (panel flag 0x80), the turn state clears (the acting unit when an enemy acted), the fight talk's countdown steps,
// and a role target with effect flag 3 strikes back with the last damage (a defeat flashing white).
void Game::EnemyAfterAction(BattleEnemy& unit, int enemy)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    battle.unk53BF9C = 0;
    const auto target = static_cast<i16>(unit.GetWord29A2());
    unit.ClearWord29A2();

    if (unit.GetAA8() == 1)
    {
        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];
            role.DecreaseHits();
            this->ReturnHeldEscort(role, index, true);
        }
    }
    else if (battle.unk53AE80 != 0)
    {
        this->battleEnemies[static_cast<std::size_t>(target)].DecreaseHits();
    }
    else
    {
        BattleRole& role = this->battleRoles[static_cast<std::size_t>(target)];
        this->ReturnHeldEscort(role, static_cast<std::size_t>(target), false);
        role.DecreaseHits();
    }

    unit.ResetTurn(services);

    if ((battle.panelFlags & kFacingPanel) != 0)
    {
        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];

            if (role.guestControl == 0 && !role.IsDefeated())
            {
                role.SetFacingIfAble(0);
            }
        }

        battle.panelFlags &= ~kFacingPanel;
    }

    battle.enemyTargetPicked = 0;
    battle.hitsAllRoles = 0;
    battle.unk53BF74 = 0;
    battle.unk53BFC0 = 0;

    if (battle.actingUnit < BattleState::FirstRoleUnit)
    {
        battle.ClearActing();
        battle.actingUnit = -1;
        battle.unk53AE78 = 0;
    }

    battle.unk53BF46 = 0;
    battle.unk53BF48 = 0;
    battle.enemySweeps = 0;
    battle.enemyHitsAll = 0;
    battle.gatheringSweep = 0;
    battle.sweepTargets = 0;
    battle.spellStage = 0;
    battle.enemyOrders[static_cast<std::size_t>(enemy)] = 0;
    battle.orderSorted = 0;

    if (battle.unk53BF98 == 1)
    {
        battle.undarkening = 1;
    }

    if ((this->fightTalk.unk53CCDC & kTalkCountdown) != 0)
    {
        --this->fightTalk.unk53CCDC;
    }

    if (this->battleRoles[static_cast<std::size_t>(target)].EffectFlag3() != 1)
    {
        return;
    }

    unit.PushTarget(BattleUnit::DamageTarget);
    unit.RaiseFlag26B8();
    unit.SetDamageDigits(services, battle.lastDamage);
    unit.StepTargetCount(1);

    if (unit.Damage(services, battle.lastDamage, 0, 0) == 1)
    {
        battle.unk502984[static_cast<std::size_t>(enemy)] = enemy;
        battle.hitDamage = -1;
        battle.flashing = 1;
        this->FlashFrameWhite();
    }
}
}  // namespace swd3
