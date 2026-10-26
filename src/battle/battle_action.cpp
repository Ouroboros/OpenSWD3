#include "game/game.h"

#include <algorithm>
#include <type_traits>

#include "core/format.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr u16 kReleaseControl = 0x63;  // RoleActionFrame: the command A of the acting role
constexpr u16 kWalkCommand = 0xC8;
constexpr u16 kDoneCommand = 0x64;
constexpr u16 kReturnCommand = 0x12C;
constexpr u16 kChargeCommand = 0x190;
constexpr u16 kSwarmCommand = 0x192;
constexpr u16 kComboCommand = 0x194;
constexpr u16 kDrainSpellCommand = 0x195;
constexpr u16 kBeamCommand = 0x196;
constexpr u16 kLeapCommand = 0x199;
constexpr u16 kMagicCommand = 0x1F4;
constexpr int kAttackCommand = 1;  // RoleActionFrame: the command B
constexpr int kSpellCommand = 2;
constexpr int kItemCommand = 3;
constexpr int kMagicAttackCommand = 4;
constexpr int kGuardCommand = 5;
constexpr int kAbsorbCommand = 6;
constexpr int kDefeatCommand = 7;
constexpr int kFadeInCommand = 0xB;
constexpr int kFadeOutCommand = 0xC;
constexpr int kThrowCommand = 0xD;
constexpr int kCatchCommand = 0xE;
constexpr int kEscortCommand = 0xF;
constexpr int kLeaveCommand = 0x11;
constexpr int kCoinCommand = 0x16;
constexpr int kStealCommand = 0x17;
constexpr int kPoseCommand = 0x18;
constexpr int kControlCommand = 0x19;
constexpr int kTimingCommand = 0x1A;
constexpr int kDirectCommand = 0x1B;
constexpr int kHideCommand = 0x1C;
constexpr int kSlowCommand = 0x1D;
constexpr int kTimerCommand = 0x1F;
constexpr int kBarrierCommand = 0x20;
constexpr int kKillCommand = 0x21;
constexpr int kSelfDamageCommand = 0x22;
constexpr int kSelfSpiritCommand = 0x23;
constexpr int kSelfStaminaCommand = 0x24;
constexpr int kAfterAction = 0x12C;
constexpr std::size_t kSpecialPoints = 1;  // RoleBattleBlock::unk00 [1] (kBattlesFought of battle_turn.cpp)
constexpr i32 kHitPoints = 2;
constexpr i32 kDefeatPoints = 5;
constexpr i16 kDarkTint = -12;
constexpr i32 kEndCheckState = 0x63;
constexpr i32 kCraftState = 0x62;
constexpr int kSlotRecordEnemy = 2;
constexpr u16 kVampireDrain = 8;  // BattleUnit::DrainKind
constexpr u16 kVampireRing = 0x38;
constexpr u16 kVampireStep = 5;
constexpr i32 kTenthPercent = 10;
constexpr i32 kPercent = 100;
constexpr u16 kVampireTarget = 0x246F;
constexpr u16 kVampireDelay = 8;
constexpr i32 kComboStarted = 1;  // BattleState::unk53BF2C
constexpr i32 kComboRunning = 2;
constexpr u16 kComboCount = 0x7FFF;
constexpr i32 kComboMark = 4;  // marks set on every other unit while the combo runs
constexpr i32 kComboStill = 0x40;
constexpr u16 kComboBand = 1;
constexpr u16 kAbsorbWait = 0x28;   // RoleActionFrame command 6: the frames shown after a failed absorb
constexpr u16 kAbsorbShown = 0x1E;  // and after a successful one
constexpr u16 kAbsorbMessage = 2;
constexpr i16 kKnownAbsorbs = 0x14;
constexpr u16 kGuardedAct = 0x1C;
constexpr u16 kSucceededSound = 0x117;
constexpr u16 kFailedSound = 0x116;
constexpr int kNoticeX = 0x118;
constexpr int kNoticeY = 0xA;
constexpr i16 kAbsorbFrames = 0x28;
constexpr i16 kStealFrames = 0x32;
constexpr i16 kStealFailFrames = 0x1E;
constexpr i16 kControlFrames = 0x32;
constexpr int kNoticeFlags = BattleMessage::Frame | BattleMessage::Centered;
constexpr std::u8string_view kAbsorbSucceeded = u8"靈契成功";             // swd32.exe 0x004A77F0
constexpr std::u8string_view kAbsorbFailed = u8"靈契失敗";                // swd32.exe 0x004A77E4
constexpr std::u8string_view kStoleFormat = u8"成功偷到[%s]!!";           // swd32.exe 0x004A77D4
constexpr std::u8string_view kNothingToSteal = u8"敵人身上無任何物品!!";  // swd32.exe 0x004A77BC
constexpr std::u8string_view kStealFailed = u8"偷竊失敗!!";               // swd32.exe 0x004A77B0
constexpr std::u8string_view kControlled = u8"控制成功!!";                // swd32.exe 0x004A77A4
constexpr std::u8string_view kControlFailed = u8"控制失敗!!";             // swd32.exe 0x004A7798
constexpr u16 kNothingStolen = 0x61A8;
constexpr u16 kThrownMissed = 0x2000;  // SkillScript::ailment of a thrown talisman (MissedAilment)
constexpr int kQueueSlots = 8;
constexpr int kSpellCast = 0x1965;  // the cast acts of the accessory commands (CastAt)
constexpr int kSlowCast = 0x1791;
constexpr int kSelfCast = 0x17BA;
constexpr u16 kHideAccessory = 0x1C;
constexpr u16 kSlowAccessory = 0x1D;
constexpr u16 kBarrierAccessory = 0x20;
constexpr u16 kKillAccessory = 0x21;
constexpr u32 kBuffTurnsScale = 4;
constexpr u8 kBuffTurnsBase = 2;
constexpr u8 kKilledPose = 7;
constexpr u16 kCoinTossSweep = 6;
constexpr u16 kCoinItem = 0x5FD;
constexpr u16 kPoseHit = 0x8000;  // BattleRole::AttackPose: the cast act once the act strikes
constexpr u16 kPoseAct = 0x7FFF;
constexpr u16 kPoseDone = 2;
constexpr i32 kMaxHitDamage = 0x270F;
constexpr u16 kOrderPrepared = 0x8000;  // BattleState::enemyOrders
constexpr u16 kOrderSpell = 0x4000;
constexpr int kTimingX = 0x140;  // RoleActionFrame command 0x1A: the timing bar
constexpr int kTimingY = 0xC8;
constexpr i32 kTimingPressed = 1;  // BattleState::unk53BDA4
constexpr i32 kTimingTaken = 0x8000;
constexpr i32 kTimingKept = 0xFFF;
constexpr u16 kTimingDone = 0x8000;
constexpr u16 kTimingSound = 0x2C;
constexpr u32 kLowWord = 0xFFFF;
constexpr u32 kHighWord = 0xFFFF0000;
constexpr u16 kTimingStart = 1;
constexpr i32 kTimerStarted = static_cast<i32>(0x80000000u);  // BattleState::unk53BDAC
constexpr i32 kTimerPressed = 1;
constexpr int kTimerSeconds = 5;
constexpr int kTimerFlag = 0x4B;
constexpr int kTimerDefeatFlag = 0x4A;
constexpr u16 kTimerSound = 0x2E;
constexpr i32 kNoEscortPanel = 4;  // BattleState::panelFlags: no escort comes on
constexpr u16 kMaxEscorts = 2;
constexpr i8 kListGlowOn = static_cast<i8>(0x80);
constexpr u16 kNoUnit = 0xFFFF;
constexpr u16 kAnswersStart = 2;
constexpr i16 kFadeWord = 8;
constexpr int kRippleGrid = 0xC8;
constexpr u16 kGaugeFullSound = 0x2C;  // RoleFrame
constexpr i16 kSkillNameFrames = 0x28;
constexpr u16 kRetreatLevel = 0xC8;
constexpr u16 kRetreatAsked = 0x4000;  // BattleState::unk53BF1C
constexpr u16 kRetreating = 0x8000;
constexpr u16 kRetreatRoles = 0x7FFF;
constexpr i16 kRetreatFrames = 0x1E;
constexpr std::u8string_view kRetreatFailed = u8"撤退失敗!!";  // swd32.exe 0x004A7808
constexpr std::u8string_view kRetreated = u8"撤退成功!!";      // swd32.exe 0x004A77FC
constexpr u16 kRetreatFailedSound = 0x8C;
constexpr i32 kRetreatedState = 0x68;
constexpr i32 kFacingPanel = 0x80;
constexpr u16 kTalkCountdown = 0x7FFF;

}  // namespace

