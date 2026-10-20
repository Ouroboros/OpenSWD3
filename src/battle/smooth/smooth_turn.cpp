#include "battle/smooth/smooth_turn.h"

#include <algorithm>
#include <array>
#include <span>
#include <string>

#include "battle/battle_turn_constants.h"
#include "battle/smooth/smooth_hud.h"
#include "battle/smooth/smooth_messages.h"
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

namespace swd3::smooth
{

namespace
{

constexpr i32 kFieldFadeStep = 2;
constexpr i32 kFieldFadeEnd = -0x20;
constexpr i32 kFuryFullBlock = 0x38;  // roleBlock[0] of a full fury gauge
constexpr u16 kHighlightBackground = 0xF000;
constexpr int kMenuColumnWidth = 0x30;
constexpr int kTabsDrop = 0x20;
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
constexpr int kListRowTop = 0x2C;
constexpr int kListNameX = 0x10;
constexpr int kListCountX = 0x90;
constexpr std::string_view kCountFormat = "%2d";  // swd32.exe 0x004A79CC
constexpr int kListBarTop = 0x28;
constexpr int kListBarWidth = 0xC2;
constexpr int kListBarRed = 0x1F;
constexpr int kListBarGreen = 0x14;
constexpr int kRefineBoxWidth = 0xF2;  // the 煉符 grid
constexpr int kRefineBoxHeight = 0x9C;
constexpr int kRefineFrameRight = 0xEE;
constexpr int kRefineFrameBottom = 0x98;
constexpr int kRefineTitleX = 0x6A;
constexpr u16 kRefineCommand = 0x1E;
constexpr int kRefineColumns = 2;
constexpr int kRefineBarLeft = 4;
constexpr int kRefineBarUp = 3;
constexpr int kRefineBarWidth = 0x6A;
constexpr std::u8string_view kNoObject = u8"無";  // swd32.exe 0x0049F9FC
constexpr int kSpecialBoxHeight = 0xAC;           // the 絕招 list
constexpr int kSpecialFrameBottom = 0xA8;
constexpr std::size_t kSpecialCommandName = 3;
constexpr u16 kSkillListEnd = 0xFFFF;
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
constexpr int kLevelUpTitleX = 0x108;
constexpr std::u8string_view kLevelUpTitle = u8"升級";       // swd32.exe 0x004A7A44
constexpr std::u8string_view kLevelUpLine = u8"%s升第%d級";  // swd32.exe 0x004A7A38: the name and the new level
constexpr int kLevelUpBodyBottom = 0xF4;
constexpr int kLevelUpGrowDivisor = 3;
constexpr int kLevelUpTextY = 0xDC;
constexpr std::string_view kBracketFormat = "[%s]";  // swd32.exe 0x004A7AAC
constexpr int kLearnedShadeX = 0xF6;
constexpr int kLearnedFrameLeft = 0xFA;
constexpr int kLearnedBodyBottom = 0xEC;
constexpr int kLearnedTextX = 0x104;
constexpr int kLearnedTextY = 0xBC;
constexpr int kLearnedSkillY = 0xD2;
constexpr u16 kLearnedSkillColor = 0xF000;
constexpr std::size_t kStatRolls = 9;  // the stats of a level-up
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

constexpr std::string_view kArrow = " ==> ";         // swd32.exe 0x004A7A50
constexpr std::string_view kNewValueFormat = "%4d";  // swd32.exe 0x004A7A4C
constexpr int kStatsArrowX = 0x136;
constexpr int kStatsNewX = 0x158;
constexpr int kDebugTop = 0xA;  // the debug overlay
constexpr int kDebugRolesX = 0x208;
constexpr int kDebugListX = 0x64;
constexpr int kDebugPartyX = 0x78;
constexpr int kDebugPartyRow = 0x1E;
constexpr int kDebugDrawRow = 0x32;
constexpr int kDebugStateX = 0xF0;
constexpr int kDebugRightX = 0x1A4;
constexpr int kDebugFarX = 0x1C2;
constexpr int kDebugFlagsY = 0x1E;
constexpr int kDebugMenuY = 0x32;
constexpr int kDebugUnitsY = 0x46;
constexpr int kDebugCountsY = 0x5A;
constexpr int kDebugAimY = 0x6E;
constexpr int kKilobyte = 1000;
constexpr i32 kMillisecondsPerSecond = 1000;
constexpr std::string_view kDebugNumber = "%d";
constexpr std::u8string_view kEnemyDebugLine = u8"生命:%-3d 鎖定:%-1d 命令:%-1d lv:%-2d";  // swd32.exe 0x004A792C
constexpr std::u8string_view kRoleDebugLine = u8"鎖定:%d 命令:%-3d";                       // swd32.exe 0x004A7918
constexpr std::u8string_view kAttackOrder = u8"攻擊順序:";                                 // swd32.exe 0x004A790C
constexpr std::u8string_view kPartyOrder = u8"我方等待順序:";                              // swd32.exe 0x004A78FC
constexpr std::u8string_view kDrawOrder = u8"排列順序:";                                   // swd32.exe 0x004A78F0
constexpr std::u8string_view kUnitsDebugLine = u8"目前角色:%d SEL:%d";                     // swd32.exe 0x004A78DC
constexpr std::u8string_view kFlagsDebugLine = u8"無敵:%d UM:%d 戰鬥:%d";                  // swd32.exe 0x004A78C4
constexpr std::string_view kMenuDebugLine = "fMenu:%d mMove%d";
constexpr std::string_view kCountsDebugLine = "MsD:%d dRole1:%d CanS:%d";
constexpr std::string_view kAimDebugLine = "MS:%d Stop:%d mStop%d";
constexpr std::string_view kLockDebugLine = "A.Lockc:%d ";
constexpr std::string_view kCacheDebugLine = "TswMem:%dK";
constexpr std::string_view kLevelUpDebugLine = "w"
                                               "Ll:%d iMn:%d";  // swd32.exe 0x004A7854 (one string)
constexpr std::string_view kBfDebugLine = "bf:%d";
constexpr std::string_view kFrameDebugLine = "FRAME:%d";
constexpr int kMenuPointerShiftX = 0x10;
constexpr int kMenuPointerShiftY = 0x30;
constexpr int kMenuHomeX = 0x122;
constexpr int kMenuHomeY = 0xA0;
constexpr i32 kMenuPointerLeft = 0x32;
constexpr i32 kMenuPointerRight = 0x23A;
constexpr i32 kMenuPointerTop = 0x1E;
constexpr i32 kMenuPointerBottom = 0x168;
constexpr int kMenuNameStep = 8;
constexpr int kMenuNameCenter = 5;
constexpr int kMenuNameShift = 2;
constexpr int kMenuNameY = 0xE;
constexpr int kCommandMenuX = 0xE;
constexpr int kCommandMenuY = 0x2A;
}  // namespace

TurnFrame::TurnFrame(Game& game)
    : game(game)
{
}

// What Game::FadeFieldOut decides of the frame; the draws into `scene`.
int TurnFrame::FadeFieldOutAdvance(Scene& scene)
{
    Game& game = this->game;
    BattleServices& services = game.battleServices;
    ScenePainter painter(services, scene);
    BattleState& battle = game.battleState;
    DrawOptions& options = painter.Options();
    options.tintRed = battle.blackFade;
    options.tintGreen = battle.blackFade;
    options.tintBlue = battle.blackFade;
    const std::span<u16> frame(game.drawSurface, static_cast<std::size_t>(Display::Width * Display::Height));
    painter.ShiftRed(frame, battle.blackFade);
    painter.ShiftGreenPairs(frame, options.tintGreen);
    painter.ShiftBluePairs(frame, options.tintBlue);
    battle.blackFade -= kFieldFadeStep;

    if (battle.blackFade > kFieldFadeEnd)
    {
        return 0;
    }

    battle.blackFade = 0;
    return 1;
}

// What Game::FieldFadeFrame decides of the frame; the draws into `scene`.
void TurnFrame::FieldFadeFrameAdvance(int counting, Scene& scene)
{
    Game& game = this->game;
    BattleServices& services = game.battleServices;
    ScenePainter painter(services, scene);
    ColorFade& fade = game.battleState.fieldFade;

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
    const std::span<u16> field(game.drawSurface, static_cast<std::size_t>(Display::Width * BattleState::FieldHeight));
    painter.TintPixels(field, red, green, blue);
}

// What Game::DrawListTabs decides of the frame; the draws into `scene`.
void TurnFrame::DrawListTabsAdvance(int x, int y, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    const int tabsY = y + kTabsDrop;
    int tabX = x + kTabWidth * (kListPages - 1);

    for (int tab = kListPages - 1; tab >= 0; --tab)
    {
        HudFrame(game.battleHud).DrawEffectAtAdvance(kTabEffect, tab, tabX, tabsY, 0, scene);
        tabX -= kTabWidth;
    }

    HudFrame(game.battleHud)
        .DrawEffectAtAdvance(
            kTabEffect,
            battle.listPage + kOpenTabFrame,
            x + kTabWidth * battle.listPage,
            tabsY,
            0,
            scene
        );
    game.mediumFont.SetBackground(kHighlightBackground);
    game.mediumFont.SetBackground(TextFont::NoBackground);
    messages.panelAct = ActState{};
    battle.unk53BD90 = kListPanelFrames;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter,
        x,
        y + kListBoxDrop,
        kListBoxWidth,
        kListBoxHeight,
        0,
        kListShade,
        kListShade,
        FillMode::Quarter,
        scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kListFrameRight,
        y + kListFrameBottom,
        0,
        kListFrameBorder,
        scene
    );
    battle.menuGrow = std::min(battle.menuGrow + kListGrowStep, kListGrowMost);
}

