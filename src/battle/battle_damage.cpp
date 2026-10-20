#include <string_view>

#include "battle/battle_damage_constants.h"
#include "battle/smooth/smooth_damage.h"
#include "game/game.h"

namespace swd3
{

namespace
{

constexpr std::u8string_view kNoRetreat = u8"無法撤退!!";  // swd32.exe 0x004A7954
constexpr int kRetreatX = 0x118;
constexpr int kRetreatY = 0xA;
constexpr i16 kRetreatFrames = 0x32;
constexpr u16 kNoRetreatSound = 0x8C;

// DrainSpellFrame.

// MagicAttackFrame: a waiting follow-up darkens by kMagicFadeStep a frame down to kMagicFadeEnd.

// EnemyAttackFrame.

}  // namespace

// swd32.exe 0x0045D690: the drain of `attacker`'s command applied after a hit on `target`. A life drain heals the
// attacker by the damage; a spirit or stamina drain takes the damage (at most what the target has) from the target to
// the attacker, shown on both, and records it as the spirit or stamina damage of the hit.
void Game::ApplyDrain(BattleUnit& attacker, BattleUnit& target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    i32 amount = battle.hitDamage;

    if (amount == 0)
    {
        return;
    }

    u16 maximum = 0;
    u16 value = 0;
    const u16 drain = attacker.DrainKind();

    if (drain == BattleUnit::DrainedHitPoints)
    {
        attacker.PushTarget(BattleUnit::HealTarget);
        amount = -amount;
        attacker.SetDamageDigits(services, amount);
        attacker.StepTargetCount(1);
        attacker.Damage(services, amount, 0, 0);
        return;
    }

    if (drain != BattleUnit::DrainedSpirit && drain != BattleUnit::DrainedStamina)
    {
        return;
    }

    if (drain == BattleUnit::DrainedSpirit)
    {
        target.GetSpirit(maximum, value);
    }
    else
    {
        target.GetStamina(maximum, value);
    }

    if (static_cast<i16>(value) - amount <= 0)
    {
        amount = static_cast<i16>(value);
        target.SetDamageDigits(services, amount);
    }

    target.PushTarget(BattleUnit::DamageTarget);
    target.StepTargetCount(1);
    attacker.PushTarget(
        drain == BattleUnit::DrainedSpirit ? BattleUnit::SpiritGainTarget : BattleUnit::StaminaGainTarget
    );
    attacker.SetDamageDigits(services, amount);
    attacker.StepTargetCount(1);

    if (drain == BattleUnit::DrainedSpirit)
    {
        attacker.Damage(services, 0, static_cast<i16>(amount), 0);
        battle.spiritDamage = static_cast<i16>(-amount);
    }
    else
    {
        attacker.Damage(services, 0, 0, static_cast<i16>(amount));
        battle.staminaDamage = static_cast<i16>(-amount);
    }

    battle.hitDamage = 0;
}

// swd32.exe 0x0045EA80: the retreat command of role `role`. When the role can act, a party whose first role is held
// or whose battle forbids it gets the "無法撤退!!" message; otherwise the turn state is cleared for the retreat.
void Game::TryRetreat(int role)
{
    BattleState& battle = this->battleState;

    if (this->battleRoles[static_cast<std::size_t>(role)].ActFlag11() != 1)
    {
        return;
    }

    if (!this->battleRoles[0].MarkBit11Clear() || (battle.panelFlags & BattleState::NoRetreat) != 0)
    {
        this->battleMessages
            .Add(kRetreatX, kRetreatY, kRetreatFrames, kNoRetreat, BattleMessage::BlackBar | BattleMessage::Centered);
        this->audio.PlaySound(kNoRetreatSound, this->config.soundVolume);
        return;
    }

    battle.unk53BFC0 = 1;
    battle.unk53BFC4 = 1;
    battle.unk53BF5C = 0;
    battle.undarkening = 0;
    battle.turnUnit = 0;
    battle.currentUnit = 0;
    battle.actingUnit = -1;
    battle.result = 0;
    battle.enemiesGone = static_cast<u8>(battle.enemyCount);
    battle.endFade = 1;
}

// swd32.exe 0x004731A0: one frame of `role`'s drain spell (its magic act, slot act 8) at its place over its shadow;
// the act's colour darkens the field. Once the act strikes or follows up, the cast act flies from the target back
// into the role (one more step each frame); when it arrives the hit lands on the target (at most kMaxDamage) and the
// role regains it: the negated damage shows as healing and the hit's damage as a stamina gain 8 frames later, and the
// role takes both. 1 (both acts cleared) then.
int Game::DrainSpellFrame(BattleRole& role, BattleUnit& target)
{
    smooth::Scene scene;
    const auto result = smooth::DamageFrame(*this).DrainSpellFrameAdvance(role, target, scene);
    scene.Show(this->battleServices);
    return result;
}

// swd32.exe 0x004745B0: one frame of `role`'s magic attack (its magic act in slot act 8, drawn by DrawAction; the
// act's colour darkens the field); 1 once it is over. The magic act's target bits play the effect in the recoil act
// (holding the act), quake, raise flag 26B8 of the target, fade the field, start the follow-up (the cast act or the
// effect, slot act 0) or strike. The follow-up flies to the target (TargetsUnit; the original takes the start from
// the stand act and marks the stand act waiting) or is placed on it, and may quake, raise flag 26B8 or strike; once
// it waits it darkens by 8 a frame (not while held) before the acts, the trails and the targets are cleared.
int Game::MagicAttackFrame(BattleRole& role, BattleUnit& target)
{
    smooth::Scene scene;
    const auto result = smooth::DamageFrame(*this).MagicAttackFrameAdvance(role, target, scene);
    scene.Show(this->battleServices);
    return result;
}

// swd32.exe 0x004758A0: one frame of `enemy`'s attack on `target`; 1 once it is over (its gauge then gains 10..129).
// An UnkFlag8 command fades the enemy out and in at the target first. The attack act (by unk2A8C or `action`) is
// drawn; its target bits play the effect in the recoil act, raise flag 26B8 of the target, fade the field, start the
// follow-up (the cast act), quake or strike; the act's colour darkens the field. The follow-up flies to the target
// (TargetsUnit) or is placed on it and may strike; once it waits it darkens by 4 a frame (the original takes the
// palette from the stand picture) before the acts, the trails and the targets are cleared.
int Game::EnemyAttackFrame(BattleUnit& enemy, BattleUnit& target)
{
    smooth::Scene scene;
    const auto result = smooth::DamageFrame(*this).EnemyAttackFrameAdvance(enemy, target, scene);
    scene.Show(this->battleServices);
    return result;
}

// A strike of EnemyAttackFrame (inline twice in the original): unless the enemy has struck this turn the target may
// evade by `chance`; the Strikes bit raises its flag 26B8; a hit's damage (at most kSpellMaxDamage) is counted and
// shown on the target, or counted and shown on the enemy when the target reflects it (`attackFirst`: the attack act's
// strike marks the enemy before it pushes the number). A hit that does not drain counts on the target. `chance` keeps
// the drain kind of the attack act's strike (as in the original).
void Game::EnemyStrike(BattleUnit& enemy, BattleUnit& target, u16 bits, u16& chance, bool attackFirst)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    battle.lastDamage = 0;
    int evaded = 0;

