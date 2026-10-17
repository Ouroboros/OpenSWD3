#include "core/fault.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "font/text_font.h"
#include "game/game.h"
#include "item/item_list.h"
#include "org/fame.h"
#include "platform/display.h"
#include "text/big5.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr u16 kAimNone = 0x8000;  // GetWord29A2: the role aims at no enemy
constexpr i32 kBattleWon = 0x63;
constexpr i32 kBattleLost = 0x67;
constexpr i32 kTurnsRunning = 1;   // BattleState::result while no side has lost
constexpr u16 kBossReward = 0x14;  // unk53BF16 gained by an enemy of ObjectData::UnkKind20
constexpr u16 kEnemyReward = 3;
constexpr i16 kDefeatCount = 1;  // the bestiary count of a defeated enemy
constexpr std::size_t kLastTurnUnit = BattleState::RoleUnits - 1;
constexpr i32 kFieldFadeStep = 2;
constexpr i32 kFieldFadeEnd = -0x20;
constexpr u16 kThrownObject = 0x300;  // the object a thrown attack takes (given back unless it lands)
constexpr i32 kLostEnd = 2;           // unk53BDC8 when the party is down
constexpr i32 kLostQuietly = 0x68;    // a battle state that ends with unk53BDC8 0
constexpr i32 kPromptFrames = 0x12C;
constexpr i32 kWaitState1 = 1;  // the battle states of the wait pointer
constexpr i32 kWaitState2 = 2;
constexpr i32 kTurnState = 3;
constexpr i32 kWaitState4 = 4;
constexpr i32 kWaitState5 = 5;
constexpr i32 kWaitState8 = 8;
constexpr i32 kWaitState1B = 0x1B;
constexpr i32 kWaitState1E = 0x1E;
constexpr int kNoUnitPointer = 0x238C;
constexpr int kUnitPointer = 0x238D;
constexpr int kWaitPointer = 0x238E;
constexpr int kTurnPointer = 0x238F;
constexpr int kNoTurnPointer = 0x2393;
constexpr int kMarkEffect = 0x23A0;
constexpr i32 kListPage = 7;  // a back key scrolls a list up by this
constexpr u16 kBackSound = 0x2E;
constexpr i32 kListState2 = 2;
constexpr i32 kListState4 = 4;
constexpr i32 kListState1B = 0x1B;
constexpr i32 kGuardOrder = 5;                               // the order of a role that guards in the auto battle
constexpr i32 kFirstPanelUnit = BattleState::FirstRoleUnit;  // the four party roles have the panels
constexpr i32 kLastPanelUnit = BattleState::FirstRoleUnit + 3;
constexpr u16 kNoPanelRole = 0xFFFF;
constexpr i32 kMenuState = 1;          // the command menu
constexpr i32 kMenuColumn = 4;         // commands per column
constexpr i32 kMenuFixedCommands = 5;  // before the accessory commands
constexpr i32 kListPages = 3;
constexpr i32 kMenuStep = 5;
constexpr i32 kMenuStepMost = 10;
constexpr std::size_t kFixedCommands = 5;  // the commands of the menu before the accessory ones
constexpr std::size_t kAccessorySlots = 2;
constexpr std::size_t kFirstAccessorySlot = 7;  // the equipment slots of the accessories
constexpr u16 kFirstAccessoryCommand = 0x15;    // the accessory kinds that add a command
constexpr u16 kLastAccessoryCommand = 0x31;
constexpr u16 kSpiritPact = 6;
constexpr i32 kGuardedCommand = 2;
constexpr i32 kFuryFullBlock = 0x38;  // roleBlock[0] of a full fury gauge
constexpr u16 kHighlightBackground = 0xF000;
constexpr int kMenuRowHeight = 0x18;
constexpr int kMenuColumnWidth = 0x30;
constexpr i32 kTargetState = 3;  // battle states of the menu
constexpr i32 kTalkState = 7;
constexpr i32 kSkillListState = 8;
constexpr i32 kSpellCommand = 2;  // the menu commands that open a list
constexpr i32 kItemCommand = 3;
constexpr i32 kSkillCommand = 4;
constexpr u16 kNoTalkRole = 0xFFFF;
constexpr i32 kListRows = 5;
constexpr int kTabEffect = 0x2394;  // the page tabs of the list of state 2
constexpr int kTabsDrop = 0x20;
constexpr int kTabWidth = 0x2A;
constexpr int kOpenTabFrame = 4;
constexpr i32 kListPanelFrames = 0xA;
constexpr int kListBoxDrop = 0x24;
constexpr int kListBoxWidth = 0xBE;
constexpr int kListBoxHeight = 0x98;
constexpr int kListShade = 4;
constexpr int kListFrameInset = 6;
constexpr int kListFrameTop = 0x28;
constexpr int kListFrameRight = 0xBA;
constexpr int kListFrameBottom = 0xB8;
constexpr int kListFrameBorder = BattleMessage::Frame | 8;
constexpr i32 kListGrowStep = 2;
constexpr i32 kListGrowMost = 7;
constexpr int kThrowBoxHeight = 0xBC;  // the 投擲 list
constexpr int kThrowShade = -8;
constexpr int kThrowTitleTop = 8;
constexpr int kThrowTitleBottom = 0x18;
constexpr int kThrowTitleX = 0x50;
constexpr std::u8string_view kThrowTitle = u8"投擲";  // swd32.exe 0x004A76A0
constexpr u16 kTitleColor = 0xFFC0;
constexpr int kListAdvance = 0x10;
constexpr int kThrowCategory = 4;
constexpr int kListRowsShown = 7;
constexpr int kListRowHeight = 0x14;
constexpr int kListRowTop = 0x2C;
constexpr int kListNameX = 0x10;
constexpr int kListCountX = 0x90;
constexpr std::string_view kCountFormat = "%2d";  // swd32.exe 0x004A79CC
constexpr int kListBarTop = 0x28;
constexpr int kListBarWidth = 0xC2;
constexpr int kListBarHeight = 0x18;
constexpr int kListBarRed = 0x1F;
constexpr int kListBarGreen = 0x14;
constexpr int kRefineBoxWidth = 0xF2;  // the 煉符 grid
constexpr int kRefineBoxHeight = 0x9C;
constexpr int kRefineFrameRight = 0xEE;
constexpr int kRefineFrameBottom = 0x98;
constexpr int kRefineTitleX = 0x6A;
constexpr u16 kRefineCommand = 0x1E;
constexpr int kRefineColumns = 2;
constexpr int kRefineRows = 5;
constexpr int kRefineBarLeft = 4;
constexpr int kRefineBarUp = 3;
constexpr int kRefineBarWidth = 0x6A;
constexpr int kRefineColumnWidth = 0x70;
constexpr std::u8string_view kNoObject = u8"無";  // swd32.exe 0x0049F9FC
constexpr int kSpecialBoxHeight = 0xAC;           // the 絕招 list
constexpr int kSpecialFrameBottom = 0xA8;
constexpr std::size_t kSpecialCommandName = 3;
constexpr u16 kSkillListEnd = 0xFFFF;
constexpr int kSpecialRowHeight = 0x16;
constexpr int kSpecialBarTop = 0x25;
constexpr int kSpecialBarWidth = 0xC0;
constexpr int kEscortShadeX = 0xC4;  // the 佈置護駕 panel
constexpr int kEscortShadeY = 0xB0;
constexpr int kEscortShadeWidth = 0xBC;
constexpr int kEscortShadeHeight = 0x58;
constexpr int kEscortFrameLeft = 0xC8;
constexpr int kEscortFrameRight = 0x178;
constexpr int kEscortTitleTop = 0xB4;
constexpr int kEscortTitleBottom = 0xC4;
constexpr int kEscortTitleX = 0xFC;
constexpr std::u8string_view kEscortTitle = u8"佈置護駕";  // swd32.exe 0x004A79D0
constexpr int kEscortBodyTop = 0xD4;
constexpr int kEscortBodyBottom = 0x104;
constexpr int kEscortBarLeft = 2;
constexpr int kEscortBarUp = 0x12;
constexpr int kEscortAdvance = 0x12;
constexpr int kEscortRowTop = 8;
constexpr int kEscortNameX = 0xC;
constexpr int kEscortNoneTop = 0x1E;
constexpr u16 kWhite = 0xFFFF;
constexpr int kTagMirrorRight = 0x276;  // the name tag of the aimed enemy
constexpr int kTagCharWidth = 0x14;
constexpr int kTagMargin = 4;
constexpr int kTagFrameHeight = 0x10;
constexpr int kTagNameX = 2;
constexpr u16 kKnownHitPoints = 10;  // bestiary counts that show the hit points and the gauge
constexpr u16 kKnownGauge = 0xF;
constexpr std::u8string_view kHitPointsFormat = u8"生命:%d/%d";  // swd32.exe 0x004A79DC
constexpr int kTagHitPointsLeft = 0x8C;
constexpr int kTagHitPointsGap = 0xA;
constexpr int kTagHitPointsUp = 2;
constexpr int kTagGaugeTop = 0x11;
constexpr int kTagGaugeHeight = 3;
constexpr u32 kTagGaugeRed = 0x18;
constexpr int kControlTiles = 0x234E;  // the 控制 menu
constexpr int kControlBoxLeft = 8;
constexpr int kControlTitleUp = 0x28;
constexpr int kControlTitleTextUp = 0x20;
constexpr int kControlColumns = 4;
constexpr int kControlTitleRows = 2;
constexpr i8 kControlFill = 0x48;
constexpr u16 kControlCommand = 0x19;
constexpr int kControlObjects = 3;
constexpr int kControlSkills = 2;
constexpr std::u8string_view kSpecialFormat = u8"特殊%d";  // swd32.exe 0x004A79F0
constexpr std::u8string_view kReleaseName = u8"釋放";      // swd32.exe 0x004A79E8
constexpr u16 kStaminaCostBit = 0x8000;                    // the spell list
constexpr int kElementIcons = 0x241C;
constexpr int kSpellIconLeft = 4;
constexpr int kSpellRowTop = 0x25;
constexpr int kSpellNameX = 0xC;
constexpr std::string_view kCostFormat = "%3d";
constexpr int kSpellBarLeft = 8;
constexpr int kSpellBarUp = 4;
constexpr int kDescriptionTop = 0x15E;
constexpr int kDescriptionHeight = 0x28;
constexpr int kDescriptionGreen = 5;
constexpr int kDescriptionBlue = 0x1F;
constexpr int kDescriptionCentre = 0x140;
constexpr int kDescriptionTextTop = 0x168;
constexpr int kItemPages = 4;  // the object list
constexpr int kItemBoxWidth = 0xCC;
constexpr int kItemFrameRight = 0xC8;
constexpr u16 kShownItem = 0x8000;
constexpr u16 kCostlyItem = 0x4000;
constexpr int kItemBarWidth = 0xCE;
constexpr int kVictoryShadeExtra = 0x28;  // the 戰鬥勝利 panel
constexpr int kVictoryTitleX = 0xF8;
constexpr std::u8string_view kVictoryTitle = u8"戰鬥勝利";  // swd32.exe 0x004A7A2C
constexpr u16 kGoldGiven = 0x8000;                          // BattleState::unk53BF12: the gold of the battle is given
constexpr u16 kGoldMask = 0x7FFF;
constexpr u16 kVictorySound = 0x12C;
constexpr std::size_t kBattlesFought = 1;  // RoleBattleBlock::unk00 [1] (inferred: battles fought)
constexpr u32 kManyEnemies = 3;
constexpr std::size_t kGoldVariable = 0;
constexpr int kVictoryBodyBottom = 0x11C;
constexpr int kVictoryGrowDivisor = 2;
constexpr int kVictoryTextX = 0xD0;
constexpr int kVictoryExperienceY = 0xDC;
constexpr int kVictoryGoldY = 0xF0;
constexpr int kVictoryProficiencyY = 0x104;
constexpr std::u8string_view kExperienceLine = u8"每人得到經驗值:%d";   // swd32.exe 0x004A7A18
constexpr std::u8string_view kGoldLine = u8"得到銀幣:%d";               // swd32.exe 0x004A7A0C
constexpr std::u8string_view kProficiencyLine = u8"得到法寶經驗值:%d";  // swd32.exe 0x004A79F8
constexpr u8 kNoLevelUp = 0xFF;                                         // the 升級 panel
constexpr int kLevelUpTitleX = 0x108;
constexpr std::u8string_view kLevelUpTitle = u8"升級";  // swd32.exe 0x004A7A44
constexpr int kLevelUpBodyBottom = 0xF4;
constexpr int kLevelUpGrowDivisor = 3;
constexpr int kLevelUpTextY = 0xDC;
constexpr u16 kLevelUpSound = 0x12B;
constexpr std::u8string_view kNewSpellLine = u8"%s學到新魔法";    // swd32.exe 0x004A7AB4
constexpr std::u8string_view kNewSpecialLine = u8"%s學到新絕招";  // swd32.exe 0x004A7AC4
constexpr std::string_view kBracketFormat = "[%s]";               // swd32.exe 0x004A7AAC
constexpr u16 kLearnedSound = 0x160;
constexpr int kLearnedShadeX = 0xF6;
constexpr int kLearnedFrameLeft = 0xFA;
constexpr int kLearnedBodyBottom = 0xEC;
constexpr int kLearnedTextX = 0x104;
constexpr int kLearnedTextY = 0xBC;
constexpr int kLearnedSkillY = 0xD2;
constexpr u16 kLearnedSkillColor = 0xF000;
constexpr std::size_t kNeededBattles = 2;  // RoleBattleBlock::unk00: the battles the next special needs
constexpr std::size_t kNextSpecial = 3;    // the next special skill with bit 31, -1: none
constexpr i32 kNoSpecial = -1;
constexpr u32 kSpecialMark = 0x80000000u;
constexpr u32 kSpecialIdMask = 0x7FFFFFFF;
constexpr u16 kSpecialType = 0x1F;
constexpr int kSpecialsFlag = 0x1BB0;
constexpr u32 kBarredSpecialA = 0x665;
constexpr u32 kBarredSpecialB = 0x669;
constexpr std::size_t kStatRolls = 9;  // the stats of a level-up
constexpr std::size_t kStatLines = 7;
constexpr int kStatsShadeY = 0x4C;
constexpr int kStatsShadeWidth = 0xD4;
constexpr int kStatsTitleTop = 0x50;
constexpr int kStatsTitleBottom = 0x60;
constexpr int kStatsFrameRight = 0x190;
constexpr int kStatsTitleX = 0x118;
constexpr int kStatsBodyTop = 0x70;
constexpr int kStatsBodyBottom = 0x10C;
constexpr int kStatsTextX = 0xD8;
constexpr int kStatsFirstY = 0x78;
constexpr int kStatsLineHeight = 0x14;
constexpr std::array<std::u8string_view, kStatLines> kStatLabels = {
    u8"生命力:%4d",  // swd32.exe 0x004A7AA0
    u8"魔法力:%4d",  // swd32.exe 0x004A7A94
    u8"體  力:%4d",  // swd32.exe 0x004A7A88
    u8"力  量:%4d",  // swd32.exe 0x004A7A7C
    u8"耐  力:%4d",  // swd32.exe 0x004A7A70
    u8"智  慧:%4d",  // swd32.exe 0x004A7A64
    u8"敏  捷:%4d",  // swd32.exe 0x004A7A58
};