// swd32.exe 0x004539B0: one frame of the command of role `role` on unit `target` (an enemy, or a role while
// atParty); 1 once the command is over (at once for a defeated role or one without commands). Command A 0x63 ends
// the control of an enemy; the larger ones run the special attacks (RoleSpecialFrame); the others, and command B,
// run RoleCommandFrame.
// swd32.exe 0x00498350 (EH unwind: the new ripple, freed by its unique_ptr)
// swd32.exe 0x0049835B (EH handler of 0x004539B0)
int Game::RoleActionFrame(int role, int target)
{
    BattleState& battle = this->battleState;
    BattleRole& unit = this->battleRoles[static_cast<std::size_t>(role)];
    const u16 command = unit.GetCommandA();

    if (unit.IsDefeated())
    {
        return 1;
    }

    if (command == 0)
    {
        return unit.GetCommandB() == 0 ? 1 : 0;
    }

    if (command == kReleaseControl)
    {
        BattleEnemy& controlled =
            this->battleEnemies[static_cast<std::size_t>(static_cast<i16>(battle.controlledEnemy))];
        controlled.SetHeld(0);
        controlled.SetFacing(0);
        battle.controlledEnemy = kNoUnit;
        battle.talkRole = kNoUnit;
        battle.darkenUnit = -1;
        battle.controlCursor = 1;
        battle.controlRows = kAnswersStart;
        return 1;
    }

    if (command > kReleaseControl)
    {
        return this->RoleSpecialFrame(unit, role, target, command);
    }

    return this->RoleCommandFrame(unit, role, target, command);
}

// The special-move points of the party role of battle role `role` (inline in 0x004539B0).
void Game::AddSpecialPoints(int role, i32 points)
{
    const auto partyRole = static_cast<std::size_t>(this->battleState.partyRoles[static_cast<std::size_t>(role)]);
    this->party.battleBlocks[partyRole].values[kSpecialPoints] += points;
}

// A hit of a role landed on enemy `target` (inline in 0x004539B0).
void Game::RoleHitLanded(int target)
{
    BattleState& battle = this->battleState;
    battle.hitLanded = 1;
    battle.lastEnemyHit = static_cast<u16>(target);
    battle.slotTargets[static_cast<std::size_t>(target)] = target;
    battle.unk53BFC0 = 0;
}

// Unit `target` defeated by a role (inline in 0x004539B0): recorded for the removal, the frame flashed white.
void Game::RoleDefeated(int target)
{
    BattleState& battle = this->battleState;
    battle.unk4A7626 = static_cast<u16>(target);
    battle.unk502984[static_cast<std::size_t>(target)] = target;
    battle.flashing = 1;
    this->FlashFrameWhite();
}

// The command A above 0x63 of RoleActionFrame (inline in the original): 100 at once; 200 walks to the target and 300
// returns home; the special attacks 0x190 (charge), 0x192 (swarm, the field darkened by tint), 0x194 (the combo: the
// other units marked still while it runs), 0x195 (drain spell), 0x196 (beam), 0x199 (leap) and 0x1F4 (magic) land
// their hit (two special points, five more for a defeat) and end with command 0x12C and no fury (0x199 is over then).
int Game::RoleSpecialFrame(BattleRole& unit, int role, int target, u16 command)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto at = static_cast<std::size_t>(target);
    BattleUnit& aimed = battle.atParty != 0 ? static_cast<BattleUnit&>(this->battleRoles[at])
                                            : static_cast<BattleUnit&>(this->battleEnemies[at]);
    BattleEnemy& enemy = this->battleEnemies[at];

    // The damage of a landed special attack (inline in the original).
    auto damage = [this, &battle, &unit, &enemy, role, target]
    {
        if (battle.unk53AF68 == 0 && enemy.Damage(this->battleServices, battle.hitDamage, 0, 0) == 1)
        {
            this->RoleDefeated(target);
            battle.hitDamage = -1;
            unit.SetCommand(kAfterAction);
            this->AddSpecialPoints(role, kDefeatPoints);
        }

        battle.hitDamage = 0;
        unit.AddFury(this->battleServices, 0);
        unit.SetCommand(kAfterAction);
        return 0;
    };

    auto landed = [this, role, target, &damage](int done)
    {
        if (done != 1)
        {
            return 0;
        }

        this->RoleHitLanded(target);
        this->AddSpecialPoints(role, kHitPoints);
        return damage();
    };

    switch (command)
    {
        case kDoneCommand:
            return 1;

        case kWalkCommand:
            unit.WalkToTarget(services, aimed);
            return 0;

        case kReturnCommand:
            if (unit.ReturnHome(services, aimed, -1, 0) == 1)
            {
                battle.darkenUnit = -1;
                return 1;
            }

            return 0;

        case kChargeCommand:
            return landed(unit.ChargeAttackFrame(services, enemy, 0));

        case kSwarmCommand:
            battle.darkRed = kDarkTint;
            battle.darkGreen = kDarkTint;
            battle.darkBlue = kDarkTint;
            battle.unk53BF94 = 1;
            battle.darkenByTint = 1;

            if (unit.SwarmSpellFrame(services, enemy) != 1)
            {
                return 0;
            }

            this->AddSpecialPoints(role, kHitPoints);
            return damage();

        case kComboCommand:
            return this->ComboFrame(unit, role, target);

        case kDrainSpellCommand:
            return landed(this->DrainSpellFrame(unit, enemy));

        case kBeamCommand:
            return landed(unit.BeamSpellFrame(services, enemy));

        case kLeapCommand:
            if (unit.LeapSpellFrame(services, enemy) != 1)
            {
                return 0;
            }

            this->RoleHitLanded(target);
            this->AddSpecialPoints(role, kHitPoints);

            if (battle.unk53AF68 == 0 && enemy.Damage(services, battle.hitDamage, 0, 0) == 1)
            {
                this->RoleDefeated(target);
                this->AddSpecialPoints(role, kDefeatPoints);
            }

            battle.hitDamage = 0;
            battle.darkenUnit = -1;
            unit.SetCommand(kAfterAction);
            unit.AddFury(services, 0);
            return 1;

        case kMagicCommand:
            if (unit.MagicFrame(services, enemy) != 1)
            {
                return 0;
            }

            this->RoleHitLanded(target);
            this->AddSpecialPoints(role, kHitPoints);
            battle.hitDamage = 0;
            unit.AddFury(services, 0);
            unit.SetCommand(kAfterAction);
            return 0;

        default:
            return 0;
    }
}

// The combo of RoleSpecialFrame (inline in the original): its first frame marks every other role not guarding and
// every enemy still (kComboMark, kComboStill), the next opens the grey band; then the magic attack lands like the
// other special attacks and every mark is cleared again.
int Game::ComboFrame(BattleRole& unit, int role, int target)
{
    BattleState& battle = this->battleState;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];

    auto markAll = [this, &battle, role](bool mark)
    {
        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& other = this->battleRoles[index];

            if (static_cast<int>(index) == role || (mark && other.IsGuarding()))
            {
                continue;
            }

            if (mark)
            {
                other.SetStatus(kComboMark);
                other.SetStatus(kComboStill);
            }
            else
            {
                other.ClearStatus(kComboMark);
                other.ClearStatus(kComboStill);
            }
        }

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& other = this->battleEnemies[index];

            if (mark)
            {
                other.SetStatus(kComboStill);
                other.SetStatus(kComboMark);
            }
            else
            {
                other.ClearStatus(kComboMark);
                other.ClearStatus(kComboStill);
            }
        }
    };

    if ((battle.unk53BF2C & kComboCount) == 0)
    {
        battle.unk53BF2C = kComboStarted;
        markAll(true);
    }

    if ((battle.unk53BF2C & kComboCount) == kComboStarted)
    {
        battle.unk53BF2C = kComboRunning;
        battle.grayBandShown = 1;
        battle.grayBand = kComboBand;
    }

    if ((battle.unk53BF2C & kComboCount) != kComboRunning || this->MagicAttackFrame(unit, enemy) != 1)
    {
        return 0;
    }

    this->RoleHitLanded(target);
    this->AddSpecialPoints(role, kHitPoints);

    if (battle.unk53AF68 == 0 && enemy.Damage(this->battleServices, battle.hitDamage, 0, 0) == 1)
    {
        this->RoleDefeated(target);
        battle.hitDamage = -1;
        unit.SetCommand(kAfterAction);
        this->AddSpecialPoints(role, kDefeatPoints);
    }

    battle.hitDamage = 0;
    battle.unk53BF2C = 0;
    battle.grayBand = 0;
    battle.grayBandShown = 0;
    unit.AddFury(this->battleServices, 0);
    unit.SetCommand(kAfterAction);
    markAll(false);
    return 0;
}