// What Game::DrawCommandMenu decides of the frame; the draws into `scene`.
void TurnFrame::DrawCommandMenuAdvance(int x, int y, int cursor, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    SceneFont font(game.largeFont, scene);

    if (battle.currentUnit == 0)
    {
        return;
    }

    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    BattleRole& role = game.battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];

    if (role.roleBlock[0] == kFuryFullBlock && role.MarkBit1() == 0)
    {
        battle.commandEnabled[3] = 1;
    }

    game.BuildCommandMenu();

    auto highlight = [&game, &font, &battle](int left, int top, std::u8string_view name)
    {
        font.SetBackground(kHighlightBackground);
        font.DrawText(game.drawSurface, left, top, name, battle.lightColor, TextFont::BoldOutline);
        font.SetBackground(TextFont::NoBackground);
    };

    for (std::size_t command = 0; command < BattleState::FixedCommandNames.size(); ++command)
    {
        const std::u8string_view name = BattleState::FixedCommandNames[command];
        const u16 color = battle.commandEnabled[command] == 1 ? battle.lightColor : battle.darkColor;
        const auto row = static_cast<int>(command % static_cast<std::size_t>(kMenuColumn));
        const auto column = static_cast<int>(command / static_cast<std::size_t>(kMenuColumn));
        const int top = y + kMenuRowHeight * row;
        font.DrawText(game.drawSurface, x + kMenuColumnWidth * column, top, name, color, TextFont::BoldShadow);

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
            font.DrawText(game.drawSurface, x + kMenuColumnWidth, top, name, battle.lightColor, TextFont::BoldShadow);
        }
        else
        {
            enabled = 0;
            font.DrawText(game.drawSurface, x + kMenuColumnWidth, top, name, battle.darkColor, TextFont::BoldShadow);
        }

        if (static_cast<int>(kFixedCommands + extra) + 1 == cursor)
        {
            highlight(x + kMenuColumnWidth - 1, top - 1, name);
        }
    }

    font.SetBackground(TextFont::NoBackground);
}

// What Game::DrawThrowList decides of the frame; the draws into `scene`.
void TurnFrame::DrawThrowListAdvance(int x, int y, int cursor, int top, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = game.battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    messages.panelAct = ActState{};
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter, x, y, kListBoxWidth, kThrowBoxHeight, 0, kThrowShade, kThrowShade, FillMode::Tint, scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kThrowTitleTop,
        x + kListFrameRight,
        y + kThrowTitleBottom,
        0,
        kListFrameBorder,
        scene
    );
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kListFrameRight,
        y + kListFrameBottom,
        0,
        kListFrameBorder,
        scene
    );
    font.DrawText(
        game.drawSurface,
        x + kThrowTitleX,
        y + kThrowTitleTop,
        kThrowTitle,
        kTitleColor,
        TextFont::BoldOutline
    );
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.roleListEntries = 0;
    role.CountObjects(kThrowCategory, battle.roleListEntries);
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
            game.drawSurface,
            x + kListNameX,
            rowY + kListRowTop,
            name,
            battle.lightColor,
            TextFont::BoldOutline
        );
        const std::string number = WinFormat(kCountFormat, {static_cast<i32>(static_cast<i16>(count))});
        font.DrawText(
            game.drawSurface,
            x + kListCountX,
            rowY + kListRowTop,
            number,
            battle.lightColor,
            TextFont::BoldOutline
        );

        if (cursor == row + 1)
        {
            font.DrawText(
                game.drawSurface,
                x + kListNameX - 1,
                rowY + kListRowTop - 1,
                name,
                battle.lightColor,
                TextFont::BoldOutline
            );
            font.SetBackground(TextFont::NoBackground);
            Box(game.painter,
                x,
                rowY + kListBarTop,
                kListBarWidth,
                kListBarHeight,
                kListBarRed,
                kListBarGreen,
                0,
                FillMode::Gradient,
                scene);
            battle.listCursor = next;
            battle.unk53BFB8 = 1;
        }

        font.SetBackground(TextFont::NoBackground);
        entry = next;
        ++row;
    }

    role.ShowAllItems();
    font.SetBackground(TextFont::NoBackground);
}