constexpr u16 kShowNow = 0x64;  // BattleState::unk53BDEA: the new values show at once
constexpr u16 kRollStart = 0x1E;
constexpr std::string_view kArrow = " ==> ";         // swd32.exe 0x004A7A50
constexpr std::string_view kNewValueFormat = "%4d";  // swd32.exe 0x004A7A4C
constexpr int kStatsArrowX = 0x136;
constexpr int kStatsNewX = 0x158;
constexpr u16 kRollSound = 0x125;

// The name of accessory command `kind`: kinds past the table read the variables after it in the original; no name
// here.
std::u8string_view AccessoryCommandName(u16 kind)
{
    const auto at = static_cast<std::size_t>(kind - kFirstAccessoryCommand);
    return at < BattleState::AccessoryCommandNames.size() ? BattleState::AccessoryCommandNames[at]
                                                          : std::u8string_view{};
}

}  // namespace

// swd32.exe 0x00464CC0: role unit `unit` takes the turn (unk53BD50, its unit turn and SetTarget 1); while an enemy
// stands, unk4A754C picks one at random (1-based), going on past defeated ones for at most enemyCount tries.
void Game::StartRoleTurn(int unit)
{
    BattleState& battle = this->battleState;
    battle.unk53BD50 = unit;
    battle.unk53BCE8 = 1;
    battle.unk53BF60 = 1;
    battle.unk53AF38[static_cast<std::size_t>(unit)] = 1;
    this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].SetTarget(1);
    const auto count = static_cast<i32>(battle.enemyCount);

    if (battle.unk53BF00 >= count)
    {
        return;
    }

    battle.unk4A754C = static_cast<i32>(this->random.Below(battle.enemyCount)) + 1;
    int tries = 0;

    while (this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)].IsDefeated())
    {
        ++battle.unk4A754C;

        if (battle.unk4A754C > count)
        {
            battle.unk4A754C = 1;
        }

        if (++tries >= count)
        {
            break;
        }
    }
}

// swd32.exe 0x0045ADF0: when `enemy` is unk4A7626 its turn is reset, and each role but `role` aiming at it turns to
// the first standing enemy other than it (unk53C010 set). With none and one enemy left, such a role's aim, command,
// target and turn are reset and the turn order is cleared.
void Game::RetargetRoles(int role, int enemy)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if (battle.unk4A7626 != enemy)
    {
        return;
    }

    this->battleEnemies[static_cast<std::size_t>(enemy)].ResetTurn(services);

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        if (static_cast<int>(index) == role)
        {
            continue;
        }

        BattleRole& unit = this->battleRoles[index];
        const u16 aim = unit.GetWord29A2();

        if ((aim & kAimNone) != 0 || aim != battle.unk4A7626)
        {
            continue;
        }

        const auto count = static_cast<i32>(battle.enemyCount);
        bool turned = false;

        for (int other = 0; other < count; ++other)
        {
            if (battle.unk4A7626 != other && !this->battleEnemies[static_cast<std::size_t>(other)].IsDefeated())
            {
                unit.ClearWord29A2();
                this->battleEnemies[aim].DecreaseHits();
                unit.Retarget(static_cast<i16>(other));
                battle.unk53C010 = 1;
                turned = true;
                break;
            }
        }

        if (turned || battle.unk53BF00 + 1 != count)
        {
            continue;
        }

        unit.ClearWord29A2();
        this->battleEnemies[aim].DecreaseHits();
        unit.SetCommand(0);
        unit.SetTarget(0);
        unit.ResetTurn(services);
        battle.unk53BF64 = 0;
        battle.unk520DD0 = {};
        battle.unk53BD50 = 0;
        battle.currentUnit = 0;
        battle.unk53AE70 = -1;
        battle.WipeSlotRecords();
    }
}

// swd32.exe 0x0045AA00: one frame of the defeat of unit `index` (a role when `role` is 1, else an enemy, none for
// -1); 1 once it is over. An enemy adds its rewards (unk53BF14, unk53BF12, unk53BF16), its bestiary count and the
// defeated count; all down wins the battle, and a lone standing first enemy of a unk53BF28 battle ends the others. A
// role counts as defeated (a forced one as a defeated guest: once all are down their places go and the units are
// measured again), drops its command, slot record, turn and order; with no party role standing the battle is lost,
// else one with EffectFlag2 takes the next turn.
int Game::RemoveDefeated(int index, int role)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if (role != 1)
    {
        if (index == -1)
        {
            return 0;
        }

        BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(index)];

        if (enemy.DefeatFrame(services) != 1)
        {
            return 0;
        }

        u16 first = 0;
        u16 second = 0;
        enemy.Unk62And8A(first, second);
        battle.unk53BF14 = static_cast<u16>(battle.unk53BF14 + first);
        battle.unk53BF12 = static_cast<u16>(battle.unk53BF12 + second);
        battle.unk5028FC = enemy.Data();
        const bool boss = (battle.unk5028FC->data.kind & ObjectData::UnkKind20) != 0;
        battle.unk53BF16 = static_cast<u16>(battle.unk53BF16 + (boss ? kBossReward : kEnemyReward));
        AddCounterCapped(this->party.fame.lists[Fame::FameList], static_cast<i16>(enemy.ActId()), kDefeatCount);
        battle.RemoveSlotRecord(index);
        const auto count = static_cast<i32>(battle.enemyCount);

        if (index >= 0 && index <= count)
        {
            ++battle.unk53BF00;
        }

        if (battle.unk53BF00 - battle.unk53BF02 >= count)
        {
            battle.result = kBattleWon;
            battle.autoBattle = 0;
            battle.unk53BFC4 = 1;
            battle.unk53BFC0 = 1;
        }

        if (battle.unk53BF28 != 0 && !this->battleEnemies[0].IsDefeated() &&
            static_cast<i32>(battle.enemyCount) - battle.unk53BF00 == 1)
        {
            battle.enemyCount = 1;
            battle.unk53BF00 = 0;
            battle.unk53BF28 = 0;
            this->MeasureUnits();
            battle.SortDrawOrder();
            battle.ListEnemyOrder();
        }

        return 1;
    }

    BattleRole& unit = this->battleRoles[static_cast<std::size_t>(index)];

    if (unit.DefeatFrame(services) != 1)
    {
        return 0;
    }

    if (unit.unk2B00 == 1)
    {
        ++battle.unk53BF01;

        if (battle.unk53BF01 >= battle.unk53BF26)
        {
            const u16 guests = battle.unk53BF26;

            // The original clears the places from the end of the list down, before it when there are fewer.
            for (u16 cleared = 0; cleared < guests && cleared < battle.roleCount; ++cleared)
            {
                battle.rolePlaces[battle.roleCount - 1 - cleared] = UnitPlacement{};
            }

            battle.unk53BF26 = 0;
            battle.roleCount -= guests;
        }

        this->MeasureUnits();
        battle.SortDrawOrder();
        battle.ListEnemyOrder();
    }
    else
    {
        ++battle.defeatedRoles;
    }

    unit.CancelCommand();
    const int unitId = index + BattleState::FirstRoleUnit;
    battle.RemoveSlotRecord(unitId);
    battle.unk524268[static_cast<std::size_t>(index)] = 0;

    if (battle.currentUnit == unitId)
    {
        for (u32 other = 0; other < battle.roleCount; ++other)
        {
            this->battleRoles[other].SetAC0(0);
        }

        for (u32 other = 0; other < battle.enemyCount; ++other)
        {
            this->battleEnemies[other].SetAC0(0);
        }

        battle.currentUnit = 0;
        battle.unk53BCE8 = 0;
        battle.unk4A754C = 1;
        battle.result = kTurnsRunning;
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.unk53BF74 = 0;
    }

    if (battle.unk53AE70 == unitId)
    {
        battle.unk53AE70 = -1;
        battle.unk53BF74 = 0;
    }

    for (std::size_t at = 0; static_cast<i32>(at) < static_cast<i32>(battle.roleCount); ++at)
    {
        if (battle.unk520DD0[at] != unitId)
        {
            continue;
        }

        for (std::size_t next = at; next < kLastTurnUnit; ++next)
        {
            battle.unk520DD0[next] = battle.unk520DD0[next + 1];
        }

        battle.unk520DD0[kLastTurnUnit] = 0;
        break;
    }

    if (battle.defeatedRoles >= battle.roleCount - battle.guestCount - battle.unk53BF26)
    {
        battle.WipeSlotRecords();
        battle.unk53AE70 = -1;
        battle.unk53BFC0 = 1;
        battle.unk53BFC4 = 1;
        battle.result = kBattleLost;
        return 1;
    }

    if (unit.EffectFlag2() == 1)
    {
        battle.unk4A754C = battle.unk53AE70 + 1;
        battle.unk53BD50 = unitId;
        unit.SetTarget(1);
        battle.unk53BCE8 = 1;
        battle.unk53AF38[static_cast<std::size_t>(unitId)] = 1;
        battle.unk53BF60 = 1;
        battle.pendingOrders[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)] = PendingOrder{};
    }

    return 1;
}

// swd32.exe 0x0045B280: the draw order with the acting unit (unk53AE70) drawn right after the unit it aims at (none
// while unk53C4C0, unk53AE7C or unk53AE80 is set or no unit acts): a role aims at an enemy, an enemy at a role.
void Game::SortActingDrawOrder()
{
    BattleState& battle = this->battleState;

    if (battle.unk53C4C0 == 1 || battle.unk53AE7C == 1 || battle.unk53AE80 == 1 || battle.unk53AE70 == -1)
    {
        return;
    }

    const i32 acting = battle.unk53AE70;
    i32 target = 0;

    if (acting >= BattleState::FirstRoleUnit)
    {
        const auto role = static_cast<std::size_t>(acting - BattleState::FirstRoleUnit);
        target = static_cast<i16>(this->battleRoles[role].GetWord29A2());

        // Never so here (the original returned above).
        if (battle.unk53AE7C == 1)
        {
            target += BattleState::FirstRoleUnit;
        }
    }
    else
    {
        target = static_cast<i16>(this->battleEnemies[static_cast<std::size_t>(acting)].GetWord29A2());

        if (battle.unk53AE80 == 0)
        {
            target += BattleState::FirstRoleUnit;
        }
    }

    battle.SortDrawOrderAround(acting, target);
}

// swd32.exe 0x0045BD10: one frame of the field fading to black: the frame's pixels shifted by the fade (unk53C4A8,
// also the painter's tint), 2 darker a frame; 1 once past -32 (the fade reset to 0).
int Game::FadeFieldOut()
{
    BattleState& battle = this->battleState;
    DrawOptions& options = this->painter.Options();
    options.tintRed = battle.unk53C4A8;
    options.tintGreen = battle.unk53C4A8;
    options.tintBlue = battle.unk53C4A8;
    const std::span<u16> frame(this->drawSurface, static_cast<std::size_t>(Display::Width * Display::Height));
    this->painter.ShiftRed(frame, battle.unk53C4A8);
    this->painter.ShiftGreenPairs(frame, options.tintGreen);
    this->painter.ShiftBluePairs(frame, options.tintBlue);
    battle.unk53C4A8 -= kFieldFadeStep;

    if (battle.unk53C4A8 > kFieldFadeEnd)
    {
        return 0;
    }

    battle.unk53C4A8 = 0;
    return 1;
}

// swd32.exe 0x0045E9C0: the objects held back in the battle given to the inventory: unk4FF2EA[1] and [2], and one
// kThrownObject for each still thrown (unk53C4B0); the counts and unk53BF26 are cleared.
void Game::ReturnBattleItems()
{
    BattleState& battle = this->battleState;

    for (std::size_t at = 1; at < battle.unk4FF2EA.size(); ++at)
    {
        if (battle.unk4FF2EA[at] != 0)
        {
            AddOneItem(this->party.inventory, battle.unk4FF2EA[at], 0, this->database);
        }
    }

    for (i32 thrown = 0; thrown < battle.unk53C4B0; ++thrown)
    {
        AddOneItem(this->party.inventory, kThrownObject, 0, this->database);
    }

    battle.unk53BF26 = 0;
    battle.unk53C4B0 = 0;
    battle.unk4FF2EA[1] = 0;
    battle.unk4FF2EA[2] = 0;
}

