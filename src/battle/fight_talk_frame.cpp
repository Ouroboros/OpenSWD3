#include "game/game.h"

#include <algorithm>
#include <cstring>

#include "core/fault.h"
#include "core/format.h"
#include "core/spline.h"

namespace swd3
{

namespace
{

constexpr u16 kTalkEnd = 0xFFFF;  // FightTalkFrame: the commands of the battle talk
constexpr u16 kTalkWait = 1;
constexpr u16 kTalkWindow = 2;
constexpr u16 kTalkStep = 3;
constexpr u16 kTalkSetUnk4A7B54 = 4;
constexpr u16 kTalkMove = 5;
constexpr u16 kTalkPause = 6;
constexpr u16 kTalkHitPoints = 8;
constexpr u16 kTalkOrder = 9;
constexpr u16 kCountdown = 0x8000;  // FightTalk::unk53CCDC and unk53CCD4: counting
constexpr u16 kCountMask = 0x7FFF;
constexpr u16 kWindowShown = 0x8000;  // the high word of FightTalk::unk53CE8C
constexpr int kTextEndSearch = 0xFF;
constexpr char kTextEscape = '%';
constexpr char kTextEnd = 'Q';
constexpr u32 kNoTimeout = 0xFFFF;
constexpr u16 kWindowSpeed = 2;
constexpr i16 kTalkAction = 0x24;
constexpr i32 kLastEnemy = 7;
constexpr u16 kOrderForced = 0x8000;  // BattleState::enemyOrders
constexpr int kEnemySlot = 2;
constexpr int kHangDraws = 0x10000;
constexpr std::size_t kTalkEnemies = 10;  // the enemies the original walks (two past the enemy array)
constexpr u32 kLowWordMask = 0xFFFF;
constexpr int kHighWordShift = 16;
constexpr int kEscapeCommand = 0x11;
constexpr int kAimSelfCommand = 0xB;
constexpr int kHoldCommand = 0xC;
constexpr i32 kLastPlayState = 0x61;
constexpr i32 kEndCheckState = 0x63;
constexpr i32 kEndLootState = 0x66;
constexpr u16 kListEnd = 0xFFFF;
constexpr std::size_t kListTail = 4;
constexpr u16 kOrderSpell = 0x4000;
constexpr i32 kSpellOrder = 2;
constexpr u16 kTalkDefeat = 0xA;
constexpr u16 kTalkEscape = 0xB;
constexpr u16 kTalkWin = 0xC;
constexpr u16 kTalkLift = 0xD;
constexpr u16 kTalkAllDone = 0xE;
constexpr u16 kTalkSetMarks = 0xF;
constexpr u16 kTalkRepeat = 0x10;
constexpr u16 kTalkSkip = 0x11;
constexpr u16 kTalkClearMarks = 0x12;
constexpr u16 kTalkRandomJump = 0x13;
constexpr u16 kTalkFace = 0x14;
constexpr u16 kTalkAimSelf = 0x15;
constexpr u16 kTalkShift = 0x16;
constexpr u16 kTalkSkill = 0x17;
constexpr u16 kTalkNextBattle = 0x18;
constexpr u16 kTalkLoot = 0x19;
constexpr u16 kTalkHold = 0x1A;
constexpr u16 kTalkSetAction = 0x1B;
constexpr u16 kTalkVitals = 0x1C;
constexpr u16 kTalkToggle = 0x1D;
constexpr u16 kTalkSound = 0x1E;
constexpr u16 kTalkChoice = 0x1F;
constexpr u16 kTalkHeadIn = 0x21;
constexpr u16 kTalkHeadOut = 0x22;
constexpr u16 kTalkFlagsA0 = 0x23;
constexpr u16 kTalkLines = 0x24;
constexpr u16 kTalkLinesMode = 0x25;
constexpr u16 kTalkCurve = 0x27;
constexpr u16 kTalkSlide = 0x28;
constexpr u16 kTalkIntro = 0x29;
constexpr u16 kTalkSpeaker = 0x2A;
constexpr u16 kTalkQuietWin = 0x2B;
constexpr u16 kTalkTimedWindow = 0x2C;
constexpr u16 kTalkMirror = 0x2D;
constexpr u16 kTalkSetFlag = 0x2E;
constexpr u16 kTalkClearFlag = 0x2F;
constexpr u16 kTalkFlagJump = 0x30;
constexpr u16 kTalkPartyGone = 0x31;
constexpr u16 kTalkJoinFirst = 0x32;
constexpr u16 kTalkDefeatPose = 0x33;
constexpr u16 kTalkAttributes = 0x34;
constexpr u16 kTalkAddLoot = 0x35;
constexpr u16 kTalkEnemyCommand = 0x36;
constexpr u16 kTalkLearn = 0x37;
constexpr u16 kTalkWords66 = 0x38;
constexpr u16 kTalkBackgroundAgain = 0x39;
constexpr u16 kTalkPartyOrder = 0x3A;
constexpr u16 kTalkLineWindow = 0x3B;
constexpr u16 kTalkMusic = 0x3C;
constexpr u16 kTalkAllCounted = 0x3D;
constexpr u16 kTalkFieldFade = 0x3E;
constexpr u16 kTalkFadeWait = 0x3F;
constexpr u16 kTalkPictureWait = 0x40;
constexpr u16 kTalkHasObjects = 0x41;
constexpr u16 kTalkTakeObjects = 0x42;
constexpr u16 kTalkCritical = 0x43;
constexpr u16 kTalkPlace = 0x44;
constexpr u16 kTalkPanel8 = 0x45;
constexpr u16 kTalkLevel = 0x46;
constexpr u16 kTalkLose = 0x47;
constexpr u16 kTalkBackground = 0x48;
constexpr u16 kTalkSlideTo = 0x49;
constexpr u16 kTalkResistances = 0x4A;
constexpr u16 kTalkWords72 = 0x4B;
constexpr u16 kTalkKill = 0x4C;
constexpr u16 kTalkPanel40 = 0x4D;
constexpr u16 kTalkCatch = 0x4E;
constexpr u16 kTalkDamage = 0x4F;
constexpr u16 kTalkReload = 0x50;
constexpr u16 kTalkUsedItem = 0x51;
constexpr u16 kTalkQuake = 0x52;
constexpr u16 kTalkNoRetreat = 0x53;
constexpr u32 kWindowFlagsA0 = TalkWindow::Overlay | TalkWindow::Skippable;  // command 0x23: for the next window
constexpr std::u8string_view kErrorTitle = u8"錯誤";                         // 0x004A60B0
constexpr std::u8string_view kWrongButtons = u8"按鈕個數不對!!";             // 0x004A7BAC
constexpr u16 kEffectNumbers = 0x100;                                        // ScriptEffect numbers
constexpr u16 kEvenPixels = 0xFFFE;
constexpr u16 kRisingLines = 1;
constexpr u16 kStillLines = 2;
constexpr i16 kLineStart = 2;
constexpr u16 kOpenLines = 0;
constexpr u16 kCloseLines = 1;
constexpr u16 kEndLines = 2;
constexpr u16 kCurveSpans = 6;
constexpr std::size_t kCurveBytes = 0x18;
constexpr std::size_t kCurveOffsets = 5;
constexpr u16 kCurveSteps = 0x14;
constexpr u16 kSlideByThirds = 0x10;
constexpr int kSlideThird = 3;
constexpr int kSlideThirdNegative = -3;
constexpr i32 kNoTalkTimeout = 0xFFFF;
constexpr i32 kMirroredPanelSpan = 0x270;
constexpr i32 kQuakeFrames = 2;
constexpr u8 kNoRolesLeft = 0xFF;
constexpr i32 kBackgroundPanel = 1;  // BattleState::panelFlags
constexpr i32 kLostPanel = 0x10;
constexpr i32 kNoFirstStrikePanel = 0x40;
constexpr i32 kBattleLostState = 0x67;
constexpr int kCatchCommand = 6;
constexpr i16 kMaxVital = 0x270F;
constexpr int kEnemyKillHeal = -100000;
constexpr std::size_t kResistanceWords = 9;
constexpr std::size_t kCountedSkipWords = 4;

}  // namespace

// swd32.exe 0x00469D20: one step of the battle talk (figtalk.dat): the command at the cursor runs, most of them
// with a battle frame. 0 once the talk and the battle are over (command 0xFFFF); otherwise 1, or the end a frame
// returned (command 1). Unknown commands return 1 without moving (the original stays on them).
int Game::FightTalkFrame()
{
    FightTalk& talk = this->fightTalk;

    const u16 command = talk.UnsignedWord(0);

    switch (command)
    {
        case kTalkEnd:
            this->FightTalkEnd();
            return 0;

        case kTalkWait:
            return this->FightTalkWait();

        case kTalkWindow:
            return this->FightTalkWindow();

        case kTalkStep:
            talk.unk4A7B58 = 0;
            this->BattleFrame();
            talk.cursor += sizeof(u16);
            talk.unk4A7B58 = 1;
            return 1;

        case kTalkSetUnk4A7B54:
            talk.unk4A7B54 = talk.Word(1);
            this->BattleFrame();
            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkMove:
            return this->FightTalkMove();

        case kTalkPause:
            return this->FightTalkPause();

        case kTalkHitPoints:
            return this->FightTalkHitPoints();

        case kTalkOrder:
            return this->FightTalkOrder();

        case kTalkDefeat:
            return this->FightTalkDefeat();

        case kTalkEscape:
            return this->FightTalkCommand(kEscapeCommand);

        case kTalkWin:
            return this->FightTalkWin();

        case kTalkLift:
            return this->FightTalkLift();

        case kTalkAllDone:
            return this->FightTalkAllDone();

        case kTalkSetMarks:
            return this->FightTalkMarks(true);

        case kTalkRepeat:
            return this->FightTalkRepeat();

        case kTalkSkip:
            talk.cursor += sizeof(u16);
            return 1;

        case kTalkClearMarks:
            return this->FightTalkMarks(false);

        case kTalkRandomJump:
            return this->FightTalkRandomJump();

        case kTalkFace:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            this->TalkUnit(static_cast<i16>(this->TalkCommandUnit())).SetFacing(1);
            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkAimSelf:
            return this->FightTalkCommand(kAimSelfCommand);

        case kTalkShift:
            return this->FightTalkShift();

        case kTalkSkill:
            return this->FightTalkSkill();

        case kTalkNextBattle:
            return this->FightTalkNextBattle();

        case kTalkLoot:
            return this->FightTalkLoot();

        case kTalkHold:
            return this->FightTalkCommand(kHoldCommand);

        default:
            if (command >= kTalkSetAction && command <= kTalkLinesMode)
            {
                return this->FightTalkUnits(command);
            }

            return this->FightTalkMisc(command);
    }
}

// Every unit reset, the battle ended and the talk closed (inline in 0x00469D20, command 0xFFFF and the end of a battle
// run by command 1).
void Game::FightTalkEnd()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        this->battleEnemies[index].Reset(this->battleServices);
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        this->battleRoles[index].Reset(this->battleServices);
    }

    this->fightTalk.unk53CCDC = 0;
    this->EndBattle();
    this->fightTalk.Close();
}

