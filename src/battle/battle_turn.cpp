#include "battle/battle_turn_constants.h"
#include "battle/smooth/smooth_turn.h"
#include "core/fault.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "font/text_font.h"
#include "game/game.h"
#include "item/item_list.h"
#include "org/fame.h"
#include "pic/picture_ops.h"
#include "platform/display.h"
#include "platform/system_info.h"
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

constexpr u16 kThrownObject = 0x300;  // the object a thrown attack takes (given back unless it lands)
constexpr i32 kLostEnd = 2;           // frameResult when the party is down
constexpr i32 kLostQuietly = 0x68;    // a battle state that ends with frameResult 0
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
constexpr i32 kMenuState = 1;  // the command menu

constexpr i32 kMenuFixedCommands = 5;  // before the accessory commands

constexpr i32 kMenuStep = 5;
constexpr i32 kMenuStepMost = 10;

constexpr std::size_t kAccessorySlots = 2;
constexpr std::size_t kFirstAccessorySlot = 7;  // the equipment slots of the accessories

constexpr u16 kLastAccessoryCommand = 0x31;
constexpr u16 kSpiritPact = 6;
constexpr i32 kGuardedCommand = 2;

constexpr i32 kTargetState = 3;  // battle states of the menu
constexpr i32 kTalkState = 7;
constexpr i32 kSkillListState = 8;
constexpr i32 kSpellCommand = 2;  // the menu commands that open a list
constexpr i32 kItemCommand = 3;
constexpr i32 kSkillCommand = 4;
constexpr u16 kNoTalkRole = 0xFFFF;
constexpr i32 kListRows = 5;

constexpr u16 kLevelUpSound = 0x12B;
constexpr std::u8string_view kNewSpellLine = u8"%s學到新魔法";    // swd32.exe 0x004A7AB4
constexpr std::u8string_view kNewSpecialLine = u8"%s學到新絕招";  // swd32.exe 0x004A7AC4

constexpr std::size_t kNeededBattles = 2;  // RoleBattleBlock::unk00: the battles the next special needs
constexpr std::size_t kNextSpecial = 3;    // the next special skill with bit 31, -1: none
constexpr i32 kNoSpecial = -1;
constexpr u32 kSpecialMark = 0x80000000u;
constexpr u32 kSpecialIdMask = 0x7FFFFFFF;
constexpr u16 kSpecialType = 0x1F;
constexpr int kSpecialsFlag = 0x1BB0;
constexpr u32 kBarredSpecialA = 0x665;
constexpr u32 kBarredSpecialB = 0x669;

constexpr i32 kLastMenuCommand = 8;  // the up key
constexpr i32 kEscortRows = 2;
constexpr int kHangTries = 2;      // full cycles after which a search of the original would never end
constexpr u32 kDebugPause = 0xC8;  // the debug keys
constexpr u32 kDebugShortPause = 0x64;
constexpr int kDebugMessageX = 0x208;
constexpr int kDebugMessageY = 0xA;
constexpr i16 kDebugMessageFrames = 0x1E;
constexpr std::u8string_view kStrongAttack = u8"強攻擊";    // swd32.exe 0x004A7838
constexpr std::u8string_view kNormalPact = u8"正常靈契";    // swd32.exe 0x004A782C
constexpr std::u8string_view kAbsolutePact = u8"絕對靈契";  // swd32.exe 0x004A7820
constexpr i16 kRefill = -1;
constexpr int kFillFury = -1;
constexpr i16 kDebugAttack = 0x26AC;
constexpr i16 kDebugDefense = 0x9B;
constexpr i16 kDebugAgility = 0xC8;
constexpr int kSmallHurt = 0x50;
constexpr i16 kSmallDrain = -10;
constexpr int kBigHurt = 0x1F4;
constexpr i16 kBigDrain = -5;
constexpr int kEnemyHurt = 10;
constexpr int kDefeatHurt = 0x7530;
constexpr int kToggleStates = 2;
constexpr std::size_t kClearedTurnUnits = 8;  // the original clears 0x28 bytes from 0x0053AF30
constexpr i16 kDebugStep = 10;

constexpr int kPanelsTop = 0x182;  // the pointer over the battle
constexpr int kPanelLeftMargin = 0x18;
constexpr int kPanelWidth = 0x74;
constexpr int kMenuHitLeft = 0xA;
constexpr int kMenuHitRight = 0x76;
constexpr int kMenuHitTop = 0x28;
constexpr int kMenuHitBottom = 0x88;
constexpr int kMenuHitColumn = 0x36;
constexpr int kTabsHitTop = 0x82;
constexpr int kTabsHitBottom = 0xA0;
constexpr int kTabsHitFirst = 0x10A;
constexpr int kSpellTabsEnd = 0x188;
constexpr int kItemTabsEnd = 0x1B2;
constexpr int kListHitLeft = 0xE0;
constexpr int kSpellListRight = 0x180;
constexpr int kItemListRight = 0x19C;
constexpr int kThrowListRight = 0x194;
constexpr int kListHitFirst = 0xBE;
constexpr int kListHitEnd = 0x14A;
constexpr int kScrollUpTop = 0x9E;
constexpr int kScrollUpBottom = 0xAE;
constexpr int kSpellScrollLeft = 0x192;
constexpr int kSpellScrollRight = 0x1A4;
constexpr int kSpellScrollDownTop = 0x12A;
constexpr int kSpellScrollDownBottom = 0x13A;
constexpr int kItemScrollLeft = 0x19E;
constexpr int kItemScrollRight = 0x1B0;
constexpr int kItemScrollDownTop = 0x12E;
constexpr int kItemScrollDownBottom = 0x13E;
constexpr int kThrowScrollDownBottom = 0x140;
constexpr i32 kScrollUp = 1;  // BattleState::hoverScrollPart: the scroll part under the pointer
constexpr i32 kScrollDown = 2;
constexpr i32 kScrollAbove = 3;
constexpr i32 kScrollBelow = 4;
constexpr int kEscortHitLeft = 0xC4;
constexpr int kEscortHitRight = 0x178;
constexpr int kEscortHitFirst = 0xEC;
constexpr int kEscortHitEnd = 0x118;
constexpr int kSpecialHitRight = 0x199;
constexpr int kSpecialHitFirst = 0xBA;
constexpr int kSpecialHitEnd = 0x162;
constexpr int kRefineHitLeft = 0xEA;
constexpr int kRefineHitFirst = 0x15A;
constexpr int kRefineHitEnd = 0x23A;
constexpr int kRefineRowFirst = 0xBA;
constexpr int kRefineRowEnd = 0x11E;
constexpr int kHitSquare = 8;
constexpr int kEnemyHitStep = 2;
constexpr i32 kRefineMenuCommand = 6;
constexpr int kPortraitHitTop = 0x16A;
constexpr int kPortraitHitBottom = 0x1B6;
constexpr int kPortraitHitWidth = 0x7C;
constexpr int kNoPicture = -1;
constexpr int kPictureMissed = 0;
constexpr int kPictureHit = 1;

constexpr i32 kFirstAccessoryMenuCommand = 6;
constexpr int kListPointerX = 0x108;
constexpr int kListPointerY = 0xB0;
constexpr int kListMouseX = 0xF0;
constexpr int kListMouseY = 0xC6;
// The rebuild: the keys after which the menu opens at its place (BattleKeys).
constexpr std::array<Button, 9> kMenuKeys = {
    Button::Escape,
    Button::Space,
    Button::Left,
    Button::Up,
    Button::Right,
    Button::Down,
    Button::PageUp,
    Button::PageDown,
    Button::Enter,
};

constexpr i32 kOpenListRows = 4;
constexpr i32 kAttackCommand = 1;
constexpr i32 kGuardCommand = 5;
constexpr i32 kAimedCommand15 = 0x15;
constexpr i32 kAimedCommand17 = 0x17;
constexpr i32 kAimedCommand1D = 0x1D;
constexpr i32 kAimedCommand1F = 0x1F;
constexpr i32 kAimedCommand21 = 0x21;
constexpr i32 kLeaveCommand = 0x11;
constexpr i32 kLeaveState = 6;
constexpr i32 kSelfCommand = 0x16;
constexpr i32 kPartyAimCommand = 0x18;
constexpr i32 kControlMark = 0x10;
constexpr i32 kStealCommand = 0x1A;
constexpr i32 kOnceCommand1C = 0x1C;
constexpr i32 kOnceCommand20 = 0x20;
constexpr i32 kOnceCommand22 = 0x22;
constexpr i32 kOnceCommand23 = 0x23;
constexpr i32 kOnceCommand24 = 0x24;
constexpr i32 kHealPage = 2;
constexpr i32 kThrowOrder = 0xD;
constexpr u16 kNoObjectKind = 0xFFFF;
constexpr i32 kItemCategory3 = 3;
constexpr int kRefusalMessageX = 0x118;
constexpr int kRefusalMessageY = 0xA;
constexpr i16 kRefusalMessageFrames = 0x14;
constexpr int kRefusalMessageFlags = BattleMessage::BlackBar | BattleMessage::Centered;
constexpr std::u8string_view kStaminaShort = u8"體力點數不夠!!";  // swd32.exe 0x004A7990
constexpr std::u8string_view kEscortCalling = u8"護駕召喚中!!";   // swd32.exe 0x004A7980
constexpr u16 kRefusedSound = 0x8C;
constexpr i32 kEscortOrder = 0xF;
constexpr i16 kEscortX = 0x12C;
constexpr i16 kFirstEscortY = 0x10E;
constexpr i16 kSecondEscortY = 0x154;
constexpr i32 kTalkOrder = 0x63;
constexpr int kRefineCategory = 5;
constexpr i32 kEndCraft = 0x62;  // battle states of the end
constexpr i32 kEndVictory = 0x64;
constexpr i32 kEndLevelUps = 0x65;
constexpr i32 kEndLoot = 0x66;
constexpr i32 kEndStats = 0x6E;
constexpr i32 kEndNewSpell = 0x6F;
constexpr i32 kEndSpecials = 0x70;
constexpr i32 kEndMastery = 0x71;
constexpr i32 kCloseMenuState = 0xC8;
constexpr i32 kEndMessageFrames = 0x14;
constexpr i32 kEndLevelUpFrames = 0x1E;
constexpr u16 kLevelUpPanelSound = 0x2C;
constexpr i16 kLearnedPose = 8;
constexpr u16 kOpenSound = 0x2D;
constexpr i32 kTalkFixedAnswers = 2;  // 攻擊 and 釋放
constexpr i32 kRepeatDelay = 0xF;     // the keys of the battle: held keys repeat from this frame
constexpr i32 kRepeatStep = 3;
constexpr i32 kConfirmRepeatStep = 6;
constexpr int kDigitCommands = 8;
constexpr int kDigitKey1 = 2;  // the scan code of 1
constexpr i32 kSpaceMark = 1;
constexpr u16 kRetreatMark = 0x4000;
constexpr u32 kRefusedPause = 0x14;
constexpr u32 kRetreatPause = 0x32;
constexpr i16 kRetreatMessageFrames = 5;
constexpr std::u8string_view kNoRetreatText = u8"無法撤退!!";  // swd32.exe 0x004A7954
constexpr i32 kThumbPart = 5;
constexpr i32 kConfirmIdle = 0x12C;
constexpr i32 kChoiceSignBit = 0x8000;
constexpr int kListX = 0xE0;
constexpr int kListY = 0x7E;
constexpr int kSpellListX = 0xE8;
constexpr int kSpellListY = 0x86;
constexpr i32 kListGrown = 7;
constexpr i32 kListOpened = 0xA;
constexpr u16 kListScrollBar = 0x232A;
constexpr int kSpellScrollX = 0x190;
constexpr int kItemScrollX = 0x19E;
constexpr int kThrowScrollX = 0x192;
constexpr int kListScrollY = 0x9E;
constexpr int kListScrollRows = 4;
constexpr int kEscortListX = 0xC4;
constexpr int kEscortListY = 0xCE;
constexpr int kControlMenuX = 0xC;
constexpr int kControlMenuY = 8;

constexpr int kRoleAimDrop = 0xA;
constexpr int kTargetTagX = 0xC;
constexpr int kTargetTagY = 0xE;
constexpr i32 kEndBack = 0x60;  // BattleEndFrame
constexpr i32 kEndGlow = 0x61;
constexpr i32 kEndCheck = 0x63;
constexpr i32 kEndPanelFrames = 0x96;
constexpr i32 kEndQuietFrames = 0x1E;
constexpr i16 kLevelUpPose = 2;
constexpr i16 kStatsPose = 4;
constexpr u8 kCraftFailed = 2;
constexpr i32 kRefineOrder = 0xE;

// The name of accessory command `kind`: kinds past the table read the variables after it in the original; no name
// here.
std::u8string_view AccessoryCommandName(u16 kind)
{
    const auto at = static_cast<std::size_t>(kind - kFirstAccessoryCommand);
    return at < BattleState::AccessoryCommandNames.size() ? BattleState::AccessoryCommandNames[at]
                                                          : std::u8string_view{};
}

}  // namespace

// swd32.exe 0x00464CC0: role unit `unit` takes the turn (turnUnit, its unit turn and SetTarget 1); while an enemy
// stands, aimTarget picks one at random (1-based), going on past defeated ones for at most enemyCount tries.
void Game::StartRoleTurn(int unit)
{
    BattleState& battle = this->battleState;
    battle.turnUnit = unit;
    battle.turnCommand = 1;
    battle.unk53BF60 = 1;
    battle.turnTaken[static_cast<std::size_t>(unit)] = 1;
    this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].SetTarget(1);
    const auto count = static_cast<i32>(battle.enemyCount);

    if (battle.enemiesGone >= count)
    {
        return;
    }

    battle.aimTarget = static_cast<i32>(this->random.Below(battle.enemyCount)) + 1;
    int tries = 0;

    while (this->battleEnemies[static_cast<std::size_t>(battle.aimTarget - 1)].IsDefeated())
    {
        ++battle.aimTarget;

        if (battle.aimTarget > count)
        {
            battle.aimTarget = 1;
        }

        if (++tries >= count)
        {
            break;
        }
    }
}

// swd32.exe 0x0045ADF0: when `enemy` is unk4A7626 its turn is reset, and each role but `role` aiming at it turns to
// the first standing enemy other than it (retargeted set). With none and one enemy left, such a role's aim, command,
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
                battle.retargeted = 1;
                turned = true;
                break;
            }
        }

        if (turned || battle.enemiesGone + 1 != count)
        {
            continue;
        }

        unit.ClearWord29A2();
        this->battleEnemies[aim].DecreaseHits();
        unit.SetCommand(0);
        unit.SetTarget(0);
        unit.ResetTurn(services);
        battle.unk53BF64 = 0;
        battle.roleOrder = {};
        battle.turnUnit = 0;
        battle.currentUnit = 0;
        battle.actingUnit = -1;
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
        battle.lastDefeatedObject = enemy.Data();
        const bool boss = (battle.lastDefeatedObject->data.kind & ObjectData::UnkKind20) != 0;
        battle.unk53BF16 = static_cast<u16>(battle.unk53BF16 + (boss ? kBossReward : kEnemyReward));
        AddCounterCapped(this->party.fame.lists[Fame::FameList], static_cast<i16>(enemy.ActId()), kDefeatCount);
        battle.RemoveSlotRecord(index);
        const auto count = static_cast<i32>(battle.enemyCount);

        if (index >= 0 && index <= count)
        {
            ++battle.enemiesGone;
        }

        if (battle.enemiesGone - battle.unk53BF02 >= count)
        {
            battle.result = kBattleWon;
            battle.autoBattle = 0;
            battle.unk53BFC4 = 1;
            battle.unk53BFC0 = 1;
        }

        if (battle.unk53BF28 != 0 && !this->battleEnemies[0].IsDefeated() &&
            static_cast<i32>(battle.enemyCount) - battle.enemiesGone == 1)
        {
            battle.enemyCount = 1;
            battle.enemiesGone = 0;
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

    if (unit.autoControl == 1)
    {
        ++battle.unk53BF01;

        if (battle.unk53BF01 >= battle.escortCount)
        {
            const u16 guests = battle.escortCount;

            // The original clears the places from the end of the list down, before it when there are fewer.
            for (u16 cleared = 0; cleared < guests && cleared < battle.roleCount; ++cleared)
            {
                battle.rolePlaces[battle.roleCount - 1 - cleared] = UnitPlacement{};
            }

            battle.escortCount = 0;
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
        battle.turnCommand = 0;
        battle.aimTarget = 1;
        battle.result = kTurnsRunning;
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.unk53BF74 = 0;
    }

    if (battle.actingUnit == unitId)
    {
        battle.actingUnit = -1;
        battle.unk53BF74 = 0;
    }

    for (std::size_t at = 0; static_cast<i32>(at) < static_cast<i32>(battle.roleCount); ++at)
    {
        if (battle.roleOrder[at] != unitId)
        {
            continue;
        }

        for (std::size_t next = at; next < kLastTurnUnit; ++next)
        {
            battle.roleOrder[next] = battle.roleOrder[next + 1];
        }

        battle.roleOrder[kLastTurnUnit] = 0;
        break;
    }

    if (battle.defeatedRoles >= battle.roleCount - battle.guestCount - battle.escortCount)
    {
        battle.WipeSlotRecords();
        battle.actingUnit = -1;
        battle.unk53BFC0 = 1;
        battle.unk53BFC4 = 1;
        battle.result = kBattleLost;
        return 1;
    }

    if (unit.EffectFlag2() == 1)
    {
        battle.aimTarget = battle.actingUnit + 1;
        battle.turnUnit = unitId;
        unit.SetTarget(1);
        battle.turnCommand = 1;
        battle.turnTaken[static_cast<std::size_t>(unitId)] = 1;
        battle.unk53BF60 = 1;
        battle.pendingOrders[static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit)] = PendingOrder{};
    }

    return 1;
}