// swd32.exe 0x0045E580: with every party role down (unk53BF5C set) the field fades out (unk53BFE4), then the music
// stops, the objects return and unk53BDC8 becomes 2 (0 for a battle state of 0x68 or UnkPanel8); with every enemy
// down or unk53CEAC set it fades out too, the objects return and unk53BDC8 becomes 0.
void Game::CheckBattleEnd()
{
    BattleState& battle = this->battleState;

    if (battle.defeatedRoles >= battle.roleCount - battle.unk53BF26 - battle.guestCount)
    {
        battle.unk53BF5C = 1;

        if (battle.unk53BFE4 == 1 && this->FadeFieldOut() == 1)
        {
            this->audio.StopMusic();
            this->ReturnBattleItems();
            battle.unk53BDC8 = kLostEnd;

            if (battle.result == kLostQuietly)
            {
                battle.unk53BDC8 = 0;
            }

            if ((battle.panelFlags & BattleState::UnkPanel8) != 0)
            {
                battle.unk53BDC8 = 0;
            }
        }
    }

    if (battle.unk53BF00 - battle.unk53BF02 >= static_cast<i32>(battle.enemyCount) || this->fightTalk.unk53CEAC == 1)
    {
        battle.unk53BF5C = 1;

        if (battle.unk53BFE4 == 1 && this->FadeFieldOut() == 1)
        {
            this->ReturnBattleItems();
            battle.unk53BDC8 = 0;
        }
    }
}

// swd32.exe 0x0045E660: the pointer prompt of the battle state for its first 300 frames (always while unk53BDAC is
// negative): kWaitPointer in states 1, 2, 4, 5, 8, 0x1B and 0x1E; in state 3 (with unk53BFBC) kTurnPointer or
// kNoTurnPointer by unk4A7640; otherwise kUnitPointer with a current unit, else kNoUnitPointer, or with a negative
// unk53BDAC the effect kMarkEffect at (unk53BF50, unk53BF52) (unk53BF58 cleared once it ends).
void Game::DrawBattlePrompt()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;
    const int x = this->state.pointerX;
    const int y = this->state.pointerY;
    ++battle.unk53BDA8;

    if (battle.unk53BDA8 >= kPromptFrames && battle.unk53BDAC >= 0)
    {
        return;
    }

    switch (battle.result)
    {
        case kWaitState1:
        case kWaitState2:
        case kWaitState4:
        case kWaitState5:
        case kWaitState8:
        case kWaitState1B:
        case kWaitState1E:
            hud.DrawEffectAt(kWaitPointer, 0, x, y, 0);
            break;

        case kTurnState:
            if (battle.unk53BFBC == 1)
            {
                hud.DrawEffectAt(battle.unk4A7640 != 0 ? kTurnPointer : kNoTurnPointer, 0, x, y, 0);
            }

            break;

        default:
            if (battle.unk53BDAC >= 0)
            {
                hud.DrawEffectAt(battle.currentUnit != 0 ? kUnitPointer : kNoUnitPointer, 0, x, y, 0);
            }
            else if (
                hud.DrawEffectAt(
                    kMarkEffect,
                    battle.unk53BF58,
                    battle.unk53BF50,
                    battle.unk53BF52,
                    battle.mirrored != 0 ? 1 : 0
                ) == 1
            )
            {
                battle.unk53BF58 = 0;
            }

            break;
    }
}

// swd32.exe 0x0045D2F0: one frame of the field fade (while any step is set): `counting` counts its frames down; while
// frames are left (not negative) the levels move by the steps, otherwise the steps take the fade's end levels (the
// original copies them into the steps, not the levels). The field rows of the frame are tinted by the levels.
void Game::FieldFadeFrame(int counting)
{
    ColorFade& fade = this->battleState.fieldFade;

    if (fade.stepRed == 0 && fade.stepGreen == 0 && fade.stepBlue == 0)
    {
        return;
    }

    bool moving = true;

    if (counting != 0)
    {
        if (fade.frames < 0)
        {
            moving = false;
        }
        else
        {
            --fade.frames;
        }
    }

    if (moving && fade.frames >= 0)
    {
        fade.red += fade.stepRed;
        fade.green += fade.stepGreen;
        fade.blue += fade.stepBlue;
    }
    else
    {
        fade.stepRed = fade.startRed;
        fade.stepGreen = fade.startGreen;
        fade.stepBlue = fade.startBlue;
    }

    const auto blue = static_cast<int>(crt::Ftol(fade.blue));
    const auto green = static_cast<int>(crt::Ftol(fade.green));
    const auto red = static_cast<int>(crt::Ftol(fade.red));
    const std::span<u16> field(this->drawSurface, static_cast<std::size_t>(Display::Width * BattleState::FieldHeight));
    this->painter.TintPixels(field, red, green, blue);
}

// swd32.exe 0x0045EC80: while no unit acts, a side that is all down or turned to stone (UnkStatus4) loses: the party
// (forced units left out; the count against the roles less the guests, in 8 bits) unless unk53BFE4, the stone roles
// counted as defeated and the battle state 0x67; then, unless unk53C048, the enemies, counted as defeated, unk53C048
// set, the auto battle off and the battle state 0x63. 1 when a side lost.
int Game::CheckStoneSides()
{
    BattleState& battle = this->battleState;

    if (battle.unk53AE70 != -1)
    {
        return 0;
    }

    u8 stoneRoles = 0;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];

        if (role.unk2B00 != 1 && role.unk2B04 != 1 && role.HasEffectBits(BattleUnit::UnkStatus4) == 1)
        {
            ++stoneRoles;
        }
    }

    const auto partyRoles = static_cast<u8>(
        static_cast<u8>(battle.roleCount) - static_cast<u8>(battle.unk53BF26) - static_cast<u8>(battle.guestCount)
    );

    if (stoneRoles != 0 && battle.defeatedRoles + stoneRoles >= partyRoles && battle.unk53BFE4 == 0)
    {
        battle.defeatedRoles = static_cast<u8>(battle.defeatedRoles + stoneRoles);
        battle.result = kBattleLost;
        return 1;
    }

    if (battle.unk53C048 != 0)
    {
        return 0;
    }

    u8 stoneEnemies = 0;

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        if (this->battleEnemies[index].HasEffectBits(BattleUnit::UnkStatus4) == 1)
        {
            ++stoneEnemies;
        }
    }

    if (stoneEnemies == 0 || battle.unk53BF00 + stoneEnemies < static_cast<i32>(battle.enemyCount) ||
        battle.unk53BFE4 != 0)
    {
        return 0;
    }

    battle.unk53BF00 = static_cast<u8>(battle.unk53BF00 + stoneEnemies);
    battle.unk53C048 = 1;
    battle.autoBattle = 0;
    battle.result = kBattleWon;
    return 1;
}

// swd32.exe 0x0045F020: the next slot record taken into `taken`: the records of held roles (IsHeldOrHolding) at the
// front are passed over; a used record taken is removed (the later ones move up, the free ones after the used ones
// cleared). The original also takes unit 7 for a role (the one before the first) and reads past the records when all
// are held; both stop the search here.
void Game::TakeSlotRecord(BattleSlotRecord& taken)
{
    BattleState& battle = this->battleState;
    std::size_t at = 0;

    while (at < battle.unk524788.size())
    {
        const i32 unit = battle.unk524788[at].unit;

        if (unit < BattleState::FirstRoleUnit ||
            this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].IsHeldOrHolding() != 1)
        {
            break;
        }

        ++at;
    }

    battle.TakeSlotRecordAt(at, taken);
}

// swd32.exe 0x0045A980: one frame of the damage numbers of role `index` (`role` 1) or enemy `index`; 1 once they are
// done (the original tests unk53BF80 first, both ways alike).
int Game::DamageNumbersFrame(int index, int role)
{
    BattleServices& services = this->battleServices;
    const auto at = static_cast<std::size_t>(index);
    BattleUnit& unit =
        role == 1 ? static_cast<BattleUnit&>(this->battleRoles[at]) : static_cast<BattleUnit&>(this->battleEnemies[at]);
    return unit.DrawDamageNumbers(services) == 1 ? 1 : 0;
}

// swd32.exe 0x0045EB40: the units in draw order show their guard text and their damage numbers (once done unk53BFD8
// is set and their unk524420 entry cleared: a role by its role index, so a role and the enemy of the same index share
// it), then take their pending damage: a unit that does has its unk502984 entry set to its index and the slot record
// of that index removed (a role's index, as in the original).
void Game::ApplyHits()
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const auto units = static_cast<std::size_t>(battle.roleCount + battle.enemyCount);

    for (std::size_t order = 0; order < units; ++order)
    {
        const i32 unitId = battle.drawOrder[order];
        const bool isRole = unitId >= BattleState::FirstRoleUnit;
        const int index = isRole ? unitId - BattleState::FirstRoleUnit : unitId;
        const auto at = static_cast<std::size_t>(index);
        BattleUnit& unit = isRole ? static_cast<BattleUnit&>(this->battleRoles[at])
                                  : static_cast<BattleUnit&>(this->battleEnemies[at]);
        unit.DrawGuardText(services);

        if (this->DamageNumbersFrame(index, isRole ? 1 : 0) == 1)
        {
            battle.unk53BFD8 = 1;
            battle.unk524420[at] = -1;
        }

        if (!unit.ApplyPendingDamage(services))
        {
            continue;
        }

        battle.unk502984[at] = index;
        battle.RemoveSlotRecord(index);
    }
}

// swd32.exe 0x00465090: the next enemy takes its turn: enemy unk53BCF4 (0 past the enemies); a defeated one passes
// on through unk4A7550 (1-based in enemyOrder, wrapping) for at most enemyCount tries, after which the battle state
// is 1. The enemy is marked (SetAC0 1) and aimed at (unk4A754C, 1-based).
void Game::NextEnemyTurn()
{
    BattleState& battle = this->battleState;
    const auto count = static_cast<i32>(battle.enemyCount);

    if (battle.unk53BCF4 >= count)
    {
        battle.unk53BCF4 = 0;
    }

    if (this->battleEnemies[static_cast<std::size_t>(battle.unk53BCF4)].IsDefeated())
    {
        int tries = 0;

        while (true)
        {
            if (++battle.unk4A7550 > count)
            {
                battle.unk4A7550 = 1;
            }

            battle.unk53BCF4 = battle.enemyOrder[static_cast<std::size_t>(battle.unk4A7550 - 1)];

            if (++tries >= count)
            {
                battle.result = kTurnsRunning;
                break;
            }

            if (!this->battleEnemies[static_cast<std::size_t>(battle.unk53BCF4)].IsDefeated())
            {
                break;
            }
        }
    }

    this->battleEnemies[static_cast<std::size_t>(battle.unk53BCF4)].SetAC0(1);
    battle.unk53BFB8 = 1;
    battle.unk4A754C = battle.unk53BCF4 + 1;
}

// swd32.exe 0x00465170: the party role whose turn follows: unk4A7550 steps (1-based, wrapping over the party roles)
// through RoleCycle until it names the current unit's role, which unk4A754C then aims at (1-based). The original
// loops for ever when the cycle never names it.
void Game::NextRoleTurn()
{
    BattleState& battle = this->battleState;
    const auto roles = static_cast<i32>(battle.roleCount - battle.unk53BF26 - battle.guestCount);
    i32 at = battle.unk4A7550;
    i32 role = 0;
    int steps = 0;

    do
    {
        if (++at > roles)
        {
            at = 1;
        }

        if (++steps > static_cast<int>(BattleState::RoleCycle.size()) ||
            at >= static_cast<i32>(BattleState::RoleCycle.size()))
        {
            Hang();
        }

        role = BattleState::RoleCycle[static_cast<std::size_t>(at)];
    } while (role != battle.currentUnit - BattleState::FirstRoleUnit);

    battle.unk4A7550 = at;
    battle.unk4A754C = role + 1;
    battle.unk53BCF4 = 0;
    battle.unk53BFB8 = 1;
}

// swd32.exe 0x00461900: the back key in a list (kBackSound): battle state 2 scrolls unk53BD04 up by kListPage, states
// 4 and 0x1B unk53BD08 (state 4 also keeps unk4A755C and the scroll for role unk4A7570); each first only marks its
// list (unk4A7558, unk4A755C) unless unk53BD9C.
void Game::ScrollListBack()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto scroll = [](i32& top)
    {
        top -= kListPage;

        if (top < 0)
        {
            top = 0;
        }
    };

    if (battle.result == kListState2)
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (battle.unk53BD9C == 0 && battle.unk4A7558 != 1)
        {
            battle.unk4A7558 = 1;
            return;
        }

        scroll(battle.unk53BD04);
        battle.unk53BFB8 = 1;
        return;
    }

    if (battle.result != kListState4)
    {
        if (battle.result != kListState1B)
        {
            return;
        }

        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (battle.unk53BD9C == 0 && battle.unk4A755C != 1)
        {
            battle.unk4A755C = 1;
            return;
        }

        scroll(battle.unk53BD08);
        battle.unk53BFB8 = 1;
        return;
    }

    this->audio.PlaySound(kBackSound, this->config.soundVolume);

    if (battle.unk53BD9C == 0 && battle.unk4A755C != 1)
    {
        battle.unk4A755C = 1;
        return;
    }

    scroll(battle.unk53BD08);
    battle.unk53BFB8 = 1;
    const auto role = static_cast<std::size_t>(battle.unk4A7570);
    battle.unk4FF578[role] = battle.unk4A755C;
    battle.unk52544C[role] = battle.unk53BD08;
}