// Command 1 of FightTalkFrame (inline in the original): the battle runs (its frames) for the count given; a frame
// that ends the battle ends the talk too (its end returned). With no count the talk goes on at once and the unit
// kept by command 9 takes the panel again.
int Game::FightTalkWait()
{
    FightTalk& talk = this->fightTalk;

    if ((talk.unk53CCDC & kCountdown) == 0)
    {
        talk.unk53CCE4 = talk.UnsignedWord(1);
        talk.unk53CCDC = static_cast<u16>(talk.unk53CCE4 | kCountdown);
        return 1;
    }

    if ((talk.unk53CCDC & kCountMask) == 0)
    {
        talk.unk53CCDC = 0;
        talk.cursor += 2 * sizeof(u16);
        talk.enabled = 1;

        if (talk.unk53CE6C != 0)
        {
            this->battleState.currentUnit = talk.unk53CE6C;
        }

        talk.unk53CE6C = 0;
        return 1;
    }

    talk.unk4A7B58 = 1;
    talk.unk53CCE8 = this->BattleFrame();

    if (talk.unk53CCE8 == 1)
    {
        return 1;
    }

    const i32 end = talk.unk53CCE8;
    this->FightTalkEnd();
    talk.cursor += 2 * sizeof(u16);
    talk.unk53CCDC = 0;
    talk.enabled = 1;
    return end;
}

// Command 2 of FightTalkFrame (inline in the original): a talk window of the unit in the command's second word (the
// text after it, ended by %Q) opens at the unit's place (FixedPlace, no timeout, speed 2, the flags left in the low
// word of unk53CE8C) and the battle waits; every unit unmarked (a role speaker takes action 0x24). Once the window is
// closed the roles take action 0 again and the talk goes on after the text.
int Game::FightTalkWindow()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    auto high = [&talk]
    {
        return static_cast<u16>(static_cast<u32>(talk.unk53CE8C) >> (sizeof(u16) * 8));
    };

    auto setHigh = [&talk](u16 value)
    {
        talk.unk53CE8C = static_cast<i32>(
            (static_cast<u32>(talk.unk53CE8C) & kLowWordMask) | (static_cast<u32>(value) << (sizeof(u16) * 8))
        );
    };

    if ((high() & kWindowShown) == 0)
    {
        setHigh(talk.UnsignedWord(1));
        talk.textAt = talk.cursor + sizeof(u16);
        auto window = std::make_unique<TalkWindow>();
        const u16 speaker = high();
        u16 x = 0;
        u16 y = 0;

        if (speaker <= kLastEnemy)
        {
            this->battleEnemies[speaker].GetDrawPosition(x, y);

            for (BattleEnemy& enemy : this->battleEnemies)
            {
                enemy.SetAC0(0);
            }
        }
        else
        {
            BattleRole& role = this->battleRoles[static_cast<std::size_t>(speaker - BattleState::FirstRoleUnit)];
            role.GetDrawPosition(x, y);

            for (BattleRole& other : this->battleRoles)
            {
                other.SetAC0(0);
            }

            role.SetAction(kTalkAction);
        }

        talk.unk53CE78 = static_cast<i32>(x | (static_cast<u32>(y) << (sizeof(u16) * 8)));
        window->flags = TalkWindow::FixedPlace;
        const std::span<const u8> text =
            std::span<const u8>(talk.script).subspan(std::min(talk.textAt, talk.script.size()));
        const std::string_view name(talk.speaker.data(), strnlen(talk.speaker.data(), talk.speaker.size()));
        this->talk.SetupDialog(*window, text, name, Talk::SizedLayout, 0, 0);
        talk.window = window.get();
        this->talk.AppendWindow(std::move(window));
        talk.window->timeout |= kNoTimeout;
        talk.window->speed = kWindowSpeed;
        talk.window->flags |= static_cast<u16>(talk.unk53CE8C);
        talk.unk53CE8C = static_cast<i32>(static_cast<u32>(talk.unk53CE8C) & ~kLowWordMask);
        talk.window->fromX = x;
        talk.window->fromY = y;
        setHigh(static_cast<u16>(high() | kWindowShown));
        battle.unk53BFC0 = 1;
        battle.result = 0;
    }

    if (this->scene.talkWindows != nullptr)
    {
        talk.unk4A7B58 = 0;
        this->BattleFrame();
        return 1;
    }

    for (BattleRole& role : this->battleRoles)
    {
        role.SetAC0(0);
        role.SetAction(0);
    }

    // The original walks ten enemies here, two past the enemy array.
    for (std::size_t index = 0; index < std::min(kTalkEnemies, this->battleEnemies.size()); ++index)
    {
        this->battleEnemies[index].SetAC0(0);
    }

    int length = 0;

    while (length < kTextEndSearch)
    {
        const std::size_t at = talk.textAt + static_cast<std::size_t>(length);

        if (at + 1 < talk.script.size() && talk.script[at] == kTextEscape && talk.script[at + 1] == kTextEnd)
        {
            break;
        }

        ++length;
    }

    talk.textAt += static_cast<std::size_t>(length) + 2;
    talk.cursor = talk.textAt;
    talk.speaker = {};
    talk.unk4A7B58 = 1;
    battle.unk53BFC0 = 0;
    talk.unk53CE78 = 0;
    setHigh(0);
    return 1;
}

// Command 5 of FightTalkFrame (inline in the original): the unit of the second word moves by the third and fourth
// (a role from 8); the units are measured and sorted again (drawn at once with unk53CEB0).
int Game::FightTalkMove()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.unk4A7B58 = 0;
    const i16 unit = talk.Word(1);
    talk.unk53CE74 = talk.UnsignedWord(2);
    talk.unk53CE76 = talk.UnsignedWord(3);
    BattleUnit& moved =
        unit >= BattleState::FirstRoleUnit
            ? static_cast<BattleUnit&>(this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)])
            : static_cast<BattleUnit&>(this->battleEnemies[static_cast<std::size_t>(unit)]);
    u16 x = 0;
    u16 y = 0;
    moved.GetPosition(x, y);
    talk.unk53CCE8 = x + static_cast<i16>(talk.unk53CE74);
    talk.unk53CCEC = y + static_cast<i16>(talk.unk53CE76);
    moved.SetPosition(static_cast<i16>(talk.unk53CCE8), static_cast<i16>(talk.unk53CCEC));
    talk.unk53CE78 = 0;
    talk.cursor += 4 * sizeof(u16);
    talk.unk53CCE8 = 0;
    talk.unk53CCEC = 0;
    talk.unk53CE74 = 1;
    this->MeasureUnits();
    battle.SortDrawOrder();
    battle.ListEnemyOrder();

    if (talk.unk53CEB0 == 1)
    {
        this->BattleFrame();
    }

    talk.unk4A7B58 = 1;
    return 1;
}

// Command 6 of FightTalkFrame (inline in the original): the talk waits (battle frames without the talk) for the
// frames given.
int Game::FightTalkPause()
{
    FightTalk& talk = this->fightTalk;
    talk.unk4A7B58 = 0;

    if ((talk.unk53CCD4 & kCountdown) == 0)
    {
        talk.unk53CCD4 = talk.Word(1) | kCountdown;
        this->BattleFrame();
        return 1;
    }

    if ((talk.unk53CCD4 & kCountMask) != 0)
    {
        --talk.unk53CCD4;
    }
    else
    {
        talk.unk53CCD4 = 0;
        talk.cursor += 2 * sizeof(u16);
        talk.unk4A7B58 = 1;
    }

    this->BattleFrame();
    return 1;
}

// Command 8 of FightTalkFrame (inline in the original): after a frame, with the third word 1 the talk jumps to the
// place in the fifth word when the hit points of the unit in the second are at most the fourth; otherwise it goes on
// after the command (another third word: it stays).
int Game::FightTalkHitPoints()
{
    FightTalk& talk = this->fightTalk;
    talk.cursor += sizeof(u16);
    talk.unk4A7B58 = 1;
    this->BattleFrame();
    const i16 unit = talk.Word(0);

    if (talk.UnsignedWord(1) != 1)
    {
        return 1;
    }

    BattleUnit& checked =
        unit > kLastEnemy
            ? static_cast<BattleUnit&>(this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)])
            : static_cast<BattleUnit&>(this->battleEnemies[static_cast<std::size_t>(unit)]);
    checked.GetHitPoints(talk.unk53CCE8, talk.unk53CCEC);

    if (talk.unk53CCE8 <= talk.Word(2))
    {
        const i16 place = talk.Word(3);
        talk.cursor += 3 * sizeof(u16);
        talk.Seek(place);
        return 1;
    }

    talk.cursor += 5 * sizeof(u16);
    return 1;
}

// Command 9 of FightTalkFrame (inline in the original): a role (from 8) is set to attack the enemy in the third word
// (from 8 a role, kept for its party order) and the current unit is kept; an enemy is ordered to attack the unit in the
// third word (a defeated enemy target passes to the next standing, a defeated role target to a random standing one),
// the order forced and its slot record put first.
int Game::FightTalkOrder()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    const i16 unit = talk.Word(1);
    const i16 target = talk.Word(2);

    if (unit > kLastEnemy)
    {
        talk.unk53CE6C = battle.currentUnit;
        battle.unk53BD50 = unit;
        battle.unk4A754C = target + 1;

        if (battle.unk4A754C > kLastEnemy)
        {
            battle.unk53BFD0 = 1;
            battle.unk4A754C -= BattleState::FirstRoleUnit;
            battle.pendingOrders[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].unk00 = 1;
        }

        this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].SetTarget(1);
        battle.unk53BCE8 = 1;
        battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = 1;
    }
    else
    {
        u16& order = battle.enemyOrders[static_cast<std::size_t>(unit)];
        order = 0;
        talk.unk53CCE8 = target;

        if (target > kLastEnemy)
        {
            this->TalkStandingRole(talk.unk53CCE8);
            order = static_cast<u16>(talk.unk53CCE8 - BattleState::FirstRoleUnit);
        }
        else
        {
            if (this->battleEnemies[static_cast<std::size_t>(target)].IsDefeated())
            {
                while (++talk.unk53CCE8 <= kLastEnemy &&
                       this->battleEnemies[static_cast<std::size_t>(talk.unk53CCE8)].IsDefeated())
                {
                }
            }

            order = static_cast<u16>(talk.unk53CCE8);
            battle.unk53BFD4 = 1;
            battle.unk53C014 = 1;
        }

        order = static_cast<u16>(order | kOrderForced);
        battle.InsertSlotRecord(kEnemySlot, unit, 0);
    }

    talk.unk4A7B58 = 0;
    this->BattleFrame();
    talk.unk4A7B58 = 1;
    talk.cursor += 3 * sizeof(u16);
    return 1;
}

// The unit `unit` of a talk command (inline in 0x00469D20): a role from 8, an enemy below.
BattleUnit& Game::TalkUnit(i32 unit)
{
    if (unit >= BattleState::FirstRoleUnit)
    {
        return this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)];
    }

    return this->battleEnemies[static_cast<std::size_t>(unit)];
}

// The unit of a talk command kept in the high word of unk53CE8C (inline in 0x00469D20).
u16 Game::TalkCommandUnit() const
{
    return static_cast<u16>(static_cast<u32>(this->fightTalk.unk53CE8C) >> kHighWordShift);
}