// The commands up to 0x62 of RoleActionFrame (inline in the original), by command A (command B when A is 0 and B set:
// none of these, so 0).
int Game::RoleCommandFrame(BattleRole& unit, int role, int target, u16 command)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto at = static_cast<std::size_t>(target);

    switch (command)
    {
        case kAttackCommand:
            return this->RoleAttackFrame(unit, target);

        case kSpellCommand:
        case kItemCommand:
            return this->RoleSpellFrame(unit, role, command);

        case kMagicAttackCommand:
        {
            BattleEnemy& enemy = this->battleEnemies[at];

            if (this->MagicAttackFrame(unit, enemy) != 1)
            {
                return 0;
            }

            this->RoleHitLanded(target);
            this->AddSpecialPoints(role, kHitPoints);

            if (battle.unk53AF68 == 0 && enemy.Damage(services, battle.hitDamage, 0, 0) == 1)
            {
                this->RoleDefeated(target);
                battle.hitDamage = -1;
                unit.SetCommand(kAfterAction);
                this->AddSpecialPoints(role, kDefeatPoints);
            }

            battle.hitDamage = 0;
            unit.AddFury(services, 0);
            unit.SetCommand(kAfterAction);
            return 0;
        }

        case kGuardCommand:
            battle.darkenUnit = -1;
            return 1;

        case kAbsorbCommand:
            return this->AbsorbFrame(unit, target);

        case kDefeatCommand:
        {
            BattleEnemy& enemy = this->battleEnemies[at];

            if (enemy.DefeatFrame(services) != 1)
            {
                return 0;
            }

            enemy.SetCommand(0);
            battle.RemoveSlotRecord(target);

            if (target >= 0 && target < static_cast<int>(Party::Roles))
            {
                ++battle.enemiesGone;
            }

            return 1;
        }

        case kFadeInCommand:
        case kFadeOutCommand:
        {
            const bool fadeIn = command == kFadeInCommand;

            if ((fadeIn ? unit.DrawFadeIn(services, unit) : unit.DrawFadeOut(services)) != 1)
            {
                return 0;
            }

            unit.SetGuarding(fadeIn ? 0 : 1);
            unit.SetWord2A0A(kFadeWord);
            battle.enemyFaded = 1;
            battle.darkenUnit = -1;
            return 1;
        }

        case kThrowCommand:
            return this->ThrowTalismanFrame(unit, role, target);

        case kCatchCommand:
            if (battle.pushFrames == 0)
            {
                battle.darkRed = kDarkTint;
                battle.darkGreen = kDarkTint;
                battle.darkBlue = kDarkTint;
                battle.pushFrames = 1;
                battle.unk53BF94 = 1;
                this->PrepareDarkenedCopies();
                battle.darkLevel = 1;
            }

            if (unit.CatchAct(services, this->battleEnemies[at]) != 1)
            {
                return 0;
            }

            battle.unk53BF94 = 0;
            battle.darkRed = 0;
            battle.darkGreen = 0;
            battle.darkBlue = 0;
            battle.pushFrames = 0;
            battle.undarkening = 1;
            battle.result = kCraftState;
            battle.darkenUnit = -1;
            return 1;

        case kEscortCommand:
            return this->SummonEscortFrame(unit, role);

        case kLeaveCommand:
            if (battle.unk53BF1C != 0)
            {
                return 0;
            }

            battle.darkenUnit = -1;
            return 1;

        case kCoinCommand:
            return this->CoinTossFrame(unit, role);

        case kStealCommand:
            return this->StealFrame(unit, target);

        case kPoseCommand:
            return this->PoseSweepFrame(unit);

        case kControlCommand:
            return this->ControlFrame(unit, role, target);

        case kTimingCommand:
            return this->TimingAttackFrame(unit, target);

        case kDirectCommand:
        {
            BattleEnemy& enemy = this->battleEnemies[at];

            if (unit.DirectAttack(services, enemy) != 1)
            {
                return 0;
            }

            battle.usedUpItem = unit.UseSelectedItem();

            if (battle.unk53AF68 == 0 && enemy.Damage(services, battle.hitDamage, 0, 0) == 1)
            {
                this->RoleDefeated(target);
            }

            battle.hitDamage = 0;
            battle.darkenUnit = -1;
            return 1;
        }

        case kHideCommand:
        case kSlowCommand:
        case kBarrierCommand:
            return this->AccessoryBuffFrame(unit, target, command);

        case kTimerCommand:
            return this->TimerAttackFrame(unit, target);

        case kKillCommand:
        {
            BattleEnemy& enemy = this->battleEnemies[at];

            if (unit.BoomerangFrame(services, enemy) != 1)
            {
                return 0;
            }

            battle.darkenUnit = -1;
            battle.lastDefeatedObject = enemy.Data();

            // Without an object the original reads through a null pointer: taken as immune.
            if (battle.lastDefeatedObject == nullptr ||
                (battle.lastDefeatedObject->data.kind & ObjectData::UnkKind20) != 0)
            {
                return 1;
            }

            if (RollChance(this->random, unit.AccessoryProficiency(kKillAccessory, this->ProficiencyList())))
            {
                enemy.SetDefeatPose(kKilledPose);
                enemy.StartDefeat(1);
                this->RoleDefeated(target);
            }

            return 1;
        }

        case kSelfDamageCommand:
        case kSelfSpiritCommand:
        case kSelfStaminaCommand:
            return this->SelfHitFrame(unit, command);

        default:
            return 0;
    }
}

// The proficiency list of the party (Fame::ProficiencyList, 0x004B8A00).
CounterList& Game::ProficiencyList()
{
    return this->party.fame.lists[Fame::ProficiencyList];
}

// Command 1 of RoleActionFrame (inline in the original): the attack lands on its target (an enemy, or a role while
// atParty). The attacker drains after the hit (on the enemy of the target's index even for a role target, kept); a
// vampire ring (DrainKind 8) turns a tenth plus its proficiency of the rest into healing of the attacker (on an enemy
// target, with its digits; on a role target the share is only computed); its counters step by 5. The command goes on
// with 0x12C.
int Game::RoleAttackFrame(BattleRole& unit, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto at = static_cast<std::size_t>(target);
    BattleEnemy& enemy = this->battleEnemies[at];
    BattleUnit& aimed =
        battle.atParty != 0 ? static_cast<BattleUnit&>(this->battleRoles[at]) : static_cast<BattleUnit&>(enemy);

    if (unit.AttackFrame(services, aimed, 0) != 1)
    {
        return 0;
    }

    battle.hitLanded = 1;
    battle.lastEnemyHit = static_cast<u16>(target);
    battle.slotTargets[at] = target;
    battle.unk53BFC0 = 0;

    // 0x00453ACF..0x00453B10: a defeat keeps the damage (no -1 as after the special attacks)
    if (battle.unk53AF68 == 0 && aimed.Damage(services, battle.hitDamage, 0, 0) == 1)
    {
        this->RoleDefeated(target);
        unit.SetCommand(kAfterAction);
    }

    if (battle.atParty != 0)
    {
        this->ApplyDrain(unit, enemy);
    }

    if (unit.DrainKind() == kVampireDrain)
    {
        const i32 share = static_cast<u16>(unit.AccessoryProficiency(kVampireRing, this->ProficiencyList()));
        const i32 tenth = battle.hitDamage * kTenthPercent / kPercent;

        if (battle.atParty != 0)
        {
            battle.hitDamage = tenth + battle.hitDamage * share / kPercent;
            battle.lastDamage = 0;
        }
        else
        {
            battle.lastDamage = 0;
            battle.hitDamage = tenth + (battle.hitDamage - tenth) * share / kPercent;
            unit.PushTarget(kVampireTarget);
            battle.hitDamage = -battle.hitDamage;
            unit.SetDamageDigits(services, battle.hitDamage);
            unit.Damage(services, battle.hitDamage, 0, 0);
            unit.PushTargetAlt(kVampireDelay);
            unit.StepTargetCount(1);
        }

        unit.StepAccessoryCounters(this->ProficiencyList(), kVampireStep);
    }

    if (battle.atParty == 0)
    {
        this->ApplyDrain(unit, enemy);
    }

    battle.hitDamage = 0;
    battle.staminaDamage = 0;
    battle.spiritDamage = 0;
    unit.SetCommand(kAfterAction);
    return 0;
}

// Commands 2 and 3 of RoleActionFrame (inline in the original): the first frame sets SpellCast, the sweep reset
// (sweepTotal at most the units of the target side) and, for a spell with ActFlag13, the ripple battle; once the spell
// is over (SpellOver, not while the resistance panel shows for a spell) the ripple goes, an item is used up
// (usedUpItem), and the role goes on with 0x12C and retreats if it asked to.
int Game::RoleSpellFrame(BattleRole& unit, int role, u16 command)
{
    BattleState& battle = this->battleState;

    if ((battle.spellStage & BattleState::SpellCast) == 0)
    {
        battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellCast);
        battle.sweepGap = unit.GetWordDA6();
        battle.slotTargets.fill(-1);
        battle.sweptUnits = 1;
        battle.slotDamage.fill(0);

        if (command == kSpellCommand && unit.ActFlag13() == 1)
        {
            battle.rippleHits = 1;
            this->battleRipple =
                std::make_unique<ScriptObject>(Display::Width, Display::Height, 0, 0, kRippleGrid, kRippleGrid);
        }

        const auto units = static_cast<i32>(battle.atParty != 0 ? battle.roleCount : battle.enemyCount);

        if (battle.sweepTotal > units)
        {
            battle.sweepTotal = units;
        }
    }

    if ((battle.spellStage & BattleState::SpellOver) == 0)
    {
        return 0;
    }

    if (command == kSpellCommand)
    {
        battle.hitDamage = 0;
        battle.spiritDamage = 0;
        battle.staminaDamage = 0;

        if ((battle.panelFlags & BattleState::ResistancePanel) != 0)
        {
            return 0;
        }

        if (battle.rippleHits == 1)
        {
            this->battleRipple.reset();
        }

        battle.rippleHits = 0;
    }
    else
    {
        battle.usedUpItem = unit.UseSelectedItem();
        battle.hitDamage = 0;
        battle.spiritDamage = 0;
        battle.staminaDamage = 0;
    }

    battle.undarkening = 1;
    unit.SetCommand(kAfterAction);
    this->TryRetreat(role);
    return 0;
}