// What Game::DrawRefineList decides of the frame; the draws into `scene`.
void TurnFrame::DrawRefineListAdvance(int x, int y, int cursor, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = game.battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    messages.panelAct = ActState{};
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter, x, y, kRefineBoxWidth, kRefineBoxHeight, 0, kThrowShade, kThrowShade, FillMode::Tint, scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kThrowTitleTop,
        x + kRefineFrameRight,
        y + kThrowTitleBottom,
        0,
        kListFrameBorder,
        scene
    );
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kRefineFrameRight,
        y + kRefineFrameBottom,
        0,
        kListFrameBorder,
        scene
    );
    const std::string title = Big5(BattleState::AccessoryCommandNames[kRefineCommand - kFirstAccessoryCommand]);
    font.DrawText(game.drawSurface, x + kRefineTitleX, y + kThrowTitleTop, title, kTitleColor, TextFont::BoldOutline);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    std::string name;
    u16 count = 0;
    int object = 1;
    int piece = 0;
    role.ThrowableObject(0, 1, name, count);
    battle.roleListEntries = count;
    role.ShowAllItems();
    u16 pieces = 0;
    role.CountUsableObjects(pieces);
    role.ShowAllItems();
    count = 0;
    battle.listCursor = 1;
    int cell = 1;
    int columnX = x + kListNameX;

    for (int column = 0; column < kRefineColumns; ++column)
    {
        int rowY = y + kListRowTop;

        for (int row = 0; row < kRefineRows; ++row)
        {
            if (cell > battle.roleListEntries)
            {
                if (cell <= battle.roleListEntries + pieces)
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
                Box(game.painter,
                    columnX - kRefineBarLeft,
                    rowY - kRefineBarUp,
                    kRefineBarWidth,
                    kListBarHeight,
                    kListBarRed,
                    kListBarGreen,
                    0,
                    FillMode::Gradient,
                    scene);
                font.DrawText(game.drawSurface, columnX, rowY, name, battle.lightColor, TextFont::BoldOutline);
                font.SetBackground(TextFont::NoBackground);
                battle.unk53BFB8 = 1;
                battle.listCursor = piece == 1 ? object + 1 : object;

                if (battle.roleListEntries == 0)
                {
                    --battle.listCursor;
                }
            }

            font.DrawText(game.drawSurface, columnX, rowY, name, battle.lightColor, TextFont::BoldOutline);
            font.SetBackground(TextFont::NoBackground);
            ++cell;
            rowY += kListRowHeight;
        }

        columnX += kRefineColumnWidth;
    }

    font.SetBackground(TextFont::NoBackground);
    battle.roleListEntries = static_cast<u16>(battle.roleListEntries + pieces);
}