// The unit kept in the high word of unk53CE8C set (inline in 0x00469D20).
void Game::SetTalkCommandUnit(u16 unit)
{
    FightTalk& talk = this->fightTalk;
    talk.unk53CE8C = static_cast<i32>(
        (static_cast<u32>(talk.unk53CE8C) & kLowWordMask) | (static_cast<u32>(unit) << kHighWordShift)
    );
}

// A unit command started by a talk command (inline in 0x00469D20, LABEL_198): the command marked running, the
// battle runs a frame.
int Game::TalkCommandStarted()
{
    this->SetTalkCommandUnit(static_cast<u16>(this->TalkCommandUnit() | kWindowShown));
    this->battleState.unk53BFE0 = 0;
    this->fightTalk.unk4A7B58 = 1;
    this->BattleFrame();
    return 1;
}

// Command 0xA of FightTalkFrame (inline in the original): the unit of the second word is defeated (an enemy: kept
// for the removal, unk53BFDC; a role: unk53BFD8), the field flashes, a frame runs and the talk goes on.
int Game::FightTalkDefeat()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if ((this->TalkCommandUnit() & kWindowShown) == 0)
    {
        const u16 unit = talk.UnsignedWord(1);
        this->SetTalkCommandUnit(unit);
        this->TalkUnit(unit).StartDefeat(1);

        if (unit <= kLastEnemy)
        {
            battle.unk4A7626 = unit;
            battle.unk53BFDC = 1;
            battle.unk502984[unit] = unit;
            battle.flashing = 1;
            battle.unk53BFE0 = 0;
            talk.unk4A7B58 = 0;
        }
        else
        {
            battle.unk53BFD8 = 1;
            battle.flashing = 1;
            battle.unk4A7624 = static_cast<u16>(unit - BattleState::FirstRoleUnit);
            battle.unk53BFE0 = 0;
        }

        this->SetTalkCommandUnit(static_cast<u16>(unit | kWindowShown));
    }

    this->BattleFrame();
    talk.cursor += 2 * sizeof(u16);
    this->SetTalkCommandUnit(0);
    battle.unk53BFC0 = 0;
    talk.unk4A7B58 = 1;
    return 1;
}

// Commands 0xB, 0x15 and 0x1A of FightTalkFrame (inline in the original): the unit of the second word takes command
// `command` (0x11 escape, 0xB, 0xC) with word 2A0A from the third word, its slot record put first (a role's turn
// set up; command 0xB makes an enemy aim at itself and a role's order a party order; the enemy of command 0xB holds
// the panels). The talk goes on once the unit is done (unk53BFE0).
int Game::FightTalkCommand(int command)
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if ((this->TalkCommandUnit() & kWindowShown) != 0)
    {
        if (battle.unk53BFE0 != 1)
        {
            if (command == kEscapeCommand)
            {
                talk.unk4A7B58 = 1;
            }

            this->BattleFrame();
            return 1;
        }

        talk.cursor += 3 * sizeof(u16);
        this->SetTalkCommandUnit(0);
        battle.unk53BFC0 = 0;

        if (command == kAimSelfCommand)
        {
            talk.unk4A7B58 = 0;
            this->BattleFrame();
        }
        else if (command == kHoldCommand)
        {
            talk.unk4A7B58 = 0;
        }

        return 1;
    }

    const u16 unit = talk.UnsignedWord(1);
    this->SetTalkCommandUnit(unit);
    BattleUnit& actor = this->TalkUnit(unit);

    if (unit > kLastEnemy)
    {
        battle.unk53BD50 = unit;
        battle.unk4A754C = unit - (BattleState::FirstRoleUnit - 1);
    }

    if (command != kAimSelfCommand)
    {
        actor.Retarget(0);
    }
    else if (unit <= kLastEnemy)
    {
        actor.Retarget(static_cast<i16>(unit));
    }

    actor.SetCommand(command);
    actor.SetWord2A0A(talk.Word(2));

    if (command == kEscapeCommand)
    {
        battle.InsertSlotRecord(unit <= kLastEnemy ? kEnemySlot : 1, unit, 0);
    }
    else if (command == kAimSelfCommand)
    {
        if (unit <= kLastEnemy)
        {
            battle.unk53BFC0 = 1;
        }
        else
        {
            battle.pendingOrders[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].unk00 = 1;
            battle.InsertSlotRecord(1, unit, 0);
        }
    }

    return this->TalkCommandStarted();
}

// Command 0xC of FightTalkFrame (inline in the original): once every defeated enemy is taken out the battle ends as
// won (unless the talk already ended it: every enemy counted gone, the end check state 0x63 unless a later end runs).
int Game::FightTalkWin()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        if (battle.unk502984[index] != -1)
        {
            this->BattleFrame();
            talk.unk53CE90 =
                static_cast<i32>((static_cast<u32>(talk.unk53CE90) & kLowWordMask) | (1u << kHighWordShift));
            return 1;
        }
    }

    if (talk.unk53CEAC == 0)
    {
        battle.unk53BF00 = static_cast<u8>(battle.enemyCount);
        battle.unk53BF02 = 0;

        if (battle.result <= kLastPlayState)
        {
            battle.autoBattle = 0;
            battle.result = kEndCheckState;
        }
    }

    talk.cursor += sizeof(u16);
    talk.unk53CEAC = 1;
    talk.unk4A7B58 = 1;
    return 1;
}

// Command 0xD of FightTalkFrame (inline in the original): the unit of the second word moves down by the third word.
int Game::FightTalkLift()
{
    FightTalk& talk = this->fightTalk;
    talk.unk53CE74 = talk.UnsignedWord(2);
    BattleUnit& moved = this->TalkUnit(talk.Word(1));
    u16 x = 0;
    u16 y = 0;
    moved.GetPosition(x, y);
    talk.unk53CCE8 = x;
    talk.unk53CCEC = y + static_cast<i16>(talk.unk53CE74);
    moved.SetPosition(static_cast<i16>(talk.unk53CCE8), static_cast<i16>(talk.unk53CCEC));
    talk.cursor += 3 * sizeof(u16);
    talk.unk53CE78 = 0;
    talk.unk53CCE8 = 0;
    talk.unk53CCEC = 0;
    talk.unk53CE74 = 0;
    this->MeasureUnits();
    this->BattleFrame();
    return 1;
}

// Command 0xE of FightTalkFrame (inline in the original): the units listed after it (ended by 0xFFFF) are checked
// (IsAB8); once all of them are, the talk jumps to the place after the list, otherwise it goes on past the list and
// the place (and one more word).
int Game::FightTalkAllDone()
{
    FightTalk& talk = this->fightTalk;
    talk.unk4A7B58 = 1;
    u16 listed = 0;

    while (talk.UnsignedWord(static_cast<std::size_t>(listed) + 1) != kListEnd)
    {
        ++listed;
    }

    talk.unk53CE9E = listed;
    u16 done = 0;

    for (u16 index = 0; index < listed; ++index)
    {
        const u16 unit = talk.UnsignedWord(static_cast<std::size_t>(index) + 1);
        talk.unk4A7B58 = 0;
        this->SetTalkCommandUnit(unit);

        if (this->TalkUnit(unit).IsAB8() && ++done == listed)
        {
            talk.cursor += 2 * static_cast<std::size_t>(listed) + 2 * sizeof(u16);
            talk.Seek(talk.Word(0));
            talk.unk53CE9E = 0;
            talk.unk53CE94 = 0;
            return 1;
        }
    }

    talk.unk53CE94 = 0;

    if (done == listed)
    {
        talk.unk53CE9E = 0;
        return 1;
    }

    talk.cursor += 2 * (static_cast<std::size_t>(listed) + kListTail);
    return 1;
}

// Commands 0xF and 0x12 of FightTalkFrame (inline in the original): the unit of the second word gets (`set`) or loses
// the marks of the third word (a role's DefeatedMark counts it in defeatedRoles); 0x12 runs a frame.
int Game::FightTalkMarks(bool set)
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if (set)
    {
        talk.unk4A7B58 = 0;
    }

    const i16 unit = talk.Word(1);
    const u16 marks = talk.UnsignedWord(2);
    BattleUnit& marked = this->TalkUnit(unit);

    if (set)
    {
        marked.SetStatus(marks);
    }
    else
    {
        marked.ClearStatus(marks);
    }

    if (unit > kLastEnemy && (marks & BattleUnit::DefeatedMark) != 0)
    {
        if (set)
        {
            ++battle.defeatedRoles;
        }
        else
        {
            --battle.defeatedRoles;
        }
    }

    talk.cursor += 3 * sizeof(u16);

    if (set)
    {
        talk.unk4A7B58 = 1;
        return 1;
    }

    talk.unk4A7B58 = 0;
    this->BattleFrame();
    talk.unk4A7B58 = 1;
    return 1;
}

// Command 0x10 of FightTalkFrame (inline in the original): the talk jumps to the place in the third word the count
// of the second word less one times (a frame each), then goes on.
int Game::FightTalkRepeat()
{
    FightTalk& talk = this->fightTalk;
    talk.unk4A7B58 = 0;

    auto low = [&talk]
    {
        return static_cast<u16>(talk.unk53CE90);
    };

    auto setLow = [&talk](u16 value)
    {
        talk.unk53CE90 = static_cast<i32>((static_cast<u32>(talk.unk53CE90) & ~kLowWordMask) | value);
    };

    if ((low() & kCountdown) == 0)
    {
        setLow(static_cast<u16>((talk.UnsignedWord(1) | kCountdown) - 1));
    }

    if ((low() & kCountMask) != 0)
    {
        const i16 place = talk.Word(2);
        talk.Seek(place);
        setLow(static_cast<u16>(low() - 1));
    }
    else
    {
        setLow(0);
        talk.cursor += 4 * sizeof(u16);
    }

    this->BattleFrame();
    talk.unk4A7B58 = 1;
    return 1;
}

// Command 0x13 of FightTalkFrame (inline in the original): the talk jumps to one of the places listed after it (every
// other word, ended by 0xFFFF) at random (the only one when there is one).
int Game::FightTalkRandomJump()
{
    FightTalk& talk = this->fightTalk;
    talk.cursor += sizeof(u16);
    u16 count = static_cast<u16>(talk.unk53CE94);

    if (count == 0)
    {
        u16 at = static_cast<u16>(static_cast<u32>(talk.unk53CE94) >> kHighWordShift);

        while (true)
        {
            talk.unk53CE74 = talk.UnsignedWord(at);

            if (talk.unk53CE74 == kListEnd)
            {
                break;
            }

            at = static_cast<u16>(at + 2);
            ++count;
            talk.unk53CE94 = static_cast<i32>(count | (static_cast<u32>(at) << kHighWordShift));
        }
    }

    if (count == 1)
    {
        talk.Seek(talk.Word(0));
    }
    else
    {
        talk.unk53CE74 = static_cast<u16>(2 * this->random.Below(static_cast<u16>(talk.unk53CE94)));
        talk.Seek(talk.Word(talk.unk53CE74));
    }

    talk.unk53CE74 = 0;
    talk.unk53CE94 = 0;
    talk.unk4A7B58 = 1;
    return 1;
}

