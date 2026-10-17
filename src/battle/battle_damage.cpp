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

// DrainSpellFrame.
constexpr int kSpellShadowShare = 3;
constexpr std::size_t kSpellShadowShearShift = 2;
constexpr int kSpellShadowTint = -6;
constexpr int kFaintSpellShadowTint = -1;
constexpr int kSpellShadowLeft = 5;
constexpr int kSpellPanCentre = 0x140;
constexpr int kSpellPan = 0x10;
constexpr i32 kSpellMaxDamage = 0x270F;
constexpr u16 kDrainGainDelay = 8;  // the stamina gain shows 8 frames after the healing

// MagicAttackFrame: a waiting follow-up darkens by kMagicFadeStep a frame down to kMagicFadeEnd.
constexpr i16 kMagicFadeEnd = -0x20;
constexpr i16 kMagicFadeStep = 8;

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

// swd32.exe 0x004731A0: one frame of `role`'s drain spell (its magic act, slot act 8) at its place over its shadow;
// the act's colour darkens the field. Once the act strikes or follows up, the cast act flies from the target back
// into the role (one more step each frame); when it arrives the hit lands on the target (at most kMaxDamage) and the
// role regains it: the negated damage shows as healing and the hit's damage as a stamina gain 8 frames later, and the
// role takes both. 1 (both acts cleared) then.
int Game::DrainSpellFrame(BattleRole& role, BattleUnit& target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    Painter& painter = this->painter;
    ActState& act = role.slotActs[BattleUnit::UnkSlot8];
    role.unk2AAC = 1;
    act.id = role.actId + BattleRole::MagicActs;
    act.action = role.unk2A8A;

    if (this->acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = this->archive.GetPicture(act.frame, act.picture);

    // A missing picture is read through a null pointer in the original.
    if (role.picture == nullptr)
    {
        return 0;
    }

    const Picture& picture = *role.picture;
    painter.SetSource(picture.data.data());
    role.actOffsetX = static_cast<u16>(act.x);
    u32 flags = act.flags;
    role.actAnchorX = act.anchorX;

    if (role.facing == 1)
    {
        flags ^= Painter::Mirror;
        role.actOffsetX = static_cast<u16>(picture.width - static_cast<u16>(act.x));

        if (act.anchorX != 0)
        {
            role.actAnchorX = static_cast<u16>(picture.width - act.anchorX);
        }
    }

    const int shadowHeight = picture.height / kSpellShadowShare;
    DrawOptions& options = painter.Options();
    options.scaleHeight = shadowHeight;
    options.shear = picture.height >> kSpellShadowShearShift;
    const int tint = role.unk2B00 == 1 ? kFaintSpellShadowTint : kSpellShadowTint;
    options.tintRed = tint;
    options.tintGreen = tint;
    options.tintBlue = tint;
    this->audio.PlaySound(act.view, this->config.soundVolume);
    const int x = role.placement.x - static_cast<i16>(role.actOffsetX);
    this->audio.SetSoundPan(act.view, x >= kSpellPanCentre ? kSpellPan : -kSpellPan);
    act.view = 0;
    role.unk26A4 = (flags & (Painter::RleSource | Painter::Mirror | Painter::Flip)) | Painter::Shadow;
    painter.DrawPicture(
        x - kSpellShadowLeft,
        role.placement.y - shadowHeight,
        picture.width,
        picture.height,
        role.unk26A4
    );
    painter.DrawPicture(x, role.placement.y - act.y, picture.width, picture.height, flags);
    battle.darkRed = static_cast<i16>(act.red);
    battle.darkGreen = static_cast<i16>(act.green);
    battle.darkBlue = static_cast<i16>(act.blue);
    this->PrepareDarkenedCopies();

    if (battle.darkRed != 0 || battle.darkGreen != 0 || battle.darkBlue != 0)
    {
        battle.unk53BF94 = 1;
    }

    if ((act.target & (ActState::Strikes | ActState::FollowsUp)) == 0)
    {
        return 0;
    }

    u16 targetX = 0;
    u16 targetY = 0;
    target.GetDrawPosition(targetX, targetY);
    ++role.absorbFrames;
    const int fromX = static_cast<i16>(role.actAnchorX) + x;
    const int fromY = role.placement.y + static_cast<i16>(act.anchorY) - act.y;

    if (role.FlyAct(
            services,
            target,
            role.slotActs[0],
            0,
            role.command.castAct,
            fromX,
            fromY,
            static_cast<i16>(role.command.target),
            1
        ) != 1)
    {
        return 0;
    }

    target.RaiseFlag26B8();
    i16 spirit = 0;
    i16 stamina = 0;
    i32 damage = role.HitDamage(services, target, spirit, stamina);
    battle.lastDamage = damage;

    if (damage >= kSpellMaxDamage)
    {
        damage = kSpellMaxDamage;
        battle.lastDamage = damage;
    }

    battle.hitDamage += damage;
    target.SetDamageDigits(services, damage);
    target.StepTargetCount(1);
    battle.lastDamage = -battle.lastDamage;
    role.PushTarget(BattleUnit::HealTarget);
    role.SetDamageDigits(services, battle.lastDamage);
    role.PushTargetAlt(0);
    role.StepTargetCount(1);
    role.PushTarget(BattleUnit::StaminaGainTarget);
    role.SetDamageDigits(services, battle.hitDamage);
    role.PushTargetAlt(kDrainGainDelay);
    role.StepTargetCount(1);
    role.Damage(services, battle.lastDamage, 0, static_cast<i16>(battle.hitDamage));
    role.absorbFrames = 0;
    role.slotActs[0] = ActState{};
    act = ActState{};
    return 1;
}

// swd32.exe 0x004745B0: one frame of `role`'s magic attack (its magic act in slot act 8, drawn by DrawAction; the
// act's colour darkens the field); 1 once it is over. The magic act's target bits play the effect in the recoil act
// (holding the act), quake, raise flag 26B8 of the target, fade the field, start the follow-up (the cast act or the
// effect, slot act 0) or strike. The follow-up flies to the target (TargetsUnit; the original takes the start from
// the stand act and marks the stand act waiting) or is placed on it, and may quake, raise flag 26B8 or strike; once
// it waits it darkens by 8 a frame (not while held) before the acts, the trails and the targets are cleared.
int Game::MagicAttackFrame(BattleRole& role, BattleUnit& target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    Painter& painter = this->painter;

    if (role.unk2A74 != 0 || role.unk2AD8 == 1)
    {
        return 0;
    }

    ActState& magic = role.slotActs[BattleUnit::UnkSlot8];
    role.unk2AAC = 1;
    magic.hold = 0;

    if ((magic.target & ActState::Holds) != 0)
    {
        magic.hold = 1;
    }

    magic.id = role.actId + BattleRole::MagicActs;
    magic.action = role.unk2A8A;
    role.DrawAction(services, target, magic);

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

        magic.target = static_cast<u16>(magic.target & ~ActState::Recoils);
        magic.effectA = 0;
        magic.effectQ = 0;
    }

    if ((role.unk267C & BattleUnit::Recoiling) != 0 &&
        role.DrawRecoil(services, role.recoilAct, static_cast<i16>(magic.anchorY)) == 1)
    {
        magic.target = 0;
        magic.hold = 0;
        role.unk267C &= ~BattleUnit::Recoiling;
    }

    if ((magic.target & ActState::Quakes) != 0)
    {
        battle.quaking = 1;
        battle.quakePhase = 0;
    }

    if ((magic.target & ActState::UnkTarget4) != 0)
    {
        magic.target = 0;
        role.unk267C |= BattleUnit::FollowingUp;
        target.RaiseFlag26B8();
    }

    ActState& follow = role.slotActs[0];

    if ((magic.target & ActState::FollowsUp) != 0)
    {
        if ((magic.target & ActState::FadesField) != 0)
        {
            battle.fadingField = 1;
            battle.StartFieldFade(
                static_cast<i16>(magic.lights[0]),
                static_cast<i16>(magic.lights[1]),
                static_cast<i16>(magic.lights[2]),
                static_cast<i16>(magic.lights[3]),
                static_cast<i16>(magic.lights[4]),
                static_cast<i16>(magic.lights[5]),
                static_cast<i16>(magic.lights[6])
            );
            magic.target = static_cast<u16>(magic.target & ~ActState::FadesField);
        }

        magic.target = static_cast<u16>(magic.target & ~ActState::FollowsUp);
        role.unk267C |= BattleUnit::FollowingUp;
        role.slotTint = 0;
        magic.target = 0;
        follow = ActState{};
    }

    if ((magic.target & ActState::Strikes) != 0)
    {
        role.unk267C |= BattleUnit::FollowingUp;
        role.slotTint = 0;
        magic.target = 0;
        role.Strike(services, target, 0);
        follow = ActState{};
    }

    battle.darkRed = static_cast<i16>(magic.red);
    battle.darkGreen = static_cast<i16>(magic.green);
    battle.darkBlue = static_cast<i16>(magic.blue);
    this->PrepareDarkenedCopies();

    if (battle.darkRed != 0 || battle.darkGreen != 0 || battle.darkBlue != 0)
    {
        battle.unk53BF94 = 1;
    }

    if ((role.unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    follow.action = 0;
    follow.id = role.command.castAct;

    if (magic.effectA != 0)
    {
        follow.id = magic.effectA;
        follow.action = magic.effectQ;
    }

    if ((role.command.flags & UnitCommand::TargetsUnit) != 0)
    {
        const int fromX = role.placement.x + static_cast<i16>(role.actAnchorX) - static_cast<i16>(role.actOffsetX);
        const int fromY = static_cast<i16>(role.standAct.anchorY) + role.placement.y - role.standAct.y;

        if (role.FlyAct(
                services,
                target,
                follow,
                0,
                follow.id,
                fromX,
                fromY,
                static_cast<i16>(role.command.target),
                0
            ) != 1)
        {
            return 0;
        }

        follow.target |= ActState::Strikes;
        role.standAct.waiting = 1;
        follow.waiting = 1;
        role.slotTint = 0;
    }
    else
    {
        role.AimAction(services, target, follow, static_cast<i16>(magic.anchorX), static_cast<i16>(magic.anchorY));
    }

    if ((follow.target & ActState::Quakes) != 0)
    {
        battle.quaking = 1;
        battle.quakePhase = 0;
    }

    if ((follow.target & ActState::UnkTarget4) != 0)
    {
        target.RaiseFlag26B8();
        follow.target = 0;
    }

    if ((follow.target & ActState::Strikes) != 0)
    {
        role.slotTint = 0;
        battle.proficiencyBonus = 0;
        follow.target = 0;
        role.Strike(services, target, 0);
    }

    // The original reads the current or the shadow picture through a null pointer when there is none.
    if (role.picture != nullptr)
    {
        painter.SetSource(role.picture->data.data());
    }

    if (follow.waiting != 1)
    {
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

    if ((role.unk267C & BattleUnit::Recoiling) != 0)
    {
        return 0;
    }

    magic.effectA = 0;
    role.attackAct.effectA = 0;

    if ((role.command.flags & UnitCommand::TargetsUnit) == 0)
    {
        if (role.slotTint > kMagicFadeEnd)
        {
            DrawOptions& options = painter.Options();
            options.tintRed = role.slotTint;
            options.tintGreen = role.slotTint;
            options.tintBlue = role.slotTint;

            if (role.picture != nullptr)
            {
                painter.DrawPicture(
                    static_cast<i16>(role.aimX),
                    static_cast<i16>(role.aimY),
                    role.picture->width,
                    role.picture->height,
                    role.unk26A4,
                    PaletteArgument(*role.picture)
                );
            }

            role.slotTint = static_cast<i16>(role.slotTint - kMagicFadeStep);

            if (role.held == 1)
            {
                role.slotTint = static_cast<i16>(role.slotTint + kMagicFadeStep);
            }

            return 0;
        }

        role.slotTint = 0;
    }

    if (magic.waiting != 1 || role.slotTint != 0)
    {
        return 0;
    }

    magic = ActState{};
    role.attackAct = ActState{};
    role.recoilAct = ActState{};
    follow = ActState{};
    role.trails = {};
    role.targets.fill(BattleUnit::NoTarget);
    role.slotTint = 0;
    role.unk2AB0 = 0;
    role.unk267C = 0;
    role.unk26D4 = 0;
    battle.criticalAttack = 0;
    ++battle.attacksSinceCritical;
    return 1;
}
}  // namespace swd3