// swd32.exe 0x0045B280: the draw order with the acting unit (actingUnit) drawn right after the unit it aims at (none
// while unk53C4C0, unk53AE7C or unk53AE80 is set or no unit acts): a role aims at an enemy, an enemy at a role.
void Game::SortActingDrawOrder()
{
    BattleState& battle = this->battleState;

    if (battle.unk53C4C0 == 1 || battle.unk53AE7C == 1 || battle.unk53AE80 == 1 || battle.actingUnit == -1)
    {
        return;
    }

    const i32 acting = battle.actingUnit;
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

// swd32.exe 0x0045BD10: one frame of the field fading to black: the frame's pixels shifted by the fade (blackFade,
// also the painter's tint), 2 darker a frame; 1 once past -32 (the fade reset to 0).
int Game::FadeFieldOut()
{
    smooth::Scene scene;
    const auto result = smooth::TurnFrame(*this).FadeFieldOutAdvance(scene);
    scene.Show(this->battleServices);
    return result;
}

// swd32.exe 0x0045E9C0: the objects held back in the battle given to the inventory: givenBack[1] and [2], and one
// kThrownObject for each still thrown (unk53C4B0); the counts and escortCount are cleared.
void Game::ReturnBattleItems()
{
    BattleState& battle = this->battleState;

    for (std::size_t at = 1; at < battle.givenBack.size(); ++at)
    {
        if (battle.givenBack[at] != 0)
        {
            AddOneItem(this->party.inventory, battle.givenBack[at], 0, this->database);
        }
    }

    for (i32 thrown = 0; thrown < battle.unk53C4B0; ++thrown)
    {
        AddOneItem(this->party.inventory, kThrownObject, 0, this->database);
    }

    battle.escortCount = 0;
    battle.unk53C4B0 = 0;
    battle.givenBack[1] = 0;
    battle.givenBack[2] = 0;
}

// swd32.exe 0x0045E580: with every party role down (unk53BF5C set) the field fades out (endFade), then the music
// stops, the objects return and frameResult becomes 2 (0 for a battle state of 0x68 or UnkPanel8); with every enemy
// down or wonByTalk set it fades out too, the objects return and frameResult becomes 0.
void Game::CheckBattleEnd()
{
    BattleState& battle = this->battleState;

    if (battle.defeatedRoles >= battle.roleCount - battle.escortCount - battle.guestCount)
    {
        battle.unk53BF5C = 1;

        if (battle.endFade == 1 && this->FadeFieldOut() == 1)
        {
            this->audio.StopMusic();
            this->ReturnBattleItems();
            battle.frameResult = kLostEnd;

            if (battle.result == kLostQuietly)
            {
                battle.frameResult = 0;
            }

            if ((battle.panelFlags & BattleState::UnkPanel8) != 0)
            {
                battle.frameResult = 0;
            }
        }
    }

    if (battle.enemiesGone - battle.unk53BF02 >= static_cast<i32>(battle.enemyCount) || this->fightTalk.wonByTalk == 1)
    {
        battle.unk53BF5C = 1;

        if (battle.endFade == 1 && this->FadeFieldOut() == 1)
        {
            this->ReturnBattleItems();
            battle.frameResult = 0;
        }
    }
}

// swd32.exe 0x0045E660: the pointer prompt of the battle state for its first 300 frames (always while unk53BDAC is
// negative): kWaitPointer in states 1, 2, 4, 5, 8, 0x1B and 0x1E; in state 3 (with unk53BFBC) kTurnPointer or
// kNoTurnPointer by turnPointer; otherwise kUnitPointer with a current unit, else kNoUnitPointer, or with a negative
// unk53BDAC the effect kMarkEffect at (unk53BF50, unk53BF52) (unk53BF58 cleared once it ends).
void Game::DrawBattlePrompt()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;
    const int x = this->state.pointerX;
    const int y = this->state.pointerY;
    ++battle.promptFrames;

    if (battle.promptFrames >= kPromptFrames && battle.unk53BDAC >= 0)
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
        {
            const smooth::Scene::Pointer following(this->battleServices);
            hud.DrawEffectAt(kWaitPointer, 0, x, y, 0);
            break;
        }

        case kTurnState:
            if (battle.unk53BFBC == 1)
            {
                const smooth::Scene::Pointer following(this->battleServices);
                hud.DrawEffectAt(battle.turnPointer != 0 ? kTurnPointer : kNoTurnPointer, 0, x, y, 0);
            }

            break;

        default:
            if (battle.unk53BDAC >= 0)
            {
                const smooth::Scene::Pointer following(this->battleServices);
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
    smooth::Scene scene;
    smooth::TurnFrame(*this).FieldFadeFrameAdvance(counting, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x0045EC80: while no unit acts, a side that is all down or turned to stone (UnkStatus4) loses: the party
// (forced units left out; the count against the roles less the guests, in 8 bits) unless endFade, the stone roles
// counted as defeated and the battle state 0x67; then, unless unk53C048, the enemies, counted as defeated, unk53C048
// set, the auto battle off and the battle state 0x63. 1 when a side lost.
int Game::CheckStoneSides()
{
    BattleState& battle = this->battleState;

    if (battle.actingUnit != -1)
    {
        return 0;
    }

    u8 stoneRoles = 0;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];

        if (role.autoControl != 1 && role.guestControl != 1 && role.HasEffectBits(BattleUnit::UnkStatus4) == 1)
        {
            ++stoneRoles;
        }
    }

    const auto partyRoles = static_cast<u8>(
        static_cast<u8>(battle.roleCount) - static_cast<u8>(battle.escortCount) - static_cast<u8>(battle.guestCount)
    );

    if (stoneRoles != 0 && battle.defeatedRoles + stoneRoles >= partyRoles && battle.endFade == 0)
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

    if (stoneEnemies == 0 || battle.enemiesGone + stoneEnemies < static_cast<i32>(battle.enemyCount) ||
        battle.endFade != 0)
    {
        return 0;
    }

    battle.enemiesGone = static_cast<u8>(battle.enemiesGone + stoneEnemies);
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

    while (at < battle.slotRecords.size())
    {
        const i32 unit = battle.slotRecords[at].unit;

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

// swd32.exe 0x0045EB40: the units in draw order show their guard text and their damage numbers (once done damageShown
// is set and their slotTargets entry cleared: a role by its role index, so a role and the enemy of the same index share
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
            battle.damageShown = 1;
            battle.slotTargets[at] = -1;
        }

        if (!unit.ApplyPendingDamage(services))
        {
            continue;
        }

        battle.unk502984[at] = index;
        battle.RemoveSlotRecord(index);
    }
}

// swd32.exe 0x00465090: the next enemy takes its turn: enemy actingEnemy (0 past the enemies); a defeated one passes
// on through orderStep (1-based in enemyOrder, wrapping) for at most enemyCount tries, after which the battle state
// is 1. The enemy is marked (SetAC0 1) and aimed at (aimTarget, 1-based).
void Game::NextEnemyTurn()
{
    BattleState& battle = this->battleState;
    const auto count = static_cast<i32>(battle.enemyCount);

    if (battle.actingEnemy >= count)
    {
        battle.actingEnemy = 0;
    }

    if (this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated())
    {
        int tries = 0;

        while (true)
        {
            if (++battle.orderStep > count)
            {
                battle.orderStep = 1;
            }

            battle.actingEnemy = battle.enemyOrder[static_cast<std::size_t>(battle.orderStep - 1)];

            if (++tries >= count)
            {
                battle.result = kTurnsRunning;
                break;
            }

            if (!this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated())
            {
                break;
            }
        }
    }

    this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].SetAC0(1);
    battle.unk53BFB8 = 1;
    battle.aimTarget = battle.actingEnemy + 1;
}

// swd32.exe 0x00465170: the party role whose turn follows: orderStep steps (1-based, wrapping over the party roles)
// through RoleCycle until it names the current unit's role, which aimTarget then aims at (1-based). The original
// loops for ever when the cycle never names it.
void Game::NextRoleTurn()
{
    BattleState& battle = this->battleState;
    const auto roles = static_cast<i32>(battle.roleCount - battle.escortCount - battle.guestCount);
    i32 at = battle.orderStep;
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

    battle.orderStep = at;
    battle.aimTarget = role + 1;
    battle.actingEnemy = 0;
    battle.unk53BFB8 = 1;
}

// swd32.exe 0x00461900: the back key in a list (kBackSound): battle state 2 scrolls listScroll up by kListPage, states
// 4 and 0x1B roleListScroll (state 4 also keeps roleListRow and the scroll for role listRole); each first only marks
// its list (pageListRow, roleListRow) unless unk53BD9C.
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

        if (battle.unk53BD9C == 0 && battle.pageListRow != 1)
        {
            battle.pageListRow = 1;
            return;
        }

        scroll(battle.listScroll);
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

        if (battle.unk53BD9C == 0 && battle.roleListRow != 1)
        {
            battle.roleListRow = 1;
            return;
        }

        scroll(battle.roleListScroll);
        battle.unk53BFB8 = 1;
        return;
    }

    this->audio.PlaySound(kBackSound, this->config.soundVolume);

    if (battle.unk53BD9C == 0 && battle.roleListRow != 1)
    {
        battle.roleListRow = 1;
        return;
    }

    scroll(battle.roleListScroll);
    battle.unk53BFB8 = 1;
    const auto role = static_cast<std::size_t>(battle.listRole);
    battle.roleListRows[role] = battle.roleListRow;
    battle.roleListScrolls[role] = battle.roleListScroll;
}

// swd32.exe 0x00461A30: the forward key in a list (kBackSound): battle state 2 scrolls listScroll down by kListPage
// within listEntries entries (nothing below a page), states 4 and 0x1B roleListScroll within roleListEntries; each
// first only marks the last row of its page (pageListRow, roleListRow, at most the entries) unless unk53BD9C. A list
// shorter than a page goes back to 0 with the mark on its end. State 4, and the first mark of state 0x1B (the original
// shares that code), keep the mark and the scroll for role listRole.
void Game::ScrollListForward()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto keepForRole = [&battle](i32 mark)
    {
        const auto role = static_cast<std::size_t>(battle.listRole);
        battle.unk53BFB8 = 1;
        battle.roleListRows[role] = mark;
        battle.roleListScrolls[role] = battle.roleListScroll;
    };

    auto markPageEnd = [&battle, &keepForRole]
    {
        battle.roleListRow = std::min<i32>(kListPage, battle.roleListEntries);
        keepForRole(battle.roleListRow);
    };

    if (battle.result == kListState2)
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);

        if (static_cast<i8>(battle.listEntries) < kListPage)
        {
            return;
        }

        if (battle.unk53BD9C == 0 && battle.pageListRow != kListPage)
        {
            battle.pageListRow = kListPage;
            return;
        }

        battle.listScroll += kListPage;

        if (battle.listScroll + kListPage > static_cast<i8>(battle.listEntries))
        {
            battle.listScroll = static_cast<i8>(battle.listEntries) - kListPage;
        }

        if (battle.listScroll < 0)
        {
            battle.listScroll = 0;
            battle.pageListRow = static_cast<i8>(battle.listEntries);
        }

        battle.unk53BFB8 = 1;
        return;
    }

    if (battle.result != kListState4 && battle.result != kListState1B)
    {
        return;
    }

    this->audio.PlaySound(kBackSound, this->config.soundVolume);

    if (battle.unk53BD9C == 0 && battle.roleListRow != kListPage)
    {
        markPageEnd();
        return;
    }

    battle.roleListScroll += kListPage;

    if (battle.roleListScroll + kListPage > battle.roleListEntries)
    {
        battle.roleListScroll = battle.roleListEntries - kListPage;
    }

    if (battle.roleListScroll < 0)
    {
        battle.roleListScroll = 0;
        battle.roleListRow = battle.roleListEntries;
    }

    if (battle.result == kListState1B)
    {
        battle.unk53BFB8 = 1;
        return;
    }

    keepForRole(battle.roleListRow);
}

// The rebuild's auto combo (deviations.md): whether role unit `unit` attacks with 連擊 in the auto battle (the setting
// on, the role wearing its accessory with the stamina for it), as when it is chosen from the menu.
bool Game::AutoCombo(i32 unit)
{
    const i32 index = unit - BattleState::FirstRoleUnit;

    if (!this->config.autoCombo || index < 0 || static_cast<std::size_t>(index) >= this->battleRoles.size())
    {
        return false;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(index)];
    const bool wears = std::any_of(
        role.accessories.begin(),
        role.accessories.end(),
        [](const ObjectRecord& accessory)
        {
            return accessory.data.actionKind == kAimedCommand15;
        }
    );
    return wears && role.CanUseAccessory(kAimedCommand15) == 1;
}

// swd32.exe 0x0045D490: the auto battle takes the current role's turn (unless in battle state 3): with no remembered
// enemy (autoTarget) the state becomes 3; otherwise the role acts on it (aimTarget), or guards with MarkBit1 (its
// order taken as 1); a defeated enemy gives way to the first standing one, and with none the auto battle ends. The
// rebuild's auto combo attacks with 連擊 instead when it can (deviations.md).
void Game::AutoBattleTurn()
{
    BattleState& battle = this->battleState;

    if (battle.autoBattle != 1 || battle.currentUnit == 0 || battle.result == kTurnState)
    {
        return;
    }

    battle.turnCommand = 1;
    battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = 1;

    if (battle.autoTarget == -1)
    {
        battle.unk53BFB8 = 1;
        battle.result = kTurnState;
        return;
    }

    battle.turnUnit = battle.currentUnit;
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;
    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit)];
    role.SetTarget(1);
    battle.aimTarget = battle.autoTarget;
    battle.result = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 0;

    if (role.MarkBit1() == 1)
    {
        battle.turnCommand = kGuardOrder;
        battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = kGuardOrder;
        role.ResetAfterGuard();
        role.SetTarget(1);
        battle.pendingOrders[static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit)].unk00 = 1;
        return;
    }

    if (this->AutoCombo(battle.turnUnit))
    {
        battle.turnCommand = kAimedCommand15;
        battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = kAimedCommand15;
    }

    if (!this->battleEnemies[static_cast<std::size_t>(battle.aimTarget - 1)].IsDefeated())
    {
        return;
    }

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        if (!this->battleEnemies[enemy].IsDefeated())
        {
            battle.aimTarget = static_cast<i32>(enemy) + 1;
            return;
        }
    }

    role.SetTarget(0);
    battle.turnCommand = 0;
    battle.aimTarget = 1;
    battle.autoTarget = -1;
    battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = 0;
    battle.autoBattle = 0;
}