    if (enemy.evadeRolled == 0)
    {
        evaded = target.RollEvade(services, chance);
    }

    if ((bits & ActState::Strikes) != 0)
    {
        target.RaiseFlag26B8();
    }

    if (evaded != 0)
    {
        return;
    }

    i16 damage = enemy.EnemyHitDamage(services, target);
    battle.lastDamage = damage;

    if (damage >= kSpellMaxDamage)
    {
        damage = static_cast<i16>(kSpellMaxDamage);
        battle.lastDamage = kSpellMaxDamage;
    }

    battle.hitDamage += damage;

    if (target.EffectFlag3() == 1)
    {
        if (attackFirst)
        {
            enemy.RaiseFlag26B8();
            enemy.PushTarget(BattleUnit::DamageTarget);
        }
        else
        {
            enemy.PushTarget(BattleUnit::DamageTarget);
            enemy.RaiseFlag26B8();
        }

        enemy.SetDamageDigits(services, battle.lastDamage);
        // 0x00475C03 / 0x00475E41: the reflected hit counts on the enemy (ecx = esi)
        enemy.StepTargetCount(1);
        return;
    }

    if (battle.lastDamage == -1)
    {
        return;
    }

    target.SetDamageDigits(services, battle.lastDamage);
    const u16 kind = enemy.DrainKind();

    if (attackFirst)
    {
        chance = kind;
    }

    if (kind <= BattleUnit::DrainedHitPoints)
    {
        target.StepTargetCount(1);
    }
}
}  // namespace swd3