// What Game::DrawSpecialList decides of the frame; the draws into `scene`.
void TurnFrame::DrawSpecialListAdvance(int x, int y, int cursor, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = game.battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter, x, y, kListBoxWidth, kSpecialBoxHeight, 0, kThrowShade, kThrowShade, FillMode::Tint, scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kThrowTitleTop,
        x + kListFrameRight,
        y + kThrowTitleBottom,
        0,
        kListFrameBorder,
        scene
    );
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kListFrameRight,
        y + kSpecialFrameBottom,
        0,
        kListFrameBorder,
        scene
    );
    const std::string title = Big5(BattleState::FixedCommandNames[kSpecialCommandName]);
    font.DrawText(game.drawSurface, x + kThrowTitleX, y + kThrowTitleTop, title, kTitleColor, TextFont::BoldOutline);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.specialCount = 0;
    role.CountSkills(0, 1, battle.specialCount);
    role.ShowAllSkills();
    int row = 0;

    for (int index = 1;; ++index)
    {
        std::string name;
        u16 cost = 0;
        role.ListSkill(game.database, 0, 1, index, name, cost);

        if (cost == kSkillListEnd)
        {
            break;
        }

        if (cost != 0)
        {
            battle.listName = {};
            std::copy_n(name.begin(), std::min(name.size(), battle.listName.size() - 1), battle.listName.begin());
            const std::string_view shown(battle.listName.data());
            const int rowY = y + kSpecialRowHeight * row;
            font.DrawText(
                game.drawSurface,
                x + kListNameX,
                rowY + kListFrameTop,
                shown,
                battle.lightColor,
                TextFont::BoldOutline
            );

            if (cursor == row + 1)
            {
                font.DrawText(
                    game.drawSurface,
                    x + kListNameX - 1,
                    rowY + kListFrameTop - 1,
                    shown,
                    battle.lightColor,
                    TextFont::BoldOutline
                );
                Box(game.painter,
                    x,
                    rowY + kSpecialBarTop,
                    kSpecialBarWidth,
                    kListBarHeight,
                    kListBarRed,
                    kListBarGreen,
                    0,
                    FillMode::Gradient,
                    scene);
                font.SetBackground(TextFont::NoBackground);
                battle.unk53BFB8 = 1;
                battle.specialCursor = index;
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

// What Game::DrawEscortList decides of the frame; the draws into `scene`.
void TurnFrame::DrawEscortListAdvance(int x, int y, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter,
        kEscortShadeX,
        kEscortShadeY,
        kEscortShadeWidth,
        kEscortShadeHeight,
        0,
        kListShade,
        kListShade,
        FillMode::Blend,
        scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortTitleTop,
        kEscortFrameRight,
        kEscortTitleBottom,
        0,
        kListFrameBorder,
        scene
    );
    font.DrawText(
        game.drawSurface,
        kEscortTitleX,
        kEscortTitleTop,
        Big5(kEscortTitle),
        kTitleColor,
        TextFont::BoldOutline
    );
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortBodyTop,
        kEscortFrameRight,
        kEscortBodyBottom,
        0,
        kListFrameBorder,
        scene
    );
    Box(game.painter,
        x - kEscortBarLeft,
        y + kSpecialRowHeight * battle.guardCursor - kEscortBarUp,
        kEscortShadeWidth,
        kListBarHeight,
        kListBarRed,
        kListBarGreen,
        0,
        FillMode::Gradient,
        scene);

    for (int left = battle.escortCount; left > 0; --left)
    {
        font.SetAdvance(kEscortAdvance);
        const BattleRole& escort = game.battleRoles[battle.roleCount - static_cast<u32>(left)];
        const int rowY = y + kSpecialRowHeight * (battle.escortCount - left) + kEscortRowTop;
        font.DrawText(
            game.drawSurface,
            x + kEscortNameX,
            rowY,
            ObjectName(*escort.Data()),
            kWhite,
            TextFont::BoldOutline
        );
    }

    if (battle.escortCount == 1)
    {
        font.DrawText(
            game.drawSurface,
            x + kEscortNameX,
            y + kEscortNoneTop,
            Big5(kNoObject),
            kWhite,
            TextFont::BoldOutline
        );
    }

    font.SetAdvance(kListAdvance);
}

// What Game::DrawTargetTag decides of the frame; the draws into `scene`.
void TurnFrame::DrawTargetTagAdvance(int x, int y, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleServices& services = game.battleServices;
    ScenePainter painter(services, scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);
    // Without a current unit (the auto battle) the original reads the order before the first: taken as not set.
    const bool partyOrder =
        battle.currentUnit >= BattleState::FirstRoleUnit &&
        battle.pendingOrders[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)].unk00 == 1;

    if (partyOrder || battle.unk53BF88 == 1 || battle.aimTarget <= 0 ||
        battle.aimTarget > static_cast<i32>(battle.enemyCount))
    {
        return;
    }

    BattleEnemy& enemy = game.battleEnemies[static_cast<std::size_t>(battle.aimTarget - 1)];
    const std::string name = ObjectName(*enemy.Data());
    const int halves = static_cast<int>(name.size()) / 2;
    const int left = battle.mirrored == 1 ? kTagMirrorRight - kTagCharWidth * halves - x : x;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    const int width = kTagCharWidth * halves;
    Box(game.painter,
        left - kTagMargin,
        y - kTagMargin,
        width + 2 * kTagMargin,
        kListBarHeight,
        0,
        kThrowShade,
        kThrowShade,
        FillMode::Tint,
        scene);
    const int right = left + width;
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        left,
        y,
        right,
        y + kTagFrameHeight,
        0,
        kListFrameBorder,
        scene
    );
    font.SetAdvance(kTagCharWidth);
    font.DrawText(game.drawSurface, left + kTagNameX, y, name, kWhite, TextFont::BoldOutline);
    font.SetAdvance(kListAdvance);
    const u16 known =
        static_cast<u16>(FindCounterValue(game.party.fame.lists[Fame::FameList], static_cast<i16>(enemy.ActId())));

    if (known >= kKnownHitPoints)
    {
        i32 hitPoints = 0;
        i32 maximum = 0;
        enemy.GetHitPoints(hitPoints, maximum);
        const std::string text = WinFormat(kHitPointsFormat, {hitPoints, maximum});
        const int textX = battle.mirrored != 0 ? left - kTagHitPointsLeft : right + kTagHitPointsGap;
        font.DrawText(game.drawSurface, textX, y - kTagHitPointsUp, text, kWhite, TextFont::BoldOutline);
    }

    if (known < kKnownGauge)
    {
        return;
    }

    const auto gauge = static_cast<u16>(enemy.GaugeLength(game.battleServices));

    if (gauge == 0)
    {
        return;
    }

    const int barX = battle.mirrored != 0 ? left - kTagHitPointsLeft : right + kTagHitPointsGap;
    HudFrame(game.battleHud)
        .FillColor(barX, y + kTagGaugeTop, gauge, kTagGaugeHeight, painter.MakeColor(kTagGaugeRed, 0, 0), scene);
}