// Command 0x16 of FightTalkFrame (inline in the original): every unit moves across by the second word (also the
// shake of the background).
int Game::FightTalkShift()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.unk53CE74 = talk.UnsignedWord(1);
    battle.shake = static_cast<i16>(talk.unk53CE74);

    auto shift = [&talk, &battle](BattleUnit& unit)
    {
        u16 x = 0;
        u16 y = 0;
        unit.GetPosition(x, y);
        talk.unk53CCE8 = x + battle.shake;
        talk.unk53CCEC = y;
        unit.SetPosition(static_cast<i16>(talk.unk53CCE8), static_cast<i16>(talk.unk53CCEC));
    };

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        shift(this->battleRoles[index]);
    }

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        shift(this->battleEnemies[index]);
    }

    talk.cursor += 2 * sizeof(u16);
    talk.unk53CE74 = 0;
    return 1;
}

// Command 0x17 of FightTalkFrame (inline in the original): the unit of the third word uses the skill (an enemy) or
// the object (a role) of the second word on the unit of the fourth, as command 9 sets an attack (an enemy's order
// is a spell order).
int Game::FightTalkSkill()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.unk53CCE8 = talk.Word(1);

    if (talk.UnsignedWord(2) <= kLastEnemy)
    {
        talk.unk53CCEC = talk.Word(2);
        talk.unk53CCF0 = talk.Word(3);
        u16& order = battle.enemyOrders[static_cast<std::size_t>(talk.unk53CCEC)];
        order = 0;

        if (talk.unk53CCF0 <= kLastEnemy)
        {
            if (this->battleEnemies[static_cast<std::size_t>(talk.unk53CCF0)].IsDefeated())
            {
                while (++talk.unk53CCF0 <= kLastEnemy &&
                       this->battleEnemies[static_cast<std::size_t>(talk.unk53CCF0)].IsDefeated())
                {
                }
            }

            battle.unk53BFD4 = 1;
            battle.unk53C014 = 1;
            order = static_cast<u16>(talk.unk53CCF0);
        }
        else
        {
            this->TalkStandingRole(talk.unk53CCF0);
            order = static_cast<u16>(talk.unk53CCF0 - BattleState::FirstRoleUnit);
        }

        order = static_cast<u16>(order | kOrderSpell);
        u16 act = static_cast<u16>(battle.unk53BD40);
        this->battleEnemies[static_cast<std::size_t>(talk.unk53CCEC)]
            .LoadSkillAction(this->database, static_cast<u16>(talk.unk53CCE8), act);
        battle.unk53BD40 = static_cast<i32>((static_cast<u32>(battle.unk53BD40) & ~kLowWordMask) | act);
        battle.InsertSlotRecord(kEnemySlot, talk.unk53CCEC, 0);
    }
    else
    {
        talk.unk53CE6C = battle.currentUnit;
        battle.unk53BD50 = talk.Word(2);
        i32 target = talk.Word(3);
        const auto role = static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit);

        if (target > kLastEnemy)
        {
            battle.unk53BFD0 = 1;
            target -= BattleState::FirstRoleUnit;
            battle.pendingOrders[role].unk00 = 1;
        }

        battle.unk4A754C = target + 1;
        BattleRole& actor = this->battleRoles[role];
        actor.SetTarget(1);
        u32 act = static_cast<u32>(battle.unk4FE5D4[role]);
        actor.LoadItemAction(this->database, static_cast<u16>(talk.unk53CCE8), act);
        battle.unk4FE5D4[role] = static_cast<i32>(act);
        battle.pendingOrders[role].unk0C = static_cast<u16>(actor.ActSpeed());

        if (actor.ActFlag1() == 1)
        {
            battle.unk53BFF0 = 1;
            battle.pendingOrders[role].unk08 = 1;
        }

        if (actor.ActFlag0() == 1)
        {
            battle.unk53BF88 = 1;
        }

        battle.unk53BCE8 = kSpellOrder;
        battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = kSpellOrder;
    }

    talk.unk4A7B58 = 0;
    this->BattleFrame();
    talk.unk4A7B58 = 1;
    talk.cursor += 4 * sizeof(u16);
    return 1;
}

// A defeated role target of a talk order passes to a random standing role that is not unk2B00 or a guest (inline in
// 0x00469D20, commands 9 and 0x17; the original draws for ever when none qualifies).
void Game::TalkStandingRole(i32& unit)
{
    BattleState& battle = this->battleState;
    int draws = 0;

    while (this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].IsDefeated())
    {
        do
        {
            if (++draws > kHangDraws)
            {
                Hang();
            }

            if (battle.roleCount != 0)
            {
                unit = static_cast<i32>(this->random.Below(battle.roleCount)) + BattleState::FirstRoleUnit;
            }
        } while (this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].unk2B00 == 1 ||

                 this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].unk2B04 == 1);
    }
}

// Command 0x18 of FightTalkFrame (inline in the original): every unit reset and the battle ended, battle `number` of
// the second word starts (its talk read anew).
int Game::FightTalkNextBattle()
{
    BattleState& battle = this->battleState;
    const u16 number = this->fightTalk.UnsignedWord(1);

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        this->battleEnemies[index].Reset(this->battleServices);
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        this->battleRoles[index].Reset(this->battleServices);
    }

    this->EndBattle();
    this->StartBattle(number);
    return 1;
}

// Command 0x19 of FightTalkFrame (inline in the original): the battle ends as won (to the loot, 0x66, when there is
// loot and the end is not running yet), every enemy counted gone; the talk waits until a frame ends the battle.
int Game::FightTalkLoot()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if (this->battleMessages.lootCount == 0 || battle.unk53BFE4 != 0)
    {
        battle.unk53BFE4 = 1;
    }
    else
    {
        battle.result = kEndLootState;
    }

    talk.unk53CEAC = 1;
    battle.unk53BF00 = static_cast<u8>(battle.enemyCount);
    battle.unk53BF02 = 0;
    talk.unk4A7B58 = 0;

    if (this->BattleFrame() != 0)
    {
        return 1;
    }

    talk.unk53CEAC = 0;
    talk.cursor += sizeof(u16);
    return 1;
}

// Commands 0x1B..0x26 of FightTalkFrame (inline in the original); see FightTalkFrame.
int Game::FightTalkUnits(u16 command)
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    switch (command)
    {
        case kTalkSetAction:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            this->TalkUnit(this->TalkCommandUnit()).SetAction(talk.Word(2));
            talk.cursor += 3 * sizeof(u16);
            return 1;

        case kTalkVitals:
        {
            const u16 unit = talk.UnsignedWord(1);
            this->SetTalkCommandUnit(unit);
            BattleUnit& set = this->TalkUnit(unit);
            set.SetVitals(talk.Word(2), talk.Word(3), talk.Word(4));

            if (unit > kLastEnemy)
            {
                const auto role = static_cast<std::size_t>(unit - BattleState::FirstRoleUnit);
                set.GetHitPoints(talk.unk53CCE8, talk.unk53CCEC);
                battle.hitPoints[role] = talk.unk53CCEC;
                set.GetSpirit(talk.unk53CE98, talk.unk53CE9A);
                battle.spirit[role] = talk.unk53CE9A;
            }

            talk.cursor += 5 * sizeof(u16);
            talk.unk53CE98 = 0;
            return 1;
        }

        case kTalkToggle:
        {
            const u16 unit = talk.UnsignedWord(1);
            this->SetTalkCommandUnit(unit);
            this->TalkUnit(unit).ToggleAF8();

            if (unit <= kLastEnemy)
            {
                ++battle.unk53BF02;
            }

            talk.cursor += 2 * sizeof(u16);
            this->SetTalkCommandUnit(0);
            return 1;
        }

        case kTalkSound:
            talk.unk4A7B58 = 0;
            this->audio.PlaySound(talk.UnsignedWord(1), this->config.soundVolume);
            this->BattleFrame();
            talk.cursor += 2 * sizeof(u16);
            talk.unk4A7B58 = 1;
            return 1;

        case kTalkChoice:
            return this->FightTalkChoice();

        case kTalkHeadIn:
        {
            auto sprite = std::make_unique<ScriptSprite>();
            ResetPlayback(sprite->act);
            sprite->act.id = talk.UnsignedWord(1);
            sprite->act.action = talk.UnsignedWord(2);
            sprite->targetX = talk.Word(3);
            sprite->targetY = talk.Word(4);
            sprite->headX = static_cast<i16>(-ScriptSprite::HeadMargin);

            if (sprite->targetX > Display::Width / 2)
            {
                sprite->headX = static_cast<i16>(Display::Width + ScriptSprite::HeadMargin);
            }

            sprite->headSpeed = 0;
            talk.cursor += 5 * sizeof(u16);
            sprite->next = std::move(this->scene.spritesB);
            this->scene.spritesB = std::move(sprite);
            return 1;
        }

        case kTalkHeadOut:
        {
            const u16 id = talk.UnsignedWord(1);
            const u16 action = talk.UnsignedWord(2);

            for (ScriptSprite* sprite = this->scene.spritesB.get(); sprite != nullptr; sprite = sprite->next.get())
            {
                if (static_cast<u32>(sprite->act.id) == id && static_cast<u32>(sprite->act.action) == action)
                {
                    sprite->headSpeed = -1;

                    if (sprite->headX > Display::Width / 2)
                    {
                        sprite->headSpeed = 1;
                    }

                    break;
                }
            }

            talk.cursor += 3 * sizeof(u16);
            return 1;
        }

        case kTalkFlagsA0:
            talk.unk53CE8C = static_cast<i32>(static_cast<u32>(talk.unk53CE8C) | kWindowFlagsA0);
            talk.cursor += sizeof(u16);
            return 1;

        case kTalkLines:
            return this->FightTalkLines();

        case kTalkLinesMode:
            return this->FightTalkLinesMode();

        default:
            return 1;
    }
}

// Command 0x1F of FightTalkFrame (inline in the original): the talk jumps to the place of the choice taken (every
// other word after it, ended by 0xFFFF; Talk::choice); a choice past them shows 按鈕個數不對!! and the talk ends.
int Game::FightTalkChoice()
{
    FightTalk& talk = this->fightTalk;
    talk.cursor += sizeof(u16);
    u16 at = static_cast<u16>(static_cast<u32>(talk.unk53CE94) >> kHighWordShift);

    while (true)
    {
        talk.unk53CE74 = talk.UnsignedWord(at);

        if (talk.unk53CE74 == kListEnd)
        {
            break;
        }

        at = static_cast<u16>(at + 2);
        talk.unk53CE94 = static_cast<i32>(
            (static_cast<u32>(talk.unk53CE94) & kLowWordMask) | (static_cast<u32>(at) << kHighWordShift)
        );
        ++talk.unk53CEA0;
    }

    if (static_cast<u32>(this->talk.choice + 1) <= static_cast<u32>(talk.unk53CEA0))
    {
        talk.Seek(talk.Word(2 * static_cast<std::size_t>(this->talk.choice)));
        talk.unk53CEA0 = 0;
        talk.unk53CE94 = static_cast<i32>(static_cast<u32>(talk.unk53CE94) & kLowWordMask);
        return 1;
    }

    this->errors.ShowError(kErrorTitle, kWrongButtons);
    return 0;
}