// swd32.exe 0x004624C0: whether role unit `unit` can take the current panel: it is in roleOrder (among the roleCount
// first) and not held
bool Game::CanSelectRole(int unit)
{
    BattleState& battle = this->battleState;

    for (u32 at = 0; at < battle.roleCount; ++at)
    {
        if (battle.roleOrder[at] == unit)
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

// swd32.exe 0x00462420: role unit `unit` takes the current panel (in battle state 0, or with anyStateRole set): when it
// is among the first roleCount - 1 of roleOrder and not held, it trades places with the current unit; the inputs of
// the panel (commandNames, listedCommands) are cleared either way.
void Game::SelectRoleUnit(int unit)
{
    BattleState& battle = this->battleState;

    if (battle.result != 0 && battle.anyStateRole == kNoPanelRole)
    {
        return;
    }

    if (battle.roleCount != 1)
    {
        for (u32 at = 0; at < battle.roleCount - 1; ++at)
        {
            const i32 listed = battle.roleOrder[at];

            if (listed != 0 && listed == unit &&
                this->battleRoles[static_cast<std::size_t>(listed - BattleState::FirstRoleUnit)].IsHeldOrHolding() != 1)
            {
                battle.roleOrder[at] = battle.currentUnit;
                battle.currentUnit = listed;
                break;
            }
        }
    }

    battle.commandNames = {};
    battle.listedCommands = 0;
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

// swd32.exe 0x00462510: the right key in the command menu (kBackSound): state 1 moves the command (menuCommand,
// 1-based) a column on within listedCommands + 5 entries, back when that one is not enabled; state 2 turns the page of
// the list (listPage, 0..2) to its top; state 4 goes to the next party role's list (listRole, its kept mark and
// scroll); state 0x1E raises roleListRow by 5 up to 10.
void Game::MenuRight()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    if (battle.result == kMenuState)
    {
        i32 command = battle.menuCommand;

        if (command + kMenuColumn <= battle.listedCommands + kMenuFixedCommands)
        {
            command += kMenuColumn;
            battle.menuCommand = command;
        }

        if (battle.commandEnabled[static_cast<std::size_t>(command - 1)] == 0)
        {
            battle.menuCommand = command - kMenuColumn;
        }

        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState2)
    {
        if (++battle.listPage >= kListPages)
        {
            battle.listPage = 0;
        }

        battle.pageListRow = 1;
        battle.listScroll = 0;
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState4)
    {
        if (++battle.listRole >= static_cast<i32>(Party::Roles))
        {
            battle.listRole = 0;
        }

        const auto role = static_cast<std::size_t>(battle.listRole);
        battle.roleListRow = battle.roleListRows[role];
        battle.roleListScroll = battle.roleListScrolls[role];
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kWaitState1E)
    {
        battle.roleListRow = std::min(battle.roleListRow + kMenuStep, kMenuStepMost);
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }
}

// swd32.exe 0x00462630: the left key in the command menu: state 1 moves the command a column back down to 1 (on again
// when that one is not enabled); state 2 turns the page back (wrapping to 2, the scroll kept); state 4 goes to the
// previous party role's list; state 0x1E lowers roleListRow by 5 down to 1.
void Game::MenuLeft()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    if (battle.result == kMenuState)
    {
        i32 command = battle.menuCommand;

        if (command - kMenuColumn >= 1)
        {
            command -= kMenuColumn;
            battle.menuCommand = command;
        }

        if (battle.commandEnabled[static_cast<std::size_t>(command - 1)] == 0)
        {
            battle.menuCommand = command + kMenuColumn;
        }

        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState2)
    {
        if (--battle.listPage < 0)
        {
            battle.listPage = kListPages - 1;
        }

        battle.pageListRow = 1;
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kListState4)
    {
        if (--battle.listRole < 0)
        {
            battle.listRole = static_cast<i32>(Party::Roles) - 1;
        }

        const auto role = static_cast<std::size_t>(battle.listRole);
        battle.roleListRow = battle.roleListRows[role];
        battle.roleListScroll = battle.roleListScrolls[role];
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    }

    if (battle.result == kWaitState1E)
    {
        battle.roleListRow = std::max(battle.roleListRow - kMenuStep, 1);
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
    battle.listedCommands = 0;
    const int index = battle.currentUnit - BattleState::FirstRoleUnit;
    std::size_t listed = 0;

    auto add = [&battle, &listed](u16 kind, std::u8string_view name)
    {
        battle.commandEnabled[kFixedCommands + listed] = 1;
        battle.commandKinds[listed] = kind;
        battle.commandNames[listed] = name;
        ++listed;
        battle.listedCommands = static_cast<u16>(listed);
    };

    auto addAccessories = [this, &add](std::size_t role)
    {
        for (std::size_t slot = 0; slot < kAccessorySlots; ++slot)
        {
            const std::unique_ptr<ItemNode>& node =
                this->party.equipment[role * Party::SlotsPerRole + kFirstAccessorySlot + slot];

            // An empty slot holds the empty entry; a missing one is read through a null pointer in the original.
            const u16 kind = node != nullptr ? node->record.data.actionKind : u16{0};

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

        if (!this->CommandEnabled(battle.menuCommand))
        {
            battle.menuCommand = kGuardedCommand;
        }
    }
}

// swd32.exe 0x004651D0: the command menu of the current role at (x, y): the five commands (FixedCommandNames) in
// columns of four, light when enabled and dark otherwise, then the accessory commands in the second column below the
// first (dark, and disabled, when CanUseAccessory refuses); the entry `cursor` (1-based) is drawn again over a dark
// box. The original first enables command 4 for a role whose fury is full (BuildCommandMenu then sets it anew).
void Game::DrawCommandMenu(int x, int y, int cursor)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawCommandMenuAdvance(x, y, cursor, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00468FF0: the first party role (not forced, standing) whose accessory proficiency has just filled
// (FinishProficiency): its object read into lastObject (the description freed), its name kept in lastLearned for the
// message, learnedPending set and levelUpRole the role
void Game::AnnounceMastery()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];

        if (role.autoControl == 1 || role.guestControl == 1 || role.IsDefeated())
        {
            continue;
        }

        const u16 finished = role.FinishProficiency(this->party.fame.lists[Fame::ProficiencyList]);

        if (finished == 0)
        {
            continue;
        }

        this->database.LoadObject(battle.lastObject, finished);
        Database::FreeDescription(battle.lastObject);
        battle.learnedPending = 1;
        battle.lastLearned.name = ObjectName(battle.lastObject);
        battle.levelUpRole = static_cast<u8>(index);
        return;
    }
}

// swd32.exe 0x00461C10: the cancel key of the battle menu. In the auto battle it ends it (the aimed enemy unmarked,
// the menu closed). Otherwise by battle state: 1 closes the menu; 2 (a list) and 0x1B, 0x1E, 8 go back to the menu
// (its name buffer cleared, the role's command cancelled); 3 (choosing a target) goes back to the list the menu
// command opened (state 2, 4, 0x1B, 0x1E or 8 for commands 2, 3, 0x1B, 0x1E, 4; to the menu with command
// listReturnCommand for others), every unit unmarked and the portraits plain, the role's order cleared; 4 goes back to
// the menu; 5 to state 4; 7 closes the menu with the command back on 1.
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
        battle.listPanelFrames = 0;
        battle.menuGrow = 0;
    };

    auto clearList = [&battle]
    {
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.unk53BFC8 = 0;
        battle.unk53BFB0 = 0;
        battle.listName = {};
        battle.unk53C49C = 1;
        battle.listRows = kListRows;
    };

    if (battle.autoBattle == 1)
    {
        battle.result = 0;
        battle.unk53BFB8 = 0;
        this->battleEnemies[static_cast<std::size_t>(battle.aimTarget - 1)].SetAC0(0);
        battle.unk53BFC4 = 0;
        battle.commandNames[0] = {};
        battle.unk53BFC0 = 0;
        battle.commandNames[1] = {};
        battle.autoBattle = 0;
        battle.listedCommands = 0;
        battle.autoTarget = -1;
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
            battle.listedCommands = 0;
            battle.unk53BFB8 = 0;
            closePanels();
            battle.listRows = kListRows;
            battle.commandNames[2] = {};
            return;

        case kListState2:
            battle.result = kMenuState;
            battle.pageListRow = 1;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        case kTargetState:
        {
            if (battle.talkRole == kNoTalkRole)
            {
                battle.result = kMenuState;
                battle.listRows = kListRows;
                battle.unk53BFB0 = 0;
            }
            else
            {
                battle.orderedSkill = 0;
                battle.unk53BD48 = 0;
                battle.result = kTalkState;
            }

            battle.turnPointer = 1;
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

            switch (battle.menuCommand)
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
                    battle.menuCommand = battle.listReturnCommand;
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
            battle.roleListScroll = 0;
            battle.listRows = kListRows;
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
            battle.controlRows = 2;
            battle.controlCursor = 1;
            battle.menuCommand = 1;
            return;

        case kSkillListState:
            battle.result = kMenuState;
            battle.roleListRow = 1;
            battle.specialRow = 1;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        case kListState1B:
            battle.result = kMenuState;
            battle.roleListRow = 1;
            battle.menuCommand = battle.listReturnCommand;
            battle.roleListScroll = 0;
            clearList();
            role().CancelCommand();
            closePanels();
            return;

        case kWaitState1E:
            battle.result = kMenuState;
            battle.menuCommand = battle.listReturnCommand;
            battle.roleListRow = 1;
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
// left) and the open page's tab over them (frame 4 + page), the panel act restarted (listPanelFrames 10), a
// quarter-dark box with the panel frame below the tabs; menuGrow grows by 2 a call up to 7.
void Game::DrawListTabs(int x, int y)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawListTabsAdvance(x, y, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00465E50: the 投擲 list of the current role at (x, y): a darkened box with a title frame (投擲) and a
// body frame, then up to 7 listed objects from entry `top` + 1 with their counts ("%2d"); roleListEntries counts them.
// Row `cursor` (from 1) is drawn again a pixel up and left under a gradient bar, and its entry becomes listCursor.
void Game::DrawThrowList(int x, int y, int cursor, int top)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawThrowListAdvance(x, y, cursor, top, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00466190: the 煉符 grid of the current role at (x, y): two columns of five cells. The first cells, one
// per talisman paper (kThrownObject, their count kept in roleListEntries), show its name; then each throwable object
// once per piece; the rest 無. Cell `cursor` (from 1) gets a gradient bar and sets listCursor to the object under it
// (less 1 without paper). roleListEntries then adds the throwable pieces.
void Game::DrawRefineList(int x, int y, int cursor)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawRefineListAdvance(x, y, cursor, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00466500: the 絕招 list of the current role at (x, y): the panel act run on (not restarted), a darkened
// box with a title and a body frame, then up to 7 skills of type 1 (their names kept in listName, cut to its bytes:
// the original copies the whole name over what follows); specialCount counts them. Row `cursor` (from 1) is drawn again
// a pixel up and left under a gradient bar, and its skill index becomes specialCursor.
void Game::DrawSpecialList(int x, int y, int cursor)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawSpecialListAdvance(x, y, cursor, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x004667B0: the 佈置護駕 panel: the panel act run on, a shade with a title frame (佈置護駕) and a body
// frame, a gradient bar on row guardCursor at (x, y), and the names of the escorts (the last escortCount battle roles)
// in white at an 18-pixel advance, with 無 below a lone escort.
void Game::DrawEscortList(int x, int y)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawEscortListAdvance(x, y, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00466950: the name tag of the aimed enemy (aimTarget, 1-based) at (x, y), unless the current role's
// order is taken or unk53BF88: a darkened box and a frame as wide as the name (20 pixels per two bytes; from the right
// edge when the sides are swapped), the name in white. Once the bestiary counts 10 of it its hit points follow
// ("生命:%d/%d", to the left when swapped); from 15 its action gauge as a dark red bar below.
void Game::DrawTargetTag(int x, int y)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawTargetTagAdvance(x, y, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00466C00: the 控制 menu of the controlled enemy (controlledEnemy) at (x, y): a tile box with the title, a
// tile box of controlRows rows, then 攻擊, its objects by name, its skills as 特殊1 and 特殊2, and 釋放, 20 pixels a
// row. Entry `cursor` (from 1) is drawn again over a dark background; an object sets orderedSkill to its id (always the
// first object's, see ItemName) and a skill unk53BD48 to its number.
void Game::DrawControlMenu(int x, int y, int cursor)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawControlMenuAdvance(x, y, cursor, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x004655B0: the spells of the current role's open page (listPage) from entry `top` + 1 at (x, y), up to
// 7 rows (listEntries counts them): each with its element icon, its name (kept in listName) and its cost ("%3d"; with
// bit 15 a stamina cost), dark when the role has less than it. Row `cursor` (from 1) is drawn again a pixel up and
// left under a gradient bar, with the spell's description centred in a bar at the bottom; its index becomes
// specialCursor.
void Game::DrawSpellList(int x, int y, int cursor, int top)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawSpellListAdvance(x, y, cursor, top, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x004659C0: the object list of category listRole at (x, y): its four page tabs (frames 3..0 right to
// left, the open one frame 4 + category), the panel act restarted, a quarter-dark box and a frame; then from entry
// `top` + 1 up to 7 shown objects (roleListEntries counts them) with their counts ("%2d"), dark when too costly. Row
// `cursor` (from 1) is drawn again a pixel up and left under a gradient bar, its entry becomes listCursor, and the
// object's description is centred in a bar at the bottom.
void Game::DrawItemList(int x, int y, int cursor, int top)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawItemListAdvance(x, y, cursor, top, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00467710: the 戰鬥勝利 panel. The first time (the gold not yet given, bit 15 of unk53BF12): the music
// and the sounds stop, kVictorySound plays; each enemy's drop is added to the inventory as pending and to the loot
// list (a new object at the end, at most LootSlots of them); each party role standing (not forced) below the level
// cap (timingWindow) gains unk53BF14 experience, every worn accessory unk53BF16 proficiency (accessoryGained when one
// counts), its battle count 1 (2 against three enemies or more), its effects end and its main act restarts; then the
// gold is given. Once the panel has grown, the experience, the gold and the proficiency are shown.
void Game::DrawVictoryPanel()
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawVictoryPanelAdvance(scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00467AC0: the 升級 panel while a role levels up (levelUpRole) or a skill was learned (learnedPending): a
// shade with a title and a body frame; once the panel has grown, "<name>升第<level>級" of the role that levelled up.
void Game::DrawLevelUpPanel()
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawLevelUpPanelAdvance(scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00467C50: the first party role standing (not forced) with the experience of its next level (below the
// level cap) levels up: the record is kept in levelUpBefore, the next level's maxima and attributes (LevelUp of both
// levels; a skill learned sets learnedPending) are added to it as the difference, hit points, spirit and stamina
// refilled, its level and experience mark taken; levelUpRole becomes the role and the level-up sound plays. Otherwise
// levelUpRole is kNoLevelUp. unk53C4C8 is set either way.
void Game::CheckLevelUps()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        const BattleRole& unit = this->battleRoles[index];

        if (unit.autoControl == 1 || unit.guestControl == 1)
        {
            continue;
        }

        const auto role = battle.partyRoles[index];
        RoleRecord& record = this->party.roles[static_cast<std::size_t>(role)];
        const int next = record.level + 1;
        u32 needed = 0;

        if (record.level >= battle.timingWindow ||
            (this->database.LoadLevelEntry(role + 1, next, needed), record.totalExperience < static_cast<i32>(needed)))
        {
            battle.levelUpRole = kNoLevelUp;
            continue;
        }

        RoleRecord before;
        RoleRecord after;
        battle.levelUpBefore = record;
        LevelUp(
            before,
            role + 1,
            record.level,
            battle.learnedPending,
            this->party,
            this->database,
            battle.lastLearned,
            this->errors
        );
        LevelUp(
            after,
            role + 1,
            next,
            battle.learnedPending,
            this->party,
            this->database,
            battle.lastLearned,
            this->errors
        );
        record.level = after.level;
        record.experience = after.experience;
        record.maxHitPoints = static_cast<i16>(record.maxHitPoints + after.maxHitPoints - before.maxHitPoints);
        record.hitPoints = record.maxHitPoints;
        record.maxSpirit = static_cast<i16>(record.maxSpirit + after.maxSpirit - before.maxSpirit);
        record.spirit = record.maxSpirit;
        record.maxStamina = static_cast<i16>(record.maxStamina + after.maxStamina - before.maxStamina);
        record.stamina = record.maxStamina;
        record.strength = static_cast<i16>(record.strength + after.strength - before.strength);
        record.endurance = static_cast<i16>(record.endurance + after.endurance - before.endurance);
        record.wisdom = static_cast<i16>(record.wisdom + after.wisdom - before.wisdom);
        record.agility = static_cast<i16>(record.agility + after.agility - before.agility);
        record.rapport = static_cast<i16>(record.rapport + after.rapport - before.rapport);
        record.reaction = static_cast<i16>(record.reaction + after.reaction - before.reaction);
        record.ailmentChance = static_cast<i16>(record.ailmentChance + after.ailmentChance - before.ailmentChance);
        record.evasion = static_cast<i16>(record.evasion + after.evasion - before.evasion);
        battle.levelUpRole = static_cast<u8>(index);
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

// The box of 0x00468930 and 0x00468AD0 (inline in each): while a skill was learned (learnedPending), the line with the
// role's name in a box as wide as it (20 pixels per two bytes), grown open; then the line in white and the skill's
// name in brackets, centred below it on a dark background.
void Game::DrawLearnedBox(std::u8string_view line, bool sound)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawLearnedBoxAdvance(line, sound, scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00468C80: each battle role standing (not forced) whose next special skill is not exhausted (battle
// block [3] not -1) learns it once it has fought enough battles: its type 1 skills (from Party::skills by battle role
// index, as in the original) set the battle block's needed battles ([2], the skill's counterMax) and next special
// ([3], its masteryValue with bit 31). With [1] battles at least [2] and a next special (not 0x665 or 0x669 while game
// flag kSpecialsFlag is set), the special is appended to the list, its name kept for the message (learnedPending,
// levelUpRole the role) and the block restarted from it (-1 without a further one).
void Game::LearnSpecials()
{
    BattleState& battle = this->battleState;

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& role = this->battleRoles[index];
        RoleBattleBlock& block = this->party.battleBlocks[static_cast<std::size_t>(battle.partyRoles[index])];
        std::unique_ptr<ItemNode>& list = this->party.skills[index];

        if (role.autoControl == 1 || role.guestControl == 1 || role.IsDefeated() ||
            block.values[kNextSpecial] == kNoSpecial || list == nullptr)
        {
            continue;
        }

        auto takeNext = [this, &battle, &block](u16 id)
        {
            this->database.LoadObject(battle.lastObject, id);
            Database::FreeDescription(battle.lastObject);
            block.values[kNeededBattles] = battle.lastObject.data.counterMax;
            block.values[kNextSpecial] = static_cast<i32>(battle.lastObject.data.masteryValue | kSpecialMark);
        };

        for (ItemNode* node = list->next.get(); node != nullptr; node = node->next.get())
        {
            if (node->record.data.category == kSpecialType)
            {
                takeNext(node->id);
            }
        }

        const auto next = static_cast<u32>(block.values[kNextSpecial]) & kSpecialIdMask;

        if (block.values[kBattlesFought] < block.values[kNeededBattles] || next == 0)
        {
            continue;
        }

        if (this->state.GameFlag(kSpecialsFlag) && (next == kBarredSpecialA || next == kBarredSpecialB))
        {
            continue;
        }

        const auto id = static_cast<u16>(block.values[kNextSpecial]);
        this->database.LoadObject(battle.lastObject, id);
        Database::FreeDescription(battle.lastObject);
        ItemNode* last = list.get();

        while (last->next != nullptr)
        {
            last = last->next.get();
        }

        last->next = std::make_unique<ItemNode>();
        ItemNode& learned = *last->next;
        learned.id = id;
        this->database.LoadObject(learned.record, id);
        battle.learnedPending = 1;
        battle.lastLearned.name = ObjectName(learned.record);
        battle.levelUpRole = static_cast<u8>(index);
        block.values[kBattlesFought] = 0;
        block.values[kNeededBattles] = battle.lastObject.data.counterMax;
        block.values[kNextSpecial] = static_cast<i32>(battle.lastObject.data.masteryValue | kSpecialMark);

        if (battle.lastObject.data.masteryValue == 0)
        {
            block.values[kNextSpecial] = kNoSpecial;
        }
    }
}

// swd32.exe 0x00467F00: the stats of the role that levelled up (levelUpRole): its name over a frame of the seven values
// before the level-up (levelUpBefore); once grown, after kRollStart frames (levelUpFrames, kShowNow shows at once) each
// value that rose shows " ==> " and the new value less a roll that counts down a point a frame (kRollSound each point),
// one after another: the maxima first, then the attributes (strength does not wait for the maxima, as in the original).
void Game::DrawLevelUpStats()
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawLevelUpStatsAdvance(scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x00460C40: the up key of the battle menu, by battle state: 1 moves the command up (skipping disabled
// ones, wrapping to the last accessory command; past 8 it restarts at 1); 2 and 0x1B move the list cursor up, then
// scroll; 4 too, keeping the list for role listRole; 5 cycles 2..1; 7 and 8 cycle their entries; 0x1E cycles 10..1.
// 3 (choosing a target, unless unk53BF88) moves the aim: an order on the party goes back through RoleCycle (four
// party roles or more) or the 1-based roles (fewer), skipping forced and guarding roles; otherwise back through the
// standing enemies. The aimed unit is marked, the others unmarked.
void Game::MenuUp()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto tick = [this]
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    };

    switch (battle.result)
    {
        case kMenuState:
        {
            if (battle.menuCommand > kLastMenuCommand)
            {
                battle.menuCommand = 1;
                return;
            }

            tick();
            i32 command = battle.menuCommand - 1;
            battle.menuCommand = command;

            if (this->CommandEnabled(command))
            {
                battle.unk53BFB8 = 1;
                return;
            }

            do
            {
                if (--command < 1)
                {
                    command = battle.listedCommands + kMenuFixedCommands;
                }
            } while (!this->CommandEnabled(command));

            battle.unk53BFB8 = 1;
            battle.menuCommand = command;
            return;
        }

        case kListState2:
            if (--battle.pageListRow >= 1)
            {
                tick();
                battle.unk53BFB8 = 1;
                return;
            }

            battle.pageListRow = 1;

            if (battle.listScroll > 0)
            {
                tick();
            }

            battle.unk53BFB8 = 1;

            if (--battle.listScroll < 0)
            {
                battle.listScroll = 0;
            }

            return;

        case kTargetState:
            if (battle.unk53BF88 == 1)
            {
                return;
            }

            tick();
            this->AimStep(-1);
            return;

        case kListState4:
            if (--battle.roleListRow < 1)
            {
                battle.roleListRow = battle.roleListEntries != 0 ? 1 : 0;

                if (--battle.roleListScroll < 0)
                {
                    battle.roleListScroll = 0;
                }
            }

            tick();
            battle.unk53BFB8 = 1;
            battle.roleListRows[static_cast<std::size_t>(battle.listRole)] = battle.roleListRow;
            battle.roleListScrolls[static_cast<std::size_t>(battle.listRole)] = battle.roleListScroll;
            return;

        case kWaitState5:
            tick();

            if (--battle.guardCursor < 1)
            {
                battle.guardCursor = kEscortRows;
            }

            return;

        case kTalkState:
            battle.orderedSkill = 0;
            battle.unk53BD48 = 0;
            tick();

            if (--battle.controlCursor < 1)
            {
                battle.controlCursor = battle.controlRows;
            }

            return;

        case kSkillListState:
            if (battle.specialCount == 0)
            {
                return;
            }

            if (--battle.specialRow < 1)
            {
                battle.specialRow = static_cast<i8>(battle.specialCount);
            }

            tick();
            battle.unk53BFB8 = 1;
            return;

        case kListState1B:
            if (--battle.roleListRow >= 1)
            {
                tick();
                battle.unk53BFB8 = 1;
                return;
            }

            battle.roleListRow = battle.roleListEntries != 0 ? 1 : 0;
            battle.unk53BFB8 = 1;

            if (--battle.roleListScroll < 0)
            {
                battle.roleListScroll = 0;
            }

            return;

        case kWaitState1E:
            if (--battle.roleListRow < 1)
            {
                battle.roleListRow = kMenuStepMost;
            }

            tick();
            battle.unk53BFB8 = 1;
            return;

        default:
            return;
    }
}

// The command `command` (1-based) of the menu is enabled; command 0 reads the byte before the table in the original
// (taken as disabled here).
bool Game::CommandEnabled(i32 command) const
{
    const auto at = static_cast<std::size_t>(command - 1);
    return command >= 1 && at < this->battleState.commandEnabled.size() && this->battleState.commandEnabled[at] != 0;
}

// A 1-based place moved by `step`, wrapping at the end it moves towards only (0x00460C40: below 1 to `count`;
// 0x00461240: past `count` to 1). A place left past the other end by another count stays there.
void Game::StepWrapped(i32& place, int step, i32 count)
{
    place += step;

    if (step < 0 && place < 1)
    {
        place = count;
    }
    else if (step > 0 && place > count)
    {
        place = 1;
    }
}

// The aim of MenuUp (`step` -1) and MenuDown (1) in battle state 3 (inline in each); see MenuUp.
void Game::AimStep(int step)
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    UnitRect rect;
    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);

    auto markRole = [this, &battle]
    {
        for (std::size_t panel = 0; panel < static_cast<std::size_t>(Party::Roles); ++panel)
        {
            this->battleRoles[panel].SetAC0(0);
            this->battleHud.portraitHighlight[panel] = 0;
        }

        this->battleRoles[static_cast<std::size_t>(battle.aimTarget - 1)].SetAC0(1);
        battle.unk53BFB8 = 1;
        battle.unk53C49C = 1;
    };

    // The original checks role `role` - 1 (role 0: the memory before the roles; here neither forced nor guarding).
    auto selectable = [this](i32 role)
    {
        if (role < 1)
        {
            return true;
        }

        const BattleRole& checked = this->battleRoles[static_cast<std::size_t>(role - 1)];
        return checked.autoControl != 1 && checked.guestControl != 1 && !checked.IsGuarding();
    };

    if (battle.pendingOrders[unit].unk00 != 0)
    {
        const auto partyRoles = static_cast<i32>(battle.roleCount - battle.escortCount - battle.guestCount);

        if (partyRoles >= static_cast<i32>(Party::Roles))
        {
            int tries = 0;

            while (true)
            {
                StepWrapped(battle.orderStep, step, partyRoles);

                if (++tries > static_cast<int>(BattleState::RoleCycle.size()) * kHangTries ||
                    battle.orderStep >= static_cast<i32>(BattleState::RoleCycle.size()))
                {
                    Hang();
                }

                battle.actingEnemy = BattleState::RoleCycle[static_cast<std::size_t>(battle.orderStep)];

                if (selectable(battle.actingEnemy))
                {
                    break;
                }
            }

            this->battleRoles[static_cast<std::size_t>(battle.actingEnemy)].GetShadowRect(services, rect);
            battle.aimTarget = battle.actingEnemy + 1;
            battle.actingEnemy = 0;
            markRole();
            return;
        }

        int tries = 0;

        do
        {
            StepWrapped(battle.aimTarget, step, static_cast<i32>(battle.roleCount));

            if (++tries > static_cast<int>(BattleState::RoleUnits) * kHangTries)
            {
                Hang();
            }
        } while (!selectable(battle.aimTarget));

        this->battleRoles[static_cast<std::size_t>(battle.aimTarget - 1)].GetShadowRect(services, rect);
        markRole();
        return;
    }

    StepWrapped(battle.orderStep, step, static_cast<i32>(battle.enemyCount));
    int tries = 0;

    for (battle.actingEnemy = battle.enemyOrder[static_cast<std::size_t>(battle.orderStep - 1)];
         this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated();
         battle.actingEnemy = battle.enemyOrder[static_cast<std::size_t>(battle.orderStep - 1)])
    {
        StepWrapped(battle.orderStep, step, static_cast<i32>(battle.enemyCount));

        // The original loops for ever when every enemy is down.
        if (++tries > BattleState::Enemies)
        {
            Hang();
        }
    }

    this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].GetShadowRect(services, rect);

    for (u32 enemy = 0; enemy < battle.enemyCount; ++enemy)
    {
        this->battleEnemies[enemy].SetAC0(0);
    }

    this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].SetAC0(1);
    battle.unk53BFB8 = 1;
    battle.unk53C49C = 1;
    battle.aimTarget = battle.actingEnemy + 1;
}

// swd32.exe 0x00461240: the down key of the battle menu, as MenuUp the other way: 1 moves the command down (past the
// last accessory command to 1); 2, 4 and 0x1B move the list cursor down to the seventh row, then scroll, stopping at
// the end of the list (state 4 keeps the list for role listRole); 5 cycles 1..2; 7, 8 and 0x1E cycle forward;
// 3 moves the aim forward.
void Game::MenuDown()
{
    BattleState& battle = this->battleState;
    battle.unk53BFBC = 0;

    auto tick = [this]
    {
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
    };

    // The list cursor of states 2, 4 and 0x1B (inline in the original): false when it stopped at the end.
    auto listDown = [](i32& cursor, i32& scroll, i32 entries)
    {
        if (++cursor <= kListRowsShown)
        {
            if (cursor <= entries)
            {
                return true;
            }

            cursor = entries;
            return false;
        }

        cursor = kListRowsShown;

        if (++scroll + kListRowsShown <= entries)
        {
            return true;
        }

        scroll = entries - kListRowsShown;

        return false;
    };

    switch (battle.result)
    {
        case kMenuState:
        {
            if (battle.menuCommand > kLastMenuCommand)
            {
                battle.menuCommand = 1;
                return;
            }

            tick();
            i32 command = battle.menuCommand + 1;
            battle.menuCommand = command;

            if (this->CommandEnabled(command))
            {
                battle.unk53BFB8 = 1;
                return;
            }

            do
            {
                if (++command > battle.listedCommands + kMenuFixedCommands)
                {
                    command = 1;
                }
            } while (!this->CommandEnabled(command));

            battle.unk53BFB8 = 1;
            battle.menuCommand = command;
            return;
        }

        case kListState2:
            if (listDown(battle.pageListRow, battle.listScroll, static_cast<i8>(battle.listEntries)))
            {
                tick();
            }

            battle.unk53BFB8 = 1;
            return;

        case kTargetState:
            if (battle.unk53BF88 == 1)
            {
                return;
            }

            tick();
            this->AimStep(1);
            return;

        case kListState4:
            if (listDown(battle.roleListRow, battle.roleListScroll, battle.roleListEntries))
            {
                tick();
            }

            battle.roleListRows[static_cast<std::size_t>(battle.listRole)] = battle.roleListRow;
            battle.roleListScrolls[static_cast<std::size_t>(battle.listRole)] = battle.roleListScroll;
            battle.unk53BFB8 = 1;
            return;

        case kWaitState5:
            tick();

            if (++battle.guardCursor > kEscortRows)
            {
                battle.guardCursor = 1;
            }

            return;

        case kTalkState:
            battle.orderedSkill = 0;
            battle.unk53BD48 = 0;
            tick();

            if (++battle.controlCursor > battle.controlRows)
            {
                battle.controlCursor = 1;
            }

            return;

        case kSkillListState:
            if (battle.specialCount == 0)
            {
                return;
            }

            if (++battle.specialRow > static_cast<i8>(battle.specialCount))
            {
                battle.specialRow = 1;
            }

            tick();
            battle.unk53BFB8 = 1;
            return;

        case kListState1B:
            if (listDown(battle.roleListRow, battle.roleListScroll, battle.roleListEntries))
            {
                tick();
            }

            battle.unk53BFB8 = 1;
            return;

        case kWaitState1E:
            if (++battle.roleListRow > kMenuStepMost)
            {
                battle.roleListRow = 1;
            }

            tick();
            battle.unk53BFB8 = 1;
            return;

        default:
            return;
    }
}

// swd32.exe 0x0045D8F0: the debug keys of the battle (with the debug mode). With a Ctrl key: F3 stops the music, F1
// and X toggle unk5244E0 and unk53AF68, K starts a strong attack (強攻擊), Z refills the roles (vitals, fury and
// strong attributes), D and F hurt the roles, V the enemies, F9 toggles skipping the talk, E ends the frame (0), C
// ends the spell and the acting unit (the original then resets the turn of role actingUnit - 8 with actingUnit just set
// to -1: not done here), F5 restarts the battle music, F2 toggles the absolute spirit pact, W defeats the enemies.
// Without Ctrl: H and J move every unit and the background 10 pixels. P toggles the snapshot in any mode. 1 otherwise.
int Game::BattleDebugKeys()
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    Input& input = this->input;

    auto pressed = [&input](int key)
    {
        return input.IsKeyDown(key) != 0;
    };

    auto pause = []
    {
        SleepMilliseconds(kDebugPause);
    };

    if (this->config.debugMode == 1)
    {
        if (pressed(Input::LeftControl) || pressed(Input::RightControl))
        {
            if (pressed(Input::F3))
            {
                this->audio.StopMusic();
            }

            if (pressed(Input::F1))
            {
                pause();
                battle.unk5244E0 = battle.unk5244E0 == 0 ? 1 : 0;
            }

            if (pressed(Input::X))
            {
                pause();
                battle.unk53AF68 = battle.unk53AF68 == 0 ? 1 : 0;
            }

            if (pressed(Input::K))
            {
                if (battle.criticalAttack == 0)
                {
                    battle.criticalAttack = 1;
                }

                this->battleMessages
                    .Add(kDebugMessageX, kDebugMessageY, kDebugMessageFrames, kStrongAttack, BattleMessage::Centered);
            }

            if (pressed(Input::Z))
            {
                pause();

                for (u32 index = 0; index < battle.roleCount; ++index)
                {
                    BattleRole& role = this->battleRoles[index];
                    role.SetVitals(kRefill, kRefill, kRefill);
                    role.AddFury(services, kFillFury);
                }

                for (u32 index = 0; index < battle.roleCount; ++index)
                {
                    this->battleRoles[index].SetAttributes(kDebugAttack, kDebugDefense, kDebugAgility);
                }
            }

            auto hurtRoles = [this, &battle, &services](int hitPoints, i16 drain)
            {
                for (u32 index = 0; index < battle.roleCount; ++index)
                {
                    BattleRole& role = this->battleRoles[index];

                    if (role.autoControl != 1 && role.guestControl != 1)
                    {
                        role.Damage(services, hitPoints, drain, drain);
                    }
                }
            };

            if (pressed(Input::D))
            {
                pause();
                hurtRoles(kSmallHurt, kSmallDrain);
            }

            if (pressed(Input::F))
            {
                SleepMilliseconds(kDebugShortPause);
                hurtRoles(kBigHurt, kBigDrain);
            }

            if (pressed(Input::V))
            {
                pause();

                for (u32 index = 0; index < battle.enemyCount; ++index)
                {
                    this->battleEnemies[index].Damage(services, kEnemyHurt, 0, 0);
                }
            }

            if (pressed(Input::F9))
            {
                pause();
                this->talk.skipAll = (this->talk.skipAll + 1) % kToggleStates;
            }

            if (pressed(Input::E))
            {
                return 0;
            }

            if (pressed(Input::C))
            {
                battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellOver);
                battle.unk53BF74 = 0;
                battle.ClearActing();
                battle.unk53BFC0 = 0;
                battle.unk53BFC4 = 0;
                battle.actingUnit = -1;

                if (battle.unk53BF64 == 1)
                {
                    battle.unk53BF64 = 0;
                }

                if (battle.hitsAllRoles == 1)
                {
                    battle.hitsAllRoles = 0;
                }
            }

            if (pressed(Input::F5))
            {
                this->audio.StopMusic();
                this->PlayMusic(battle.battleMusic);
            }

            if (pressed(Input::F2))
            {
                pause();

                if (battle.unk53C02C == 1)
                {
                    battle.panelFlags &= ~BattleState::UnkPanel2;
                    battle.unk53C02C = 0;
                    this->battleMessages
                        .Add(kDebugMessageX, kDebugMessageY, kDebugMessageFrames, kNormalPact, BattleMessage::Centered);
                }
                else
                {
                    battle.panelFlags |= BattleState::UnkPanel2;
                    battle.unk53C02C = 1;
                    this->battleMessages.Add(
                        kDebugMessageX,
                        kDebugMessageY,
                        kDebugMessageFrames,
                        kAbsolutePact,
                        BattleMessage::Centered
                    );
                }
            }

            if (pressed(Input::W))
            {
                for (u32 index = 0; index < battle.enemyCount; ++index)
                {
                    BattleEnemy& enemy = this->battleEnemies[index];

                    if (!enemy.IsDefeated())
                    {
                        battle.unk502984[index] = static_cast<i32>(index);
                        battle.slotDamage[index] = 0;
                        enemy.Damage(services, kDefeatHurt, 0, 0);
                    }
                }

                battle.unk53BD88 = static_cast<i32>(battle.enemyCount);
                battle.unk53BF80 = 1;
                battle.unk53BFC0 = 1;
                battle.unk53BFC4 = 1;
                battle.roleOrder = {};
                battle.unk53AF30 = 0;
                battle.unk53AF34 = 0;
                std::fill_n(battle.turnTaken.begin(), kClearedTurnUnits, 0);
                battle.slotRecords.fill(BattleSlotRecord{});
                battle.unk4A7626 = kNoTalkRole;
                battle.unk53BD80 = 0;
                battle.unk53BF64 = 0;
                battle.unk53BF5C = 0;
                battle.turnUnit = 0;
                battle.currentUnit = 0;
                battle.actingUnit = -1;
                battle.result = 0;
            }
        }

        auto moveAll = [this, &battle](i16 step)
        {
            for (u32 index = 0; index < battle.roleCount; ++index)
            {
                this->battleRoles[index].Move(step, 0);
            }

            for (u32 index = 0; index < battle.enemyCount; ++index)
            {
                this->battleEnemies[index].Move(step, 0);
            }

            battle.shake = step;
        };

        if (pressed(Input::H))
        {
            moveAll(kDebugStep);
        }

        if (pressed(Input::J))
        {
            moveAll(-kDebugStep);
        }
    }

    if (pressed(Input::P))
    {
        battle.snapNext = battle.snapNext != 1 ? 1 : 0;
    }

    return 1;
}

// swd32.exe 0x0045DEE0: the debug overlay (unk5244E0): each enemy's hit points, lock (GetWord2A74), command and
// level, each role's lock and command, the slot records (攻擊順序), the party order (我方等待順序) and the draw order
// (排列順序), the acting and current units, each enemy's action gauge as a two-row bar of kGaugePixel at its place,
// and the state values (無敵, UM, 戰鬥, the menu, the counts, the aim, the locks, the picture cache, the frame).
void Game::DrawBattleDebug()
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawBattleDebugAdvance(scene);
    scene.Show(this->battleServices);
}

// swd32.exe 0x0045FC60: the pointer over the battle (only when it moved, or while unk53BFBC): the click regions of
// a talk choose Talk::choice; then by battle state: 0 and 1 (outside the menu) find the party panel under it
// (anyStateRole); 1 picks the menu command under it (an accessory command checked with the kind words after
// commandKinds, as in the original); 2, 4 and 0x1B pick a page tab and a list row, or the scroll arrows, track and
// thumb (hoverScrollPart 1..4); 3 aims at the unit whose picture is under it (or the party portrait); 5, 8 and 0x1E
// pick a row or a cell. 1 when something is under the pointer.
int Game::MouseHover()
{
    BattleState& battle = this->battleState;
    const int x = this->state.pointerX;
    const int y = this->state.pointerY;

    if (battle.pointerX == x && battle.pointerY == y)
    {
        if (battle.unk53BFBC == 0)
        {
            return 0;
        }
    }
    else
    {
        battle.unk53BFBC = 1;
        battle.promptFrames = 0;
    }

    battle.pointerX = x;
    battle.pointerY = y;

    if (!this->state.clickRegions.regions.empty())
    {
        const int index = this->state.clickRegions.Find(x, y, battle.hoverRegion);

        if (battle.hoverRegion != nullptr)
        {
            this->talk.choice = index;
        }
    }

    // The pointer is compared unsigned throughout (jbe/jnb in the original).
    auto inside = [](int value, int low, int high)
    {
        return static_cast<u32>(value) > static_cast<u32>(low) && static_cast<u32>(value) < static_cast<u32>(high);
    };

    // The step of a hit test over bands (inline in the original): the band of `value` among `bands` bands of `size`
    // ending at `first`, `first` + `size`, ...; -1 outside them.
    auto band = [&inside](int value, int first, int size, int end)
    {
        int found = 0;

        for (int edge = first; edge < end; edge += size)
        {
            if (inside(value, edge - size, edge))
            {
                return found;
            }

            ++found;
        }

        return -1;
    };

    auto panelUnder = [&battle, x, y, &inside](bool lastWins)
    {
        if (!inside(y, kPanelsTop, Display::Height))
        {
            return false;
        }

        bool found = false;

        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            const int left = BattleState::PortraitX[static_cast<std::size_t>(battle.partyRoles[index])];

            if (inside(x, left - kPanelLeftMargin, left + kPanelWidth))
            {
                battle.anyStateRole = static_cast<u16>(index + BattleState::FirstRoleUnit);
                found = true;

                if (!lastWins)
                {
                    return true;
                }
            }
        }

        return found;
    };

    auto scrollParts = [this, &battle, x, y, &inside](int left, int right, int downTop, int downBottom)
    {
        const BattleHud& hud = this->battleHud;

        if (!inside(x, left, right))
        {
            battle.hoverScrollPart = 0;
            battle.unk53BD9C = 0;
            battle.unk53BFB8 = 0;
            return 0;
        }

        int part = 0;

        if (inside(y, kScrollUpTop, kScrollUpBottom))
        {
            part = kScrollUp;
        }
        else if (inside(y, downTop, downBottom))
        {
            part = kScrollDown;
        }
        else if (inside(y, hud.scrollTrackTop, hud.scrollThumbTop))
        {
            part = kScrollAbove;
        }
        else if (
            static_cast<u32>(x) < static_cast<u32>(right) && inside(y, hud.scrollThumbBottom, hud.scrollTrackBottom)
        )
        {
            part = kScrollBelow;
        }

        if (part == 0)
        {
            battle.hoverScrollPart = 0;
            battle.unk53BD9C = 0;
            battle.unk53BFB8 = 0;
            return 0;
        }

        battle.unk53BFB8 = 1;
        battle.hoverScrollPart = part;
        return 1;
    };

    auto pickRow = [this, &battle](i32& cursor, int row)
    {
        if (cursor != row + 1)
        {
            this->audio.PlaySound(kBackSound, this->config.soundVolume);
        }

        cursor = row + 1;
        battle.hoverScrollPart = 0;
        battle.unk53BFB8 = 1;
        battle.unk53BFBC = 1;
        return 1;
    };

    auto leaveList = [this, &battle]
    {
        battle.unk53BFB8 = 0;

        if (this->controls.buttons[static_cast<std::size_t>(Button::MouseLeft)].heldFrames >= 1)
        {
            return false;
        }

        battle.unk53BD9C = 0;
        battle.unk53BD6C = 0;
        battle.unk53BD70 = 0;
        battle.unk53BFF8 = 0;
        return battle.roleListEntries > kListRowsShown;
    };

    switch (battle.result)
    {
        case 0:
            if (battle.enemiesGone >= static_cast<i32>(battle.enemyCount))
            {
                return 0;
            }

            if (panelUnder(false))
            {
                return 1;
            }

            battle.anyStateRole = kNoPanelRole;
            return 0;

        case kMenuState:
        {
            if (battle.currentUnit == 0)
            {
                return 0;
            }

            const int left = battle.menuX;
            const int top = battle.menuY;

            if (inside(x, left + kMenuHitLeft, left + kMenuHitRight) &&
                inside(y, top + kMenuHitTop, top + kMenuHitBottom))
            {
                const auto index = static_cast<int>(
                    static_cast<u32>(y - top - kMenuHitTop) / kMenuRowHeight +
                    kMenuColumn * (static_cast<u32>(x - left - kMenuHitLeft) / kMenuHitColumn)
                );

                if (index < battle.listedCommands + kMenuFixedCommands)
                {
                    BattleRole& role =
                        this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];

                    if (battle.commandEnabled[static_cast<std::size_t>(index)] == 0 ||
                        (index >= kMenuColumn &&
                         (role.CanUseAccessory(battle.MenuKindWord(static_cast<std::size_t>(index))) == 0 ||
                          battle.commandEnabled[static_cast<std::size_t>(index)] == 0)))
                    {
                        return 0;
                    }

                    if (battle.menuCommand != index + 1)
                    {
                        this->audio.PlaySound(kBackSound, this->config.soundVolume);
                    }

                    battle.menuCommand = std::min(index + 1, battle.listedCommands + kMenuFixedCommands);
                    battle.unk53BFBC = 1;
                    battle.unk53BFB8 = 1;
                    return 1;
                }
            }

            battle.unk53BFB8 = 0;
            battle.anyStateRole = kNoPanelRole;
            panelUnder(true);
            return 0;
        }

        case kListState2:
        {
            battle.hoverPageTab = -1;

            if (inside(y, kTabsHitTop, kTabsHitBottom))
            {
                const int tab = band(x, kTabsHitFirst, kTabWidth, kSpellTabsEnd);

                if (tab >= 0)
                {
                    battle.hoverPageTab = tab;
                    battle.unk53BFBC = 1;
                }
            }

            if (inside(x, kListHitLeft, kSpellListRight))
            {
                const int row = band(y, kListHitFirst, kListRowHeight, kListHitEnd);

                if (row >= 0 && battle.listScroll + row + 1 <= static_cast<i8>(battle.listEntries))
                {
                    return pickRow(battle.pageListRow, row);
                }
            }

            return scrollParts(kSpellScrollLeft, kSpellScrollRight, kSpellScrollDownTop, kSpellScrollDownBottom);
        }

        case kTargetState:
            return this->HoverTarget();

        case kListState4:
        {
            battle.hoverRoleTab = -1;

            if (inside(y, kTabsHitTop, kTabsHitBottom))
            {
                const int tab = band(x, kTabsHitFirst, kTabWidth, kItemTabsEnd);

                if (tab >= 0)
                {
                    if (battle.listRole != tab)
                    {
                        battle.hoverRoleTab = tab;
                    }

                    battle.unk53BFBC = 1;
                }
            }

            if (inside(x, kListHitLeft, kItemListRight))
            {
                const int row = band(y, kListHitFirst, kListRowHeight, kListHitEnd);

                if (row >= 0 && battle.roleListScroll + row + 1 <= battle.roleListEntries)
                {
                    return pickRow(battle.roleListRow, row);
                }
            }

            if (!leaveList())
            {
                return 0;
            }

            return scrollParts(kItemScrollLeft, kItemScrollRight, kItemScrollDownTop, kItemScrollDownBottom);
        }

        case kWaitState5:
        {
            if (!inside(x, kEscortHitLeft, kEscortHitRight))
            {
                battle.unk53BFB8 = 0;
                return 0;
            }

            const int row = band(y, kEscortHitFirst, kSpecialRowHeight, kEscortHitEnd);

            if (row < 0)
            {
                battle.unk53BFB8 = 0;
                return 0;
            }

            if (battle.guardCursor != row + 1)
            {
                this->audio.PlaySound(kBackSound, this->config.soundVolume);
            }

            battle.guardCursor = row + 1;
            battle.unk53BFB8 = 1;
            battle.unk53BFBC = 1;
            return 1;
        }

        case kSkillListState:
        {
            if (!inside(x, kListHitLeft, kSpecialHitRight))
            {
                battle.unk53BFB8 = 0;
                return 0;
            }

            const int row = band(y, kSpecialHitFirst, kListBarHeight, kSpecialHitEnd);

            if (row < 0 || row + 1 > static_cast<i8>(battle.specialCount))
            {
                battle.unk53BFB8 = 0;
                return 0;
            }

            if (battle.specialRow != row + 1)
            {
                this->audio.PlaySound(kBackSound, this->config.soundVolume);
            }

            battle.specialRow = row + 1;
            battle.unk53BFB8 = 1;
            battle.unk53BFBC = 1;
            return 1;
        }

        case kListState1B:
        {
            if (inside(x, kListHitLeft, kThrowListRight))
            {
                const int row = band(y, kListHitFirst, kListRowHeight, kListHitEnd);

                if (row >= 0 && battle.roleListScroll + row + 1 <= battle.roleListEntries)
                {
                    return pickRow(battle.roleListRow, row);
                }
            }

            if (!leaveList())
            {
                return 0;
            }

            return scrollParts(kSpellScrollLeft, kSpellScrollRight, kItemScrollDownTop, kThrowScrollDownBottom);
        }

        case kWaitState1E:
            return this->HoverRefineCell();

        default:
            return 0;
    }
}

// The 煉符 grid of MouseHover (inline in the original): the column whose right edge is the first past the pointer
// (from kRefineHitLeft), then the row; a column without a row under the pointer passes to the next. The cell is
// 5 * column + row + 1, at most 10.
int Game::HoverRefineCell()
{
    BattleState& battle = this->battleState;
    const int x = this->state.pointerX;
    const int y = this->state.pointerY;
    int column = 0;

    for (int right = kRefineHitFirst;; right += kRefineColumnWidth, ++column)
    {
        if (right >= kRefineHitEnd)
        {
            battle.unk53BFB8 = 0;
            battle.anyStateRole = kNoPanelRole;
            return 0;
        }

        if (static_cast<u32>(x) <= static_cast<u32>(kRefineHitLeft) || static_cast<u32>(x) >= static_cast<u32>(right))
        {
            continue;
        }

        int row = 0;
        int bottom = kRefineRowFirst;

        while (static_cast<u32>(y) <= static_cast<u32>(bottom - kListRowHeight) ||
               static_cast<u32>(y) >= static_cast<u32>(bottom))
        {
            bottom += kListRowHeight;
            ++row;

            if (bottom >= kRefineRowEnd)
            {
                row = -1;
                break;
            }
        }

        if (row < 0)
        {
            continue;
        }

        const int cell = kRefineRows * column + row + 1;

        if (battle.roleListRow != cell)
        {
            this->audio.PlaySound(kBackSound, this->config.soundVolume);
        }

        battle.roleListRow = std::min(cell, kMenuStepMost);
        battle.unk53BFBC = 1;
        battle.unk53BFB8 = 1;
        return 1;
    }
}

// The aim of MouseHover in battle state 3 (inline in the original): with an order on the party, the party roles
// (through RoleCycle with four or more, else every role from the last) not forced nor guarding whose idle picture is
// under the pointer (sampled every pixel of an 8 x 8 square; the current role only for menu commands 2 and 3), or
// whose portrait is; otherwise the standing enemies from the last whose picture is under it (every second pixel; a
// small machine clears turnPointer for menu command 6). The unit aimed at is marked.
int Game::HoverTarget()
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;
    const int x = this->state.pointerX;
    const int y = this->state.pointerY;

    if (battle.unk53BF88 == 1 || battle.unk53C4C0 == 1 || battle.unk53BF2A != 0)
    {
        return 0;
    }

    // The picture of `unit` under the pointer, sampled from it by `step` over kHitSquare (inline in the original):
    // kNoPicture without picture data.
    auto under = [&services, x, y](BattleUnit& unit, int step)
    {
        UnitRect rect;
        unit.GetShadowRect(services, rect);
        const Picture* picture = unit.StepIdleAction(services);

        // A missing picture is read through a null pointer in the original (for the roles).
        if (picture == nullptr || picture->data.empty())
        {
            return kNoPicture;
        }

        for (int down = 0; down < kHitSquare; down += step)
        {
            for (int across = 0; across < kHitSquare; across += step)
            {
                const int pointX = unit.Mirrored() == 1 ? picture->width + 2 * rect.x - x - across : across + x;

                if (PictureHitTest(*picture, pointX, down + y, rect.x, rect.y) != 0)
                {
                    return kPictureHit;
                }
            }
        }

        return kPictureMissed;
    };

    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);

    if (battle.pendingOrders[unit].unk00 == 0)
    {
        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            this->battleEnemies[index].SetAC0(0);
        }

        for (i32 index = static_cast<i32>(battle.enemyCount) - 1; index >= 0; --index)
        {
            BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(index)];

            if (enemy.IsDefeated())
            {
                continue;
            }

            const int hit = under(enemy, kEnemyHitStep);

            if (hit != kNoPicture)
            {
                battle.turnPointer = 1;
            }

            if (hit != kPictureHit)
            {
                if (hit == kPictureMissed)
                {
                    battle.unk53BFB8 = 0;
                }

                continue;
            }

            battle.aimTarget = index + 1;
            battle.hoverUnit = index;
            enemy.SetAC0(1);

            if (battle.menuCommand == kRefineMenuCommand && !enemy.IsSmallMachine())
            {
                battle.turnPointer = 0;
            }

            battle.unk53BFB8 = 1;
            battle.unk53BFBC = 1;
            return 1;
        }

        return 0;
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        this->battleRoles[index].SetAC0(0);
    }

    this->battleRoles[static_cast<std::size_t>(battle.hoverUnit)].SetAC0(0);
    auto& portraits = this->battleHud.portraitHighlight;
    const auto partyRoles = static_cast<i32>(battle.roleCount - battle.escortCount - battle.guestCount);

    auto chooses = [&battle](i32 role)
    {
        return battle.currentUnit - BattleState::FirstRoleUnit != role || battle.menuCommand == kSpellCommand ||
               battle.menuCommand == kItemCommand;
    };

    auto aim = [this, &battle](i32 role)
    {
        battle.aimTarget = role + 1;
        battle.hoverUnit = role;
        this->battleRoles[static_cast<std::size_t>(role)].SetAC0(1);
        battle.unk53BFB8 = 1;
        battle.unk53BFBC = 1;
    };

    // The pointer is compared unsigned (jb/ja in the original).
    auto overPortraitRow = [y]
    {
        return static_cast<u32>(y) >= static_cast<u32>(kPortraitHitTop) &&
               static_cast<u32>(y) <= static_cast<u32>(kPortraitHitBottom);
    };

    auto overPortraitAt = [x](int left)
    {
        return static_cast<u32>(x) >= static_cast<u32>(left) &&
               static_cast<u32>(x) <= static_cast<u32>(left + kPortraitHitWidth);
    };

    auto overPortrait = [&overPortraitRow, &overPortraitAt](i32 role)
    {
        return overPortraitRow() && overPortraitAt(BattleState::PortraitX[static_cast<std::size_t>(role)]);
    };

    if (partyRoles >= static_cast<i32>(Party::Roles))
    {
        for (i32 place = partyRoles - 1; place >= 0; --place)
        {
            const i32 role = BattleState::RoleCycle[static_cast<std::size_t>(place + 1)];
            BattleRole& checked = this->battleRoles[static_cast<std::size_t>(role)];

            if (checked.autoControl == 1 || checked.guestControl == 1 || checked.IsGuarding())
            {
                continue;
            }

            if (under(checked, 1) == kPictureHit)
            {
                if (chooses(role))
                {
                    aim(role);
                    return 1;
                }
            }

            portraits[static_cast<std::size_t>(battle.hoverUnit)] = 0;

            if (overPortrait(role))
            {
                portraits[static_cast<std::size_t>(role)] = 1;
                battle.aimTarget = role + 1;
                battle.hoverUnit = role;
                battle.unk53BFB8 = 1;
                battle.unk53BFBC = 1;
                return 1;
            }

            battle.unk53BFB8 = 0;
        }

        return 0;
    }

    for (i32 role = static_cast<i32>(battle.roleCount) - 1; role >= 0; --role)
    {
        BattleRole& checked = this->battleRoles[static_cast<std::size_t>(role)];

        if (checked.autoControl == 1 || checked.guestControl == 1 || checked.IsGuarding())
        {
            continue;
        }

        if (under(checked, 1) == kPictureHit && chooses(role))
        {
            aim(role);
            std::fill_n(portraits.begin(), Party::Roles, u8{0});
            return 1;
        }
    }

    portraits[static_cast<std::size_t>(battle.hoverUnit)] = 0;

    if (!overPortraitRow())
    {
        battle.unk53BFB8 = 0;
        return 0;
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& checked = this->battleRoles[index];

        if (checked.autoControl == 1 || checked.guestControl == 1 || checked.IsGuarding())
        {
            continue;
        }

        if (overPortraitAt(BattleState::PortraitX[static_cast<std::size_t>(battle.partyRoles[index])]))
        {
            battle.hoverUnit = battle.partyRoles[index];
            portraits[static_cast<std::size_t>(battle.hoverUnit)] = 1;
            battle.unk53BFB8 = 1;
            battle.unk53BFBC = 1;
            battle.aimTarget = static_cast<i32>(index) + 1;
            return 1;
        }
    }

    battle.unk53BFB8 = 0;
    return 0;
}

// swd32.exe 0x00462740: the confirm key of the battle menu (while unk53BF5C), by battle state: 1 takes the menu
// command; 2, 4, 8, 0x1B and 0x1E take a list entry; 3 takes the aimed unit and ends the role's choice; 5 places an
// escort; 7 answers the talk; 0x62..0x71 step the end of the battle (the messages wait out endFrames frames);
// 0xC8 closes the menu.
void Game::MenuConfirm()
{
    BattleState& battle = this->battleState;

    if (battle.unk53BF5C != 1)
    {
        return;
    }

    battle.unk53BFB0 = 0;

    switch (battle.result)
    {
        case kMenuState:
            this->ConfirmCommand();
            return;

        case kListState2:
            this->ConfirmSpell();
            return;

        case kTargetState:
            this->ConfirmTarget();
            return;

        case kListState4:
            this->ConfirmItem();
            return;

        case kWaitState5:
            this->ConfirmEscort();
            return;

        case kTalkState:
            this->ConfirmTalk();
            return;

        case kSkillListState:
            this->ConfirmSpecial();
            return;

        case kListState1B:
            this->ConfirmThrow();
            return;

        case kWaitState1E:
            this->ConfirmRefine();
            return;

        default:
            this->ConfirmEnd();
            return;
    }
}

// The order of the current role as its command sets it (inline in 0x00462740): the speed of its act, the act's flag 1
// (unk53BFF0 and the order's unk08); 1 when the act's flag 0 is set (the command aims at the party).
bool Game::PrepareOrder()
{
    BattleState& battle = this->battleState;
    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    BattleRole& role = this->battleRoles[unit];
    PendingOrder& order = battle.pendingOrders[unit];
    order.unk0C = static_cast<u16>(role.ActSpeed());
    order.unk0E = 0;
    battle.unk53BFF0 = 0;

    if (role.ActFlag1() == 1)
    {
        battle.unk53BFF0 = 1;
        order.unk08 = 1;
    }

    battle.unk53BF88 = 0;
    return role.ActFlag0() == 1;
}

// The aim at the party of a chosen command (inline in 0x00462740): fewer than four party roles aim at the role
// itself (marked, unless the pointer aims), otherwise the next role in RoleCycle.
void Game::AimAtParty()
{
    BattleState& battle = this->battleState;
    const auto partyRoles = static_cast<i32>(battle.roleCount - battle.escortCount - battle.guestCount);

    if (partyRoles >= static_cast<i32>(Party::Roles))
    {
        this->NextRoleTurn();
        return;
    }

    battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);

    if (battle.unk53BFBC == 0)
    {
        BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
        role.SetAC0(1);
        UnitRect rect;
        role.GetShadowRect(this->battleServices, rect);
        battle.unk53BFB8 = 1;
    }
}

// Battle state 3 opened from a list (inline in 0x00462740): the panels closed and the confirm held.
void Game::OpenTargets()
{
    BattleState& battle = this->battleState;
    battle.result = kTargetState;
    battle.listPanelFrames = 0;
    battle.menuGrow = 0;
    battle.listRows = kOpenListRows;
    this->controls.PressConfirm();
}

// The end of a role's choice (inline in 0x00462740): the choosing role becomes turnUnit and the menu closes.
void Game::EndChoice()
{
    BattleState& battle = this->battleState;
    battle.turnUnit = battle.currentUnit;
    battle.commandNames = {};
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;
    battle.result = 0;
    battle.unk53BFB8 = 0;
    battle.listedCommands = 0;
}

// Battle state 1 of MenuConfirm (once the menu has opened, menuGrow at least 6): the command (an accessory command
// its kind) opens its list, aims, or acts at once.
void Game::ConfirmCommand()
{
    BattleState& battle = this->battleState;

    if (battle.unk53BFB8 == 0 || battle.menuGrow < kMenuOpened)
    {
        return;
    }

    i32 command = battle.menuCommand;
    battle.unk53BFB8 = 0;
    battle.unk53BF88 = 0;
    battle.unk53BFF0 = 0;
    battle.unk53C49C = 1;
    battle.listReturnCommand = command;

    if (command >= kFirstAccessoryMenuCommand)
    {
        const auto at = static_cast<std::size_t>(command - kFirstAccessoryMenuCommand);

        if (at < battle.commandKinds.size() && battle.commandKinds[at] != 0)
        {
            command = battle.commandKinds[at];
            battle.menuCommand = command;
        }
    }

    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    BattleRole& role = this->battleRoles[unit];

    auto aimedEnemy = [&battle]
    {
        return battle.enemyOrder[static_cast<std::size_t>(battle.orderStep - 1)];
    };

    auto openList = [this, &battle, &aimedEnemy](i32 state)
    {
        battle.result = state;
        battle.unk53BFB8 = 1;
        this->display.WarpPointer(kListPointerX, kListPointerY);
        battle.actingEnemy = aimedEnemy();
        this->input.SetMousePosition(kListMouseX, kListMouseY);
    };

    auto actAtOnce = [this, &battle, &role, unit](i32 order)
    {
        battle.turnCommand = order;
        battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = order;
        role.SetTarget(1);
        battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);
        battle.pendingOrders[unit].unk00 = 1;
        this->EndChoice();
    };

    switch (command)
    {
        case kAttackCommand:
            battle.result = kTargetState;
            battle.actingEnemy = aimedEnemy();
            break;

        case kSpellCommand:
            battle.listPanelFrames = 0;
            battle.menuGrow = 0;
            battle.listRows = kOpenListRows;
            battle.listScroll = 0;
            openList(kListState2);
            break;

        case kItemCommand:
            battle.roleListRow = 1;
            battle.roleListScroll = 0;
            openList(kListState4);
            break;

        case kSkillCommand:
            battle.specialRow = 1;
            openList(kSkillListState);
            break;

        case kGuardCommand:
            battle.turnCommand = command;
            battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = command;
            role.ResetAfterGuard();
            role.SetTarget(1);
            battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);
            battle.pendingOrders[unit].unk00 = 1;
            this->EndChoice();
            break;

        case kRefineMenuCommand:
        case kAimedCommand15:
        case kAimedCommand17:
        case kAimedCommand1D:
        case kAimedCommand1F:
        case kAimedCommand21:
            battle.result = kTargetState;
            break;

        case kLeaveCommand:
            battle.turnUnit = battle.currentUnit;
            battle.currentUnit = 0;
            battle.result = kLeaveState;
            break;

        case kSelfCommand:
            battle.turnCommand = command;
            battle.result = 0;
            battle.unk53BFB8 = 1;
            battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = command;
            role.SetTarget(1);
            battle.commandNames[0] = {};
            battle.turnUnit = battle.currentUnit;
            battle.commandNames[1] = {};
            battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);
            battle.currentUnit = 0;
            battle.unk53BF60 = 1;
            battle.unk53BFB8 = 0;
            battle.commandNames[2] = {};
            battle.listedCommands = 0;
            break;

        case kPartyAimCommand:
            battle.result = kTargetState;
            battle.unk53BFB8 = 1;
            battle.unk53BF88 = 1;
            break;

        case kControlCommand:
            battle.unk53BCDC |= kControlMark;
            battle.result = kTargetState;
            break;

        case kStealCommand:
            battle.result = kTargetState;
            battle.unk53BDA4 = 0;
            break;

        case kListState1B:
            battle.result = kListState1B;
            battle.roleListRow = 1;
            battle.roleListScroll = 0;
            this->input.SetMousePosition(kListMouseX, kListMouseY);
            break;

        case kOnceCommand1C:
        case kOnceCommand20:
        case kOnceCommand22:
        case kOnceCommand23:
        case kOnceCommand24:
            actAtOnce(command);
            break;

        case kWaitState1E:
            battle.result = kWaitState1E;
            battle.roleListRow = 1;
            this->input.SetMousePosition(kListMouseX, kListMouseY);
            break;

        default:
            break;
    }

    if (battle.unk53BFBC == 0 && battle.result == kTargetState)
    {
        this->NextEnemyTurn();
    }

    this->controls.PressConfirm();
    battle.listRows = kOpenListRows;
    battle.listPanelFrames = 0;
    battle.menuGrow = 0;
}