// What Game::DrawControlMenu decides of the frame; the draws into `scene`.
void TurnFrame::DrawControlMenuAdvance(int x, int y, int cursor, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    SceneFont font(game.mediumFont, scene);
    BattleEnemy& enemy = game.battleEnemies[static_cast<std::size_t>(static_cast<i16>(battle.unk4A762C))];

    auto entry = [&game, &font, x](int top, const std::string& text, bool chosen)
    {
        font.SetBackground(TextFont::NoBackground);
        font.DrawText(game.drawSurface, x, top, text, kWhite, TextFont::BoldOutline);

        if (chosen)
        {
            font.SetBackground(kHighlightBackground);
            font.DrawText(game.drawSurface, x, top, text, kWhite, TextFont::BoldOutline);
        }
    };

    HudFrame(game.battleHud)
        .DrawTileBoxAdvance(
            kControlTiles,
            x - kControlBoxLeft,
            y - kControlTitleUp,
            kControlColumns,
            kControlTitleRows,
            kControlFill,
            scene
        );
    font.DrawText(
        game.drawSurface,
        x,
        y - kControlTitleTextUp,
        Big5(BattleState::AccessoryCommandNames[kControlCommand - kFirstAccessoryCommand]),
        kWhite,
        TextFont::BoldOutline
    );
    HudFrame(game.battleHud)
        .DrawTileBoxAdvance(
            kControlTiles,
            x - kControlBoxLeft,
            y - kControlBoxLeft,
            kControlColumns,
            battle.unk4A7568,
            kControlFill,
            scene
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

        if (enemy.ItemName(game.database, choice, name, id) != 1)
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

        if (enemy.SkillName(game.database, choice, name) != 1)
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

// What Game::DrawSpellList decides of the frame; the draws into `scene`.
void TurnFrame::DrawSpellListAdvance(int x, int y, int cursor, int top, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    SceneFont font(game.mediumFont, scene);

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = game.battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.listEntries = 0;
    role.CountSkills(battle.listPage, 0, battle.listEntries);
    role.ShowAllSkills();
    int row = 0;

    for (int index = top + 1;; ++index)
    {
        std::string name;
        u16 cost = 0;
        const u16 icon = role.ListSkill(game.database, battle.listPage, 0, index, name, cost);

        if (cost == kSkillListEnd)
        {
            break;
        }

        battle.listName = {};
        std::copy_n(name.begin(), std::min(name.size(), battle.listName.size() - 1), battle.listName.begin());
        const std::string_view shown(battle.listName.data());
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
            HudFrame(game.battleHud).DrawPictureFrameAdvance(kElementIcons, icon - 1, x - kSpellIconLeft, rowY, scene);
        }

        const bool isShort = static_cast<i16>(have) < static_cast<i16>(cost);
        const u16 color = isShort ? battle.darkColor : battle.lightColor;
        font.DrawText(game.drawSurface, x + kSpellNameX, rowY, shown, color, TextFont::BoldOutline);
        const std::string number = WinFormat(kCostFormat, {static_cast<i32>(static_cast<i16>(cost))});
        font.DrawText(game.drawSurface, x + kListCountX, rowY, number, color, TextFont::BoldOutline);

        if (cursor == row + 1)
        {
            font.DrawText(game.drawSurface, x + kSpellNameX - 1, rowY - 1, shown, color, TextFont::BoldOutline);
            Box(game.painter,
                x - kSpellBarLeft,
                rowY - kSpellBarUp,
                kSpecialBarWidth,
                kListBarHeight,
                kListBarRed,
                kListBarGreen,
                0,
                FillMode::Gradient,
                scene);
            Box(game.painter,
                0,
                kDescriptionTop,
                Display::Width,
                kDescriptionHeight,
                0,
                kDescriptionGreen,
                kDescriptionBlue,
                FillMode::Gradient,
                scene);
            const std::string description =
                role.skills != nullptr ? ObjectDescription(game.databases.mapsData, role.skills->id) : std::string();
            const auto halves = static_cast<int>(description.size()) >> 1;
            font.DrawText(
                game.drawSurface,
                kDescriptionCentre - ((kListAdvance * halves) >> 1),
                kDescriptionTextTop,
                description,
                kWhite,
                TextFont::BoldOutline
            );
            font.SetBackground(TextFont::NoBackground);
            battle.unk53BFB8 = 1;
            battle.specialCursor = index;
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

// What Game::DrawItemList decides of the frame; the draws into `scene`.
void TurnFrame::DrawItemListAdvance(int x, int y, int cursor, int top, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.currentUnit == 0)
    {
        return;
    }

    BattleRole& role = game.battleRoles[static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit)];
    const int tabsY = y + kTabsDrop;
    int tabX = x + kTabWidth * (kItemPages - 1);

    for (int tab = kItemPages - 1; tab >= 0; --tab)
    {
        HudFrame(game.battleHud).DrawEffectAtAdvance(kTabEffect, tab, tabX, tabsY, 0, scene);
        tabX -= kTabWidth;
    }

    HudFrame(game.battleHud)
        .DrawEffectAtAdvance(
            kTabEffect,
            battle.listRole + kOpenTabFrame,
            x + kTabWidth * battle.listRole,
            tabsY,
            0,
            scene
        );
    font.SetBackground(kHighlightBackground);
    font.SetBackground(TextFont::NoBackground);
    messages.panelAct = ActState{};
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter,
        x,
        y + kListBoxDrop,
        kItemBoxWidth,
        kListBoxHeight,
        0,
        kListShade,
        kListShade,
        FillMode::Quarter,
        scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        x + kListFrameInset,
        y + kListFrameTop,
        x + kItemFrameRight,
        y + kListFrameBottom,
        0,
        kListFrameBorder,
        scene
    );
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    font.SetAdvance(kListAdvance);
    battle.roleListEntries = 0;
    role.CountObjects(battle.listRole, battle.roleListEntries);
    role.ShowAllItems();
    int row = 0;

    for (int index = top + 1;; ++index)
    {
        std::string name;
        u16 flags = 0;
        u16 count = 0;

        if (role.ListObject(battle.listRole, index, name, flags, count) == 0)
        {
            break;
        }

        if ((flags & kShownItem) != 0)
        {
            const std::string number = WinFormat(kCountFormat, {static_cast<i32>(static_cast<i16>(count))});
            const u16 color = (flags & kCostlyItem) != 0 ? battle.darkColor : battle.lightColor;
            const int rowY = y + kListRowHeight * row;
            font.DrawText(game.drawSurface, x + kListCountX, rowY + kListRowTop, number, color, TextFont::BoldOutline);
            font.DrawText(game.drawSurface, x + kListNameX, rowY + kListRowTop, name, color, TextFont::BoldOutline);

            if (cursor == row + 1)
            {
                font.DrawText(
                    game.drawSurface,
                    x + kListNameX - 1,
                    rowY + kListRowTop - 1,
                    name,
                    color,
                    TextFont::BoldOutline
                );
                font.SetBackground(TextFont::NoBackground);
                Box(game.painter,
                    x,
                    rowY + kListBarTop,
                    kItemBarWidth,
                    kListBarHeight,
                    kListBarRed,
                    kListBarGreen,
                    0,
                    FillMode::Gradient,
                    scene);
                battle.listCursor = index;
                battle.unk53BFB8 = 1;
                Box(game.painter,
                    0,
                    kDescriptionTop,
                    Display::Width,
                    kDescriptionHeight,
                    0,
                    kDescriptionGreen,
                    kDescriptionBlue,
                    FillMode::Gradient,
                    scene);
                const std::string description = role.inventoryNode != nullptr
                                                    ? ObjectDescription(game.databases.mapsData, role.inventoryNode->id)
                                                    : std::string();
                const auto halves = static_cast<int>(description.size()) >> 1;
                font.DrawText(
                    game.drawSurface,
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

// What Game::DrawVictoryPanel decides of the frame; the draws into `scene`.
void TurnFrame::DrawVictoryPanelAdvance(Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleServices& services = game.battleServices;
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter,
        kEscortShadeX,
        kEscortShadeY,
        kEscortShadeWidth,
        messages.panelHeight + kVictoryShadeExtra,
        0,
        kListShade,
        kListShade,
        FillMode::Blend,
        scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortTitleTop,
        kEscortFrameRight,
        kEscortTitleBottom,
        0,
        kListFrameBorder,
        scene
    );
    font.DrawText(
        game.drawSurface,
        kVictoryTitleX,
        kEscortTitleTop,
        Big5(kVictoryTitle),
        kTitleColor,
        TextFont::BoldOutline
    );
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kEscortBodyTop,
        kEscortFrameRight,
        messages.panelHeight + kEscortBodyTop,
        0,
        kListFrameBorder,
        scene
    );

    if ((battle.unk53BF12 & kGoldGiven) == 0)
    {
        game.audio.StopMusic();
        game.audio.StopAllSounds();
        game.audio.PlaySound(kVictorySound, game.config.soundVolume);

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            const u16 drop = game.battleEnemies[index].RollDrop(services);

            if (drop == 0)
            {
                continue;
            }

            const auto known = std::find(battle.lootIds.begin(), battle.lootIds.end(), drop);

            if (known != battle.lootIds.end())
            {
                AddOneItem(game.party.inventory, drop, 1, game.database);
                ++messages.lootCounts[static_cast<std::size_t>(known - battle.lootIds.begin())];
                continue;
            }

            // The original appends past its ten slots when more objects drop.
            const std::size_t slot = messages.lootCount;
            ObjectRecord* record = AddOneItem(game.party.inventory, drop, 1, game.database);

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
            BattleRole& role = game.battleRoles[index];

            if (role.autoControl == 1 || role.guestControl == 1 || role.IsDefeated())
            {
                continue;
            }

            const auto partyRole = static_cast<std::size_t>(battle.partyRoles[index]);
            RoleRecord& record = game.party.roles[partyRole];

            if (record.level < battle.timingWindow)
            {
                record.totalExperience += battle.unk53BF14;
            }

            if (role.StepWornCounters(game.party.fame.lists[Fame::ProficiencyList], battle.unk53BF16) == 1)
            {
                battle.accessoryGained = 1;
            }

            i32& battles = game.party.battleBlocks[partyRole].unk00[kBattlesFought];
            ++battles;

            if (battle.enemyCount >= kManyEnemies)
            {
                ++battles;
            }

            role.ExpireEffects(services, 1);
            role.RestartMainAct(1, 1);
        }

        battle.endFrames = 0;
        battle.unk53BF12 = static_cast<u16>(battle.unk53BF12 | kGoldGiven);
        game.party.variables[kGoldVariable] += battle.unk53BF12 & kGoldMask;
    }

    if (messages.GrowPanel(kEscortBodyTop, kVictoryBodyBottom, kVictoryGrowDivisor) != 1)
    {
        return;
    }

    font.DrawText(
        game.drawSurface,
        kVictoryTextX,
        kVictoryExperienceY,
        WinFormat(kExperienceLine, {static_cast<i32>(battle.unk53BF14)}),
        kTitleColor,
        TextFont::BoldOutline
    );
    font.DrawText(
        game.drawSurface,
        kVictoryTextX,
        kVictoryGoldY,
        WinFormat(kGoldLine, {static_cast<i32>(battle.unk53BF12 & kGoldMask)}),
        kTitleColor,
        TextFont::BoldOutline
    );
    font.DrawText(
        game.drawSurface,
        kVictoryTextX,
        kVictoryProficiencyY,
        WinFormat(kProficiencyLine, {static_cast<i32>(battle.unk53BF16)}),
        kTitleColor,
        TextFont::BoldOutline
    );
}

// What Game::DrawLevelUpPanel decides of the frame; the draws into `scene`.
void TurnFrame::DrawLevelUpPanelAdvance(Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.levelUpRole != kNoLevelUp || battle.unk53BFFC == 1)
    {
        messages.panelAct.id = BattleMessages::PanelAct;
        messages.panelAct.action = 0;
        game.acts.Run(messages.panelAct);
        Box(game.painter,
            kEscortShadeX,
            kEscortShadeY,
            kEscortShadeWidth,
            messages.panelHeight + kVictoryShadeExtra,
            0,
            kListShade,
            kListShade,
            FillMode::Blend,
            scene);
        WindowFrame(
            game.painter,
            game.archive,
            messages.panelAct.frame,
            kEscortFrameLeft,
            kEscortTitleTop,
            kEscortFrameRight,
            kEscortTitleBottom,
            0,
            kListFrameBorder,
            scene
        );
        font.DrawText(
            game.drawSurface,
            kLevelUpTitleX,
            kEscortTitleTop,
            Big5(kLevelUpTitle),
            kTitleColor,
            TextFont::BoldOutline
        );
        WindowFrame(
            game.painter,
            game.archive,
            messages.panelAct.frame,
            kEscortFrameLeft,
            kEscortBodyTop,
            kEscortFrameRight,
            messages.panelHeight + kEscortBodyTop,
            0,
            kListFrameBorder,
            scene
        );
    }

    if (messages.GrowPanel(kEscortBodyTop, kLevelUpBodyBottom, kLevelUpGrowDivisor) != 1 ||
        battle.levelUpRole == kNoLevelUp)
    {
        return;
    }

    const auto role = static_cast<std::size_t>(battle.partyRoles[battle.levelUpRole]);
    const Party::Name& name = game.party.names[role];
    const std::string roleName(name.data(), std::find(name.begin(), name.end(), '\0') - name.begin());
    const std::string text = WinFormat(kLevelUpLine, {roleName, static_cast<i32>(game.party.roles[role].level)});
    font.DrawText(game.drawSurface, kVictoryTextX, kLevelUpTextY, text, kTitleColor, TextFont::BoldOutline);
}

// What Game::DrawLearnedBox decides of the frame; the draws into `scene`.
void TurnFrame::DrawLearnedBoxAdvance(std::u8string_view line, bool sound, Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.unk53BFFC != 1)
    {
        return;
    }

    if (sound && messages.panelHeight == 0)
    {
        game.audio.PlaySound(kLearnedSound, game.config.soundVolume);
    }

    const auto role = static_cast<std::size_t>(battle.partyRoles[battle.levelUpRole]);
    const Party::Name& roleName = game.party.names[role];
    const std::string name(roleName.data(), std::find(roleName.begin(), roleName.end(), '\0') - roleName.begin());
    const std::string text = WinFormat(line, {name});
    const int halves = static_cast<int>(text.size()) / 2;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter,
        kLearnedShadeX,
        kEscortShadeY,
        kTagCharWidth * halves + 2 * kTagMargin,
        messages.panelHeight + 2 * kTagMargin,
        0,
        kListShade,
        kListShade,
        FillMode::Blend,
        scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kLearnedFrameLeft,
        kEscortTitleTop,
        kTagCharWidth * halves + kLearnedFrameLeft,
        messages.panelHeight + kEscortTitleTop,
        0,
        kListFrameBorder,
        scene
    );

    if (messages.GrowPanel(kEscortTitleTop, kLearnedBodyBottom, kLevelUpGrowDivisor) != 1)
    {
        return;
    }

    font.DrawText(game.drawSurface, kLearnedTextX, kLearnedTextY, text, kWhite, TextFont::BoldOutline);
    const std::string skill = WinFormat(kBracketFormat, {battle.lastLearned.name});
    const int quarters = static_cast<int>(skill.size()) >> 2;
    const int skillX = ((kListAdvance * halves) >> 1) - kListAdvance * quarters + kLearnedFrameLeft;
    font.DrawText(game.drawSurface, skillX, kLearnedSkillY, skill, kLearnedSkillColor, TextFont::BoldOutline);
}