// Command 0x24 of FightTalkFrame (inline in the original): line effect `number` (below 0x100) of the screen starts
// unless it runs (unk53CE90 then 1): `count` columns from (x, y), falling, rising (mode 1, from limit less 2) or
// still (mode 2); one off the screen is dropped.
int Game::FightTalkLines()
{
    FightTalk& talk = this->fightTalk;
    const u16 number = talk.UnsignedWord(1);

    auto running = [this, number]
    {
        for (const ScriptEffect* effect = this->scene.effects.get(); effect != nullptr; effect = effect->next.get())
        {
            if (static_cast<u8>(effect->mode) == number)
            {
                return true;
            }
        }

        return false;
    };

    if (number < kEffectNumbers)
    {
        talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);

        if (running())
        {
            talk.unk53CE90 =
                static_cast<i32>((static_cast<u32>(talk.unk53CE90) & kLowWordMask) | (1u << kHighWordShift));
        }
        else
        {
            auto effect = std::make_unique<ScriptEffect>();
            effect->mode = number;
            effect->color = talk.Word(2);
            effect->x = static_cast<i16>(talk.UnsignedWord(4) & kEvenPixels);
            effect->y = static_cast<i16>(talk.UnsignedWord(5) & kEvenPixels);
            effect->limit = static_cast<i16>(talk.UnsignedWord(6) & kEvenPixels);
            effect->count = static_cast<i16>(talk.UnsignedWord(7) & kEvenPixels);

            if (effect->x >= 0 && effect->y >= 0 && effect->x + effect->limit <= Display::Width &&
                effect->count + effect->y <= Display::Height)
            {
                const auto columns = static_cast<std::size_t>(std::max<int>(effect->count, 0));
                effect->offsets.assign(columns, 0);
                effect->lengths.assign(columns, 0);
                u16 start = 0;
                u16 kind = ScriptEffect::Falling;
                const u16 lines = talk.UnsignedWord(3);

                if (lines == kRisingLines)
                {
                    start = static_cast<u16>(effect->limit - kLineStart);
                    kind = ScriptEffect::Rising;
                }

                if (lines == kStillLines)
                {
                    start = 0;
                    kind = ScriptEffect::Still;
                }

                talk.unk53CE94 = static_cast<i32>(start | (static_cast<u32>(kind) << kHighWordShift));
                effect->mode = static_cast<u16>(effect->mode | kind);

                for (int column = 0; column < effect->count; ++column)
                {
                    effect->offsets[static_cast<std::size_t>(column)] = static_cast<i16>(start);
                    effect->lengths[static_cast<std::size_t>(column)] = kLineStart;
                }

                talk.unk53CE90 = static_cast<i32>(
                    (static_cast<u32>(talk.unk53CE90) & kLowWordMask) |
                    (static_cast<u32>(std::max<int>(effect->count, 0)) << kHighWordShift)
                );
                effect->next = std::move(this->scene.effects);
                this->scene.effects = std::move(effect);
            }
        }
    }

    talk.cursor += 8 * sizeof(u16);
    return 1;
}

// Command 0x25 of FightTalkFrame (inline in the original): running line effect `number` opens (mode 0), closes (1)
// or ends (2); another mode gives it the kind left in unk53CE90.
int Game::FightTalkLinesMode()
{
    FightTalk& talk = this->fightTalk;
    const u16 number = talk.UnsignedWord(1);

    if (number < kEffectNumbers)
    {
        std::unique_ptr<ScriptEffect>* link = &this->scene.effects;

        while (*link != nullptr && static_cast<u8>((*link)->mode) != number)
        {
            link = &(*link)->next;
        }

        if (*link != nullptr)
        {
            const u16 mode = talk.UnsignedWord(2);

            if (mode == kOpenLines)
            {
                talk.unk53CE90 = static_cast<i32>(
                    (static_cast<u32>(talk.unk53CE90) & kLowWordMask) |
                    (static_cast<u32>(ScriptEffect::Opening) << kHighWordShift)
                );
            }

            if (mode == kCloseLines)
            {
                talk.unk53CE90 = static_cast<i32>(
                    (static_cast<u32>(talk.unk53CE90) & kLowWordMask) |
                    (static_cast<u32>(ScriptEffect::Closing) << kHighWordShift)
                );
            }

            if (mode == kEndLines)
            {
                std::unique_ptr<ScriptEffect> ended = std::move(*link);
                *link = std::move(ended->next);
            }
            else
            {
                (*link)->mode = static_cast<u16>(
                    static_cast<u8>((*link)->mode) |
                    static_cast<u16>(static_cast<u32>(talk.unk53CE90) >> kHighWordShift)
                );
            }
        }
    }

    talk.cursor += 3 * sizeof(u16);
    return 1;
}

// Every unit moved across by the background's shake (inline in 0x00469D20, commands 0x28 and 0x49).
void Game::ShiftTalkUnits()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    auto shift = [&talk, &battle](BattleUnit& unit)
    {
        u16 x = 0;
        u16 y = 0;
        unit.GetPosition(x, y);
        x = static_cast<u16>(x + battle.shake);
        talk.unk53CE78 = static_cast<i32>(x | (static_cast<u32>(y) << kHighWordShift));
        unit.SetPosition(static_cast<i16>(x), static_cast<i16>(y));
    };

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        shift(this->battleRoles[index]);
    }

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        shift(this->battleEnemies[index]);
    }
}

// Command 0x27 of FightTalkFrame (inline in the original): the unit of the second word moves along a curve through
// the six offsets after it (from its place), 21 steps a span over six spans, a frame each.
int Game::FightTalkCurve()
{
    FightTalk& talk = this->fightTalk;
    u16 span = talk.unk53CE76;

    if ((this->TalkCommandUnit() & kWindowShown) != 0)
    {
        if (span >= kCurveSpans)
        {
            talk.cursor += kCurveBytes;
            this->SetTalkCommandUnit(0);
            talk.unk53CE74 = 0;
            talk.unk53CE76 = 0;
            talk.unk53CCE8 = 0;
            talk.unk53CCEC = 0;
            return 1;
        }
    }
    else
    {
        this->SetTalkCommandUnit(static_cast<u16>(talk.UnsignedWord(1) | kWindowShown));
        u16 x = 0;
        u16 y = 0;
        this->TalkUnit(this->TalkCommandUnit() & kCountMask).GetPosition(x, y);
        talk.unk53CCE8 = static_cast<i32>((static_cast<u32>(talk.unk53CCE8) & ~kLowWordMask) | x);
        talk.unk53CCEC = static_cast<i32>((static_cast<u32>(talk.unk53CCEC) & ~kLowWordMask) | y);
        talk.curve[0] = {static_cast<i16>(x), static_cast<i16>(y)};
        talk.curve[1] = talk.curve[0];

        for (std::size_t point = 0; point < kCurveOffsets; ++point)
        {
            talk.curve[point + 2] = {
                static_cast<i16>(x + talk.UnsignedWord(2 * point + 2)),
                static_cast<i16>(y + talk.UnsignedWord(2 * point + 3))
            };
        }

        talk.curve[kCurveOffsets + 2] = talk.curve[kCurveOffsets + 1];
        talk.curve[kCurveOffsets + 3] = talk.curve[kCurveOffsets + 1];
        span = 0;
        talk.unk53CE74 = 1;
        talk.unk53CE76 = 0;
    }

    const std::array<std::array<i16, 2>, 4> points =
        {talk.curve[span], talk.curve[span + 1u], talk.curve[span + 2u], talk.curve[span + 3u]};
    TalkCurvePoint(static_cast<float>(static_cast<i16>(talk.unk53CE74)), talk.unk53CCE8, talk.unk53CCEC, points);
    this->TalkUnit(this->TalkCommandUnit() & kCountMask)
        .SetPosition(static_cast<i16>(talk.unk53CCE8), static_cast<i16>(talk.unk53CCEC));

    if (++talk.unk53CE74 > kCurveSteps)
    {
        ++talk.unk53CE76;
        talk.unk53CE74 = 0;
    }

    this->MeasureUnits();
    this->BattleFrame();
    return 1;
}

// Command 0x28 of FightTalkFrame (inline in the original): every unit slides across, by a third of what is left each
// frame of the second word (above 16), or to bring the unit of the second word to the middle of the screen; the talk
// goes on once the slide is 0.
int Game::FightTalkSlide()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    i16 left = static_cast<i16>(talk.unk53CE74);
    const u16 value = talk.UnsignedWord(1);
    talk.unk53CE76 = value;

    if (talk.unk53CE74 == 0)
    {
        left = static_cast<i16>(value);
        talk.unk53CE74 = value;
    }

    i32 slide = 0;

    if (value > kSlideByThirds)
    {
        slide = left / kSlideThirdNegative;
        talk.unk53CE74 = static_cast<u16>(left / kSlideThird);
    }
    else
    {
        u16 x = 0;
        u16 y = 0;
        this->TalkUnit(static_cast<i16>(value)).GetPosition(x, y);
        talk.unk53CE78 = static_cast<i32>(x | (static_cast<u32>(y) << kHighWordShift));
        talk.unk53CE74 = static_cast<u16>((static_cast<i16>(x) + Display::Width / 2) / 2);
        slide = Display::Width / 2 - static_cast<i16>(talk.unk53CE74);
    }

    battle.shake = slide;
    talk.unk4A7B58 = 0;

    if (slide == 0)
    {
        talk.cursor += 2 * sizeof(u16);
        talk.unk53CE78 = 0;
        talk.unk53CE74 = 0;
        talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
        talk.unk53CE98 = 0;
        talk.unk53CE76 = 0;
        talk.unk4A7B58 = 1;
        return 1;
    }

    this->ShiftTalkUnits();
    this->MeasureUnits();
    this->BattleFrame();
    return 1;
}

// Command 0x2A of FightTalkFrame (inline in the original): the speaker of the next window (up to 0x20 bytes before
// %Q, none without it), a default role name turned into the role's name.
int Game::FightTalkSpeaker()
{
    FightTalk& talk = this->fightTalk;
    talk.textAt = talk.cursor + sizeof(u16);
    talk.speaker = {};
    std::size_t length = 0;

    while (true)
    {
        const std::size_t at = talk.textAt + length;

        if (at + 1 < talk.script.size() && talk.script[at] == kTextEscape && talk.script[at + 1] == kTextEnd)
        {
            std::copy_n(talk.script.begin() + static_cast<std::ptrdiff_t>(talk.textAt), length, talk.speaker.begin());
            length += 2;
            break;
        }

        if (++length >= talk.speaker.size())
        {
            break;
        }
    }

    const std::array<Party::Name, Party::Roles> defaults = Party::DefaultNames();

    auto nameOf = [](const auto& buffer)
    {
        return std::string_view(buffer.data(), strnlen(buffer.data(), buffer.size()));
    };

    std::size_t role = 0;

    while (role < defaults.size() && nameOf(defaults[role]) != nameOf(talk.speaker))
    {
        ++role;
    }

    talk.unk53CE90 = static_cast<i32>(
        (static_cast<u32>(talk.unk53CE90) & kLowWordMask) | (static_cast<u32>(role) << kHighWordShift)
    );

    if (role < defaults.size())
    {
        std::string text(talk.speaker.data(), talk.speaker.size());
        Talk::ReplacePrefix(
            text,
            0,
            nameOf(defaults[role]),
            nameOf(this->party.names[role]),
            static_cast<int>(talk.speaker.size())
        );
        std::copy_n(text.begin(), std::min(text.size(), talk.speaker.size()), talk.speaker.begin());
    }

    talk.unk4A7B58 = 0;
    this->BattleFrame();
    talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
    talk.textAt += length;
    talk.cursor = talk.textAt;
    talk.unk4A7B58 = 1;
    return 1;
}