// swd32.exe 0x00461A30: the forward key in a list (kBackSound): battle state 2 scrolls unk53BD04 down by kListPage
// within unk53BDF2 entries (nothing below a page), states 4 and 0x1B unk53BD08 within unk53BDF4; each first only
// marks the last row of its page (unk4A7558, unk4A755C, at most the entries) unless unk53BD9C. A list shorter than a
// page goes back to 0 with the mark on its end. State 4, and the first mark of state 0x1B (the original shares that
// code), keep the mark and the scroll for role unk4A7570.
void Game::ScrollListForward()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto keepForRole = [&battle](i32 mark)
    {
        const auto role = static_cast<std::size_t>(battle.unk4A7570);
        battle.unk53BFB8 = 1;
        battle.unk4FF578[role] = mark;
        battle.unk52544C[role] = battle.unk53BD08;
    };

    auto markPageEnd = [&battle, &keepForRole]
    {
        battle.unk4A755C = std::min<i32>(kListPage, battle.unk53BDF4);
        keepForRole(battle.unk4A755C);
    };

    if (battle.result == kListState2)
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (battle.unk53BDF2 < kListPage)
        {
            return;
        }

        if (battle.unk53BD9C == 0 && battle.unk4A7558 != kListPage)
        {
            battle.unk4A7558 = kListPage;
            return;
        }

        battle.unk53BD04 += kListPage;

        if (battle.unk53BD04 + kListPage > battle.unk53BDF2)
        {
            battle.unk53BD04 = battle.unk53BDF2 - kListPage;
        }

        if (battle.unk53BD04 < 0)
        {
            battle.unk53BD04 = 0;
            battle.unk4A7558 = battle.unk53BDF2;
        }

        battle.unk53BFB8 = 1;
        return;
    }

    if (battle.result != kListState4 && battle.result != kListState1B)
    {
        return;
    }

    this->audio.PlaySound(kBackSound, this->config.soundVolume);

    if (battle.unk53BD9C == 0 && battle.unk4A755C != kListPage)
    {
        markPageEnd();
        return;
    }

    battle.unk53BD08 += kListPage;

    if (battle.unk53BD08 + kListPage > battle.unk53BDF4)
    {
        battle.unk53BD08 = battle.unk53BDF4 - kListPage;
    }

    if (battle.unk53BD08 < 0)
    {
        battle.unk53BD08 = 0;
        battle.unk4A755C = battle.unk53BDF4;
    }

    if (battle.result == kListState1B)
    {
        battle.unk53BFB8 = 1;
        return;
    }

    keepForRole(battle.unk4A755C);
}

// swd32.exe 0x0045D490: the auto battle takes the current role's turn (unless in battle state 3): with no remembered
// enemy (unk4A7564) the state becomes 3; otherwise the role acts on it (unk4A754C), or guards with MarkBit1 (its
// order taken as 1); a defeated enemy gives way to the first standing one, and with none the auto battle ends.
void Game::AutoBattleTurn()
{
    BattleState& battle = this->battleState;

    if (battle.autoBattle != 1 || battle.currentUnit == 0 || battle.result == kTurnState)
    {
        return;
    }

    battle.unk53BCE8 = 1;
    battle.unk53AF38[static_cast<std::size_t>(battle.currentUnit)] = 1;

    if (battle.unk4A7564 == -1)
    {
        battle.unk53BFB8 = 1;
        battle.result = kTurnState;
        return;
    }

    battle.unk53BD50 = battle.currentUnit;
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;
    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)];
    role.SetTarget(1);
    battle.unk4A754C = battle.unk4A7564;
    battle.result = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 0;

    if (role.MarkBit1() == 1)
    {
        battle.unk53BCE8 = kGuardOrder;
        battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = kGuardOrder;
        role.ResetAfterGuard();
        role.SetTarget(1);
        battle.pendingOrders[static_cast<std::size_t>(battle.unk53BD50 - BattleState::FirstRoleUnit)].unk00 = 1;
        return;
    }

    if (!this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)].IsDefeated())
    {
        return;
    }

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        if (!this->battleEnemies[enemy].IsDefeated())
        {
            battle.unk4A754C = static_cast<i32>(enemy) + 1;
            return;
        }
    }

    role.SetTarget(0);
    battle.unk53BCE8 = 0;
    battle.unk4A754C = 1;
    battle.unk4A7564 = -1;
    battle.unk53AF38[static_cast<std::size_t>(battle.unk53BD50)] = 0;
    battle.autoBattle = 0;
}

// swd32.exe 0x004624C0: whether role unit `unit` can take the current panel: it is in unk520DD0 (among the roleCount
// first) and not held
bool Game::CanSelectRole(int unit)
{
    BattleState& battle = this->battleState;

    for (u32 at = 0; at < battle.roleCount; ++at)
    {
        if (battle.unk520DD0[at] == unit)
        {
            return this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].IsHeldOrHolding() ==
                   0;
        }
    }

    return false;
}

// The place of `unit` in PanelUnits for 0x00464DD0 and 0x00464E40 (inline in the original; past the end when absent).
std::size_t Game::PanelPlace(int unit)
{
    std::size_t place = 0;

    while (place < BattleState::PanelUnits.size() && BattleState::PanelUnits[place] != unit)
    {
        ++place;
    }

    return place;
}

// swd32.exe 0x00464DD0: the first role unit from `unit` on in PanelUnits (wrapping, four tries) that can take the
// panel; 0 when none can
int Game::NextSelectableRole(int unit)
{
    std::size_t place = Game::PanelPlace(unit);

    // Never so: the callers pass panel units (the original reads past the table).
    if (place == BattleState::PanelUnits.size())
    {
        return 0;
    }

    int candidate = BattleState::PanelUnits[place];

    if (this->CanSelectRole(candidate))
    {
        return candidate;
    }

    ++place;

    for (int tries = 1; tries < static_cast<int>(BattleState::PanelUnits.size()); ++tries)
    {
        if (place == BattleState::PanelUnits.size())
        {
            place = 0;
        }

        candidate = BattleState::PanelUnits[place];
        ++place;

        if (this->CanSelectRole(candidate))
        {
            return candidate;
        }
    }

    return 0;
}

// swd32.exe 0x00464E40: the first role unit from `unit` back in PanelUnits (wrapping, four tries) that can take the
// panel; 0 when none can
int Game::PreviousSelectableRole(int unit)
{
    auto place = static_cast<int>(Game::PanelPlace(unit));

    // Never so: the callers pass panel units (the original reads past the table).
    if (place == static_cast<int>(BattleState::PanelUnits.size()))
    {
        return 0;
    }

    for (int tries = 0; tries < static_cast<int>(BattleState::PanelUnits.size()); ++tries)
    {
        const int candidate = BattleState::PanelUnits[static_cast<std::size_t>(place)];

        if (--place < 0)
        {
            place = static_cast<int>(BattleState::PanelUnits.size()) - 1;
        }

        if (this->CanSelectRole(candidate))
        {
            return candidate;
        }
    }

    return 0;
}

// swd32.exe 0x00462420: role unit `unit` takes the current panel (in battle state 0, or with unk4A7644 set): when it
// is among the first roleCount - 1 of unk520DD0 and not held, it trades places with the current unit; the inputs of
// the panel (commandNames, unk53BF22) are cleared either way.
void Game::SelectRoleUnit(int unit)
{
    BattleState& battle = this->battleState;

    if (battle.result != 0 && battle.unk4A7644 == kNoPanelRole)
    {
        return;
    }

    if (battle.roleCount != 1)
    {
        for (u32 at = 0; at < battle.roleCount - 1; ++at)
        {
            const i32 listed = battle.unk520DD0[at];

            if (listed != 0 && listed == unit &&
                this->battleRoles[static_cast<std::size_t>(listed - BattleState::FirstRoleUnit)].IsHeldOrHolding() != 1)
            {
                battle.unk520DD0[at] = battle.currentUnit;
                battle.currentUnit = listed;
                break;
            }
        }
    }

    battle.commandNames = {};
    battle.unk53BF22 = 0;
}

// swd32.exe 0x00462320: the panel goes from the current role unit (8..11) to the one numbered before it (11 before 8)
// or, past it, the next in PanelUnits that can take the panel
void Game::SelectPreviousRole()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;
    const i32 unit = battle.currentUnit;

    if (unit < kFirstPanelUnit || unit > kLastPanelUnit)
    {
        return;
    }

    this->SelectRoleUnit(this->NextSelectableRole(unit == kFirstPanelUnit ? kLastPanelUnit : unit - 1));
}

// swd32.exe 0x004623A0: the panel goes from the current role unit (8..11) to the one numbered after it (8 after 11)
// or, before it, the previous in PanelUnits that can take the panel
void Game::SelectNextRole()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;
    const i32 unit = battle.currentUnit;

    if (unit < kFirstPanelUnit || unit > kLastPanelUnit)
    {
        return;
    }

    this->SelectRoleUnit(this->PreviousSelectableRole(unit == kLastPanelUnit ? kFirstPanelUnit : unit + 1));
}

// swd32.exe 0x00462510: the right key in the command menu (kBackSound): state 1 moves the command (unk4A7548,
// 1-based) a column on within unk53BF22 + 5 entries, back when that one is not enabled; state 2 turns the page of the
// list (unk53BD18, 0..2) to its top; state 4 goes to the next party role's list (unk4A7570, its kept mark and
// scroll); state 0x1E raises unk4A755C by 5 up to 10.
void Game::MenuRight()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    if (battle.result == kMenuState)
    {
        i32 command = battle.unk4A7548;

        if (command + kMenuColumn <= battle.unk53BF22 + kMenuFixedCommands)
        {
            command += kMenuColumn;
            battle.unk4A7548 = command;
        }

        if (battle.commandEnabled[static_cast<std::size_t>(command - 1)] == 0)
        {
            battle.unk4A7548 = command - kMenuColumn;
        }

        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState2)
    {
        if (++battle.unk53BD18 >= kListPages)
        {
            battle.unk53BD18 = 0;
        }

        battle.unk4A7558 = 1;
        battle.unk53BD04 = 0;
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState4)
    {
        if (++battle.unk4A7570 >= static_cast<i32>(Party::Roles))
        {
            battle.unk4A7570 = 0;
        }

        const auto role = static_cast<std::size_t>(battle.unk4A7570);
        battle.unk4A755C = battle.unk4FF578[role];
        battle.unk53BD08 = battle.unk52544C[role];
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kWaitState1E)
    {
        battle.unk4A755C = std::min(battle.unk4A755C + kMenuStep, kMenuStepMost);
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }
}

// swd32.exe 0x00462630: the left key in the command menu: state 1 moves the command a column back down to 1 (on again
// when that one is not enabled); state 2 turns the page back (wrapping to 2, the scroll kept); state 4 goes to the
// previous party role's list; state 0x1E lowers unk4A755C by 5 down to 1.
void Game::MenuLeft()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    if (battle.result == kMenuState)
    {
        i32 command = battle.unk4A7548;

        if (command - kMenuColumn >= 1)
        {
            command -= kMenuColumn;
            battle.unk4A7548 = command;
        }

        if (battle.commandEnabled[static_cast<std::size_t>(command - 1)] == 0)
        {
            battle.unk4A7548 = command + kMenuColumn;
        }

        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState2)
    {
        if (--battle.unk53BD18 < 0)
        {
            battle.unk53BD18 = kListPages - 1;
        }

        battle.unk4A7558 = 1;
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState4)
    {
        if (--battle.unk4A7570 < 0)
        {
            battle.unk4A7570 = static_cast<i32>(Party::Roles) - 1;
        }

        const auto role = static_cast<std::size_t>(battle.unk4A7570);
        battle.unk4A755C = battle.unk4FF578[role];
        battle.unk53BD08 = battle.unk52544C[role];
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kWaitState1E)
    {
        battle.unk4A755C = std::max(battle.unk4A755C - kMenuStep, 1);
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }
}

// swd32.exe 0x00464E90: the command menu of the current role: the four commands and guard enabled; then the accessory
// commands (kinds kFirstAccessoryCommand..kLastAccessoryCommand of the two accessory slots, named by
// AccessoryCommandNames) after them, for battle role 0 in the party first the spirit pact (kSpiritPact, with
// UnkPanel2) of the first party role's slots, otherwise of its own party role's. A role without MarkBit8Clear loses
// command 2, one without HasBlock38 command 4; a MarkBit1 role keeps command 2 and guard only (the cursor on 2 unless
// its command is enabled).
void Game::BuildCommandMenu()
{
    BattleState& battle = this->battleState;
    battle.commandEnabled = {1, 1, 1, 1, 1, 0, 0, 0};
    battle.commandNames = {};
    battle.unk53BF22 = 0;
    const int index = battle.currentUnit - BattleState::FirstRoleUnit;
    std::size_t listed = 0;

    auto add = [&battle, &listed](u16 kind, std::u8string_view name)
    {
        battle.commandEnabled[kFixedCommands + listed] = 1;
        battle.commandKinds[listed] = kind;
        battle.commandNames[listed] = name;
        ++listed;
        battle.unk53BF22 = static_cast<u16>(listed);
    };

    auto addAccessories = [this, &add](std::size_t role)
    {
        for (std::size_t slot = 0; slot < kAccessorySlots; ++slot)
        {
            const std::unique_ptr<ItemNode>& node =
                this->party.equipment[role * Party::SlotsPerRole + kFirstAccessorySlot + slot];

            // An empty slot holds the empty entry; a missing one is read through a null pointer in the original.
            const u16 kind = node != nullptr ? node->record.data.unk48 : u16{0};

            if (kind >= kFirstAccessoryCommand && kind <= kLastAccessoryCommand)
            {
                add(kind, AccessoryCommandName(kind));
            }
        }
    };

    if (battle.currentUnit == BattleState::FirstRoleUnit && battle.inParty[0] == 1)
    {
        if ((battle.panelFlags & BattleState::UnkPanel2) != 0)
        {
            add(kSpiritPact, BattleState::SpiritPactName);
        }

        addAccessories(0);
    }
    else
    {
        addAccessories(static_cast<std::size_t>(battle.partyRoles[static_cast<std::size_t>(index)]));
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(index)];

    if (!role.MarkBit8Clear())
    {
        battle.commandEnabled[1] = 0;
    }

    if (role.HasBlock38() == 0)
    {
        battle.commandEnabled[3] = 0;
    }

    if (role.MarkBit1() == 1)
    {
        battle.commandEnabled = {0, 1, 0, 0, 1, 0, 0, 0};

        if (battle.commandEnabled[static_cast<std::size_t>(battle.unk4A7548 - 1)] == 0)
        {
            battle.unk4A7548 = kGuardedCommand;
        }
    }
}