// What Game::DrawLevelUpStats decides of the frame; the draws into `scene`.
void TurnFrame::DrawLevelUpStatsAdvance(Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleState& battle = game.battleState;
    BattleMessages& messages = game.battleMessages;
    SceneFont font(game.mediumFont, scene);

    if (battle.levelUpRole == kNoLevelUp)
    {
        return;
    }

    const auto partyRole = static_cast<std::size_t>(battle.partyRoles[battle.levelUpRole]);
    const RoleRecord& now = game.party.roles[partyRole];
    const RoleRecord& before = battle.levelUpBefore;
    std::array<i16, kStatRolls>& rolls = battle.statRolls;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    game.acts.Run(messages.panelAct);
    Box(game.painter,
        kEscortShadeX,
        kStatsShadeY,
        kStatsShadeWidth,
        messages.panelHeight + kVictoryShadeExtra,
        0,
        kListShade,
        kListShade,
        FillMode::Blend,
        scene);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kStatsTitleTop,
        kStatsFrameRight,
        kStatsTitleBottom,
        0,
        kListFrameBorder,
        scene
    );
    const Party::Name& name = game.party.names[partyRole];
    const std::string roleName(name.data(), std::find(name.begin(), name.end(), '\0') - name.begin());
    font.DrawText(game.drawSurface, kStatsTitleX, kStatsTitleTop, roleName, kTitleColor, TextFont::BoldOutline);
    WindowFrame(
        game.painter,
        game.archive,
        messages.panelAct.frame,
        kEscortFrameLeft,
        kStatsBodyTop,
        kStatsFrameRight,
        messages.panelHeight + kStatsBodyTop,
        0,
        kListFrameBorder,
        scene
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
        static_cast<u16>(before.wisdom),
        static_cast<u16>(before.agility),
    };

    for (std::size_t line = 0; line < kStatLines; ++line)
    {
        const int y = kStatsFirstY + kStatsLineHeight * static_cast<int>(line);
        font.DrawText(
            game.drawSurface,
            kStatsTextX,
            y,
            WinFormat(kStatLabels[line], {old[line]}),
            kWhite,
            TextFont::BoldOutline
        );
    }

    const bool showNow = battle.levelUpFrames == kShowNow;

    if (showNow)
    {
        rolls = {};
    }
    else
    {
        if (battle.levelUpFrames == kRollStart - 1)
        {
            rolls[0] = static_cast<i16>(now.maxHitPoints - before.maxHitPoints);
            rolls[1] = static_cast<i16>(now.maxSpirit - before.maxSpirit);
            rolls[2] = static_cast<i16>(now.maxStamina - before.maxStamina);
            rolls[3] = static_cast<i16>(now.attack - before.attack);
            rolls[4] = static_cast<i16>(now.defense - before.defense);
            rolls[5] = static_cast<i16>(now.wisdom - before.wisdom);
            rolls[6] = static_cast<i16>(now.agility - before.agility);
            rolls[7] = static_cast<i16>(now.rapport - before.rapport);
            rolls[8] = static_cast<i16>(now.reaction - before.reaction);
        }

        if (battle.levelUpFrames < kRollStart && ++battle.levelUpFrames < kRollStart)
        {
            return;
        }
    }

    auto rise = [&game, &font](int y, i32 value)
    {
        font.DrawText(game.drawSurface, kStatsArrowX, y, kArrow, kWhite, TextFont::BoldOutline);
        font.DrawText(
            game.drawSurface,
            kStatsNewX,
            y,
            WinFormat(kNewValueFormat, {value}),
            kLearnedSkillColor,
            TextFont::BoldOutline
        );
    };

    auto tick = [&game]
    {
        game.audio.PlaySound(kRollSound, game.config.soundVolume);
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
    attribute(5, now.wisdom, before.wisdom, 5, rolls[4] == 0);
    attribute(6, now.agility, before.agility, 6, rolls[5] == 0);
}