// Command 0x2C of FightTalkFrame (inline in the original): a timed window of the unit of the second word that the
// keys do not advance (timeout unk4A7B54, which goes back to 0xFFFF), and the talk goes on at once after its text.
int Game::FightTalkTimedWindow()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    const u16 unit = talk.UnsignedWord(1);
    this->SetTalkCommandUnit(unit);
    talk.textAt = talk.cursor + sizeof(u16);
    auto window = std::make_unique<TalkWindow>();
    window->flags = TalkWindow::FixedPlace;
    const std::span<const u8> text =
        std::span<const u8>(talk.script).subspan(std::min(talk.textAt, talk.script.size()));
    const std::string_view name(talk.speaker.data(), strnlen(talk.speaker.data(), talk.speaker.size()));
    this->talk.SetupDialog(*window, text, name, Talk::SizedLayout, 0, 0);
    talk.window = window.get();
    this->talk.AppendWindow(std::move(window));
    talk.window->timeout |= static_cast<u32>(talk.unk4A7B54);
    talk.window->flags |= TalkWindow::Timed;
    talk.window->flags |= TalkWindow::NoAdvance;
    talk.window->speed = kWindowSpeed;
    talk.window->flags |= static_cast<u16>(talk.unk53CE8C);
    talk.unk53CE8C = static_cast<i32>(static_cast<u32>(talk.unk53CE8C) & ~kLowWordMask);
    talk.unk4A7B54 = kNoTalkTimeout;
    u16 x = 0;
    u16 y = 0;

    if (unit <= kLastEnemy)
    {
        BattleEnemy& enemy = this->battleEnemies[unit];
        enemy.GetDrawPosition(x, y);
        enemy.SetAC0(1);

        for (BattleEnemy& other : this->battleEnemies)
        {
            other.SetAC0(0);
        }
    }
    else
    {
        BattleRole& role = this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)];
        role.GetDrawPosition(x, y);

        for (BattleRole& other : this->battleRoles)
        {
            other.SetAC0(0);
        }

        role.SetAction(kTalkAction);
    }

    talk.unk53CE78 = static_cast<i32>(x | (static_cast<u32>(y) << kHighWordShift));
    talk.window->fromX = x;
    talk.window->fromY = y;
    battle.result = 0;
    this->ClearTalkMarks(false);
    this->SkipTalkText();
    return 1;
}

// Every unit unmarked once a talk window is set up (inline in 0x00469D20); `actions` also gives the roles action 0.
// The original walks ten enemies, two past the enemy array.
void Game::ClearTalkMarks(bool actions)
{
    for (BattleRole& role : this->battleRoles)
    {
        role.SetAC0(0);

        if (actions)
        {
            role.SetAction(0);
        }
    }

    for (std::size_t index = 0; index < std::min(kTalkEnemies, this->battleEnemies.size()); ++index)
    {
        this->battleEnemies[index].SetAC0(0);
    }
}

// The talk goes on after the text of its window (inline in 0x00469D20, LABEL_68): past %Q (within 0xFF bytes), the
// speaker cleared.
void Game::SkipTalkText()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    int length = 0;

    while (length < kTextEndSearch)
    {
        const std::size_t at = talk.textAt + static_cast<std::size_t>(length);

        if (at + 1 < talk.script.size() && talk.script[at] == kTextEscape && talk.script[at + 1] == kTextEnd)
        {
            break;
        }

        ++length;
    }

    talk.textAt += static_cast<std::size_t>(length) + 2;
    talk.cursor = talk.textAt;
    talk.unk4A7B58 = 1;
    battle.unk53BFC0 = 0;
    talk.speaker = {};
    talk.unk53CE78 = 0;
    this->SetTalkCommandUnit(0);
}

// Command 0x2D of FightTalkFrame (inline in the original): the sides swap: every unit faces the other way (a guest
// role away) at the mirrored place and the role panels move across; when mirrored already only the flag goes back.
int Game::FightTalkMirror()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if (battle.mirrored == 1)
    {
        talk.cursor += sizeof(u16);
        battle.mirrored = 0;
        return 1;
    }

    battle.mirrored = 1;

    auto mirror = [&talk](BattleUnit& unit)
    {
        u16 x = 0;
        u16 y = 0;
        unit.GetPosition(x, y);
        x = static_cast<u16>(Display::Width - x);
        talk.unk53CE78 = static_cast<i32>(x | (static_cast<u32>(y) << kHighWordShift));
        unit.SetPosition(static_cast<i16>(x), static_cast<i16>(y));
    };

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        BattleEnemy& enemy = this->battleEnemies[index];
        enemy.SetFacing(1);
        mirror(enemy);
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];
        role.SetFacing(role.unk2B04 == 1 ? 0 : 1);
        mirror(role);
        i32& panelX = battle.panelPlaces[2 * index];
        panelX = kMirroredPanelSpan - panelX;
    }

    talk.cursor += sizeof(u16);
    return 1;
}

// Command 0x36 of FightTalkFrame (inline in the original): enemy `actor` (the third word; a role: nothing) takes the
// command of choice `choice` (the second word) at the unit of the fourth, as command 0x17 aims (its order a forced
// order when the command loads, a spell order otherwise).
int Game::FightTalkEnemyCommand()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.unk53CCE8 = talk.Word(1);
    talk.unk53CCEC = talk.Word(2);
    talk.unk53CCF0 = talk.Word(3);

    if (talk.unk53CCEC > kLastEnemy)
    {
        talk.cursor += 4 * sizeof(u16);
        return 1;
    }

    u16& order = battle.enemyOrders[static_cast<std::size_t>(talk.unk53CCEC)];
    order = 0;

    if (talk.unk53CCF0 <= kLastEnemy)
    {
        if (this->battleEnemies[static_cast<std::size_t>(talk.unk53CCF0)].IsDefeated())
        {
            while (++talk.unk53CCF0 <= kLastEnemy &&
                   this->battleEnemies[static_cast<std::size_t>(talk.unk53CCF0)].IsDefeated())
            {
            }
        }

        battle.unk53BFD4 = 1;
        battle.unk53C014 = 1;
        order = static_cast<u16>(talk.unk53CCF0);
    }
    else
    {
        this->TalkStandingRole(talk.unk53CCF0);
        order = static_cast<u16>(talk.unk53CCF0 - BattleState::FirstRoleUnit);
    }

    u32 act = static_cast<u32>(battle.unk53BD40);
    const int loaded = this->battleEnemies[static_cast<std::size_t>(talk.unk53CCEC)]
                           .LoadEnemyCommand(this->database, talk.unk53CCE8, act);
    battle.unk53BD40 = static_cast<i32>(act);
    order = static_cast<u16>(order | (loaded == 1 ? kOrderForced : kOrderSpell));
    battle.InsertSlotRecord(kEnemySlot, talk.unk53CCEC, 0);
    talk.unk4A7B58 = 0;
    this->BattleFrame();
    talk.unk53CCE8 = 0;
    talk.unk53CCEC = 0;
    talk.unk53CCF0 = 0;
    talk.unk4A7B58 = 1;
    talk.cursor += 4 * sizeof(u16);
    return 1;
}

// Command 0x3B of FightTalkFrame (inline in the original): as command 2, a window of one line placed at the unit (its
// drawing x and the y of its origin).
int Game::FightTalkLineWindow()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if ((this->TalkCommandUnit() & kWindowShown) == 0)
    {
        const u16 unit = talk.UnsignedWord(1);
        this->SetTalkCommandUnit(unit);
        talk.textAt = talk.cursor + sizeof(u16);
        auto window = std::make_unique<TalkWindow>();
        BattleUnit& speaker = this->TalkUnit(unit);
        u16 x = 0;
        u16 y = 0;
        speaker.GetDrawPosition(x, y);
        u16 originX = 0;
        speaker.GetOrigin(originX, y);
        talk.unk53CE74 = originX;

        if (unit <= kLastEnemy)
        {
            for (BattleEnemy& enemy : this->battleEnemies)
            {
                enemy.SetAC0(0);
            }
        }
        else
        {
            for (BattleRole& role : this->battleRoles)
            {
                role.SetAC0(0);
            }

            speaker.SetAction(kTalkAction);
        }

        talk.unk53CE78 = static_cast<i32>(x | (static_cast<u32>(y) << kHighWordShift));
        window->flags = TalkWindow::FixedPlace;
        const std::span<const u8> text =
            std::span<const u8>(talk.script).subspan(std::min(talk.textAt, talk.script.size()));
        const std::string_view name(talk.speaker.data(), strnlen(talk.speaker.data(), talk.speaker.size()));
        this->talk.SetupDialog(*window, text, name, Talk::OneLine, static_cast<i16>(x), static_cast<i16>(y));
        talk.window = window.get();
        this->talk.AppendWindow(std::move(window));
        talk.window->timeout |= kNoTimeout;
        talk.window->speed = kWindowSpeed;
        talk.window->flags |= static_cast<u16>(talk.unk53CE8C);
        talk.unk53CE8C = static_cast<i32>(static_cast<u32>(talk.unk53CE8C) & ~kLowWordMask);
        talk.window->fromX = x;
        talk.window->fromY = y;
        this->SetTalkCommandUnit(static_cast<u16>(this->TalkCommandUnit() | kWindowShown));
        battle.unk53BFC0 = 1;
        battle.result = 0;
    }

    if (this->scene.talkWindows != nullptr)
    {
        talk.unk4A7B58 = 0;
        this->BattleFrame();
        return 1;
    }

    this->ClearTalkMarks(true);
    this->SkipTalkText();
    return 1;
}

// Command 0x3E of FightTalkFrame (inline in the original): the field fades from the colour of words 2..4 to that of
// words 5..7 over the frames of word 8 (the high word of the frames keeps the sign of word 2, as the original's
// register does).
int Game::FightTalkFieldFade()
{
    ColorFade& fade = this->battleState.fieldFade;
    FightTalk& talk = this->fightTalk;
    fade.red = static_cast<float>(talk.Word(1));
    fade.green = static_cast<float>(talk.Word(2));
    fade.blue = static_cast<float>(talk.Word(3));
    talk.unk53CE98 = talk.UnsignedWord(4);
    talk.unk53CE9A = talk.UnsignedWord(5);
    talk.unk53CE9C = talk.UnsignedWord(6);
    const i32 toRed = static_cast<i16>(talk.unk53CE98);
    const i32 toGreen = static_cast<i16>(talk.unk53CE9A);
    const i32 toBlue = static_cast<i16>(talk.unk53CE9C);
    fade.startRed = static_cast<float>(toRed);
    fade.startGreen = static_cast<float>(toGreen);
    fade.startBlue = static_cast<float>(toBlue);
    const u32 signWord = talk.Word(1) < 0 ? ~kLowWordMask : 0;
    fade.frames = static_cast<i32>(signWord | talk.UnsignedWord(7));
    const auto frames = static_cast<double>(fade.frames);
    fade.stepRed = static_cast<float>(static_cast<double>(toRed - static_cast<i32>(crt::Ftol(fade.red))) / frames);
    fade.stepGreen =
        static_cast<float>(static_cast<double>(toGreen - static_cast<i32>(crt::Ftol(fade.green))) / frames);
    fade.stepBlue = static_cast<float>(static_cast<double>(toBlue - static_cast<i32>(crt::Ftol(fade.blue))) / frames);
    talk.cursor += 8 * sizeof(u16);
    talk.unk53CE98 = 0;
    talk.unk53CE9A = 0;
    talk.unk53CE9C = 0;
    return 1;
}