// Command 6 of RoleActionFrame (inline in the original): the absorb (靈契) of the enemy: a known enemy (absorbed
// kKnownAbsorbs times, the fame list) or one CanAbsorb allows is absorbed (the field darkened, an enemy of act 0x1C
// guarding); otherwise it fails at once. Once absorbed the enemy counts in the fame list and leaves, its object
// joins the inventory; the message (靈契成功 or 靈契失敗) shows after the wait, and with every enemy gone the battle
// ends.
int Game::AbsorbFrame(BattleRole& unit, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];
    CounterList& fame = this->party.fame.lists[Fame::FameList];

    if (battle.pushFrames > 1)
    {
        if (--battle.pushFrames != kAbsorbMessage)
        {
            return 0;
        }

        const bool succeeded = battle.absorbSucceeded == 1;
        this->audio.PlaySound(succeeded ? kSucceededSound : kFailedSound, this->config.soundVolume);
        this->battleMessages
            .Add(kNoticeX, kNoticeY, kAbsorbFrames, succeeded ? kAbsorbSucceeded : kAbsorbFailed, kNoticeFlags);
        battle.unk53BF94 = 0;
        battle.darkRed = 0;
        battle.darkGreen = 0;
        battle.darkBlue = 0;
        battle.darkenUnit = -1;
        battle.pushFrames = 0;
        battle.unk4A7626 = kNoUnit;
        battle.unk53C02C = 0;
        battle.undarkening = 1;

        if (static_cast<i32>(battle.enemiesGone) - battle.unk53BF02 >= static_cast<i32>(battle.enemyCount))
        {
            battle.autoBattle = 0;
            battle.result = kEndCheckState;
        }

        return 1;
    }

    if (battle.pushFrames == 0)
    {
        if (static_cast<u16>(FindCounterValue(fame, static_cast<i16>(enemy.ActId()))) >= kKnownAbsorbs)
        {
            battle.unk53C02C = 1;
        }

        if (unit.CanAbsorb(services, enemy) == 1)
        {
            battle.unk53C02C = 1;
        }
        else if (battle.unk53C02C != 1)
        {
            battle.absorbSucceeded = 0;
            battle.pushFrames = kAbsorbWait;
            return 0;
        }

        unit.StartAbsorb(services, enemy);
        battle.absorbSucceeded = 1;
        enemy.StartDefeat(1);
        battle.darkRed = kDarkTint;
        battle.darkGreen = kDarkTint;
        battle.darkBlue = kDarkTint;
        battle.pushFrames = 1;
        battle.unk4A7626 = static_cast<u16>(target);
        battle.unk53BF94 = 1;
        this->PrepareDarkenedCopies();

        if (enemy.ActId() == kGuardedAct)
        {
            enemy.SetGuarding(1);
        }
    }

    if (unit.AbsorbFrame(services) == 1)
    {
        enemy.StartDefeat(1);
        AddCounterCapped(fame, static_cast<i16>(enemy.ActId()), 1);
        ++battle.enemiesGone;
        battle.RemoveSlotRecord(target);
        battle.unk4A7626 = static_cast<u16>(target);
        battle.pushFrames = kAbsorbShown;
        AddOneItem(
            this->party.inventory,
            battle.enemyPlaces[static_cast<std::size_t>(target)].actId,
            1,
            this->database
        );
    }

    return 0;
}

// Command 0xD of RoleActionFrame (inline in the original): the talisman thrown (the field darkened) marks the enemy
// missed (an ailment with MissedAilment) and queues the enemy (1-based) in the role's row of actionQueue (its first
// free slot of the first eight; none: nothing).
int Game::ThrowTalismanFrame(BattleRole& unit, int role, int target)
{
    BattleState& battle = this->battleState;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];

    if (battle.pushFrames == 0)
    {
        battle.darkRed = kDarkTint;
        battle.darkGreen = kDarkTint;
        battle.darkBlue = kDarkTint;
        battle.pushFrames = 1;
        battle.unk53BF94 = 1;
        this->PrepareDarkenedCopies();
    }

    if (unit.ThrowAct(this->battleServices, enemy) != 1)
    {
        return 0;
    }

    battle.accessoryEffect = SkillScript{};
    battle.accessoryEffect.ailment = static_cast<u16>(battle.accessoryEffect.ailment | kThrownMissed);
    battle.darkenUnit = -1;
    enemy.AddAilment(this->battleServices, battle.accessoryEffect);
    battle.darkenUnit = -1;
    battle.undarkening = 1;
    battle.unk53BF94 = 0;
    battle.darkRed = 0;
    battle.darkGreen = 0;
    battle.darkBlue = 0;
    battle.pushFrames = 0;
    const auto row = static_cast<std::size_t>(role * BattleState::QueueColumns);

    for (int slot = 0; slot < kQueueSlots; ++slot)
    {
        i16& queued = battle.actionQueue[row + static_cast<std::size_t>(slot)];

        if (queued == 0)
        {
            queued = static_cast<i16>(target + 1);
            break;
        }
    }

    return 1;
}

// Command 0xF of RoleActionFrame (inline in the original): the escort of the role (escortCalled) comes on (one more
// role unit, at most kMaxEscorts, unless panel flag 4 holds it back): set up at its place, guarding, facing the
// enemies; the role glows (the field darkened) and the escort takes its place in the order.
int Game::SummonEscortFrame(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    u16& escortOf = battle.escortCalled[static_cast<std::size_t>(role + BattleState::FirstRoleUnit)];

    if (battle.pushFrames == 0)
    {
        if ((battle.panelFlags & kNoEscortPanel) == 0 && battle.escortCount < kMaxEscorts)
        {
            ++battle.escortCount;
            ++battle.roleCount;
        }

        battle.callingEscort = escortOf;
        BattleRole& escort = this->battleRoles[battle.callingEscort];
        escort.Reset(services);
        escort.SetB00(1);
        escort.ClearIdleAction();
        escort.SetGuarding(1);

        if (battle.mirrored == 0)
        {
            escort.SetFacing(1);
        }

        escort.SetupEscort(battle.rolePlaces[battle.callingEscort], this->database, this->errors);
        battle.pushFrames = 1;
        battle.unk53BDD4 = 0;
        battle.unk53BDD8 = 0;
    }

    battle.darkRed = kDarkTint;
    battle.darkGreen = kDarkTint;
    battle.darkBlue = kDarkTint;
    battle.unk53BF94 = 1;
    this->PrepareDarkenedCopies();
    const UnitPlacement& place = battle.rolePlaces[battle.callingEscort];

    if (unit.GlowAct(services, place.x, place.y) != 1)
    {
        return 0;
    }

    battle.RemoveSlotRecord(battle.callingEscort + BattleState::FirstRoleUnit);
    battle.panelFlags &= ~kNoEscortPanel;

    // The original clears a dword past unk524268 for an escort at place 8 or later.
    if (battle.callingEscort < battle.unk524268.size())
    {
        battle.unk524268[battle.callingEscort] = 0;
    }

    battle.callingEscort = 0;
    escortOf = 0;
    battle.listGlow = kListGlowOn;
    battle.guardCursor = 0;
    battle.darkenUnit = -1;
    battle.pushFrames = 0;
    this->MeasureUnits();
    battle.SortDrawOrder();
    return 1;
}

// Command 0x16 of RoleActionFrame (inline in the original): the coin toss (DrawCoinToss) picks the side (coinState):
// every unit standing on it counts for the sweep, the first one standing is aimed at, and the coin item's act is
// cast as a spell (targetsAll) over kCoinTossSweep frames; once the spell is over the command is.
int Game::CoinTossFrame(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;

    if ((battle.spellStage & BattleState::SpellCast) != 0)
    {
        if ((battle.spellStage & BattleState::SpellOver) == 0)
        {
            return 0;
        }

        battle.undarkening = 1;
        return 1;
    }

    if (this->battleHud.DrawCoinToss() != 1)
    {
        return 0;
    }

    battle.sweepGap = kCoinTossSweep;
    const bool party = (this->battleHud.coinState & kLowWord) != 0;
    this->battleEnemies[static_cast<std::size_t>(static_cast<i16>(this->battleRoles[0].GetWord29A2()))].DecreaseHits();
    battle.sweepTotal = 0;

    if (party)
    {
        battle.atParty = 1;
    }

    const u32 units = party ? battle.roleCount : battle.enemyCount;

    auto down = [this, party](u32 index)
    {
        return party ? this->battleRoles[index].IsDefeated() : this->battleEnemies[index].IsDefeated();
    };

    for (u32 index = 0; index < units; ++index)
    {
        if (!down(index))
        {
            ++battle.sweepTotal;
        }
    }

    for (u32 index = 0; index < units; ++index)
    {
        if (!down(index))
        {
            unit.Retarget(static_cast<i16>(index));
            break;
        }
    }

    battle.targetsAll = 1;
    u32 act = static_cast<u32>(battle.unk4FE5D4[static_cast<std::size_t>(role)]);
    unit.LoadItemAction(this->database, kCoinItem, act);
    battle.unk4FE5D4[static_cast<std::size_t>(role)] = static_cast<i32>(act);
    unit.SetAA8();
    battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellCast);
    return 0;
}