// Battle state 2 of MenuConfirm: a page tab under the pointer opens its page; otherwise the chosen spell (affordable)
// becomes the command and its target is chosen: the party (a spell of the act's flag 0 aims at the whole party),
// or the enemies.
void Game::ConfirmSpell()
{
    BattleState& battle = this->battleState;

    if (battle.hoverPageTab != -1)
    {
        battle.listPage = battle.hoverPageTab;
        battle.hoverPageTab = -1;
        battle.pageListRow = 1;
        battle.listScroll = 0;
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
        return;
    }

    if (battle.unk53BFB8 == 0)
    {
        return;
    }

    battle.unk53BFB8 = 0;

    if (battle.unk53BD9C != 0 || battle.hoverScrollPart != 0 || battle.specialCursor == 0 || battle.currentUnit == 0)
    {
        return;
    }

    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    BattleRole& role = this->battleRoles[unit];

    if (role.CanPaySkill(this->battleServices, battle.listPage, battle.specialCursor) == 0)
    {
        return;
    }

    u32 act = static_cast<u32>(battle.unk4FE5D4[unit]);
    const u16 kind = role.SelectSkill(this->database, battle.listPage, 0, battle.specialCursor, act);
    battle.unk4FE5D4[unit] = static_cast<i32>(act);
    PendingOrder& order = battle.pendingOrders[unit];

    if (this->PrepareOrder())
    {
        battle.unk53BF88 = 1;
        battle.unk53BFB8 = 1;
        battle.aimTarget = static_cast<i32>(battle.enemyCount);

        if (battle.listPage == kHealPage || kind == 0)
        {
            battle.unk53BFD0 = 1;
            order.unk00 = 1;
        }

        this->OpenTargets();
        return;
    }

    if (battle.listPage != 0)
    {
        order.unk00 = kind == 0 ? 1 : 0;

        if (order.unk00 == 1)
        {
            this->AimAtParty();
            this->OpenTargets();
            return;
        }
    }

    if (battle.unk53BFBC == 0)
    {
        this->NextEnemyTurn();
    }

    this->OpenTargets();
}

