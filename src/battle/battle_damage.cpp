#include <string_view>

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
    battle.unk53BFEC = 0;
    battle.unk53BD50 = 0;
    battle.currentUnit = 0;
    battle.unk53AE70 = -1;
    battle.result = 0;
    battle.unk53BF00 = static_cast<u8>(battle.enemyCount);
    battle.unk53BFE4 = 1;
}
}  // namespace swd3