// swd32.exe 0x004651D0: the command menu of the current role at (x, y): the five commands (FixedCommandNames) in
// columns of four, light when enabled and dark otherwise, then the accessory commands in the second column below the
// first (dark, and disabled, when CanUseAccessory refuses); the entry `cursor` (1-based) is drawn again over a dark
// box. The original first enables command 4 for a role whose fury is full (BuildCommandMenu then sets it anew).
void Game::DrawCommandMenu(int x, int y, int cursor)
{
    BattleState& battle = this->battleState;
    TextFont& font = this->largeFont;

    if (battle.currentUnit == 0)
    {
        return;
    }

    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];

    if (role.roleBlock[0] == kFuryFullBlock && role.MarkBit1() == 0)
    {
        battle.commandEnabled[3] = 1;
    }

    this->BuildCommandMenu();

    auto highlight = [this, &font, &battle](int left, int top, std::u8string_view name)
    {
        font.SetBackground(kHighlightBackground);
        font.DrawText(this->drawSurface, left, top, name, battle.lightColor, TextFont::BoldOutline);
        font.SetBackground(TextFont::NoBackground);
    };

    for (std::size_t command = 0; command < BattleState::FixedCommandNames.size(); ++command)
    {
        const std::u8string_view name = BattleState::FixedCommandNames[command];
        const u16 color = battle.commandEnabled[command] == 1 ? battle.lightColor : battle.darkColor;
        const auto row = static_cast<int>(command % static_cast<std::size_t>(kMenuColumn));
        const auto column = static_cast<int>(command / static_cast<std::size_t>(kMenuColumn));
        const int top = y + kMenuRowHeight * row;
        font.DrawText(this->drawSurface, x + kMenuColumnWidth * column, top, name, color, TextFont::BoldShadow);

        if (static_cast<int>(command) + 1 == cursor)
        {
            highlight(kMenuColumnWidth * column + x - 1, top - 1, name);
        }
    }

    for (std::size_t extra = 0; extra < battle.commandNames.size(); ++extra)
    {
        const std::u8string_view name = battle.commandNames[extra];
        const int top = y + kMenuRowHeight * (static_cast<int>(extra) + 1);

        if (name.empty())
        {
            continue;
        }

        u8& enabled = battle.commandEnabled[kFixedCommands + extra];

        if (role.CanUseAccessory(battle.commandKinds[extra]) == 1 && enabled == 1)
        {
            font.DrawText(this->drawSurface, x + kMenuColumnWidth, top, name, battle.lightColor, TextFont::BoldShadow);
        }
        else
        {
            enabled = 0;
            font.DrawText(this->drawSurface, x + kMenuColumnWidth, top, name, battle.darkColor, TextFont::BoldShadow);
        }

        if (static_cast<int>(kFixedCommands + extra) + 1 == cursor)
        {
            highlight(x + kMenuColumnWidth - 1, top - 1, name);
        }
    }

    font.SetBackground(TextFont::NoBackground);
}

// swd32.exe 0x00468FF0: the first party role (not forced, standing) whose accessory proficiency has just filled
// (FinishProficiency): its object read into unk53BC28 (the description freed), its name kept in lastLearned for the
// message, unk53BFFC set and unk4A7646 the role
void Game::AnnounceMastery()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];

        if (role.unk2B00 == 1 || role.unk2B04 == 1 || role.IsDefeated())
        {
            continue;
        }

        const u16 finished = role.FinishProficiency(this->party.fame.lists[Fame::ProficiencyList]);

        if (finished == 0)
        {
            continue;
        }

        this->database.LoadObject(battle.unk53BC28, finished);
        Database::FreeDescription(battle.unk53BC28);
        battle.unk53BFFC = 1;
        battle.lastLearned.name = ObjectName(battle.unk53BC28);
        battle.unk4A7646 = static_cast<u8>(index);
        return;
    }
}

// swd32.exe 0x00461C10: the cancel key of the battle menu. In the auto battle it ends it (the aimed enemy unmarked,
// the menu closed). Otherwise by battle state: 1 closes the menu; 2 (a list) and 0x1B, 0x1E, 8 go back to the menu
// (its name buffer cleared, the role's command cancelled); 3 (choosing a target) goes back to the list the menu
// command opened (state 2, 4, 0x1B, 0x1E or 8 for commands 2, 3, 0x1B, 0x1E, 4; to the menu with command unk53BCF0
// for others), every unit unmarked and the portraits plain, the role's order cleared; 4 goes back to the menu; 5 to
// state 4; 7 closes the menu with the command back on 1.
void Game::MenuCancel()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 1;

    auto role = [this, &battle]() -> BattleRole&
    {
        return this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    };

    auto closePanels = [&battle]
    {
        battle.unk53BD90 = 0;
        battle.unk53BD94 = 0;
    };

    auto clearList = [&battle]
    {
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.unk53BFC8 = 0;
        battle.unk53BFB0 = 0;
        battle.unk53C184 = {};
        battle.unk53C49C = 1;
        battle.unk53BD98 = kListRows;
    };

    if (battle.autoBattle == 1)
    {
        battle.result = 0;
        battle.unk53BFB8 = 0;
        this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)].SetAC0(0);
        battle.unk53BFC4 = 0;
        battle.commandNames[0] = {};
        battle.unk53BFC0 = 0;
        battle.commandNames[1] = {};
        battle.autoBattle = 0;
        battle.unk53BF22 = 0;
        battle.unk4A7564 = -1;
        battle.commandNames[2] = {};
        return;
    }

    switch (battle.result)
    {
        case kMenuState:
            battle.result = 0;
            battle.commandNames[0] = {};
            battle.unk53BFB0 = 0;
            battle.commandNames[1] = {};
            battle.unk53BF22 = 0;
            battle.unk53BFB8 = 0;
            closePanels();
            battle.unk53BD98 = kListRows;
            battle.commandNames[2] = {};
            return;

        case kListState2:
            battle.result = kMenuState;
            battle.unk4A7558 = 1;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        case kTargetState:
        {
            if (battle.unk4A762E == kNoTalkRole)
            {
                battle.result = kMenuState;
                battle.unk53BD98 = kListRows;
                battle.unk53BFB0 = 0;
            }
            else
            {
                battle.unk53BD44 = 0;
                battle.unk53BD48 = 0;
                battle.result = kTalkState;
            }

            battle.unk4A7640 = 1;
            battle.unk53C02C = 0;
            role().CancelCommand();

            for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
            {
                this->battleEnemies[enemy].SetAC0(0);
            }

            for (std::size_t panel = 0; panel < static_cast<std::size_t>(Party::Roles); ++panel)
            {
                this->battleRoles[panel].SetAC0(0);
                this->battleHud.portraitHighlight[panel] = 0;
            }

            i32 list = 0;

            switch (battle.unk4A7548)
            {
                case kSpellCommand:
                    battle.unk53BF94 = 0;
                    list = kListState2;
                    break;

                case kItemCommand:
                    list = kListState4;
                    break;

                case kListState1B:
                    list = kListState1B;
                    break;

                case kWaitState1E:
                    list = kWaitState1E;
                    break;

                case kSkillCommand:
                    list = kSkillListState;
                    break;

                default:
                    closePanels();
                    battle.unk4A7548 = battle.unk53BCF0;
                    return;
            }

            battle.result = list;
            battle.unk53BF88 = 0;
            battle.unk53BFF0 = 0;
            battle.unk53BFD0 = 0;
            const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
            battle.unk4FE5D4[unit] = 0;
            closePanels();
            battle.pendingOrders[unit].unk08 = 0;
            battle.pendingOrders[unit].unk00 = 0;
            return;
        }

        case kListState4:
            battle.result = kMenuState;
            battle.unk53BD08 = 0;
            battle.unk53BD98 = kListRows;
            battle.unk53BFC4 = 0;
            battle.unk53BFC0 = 0;
            battle.unk53BFC8 = 0;
            battle.unk53BFB0 = 0;
            role().CancelCommand();
            closePanels();
            return;

        case kWaitState5:
            battle.result = kListState4;
            role().CancelCommand();
            closePanels();
            return;

        case kTalkState:
            battle.result = 0;
            battle.unk53BFC0 = 0;
            battle.unk53BFC4 = 0;
            closePanels();
            battle.unk4A7568 = 2;
            battle.unk4A756C = 1;
            battle.unk4A7548 = 1;
            return;

        case kSkillListState:
            battle.result = kMenuState;
            battle.unk4A755C = 1;
            battle.unk4A7560 = 1;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        case kListState1B:
            battle.result = kMenuState;
            battle.unk4A755C = 1;
            battle.unk4A7548 = battle.unk53BCF0;
            battle.unk53BD08 = 0;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        case kWaitState1E:
            battle.result = kMenuState;
            battle.unk4A7548 = battle.unk53BCF0;
            battle.unk4A755C = 1;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        default:
            closePanels();
            return;
    }
}

// swd32.exe 0x00465480: the list of battle state 2 at (x, y): its three page tabs (kTabEffect, frames 2..0 right to
// left) and the open page's tab over them (frame 4 + page), the panel act restarted (unk53BD90 10), a quarter-dark
// box with the panel frame below the tabs; unk53BD94 grows by 2 a call up to 7.
void Game::DrawListTabs(int x, int y)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    const int tabsY = y + kTabsDrop;
    int tabX = x + kTabWidth * (kListPages - 1);

    for (int tab = kListPages - 1; tab >= 0; --tab)
    {
        this->battleHud.DrawEffectAt(kTabEffect, tab, tabX, tabsY, 0);
        tabX -= kTabWidth;
    }

    this->battleHud
        .DrawEffectAt(kTabEffect, battle.unk53BD18 + kOpenTabFrame, x + kTabWidth * battle.unk53BD18, tabsY, 0);
    this->mediumFont.SetBackground(kHighlightBackground);
    this->mediumFont.SetBackground(TextFont::NoBackground);
    messages.panelAct = ActState{};
    battle.unk53BD90 = kListPanelFrames;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(
        this->painter,
        x,
        y + kListBoxDrop,
        kListBoxWidth,
        kListBoxHeight,
        0,
        kListShade,
        kListShade,
        FillMode::Quarter
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kListFrameRight,
        y + kListFrameBottom,
        0,
        kListFrameBorder
    );
    battle.unk53BD94 = std::min(battle.unk53BD94 + kListGrowStep, kListGrowMost);
}

// swd32.exe 0x00465E50: the 投擲 list of the current role at (x, y): a darkened box with a title frame (投擲) and a
// body frame, then up to 7 listed objects from entry `top` + 1 with their counts ("%2d"); unk53BDF4 counts them. Row
// `cursor` (from 1) is drawn again a pixel up and left under a gradient bar, and its entry becomes unk53BD14.
void Game::DrawThrowList(int x, int y, int cursor, int top)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    messages.panelAct = ActState{};
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(this->painter, x, y, kListBoxWidth, kThrowBoxHeight, 0, kThrowShade, kThrowShade, FillMode::Tint);
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kThrowTitleTop,
        x + kListFrameRight,
        y + kThrowTitleBottom,
        0,
        kListFrameBorder
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kListFrameRight,
        y + kListFrameBottom,
        0,
        kListFrameBorder
    );
    font.DrawText(
        this->drawSurface,
        x + kThrowTitleX,
        y + kThrowTitleTop,
        kThrowTitle,
        kTitleColor,
        TextFont::BoldOutline
    );
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.unk53BDF4 = 0;
    role.CountObjects(kThrowCategory, battle.unk53BDF4);
    role.ShowAllItems();
    int entry = top;
    int row = 0;

    while (true)
    {
        const int next = entry + 1;
        std::string name;
        u16 count = 0;

        if (role.ListedObject(next, name, count) == 0)
        {
            break;
        }

        if (row >= kListRowsShown)
        {
            font.SetBackground(TextFont::NoBackground);
            return;
        }

        const int rowY = y + kListRowHeight * row;
        font.DrawText(
            this->drawSurface,
            x + kListNameX,
            rowY + kListRowTop,
            name,
            battle.lightColor,
            TextFont::BoldOutline
        );
        const std::string number = WinFormat(kCountFormat, {static_cast<i32>(static_cast<i16>(count))});
        font.DrawText(
            this->drawSurface,
            x + kListCountX,
            rowY + kListRowTop,
            number,
            battle.lightColor,
            TextFont::BoldOutline
        );

        if (cursor == row + 1)
        {
            font.DrawText(
                this->drawSurface,
                x + kListNameX - 1,
                rowY + kListRowTop - 1,
                name,
                battle.lightColor,
                TextFont::BoldOutline
            );
            font.SetBackground(TextFont::NoBackground);
            FillBox(
                this->painter,
                x,
                rowY + kListBarTop,
                kListBarWidth,
                kListBarHeight,
                kListBarRed,
                kListBarGreen,
                0,
                FillMode::Gradient
            );
            battle.unk53BD14 = next;
            battle.unk53BFB8 = 1;
        }

        font.SetBackground(TextFont::NoBackground);
        entry = next;
        ++row;
    }

    role.ShowAllItems();
    font.SetBackground(TextFont::NoBackground);
}