// Battle state 3 of MenuConfirm: the aimed unit taken (not for menu command 6 when turnPointer refuses: the refusal
// sound): the order is the menu command, the role's choice ends, every unit is unmarked and the lists reset. Command
// 0x1E throws the chosen object (state 0xD when it lands, a talisman paper back otherwise); a guard command takes
// its stamina; the auto battle remembers the aim.
void Game::ConfirmTarget()
{
    BattleState& battle = this->battleState;

    if (battle.aimTarget == -1 || (battle.currentUnit == 0 && battle.autoBattle == 0))
    {
        return;
    }

    if (battle.menuCommand == kRefineMenuCommand && battle.turnPointer == 0)
    {
        this->audio.PlaySound(kRefusedSound, this->config.soundVolume);
        return;
    }

    if (battle.unk53BFB8 == 0)
    {
        battle.unk53BFB8 = battle.unk53BFBC == 0 ? 1 : 0;
        return;
    }

    battle.turnCommand = battle.menuCommand;
    battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = battle.menuCommand;
    battle.turnUnit = battle.currentUnit;
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;

    // In the auto battle without a current unit the original works on the role 8 before the first and its order
    // before the orders: those calls and writes are left out, the rest done.
    const bool hasRole = battle.turnUnit >= BattleState::FirstRoleUnit;
    const auto unit = hasRole ? static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit) : 0;
    BattleRole* role = hasRole ? &this->battleRoles[unit] : nullptr;

    if (role != nullptr)
    {
        role->SetTarget(1);
    }

    battle.result = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 0;

    if (battle.unk53BF88 == 1)
    {
        if (role != nullptr)
        {
            battle.pendingOrders[unit].unk04 = 1;
            battle.pendingOrders[unit].targetCount = 0;
        }

        battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = battle.turnCommand;
    }

    if (battle.menuCommand == kWaitState1E)
    {
        battle.turnCommand = kItemCommand;
        battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = kItemCommand;

        if (role != nullptr)
        {
            role->UseSelectedItem();
        }

        if (role != nullptr && role->Word2A86Bit13() == 1)
        {
            battle.turnCommand = kThrowOrder;
            battle.unk53C01C = 1;
            battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = kThrowOrder;
            ++battle.unk53C4B0;
        }
        else
        {
            AddOneItem(this->party.inventory, kThrownObject, 0, this->database);
        }
    }
    else if (battle.turnCommand == kSkillCommand && role != nullptr)
    {
        role->TakeCommand2F18();
    }

    if (battle.menuCommand == kListState1B)
    {
        battle.roleListRow = 1;
        battle.roleListScroll = 0;
    }

    for (u32 index = 0; index < battle.enemyCount; ++index)
    {
        this->battleEnemies[index].SetAC0(0);
    }

    for (std::size_t panel = 0; panel < static_cast<std::size_t>(Party::Roles); ++panel)
    {
        this->battleRoles[panel].SetAC0(0);
        this->battleHud.portraitHighlight[panel] = 0;
    }

    battle.listPanelFrames = 0;
    battle.menuGrow = 0;
    this->battleHud.scrollCount = 0;
    battle.roleListRow = 1;
    battle.unk53BFC0 = 0;
    battle.unk53BFC4 = 0;
    battle.unk53BFC8 = 0;
    this->controls.PressConfirm();

    if (battle.autoBattle == 1)
    {
        if (role != nullptr && role->MarkBit1() == 1)
        {
            battle.turnCommand = kGuardCommand;
            battle.turnTaken[static_cast<std::size_t>(battle.turnUnit)] = kGuardCommand;
            role->ResetAfterGuard();
            role->SetTarget(1);
            battle.pendingOrders[unit].unk00 = 1;
        }

        battle.autoTarget = battle.aimTarget;
    }

    battle.commandNames = {};
    battle.listedCommands = 0;
}