// What Game::DrawBattleDebug decides of the frame; the draws into `scene`.
void TurnFrame::DrawBattleDebugAdvance(Scene& scene)
{
    Game& game = this->game;
    // The menus and lists stay where they are: a highlight jumps to its entry, as in the original.
    const Scene::Still still(scene);
    BattleServices& services = game.battleServices;
    BattleState& battle = game.battleState;
    SceneFont font(game.mediumFont, scene);

    auto text = [&game, &font](int x, int y, const std::string& line)
    {
        font.DrawText(game.drawSurface, x, y, line, kWhite, TextFont::BoldOutline);
    };

    if (battle.unk5244E0 == 1)
    {
        font.SetBackground(TextFont::NoBackground);
        font.SetShadow(0);
        int y = kDebugTop;

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = game.battleEnemies[index];
            battle.lastDefeatedObject = enemy.Data();
            i32 hitPoints = 0;
            i32 maximum = 0;
            enemy.GetHitPoints(hitPoints, maximum);
            const i32 level = battle.lastDefeatedObject->data.level;
            text(
                kDebugTop,
                y,
                WinFormat(
                    kEnemyDebugLine,
                    {hitPoints, static_cast<i32>(enemy.GetWord2A74()), static_cast<i32>(enemy.GetCommandA()), level}
                )
            );
            y += kListRowHeight;
        }

        y = kDebugTop;

        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = game.battleRoles[index];
            text(
                kDebugRolesX,
                y,
                WinFormat(kRoleDebugLine, {static_cast<i32>(role.GetWord2A74()), static_cast<i32>(role.GetCommandA())})
            );
            y += kListRowHeight;
        }

        const auto enemies = static_cast<int>(battle.enemyCount);
        const auto units = static_cast<int>(battle.enemyCount + battle.roleCount);
        const int recordsY = kListRowHeight * enemies + kDebugTop;
        text(kDebugTop, recordsY, Big5(kAttackOrder));

        for (int at = 0; at < units; ++at)
        {
            text(
                kDebugListX + kListRowHeight * at,
                recordsY,
                WinFormat(kDebugNumber, {battle.unk524788[static_cast<std::size_t>(at)].unit})
            );
        }

        const int partyY = kListRowHeight * enemies + kDebugPartyRow;
        text(kDebugTop, partyY, Big5(kPartyOrder));

        for (u32 at = 0; at < battle.roleCount; ++at)
        {
            text(
                kDebugPartyX + kListRowHeight * static_cast<int>(at),
                partyY,
                WinFormat(kDebugNumber, {battle.roleOrder[at]})
            );
        }

        const int drawY = kListRowHeight * enemies + kDebugDrawRow;
        text(kDebugTop, drawY, Big5(kDrawOrder));

        for (int at = 0; at < units; ++at)
        {
            text(
                kDebugListX + kListRowHeight * at,
                drawY,
                WinFormat(kDebugNumber, {battle.drawOrder[static_cast<std::size_t>(at)]})
            );
        }

        text(kDebugStateX, kDebugUnitsY, WinFormat(kUnitsDebugLine, {battle.actingUnit, battle.currentUnit}));

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = game.battleEnemies[index];
            enemy.GetPosition(battle.unk53BF4A, battle.unk53BF4E);
            const int row = Display::Width * static_cast<i16>(battle.unk53BF4E);
            const int gauge = enemy.GaugeLength(services);

            // The original writes the pixels wherever the place puts them.
            for (int pixel = 0; pixel < gauge; ++pixel)
            {
                for (const int at :
                     {row + pixel + static_cast<i16>(battle.unk53BF4A),
                      row + pixel + Display::Width + static_cast<i16>(battle.unk53BF4A)})
                {
                    if (at >= 0 && at < Display::Width * Display::Height)
                    {
                        game.drawSurface[at] = kGaugePixel;
                    }
                }
            }
        }

        text(
            kDebugStateX,
            kDebugFlagsY,
            WinFormat(
                kFlagsDebugLine,
                {battle.unk53AF68, static_cast<i32>(battle.darkenUnit), game.fightTalk.battleRuns}
            )
        );
        text(kDebugStateX, kDebugMenuY, WinFormat(kMenuDebugLine, {battle.result, battle.unk53BFBC}));
        text(
            kDebugStateX,
            kDebugCountsY,
            WinFormat(
                kCountsDebugLine,
                {static_cast<i32>(battle.enemiesGone),
                 static_cast<i32>(battle.unk4A7626),
                 static_cast<i32>(battle.sweptUnits)}
            )
        );
        text(
            kDebugStateX,
            kDebugAimY,
            WinFormat(kAimDebugLine, {battle.aimTarget, battle.unk53BFC0, battle.unk53BFC4})
        );
        text(kDebugRightX, kDebugFlagsY, WinFormat(kLockDebugLine, {battle.unk53AE74}));
        text(kDebugRightX, kDebugMenuY, WinFormat(kCacheDebugLine, {game.archive.CacheBytes() / kKilobyte}));
        text(
            kDebugRightX,
            kDebugUnitsY,
            WinFormat(
                kLevelUpDebugLine,
                {static_cast<i32>(static_cast<i8>(battle.levelUpRole)),
                 static_cast<i32>(static_cast<i16>(battle.anyStateRole))}
            )
        );
        text(kDebugFarX, kDebugCountsY, WinFormat(kBfDebugLine, {battle.unk53BDA0}));
        text(kDebugStateX, kDebugTop, WinFormat(kFrameDebugLine, {kMillisecondsPerSecond / battle.frameMilliseconds}));
    }

    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
}