// Commands 0x41 and 0x42 of FightTalkFrame (inline in the original): 0x41 jumps to the place after it when the
// inventory holds (counted and pending) at least the third word of the object of the second; 0x42 takes that many away
// (the count first; an object left with none leaves the list; a missing object faults as in the original).
int Game::FightTalkObjects(bool take)
{
    FightTalk& talk = this->fightTalk;
    const i16 id = talk.Word(1);
    talk.unk53CE90 = static_cast<i32>(
        (static_cast<u32>(talk.unk53CE90) & kLowWordMask) | (static_cast<u32>(static_cast<u16>(id)) << kHighWordShift)
    );
    u16 wanted = talk.UnsignedWord(2);
    ItemNode* node = FindItemOfKind(this->party.inventory, id);

    auto finish = [this, &talk](std::size_t bytes)
    {
        this->BattleFrame();
        talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
        talk.unk53CE94 = static_cast<i32>(static_cast<u32>(talk.unk53CE94) & ~kLowWordMask);
        talk.cursor += bytes;
        return 1;
    };

    if (!take)
    {
        talk.unk53CE94 = static_cast<i32>((static_cast<u32>(talk.unk53CE94) & ~kLowWordMask) | wanted);

        if (node != nullptr && node->count + node->pending >= wanted)
        {
            talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
            talk.cursor += 3 * sizeof(u16);
            talk.Seek(talk.Word(0));
            return 1;
        }

        return finish(5 * sizeof(u16));
    }

    if (node == nullptr)
    {
        Fault(this->errors, u8"battle talk command 0x42: the object is not in the inventory");
    }

    if (node->count != 0)
    {
        if (node->count > static_cast<i32>(wanted))
        {
            node->count = static_cast<i16>(node->count - wanted);
            wanted = 0;
        }
        else
        {
            wanted = static_cast<u16>(wanted - node->count);
            node->count = 0;
        }
    }

    if (node->pending != 0)
    {
        node->pending = static_cast<i16>(node->pending - wanted);
    }

    if (node->count == 0 && node->pending == 0)
    {
        UnlinkItem(this->party.inventory, id);
    }

    return finish(3 * sizeof(u16));
}

// Command 0x49 of FightTalkFrame (inline in the original): every unit slides across in the steps of the third word
// until the slide (unk53CE74) reaches the second word; then the talk goes on.
int Game::FightTalkSlideTo()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.unk53CE78 = static_cast<i32>((static_cast<u32>(talk.unk53CE78) & ~kLowWordMask) | talk.UnsignedWord(1));
    talk.unk53CE98 = talk.UnsignedWord(2);

    if (static_cast<i16>(talk.unk53CE98) == 0)
    {
        Fault(this->errors, u8"battle talk command 0x49: an integer division by zero");
    }

    const i32 slide =
        (static_cast<i16>(talk.unk53CE78) - static_cast<i16>(talk.unk53CE74)) / static_cast<i16>(talk.unk53CE98);
    talk.unk4A7B58 = 0;
    battle.shake = slide;
    talk.unk53CE74 = static_cast<u16>(talk.unk53CE74 + slide);

    if (slide == 0)
    {
        talk.cursor += 3 * sizeof(u16);
        talk.unk53CE78 = 0;
        talk.unk53CE74 = 0;
        talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
        talk.unk53CE98 = 0;
        talk.unk53CE76 = 0;
        talk.unk4A7B58 = 1;
        return 1;
    }

    this->ShiftTalkUnits();
    this->MeasureUnits();
    this->BattleFrame();
    return 1;
}

// Command 0x4E of FightTalkFrame (inline in the original): the role of the second word (healed fully and cured of
// bit 0x8000 when defeated) catches the enemy of the third (command 6, at once: the queues and records cleared, the
// role acting); the talk waits until the catch is over (unk53C02C).
int Game::FightTalkCatch()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    if (talk.unk53CE98 == 0)
    {
        const u16 unit = talk.UnsignedWord(1);
        talk.unk53CE90 = static_cast<i32>(
            (static_cast<u32>(talk.unk53CE90) & kLowWordMask) | (static_cast<u32>(unit) << kHighWordShift)
        );
        battle.unk4A754C = talk.Word(2) + 1;
        BattleRole& role = this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)];

        if (role.IsDefeated())
        {
            role.CureEffects(this->battleServices, BattleUnit::ReviveBit);
            role.Damage(this->battleServices, -kMaxVital, kMaxVital, kMaxVital);
        }

        battle.currentUnit = 0;
        role.Retarget(static_cast<i16>(battle.unk4A754C - 1));
        role.SetCommand(kCatchCommand);
        battle.unk53BCE8 = kCatchCommand;
        battle.unk520DD0.fill(0);
        battle.unk524788.fill(BattleSlotRecord{});
        battle.unk53AF38[unit] = kCatchCommand;
        battle.unk53BF74 = 1;
        battle.unk53BFC0 = 1;
        battle.unk53C010 = 1;
        battle.unk53BF68 = 0;
        battle.unk53AE70 = unit;
        this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)].IncreaseHits();
        battle.unk53C02C = 1;
        talk.unk53CE98 = 1;
    }

    talk.unk4A7B58 = 1;
    this->BattleFrame();

    if (battle.unk53C02C != 0)
    {
        return 1;
    }

    talk.unk53CE98 = 0;
    talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
    talk.cursor += 3 * sizeof(u16);
    return 1;
}

// Command 0x4F of FightTalkFrame (inline in the original): the unit of the second word takes the damage of words 3..5
// (an enemy is healed by the first: the original negates it), a role with its damage digits.
int Game::FightTalkDamage()
{
    FightTalk& talk = this->fightTalk;
    const u16 unit = talk.UnsignedWord(1);
    this->SetTalkCommandUnit(unit);
    talk.unk53CE98 = talk.UnsignedWord(2);
    talk.unk53CE9A = talk.UnsignedWord(3);
    talk.unk53CE9C = talk.UnsignedWord(4);

    if (unit <= kLastEnemy)
    {
        this->battleEnemies[unit].Damage(
            this->battleServices,
            -static_cast<i32>(static_cast<i16>(talk.unk53CE98)),
            static_cast<i16>(talk.unk53CE9A),
            static_cast<i16>(talk.unk53CE9C)
        );
    }
    else
    {
        this->SetTalkCommandUnit(static_cast<u16>(unit - BattleState::FirstRoleUnit));
        BattleRole& role = this->battleRoles[this->TalkCommandUnit()];
        role.Damage(
            this->battleServices,
            static_cast<i16>(talk.unk53CE98),
            static_cast<i16>(talk.unk53CE9A),
            static_cast<i16>(talk.unk53CE9C)
        );
        role.RaiseFlag26B8();
        role.PushTarget(BattleUnit::DamageTarget);
        role.SetDamageDigits(this->battleServices, static_cast<i16>(talk.unk53CE98));
        role.StepTargetCount(1);
    }

    talk.unk53CE98 = 0;
    talk.unk53CE9A = 0;
    talk.unk53CE9C = 0;
    this->SetTalkCommandUnit(0);
    talk.cursor += 5 * sizeof(u16);
    return 1;
}