// swd32.exe 0x00466190: the 煉符 grid of the current role at (x, y): two columns of five cells. The first cells, one
// per talisman paper (kThrownObject, their count kept in unk53BDF4), show its name; then each throwable object once
// per piece; the rest 無. Cell `cursor` (from 1) gets a gradient bar and sets unk53BD14 to the object under it (less 1
// without paper). unk53BDF4 then adds the throwable pieces.
void Game::DrawRefineList(int x, int y, int cursor)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    messages.panelAct = ActState{};
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(this->painter, x, y, kRefineBoxWidth, kRefineBoxHeight, 0, kThrowShade, kThrowShade, FillMode::Tint);
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kThrowTitleTop,
        x + kRefineFrameRight,
        y + kThrowTitleBottom,
        0,
        kListFrameBorder
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kRefineFrameRight,
        y + kRefineFrameBottom,
        0,
        kListFrameBorder
    );
    const std::string title = Big5(BattleState::AccessoryCommandNames[kRefineCommand - kFirstAccessoryCommand]);
    font.DrawText(this->drawSurface, x + kRefineTitleX, y + kThrowTitleTop, title, kTitleColor, TextFont::BoldOutline);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    std::string name;
    u16 count = 0;
    int object = 1;
    int piece = 0;
    role.ThrowableObject(0, 1, name, count);
    battle.unk53BDF4 = count;
    role.ShowAllItems();
    u16 pieces = 0;
    role.CountUsableObjects(pieces);
    role.ShowAllItems();
    count = 0;
    battle.unk53BD14 = 1;
    int cell = 1;
    int columnX = x + kListNameX;

    for (int column = 0; column < kRefineColumns; ++column)
    {
        int rowY = y + kListRowTop;

        for (int row = 0; row < kRefineRows; ++row)
        {
            if (cell > battle.unk53BDF4)
            {
                if (cell <= battle.unk53BDF4 + pieces)
                {
                    role.ThrowableObject(1, object, name, count);

                    if (++piece == static_cast<i16>(count))
                    {
                        ++object;
                        piece = 0;
                    }
                }
                else
                {
                    name = Big5(kNoObject);
                }
            }

            if (cursor == cell)
            {
                FillBox(
                    this->painter,
                    columnX - kRefineBarLeft,
                    rowY - kRefineBarUp,
                    kRefineBarWidth,
                    kListBarHeight,
                    kListBarRed,
                    kListBarGreen,
                    0,
                    FillMode::Gradient
                );
                font.DrawText(this->drawSurface, columnX, rowY, name, battle.lightColor, TextFont::BoldOutline);
                font.SetBackground(TextFont::NoBackground);
                battle.unk53BFB8 = 1;
                battle.unk53BD14 = piece == 1 ? object + 1 : object;

                if (battle.unk53BDF4 == 0)
                {
                    --battle.unk53BD14;
                }
            }

            font.DrawText(this->drawSurface, columnX, rowY, name, battle.lightColor, TextFont::BoldOutline);
            font.SetBackground(TextFont::NoBackground);
            ++cell;
            rowY += kListRowHeight;
        }

        columnX += kRefineColumnWidth;
    }

    font.SetBackground(TextFont::NoBackground);
    battle.unk53BDF4 = static_cast<u16>(battle.unk53BDF4 + pieces);
}

// swd32.exe 0x00466500: the 絕招 list of the current role at (x, y): the panel act run on (not restarted), a darkened
// box with a title and a body frame, then up to 7 skills of type 1 (their names kept in unk53C184, cut to its bytes:
// the original copies the whole name over what follows); unk53BDF3 counts them. Row `cursor` (from 1) is drawn again
// a pixel up and left under a gradient bar, and its skill index becomes unk53BD10.
void Game::DrawSpecialList(int x, int y, int cursor)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(this->painter, x, y, kListBoxWidth, kSpecialBoxHeight, 0, kThrowShade, kThrowShade, FillMode::Tint);
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kThrowTitleTop,
        x + kListFrameRight,
        y + kThrowTitleBottom,
        0,
        kListFrameBorder
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kListFrameRight,
        y + kSpecialFrameBottom,
        0,
        kListFrameBorder
    );
    const std::string title = Big5(BattleState::FixedCommandNames[kSpecialCommandName]);
    font.DrawText(this->drawSurface, x + kThrowTitleX, y + kThrowTitleTop, title, kTitleColor, TextFont::BoldOutline);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.unk53BDF3 = 0;
    role.CountSkills(0, 1, battle.unk53BDF3);
    role.ShowAllSkills();
    int row = 0;

    for (int index = 1;; ++index)
    {
        std::string name;
        u16 cost = 0;
        role.ListSkill(this->database, 0, 1, index, name, cost);

        if (cost == kSkillListEnd)
        {
            break;
        }

        if (cost != 0)
        {
            battle.unk53C184 = {};
            std::copy_n(name.begin(), std::min(name.size(), battle.unk53C184.size() - 1), battle.unk53C184.begin());
            const std::string_view shown(battle.unk53C184.data());
            const int rowY = y + kSpecialRowHeight * row;
            font.DrawText(
                this->drawSurface,
                x + kListNameX,
                rowY + kListFrameTop,
                shown,
                battle.lightColor,
                TextFont::BoldOutline
            );

            if (cursor == row + 1)
            {
                font.DrawText(
                    this->drawSurface,
                    x + kListNameX - 1,
                    rowY + kListFrameTop - 1,
                    shown,
                    battle.lightColor,
                    TextFont::BoldOutline
                );
                FillBox(
                    this->painter,
                    x,
                    rowY + kSpecialBarTop,
                    kSpecialBarWidth,
                    kListBarHeight,
                    kListBarRed,
                    kListBarGreen,
                    0,
                    FillMode::Gradient
                );
                font.SetBackground(TextFont::NoBackground);
                battle.unk53BFB8 = 1;
                battle.unk53BD10 = index;
            }

            ++row;
        }

        if (row >= kListRowsShown)
        {
            font.SetBackground(TextFont::NoBackground);
            return;
        }
    }

    role.ShowAllSkills();
    font.SetBackground(TextFont::NoBackground);
}

// swd32.exe 0x004667B0: the 佈置護駕 panel: the panel act run on, a shade with a title frame (佈置護駕) and a body
// frame, a gradient bar on row unk53BCF8 at (x, y), and the names of the escorts (the last unk53BF26 battle roles) in
// white at an 18-pixel advance, with 無 below a lone escort.
void Game::DrawEscortList(int x, int y)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(
        this->painter,
        kEscortShadeX,
        kEscortShadeY,
        kEscortShadeWidth,
        kEscortShadeHeight,
        0,
        kListShade,
        kListShade,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortTitleTop,
        kEscortFrameRight,
        kEscortTitleBottom,
        0,
        kListFrameBorder
    );
    font.DrawText(
        this->drawSurface,
        kEscortTitleX,
        kEscortTitleTop,
        Big5(kEscortTitle),
        kTitleColor,
        TextFont::BoldOutline
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortBodyTop,
        kEscortFrameRight,
        kEscortBodyBottom,
        0,
        kListFrameBorder
    );
    FillBox(
        this->painter,
        x - kEscortBarLeft,
        y + kSpecialRowHeight * battle.unk53BCF8 - kEscortBarUp,
        kEscortShadeWidth,
        kListBarHeight,
        kListBarRed,
        kListBarGreen,
        0,
        FillMode::Gradient
    );

    for (int left = battle.unk53BF26; left > 0; --left)
    {
        font.SetAdvance(kEscortAdvance);
        const BattleRole& escort = this->battleRoles[battle.roleCount - static_cast<u32>(left)];
        const int rowY = y + kSpecialRowHeight * (battle.unk53BF26 - left) + kEscortRowTop;
        font.DrawText(
            this->drawSurface,
            x + kEscortNameX,
            rowY,
            ObjectName(*escort.Data()),
            kWhite,
            TextFont::BoldOutline
        );
    }

    if (battle.unk53BF26 == 1)
    {
        font.DrawText(
            this->drawSurface,
            x + kEscortNameX,
            y + kEscortNoneTop,
            Big5(kNoObject),
            kWhite,
            TextFont::BoldOutline
        );
    }

    font.SetAdvance(kListAdvance);
}

// swd32.exe 0x00466950: the name tag of the aimed enemy (unk4A754C, 1-based) at (x, y), unless the current role's
// order is taken or unk53BF88: a darkened box and a frame as wide as the name (20 pixels per two bytes; from the right
// edge when the sides are swapped), the name in white. Once the bestiary counts 10 of it its hit points follow
// ("生命:%d/%d", to the left when swapped); from 15 its action gauge as a dark red bar below.
void Game::DrawTargetTag(int x, int y)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;
    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);

    if (battle.pendingOrders[unit].unk00 == 1 || battle.unk53BF88 == 1 || battle.unk4A754C <= 0 ||
        battle.unk4A754C > static_cast<i32>(battle.enemyCount))
    {
        return;
    }

    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(battle.unk4A754C - 1)];
    const std::string name = ObjectName(*enemy.Data());
    const int halves = static_cast<int>(name.size()) / 2;
    const int left = battle.mirrored == 1 ? kTagMirrorRight - kTagCharWidth * halves - x : x;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    const int width = kTagCharWidth * halves;
    FillBox(
        this->painter,
        left - kTagMargin,
        y - kTagMargin,
        width + 2 * kTagMargin,
        kListBarHeight,
        0,
        kThrowShade,
        kThrowShade,
        FillMode::Tint
    );
    const int right = left + width;
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        left,
        y,
        right,
        y + kTagFrameHeight,
        0,
        kListFrameBorder
    );
    font.SetAdvance(kTagCharWidth);
    font.DrawText(this->drawSurface, left + kTagNameX, y, name, kWhite, TextFont::BoldOutline);
    font.SetAdvance(kListAdvance);
    const u16 known =
        static_cast<u16>(FindCounterValue(this->party.fame.lists[Fame::FameList], static_cast<i16>(enemy.ActId())));

    if (known >= kKnownHitPoints)
    {
        i32 hitPoints = 0;
        i32 maximum = 0;
        enemy.GetHitPoints(hitPoints, maximum);
        const std::string text = WinFormat(kHitPointsFormat, {hitPoints, maximum});
        const int textX = battle.mirrored != 0 ? left - kTagHitPointsLeft : right + kTagHitPointsGap;
        font.DrawText(this->drawSurface, textX, y - kTagHitPointsUp, text, kWhite, TextFont::BoldOutline);
    }

    if (known < kKnownGauge)
    {
        return;
    }

    const auto gauge = static_cast<u16>(enemy.GaugeLength(this->battleServices));

    if (gauge == 0)
    {
        return;
    }

    const int barX = battle.mirrored != 0 ? left - kTagHitPointsLeft : right + kTagHitPointsGap;
    this->battleHud
        .FillColor(barX, y + kTagGaugeTop, gauge, kTagGaugeHeight, this->painter.MakeColor(kTagGaugeRed, 0, 0));
}

// swd32.exe 0x00466C00: the 控制 menu of the controlled enemy (unk4A762C) at (x, y): a tile box with the title, a tile
// box of unk4A7568 rows, then 攻擊, its objects by name, its skills as 特殊1 and 特殊2, and 釋放, 20 pixels a row.
// Entry `cursor` (from 1) is drawn again over a dark background; an object sets unk53BD44 to its id (always the
// first object's, see ItemName) and a skill unk53BD48 to its number.
void Game::DrawControlMenu(int x, int y, int cursor)
{
    BattleState& battle = this->battleState;
    TextFont& font = this->mediumFont;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(static_cast<i16>(battle.unk4A762C))];

    auto entry = [this, &font, x](int top, const std::string& text, bool chosen)
    {
        font.SetBackground(TextFont::NoBackground);
        font.DrawText(this->drawSurface, x, top, text, kWhite, TextFont::BoldOutline);

        if (chosen)
        {
            font.SetBackground(kHighlightBackground);
            font.DrawText(this->drawSurface, x, top, text, kWhite, TextFont::BoldOutline);
        }
    };

    this->battleHud.DrawTileBox(
        kControlTiles,
        x - kControlBoxLeft,
        y - kControlTitleUp,
        kControlColumns,
        kControlTitleRows,
        kControlFill
    );
    font.DrawText(
        this->drawSurface,
        x,
        y - kControlTitleTextUp,
        Big5(BattleState::AccessoryCommandNames[kControlCommand - kFirstAccessoryCommand]),
        kWhite,
        TextFont::BoldOutline
    );
    this->battleHud.DrawTileBox(
        kControlTiles,
        x - kControlBoxLeft,
        y - kControlBoxLeft,
        kControlColumns,
        battle.unk4A7568,
        kControlFill
    );
    font.SetShadow(0);
    entry(y, Big5(BattleState::FixedCommandNames[0]), cursor == 1);
    int shown = 0;
    int number = 1;
    int top = y;

    for (int choice = 0; choice < kControlObjects; ++choice)
    {
        std::string name;
        u32 id = 0;

        if (enemy.ItemName(this->database, choice, name, id) != 1)
        {
            continue;
        }

        ++shown;
        ++number;
        top += kListRowHeight;
        entry(top, name, cursor == number);

        if (cursor == number)
        {
            battle.unk53BD48 = 0;
            battle.unk53BD44 = static_cast<i32>(id);
        }
    }

    number = shown + 1;
    top = y + kListRowHeight * shown;

    for (int choice = 0; choice < kControlSkills; ++choice)
    {
        std::string name;

        if (enemy.SkillName(this->database, choice, name) != 1)
        {
            continue;
        }

        ++shown;
        ++number;
        top += kListRowHeight;
        entry(top, WinFormat(kSpecialFormat, {choice + 1}), cursor == number);

        if (cursor == number)
        {
            battle.unk53BD44 = 0;
            battle.unk53BD48 = choice + 1;
        }
    }

    const int last = shown + 1;
    entry(y + kListRowHeight * last, Big5(kReleaseName), cursor == last + 1);
    font.SetBackground(TextFont::NoBackground);
}

