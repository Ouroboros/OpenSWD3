#include "battle/smooth/smooth_damage.h"

#include <string_view>

#include "battle/battle_damage_constants.h"
#include "battle/smooth/smooth_unit.h"
#include "game/game.h"

namespace swd3::smooth
{

namespace
{

constexpr int kSpellShadowShare = 3;
constexpr std::size_t kSpellShadowShearShift = 2;
constexpr int kSpellShadowTint = -6;
constexpr int kFaintSpellShadowTint = -1;
constexpr int kSpellShadowLeft = 5;
constexpr int kSpellPanCentre = 0x140;
constexpr int kSpellPan = 0x10;
constexpr u16 kDrainGainDelay = 8;  // the stamina gain shows 8 frames after the healing
constexpr i32 kEnemyAttackAction = 0x28;
constexpr i32 kEnemyAttackAction1 = 0x30;  // unk2A8C 1
constexpr i32 kEnemyAttackAction2 = 0x31;  // unk2A8C 2
constexpr u16 kSecondEnemyAttack = 2;
constexpr std::size_t kEnemyFlightAct = 1;  // the slot act a flying follow-up uses
constexpr u16 kEnemyStrikeBits = ActState::Strikes | ActState::UnkTarget10;
constexpr u16 kStatusEvade = 8;  // UnkStatus80: the target evades 8 times in 10
constexpr i16 kEnemyFadeEnd = -0x20;
constexpr i16 kEnemyFadeStep = 4;
constexpr u32 kGaugeRestRoll = 0x78;  // the gauge gains 10..129 once the attack is over
constexpr u16 kGaugeRestBase = 10;
}  // namespace

DamageFrame::DamageFrame(Game& game)
    : game(game)
{
}

// What Game::DrainSpellFrame decides of the frame; the draws into `scene`.
int DamageFrame::DrainSpellFrameAdvance(BattleRole& role, BattleUnit& target, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    BattleServices& services = game.battleServices;
    ScenePainter painter(game.battleServices, scene);
    ActState& act = role.slotActs[BattleUnit::UnkSlot8];
    role.unk2AAC = 1;
    act.id = role.actId + BattleRole::MagicActs;
    act.action = role.unk2A8A;

    if (game.acts.Run(act) == 0)
    {
        return 0;
    }

    role.picture = game.archive.GetPicture(act.frame, act.picture);

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
    game.audio.PlaySound(act.view, game.config.soundVolume);
    const int x = role.placement.x - static_cast<i16>(role.actOffsetX);
    game.audio.SetSoundPan(act.view, x >= kSpellPanCentre ? kSpellPan : -kSpellPan);
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
    game.PrepareDarkenedCopies();

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

    if (UnitFrame(role).Fly(
            services,
            target,
            role.slotActs[0],
            0,
            role.command.castAct,
            fromX,
            fromY,
            static_cast<i16>(role.command.target),
            1,
            scene
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

// What Game::MagicAttackFrame decides of the frame; the draws into `scene`.
int DamageFrame::MagicAttackFrameAdvance(BattleRole& role, BattleUnit& target, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    BattleServices& services = game.battleServices;
    ScenePainter painter(game.battleServices, scene);

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

        magic.target = static_cast<u16>(magic.target & ~ActState::Recoils);
        magic.effectA = 0;
        magic.effectQ = 0;
    }

    if ((role.unk267C & BattleUnit::Recoiling) != 0 &&
        UnitFrame(role).Recoil(services, role.recoilAct, static_cast<i16>(magic.anchorY), scene) == 1)
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
    game.PrepareDarkenedCopies();

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

        if (UnitFrame(role).Fly(
                services,
                target,
                follow,
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

// What Game::EnemyAttackFrame decides of the frame; the draws into `scene`.
int DamageFrame::EnemyAttackFrameAdvance(BattleUnit& enemy, BattleUnit& target, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    BattleServices& services = game.battleServices;
    ScenePainter painter(game.battleServices, scene);
    u16 evadeChance = 0;

    if (enemy.unk2A74 != 0)
    {
        return 0;
    }

    ActState& attack = enemy.standAct;
    enemy.unk2AAC = 1;
    attack.hold = 0;

    if (enemy.held == 1)
    {
        attack.hold = 1;
    }

    if ((attack.target & ActState::Holds) != 0)
    {
        attack.hold = 1;
    }

    if ((enemy.command.flags & UnitCommand::UnkFlag8) != 0 && enemy.fadedIn == 0)
    {
        if (UnitFrame(enemy).DrawFadeOutAdvance(services, scene) == 0)
        {
            return 0;
        }

        const int result = UnitFrame(enemy).DrawFadeInAdvance(services, target, scene);

        if (result == 0)
        {
            enemy.fadedIn = 1;
            return result;
        }
    }

    attack.id = enemy.actId;
    attack.action = kEnemyAttackAction;

    if (enemy.unk2A8C != 0)
    {
        attack.action = enemy.unk2A8C == kSecondEnemyAttack ? kEnemyAttackAction2 : kEnemyAttackAction1;
    }

    if (enemy.action != 0)
    {
        attack.action = enemy.action;
    }

    UnitFrame(enemy).Action(services, target, attack, scene);

    if ((attack.target & ActState::Recoils) != 0)
    {
        if (attack.effectA != 0)
        {
            enemy.recoilAct = ActState{};
            enemy.unk267C |= BattleUnit::Recoiling;
            enemy.recoilAct.id = attack.effectA;
            enemy.recoilAct.action = attack.effectQ;
        }

        if ((attack.target & ActState::Holds) != 0)
        {
            attack.hold = 1;
        }

        attack.target = static_cast<u16>(attack.target & ~ActState::Recoils);
        attack.effectA = 0;
        attack.effectQ = 0;
    }

    if ((enemy.unk267C & BattleUnit::Recoiling) != 0 &&
        UnitFrame(enemy).Recoil(services, enemy.recoilAct, static_cast<i16>(attack.anchorY), scene) == 1)
    {
        attack.target = 0;
        attack.hold = 0;
        enemy.unk267C &= ~BattleUnit::Recoiling;
    }

    ActState& follow = enemy.castAct;

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
        enemy.unk267C |= BattleUnit::FollowingUp;
        follow = ActState{};
    }

    if ((attack.target & ActState::Quakes) != 0)
    {
        battle.quaking = 1;
        battle.quakePhase = 0;
    }

    if ((attack.target & ActState::UnkTarget4) != 0)
    {
        enemy.unk267C |= BattleUnit::FollowingUp;
        attack.target = 0;
        target.RaiseFlag26B8();
    }

    if ((attack.target & kEnemyStrikeBits) != 0)
    {
        enemy.slotTint = 0;
        enemy.unk267C |= BattleUnit::FollowingUp;
        follow = ActState{};
        evadeChance = (enemy.status & BattleUnit::UnkStatus80) != 0 ? kStatusEvade : 0;
        game.EnemyStrike(enemy, target, attack.target, evadeChance, true);
        attack.target = 0;
        enemy.unk2B14 = 1;
    }

    battle.darkRed = static_cast<i16>(attack.red);
    battle.darkGreen = static_cast<i16>(attack.green);
    battle.darkBlue = static_cast<i16>(attack.blue);
    game.PrepareDarkenedCopies();

    if (battle.darkRed != 0 || battle.darkGreen != 0 || battle.darkBlue != 0)
    {
        battle.unk53BF98 = 1;
    }

    if ((enemy.unk267C & BattleUnit::FollowingUp) == 0)
    {
        return 0;
    }

    follow.action = 0;
    follow.id = enemy.command.castAct;

    if (attack.effectA != 0)
    {
        follow.id = attack.effectA;
    }

    if (follow.id != 0)
    {
        if ((enemy.command.flags & UnitCommand::TargetsUnit) != 0)
        {
            const int fromX =
                enemy.placement.x + static_cast<i16>(enemy.actAnchorX) - static_cast<i16>(enemy.actOffsetX);
            const int fromY = static_cast<i16>(attack.anchorY) + enemy.placement.y - attack.y;

            if (UnitFrame(enemy).Fly(
                    services,
                    target,
                    enemy.slotActs[kEnemyFlightAct],
                    0,
                    follow.id,
                    fromX,
                    fromY,
                    static_cast<i16>(enemy.command.target),
                    0,
                    scene
                ) != 1)
            {
                return 0;
            }

            follow.target |= ActState::Strikes;
            attack.waiting = 1;
            follow.waiting = 1;
        }
        else
        {
            enemy.AimAction(
                services,
                target,
                follow,
                static_cast<i16>(attack.anchorX),
                static_cast<i16>(attack.anchorY)
            );
        }

        if ((follow.target & kEnemyStrikeBits) != 0)
        {
            enemy.slotTint = 0;
            u16 chance = (enemy.status & BattleUnit::UnkStatus80) != 0 ? kStatusEvade : evadeChance;
            game.EnemyStrike(enemy, target, follow.target, chance, false);
            enemy.unk2B14 = 1;
            follow.target = 0;
        }
    }
    else
    {
        follow.waiting = 1;
    }

    // A missing picture is read through a null pointer in the original; it is not drawn here.
    const Picture* picture = enemy.picture;

    if (picture != nullptr)
    {
        painter.SetSource(picture->data.data());
    }

    if (follow.waiting != 1)
    {
        if (picture != nullptr)
        {
            painter.DrawPicture(
                static_cast<i16>(enemy.aimX),
                static_cast<i16>(enemy.aimY),
                picture->width,
                picture->height,
                enemy.unk26A4,
                PaletteArgument(*picture)
            );
        }

        return 0;
    }

    if (follow.id != 0 && (enemy.command.flags & UnitCommand::TargetsUnit) == 0 && enemy.slotTint > kEnemyFadeEnd)
    {
        DrawOptions& options = painter.Options();
        options.tintRed = enemy.slotTint;
        options.tintGreen = enemy.slotTint;
        options.tintBlue = enemy.slotTint;

        if ((enemy.unk26A4 & ActState::NoBlend) != 0)
        {
            options.tintRed = 0;
            options.tintGreen = 0;
            options.tintBlue = 0;
            enemy.slotTint = kEnemyFadeEnd;
        }

        if (picture != nullptr && enemy.shadowPicture != nullptr)
        {
            painter.DrawPicture(
                static_cast<i16>(enemy.aimX),
                static_cast<i16>(enemy.aimY),
                picture->width,
                picture->height,
                enemy.unk26A4,
                PaletteArgument(*enemy.shadowPicture)
            );
        }

        enemy.slotTint = static_cast<i16>(enemy.slotTint - kEnemyFadeStep);

        if (enemy.held == 1)
        {
            enemy.slotTint = static_cast<i16>(enemy.slotTint + kEnemyFadeStep);
        }

        return 0;
    }

    if (attack.waiting != 1)
    {
        return 0;
    }

    enemy.standAct = ActState{};
    follow = ActState{};
    enemy.recoilAct = ActState{};
    enemy.attackAct = ActState{};
    enemy.slotActs[kEnemyFlightAct] = ActState{};
    enemy.trails = {};
    enemy.targets.fill(BattleUnit::NoTarget);
    enemy.unk267C = 0;
    enemy.unk2AB0 = 0;
    enemy.fadedIn = 0;
    enemy.nudge = 0;
    enemy.gauge = static_cast<u16>(enemy.gauge + services.random.Below(kGaugeRestRoll) + kGaugeRestBase);
    return 1;
}

}  // namespace swd3::smooth