// Command 0x17 of RoleActionFrame (inline in the original): once posed at the enemy the role steals: an object shows
// 成功偷到[name]!! and joins the inventory; nothing shows 敵人身上無任何物品!! (kNothingStolen) or 偷竊失敗!!.
int Game::StealFrame(BattleRole& unit, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];

    if (unit.PoseAtTarget(services, enemy) != 1)
    {
        return 0;
    }

    const u16 stolen = unit.Steal(services, enemy);

    if (stolen != 0 && stolen < kNothingStolen)
    {
        this->database.LoadObject(battle.lastObject, stolen);
        this->database.FreeDescription(battle.lastObject);
        const std::string name = ObjectName(battle.lastObject);
        this->audio.PlaySound(kSucceededSound, this->config.soundVolume);
        this->battleMessages.AddFormatted(
            kNoticeX,
            kNoticeY,
            kStealFrames,
            WinFormat(kStoleFormat, {std::string_view(name)}),
            kNoticeFlags
        );
        AddOneItem(this->party.inventory, stolen, 1, this->database);
    }
    else
    {
        this->audio.PlaySound(kFailedSound, this->config.soundVolume);
        this->battleMessages.Add(
            kNoticeX,
            kNoticeY,
            kStealFailFrames,
            stolen == kNothingStolen ? kNothingToSteal : kStealFailed,
            kNoticeFlags
        );
    }

    unit.SetCommand(kAfterAction);
    return 0;
}

// Command 0x18 of RoleActionFrame (inline in the original): the attack pose; once it strikes (kPoseHit) every enemy
// standing takes the role's hit (the damage running total of the frame, capped per hit, its digits, a defeat flashing
// white) and its hit act becomes enemiesHitAct; the command is over once the pose is (kPoseDone).
int Game::PoseSweepFrame(BattleRole& unit)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    battle.lastRoleHit = unit.AttackPose(services);

    if ((battle.lastRoleHit & kPoseHit) != 0)
    {
        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = this->battleEnemies[index];

            if (enemy.IsDefeated())
            {
                continue;
            }

            battle.unk53AE90[index] = static_cast<i32>(index);
            battle.lastDamage = unit.HitDamage(services, enemy, battle.spiritDamage, battle.staminaDamage);

            if (battle.lastDamage >= kMaxHitDamage)
            {
                battle.lastDamage = kMaxHitDamage;
            }

            battle.hitDamage += battle.lastDamage;
            enemy.RaiseFlag26B8();
            enemy.SetDamageDigits(services, battle.hitDamage);
            enemy.StepTargetCount(1);

            if (battle.unk53AF68 == 0 && enemy.Damage(services, battle.hitDamage, 0, 0) == 1)
            {
                this->RoleDefeated(static_cast<int>(index));
            }

            battle.hitDamage = 0;
        }

        battle.enemiesHitAct = battle.lastRoleHit & kPoseAct;
    }

    if (battle.lastRoleHit == kPoseDone)
    {
        battle.darkenUnit = -1;
        return 1;
    }

    battle.lastRoleHit = 0;
    return 0;
}

// Command 0x19 of RoleActionFrame (inline in the original): without a controlled enemy the role tries to control the
// enemy (控制成功!!: it faces the party, its turn resets and it leaves the slot records; or 控制失敗!!). With one the
// role orders it: its order (enemyOrders) is a prepared command, the chosen skill (orderedSkill) or the chosen object
// (unk53BD48), aimed at the enemy actingEnemy (1-based in aimTarget), and it is queued to act.
int Game::ControlFrame(BattleRole& unit, int role, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if (battle.controlledEnemy == kNoUnit)
    {
        BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];

        if (unit.CanTarget(enemy) != 1)
        {
            this->battleMessages.Add(kNoticeX, kNoticeY, kControlFrames, kControlFailed, kNoticeFlags);
            return 1;
        }

        this->battleMessages.Add(kNoticeX, kNoticeY, kControlFrames, kControlled, kNoticeFlags);
        enemy.SetFacing(1);
        battle.controlledEnemy = static_cast<u16>(target);
        battle.talkRole = static_cast<u16>(role);
        enemy.ResetTurn(services);
        battle.RemoveSlotRecord(target);
        battle.darkenUnit = -1;
        return 1;
    }

    const auto controlled = static_cast<std::size_t>(static_cast<i16>(battle.controlledEnemy));
    BattleEnemy& enemy = this->battleEnemies[controlled];
    u16& order = battle.enemyOrders[controlled];
    battle.aimTarget = battle.actingEnemy + 1;
    battle.enemyAimsAtEnemies = 1;
    order = kOrderPrepared;

    if (battle.orderedSkill != 0)
    {
        order = kOrderSpell;
        u16 act = static_cast<u16>(battle.enemyHitAct);
        enemy.LoadSkillAction(this->database, static_cast<u16>(battle.orderedSkill), act);
        battle.enemyHitAct = static_cast<i32>((static_cast<u32>(battle.enemyHitAct) & kHighWord) | act);
        battle.orderedSkill = 0;
    }

    if (battle.unk53BD48 != 0)
    {
        order = kOrderPrepared;
        u32 act = static_cast<u32>(battle.enemyHitAct);
        enemy.LoadEnemyCommand(this->database, 1, act);
        battle.enemyHitAct = static_cast<i32>(act);
    }

    order = static_cast<u16>(order | static_cast<u16>(battle.aimTarget - 1));
    battle.AddSlotRecord(kSlotRecordEnemy, static_cast<i16>(battle.controlledEnemy));
    battle.darkenUnit = -1;
    return 1;
}

// Command 0x1A of RoleActionFrame (inline in the original): the timing bar runs while the role holds still
// (kComboStill once the needle starts); a press (unk53BDA4) takes the timing: a hit with the needle on the mark
// (timingHit) sounds and frees the role, otherwise the next attack repeats (numberFont's low word). Each attack that
// lands holds the role again; a defeat holds the enemy and ends the bar (kTimingDone), as does the bar's end without a
// press.
int Game::TimingAttackFrame(BattleRole& unit, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleHud& hud = this->battleHud;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];

    auto setLow = [](auto& value, u32 low)
    {
        using Value = std::remove_reference_t<decltype(value)>;
        value = static_cast<Value>((static_cast<u32>(value) & kHighWord) | low);
    };

    auto end = [&unit, &battle, &hud, &setLow]
    {
        unit.ClearStatus(kComboStill);
        unit.SetCommand(kAfterAction);
        hud.timingHit = 0;
        battle.unk53C024 = 0;
        setLow(hud.numberFont, 0);
        setLow(hud.timingCounter, 0);
    };

    battle.unk53C024 = 1;

    if ((static_cast<u32>(hud.timingCounter) & kLowWord) == kTimingStart)
    {
        unit.SetStatus(kComboStill);
    }

    if (hud.DrawTimingBar(kTimingX, kTimingY) == 1 && (battle.unk53BDA4 & kTimingTaken) == 0)
    {
        end();
        return 0;
    }

    if ((battle.unk53BDA4 & kTimingPressed) != 0)
    {
        battle.unk53BDA4 = kTimingTaken;

        if ((hud.numberFont & kLowWord) == 0)
        {
            if (hud.timingHit != 0)
            {
                hud.timingHit = 0;
                this->audio.PlaySound(kTimingSound, this->config.soundVolume);
                unit.ClearStatus(kComboStill);
            }
            else
            {
                setLow(hud.numberFont, 1);
                hud.timingHit = 0;
            }
        }
    }

    if (unit.AttackFrame(services, enemy, 0) != 1)
    {
        return 0;
    }

    battle.unk53BDA4 &= kTimingKept;
    battle.hitLanded = 1;
    battle.lastEnemyHit = static_cast<u16>(target);
    battle.slotTargets[static_cast<std::size_t>(target)] = target;
    battle.unk53BFC0 = 0;
    battle.unk53C024 = 0;

    if (battle.unk53AF68 == 0 && enemy.Damage(services, battle.hitDamage, 0, 0) == 1)
    {
        battle.hitDamage = -1;
        battle.unk4A7626 = static_cast<u16>(target);
        battle.unk502984[static_cast<std::size_t>(target)] = target;
        battle.flashing = 1;
        enemy.SetHeld(1);
        this->FlashFrameWhite();
        setLow(hud.timingCounter, kTimingDone);
    }

    battle.hitDamage = 0;
    unit.SetStatus(kComboStill);

    if ((static_cast<u32>(hud.timingCounter) & kTimingDone) != 0)
    {
        end();
    }

    return 0;
}