// swd32.exe 0x004655B0: the spells of the current role's open page (unk53BD18) from entry `top` + 1 at (x, y), up to
// 7 rows (unk53BDF2 counts them): each with its element icon, its name (kept in unk53C184) and its cost ("%3d"; with
// bit 15 a stamina cost), dark when the role has less than it. Row `cursor` (from 1) is drawn again a pixel up and
// left under a gradient bar, with the spell's description centred in a bar at the bottom; its index becomes
// unk53BD10.
void Game::DrawSpellList(int x, int y, int cursor, int top)
{
    BattleState& battle = this->battleState;
    TextFont& font = this->mediumFont;

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.unk53BDF2 = 0;
    role.CountSkills(battle.unk53BD18, 0, battle.unk53BDF2);
    role.ShowAllSkills();
    int row = 0;

    for (int index = top + 1;; ++index)
    {
        std::string name;
        u16 cost = 0;
        const u16 icon = role.ListSkill(this->database, battle.unk53BD18, 0, index, name, cost);

        if (cost == kSkillListEnd)
        {
            break;
        }

        battle.unk53C184 = {};
        std::copy_n(name.begin(), std::min(name.size(), battle.unk53C184.size() - 1), battle.unk53C184.begin());
        const std::string_view shown(battle.unk53C184.data());
        u16 maximum = 0;
        u16 have = 0;

        if ((cost & kStaminaCostBit) != 0)
        {
            role.GetStamina(maximum, have);
            cost = static_cast<u16>(cost & ~kStaminaCostBit);
        }
        else
        {
            role.GetSpirit(maximum, have);
        }

        const int rowY = y + kListRowHeight * row + kSpellRowTop;

        if (icon != kSkillListEnd)
        {
            this->battleHud.DrawPictureFrame(kElementIcons, icon - 1, x - kSpellIconLeft, rowY);
        }

        const bool isShort = static_cast<i16>(have) < static_cast<i16>(cost);
        const u16 color = isShort ? battle.darkColor : battle.lightColor;
        font.DrawText(this->drawSurface, x + kSpellNameX, rowY, shown, color, TextFont::BoldOutline);
        const std::string number = WinFormat(kCostFormat, {static_cast<i32>(static_cast<i16>(cost))});
        font.DrawText(this->drawSurface, x + kListCountX, rowY, number, color, TextFont::BoldOutline);

        if (cursor == row + 1)
        {
            font.DrawText(this->drawSurface, x + kSpellNameX - 1, rowY - 1, shown, color, TextFont::BoldOutline);
            FillBox(
                this->painter,
                x - kSpellBarLeft,
                rowY - kSpellBarUp,
                kSpecialBarWidth,
                kListBarHeight,
                kListBarRed,
                kListBarGreen,
                0,
                FillMode::Gradient
            );
            FillBox(
                this->painter,
                0,
                kDescriptionTop,
                Display::Width,
                kDescriptionHeight,
                0,
                kDescriptionGreen,
                kDescriptionBlue,
                FillMode::Gradient
            );
            const std::string description =
                role.skills != nullptr ? ObjectDescription(this->databases.mapsData, role.skills->id) : std::string();
            const auto halves = static_cast<int>(description.size()) >> 1;
            font.DrawText(
                this->drawSurface,
                kDescriptionCentre - ((kListAdvance * halves) >> 1),
                kDescriptionTextTop,
                description,
                kWhite,
                TextFont::BoldOutline
            );
            font.SetBackground(TextFont::NoBackground);
            battle.unk53BFB8 = 1;
            battle.unk53BD10 = index;
        }

        if (++row >= kListRowsShown)
        {
            font.SetBackground(TextFont::NoBackground);
            return;
        }
    }

    role.ShowAllSkills();
    font.SetBackground(TextFont::NoBackground);
}

// swd32.exe 0x004659C0: the object list of category unk4A7570 at (x, y): its four page tabs (frames 3..0 right to
// left, the open one frame 4 + category), the panel act restarted, a quarter-dark box and a frame; then from entry
// `top` + 1 up to 7 shown objects (unk53BDF4 counts them) with their counts ("%2d"), dark when too costly. Row
// `cursor` (from 1) is drawn again a pixel up and left under a gradient bar, its entry becomes unk53BD14, and the
// object's description is centred in a bar at the bottom.
void Game::DrawItemList(int x, int y, int cursor, int top)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    const int tabsY = y + kTabsDrop;
    int tabX = x + kTabWidth * (kItemPages - 1);

    for (int tab = kItemPages - 1; tab >= 0; --tab)
    {
        this->battleHud.DrawEffectAt(kTabEffect, tab, tabX, tabsY, 0);
        tabX -= kTabWidth;
    }

    this->battleHud
        .DrawEffectAt(kTabEffect, battle.unk4A7570 + kOpenTabFrame, x + kTabWidth * battle.unk4A7570, tabsY, 0);
    font.SetBackground(kHighlightBackground);
    font.SetBackground(TextFont::NoBackground);
    messages.panelAct = ActState{};
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(
        this->painter,
        x,
        y + kListBoxDrop,
        kItemBoxWidth,
        kListBoxHeight,
        0,
        kListShade,
        kListShade,
        FillMode::Quarter
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kItemFrameRight,
        y + kListFrameBottom,
        0,
        kListFrameBorder
    );
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.unk53BDF4 = 0;
    role.CountObjects(battle.unk4A7570, battle.unk53BDF4);
    role.ShowAllItems();
    int row = 0;

    for (int index = top + 1;; ++index)
    {
        std::string name;
        u16 flags = 0;
        u16 count = 0;

        if (role.ListObject(battle.unk4A7570, index, name, flags, count) == 0)
        {
            break;
        }

        if ((flags & kShownItem) != 0)
        {
            const std::string number = WinFormat(kCountFormat, {static_cast<i32>(static_cast<i16>(count))});
            const u16 color = (flags & kCostlyItem) != 0 ? battle.darkColor : battle.lightColor;
            const int rowY = y + kListRowHeight * row;
            font.DrawText(this->drawSurface, x + kListCountX, rowY + kListRowTop, number, color, TextFont::BoldOutline);
            font.DrawText(this->drawSurface, x + kListNameX, rowY + kListRowTop, name, color, TextFont::BoldOutline);

            if (cursor == row + 1)
            {
                font.DrawText(
                    this->drawSurface,
                    x + kListNameX - 1,
                    rowY + kListRowTop - 1,
                    name,
                    color,
                    TextFont::BoldOutline
                );
                font.SetBackground(TextFont::NoBackground);
                FillBox(
                    this->painter,
                    x,
                    rowY + kListBarTop,
                    kItemBarWidth,
                    kListBarHeight,
                    kListBarRed,
                    kListBarGreen,
                    0,
                    FillMode::Gradient
                );
                battle.unk53BD14 = index;
                battle.unk53BFB8 = 1;
                FillBox(
                    this->painter,
                    0,
                    kDescriptionTop,
                    Display::Width,
                    kDescriptionHeight,
                    0,
                    kDescriptionGreen,
                    kDescriptionBlue,
                    FillMode::Gradient
                );
                const std::string description =
                    role.inventoryNode != nullptr ? ObjectDescription(this->databases.mapsData, role.inventoryNode->id)
                                                  : std::string();
                const auto halves = static_cast<int>(description.size()) >> 1;
                font.DrawText(
                    this->drawSurface,
                    kDescriptionCentre - ((kListAdvance * halves) >> 1),
                    kDescriptionTextTop,
                    description,
                    kWhite,
                    TextFont::BoldOutline
                );
            }

            font.SetBackground(TextFont::NoBackground);
            ++row;
        }

        if (row >= kListRowsShown)
        {
            font.SetBackground(TextFont::NoBackground);
            return;
        }
    }

    role.ShowAllItems();
    font.SetBackground(TextFont::NoBackground);
}

// swd32.exe 0x00467710: the 戰鬥勝利 panel. The first time (the gold not yet given, bit 15 of unk53BF12): the music
// and the sounds stop, kVictorySound plays; each enemy's drop is added to the inventory as pending and to the loot
// list (a new object at the end, at most LootSlots of them); each party role standing (not forced) below the level
// cap (timingWindow) gains unk53BF14 experience, every worn accessory unk53BF16 proficiency (unk53C4C4 when one
// counts), its battle count 1 (2 against three enemies or more), its effects end and its main act restarts; then the
// gold is given. Once the panel has grown, the experience, the gold and the proficiency are shown.
void Game::DrawVictoryPanel()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    BattleServices& services = this->battleServices;
    TextFont& font = this->mediumFont;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(
        this->painter,
        kEscortShadeX,
        kEscortShadeY,
        kEscortShadeWidth,
        messages.panelHeight + kVictoryShadeExtra,
        0,
        kListShade,
        kListShade,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortTitleTop,
        kEscortFrameRight,
        kEscortTitleBottom,
        0,
        kListFrameBorder
    );
    font.DrawText(
        this->drawSurface,
        kVictoryTitleX,
        kEscortTitleTop,
        Big5(kVictoryTitle),
        kTitleColor,
        TextFont::BoldOutline
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortBodyTop,
        kEscortFrameRight,
        messages.panelHeight + kEscortBodyTop,
        0,
        kListFrameBorder
    );

    if ((battle.unk53BF12 & kGoldGiven) == 0)
    {
        this->audio.StopMusic();
        this->audio.StopAllSounds();
        this->audio.PlaySound(kVictorySound, this->config.soundVolume);

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            const u16 drop = this->battleEnemies[index].RollDrop(services);

            if (drop == 0)
            {
                continue;
            }

            const auto known = std::find(battle.lootIds.begin(), battle.lootIds.end(), drop);

            if (known != battle.lootIds.end())
            {
                AddOneItem(this->party.inventory, drop, 1, this->database);
                ++messages.lootCounts[static_cast<std::size_t>(known - battle.lootIds.begin())];
                continue;
            }

            // The original appends past its ten slots when more objects drop.
            const std::size_t slot = messages.lootCount;
            ObjectRecord* record = AddOneItem(this->party.inventory, drop, 1, this->database);

            if (slot >= battle.lootIds.size())
            {
                continue;
            }

            ++messages.lootCounts[slot];
            ++messages.lootCount;
            messages.lootNames[slot] = record != nullptr
                                           ? std::string_view(record->data.name.data(), ObjectName(*record).size())
                                           : std::string_view();
            battle.lootIds[slot] = drop;
        }

        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];

            if (role.unk2B00 == 1 || role.unk2B04 == 1 || role.IsDefeated())
            {
                continue;
            }

            const auto partyRole = static_cast<std::size_t>(battle.partyRoles[index]);
            RoleRecord& record = this->party.roles[partyRole];

            if (record.level < battle.timingWindow)
            {
                record.unk00 += battle.unk53BF14;
            }

            if (role.StepWornCounters(this->party.fame.lists[Fame::ProficiencyList], battle.unk53BF16) == 1)
            {
                battle.unk53C4C4 = 1;
            }

            i32& battles = this->party.battleBlocks[partyRole].unk00[kBattlesFought];
            ++battles;

            if (battle.enemyCount >= kManyEnemies)
            {
                ++battles;
            }

            role.ExpireEffects(services, 1);
            role.RestartMainAct(1, 1);
        }

        battle.unk53BD20 = 0;
        battle.unk53BF12 = static_cast<u16>(battle.unk53BF12 | kGoldGiven);
        this->party.variables[kGoldVariable] += battle.unk53BF12 & kGoldMask;
    }

    if (messages.GrowPanel(kEscortBodyTop, kVictoryBodyBottom, kVictoryGrowDivisor) != 1)
    {
        return;
    }

    font.DrawText(
        this->drawSurface,
        kVictoryTextX,
        kVictoryExperienceY,
        WinFormat(kExperienceLine, {static_cast<i32>(battle.unk53BF14)}),
        kTitleColor,
        TextFont::BoldOutline
    );
    font.DrawText(
        this->drawSurface,
        kVictoryTextX,
        kVictoryGoldY,
        WinFormat(kGoldLine, {static_cast<i32>(battle.unk53BF12 & kGoldMask)}),
        kTitleColor,
        TextFont::BoldOutline
    );
    font.DrawText(
        this->drawSurface,
        kVictoryTextX,
        kVictoryProficiencyY,
        WinFormat(kProficiencyLine, {static_cast<i32>(battle.unk53BF16)}),
        kTitleColor,
        TextFont::BoldOutline
    );
}

// swd32.exe 0x00467AC0: the 升級 panel while a role levels up (unk4A7646) or a skill was learned (unk53BFFC): a shade
// with a title and a body frame; once the panel has grown, the name of the role that levelled up.
void Game::DrawLevelUpPanel()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.unk4A7646 != kNoLevelUp || battle.unk53BFFC == 1)
    {
        messages.panelAct.id = BattleMessages::PanelAct;
        messages.panelAct.action = 0;
        this->acts.Run(messages.panelAct);
        FillBox(
            this->painter,
            kEscortShadeX,
            kEscortShadeY,
            kEscortShadeWidth,
            messages.panelHeight + kVictoryShadeExtra,
            0,
            kListShade,
            kListShade,
            FillMode::Blend
        );
        DrawWindowFrame(
            this->painter,
            this->archive,
            messages.panelAct.frame,
            kEscortFrameLeft,
            kEscortTitleTop,
            kEscortFrameRight,
            kEscortTitleBottom,
            0,
            kListFrameBorder
        );
        font.DrawText(
            this->drawSurface,
            kLevelUpTitleX,
            kEscortTitleTop,
            Big5(kLevelUpTitle),
            kTitleColor,
            TextFont::BoldOutline
        );
        DrawWindowFrame(
            this->painter,
            this->archive,
            messages.panelAct.frame,
            kEscortFrameLeft,
            kEscortBodyTop,
            kEscortFrameRight,
            messages.panelHeight + kEscortBodyTop,
            0,
            kListFrameBorder
        );
    }

    if (messages.GrowPanel(kEscortBodyTop, kLevelUpBodyBottom, kLevelUpGrowDivisor) != 1 ||
        battle.unk4A7646 == kNoLevelUp)
    {
        return;
    }

    const auto role = static_cast<std::size_t>(battle.partyRoles[battle.unk4A7646]);
    const Party::Name& name = this->party.names[role];
    const std::string text(name.data(), std::find(name.begin(), name.end(), '\0') - name.begin());
    font.DrawText(this->drawSurface, kVictoryTextX, kLevelUpTextY, text, kTitleColor, TextFont::BoldOutline);
}