// What Game::DrawCommandPanel decides of the frame; the draws into `scene`.
void TurnFrame::DrawCommandPanelAdvance(i32 nameIndex, Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    i32 grown = battle.menuGrow;

    if (grown != kMenuOpened)
    {
        if (battle.unk53BFB0 == 0)
        {
            battle.menuX = game.state.pointerX - kMenuPointerShiftX;
            battle.menuY = game.state.pointerY - kMenuPointerShiftY;
            battle.unk53BFB0 = 1;
        }

        // The rebuild (deviations.md): after a key the menu opens at its place, whatever the pointer did.
        if (battle.promptFrames >= kMenuIdleFrames || !battle.menuByMouse)
        {
            battle.menuX = kMenuHomeX;
            battle.menuY = kMenuHomeY;
        }

        if (battle.pointerX < kMenuPointerLeft || battle.pointerX > kMenuPointerRight ||
            battle.pointerY < kMenuPointerTop || battle.pointerY > kMenuPointerBottom)
        {
            battle.menuX = kMenuHomeX;
            battle.menuY = kMenuHomeY;
            game.input.SetMousePosition(kMenuHomeX, kMenuHomeY);
            grown = battle.menuGrow;
        }
    }

    if (grown >= kMenuOpened)
    {
        grown = kMenuOpened;
        battle.menuGrow = kMenuOpened;
    }

    if (battle.unk53BD98 < 0)
    {
        battle.unk53BD98 = 0;
    }

    HudFrame(game.battleHud).DrawTallGaugeAdvance(battle.menuX, battle.menuY, grown, scene);

    if (battle.menuGrow < kMenuOpened)
    {
        ++battle.menuGrow;
        --battle.unk53BD98;
        return;
    }

    SceneFont font(game.mediumFont, scene);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    const Party::Name& name =
        game.party.names[static_cast<std::size_t>(battle.partyRoles[static_cast<std::size_t>(nameIndex)])];
    const std::string text(name.data(), std::find(name.begin(), name.end(), '\0') - name.begin());
    const int halfLength = static_cast<int>(text.size()) >> 1;
    const int x = battle.menuX + kMenuNameStep * (kMenuNameCenter - (halfLength - kMenuNameShift));
    font.DrawText(game.drawSurface, x, battle.menuY + kMenuNameY, text, kTitleColor, TextFont::BoldOutline);
    this->DrawCommandMenuAdvance(battle.menuX + kCommandMenuX, battle.menuY + kCommandMenuY, battle.menuCommand, scene);
}

}  // namespace swd3::smooth