// Commands 0x1C, 0x1D and 0x20 of RoleActionFrame (inline in the original): the accessory buff cast (on the role,
// 0x1D on the enemy) with turns of 2 plus 4 per hundred of the accessory's proficiency: 0x1C makes the role invisible,
// 0x20 sets its barrier; 0x1D slows the enemy (hit) and then, as the original falls through, gives the role the same
// buff.
int Game::AccessoryBuffFrame(BattleRole& unit, int target, int command)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];
    SkillScript& skill = battle.accessoryEffect;

    auto buff = [this, &unit, &skill](u16 accessory, u32 bits)
    {
        skill = SkillScript{};
        const u16 proficiency = static_cast<u16>(unit.AccessoryProficiency(accessory, this->ProficiencyList()));
        skill.statusBits = bits;
        skill.turns = static_cast<u8>(kBuffTurnsScale * proficiency / kPercent + kBuffTurnsBase);
    };

    if (command == kSlowCommand)
    {
        if (unit.CastAt(services, enemy, kSlowCast) != 1)
        {
            return 0;
        }

        buff(kSlowAccessory, SkillScript::AttackDown);
        enemy.AddBuff(services, skill);
        enemy.RaiseFlag26B8();
    }
    else
    {
        if (unit.CastAt(services, unit, kSpellCast) != 1)
        {
            return 0;
        }

        if (command == kHideCommand)
        {
            buff(kHideAccessory, SkillScript::Invisible);
        }
        else
        {
            buff(kBarrierAccessory, SkillScript::Barrier);
        }
    }

    unit.AddBuff(services, skill);
    battle.darkenUnit = -1;
    return 1;
}

// Command 0x1F of RoleActionFrame (inline in the original): a five-second timer (StartTimer) runs over the enemy
// (its place in unk53BF50); game flag 0x4B (the timer's end) ends the command; each press (unk53BDAC) hits the enemy
// with the role's hit (its digits) and a defeat clears game flag 0x4A and ends it.
int Game::TimerAttackFrame(BattleRole& unit, int target)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];
    battle.result = 0;

    if (battle.unk53BDAC == 0)
    {
        battle.unk53BDAC = kTimerStarted;
        this->StartTimer(0, kTimerSeconds, 0, 1);
        u16 x = 0;
        u16 y = 0;
        enemy.GetDrawPosition(x, y);
        battle.unk53BF50 = static_cast<i16>(x);
        battle.unk53BF52 = static_cast<i16>(y);
        this->battleHud.ResetEffectAt();
    }

    if (this->state.GameFlag(kTimerFlag))
    {
        this->state.ClearGameFlag(kTimerFlag);
        battle.unk53BDAC = 0;
        battle.unk53BF58 = 0;
        battle.darkenUnit = -1;
        return 1;
    }

    if ((battle.unk53BDAC & kTimerPressed) == 0)
    {
        return 0;
    }

    this->audio.PlaySound(kTimerSound, this->config.soundVolume);
    this->battleHud.ResetEffectAt();
    battle.unk53BDAC = kTimerStarted;
    battle.unk53BF58 = 1;
    battle.hitDamage = unit.HitDamage(services, enemy, battle.spiritDamage, battle.staminaDamage);
    enemy.RaiseFlag26B8();
    enemy.SetDamageDigits(services, battle.hitDamage);
    enemy.StepTargetCount(1);

    if (battle.unk53AF68 == 0 && enemy.Damage(services, battle.hitDamage, 0, 0) == 1)
    {
        this->state.ClearGameFlag(kTimerDefeatFlag);
        this->RoleDefeated(target);
        battle.hitDamage = 0;
        battle.unk53BDAC = 0;
        battle.darkenUnit = -1;
        return 1;
    }

    battle.hitDamage = 0;
    return 0;
}

// Commands 0x22..0x24 of RoleActionFrame (inline in the original): the role casts at itself and takes its own hit as
// damage (0x22), spirit (0x23) or stamina (0x24), with the digits.
int Game::SelfHitFrame(BattleRole& unit, int command)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if (unit.CastAt(services, unit, kSelfCast) != 1)
    {
        return 0;
    }

    battle.lastDamage = unit.HitDamage(services, unit, battle.spiritDamage, battle.staminaDamage);

    switch (command)
    {
        case kSelfDamageCommand:
            unit.SetDamageDigits(services, battle.lastDamage);
            unit.StepTargetCount(1);
            unit.Damage(services, battle.lastDamage, 0, 0);
            battle.lastDamage = 0;
            break;

        case kSelfSpiritCommand:
            unit.SetDamageDigits(services, battle.spiritDamage);
            unit.StepTargetCount(1);
            unit.Damage(services, 0, battle.spiritDamage, 0);
            battle.spiritDamage = 0;
            break;

        default:
            unit.SetDamageDigits(services, battle.staminaDamage);
            unit.StepTargetCount(1);
            unit.Damage(services, 0, 0, battle.staminaDamage);
            battle.staminaDamage = 0;
            break;
    }

    battle.darkenUnit = -1;
    return 1;
}

// swd32.exe 0x00456680: one frame of battle role `role`; 1 always. It is drawn at rest (its acts holding while the
// field is darkened with word 2A74, or while paused). While the fight talk lets the battle run its effects tick and
// its gauge steps; a full gauge regenerates it and, for an AI role (autoControl, a guest guestControl), chooses its
// command at a random standing enemy (none: the turn resets), for a party role queues it (roleOrder; one kept from
// acting takes its turn at once). The first queued role not held takes the panel. A role waiting with its order
// (RoleTakeOrder) is slotted; its order runs (RoleOrderFrame) and the acting role acts (RoleActFrame). Unless it waits,
// a retreat (unk53BF1C) steps the roles back (RoleRetreatFrame). A defeat is taken out.
int Game::RoleFrame(int role)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto at = static_cast<std::size_t>(role);
    BattleRole& unit = this->battleRoles[at];
    const bool holding =
        (unit.GetWord2A74() != 0 && (battle.unk53BF94 == 1 || battle.unk53BF98 == 1)) || battle.paused == 1;
    unit.IdleFrame(services, holding ? 1 : 0);

    if (this->fightTalk.battleRuns == 1 && battle.unk53BFC0 == 0 && battle.unk53BFC4 == 0)
    {
        unit.TickEffects(services);

        if (unit.StepGauge(services, battle.enemyGaugeBoost) == 1)
        {
            unit.Regenerate(services);

            if (unit.autoControl == 1 || unit.guestControl == 1)
            {
                this->RoleAutoCommand(unit, role);
            }
            else
            {
                this->audio.PlaySound(kGaugeFullSound, this->config.soundVolume);
                auto free = std::find(battle.roleOrder.begin(), battle.roleOrder.end(), 0);

                if (free != battle.roleOrder.end())
                {
                    if (unit.StatusBit5Or8() == 1)
                    {
                        this->StartRoleTurn(role + BattleState::FirstRoleUnit);
                    }
                    else
                    {
                        *free = role + BattleState::FirstRoleUnit;
                    }
                }
            }
        }
    }

    for (std::size_t index = 0; index < battle.roleOrder.size(); ++index)
    {
        if (battle.currentUnit != 0 || battle.darkenUnit != -1 || battle.roleOrder[index] == 0)
        {
            break;
        }

        const auto queued = static_cast<std::size_t>(battle.roleOrder[index] - BattleState::FirstRoleUnit);

        if (this->battleRoles[queued].IsHeldOrHolding() != 1)
        {
            battle.currentUnit = battle.roleOrder[index];
            std::copy(
                battle.roleOrder.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                battle.roleOrder.end(),
                battle.roleOrder.begin() + static_cast<std::ptrdiff_t>(index)
            );
            battle.roleOrder.back() = 0;
            break;
        }
    }

    if (battle.unk524268[at] == 0 && unit.IsHeldOrHolding() == 0 && unit.IsAimed() == 1 && unit.IsAE4Clear() &&
        battle.hitsAllRoles == 0 && battle.roleActing == 0)
    {
        if (battle.unk4A7626 != kNoUnit && static_cast<i32>(battle.enemyCount) - battle.enemiesGone <= 1)
        {
            unit.ResetTurn(services);
        }

        if (battle.result == kEndCheckState && battle.pendingActionRow == 0 && battle.pendingAction == 0)
        {
            unit.ResetTurn(services);
        }

        battle.unk524268[at] = 1;
        battle.turnUnit = role + BattleState::FirstRoleUnit;
        battle.InsertSlotRecord(1, role + BattleState::FirstRoleUnit, -1);
    }

    if (unit.target == 1)
    {
        this->RoleOrderFrame(unit, role);
    }

    if (battle.actingUnit == role + BattleState::FirstRoleUnit)
    {
        if (!this->RoleActFrame(unit, role))
        {
            this->RoleFrameEnd(role);
            return 1;
        }
    }

    if (battle.hitsAllRoles == 0 && !unit.IsDefeated() && unit.guestControl == 0 && unit.autoControl == 0)
    {
        this->RoleRetreatFrame(unit, role);
    }

    this->RoleFrameEnd(role);
    return 1;
}