// swd32.exe 0x00467C50: the first party role standing (not forced) with the experience of its next level (below the
// level cap) levels up: the record is kept in unk4FF108, the next level's maxima and attributes (LevelUp of both
// levels; a skill learned sets unk53BFFC) are added to it as the difference, hit points, spirit and stamina refilled,
// its level and experience mark taken; unk4A7646 becomes the role and the level-up sound plays. Otherwise unk4A7646
// is kNoLevelUp. unk53C4C8 is set either way.
void Game::CheckLevelUps()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        const BattleRole& unit = this->battleRoles[index];

        if (unit.unk2B00 == 1 || unit.unk2B04 == 1)
        {
            continue;
        }

        const auto role = battle.partyRoles[index];
        RoleRecord& record = this->party.roles[static_cast<std::size_t>(role)];
        const int next = record.level + 1;
        u32 needed = 0;

        if (record.level >= battle.timingWindow ||
            (this->database.LoadLevelEntry(role + 1, next, needed), record.unk00 < static_cast<i32>(needed)))
        {
            battle.unk4A7646 = kNoLevelUp;
            continue;
        }

        RoleRecord before;
        RoleRecord after;
        battle.unk4FF108 = record;
        LevelUp(
            before,
            role + 1,
            record.level,
            battle.unk53BFFC,
            this->party,
            this->database,
            battle.lastLearned,
            this->errors
        );
        LevelUp(after, role + 1, next, battle.unk53BFFC, this->party, this->database, battle.lastLearned, this->errors);
        record.level = after.level;
        record.experience = after.experience;
        record.maxHitPoints = static_cast<i16>(record.maxHitPoints + after.maxHitPoints - before.maxHitPoints);
        record.hitPoints = record.maxHitPoints;
        record.maxSpirit = static_cast<i16>(record.maxSpirit + after.maxSpirit - before.maxSpirit);
        record.spirit = record.maxSpirit;
        record.maxStamina = static_cast<i16>(record.maxStamina + after.maxStamina - before.maxStamina);
        record.stamina = record.maxStamina;
        record.attack = static_cast<i16>(record.attack + after.attack - before.attack);
        record.defense = static_cast<i16>(record.defense + after.defense - before.defense);
        record.unk14 = static_cast<i16>(record.unk14 + after.unk14 - before.unk14);
        record.agility = static_cast<i16>(record.agility + after.agility - before.agility);
        record.unk18 = static_cast<i16>(record.unk18 + after.unk18 - before.unk18);
        record.unk1A = static_cast<i16>(record.unk1A + after.unk1A - before.unk1A);
        record.ailmentChance = static_cast<i16>(record.ailmentChance + after.ailmentChance - before.ailmentChance);
        record.unk1E = static_cast<i16>(record.unk1E + after.unk1E - before.unk1E);
        battle.unk4A7646 = static_cast<u8>(index);
        this->audio.StopSound(kVictorySound);
        this->audio.PlaySound(kLevelUpSound, this->config.soundVolume);
        break;
    }

    battle.unk53C4C8 = 1;
}

// swd32.exe 0x00468930: the box of a new spell (%s學到新魔法) of the role that levelled up
void Game::DrawNewSpell()
{
    this->DrawLearnedBox(kNewSpellLine, false);
}

// swd32.exe 0x00468AD0: the box of a new special skill (%s學到新絕招), with kLearnedSound as it opens
void Game::DrawNewSpecial()
{
    this->DrawLearnedBox(kNewSpecialLine, true);
}

// The box of 0x00468930 and 0x00468AD0 (inline in each): while a skill was learned (unk53BFFC), the line with the
// role's name in a box as wide as it (20 pixels per two bytes), grown open; then the line in white and the skill's
// name in brackets, centred below it on a dark background.
void Game::DrawLearnedBox(std::u8string_view line, bool sound)
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.unk53BFFC != 1)
    {
        return;
    }

    if (sound && messages.panelHeight == 0)
    {
        this->audio.PlaySound(kLearnedSound, this->config.soundVolume);
    }

    const auto role = static_cast<std::size_t>(battle.partyRoles[battle.unk4A7646]);
    const Party::Name& roleName = this->party.names[role];
    const std::string name(roleName.data(), std::find(roleName.begin(), roleName.end(), '\0') - roleName.begin());
    const std::string text = WinFormat(line, {name});
    const int halves = static_cast<int>(text.size()) / 2;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(
        this->painter,
        kLearnedShadeX,
        kEscortShadeY,
        kTagCharWidth * halves + 2 * kTagMargin,
        messages.panelHeight + 2 * kTagMargin,
        0,
        kListShade,
        kListShade,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kLearnedFrameLeft,
        kEscortTitleTop,
        kTagCharWidth * halves + kLearnedFrameLeft,
        messages.panelHeight + kEscortTitleTop,
        0,
        kListFrameBorder
    );

    if (messages.GrowPanel(kEscortTitleTop, kLearnedBodyBottom, kLevelUpGrowDivisor) != 1)
    {
        return;
    }

    font.DrawText(this->drawSurface, kLearnedTextX, kLearnedTextY, text, kWhite, TextFont::BoldOutline);
    const std::string skill = WinFormat(kBracketFormat, {battle.lastLearned.name});
    const int quarters = static_cast<int>(skill.size()) >> 2;
    const int skillX = ((kListAdvance * halves) >> 1) - kListAdvance * quarters + kLearnedFrameLeft;
    font.DrawText(this->drawSurface, skillX, kLearnedSkillY, skill, kLearnedSkillColor, TextFont::BoldOutline);
}

// swd32.exe 0x00468C80: each battle role standing (not forced) whose next special skill is not exhausted (battle
// block [3] not -1) learns it once it has fought enough battles: its type 1 skills (from Party::skills by battle role
// index, as in the original) set the battle block's needed battles ([2], the skill's unk44) and next special ([3],
// its unk42 with bit 31). With [1] battles at least [2] and a next special (not 0x665 or 0x669 while game flag
// kSpecialsFlag is set), the special is appended to the list, its name kept for the message (unk53BFFC, unk4A7646 the
// role) and the block restarted from it (-1 without a further one).
void Game::LearnSpecials()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];
        RoleBattleBlock& block = this->party.battleBlocks[static_cast<std::size_t>(battle.partyRoles[index])];
        std::unique_ptr<ItemNode>& list = this->party.skills[index];

        if (role.unk2B00 == 1 || role.unk2B04 == 1 || role.IsDefeated() || block.unk00[kNextSpecial] == kNoSpecial ||
            list == nullptr)
        {
            continue;
        }

        auto takeNext = [this, &battle, &block](u16 id)
        {
            this->database.LoadObject(battle.unk53BC28, id);
            Database::FreeDescription(battle.unk53BC28);
            block.unk00[kNeededBattles] = battle.unk53BC28.data.unk44;
            block.unk00[kNextSpecial] = static_cast<i32>(battle.unk53BC28.data.unk42 | kSpecialMark);
        };

        for (ItemNode* node = list->next.get(); node != nullptr; node = node->next.get())
        {
            if (node->record.data.unk52 == kSpecialType)
            {
                takeNext(node->id);
            }
        }

        const auto next = static_cast<u32>(block.unk00[kNextSpecial]) & kSpecialIdMask;

        if (block.unk00[kBattlesFought] < block.unk00[kNeededBattles] || next == 0)
        {
            continue;
        }

        if (this->state.GameFlag(kSpecialsFlag) && (next == kBarredSpecialA || next == kBarredSpecialB))
        {
            continue;
        }

        const auto id = static_cast<u16>(block.unk00[kNextSpecial]);
        this->database.LoadObject(battle.unk53BC28, id);
        Database::FreeDescription(battle.unk53BC28);
        ItemNode* last = list.get();

        while (last->next != nullptr)
        {
            last = last->next.get();
        }

        last->next = std::make_unique<ItemNode>();
        ItemNode& learned = *last->next;
        learned.id = id;
        this->database.LoadObject(learned.record, id);
        battle.unk53BFFC = 1;
        battle.lastLearned.name = ObjectName(learned.record);
        battle.unk4A7646 = static_cast<u8>(index);
        block.unk00[kBattlesFought] = 0;
        block.unk00[kNeededBattles] = battle.unk53BC28.data.unk44;
        block.unk00[kNextSpecial] = static_cast<i32>(battle.unk53BC28.data.unk42 | kSpecialMark);

        if (battle.unk53BC28.data.unk42 == 0)
        {
            block.unk00[kNextSpecial] = kNoSpecial;
        }
    }
}

// swd32.exe 0x00467F00: the stats of the role that levelled up (unk4A7646): its name over a frame of the seven values
// before the level-up (unk4FF108); once grown, after kRollStart frames (unk53BDEA, kShowNow shows at once) each value
// that rose shows " ==> " and the new value less a roll that counts down a point a frame (kRollSound each point), one
// after another: the maxima first, then the attributes (strength does not wait for the maxima, as in the original).
void Game::DrawLevelUpStats()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    TextFont& font = this->mediumFont;

    if (battle.unk4A7646 == kNoLevelUp)
    {
        return;
    }

    const auto partyRole = static_cast<std::size_t>(battle.partyRoles[battle.unk4A7646]);
    const RoleRecord& now = this->party.roles[partyRole];
    const RoleRecord& before = battle.unk4FF108;
    std::array<i16, kStatRolls>& rolls = battle.statRolls;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    this->acts.Run(messages.panelAct);
    FillBox(
        this->painter,
        kEscortShadeX,
        kStatsShadeY,
        kStatsShadeWidth,
        messages.panelHeight + kVictoryShadeExtra,
        0,
        kListShade,
        kListShade,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kStatsTitleTop,
        kStatsFrameRight,
        kStatsTitleBottom,
        0,
        kListFrameBorder
    );
    const Party::Name& name = this->party.names[partyRole];
    const std::string roleName(name.data(), std::find(name.begin(), name.end(), '\0') - name.begin());
    font.DrawText(this->drawSurface, kStatsTitleX, kStatsTitleTop, roleName, kTitleColor, TextFont::BoldOutline);
    DrawWindowFrame(
        this->painter,
        this->archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kStatsBodyTop,
        kStatsFrameRight,
        messages.panelHeight + kStatsBodyTop,
        0,
        kListFrameBorder
    );

    if (messages.GrowPanel(kStatsBodyTop, kStatsBodyBottom, kVictoryGrowDivisor) != 1)
    {
        return;
    }

    const std::array<i32, kStatLines> old = {
        before.maxHitPoints,
        before.maxSpirit,
        before.maxStamina,
        static_cast<u16>(before.attack),
        static_cast<u16>(before.defense),
        static_cast<u16>(before.unk14),
        static_cast<u16>(before.agility),
    };

    for (std::size_t line = 0; line < kStatLines; ++line)
    {
        const int y = kStatsFirstY + kStatsLineHeight * static_cast<int>(line);
        font.DrawText(
            this->drawSurface,
            kStatsTextX,
            y,
            WinFormat(kStatLabels[line], {old[line]}),
            kWhite,
            TextFont::BoldOutline
        );
    }

    const bool showNow = battle.unk53BDEA == kShowNow;

    if (showNow)
    {
        rolls = {};
    }
    else
    {
        if (battle.unk53BDEA == kRollStart - 1)
        {
            rolls[0] = static_cast<i16>(now.maxHitPoints - before.maxHitPoints);
            rolls[1] = static_cast<i16>(now.maxSpirit - before.maxSpirit);
            rolls[2] = static_cast<i16>(now.maxStamina - before.maxStamina);
            rolls[3] = static_cast<i16>(now.attack - before.attack);
            rolls[4] = static_cast<i16>(now.defense - before.defense);
            rolls[5] = static_cast<i16>(now.unk14 - before.unk14);
            rolls[6] = static_cast<i16>(now.agility - before.agility);
            rolls[7] = static_cast<i16>(now.unk18 - before.unk18);
            rolls[8] = static_cast<i16>(now.unk1A - before.unk1A);
        }

        if (battle.unk53BDEA < kRollStart && ++battle.unk53BDEA < kRollStart)
        {
            return;
        }
    }

    auto rise = [this, &font](int y, i32 value)
    {
        font.DrawText(this->drawSurface, kStatsArrowX, y, kArrow, kWhite, TextFont::BoldOutline);
        font.DrawText(
            this->drawSurface,
            kStatsNewX,
            y,
            WinFormat(kNewValueFormat, {value}),
            kLearnedSkillColor,
            TextFont::BoldOutline
        );
    };

    auto tick = [this]
    {
        this->audio.PlaySound(kRollSound, this->config.soundVolume);
    };

    auto lineY = [](int line)
    {
        return kStatsFirstY + kStatsLineHeight * line;
    };

    if (now.maxHitPoints > before.maxHitPoints)
    {
        rise(lineY(0), now.maxHitPoints - rolls[0]);

        if (rolls[0] > 0)
        {
            --rolls[0];
            tick();
        }
    }

    if (now.maxSpirit > before.maxSpirit)
    {
        rise(lineY(1), now.maxSpirit - rolls[1]);

        if (rolls[0] == 0 && rolls[1] > 0)
        {
            --rolls[1];
            tick();
        }
    }

    if (now.maxStamina > before.maxStamina)
    {
        rise(lineY(2), now.maxStamina - rolls[2]);

        if (rolls[1] == 0 && rolls[2] > 0)
        {
            --rolls[2];
            tick();
        }
    }

    // The attributes compare and count unsigned.
    auto attribute = [&](std::size_t line, i16 newValue, i16 oldValue, std::size_t roll, bool ready)
    {
        if (static_cast<u16>(newValue) <= static_cast<u16>(oldValue))
        {
            return;
        }

        rise(lineY(static_cast<int>(line)), static_cast<u16>(newValue) - static_cast<u16>(rolls[roll]));

        if (ready && rolls[roll] != 0)
        {
            rolls[roll] = static_cast<i16>(rolls[roll] - 1);
            tick();
        }
    };

    attribute(3, now.attack, before.attack, 3, true);
    attribute(4, now.defense, before.defense, 4, rolls[3] == 0);
    attribute(5, now.unk14, before.unk14, 5, rolls[4] == 0);
    attribute(6, now.agility, before.agility, 6, rolls[5] == 0);
}
}  // namespace swd3