// Commands 0x29..0x53 of FightTalkFrame not handled on their own (inline in the original); see FightTalkFrame.
int Game::FightTalkMisc(u16 command)
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;

    switch (command)
    {
        case kTalkIntro:
            talk.unk53CEB0 = 0;
            this->BattleIntro(talk.UnsignedWord(1));
            talk.unk53CEB0 = 1;
            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkQuietWin:
            battle.unk53BF00 = static_cast<u8>(battle.enemyCount);
            battle.unk53BF02 = 0;
            battle.unk53C004 = 1;
            battle.result = kEndCheckState;
            battle.autoBattle = 0;
            talk.unk53CEAC = 1;
            talk.unk4A7B58 = 0;
            this->BattleFrame();
            talk.cursor += sizeof(u16);
            return 1;

        case kTalkSetFlag:
            this->state.SetGameFlag(talk.Word(1));
            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkClearFlag:
            this->state.ClearGameFlag(talk.Word(1));
            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkFlagJump:
            talk.unk53CCE8 = talk.Word(1);

            if (this->state.GameFlag(talk.unk53CCE8))
            {
                talk.cursor += 2 * sizeof(u16);
                talk.Seek(talk.Word(0));
                return 1;
            }

            talk.cursor += 4 * sizeof(u16);
            return 1;

        case kTalkPartyGone:
            battle.defeatedRoles = kNoRolesLeft;
            battle.roleCount = 0;
            this->MeasureUnits();
            talk.cursor += sizeof(u16);
            return 1;

        case kTalkJoinFirst:
        {
            BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(talk.Word(1))];
            talk.unk53CE90 = static_cast<i32>(
                (static_cast<u32>(talk.unk53CE90) & kLowWordMask) | (static_cast<u32>(enemy.ActId()) << kHighWordShift)
            );
            u16 x = 0;
            u16 y = 0;
            this->battleEnemies[0].GetPosition(x, y);
            talk.unk53CCE8 = x;
            talk.unk53CCEC = y;
            enemy.SetPosition(static_cast<i16>(x), static_cast<i16>(y));
            talk.cursor += 2 * sizeof(u16);
            talk.unk53CE90 = static_cast<i32>(static_cast<u32>(talk.unk53CE90) & kLowWordMask);
            return 1;
        }

        case kTalkDefeatPose:
            this->battleEnemies[static_cast<std::size_t>(talk.Word(1))].SetDefeatPoseFixed(
                static_cast<u8>(talk.UnsignedWord(2))
            );
            talk.cursor += 3 * sizeof(u16);
            return 1;

        case kTalkAttributes:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            this->TalkUnit(this->TalkCommandUnit()).SetAttributes(talk.Word(2), talk.Word(3), talk.Word(4));
            talk.cursor += 5 * sizeof(u16);
            return 1;

        case kTalkAddLoot:
            return this->FightTalkAddLoot();

        case kTalkAllCounted:
            return this->FightTalkCounters();

        case kTalkCurve:
            return this->FightTalkCurve();

        case kTalkSlide:
            return this->FightTalkSlide();

        case kTalkSpeaker:
            return this->FightTalkSpeaker();

        case kTalkTimedWindow:
            return this->FightTalkTimedWindow();

        case kTalkMirror:
            return this->FightTalkMirror();

        case kTalkLineWindow:
            return this->FightTalkLineWindow();

        case kTalkFieldFade:
            return this->FightTalkFieldFade();

        case kTalkHasObjects:
            return this->FightTalkObjects(false);

        case kTalkTakeObjects:
            return this->FightTalkObjects(true);

        case kTalkSlideTo:
            return this->FightTalkSlideTo();

        case kTalkEnemyCommand:
            return this->FightTalkEnemyCommand();

        case kTalkLearn:
            talk.unk53CCE8 = talk.Word(1);
            talk.unk53CCEC = talk.UnsignedWord(2);

            if (LearnSkill(
                    talk.unk53CCE8 - BattleState::FirstRoleUnit,
                    static_cast<i16>(talk.unk53CCEC),
                    this->party,
                    this->database,
                    battle.lastLearned,
                    this->errors
                ) == 1)
            {
                battle.unk53BFFC = 1;
            }

            talk.unk53CCE8 = 0;
            talk.unk53CCEC = 0;
            talk.cursor += 3 * sizeof(u16);
            return 1;

        case kTalkWords66:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            this->TalkUnit(this->TalkCommandUnit())
                .SetWords66(talk.Word(2), talk.Word(3), talk.Word(4), talk.Word(5), talk.Word(6), talk.Word(7));
            talk.cursor += 8 * sizeof(u16);
            return 1;

        case kTalkBackgroundAgain:
            battle.panelFlags |= kBackgroundPanel;
            this->LoadBattleBackground(
                battle.formation.background,
                battle.formation.animation,
                battle.formation.animationX,
                battle.formation.animationY,
                1
            );
            talk.cursor += sizeof(u16);
            return 1;

        case kTalkPartyOrder:
            if (talk.Word(1) <= kLastEnemy)
            {
                // The original tests the addresses of BattleUnit::Unk2B04 and Unk2B00 (never zero): an enemy is
                // not ordered.
                talk.unk53CCE8 = talk.unk53CE70;
                talk.cursor += 2 * sizeof(u16);
                return 1;
            }

            talk.unk53CE6C = battle.currentUnit;
            battle.unk53BD50 = talk.Word(1);
            battle.unk4A754C = talk.Word(2) + 1;

            if (battle.unk4A754C > kLastEnemy)
            {
                battle.unk53BFD0 = 1;
                battle.unk4A754C -= BattleState::FirstRoleUnit;
                battle.pendingOrders[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)].unk00 = 1;
            }

            this->battleRoles[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)].SetTarget(1);
            battle.unk53BCE8 = 1;
            battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = 1;
            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkMusic:
            return this->FightTalkMusic();

        case kTalkFadeWait:
            if (battle.fieldFade.frames <= 0)
            {
                talk.cursor += sizeof(u16);
            }

            talk.unk4A7B58 = 0;
            this->BattleFrame();
            talk.unk4A7B58 = 1;
            return 1;

        case kTalkPictureWait:
        {
            const u16 unit = talk.UnsignedWord(1);
            this->SetTalkCommandUnit(unit);
            talk.unk53CE98 = talk.UnsignedWord(2);

            if (this->TalkUnit(unit).ShowsPicture(static_cast<i16>(talk.unk53CE98)))
            {
                this->SetTalkCommandUnit(0);
                talk.unk53CE98 = 0;
                talk.cursor += 3 * sizeof(u16);
            }

            this->BattleFrame();
            this->SetTalkCommandUnit(0);
            talk.unk53CE98 = 0;
            return 1;
        }

        case kTalkCritical:
            battle.criticalAttack = 1;
            talk.cursor += sizeof(u16);
            this->BattleFrame();
            return 1;

        case kTalkPlace:
            talk.unk53CCE8 = talk.Word(1);
            talk.unk53CE98 = talk.UnsignedWord(2);
            talk.unk53CE9A = talk.UnsignedWord(3);
            this->TalkUnit(talk.unk53CCE8)
                .SetPosition(static_cast<i16>(talk.unk53CE98), static_cast<i16>(talk.unk53CE9A));
            talk.unk53CCE8 = 0;
            talk.unk53CE98 = 0;
            talk.unk53CE9A = 0;
            talk.unk4A7B58 = 0;
            this->BattleFrame();
            talk.unk4A7B58 = 1;
            talk.cursor += 4 * sizeof(u16);
            return 1;

        case kTalkPanel8:
            talk.cursor += sizeof(u16);
            battle.panelFlags |= BattleState::UnkPanel8;
            return 1;

        case kTalkLevel:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            this->TalkUnit(this->TalkCommandUnit()).SetLevel(talk.Word(2));
            talk.cursor += 3 * sizeof(u16);
            return 1;

        case kTalkLose:
            talk.cursor += sizeof(u16);
            battle.defeatedRoles = static_cast<u8>(battle.roleCount);
            battle.panelFlags |= kLostPanel;
            battle.result = kBattleLostState;
            return 1;

        case kTalkBackground:
            this->LoadBattleBackground(talk.Word(1), talk.UnsignedWord(2), talk.Word(3), talk.Word(4), 1);
            talk.cursor += 5 * sizeof(u16);
            return 1;

        case kTalkResistances:
        {
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            std::array<i16, kResistanceWords> values = {};

            for (std::size_t index = 0; index < values.size(); ++index)
            {
                values[index] = talk.Word(index + 2);
            }

            this->TalkUnit(this->TalkCommandUnit()).SetResistances(values);
            talk.cursor += (kResistanceWords + 2) * sizeof(u16);
            return 1;
        }

        case kTalkWords72:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            this->TalkUnit(this->TalkCommandUnit()).SetWords72(talk.Word(2), talk.Word(3), talk.Word(4), talk.Word(5));
            talk.cursor += 6 * sizeof(u16);
            return 1;

        case kTalkKill:
        {
            const u16 unit = talk.UnsignedWord(1);
            this->SetTalkCommandUnit(unit);

            if (unit <= kLastEnemy)
            {
                this->battleEnemies[unit].Damage(this->battleServices, kEnemyKillHeal, 0, 0);
            }
            else
            {
                this->TalkUnit(unit).Damage(this->battleServices, -kMaxVital, kMaxVital, kMaxVital);
            }

            talk.cursor += 2 * sizeof(u16);
            return 1;
        }

        case kTalkPanel40:
            talk.cursor += sizeof(u16);
            battle.panelFlags |= kNoFirstStrikePanel;
            return 1;

        case kTalkCatch:
            return this->FightTalkCatch();

        case kTalkDamage:
            return this->FightTalkDamage();

        case kTalkReload:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));
            talk.unk53CCE8 = talk.Word(2);
            this->battleEnemies[this->TalkCommandUnit()].Reload(static_cast<u16>(talk.unk53CCE8), this->database);
            talk.cursor += 3 * sizeof(u16);
            return 1;

        case kTalkUsedItem:
            this->SetTalkCommandUnit(talk.UnsignedWord(1));

            if (battle.unk53BF34 == this->TalkCommandUnit())
            {
                battle.unk53BF34 = 0;
                this->SetTalkCommandUnit(0);
                talk.cursor += 2 * sizeof(u16);
                talk.Seek(talk.Word(0));
                return 1;
            }

            talk.cursor += 4 * sizeof(u16);
            return 1;

        case kTalkQuake:
            battle.unk53BD68 = kQuakeFrames;
            this->SetTalkCommandUnit(talk.UnsignedWord(1));

            if (talk.UnsignedWord(1) == 1)
            {
                battle.panelFlags |= BattleState::UnkPanel100;
            }
            else
            {
                battle.panelFlags &= ~BattleState::UnkPanel100;
                battle.unk53BD68 = 0;
            }

            talk.cursor += 2 * sizeof(u16);
            return 1;

        case kTalkNoRetreat:
            battle.panelFlags |= BattleState::NoRetreat;
            talk.cursor += sizeof(u16);
            return 1;

        default:
            return 1;
    }
}

// Command 0x35 of FightTalkFrame (inline in the original): one of the object of the second word added to the
// inventory and to the loot of the battle. The original writes the loot past its ten slots after them: those are
// not kept.
int Game::FightTalkAddLoot()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    FightTalk& talk = this->fightTalk;
    const std::size_t slot = messages.lootCount;
    const u16 id = talk.UnsignedWord(1);
    ObjectRecord* record = AddOneItem(this->party.inventory, id, 1, this->database);

    if (slot < messages.lootNames.size())
    {
        messages.lootNames[slot] = record != nullptr
                                       ? std::string_view(record->data.name.data(), ObjectName(*record).size())
                                       : std::string_view();
        ++messages.lootCounts[slot];
        battle.lootIds[slot] = id;
    }

    ++messages.lootCount;
    talk.cursor += 2 * sizeof(u16);
    return 1;
}

// Command 0x3C of FightTalkFrame (inline in the original): the battle music becomes music\<name> (the text after it,
// ended by %Q).
int Game::FightTalkMusic()
{
    BattleState& battle = this->battleState;
    FightTalk& talk = this->fightTalk;
    talk.textAt = talk.cursor + sizeof(u16);
    std::size_t length = 0;

    while (length < kTextEndSearch)
    {
        const std::size_t at = talk.textAt + length;

        if (at + 1 < talk.script.size() && talk.script[at] == kTextEscape && talk.script[at + 1] == kTextEnd)
        {
            break;
        }

        ++length;
    }

    const std::size_t available = talk.textAt < talk.script.size() ? talk.script.size() - talk.textAt : 0;
    const std::string name(
        reinterpret_cast<const char*>(talk.script.data()) + std::min(talk.textAt, talk.script.size()),
        std::min(length, available)
    );
    talk.textAt += length + 2;
    talk.cursor = talk.textAt;
    battle.battleMusic = PathFormat("%smusic\\%s", {this->config.installDirectory, std::string_view(name.c_str())});
    this->audio.StopMusic();
    this->PlayMusic(battle.battleMusic);
    this->audio.SetMusicVolume(this->config.musicVolume);
    return 1;
}

// Command 0x3D of FightTalkFrame (inline in the original): the units listed after it (ended by 0xFFFF) are checked in
// turn; once as many have no counter (Counter2A76) as are listed, the talk jumps to the place after the list;
// otherwise it goes on two words past that place. An empty list keeps the talk on the command.
int Game::FightTalkCounters()
{
    FightTalk& talk = this->fightTalk;
    u16 count = 0;
    talk.unk53CE9E = 0;
    talk.unk4A7B58 = 0;

    while (talk.UnsignedWord(static_cast<std::size_t>(count) + 1) != kListEnd)
    {
        ++count;
        talk.unk53CE9E = count;
    }

    u16 cleared = 0;

    if (count != 0)
    {
        for (u16 index = 0; index < count; ++index)
        {
            const u16 unit = talk.UnsignedWord(static_cast<std::size_t>(index) + 1);
            talk.unk4A7B58 = 0;
            this->SetTalkCommandUnit(unit);

            if (this->TalkUnit(unit).Counter2A76() == 0 && ++cleared == count)
            {
                talk.cursor += (static_cast<std::size_t>(count) + 2) * sizeof(u16);
                talk.Seek(talk.Word(0));
                break;
            }
        }
    }

    talk.unk53CE94 = 0;

    if (cleared == count)
    {
        talk.unk53CE9E = 0;
    }
    else
    {
        talk.cursor += (static_cast<std::size_t>(count) + kCountedSkipWords) * sizeof(u16);
    }

    return 1;
}

}  // namespace swd3