// The defeat of a role taken out at the end of RoleFrame (inline in the original).
void Game::RoleFrameEnd(int role)
{
    if (this->RemoveDefeated(role, 1) == 1)
    {
        this->battleState.damageShown = 0;
        this->battleState.lastRoleDefeated = kNoUnit;
    }
}

// The command of an AI role with a full gauge in RoleFrame (inline in the original): with an enemy standing it aims at
// one at random (aiTarget, 1-based) and chooses its command (AllAtOnce and TargetsAll marking its order); otherwise its
// turn resets.
void Game::RoleAutoCommand(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;
    u32 defeated = 0;

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        if (this->battleEnemies[index].IsDefeated())
        {
            ++defeated;
        }
    }

    if (defeated >= battle.enemyCount)
    {
        unit.ResetTurn(this->battleServices);
        return;
    }

    unit.SetTarget(1);

    do
    {
        battle.aiTarget = static_cast<i32>(this->random.Below(battle.enemyCount)) + 1;
    } while (this->battleEnemies[static_cast<std::size_t>(battle.aiTarget - 1)].IsDefeated());

    battle.unk53BF60 = 1;
    const auto at = static_cast<std::size_t>(role);
    u32 act = static_cast<u32>(battle.unk4FE5D4[at]);
    const int chosen = unit.ChooseCommand(this->battleServices, this->database, act);
    battle.unk4FE5D4[at] = static_cast<i32>(act);

    if (chosen != 1)
    {
        return;
    }

    if (unit.AllAtOnce() == 1)
    {
        battle.pendingOrders[at].allAtOnce = 1;
    }

    if (unit.TargetsAll() == 1)
    {
        battle.pendingOrders[at].targetsAll = 1;
    }
}

// The order of a role whose target is set in RoleFrame (inline in the original). A sweep order (targetsAll) gathers its
// targets: the roles in battle standing not autoControl/guestControl or StatusBit13 (an order atParty), or
// the enemies standing without a pending removal (the count in unk10), and with every one gathered aims at the last
// role or the first enemy standing (unk53BF80); the order then takes its command (RoleTakeOrder). A single order takes
// its command at its aim: a role (atParty: aimTarget, 1-based), an AI role's random enemy (aiTarget) or the menu's
// enemy (aimTarget).
void Game::RoleOrderFrame(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;
    const auto at = static_cast<std::size_t>(role);
    PendingOrder& order = battle.pendingOrders[at];
    const bool automatic = unit.autoControl != 0 || unit.guestControl != 0;

    auto taken = [&battle]
    {
        battle.unk53BF60 = 0;
        battle.unk53BF5C = 0;
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.turnUnit = 0;
    };

    if (order.targetsAll != 1)
    {
        if (order.atParty != 0)
        {
            BattleRole& aimed = this->battleRoles[static_cast<std::size_t>(battle.aimTarget - 1)];

            if (aimed.GetValueAAC() != 0 || battle.actingUnit == battle.aimTarget + (BattleState::FirstRoleUnit - 1) ||
                unit.IsAimed() != 0)
            {
                return;
            }

            aimed.SetAC0(0);
            unit.SetAD4(0);

            if (unit.SetCommand(battle.turnTaken[at + static_cast<std::size_t>(BattleState::FirstRoleUnit)]) == 1)
            {
                unit.Retarget(static_cast<i16>(battle.aimTarget - 1));
                unit.PrepareCommand(this->battleServices, this->database);
                unit.SetTarget(0);
                taken();
                battle.menuCommand = 1;
                battle.pageListRow = 1;
            }

            return;
        }

        if (unit.IsAimed() != 0)
        {
            return;
        }

        unit.SetAD4(0);

        if (automatic)
        {
            BattleEnemy& aimed = this->battleEnemies[static_cast<std::size_t>(battle.aiTarget - 1)];
            aimed.SetAC0(0);
            aimed.IncreaseCounter2A76();
            unit.Retarget(static_cast<i16>(battle.aiTarget - 1));
            unit.SetTarget(0);
            taken();
            battle.aiTarget = 1;
            return;
        }

        if (unit.SetCommand(battle.turnTaken[at + static_cast<std::size_t>(BattleState::FirstRoleUnit)]) == 1)
        {
            BattleEnemy& aimed = this->battleEnemies[static_cast<std::size_t>(battle.aimTarget - 1)];
            aimed.SetAC0(0);
            aimed.IncreaseCounter2A76();
            unit.Retarget(static_cast<i16>(battle.aimTarget - 1));
            unit.SetTarget(0);
            unit.PrepareCommand(this->battleServices, this->database);
            taken();
            battle.pageListRow = 1;
            battle.specialRow = 1;
        }

        return;
    }

    this->RoleGatherSweep(unit, order);
    this->RoleTakeOrder(unit, role);
}

// The targets of a sweep order of RoleOrderFrame (inline in the original); see there.
void Game::RoleGatherSweep(BattleRole& unit, PendingOrder& order)
{
    BattleState& battle = this->battleState;
    i32 aim = -1;

    // The rebuild (deviations.md): the side is the order's own, as it hits; the original reads gathersParty, cleared
    // when another role's action ends or its order is queued while this one aims.
    if (order.atParty != 0)
    {
        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& other = this->battleRoles[index];

            if (other.autoControl != 1 && other.guestControl != 1 && other.StatusBit13() != 1 && unit.IsAimed() == 0)
            {
                other.SetAC0(0);
                ++order.targetCount;
            }
        }

        const auto standing =
            static_cast<i32>(battle.roleCount) - battle.defeatedRoles - battle.escortCount - battle.guestCount;

        if (order.targetCount < standing)
        {
            return;
        }

        for (i32 index = static_cast<i32>(battle.roleCount) - 1; index >= 0; --index)
        {
            if (!this->battleRoles[static_cast<std::size_t>(index)].IsDefeated())
            {
                aim = index;
                break;
            }
        }
    }
    else
    {
        i32 skipped = 0;

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = this->battleEnemies[index];

            if (enemy.IsDefeated() || battle.unk502984[index] != -1)
            {
                ++skipped;
                continue;
            }

            enemy.SetAC0(0);
            enemy.IncreaseCounter2A76();
            ++order.targetCount;
        }

        if (order.targetCount < static_cast<i32>(battle.enemyCount) - skipped)
        {
            return;
        }

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            if (!this->battleEnemies[index].IsDefeated())
            {
                aim = static_cast<i32>(index);
                break;
            }
        }
    }

    if (aim < 0)
    {
        return;
    }

    battle.unk53BF80 = 1;
    unit.SetAA8();
    unit.Retarget(static_cast<i16>(aim));
}

// A sweep order takes its command in RoleOrderFrame (inline in the original): an AI role just leaves the menu; a party
// role takes the command of its menu (command 4 also takes its command 2F18) and the menu resets.
void Game::RoleTakeOrder(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;

    if (unit.GetAA8() != 1)
    {
        return;
    }

    if (unit.autoControl != 0 || unit.guestControl != 0)
    {
        battle.unk53BF60 = 0;
        unit.SetTarget(0);
        battle.unk53BF5C = 0;
        battle.unk53BF88 = 0;
        battle.unk53BF80 = 0;
        return;
    }

    const i32 command = battle.turnTaken[static_cast<std::size_t>(role + BattleState::FirstRoleUnit)];

    if (unit.SetCommand(command) != 1)
    {
        return;
    }

    unit.SetAD4(0);
    battle.unk53BF60 = 0;
    unit.SetTarget(0);

    if (command == kMagicAttackCommand)
    {
        unit.TakeCommand2F18();
    }

    battle.unk53BF5C = 0;
    battle.unk53BF88 = 0;
    battle.unk53BF80 = 0;
    battle.menuCommand = 1;
    battle.pageListRow = 1;
    battle.aimTarget = 1;
    battle.specialRow = 1;
    battle.unk53BFC4 = 0;
    battle.unk53BFC0 = 0;
    battle.turnUnit = 0;
}