// Battle state 4 of MenuConfirm: a role tab under the pointer opens its list; otherwise the chosen object becomes the
// command: an escort object calls its escort (refused while one is being called; with escorts out it opens the 佈置護駕
// panel; else the escort comes at once), others choose their target as ConfirmSpell; a refused object shows
// 體力點數不夠!! (categories 0, 2, 3) with the refusal sound.
void Game::ConfirmItem()
{
    BattleState& battle = this->battleState;

    if (battle.hoverRoleTab != -1)
    {
        battle.listRole = battle.hoverRoleTab;
        battle.hoverRoleTab = -1;
        battle.roleListRow = 1;
        battle.roleListScroll = 0;
        this->audio.PlaySound(kBackSound, this->config.soundVolume);
        return;
    }

    if (battle.unk53BFB8 == 0)
    {
        return;
    }

    battle.unk53BFB8 = 0;

    if (battle.unk53BD9C != 0 || battle.hoverScrollPart != 0 || battle.currentUnit == 0)
    {
        return;
    }

    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    BattleRole& role = this->battleRoles[unit];
    u16 kind = kNoObjectKind;
    u32 act = static_cast<u32>(battle.unk4FE5D4[unit]);
    const int chosen = role.SelectObject(this->database, this->errors, battle.listRole, battle.listCursor, act, kind);
    battle.unk4FE5D4[unit] = static_cast<i32>(act);

    if (chosen == 0)
    {
        const i32 category = battle.listRole;

        if (category == kItemCategory3 || category == 0 || category == kHealPage)
        {
            if (kind == kNoObjectKind)
            {
                return;
            }

            this->battleMessages
                .Add(kRefusalMessageX, kRefusalMessageY, kRefusalMessageFrames, kStaminaShort, kRefusalMessageFlags);
        }

        this->audio.PlaySound(kRefusedSound, this->config.soundVolume);
        return;
    }

    if (role.Word2A86Bit12() == 1)
    {
        this->CallEscort(role);
        return;
    }

    PendingOrder& order = battle.pendingOrders[unit];

    if (this->PrepareOrder())
    {
        battle.aimTarget = static_cast<i32>(battle.enemyCount);
        battle.unk53BF88 = 1;
        battle.unk53BFB8 = 1;
        order.unk00 = 0;

        if (battle.listRole == kHealPage)
        {
            order.unk00 = 1;
            battle.unk53BFD0 = 1;
        }

        if (kind == 0)
        {
            order.unk00 = 1;
        }

        battle.result = kTargetState;
        this->controls.PressConfirm();
        return;
    }

    if (battle.listRole != 0)
    {
        order.unk00 = kind == 0 ? 1 : 0;

        if (order.unk00 == 1)
        {
            const auto partyRoles = static_cast<i32>(battle.roleCount - battle.escortCount - battle.guestCount);

            if (partyRoles >= static_cast<i32>(Party::Roles))
            {
                this->NextRoleTurn();
            }
            else
            {
                battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);

                if (battle.unk53BFBC == 0)
                {
                    role.SetAC0(1);
                    UnitRect rect;
                    role.GetShadowRect(this->battleServices, rect);
                    battle.unk53BFB8 = 1;
                }
            }

            battle.result = kTargetState;
            this->controls.PressConfirm();
            return;
        }
    }

    if (battle.unk53BFBC == 0)
    {
        this->NextEnemyTurn();
    }

    battle.result = kTargetState;
    this->controls.PressConfirm();
}

// An escort object of ConfirmItem (inline in the original); see ConfirmItem. The escort comes at once: the order 0xF
// and the role's choice ends, its object held back (returned item 1) and the escort placed after the roles.
void Game::CallEscort(BattleRole& role)
{
    BattleState& battle = this->battleState;

    if (battle.callingEscort != 0)
    {
        this->battleMessages
            .Add(kRefusalMessageX, kRefusalMessageY, kRefusalMessageFrames, kEscortCalling, kRefusalMessageFlags);
        this->audio.PlaySound(kRefusedSound, this->config.soundVolume);
        return;
    }

    if (battle.escortCount != 0)
    {
        battle.result = kWaitState5;
        battle.guardCursor = battle.escortCount == kEscortRows ? 1 : kEscortRows;
        return;
    }

    battle.turnCommand = kEscortOrder;
    battle.turnUnit = battle.currentUnit;
    battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = kEscortOrder;
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;
    this->battleRoles[static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit)].SetTarget(1);
    battle.result = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 0;
    battle.listPanelFrames = 0;
    battle.menuGrow = 0;
    battle.unk53BFC0 = 0;
    battle.pendingOrders[static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit)].unk00 = 1;
    battle.unk53BFC4 = 0;
    this->controls.PressConfirm();

    if (battle.autoBattle == 1)
    {
        battle.autoTarget = battle.aimTarget;
    }

    battle.commandNames = {};
    battle.listedCommands = 0;
    battle.callingEscort = role.UseSelectedItem();
    battle.SetReturnedItem(1, battle.callingEscort);

    // With every place taken the original writes the escort past the places.
    if (battle.roleCount >= battle.rolePlaces.size())
    {
        battle.callingEscort = static_cast<u16>(battle.roleCount);
        battle.escortCalled[static_cast<std::size_t>(battle.turnUnit)] = static_cast<u16>(battle.roleCount);
        return;
    }

    UnitPlacement& place = battle.rolePlaces[battle.roleCount];
    place.x = kEscortX;
    place.y = kFirstEscortY;
    place.actId = battle.callingEscort;
    place.kind = 1;

    if (battle.mirrored == 1)
    {
        place.x = static_cast<i16>(Display::Width - place.x);
    }

    battle.callingEscort = static_cast<u16>(battle.roleCount);
    battle.escortCalled[static_cast<std::size_t>(battle.turnUnit)] = static_cast<u16>(battle.roleCount);
}

// Battle state 5 of MenuConfirm: the escort object used takes the place of escort row guardCursor (an escort there
// gives its object back and marks UnkPanel4): row 1 the first place, row 2 the second; the order 0xF ends the
// role's choice and the units are measured again.
void Game::ConfirmEscort()
{
    BattleState& battle = this->battleState;

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    battle.callingEscort = role.UseSelectedItem();
    battle.turnUnit = battle.currentUnit;
    battle.turnCommand = kEscortOrder;
    battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = kEscortOrder;
    battle.currentUnit = 0;
    battle.unk53BF60 = 1;
    role.SetTarget(1);
    battle.result = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFB8 = 0;
    battle.listPanelFrames = 0;
    battle.menuGrow = 0;
    battle.unk53BFC0 = 0;
    battle.pendingOrders[static_cast<std::size_t>(battle.turnUnit - BattleState::FirstRoleUnit)].unk00 = 1;
    battle.unk53BFC4 = 0;
    this->controls.PressConfirm();
    const auto row =
        static_cast<std::size_t>(battle.guardCursor - battle.escortCount + static_cast<i32>(battle.roleCount) - 1);
    UnitPlacement& place = battle.rolePlaces[row];
    u16 placed = battle.callingEscort;

    auto replace = [this, &battle, &place](i16 y)
    {
        if (place.actId != 0)
        {
            AddOneItem(this->party.inventory, place.actId, 0, this->database);
            battle.panelFlags |= BattleState::UnkPanel4;
        }

        place.x = kEscortX;
        place.y = y;
        place.actId = battle.callingEscort;
        place.kind = 1;

        if (battle.mirrored == 1)
        {
            place.x = static_cast<i16>(Display::Width - place.x);
        }
    };

    if (battle.guardCursor == 1)
    {
        replace(kFirstEscortY);
        battle.SetReturnedItem(1, battle.callingEscort);
        placed = static_cast<u16>(row);
        battle.callingEscort = static_cast<u16>(row);
    }

    if (battle.guardCursor == kEscortRows)
    {
        replace(kSecondEscortY);
        battle.SetReturnedItem(kEscortRows, battle.callingEscort);
        placed = static_cast<u16>(row);
    }

    battle.commandNames = {};
    battle.callingEscort = 0;
    battle.escortCalled[static_cast<std::size_t>(battle.turnUnit)] = placed;
    battle.listedCommands = 0;
    this->MeasureUnits();
}

// Battle state 7 of MenuConfirm: the last answer (controlRows) ends the role's choice with order 0x63; the others aim
// at the enemies with command 0x19 (state 1 when every enemy is down).
void Game::ConfirmTalk()
{
    BattleState& battle = this->battleState;

    if (battle.controlCursor == battle.controlRows)
    {
        battle.turnCommand = kTalkOrder;
        this->controls.PressConfirm();
        this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)].SetTarget(1);
        battle.commandNames[0] = {};
        battle.unk53BF60 = 1;
        battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);
        battle.commandNames[1] = {};
        battle.turnUnit = battle.currentUnit;
        battle.currentUnit = 0;
        battle.unk53BFB8 = 0;
        battle.result = 0;
        battle.listPanelFrames = 0;
        battle.menuGrow = 0;
        battle.listedCommands = 0;
        battle.listRows = kOpenListRows;
        battle.commandNames[2] = {};
        return;
    }

    battle.result = kTargetState;
    battle.menuCommand = kControlCommand;

    if (this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated())
    {
        int tries = 0;

        while (true)
        {
            if (++battle.orderStep > static_cast<i32>(battle.enemyCount))
            {
                battle.orderStep = 1;
            }

            battle.actingEnemy = battle.enemyOrder[static_cast<std::size_t>(battle.orderStep - 1)];

            if (++tries >= static_cast<int>(battle.enemyCount))
            {
                break;
            }

            if (!this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated())
            {
                battle.unk53BFB8 = 1;
                return;
            }
        }

        battle.result = kMenuState;
    }

    battle.unk53BFB8 = 1;
}

// Battle state 8 of MenuConfirm: the chosen special skill becomes the command (aiming at the party with the act's
// flag 0, otherwise at the enemies).
void Game::ConfirmSpecial()
{
    BattleState& battle = this->battleState;

    if (battle.unk53BFB8 == 0 || battle.currentUnit == 0)
    {
        return;
    }

    battle.unk53BFB8 = 0;
    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    u32 act = static_cast<u32>(battle.unk4FE5D4[unit]);
    const u16 kind = this->battleRoles[unit].SelectSkill(this->database, 0, 1, battle.specialCursor, act);
    battle.unk4FE5D4[unit] = static_cast<i32>(act);

    if (kind == kNoObjectKind)
    {
        return;
    }

    if (this->PrepareOrder())
    {
        battle.unk53BF88 = 1;
        battle.unk53BFB8 = 1;
        battle.aimTarget = static_cast<i32>(battle.enemyCount);
    }
    else if (battle.unk53BFBC == 0)
    {
        this->NextEnemyTurn();
    }

    this->OpenTargets();
}

// Battle state 0x1B of MenuConfirm: the chosen object of the 投擲 list becomes the command, aiming at the enemies.
void Game::ConfirmThrow()
{
    BattleState& battle = this->battleState;

    if (battle.unk53BFB8 == 0)
    {
        return;
    }

    battle.unk53BFB8 = 0;

    if (battle.unk53BD9C != 0 || battle.hoverScrollPart != 0 || battle.currentUnit == 0)
    {
        return;
    }

    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    u16 kind = 0;
    u32 act = static_cast<u32>(battle.unk4FE5D4[unit]);
    const int chosen = this->battleRoles[unit]
                           .SelectObject(this->database, this->errors, kThrowCategory, battle.listCursor, act, kind);
    battle.unk4FE5D4[unit] = static_cast<i32>(act);

    if (chosen == 0)
    {
        return;
    }

    if (battle.unk53BFBC == 0)
    {
        this->NextEnemyTurn();
    }

    battle.result = kTargetState;
    this->controls.PressConfirm();
}

// Battle state 0x1E of MenuConfirm: a 煉符 cell past the objects refuses; otherwise the chosen throwable becomes the
// command (as ConfirmSpecial); a refused one sounds the refusal.
void Game::ConfirmRefine()
{
    BattleState& battle = this->battleState;

    if (battle.unk53BFB8 == 0 || battle.currentUnit == 0)
    {
        return;
    }

    battle.unk53BFB8 = 0;

    if (battle.roleListRow > battle.roleListEntries)
    {
        this->audio.PlaySound(kRefusedSound, this->config.soundVolume);
        return;
    }

    const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
    u16 kind = 0;
    u32 act = static_cast<u32>(battle.unk4FE5D4[unit]);
    const int chosen = this->battleRoles[unit]
                           .SelectObject(this->database, this->errors, kRefineCategory, battle.listCursor, act, kind);
    battle.unk4FE5D4[unit] = static_cast<i32>(act);

    if (chosen == 0)
    {
        this->audio.PlaySound(kRefusedSound, this->config.soundVolume);
        return;
    }

    if (this->PrepareOrder())
    {
        battle.unk53BF88 = 1;
        battle.unk53BFB8 = 1;
        battle.aimTarget = static_cast<i32>(battle.enemyCount);
        battle.result = kTargetState;
        this->controls.PressConfirm();
        return;
    }

    if (battle.unk53BFBC == 0)
    {
        this->NextEnemyTurn();
    }

    battle.result = kTargetState;
    this->controls.PressConfirm();
}