// The acting role of RoleFrame (inline in the original); false when the battle waits (hitsAllRoles) before it acts.
// Once its target stands (unkAAC clear) the role starts acting (retargeted): a sweep needs an enemy standing (each one
// counting a hit); a single hit at a defeated enemy passes to the first enemy standing (unless the absorb holds it,
// unk53C02C), counting a hit; the skill name shows and the cost is paid. While acting it runs RoleActionFrame; once
// that is over the action ends (RoleAfterAction).
bool Game::RoleActFrame(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;

    if (battle.retargeted != 0)
    {
        battle.roleActing = 1;
        battle.unk53BFC0 = 1;
        battle.darkenUnit = static_cast<i16>(battle.actingUnit);
        unit.MarkAC8();

        if (this->RoleActionFrame(role, static_cast<i16>(unit.GetWord29A2())) == 1)
        {
            this->RoleAfterAction(unit, role);
        }

        return true;
    }

    if (battle.hitsAllRoles != 0)
    {
        return false;
    }

    BattleEnemy& aimed = this->battleEnemies[static_cast<std::size_t>(static_cast<i16>(unit.GetWord29A2()))];

    if (aimed.GetValueAAC() != 0)
    {
        return true;
    }

    battle.retargeted = 1;
    battle.darkenUnit = static_cast<i16>(battle.actingUnit);

    if (unit.GetAA8() != 0)
    {
        if (battle.atParty == 0)
        {
            bool standing = false;

            for (u32 index = 0; index < battle.enemyCount; ++index)
            {
                BattleEnemy& enemy = this->battleEnemies[index];

                if (!enemy.IsDefeated())
                {
                    enemy.IncreaseHits();
                    standing = true;
                }
            }

            if (!standing)
            {
                battle.retargeted = 0;
                return true;
            }
        }
    }
    else if (battle.atParty == 0 && battle.pendingAction == 0)
    {
        if (!aimed.IsDefeated() || battle.unk53C02C != 0)
        {
            aimed.IncreaseHits();
        }
        else
        {
            u32 first = 0;

            while (first < battle.enemyCount && this->battleEnemies[first].IsDefeated())
            {
                ++first;
            }

            if (first >= battle.enemyCount)
            {
                battle.retargeted = 0;
                return true;
            }

            // The original takes a hit from the unit before the first enemy here: not done.
            unit.ClearWord29A2();
            unit.Retarget(static_cast<i16>(first));
            this->battleEnemies[first].IncreaseHits();
        }
    }

    if (battle.retargeted == 1)
    {
        if (unit.skillName[0] != '\0')
        {
            this->battleMessages.Add(kNoticeX, 0, kSkillNameFrames, unit.skillName, BattleMessage::Rise);
            battle.listName = {};
        }

        unit.FinishCommand();
    }

    return true;
}

// The end of a role's action in RoleActFrame (inline in the original): the target loses a hit (a sweep: every enemy)
// and the roles aiming at it turn away (RetargetRoles); a defeated enemy target frees the role it held, and with one
// enemy left the roles' effects end; the roles face again after a facing change; the role's turn and the acting state
// reset, the lists and marks of the menu too.
void Game::RoleAfterAction(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto target = static_cast<i16>(unit.GetWord29A2());
    unit.ClearWord29A2();
    battle.roleActing = 0;
    battle.shake = 0;
    battle.unk53BF03 = 0;
    battle.unk5240C4 = 0;
    battle.unk53BF74 = 0;
    battle.unk53BFC0 = 0;
    battle.unk53BFC4 = 0;
    battle.gathersParty = 0;
    battle.retargeted = 0;

    if (target != -1)
    {
        if (unit.GetAA8() == 1)
        {
            for (u32 index = 0; index < battle.enemyCount; ++index)
            {
                this->battleEnemies[index].DecreaseHits();
            }
        }
        else if (battle.atParty != 0)
        {
            this->battleRoles[static_cast<std::size_t>(target)].DecreaseHits();
        }
        else
        {
            this->battleEnemies[static_cast<std::size_t>(target)].DecreaseHits();
        }

        this->RetargetRoles(role, target);
        BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(target)];

        if (enemy.IsDefeated())
        {
            const auto held = static_cast<i16>(enemy.GetWord29A2());

            if (held != -1)
            {
                this->battleRoles[static_cast<std::size_t>(held)].DecreaseHits();
            }

            if (static_cast<i32>(battle.enemyCount) + battle.unk53BF02 - battle.enemiesGone <= 1)
            {
                for (u32 index = 0; index < battle.roleCount; ++index)
                {
                    this->battleRoles[index].ExpireEffects(services, 1);
                }
            }
        }

        if ((battle.panelFlags & kFacingPanel) != 0)
        {
            for (u32 index = 0; index < battle.enemyCount; ++index)
            {
                BattleEnemy& other = this->battleEnemies[index];

                if (!other.IsDefeated())
                {
                    other.SetFacingIfAble(0);
                }
            }

            battle.panelFlags &= ~kFacingPanel;
        }
    }

    unit.ResetTurn(services);
    battle.ClearActing();
    const auto at = static_cast<std::size_t>(role);
    battle.nameFlashes[at] = 0;
    battle.unk4FE5D4[at] = 0;
    battle.roleListRows.fill(1);
    battle.unk524268[at] = 0;
    battle.roleListScrolls.fill(0);
    battle.unk53BF46 = 0;
    battle.unk53BF48 = 0;
    battle.actingUnit = -1;
    battle.proficiencyBonus = 0;
    battle.spellStage = 0;
    battle.orderSorted = 0;

    if (battle.unk53BF94 == 1)
    {
        battle.undarkening = 1;
    }

    if ((this->fightTalk.unk53CCDC & kTalkCountdown) != 0)
    {
        --this->fightTalk.unk53CCDC;
    }
}

// The retreat of RoleFrame (inline in the original). While asked (kRetreatAsked) each role steps back; once all have
// (sweptUnits), the first role's level is rolled against the highest level of the enemies standing (200 while the
// removal of the enemy of the role's index is pending, kept): won, the retreat runs (kRetreating); lost, the roles
// come back with 撤退失敗!!. While retreating each role leaves once (its bit), and with every role gone 撤退成功!!
// ends the battle (state 0x68).
void Game::RoleRetreatFrame(BattleRole& unit, int role)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    const auto partyLeft = [&battle]
    {
        return static_cast<i32>(battle.roleCount) - battle.defeatedRoles - battle.escortCount - battle.guestCount;
    };

    if ((battle.unk53BF1C & kRetreatAsked) != 0)
    {
        battle.unk53BFC0 = 1;
        battle.unk53BFC4 = 1;

        if (unit.StepBack(services, 0) == 1 && ++battle.sweptUnits >= static_cast<u32>(partyLeft()))
        {
            battle.levelUpFrames = 0;

            for (u32 index = 0; index < battle.enemyCount; ++index)
            {
                BattleEnemy& enemy = this->battleEnemies[index];

                if (enemy.IsDefeated())
                {
                    continue;
                }

                battle.lastDefeatedObject = enemy.Data();

                if (battle.lastDefeatedObject != nullptr &&
                    battle.lastDefeatedObject->data.level > battle.levelUpFrames)
                {
                    battle.levelUpFrames = battle.lastDefeatedObject->data.level;
                }

                if (battle.unk502984[static_cast<std::size_t>(role)] != -1)
                {
                    battle.levelUpFrames = kRetreatLevel;
                }
            }

            const u16 level = battle.levelUpFrames;

            if (level == 0)
            {
                battle.unk53BF1C = 0;
                battle.sweptUnits = 1;
                battle.levelUpFrames = 0;
                battle.unk53BFC0 = 0;
                battle.unk53BFC4 = 0;
            }

            if (this->battleRoles[0].BeatsLevel(services, level) == 1)
            {
                unit.SetValueAAC(1);
                battle.unk53BF1C = kRetreating;
                battle.unk53BF5C = 0;
                battle.currentUnit = 0;
                battle.levelUpFrames = 0;
                battle.sweptUnits = static_cast<u16>(partyLeft());
            }
            else
            {
                for (u32 index = 0; index < battle.roleCount; ++index)
                {
                    this->battleRoles[index].SetValueAAC(0);
                }

                if (this->battleMessages.messages == nullptr)
                {
                    this->battleMessages.Add(kNoticeX, kNoticeY, kRetreatFrames, kRetreatFailed, kNoticeFlags);
                }

                this->audio.PlaySound(kRetreatFailedSound, this->config.soundVolume);
                battle.unk53BF1C = 0;
                battle.sweptUnits = 1;
                battle.levelUpFrames = 0;
                battle.unk53BFC0 = 0;
                battle.unk53BFC4 = 0;
            }
        }
    }

    if ((battle.unk53BF1C & kRetreating) == 0)
    {
        return;
    }

    battle.unk53BFC0 = 1;
    battle.unk53BFC4 = 1;
    battle.unk53BF5C = 0;
    battle.currentUnit = 0;
    const auto bit = static_cast<u16>(1 << role);

    if ((battle.unk53BF1C & kRetreatRoles & bit) != 0 || unit.StepBack(services, 1) != 1)
    {
        return;
    }

    battle.enemyFaded = 1;
    battle.unk53BF1C = static_cast<u16>(battle.unk53BF1C | bit);

    if (++battle.defeatedRoles >= static_cast<u32>(battle.roleCount - battle.escortCount - battle.guestCount))
    {
        this->battleMessages.Add(kNoticeX, kNoticeY, kRetreatFrames, kRetreated, kNoticeFlags);
        battle.roleOrder.fill(0);
        battle.result = kRetreatedState;
        battle.unk53BF1C = 0;
        battle.actingUnit = -1;
        battle.slotRecords.fill(BattleSlotRecord{0, 0, 0, 0, 0, 0, 0, 0});
    }
}
}  // namespace swd3