// The end states of MenuConfirm (0x62..0x71, 0xC8): the messages of the end wait out endFrames frames, the level-up
// steps (0x65: a level-up shows 0x6E, a learned skill 0x6F; none ends), 0xC8 closes the menu and unmarks every unit.
void Game::ConfirmEnd()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;

    switch (battle.result)
    {
        case kEndCraft:
            battle.pendingActionRow = 0;
            battle.pendingAction = 0;
            messages.panelHeight = 0;
            battle.endFrames = 0;
            messages.craftSucceeded = 0;
            battle.result = kBattleWon;
            battle.levelUpRole = kNoLevelUp;
            return;

        case kEndVictory:
            if (battle.endFrames >= kEndMessageFrames)
            {
                battle.endFrames = 0;
                messages.panelHeight = 0;
                battle.result = kEndLevelUps;
            }

            return;

        case kEndLevelUps:
            battle.levelUpFrames = 0;

            if (battle.levelUpPending != 0 && battle.endFrames >= kEndLevelUpFrames)
            {
                messages.panelHeight = 0;

                if (battle.levelUpRole == kNoLevelUp)
                {
                    if (messages.lootCount != 0)
                    {
                        this->audio.PlaySound(kLearnedSound, this->config.soundVolume);
                    }

                    battle.endFrames = 0;
                    battle.unk53BF5C = 0;
                    battle.levelUpPending = 0;
                    battle.result = kEndLoot;
                }
                else
                {
                    this->audio.PlaySound(kLevelUpPanelSound, this->config.soundVolume);
                    battle.levelUpPending = 0;
                    battle.endFrames = 0;
                    battle.result = kEndStats;
                }
            }

            return;

        case kEndLoot:
            if (battle.endFrames >= kEndMessageFrames)
            {
                battle.endFade = 1;
                battle.result = 0;
                battle.endFrames = 0;
                battle.unk53BF5C = 0;
                messages.panelHeight = 0;
            }

            return;

        case kBattleLost:
            if (battle.endFrames >= kEndMessageFrames)
            {
                battle.endFade = 1;
                messages.panelHeight = 0;
                battle.result = 0;
                battle.endFrames = 0;
            }

            return;

        case kLostQuietly:
            battle.endFade = 1;
            messages.panelHeight = 0;
            return;

        case kEndStats:
            messages.panelHeight = 0;

            if (battle.learnedPending == 1)
            {
                if (!this->battleRoles[static_cast<std::size_t>(static_cast<i8>(battle.levelUpRole))].IsDefeated())
                {
                    this->battleRoles[static_cast<std::size_t>(static_cast<i8>(battle.levelUpRole))].RestartMainAct(
                        kLearnedPose,
                        1
                    );
                }

                this->audio.PlaySound(kLearnedSound, this->config.soundVolume);
                battle.endFrames = 0;
                battle.result = kEndNewSpell;
            }
            else
            {
                battle.levelUpPending = 0;
                battle.endFrames = 0;
                battle.levelUpRole = kNoLevelUp;
                battle.result = kEndLevelUps;
            }

            return;

        case kEndNewSpell:
            if (battle.endFrames > kEndMessageFrames)
            {
                messages.panelHeight = 0;
                battle.levelUpPending = 0;
                battle.learnedPending = 0;
                battle.endFrames = 0;
                battle.levelUpRole = kNoLevelUp;
                battle.result = kEndLevelUps;
            }

            return;

        case kEndSpecials:
        case kEndMastery:
            if (battle.endFrames > kEndMessageFrames)
            {
                messages.panelHeight = 0;
                battle.learnedPending = 0;
                battle.endFrames = 0;
                battle.levelUpRole = kNoLevelUp;
            }

            return;

        case kCloseMenuState:
        {
            battle.turnCommand = 0;
            battle.menuCommand = 1;
            battle.currentUnit = 0;
            battle.turnUnit = 0;
            battle.result = 0;
            battle.unk53BFBC = 0;
            battle.unk53BFB8 = 0;

            for (u32 index = 0; index < battle.enemyCount; ++index)
            {
                this->battleEnemies[index].SetAC0(0);
            }

            for (u32 index = 0; index < battle.roleCount; ++index)
            {
                this->battleRoles[index].SetAC0(0);
            }

            battle.listPanelFrames = 0;
            battle.menuGrow = 0;
            battle.unk53BFC0 = 0;
            battle.unk53BFC4 = 0;
            battle.unk53BF5C = 0;
            this->controls.PressConfirm();

            if (battle.autoBattle == 1)
            {
                battle.autoTarget = battle.aimTarget;
            }

            battle.commandNames = {};
            battle.listedCommands = 0;
            return;
        }

        default:
            return;
    }
}

// swd32.exe 0x004620D0: the confirm key of the battle (not while unk53BF1C, unk53C024, the fade out or a negative
// unk53BDAC): with one standing enemy left and a talk role, the menu closes (unless won); the level-up stats skip to
// their end (0x1D, then all at once); a talk window moves on; outside the menu MenuConfirm takes it; a role taking
// the panel (not held) opens the menu (kOpenSound): the command menu, or the talk menu of the talk role with its
// answers (攻擊, the enemy's objects and skills, 釋放).
void Game::ConfirmKey()
{
    BattleState& battle = this->battleState;

    if (battle.unk53BF1C != 0 || battle.unk53C024 == 1 || battle.endFade == 1 || battle.unk53BDAC < 0)
    {
        return;
    }

    if (battle.unk53C4C0 == 0 && static_cast<i32>(battle.enemyCount) - battle.enemiesGone <= 1 &&
        battle.unk4A7626 != kNoTalkRole && battle.result != kBattleWon)
    {
        battle.result = 0;
        return;
    }

    if (battle.result == kEndStats)
    {
        if (battle.levelUpFrames < kRollStart)
        {
            battle.levelUpFrames = kRollStart - 1;
            return;
        }

        if (battle.levelUpFrames == kRollStart)
        {
            battle.levelUpFrames = kShowNow;
            return;
        }
    }

    if (this->scene.talkWindows != nullptr)
    {
        this->talk.advance = 1;
        return;
    }

    if (battle.unk53BF5C != 1 || battle.currentUnit == 0 || (battle.result != 0 && battle.anyStateRole == kNoPanelRole))
    {
        this->MenuConfirm();
        return;
    }

    battle.anyStateRole = kNoPanelRole;
    BattleRole& role = this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];

    if (role.IsHeldOrHolding() == 1)
    {
        return;
    }

    this->audio.PlaySound(kOpenSound, this->config.soundVolume);
    battle.unk53BFB8 = 1;
    battle.unk53C49C = 1;
    battle.listRows = kListRows;
    role.GetPosition(battle.unk53BF4A, battle.unk53BF4E);

    if (battle.controlledEnemy == kNoTalkRole || battle.talkRole != battle.currentUnit - BattleState::FirstRoleUnit)
    {
        battle.result = kMenuState;
        battle.menuCommand = 1;
        this->BuildCommandMenu();
        return;
    }

    battle.result = kTalkState;
    battle.controlRows = kTalkFixedAnswers;
    BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(static_cast<i16>(battle.controlledEnemy))];
    std::string name;

    for (int choice = 0; choice < kControlObjects; ++choice)
    {
        u32 id = static_cast<u32>(battle.orderedSkill);

        if (enemy.ItemName(this->database, choice, name, id) == 1)
        {
            ++battle.controlRows;
        }

        battle.orderedSkill = static_cast<i32>(id);
    }

    for (int choice = 0; choice < kControlSkills; ++choice)
    {
        if (enemy.SkillName(this->database, choice, name) == 1)
        {
            ++battle.controlRows;
        }
    }

    battle.orderedSkill = 0;
}

// A key that repeats (inline in the original): the first frame held, then every third frame from the fifteenth.
bool Game::KeyRepeats(const ButtonState& button)
{
    return button.heldFrames == 1 || (button.heldFrames >= kRepeatDelay && button.heldFrames % kRepeatStep == 1);
}

// swd32.exe 0x0045F2A0: the keys of the battle (not while paused). In the menu (battle state 0 or 1, no talk window)
// the digit keys 1..8 take that command (guard at once); right Shift moves the panel to the previous role, G
// retreats (無法撤退!! when the role is marked or the party cannot), A starts the auto battle. The right mouse button
// and right Ctrl press Escape; the left button takes the role under the pointer or the scroll part under it and
// presses Space; Enter presses Space; Space confirms (every sixth frame when held). The arrow keys move the menu, the
// panel and the talk choice, Page Up and Page Down scroll the lists, Escape cancels.
void Game::BattleKeys()
{
    BattleState& battle = this->battleState;
    Controls& controls = this->controls;
    battle.unk53BD9C = 0;

    // The rebuild (deviations.md): whether the mouse or a key was pressed last (the right button is taken for Escape
    // below).
    for (const Button key : kMenuKeys)
    {
        if (controls.At(key).press != 0)
        {
            battle.menuByMouse = false;
        }
    }

    if (controls.At(Button::MouseLeft).press != 0 || controls.At(Button::MouseRight).press != 0)
    {
        battle.menuByMouse = true;
    }

    if (battle.paused == 1)
    {
        return;
    }

    i32 unit = battle.currentUnit;

    if (battle.result <= kMenuState && battle.currentUnit != 0 && this->scene.talkWindows == nullptr)
    {
        auto takeCommand = [this, &battle](i32 command)
        {
            if (battle.result == 0)
            {
                battle.result = kMenuState;
            }

            this->BuildCommandMenu();

            if (battle.commandEnabled[static_cast<std::size_t>(command - 1)] == 1)
            {
                battle.menuGrow = kMenuOpened;
                battle.unk53BFB8 = 1;
                battle.menuCommand = command;
                this->ConfirmKey();
            }
        };

        for (int command = 1; command <= kDigitCommands; ++command)
        {
            if (this->input.IsKeyDown(kDigitKey1 + command - 1) == 0)
            {
                continue;
            }

            if (command != kGuardCommand)
            {
                takeCommand(command);
                return;
            }

            if (battle.menuCommand != kGuardCommand)
            {
                if (battle.result == 0)
                {
                    battle.result = kMenuState;
                }

                battle.menuGrow = kMenuOpened;
                battle.unk53BFB8 = 1;
                battle.menuCommand = kGuardCommand;
                this->ConfirmKey();
            }

            battle.result = 0;
            battle.unk53BFB8 = 0;
            battle.menuCommand = 1;
            return;
        }

        unit = battle.currentUnit;
    }

    const ButtonState& space = controls.At(Button::Space);

    if (battle.unk53C024 == 1 && space.heldFrames == 1 && space.press != 0)
    {
        battle.unk53BDA4 |= kSpaceMark;
    }

    const ButtonState& rightControl = controls.At(Button::RightControl);

    if (rightControl.press != 0 && rightControl.heldFrames >= 1)
    {
        controls.At(Button::Escape).press = 1;
        controls.At(Button::Escape).heldFrames = rightControl.heldFrames;
    }

    auto panelTaken = [this, &battle]
    {
        if (battle.result == kMenuState)
        {
            this->ConfirmKey();
        }

        battle.anyStateRole = kNoPanelRole;
    };

    if (controls.At(Button::RightShift).press != 0 && Game::KeyRepeats(controls.At(Button::RightShift)))
    {
        if (battle.result > kMenuState)
        {
            return;
        }

        battle.anyStateRole = 0;
        battle.menuCommand = 1;
        this->SelectPreviousRole();
        panelTaken();
        return;
    }

    if (controls.At(Button::KeyG).press != 0 && Game::KeyRepeats(controls.At(Button::KeyG)))
    {
        if (battle.unk53BF1C != 0 || battle.hitsAllRoles == 1 || battle.unk53BF64 == 1 || battle.result == kTalkOrder ||
            battle.result == kEndVictory || this->scene.talkWindows != nullptr)
        {
            return;
        }

        // Without a current unit the original reads the role 8 before the first: taken as not held.
        const bool held =
            unit >= BattleState::FirstRoleUnit &&
            this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].IsHeldOrHolding() == 1;

        if (held || (static_cast<i32>(battle.enemyCount) - battle.enemiesGone == 1 && battle.unk4A7626 != kNoTalkRole))
        {
            return;
        }

        unit = battle.currentUnit;

        if (battle.currentUnit != 0)
        {
            BattleRole& role =
                this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];

            if (!role.MarkBit11Clear() || (battle.panelFlags & BattleState::NoRetreat) != 0)
            {
                SleepMilliseconds(kRefusedPause);
                this->battleMessages.Add(
                    kRefusalMessageX,
                    kRefusalMessageY,
                    kRetreatMessageFrames,
                    kNoRetreatText,
                    kRefusalMessageFlags
                );
                this->audio.PlaySound(kRefusedSound, this->config.soundVolume);
                return;
            }

            SleepMilliseconds(kRetreatPause);
            battle.turnCommand = kLeaveCommand;
            battle.turnTaken[static_cast<std::size_t>(battle.currentUnit)] = kLeaveCommand;
            role.SetTarget(1);
            battle.unk53BF60 = 1;
            battle.turnUnit = battle.currentUnit;
            battle.result = 0;
            battle.sweptUnits = 0;
            battle.aimTarget = battle.currentUnit - (BattleState::FirstRoleUnit - 1);
            unit = 0;
            battle.unk53BF1C |= kRetreatMark;
            battle.currentUnit = 0;
            battle.unk53BFB8 = 0;
            battle.unk53BFC0 = 1;
            battle.unk53BFC4 = 1;
            battle.listPanelFrames = 0;
            battle.menuGrow = 0;
            battle.listRows = kOpenListRows;
        }
    }

    if (controls.At(Button::KeyA).press != 0 && Game::KeyRepeats(controls.At(Button::KeyA)))
    {
        if (battle.result <= kMenuState && battle.roleCount != 0 && this->scene.talkWindows == nullptr)
        {
            // The rebuild's auto combo (deviations.md): the first aimed attack with 連擊 when it can.
            const i32 command = this->AutoCombo(unit) ? kAimedCommand15 : 1;
            battle.unk53BFBC = 0;
            battle.autoBattle = 1;
            battle.turnCommand = command;
            battle.turnTaken[static_cast<std::size_t>(unit)] = command;
            battle.unk53BFB8 = 1;
            battle.menuCommand = command;
            battle.result = kTargetState;
            return;
        }
    }

    const ButtonState& mouseRight = controls.At(Button::MouseRight);

    if (mouseRight.press != 0 && mouseRight.heldFrames >= 1)
    {
        battle.promptFrames = 0;
        battle.unk53BFBC = 1;
        controls.At(Button::Escape).press = 1;
        controls.At(Button::Escape).heldFrames = mouseRight.heldFrames;
    }

    ButtonState& mouseLeft = controls.At(Button::MouseLeft);

    if (mouseLeft.press != 0 && Game::KeyRepeats(mouseLeft))
    {
        battle.unk53BFBC = 1;

        if (battle.anyStateRole != kNoPanelRole)
        {
            this->SelectRoleUnit(static_cast<i16>(battle.anyStateRole));
            panelTaken();
            return;
        }

        battle.promptFrames = 0;

        if (battle.hoverScrollPart == kScrollUp)
        {
            battle.unk53BD9C = kScrollUp;
            this->MenuUp();
        }

        if (battle.hoverScrollPart == kScrollDown)
        {
            battle.unk53BD9C = kScrollDown;
            this->MenuDown();
        }

        if (battle.hoverScrollPart == kScrollAbove)
        {
            battle.unk53BD9C = kScrollAbove;
            this->ScrollListBack();
        }

        if (battle.hoverScrollPart == kScrollBelow)
        {
            battle.unk53BD9C = kScrollBelow;
            this->ScrollListForward();
        }

        const i32 held = mouseLeft.heldFrames;
        const auto pointerY = static_cast<u32>(this->state.pointerY);

        if (pointerY > static_cast<u32>(this->battleHud.scrollThumbTop) &&
            pointerY < static_cast<u32>(this->battleHud.scrollThumbBottom) && held == 1)
        {
            battle.unk53BD6C = this->state.pointerY;
            battle.unk53BD70 = 0;
        }

        if (battle.unk53BFF8 == 1 && held >= 1)
        {
            battle.unk53BD9C = kThumbPart;
        }

        controls.At(Button::Space).press = 1;
        controls.At(Button::Space).heldFrames = held;
        battle.unk53BFBC = 1;
    }

    const ButtonState& enter = controls.At(Button::Enter);
    bool confirm = true;

    if (enter.press != 0 && enter.heldFrames % kRepeatStep == 1)
    {
        battle.unk53BFBC = 0;
        controls.At(Button::Space).press = 1;
        controls.At(Button::Space).heldFrames = enter.heldFrames;
    }
    else if (controls.At(Button::Space).press == 0)
    {
        confirm = false;
    }

    const i32 spaceHeld = controls.At(Button::Space).heldFrames;

    if (confirm && (spaceHeld == 1 || (spaceHeld >= kRepeatDelay && spaceHeld % kConfirmRepeatStep == 1)))
    {
        if (battle.unk53BDAC < 0)
        {
            battle.unk53BDAC |= 1;
        }

        if ((battle.unk53BFBC == 1 && battle.hoverRegion != nullptr) ||
            (battle.unk53BFBC != 1 && !this->state.clickRegions.regions.empty()))
        {
            this->state.clickRegions.Free();
        }

        if (mouseLeft.press == 0)
        {
            battle.unk53BFBC = 0;
            battle.promptFrames = kConfirmIdle;
            battle.unk53BFB8 = 1;
            battle.hoverScrollPart = 0;
        }

        battle.panelFlags &= ~BattleState::ResistancePanel;
        this->ConfirmKey();
    }

    if (controls.At(Button::Up).press != 0 && Game::KeyRepeats(controls.At(Button::Up)) &&
        this->scene.talkWindows == nullptr)
    {
        this->MenuUp();
        this->SelectPreviousRole();
        battle.unk53BD9C = kScrollUp;
    }

    if (controls.At(Button::Down).press != 0 && Game::KeyRepeats(controls.At(Button::Down)) &&
        this->scene.talkWindows == nullptr)
    {
        this->MenuDown();
        this->SelectNextRole();
        battle.unk53BD9C = kScrollDown;
    }

    if (controls.At(Button::Left).press != 0 && Game::KeyRepeats(controls.At(Button::Left)))
    {
        ClickRegions& regions = this->state.clickRegions;

        if (!regions.regions.empty())
        {
            --this->talk.choice;

            if ((this->talk.choice & kChoiceSignBit) != 0)
            {
                this->talk.choice = regions.Count() - 1;
            }
        }

        if (this->scene.talkWindows == nullptr)
        {
            if (battle.result == kTargetState)
            {
                this->MenuUp();
            }

            this->SelectNextRole();
            this->MenuLeft();
        }
    }

    if (controls.At(Button::Right).press != 0 && Game::KeyRepeats(controls.At(Button::Right)))
    {
        ClickRegions& regions = this->state.clickRegions;

        if (!regions.regions.empty())
        {
            ++this->talk.choice;

            if (this->talk.choice > regions.Count() - 1)
            {
                this->talk.choice = 0;
            }
        }

        if (this->scene.talkWindows == nullptr)
        {
            if (battle.result == kTargetState)
            {
                this->MenuDown();
            }

            this->MenuRight();
            this->SelectPreviousRole();
        }
    }

    if (controls.At(Button::PageUp).press != 0 && controls.At(Button::PageUp).heldFrames % kRepeatStep == 1)
    {
        this->ScrollListBack();
    }

    if (controls.At(Button::PageDown).press != 0 && controls.At(Button::PageDown).heldFrames % kRepeatStep == 1)
    {
        this->ScrollListForward();
    }

    if (controls.At(Button::Escape).press != 0 && Game::KeyRepeats(controls.At(Button::Escape)))
    {
        this->MenuCancel();
        battle.autoBoxStep = 0;
        battle.autoBoxX = 0;
    }
}

// swd32.exe 0x00464270: the battle menu of a frame (not in battle state 0x67; without a current unit only in the auto
// battle). A held current role drops its command and gives the panel to the next role in roleOrder not held; one
// that may not act (StatusBit5Or8) takes its turn at once. By battle state: 1 the command menu with the role's name
// over the tall gauge (grown for six frames at the pointer, or at kMenuHome after kMenuIdleFrames or with the pointer
// near the edge), 2, 4, 8, 0x1B and 0x1E the lists (with a scroll bar past seven rows), 3 the aim pointers, 5 the
// 佈置護駕 panel, 6 the retreat, 7 the 控制 menu.
void Game::DrawBattleMenu()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;

    if (battle.result == kBattleLost || (battle.currentUnit == 0 && battle.autoBattle != 1))
    {
        return;
    }

    // Without a current unit (the auto battle) the original reads the role 8 before the first: taken as neither held
    // nor kept from acting.
    auto currentRole = [this, &battle]() -> BattleRole*
    {
        if (battle.currentUnit < BattleState::FirstRoleUnit)
        {
            return nullptr;
        }

        return &this->battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    };

    i32 nameIndex = battle.currentUnit - BattleState::FirstRoleUnit;

    if (BattleRole* role = currentRole(); role != nullptr && role->IsHeldOrHolding() == 1)
    {
        battle.listName = {};
        battle.result = 0;
        battle.unk53BFC4 = 0;
        battle.unk53BFC0 = 0;
        battle.unk53BFC8 = 0;
        battle.unk53BFB0 = 0;
        battle.unk53C49C = 1;
        battle.listRows = kListRows;
        role->CancelCommand();

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            this->battleEnemies[index].SetAC0(0);
        }

        nameIndex = 0;

        for (; nameIndex < static_cast<i32>(battle.roleCount) - 1; ++nameIndex)
        {
            i32& next = battle.roleOrder[static_cast<std::size_t>(nameIndex)];
            const i32 unit = next;

            if (unit == 0)
            {
                next = battle.currentUnit;
                battle.currentUnit = 0;
                break;
            }

            if (this->battleRoles[static_cast<std::size_t>(unit - BattleState::FirstRoleUnit)].IsHeldOrHolding() != 1)
            {
                next = battle.currentUnit;
                battle.currentUnit = unit;
                break;
            }
        }
    }

    if (BattleRole* role = currentRole(); role != nullptr && role->StatusBit5Or8() == 1)
    {
        this->StartRoleTurn(battle.currentUnit);
        battle.currentUnit = 0;
    }

    battle.unk53BF5C = 1;

    auto listDrawn = [&battle]
    {
        battle.unk53BFC4 = 1;
        battle.unk53BFC0 = 1;
    };

    auto scrollBar = [&battle, &hud](i32 count, i32 first, int x)
    {
        hud.scrollCount = count;
        hud.scrollFirst = first;

        if (count > kListRowsShown)
        {
            hud.DrawScrollBar(kListScrollBar, x, kListScrollY, kListScrollRows, battle.unk53BD70, battle.unk53BD9C);
        }
    };

    switch (battle.result)
    {
        case kMenuState:
            if (battle.currentUnit != 0)
            {
                this->DrawCommandPanel(nameIndex);
            }

            return;

        case kListState2:
            battle.unk53BFC8 = 1;
            this->DrawListTabs(kListX, kListY);

            if (battle.menuGrow == kListGrown && battle.listPanelFrames == kListOpened)
            {
                this->DrawSpellList(kSpellListX, kSpellListY, battle.pageListRow, battle.listScroll);
                scrollBar(static_cast<i8>(battle.listEntries), battle.listScroll, kSpellScrollX);
            }

            listDrawn();
            return;

        case kTargetState:
            this->DrawAimPointers();
            return;

        case kListState4:
            battle.unk53BFC8 = 1;
            this->DrawItemList(kListX, kListY, battle.roleListRow, battle.roleListScroll);
            scrollBar(battle.roleListEntries, battle.roleListScroll, kItemScrollX);
            listDrawn();
            return;

        case kWaitState5:
            this->DrawEscortList(kEscortListX, kEscortListY);
            return;

        case kLeaveState:
            if (battle.unk53BF64 == 0 && battle.hitsAllRoles == 0)
            {
                battle.unk53BF1C |= kRetreatMark;
                battle.unk53BFC0 = 1;
                battle.unk53BFC4 = 1;
                battle.result = 0;
                battle.currentUnit = 0;
            }

            return;

        case kTalkState:
            this->DrawControlMenu(battle.menuX + kControlMenuX, battle.menuY + kControlMenuY, battle.controlCursor);
            listDrawn();
            return;

        case kSkillListState:
            battle.unk53BFC8 = 1;
            this->DrawSpecialList(kListX, kListY, battle.specialRow);
            listDrawn();
            return;

        case kListState1B:
            battle.unk53BFC8 = 1;
            this->DrawThrowList(kListX, kListY, battle.roleListRow, battle.roleListScroll);
            scrollBar(battle.roleListEntries, battle.roleListScroll, kThrowScrollX);
            listDrawn();
            return;

        case kWaitState1E:
            battle.unk53BFC8 = 1;
            this->DrawRefineList(kListX, kListY, battle.roleListRow);
            listDrawn();
            return;

        default:
            return;
    }
}

// Battle state 1 of DrawBattleMenu (inline in the original): the tall gauge grows for six frames (menuGrow) from
// the pointer, then the name of party role nameIndex (the role whose panel it is, or the step of the panel handed
// on) and the command menu.
void Game::DrawCommandPanel(i32 nameIndex)
{
    smooth::Scene scene;
    smooth::TurnFrame(*this).DrawCommandPanelAdvance(nameIndex, scene);
    scene.Show(this->battleServices);
}

// Battle state 3 of DrawBattleMenu (inline in the original). Aiming at every unit (unk53BF88), a pointer over each
// role standing (for a command at the party) or each enemy standing (the rebuild: at its middle). Otherwise a
// defeated aimed enemy passes the aim on through enemyOrder (state 1 when none stands), and the pointer marks the
// aimed role or enemy (turnPointer 0: the refusing pointer; command 6 refuses an enemy not a small machine), with the
// name tag while unk53BFB8.
void Game::DrawAimPointers()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;
    this->mediumFont.SetAdvance(TextFont::DefaultGlyphSize);

    // Without a current unit (the auto battle) the original reads the order before the first: taken as aiming at
    // the enemies.
    const bool atParty =
        battle.currentUnit >= BattleState::FirstRoleUnit &&
        battle.pendingOrders[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)].unk00 != 0;
    UnitRect rect;

    if (battle.unk53BF88 == 1)
    {
        if (atParty)
        {
            for (u32 index = 0; index < battle.roleCount; ++index)
            {
                BattleRole& role = this->battleRoles[index];

                if (role.autoControl != 1 && role.guestControl != 1 && role.StatusBit13() == 0)
                {
                    role.GetShadowRect(this->battleServices, rect);
                    role.SetAC0(1);
                    hud.DrawSlotAnimation(
                        kTurnPointer,
                        static_cast<int>(index),
                        rect.x + (rect.width >> 1),
                        rect.y + (rect.height >> 1)
                    );
                }
            }

            return;
        }

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = this->battleEnemies[index];

            if (enemy.IsDefeated())
            {
                continue;
            }

            enemy.GetShadowRect(this->battleServices, rect);
            enemy.SetAC0(1);
            enemy.GetOffset(battle.unk53BF4A, battle.unk53BF4E);
            // The original draws the pointer at the enemy's anchor when it has one, off the enemy for some (below a
            // floating one); the rebuild draws it at the middle as for a single enemy (deviations.md).
            hud.DrawSlotAnimation(
                kTurnPointer,
                static_cast<int>(index),
                rect.x + (rect.width >> 1),
                rect.y + (rect.height >> 1)
            );
        }

        return;
    }

    if (!atParty && this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated())
    {
        u32 tries = 0;

        while (true)
        {
            if (++battle.orderStep > static_cast<i32>(battle.enemyCount))
            {
                battle.orderStep = 1;
            }

            battle.actingEnemy = battle.enemyOrder[static_cast<std::size_t>(battle.orderStep - 1)];
            battle.aimTarget = battle.actingEnemy + 1;

            if (++tries >= battle.enemyCount)
            {
                battle.result = kMenuState;
                break;
            }

            if (!this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)].IsDefeated())
            {
                break;
            }
        }
    }

    if (battle.aimTarget == -1)
    {
        return;
    }

    if (battle.unk53BFBC == 0)
    {
        battle.unk53BF4E = 0;
        battle.unk53BF4A = 0;

        if (atParty)
        {
            const auto aimed = static_cast<std::size_t>(battle.aimTarget - 1);

            if (aimed < this->battleRoles.size())
            {
                BattleRole& role = this->battleRoles[aimed];
                role.GetShadowRect(this->battleServices, rect);
                role.GetOffset(battle.unk53BF4A, battle.unk53BF4E);
                role.SetAC0(1);
                battle.unk53BF4E = static_cast<u16>(battle.unk53BF4E + kRoleAimDrop);
            }
        }
        else
        {
            BattleEnemy& enemy = this->battleEnemies[static_cast<std::size_t>(battle.actingEnemy)];
            enemy.SetAC0(1);
            enemy.GetShadowRect(this->battleServices, rect);
            enemy.GetOffset(battle.unk53BF4A, battle.unk53BF4E);
            battle.aimTarget = battle.actingEnemy + 1;

            if (battle.menuCommand == kRefineMenuCommand)
            {
                battle.turnPointer = enemy.IsSmallMachine() ? 1 : 0;
            }
        }

        hud.DrawSlotAnimation(
            battle.turnPointer == 0 ? kNoTurnPointer : kTurnPointer,
            0,
            rect.x + (rect.width >> 1),
            rect.y + (rect.height >> 1)
        );
    }

    if (battle.unk53BFB8 == 1)
    {
        this->DrawTargetTag(kTargetTagX, kTargetTagY);
    }
}

// swd32.exe 0x00466F70: the frame of the battle end states (not while a talk window or a battle message shows):
// 0x60 back to state 0; 0x61 the glow of the unit after the roles (a level-up role 1 once over); 0x62 the 煉符 result;
// 0x63 the end checked (every enemy down, unless the talk or unk53C048 ended it): a waiting refine (pendingAction)
// refines its enemy into a talisman (state 0x62 when its role is down or nothing came), otherwise 0x64; 0x64 the
// victory panel; 0x65 the level-ups one role at a time (0x70 once none is left); 0x66 the loot; 0x67 the defeat;
// 0x68 a pause; 0x6E the level-up stats; 0x6F the new spell; 0x70 the new special skills (0x71 after); 0x71 the
// masteries (0x66 after). The panels confirm themselves after kEndPanelFrames.
void Game::BattleEndFrame()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;

    if (this->scene.talkWindows != nullptr || messages.messages != nullptr)
    {
        return;
    }

    auto levelRole = [this, &battle]() -> BattleRole&
    {
        return this->battleRoles[static_cast<std::size_t>(static_cast<i8>(battle.levelUpRole))];
    };

    auto noLevelRole = [&battle]
    {
        return battle.levelUpRole == kNoLevelUp;
    };

    switch (battle.result)
    {
        case kEndBack:
            battle.unk53BFC4 = 0;
            battle.unk53BFB8 = 0;
            battle.result = 0;
            return;

        case kEndGlow:
        {
            // The original reads the unit after the roles even with every place taken.
            if (battle.roleCount >= this->battleRoles.size())
            {
                return;
            }

            const UnitPlacement& place = battle.rolePlaces[battle.roleCount];

            if (this->battleRoles[battle.roleCount].GlowAct(this->battleServices, place.x, place.y) == 1)
            {
                battle.levelUpRole = 1;
            }

            return;
        }

        case kEndCraft:
            battle.unk53BFC0 = 1;
            messages.DrawCraftResultPanel();
            return;

        case kEndCheck:
            this->EndCheckFrame();
            return;

        case kEndVictory:
            if (battle.victoryWithoutPanel == 1)
            {
                battle.endFade = 1;
                return;
            }

            this->DrawVictoryPanel();
            battle.unk53BF64 = 0;
            battle.unk53BFC0 = 1;
            battle.unk53BFC4 = 1;
            battle.unk53BF74 = 1;
            battle.currentUnit = 0;

            if (++battle.endFrames >= kEndPanelFrames)
            {
                this->ConfirmKey();
            }

            return;

        case kEndLevelUps:
            if (noLevelRole())
            {
                this->CheckLevelUps();

                if (noLevelRole())
                {
                    battle.endFrames = 0;
                    battle.result = kEndSpecials;
                    return;
                }
            }

            if (battle.levelUpPending == 0)
            {
                if (!levelRole().IsDefeated())
                {
                    battle.levelUpPending = levelRole().RestartMainAct(kLevelUpPose, 0) ? 1 : 0;
                }

                if (battle.levelUpPending == 0)
                {
                    return;
                }
            }

            if (noLevelRole())
            {
                battle.endFrames = 0;
                battle.result = kEndSpecials;
            }
            else
            {
                this->DrawLevelUpPanel();
                ++battle.endFrames;
            }

            return;

        case kEndLoot:
            if (messages.lootCount == 0)
            {
                battle.endFade = 1;
                return;
            }

            if (++battle.endFrames >= kEndPanelFrames)
            {
                this->ConfirmKey();
            }

            messages.DrawLootPanel();
            return;

        case kBattleLost:
            battle.unk53BFC0 = 1;
            battle.unk53BFC4 = 1;

            if ((battle.panelFlags & BattleState::UnkPanel8) != 0)
            {
                if (++battle.endFrames >= kEndQuietFrames)
                {
                    battle.endFrames = 0;
                    battle.endFade = 1;
                }

                return;
            }

            messages.DrawDefeatPanel();

            if (++battle.endFrames >= kEndPanelFrames)
            {
                this->ConfirmKey();
            }

            return;

        case kLostQuietly:
            battle.unk53BFC0 = 1;
            battle.unk53BFC4 = 1;

            if (++battle.endFrames > kEndMessageFrames)
            {
                this->ConfirmKey();
            }

            return;

        case kEndStats:
            if (battle.levelUpPending == 0 && !levelRole().IsDefeated())
            {
                battle.levelUpPending = levelRole().RestartMainAct(kStatsPose, 0) ? 1 : 0;
            }

            if (battle.levelUpPending != 0)
            {
                this->DrawLevelUpStats();
            }

            return;

        case kEndNewSpell:
            this->DrawNewSpell();
            ++battle.endFrames;
            return;

        case kEndSpecials:
            if (noLevelRole())
            {
                this->LearnSpecials();

                if (!noLevelRole())
                {
                    this->audio.PlaySound(kLearnedSound, this->config.soundVolume);

                    if (!levelRole().IsDefeated())
                    {
                        levelRole().RestartMainAct(kLearnedPose, 1);
                    }
                }

                if (noLevelRole())
                {
                    battle.endFrames = 0;
                    battle.result = kEndMastery;
                    return;
                }
            }

            this->DrawNewSpecial();
            ++battle.endFrames;
            return;

        case kEndMastery:
            if (noLevelRole())
            {
                this->AnnounceMastery();

                if (!noLevelRole())
                {
                    this->audio.PlaySound(kLearnedSound, this->config.soundVolume);

                    if (!levelRole().IsDefeated())
                    {
                        levelRole().RestartMainAct(kLearnedPose, 1);
                    }
                }

                if (noLevelRole())
                {
                    battle.result = kEndLoot;
                    battle.endFrames = 0;

                    if (messages.lootCount != 0)
                    {
                        this->audio.PlaySound(kLearnedSound, this->config.soundVolume);
                    }

                    return;
                }
            }

            messages.DrawMastery();
            ++battle.endFrames;
            return;

        default:
            return;
    }
}

// Battle state 0x63 of BattleEndFrame (inline in the original); see there. The refine: every role's turn reset, its
// command dropped and its effects cleared, the slot records and the acting unit cleared, the role of pendingActionRow
// set on command 0xE at the enemy pendingAction (1-based), and the talisman it refines added to the inventory as the
// first loot.
void Game::EndCheckFrame()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    battle.unk53BF1C = 0;
    battle.autoBattle = 0;
    battle.unk53C4C0 = 1;

    if (this->fightTalk.wonByTalk == 0 && battle.unk53C048 == 0)
    {
        u32 defeated = 0;

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = this->battleEnemies[index];
            enemy.SetAC0(0);

            if (enemy.IsDefeated())
            {
                ++defeated;
            }
        }

        if (defeated != battle.enemyCount)
        {
            battle.result = 0;
            return;
        }
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        this->battleRoles[index].SetAC0(0);
    }

    battle.TakePendingAction();

    if (battle.pendingActionRow == 0 && battle.pendingAction == 0)
    {
        messages.craftSucceeded = 0;
        battle.result = kEndVictory;
        return;
    }

    if (messages.craftSucceeded != 0)
    {
        return;
    }

    const auto row = static_cast<std::size_t>(static_cast<u16>(battle.pendingActionRow));
    BattleRole& role = this->battleRoles[row];

    if (role.IsDefeated())
    {
        battle.result = kEndCraft;
        messages.craftSucceeded = kCraftFailed;
        return;
    }

    for (u32 index = 0; index < battle.roleCount; ++index)
    {
        BattleRole& each = this->battleRoles[index];
        each.ResetTurn(this->battleServices);
        each.CancelCommand();
        each.ExpireEffects(this->battleServices, 1);
    }

    battle.slotRecords.fill(BattleSlotRecord{});
    battle.ClearActing();
    battle.turnCommand = kRefineOrder;
    battle.actingUnit = -1;
    battle.turnTaken[row + static_cast<std::size_t>(BattleState::FirstRoleUnit)] = kRefineOrder;
    role.Retarget(0);
    role.SetCommand(battle.turnCommand);
    battle.unk53BFD0 = 1;
    battle.unk53BF60 = 0;
    battle.unk53BF5C = 0;
    battle.unk53BF74 = 0;
    battle.actingUnit = static_cast<i32>(row) + BattleState::FirstRoleUnit;
    battle.turnUnit = static_cast<i32>(row) + BattleState::FirstRoleUnit;
    battle.pendingOrders[row].unk00 = 1;
    battle.unk53BFB8 = 0;
    battle.aimTarget = static_cast<u16>(battle.pendingAction);
    battle.unk53BFC4 = 0;
    battle.unk53BFC0 = 0;
    battle.currentUnit = 0;
    const u16 proficiency = role.Unk1EProficiency(this->battleServices);
    const u8 level = this->party.roles[static_cast<std::size_t>(battle.partyRoles[row])].level;
    const auto aimed = static_cast<std::size_t>(static_cast<u16>(battle.pendingAction)) - 1;

    // An enemy 0 is read before the first enemy by the original: taken as refining nothing.
    const u16 talisman =
        aimed < this->battleEnemies.size()
            ? this->battleEnemies[aimed].RefineTalisman(this->battleServices, this->database, proficiency, level)
            : 0;

    if (talisman == 0)
    {
        messages.craftSucceeded = kCraftFailed;
        return;
    }

    ObjectRecord* record = AddOneItem(this->party.inventory, talisman, 1, this->database);
    messages.lootNames[0] =
        record != nullptr ? std::string_view(record->data.name.data(), ObjectName(*record).size()) : std::string_view();
    messages.craftSucceeded = 1;
    --battle.unk53C4B0;
}
}  // namespace swd3
