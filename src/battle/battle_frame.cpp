#include "game/game.h"

#include <span>

#include "core/format.h"
#include "game/screenshot.h"
#include "platform/display.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr int kQuitFlag = 0x11;  // BattleFrame: game flag 0x11 leaves the battle
constexpr int kQuitBattle = 3;
constexpr u16 kSlotRecordWait = 0x10;  // frames between the slot records taken
constexpr u16 kBottomBar = 0x234D;
constexpr int kBottomBarY = 0x180;
constexpr i32 kCurrentPanelAct = 0x233B;  // the frame of the current role's panel
constexpr int kCurrentPanelTop = 0x18D;
constexpr int kCurrentPanelWidth = 0x74;
constexpr int kCurrentPanelBottom = 0x1D3;
constexpr int kCurrentPanelBorder = static_cast<int>(0x80000008u);
constexpr int kPanelCursor = 0x2391;
constexpr int kTimerX = 0x190;
constexpr int kTimerY = 8;
constexpr int kSecondTimerX = 0xA;
constexpr int kFadeLight = 0x18;
constexpr int kFadeFrames = 8;
constexpr int kQuakePanel = 0x100;
constexpr u16 kSnapBase = 0x3E8;
constexpr std::u8string_view kSnapName = u8"c:\\snap\\%d.bmp";  // swd32.exe 0x004A7788

}  // namespace

// swd32.exe 0x0045B5E0: one frame of every unit in the draw order: a role (units 8 and up, below the role count)
// runs RoleFrame, an enemy below the enemy count EnemyFrame.
void Game::UpdateUnits()
{
    BattleState& battle = this->battleState;
    const u32 units = battle.roleCount + battle.enemyCount;

    for (u32 index = 0; index < units; ++index)
    {
        const i32 unit = battle.drawOrder[index];

        if (unit >= BattleState::FirstRoleUnit)
        {
            if (unit - BattleState::FirstRoleUnit < static_cast<i32>(battle.roleCount))
            {
                this->RoleFrame(unit - BattleState::FirstRoleUnit);
            }
        }
        else if (unit < static_cast<i32>(battle.enemyCount))
        {
            this->EnemyFrame(unit);
        }
    }
}

// swd32.exe 0x00453200: one frame of the battle; it returns unk53BDC8 (1, or the end CheckBattleEnd set), 3 when game
// flag 0x11 leaves it, or the debug keys' 0 (the frame not drawn). The battle music starts again when it stopped; the
// pointer, the keys and the auto battle are handled and the units measured and sorted. Unless paused: an idle battle
// takes the next slot record every kSlotRecordWait frames as the acting unit (unk53C044 lets the gauges run while none
// acts and no panel is open), the background, the units (the acting ones sorted around it), the hits, the bottom bar,
// the current role's panel frame and cursor, the role panels, the menu, the end frame, the messages, the field's
// lines and heads, the talk, the resistance panel and the timers are drawn; the end is checked, a finished fade of
// the stone sides lights the field again, the frame is shown (shaken while quaking) and saved when asked.
int Game::BattleFrame()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;
    battle.unk53BDC8 = 1;

    if (this->audio.IsMusicStopped() && battle.unk53C4C0 == 0)
    {
        this->PlayMusic(battle.battleMusic);
        this->audio.SetMusicVolume(this->config.musicVolume);
    }

    this->MouseHover();
    this->BattleKeys();
    this->AutoBattleTurn();
    this->MeasureUnits();
    battle.SortDrawOrder();
    const int keys = this->BattleDebugKeys();

    if (keys == 0)
    {
        return keys;
    }

    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);

    if (battle.paused == 1)
    {
        return battle.unk53BDC8;
    }

    if (battle.unk53BFC0 == 1 && battle.unk53AE70 == -1)
    {
        battle.unk53BFC0 = 0;
    }

    if (battle.unk524788[0].unit != -1 && battle.unk53BF74 == 0 && this->fightTalk.unk4A7B58 == 1 &&
        battle.unk53BFC0 == 0)
    {
        if (battle.unk53BF1A < kSlotRecordWait)
        {
            ++battle.unk53BF1A;
        }
        else
        {
            BattleSlotRecord acting{
                battle.unk53AE70,
                battle.unk53AE74,
                battle.unk53AE78,
                static_cast<u16>(battle.unk53AE7A),
                battle.unk53AE7C,
                battle.unk53AE80,
                battle.unk53AE84,
                battle.unk53AE88
            };

            this->TakeSlotRecord(acting);
            battle.unk53AE70 = acting.unit;
            battle.unk53AE74 = acting.unk04;
            battle.unk53AE78 = acting.kind;
            battle.unk53AE7A = static_cast<i16>(acting.unk0A);
            battle.unk53AE7C = acting.unk0C;
            battle.unk53AE80 = acting.unk10;
            battle.unk53AE84 = acting.unk14;
            battle.unk53AE88 = acting.unk18;

            if (battle.unk53AE70 != -1)
            {
                battle.unk53BF1A = 0;
                battle.unk53BF74 = 1;
                this->fightTalk.unk53CE70 = battle.unk53AE70;
            }
        }
    }

    battle.unk53C044 = battle.unk53AE70 == -1 && battle.currentUnit == 0 ? 1 : 0;
    this->DrawBattleBackground(battle.shake);

    if (battle.unk53BF64 == 1 || battle.unk53BF68 == 1)
    {
        this->SortActingDrawOrder();
    }

    this->UpdateUnits();
    this->CheckStoneSides();
    this->ApplyHits();

    if (this->HitFrame() == 1)
    {
        battle.spellStage = static_cast<u16>(battle.spellStage | BattleState::SpellOver);
    }

    hud.DrawPictureFrame0(kBottomBar, 0, kBottomBarY);

    if (battle.currentUnit != 0)
    {
        ActState& panel = this->battleMessages.panelAct;
        panel = ActState{};
        panel.id = kCurrentPanelAct;
        panel.action = 0;
        this->acts.Run(panel);
        const auto unit = static_cast<std::size_t>(battle.currentUnit - BattleState::FirstRoleUnit);
        const int x = BattleState::PortraitX[static_cast<std::size_t>(battle.partyRoles[unit])];
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            x,
            kCurrentPanelTop,
            x + kCurrentPanelWidth,
            kCurrentPanelBottom,
            0,
            kCurrentPanelBorder
        );

        if (battle.grayBandShown == 0 && this->battleRoles[unit].Flag26B8() == 0)
        {
            hud.DrawCursorAnimation(kPanelCursor, battle.panelPlaces[2 * unit], battle.panelPlaces[2 * unit + 1]);
        }
    }

    this->DrawRolePanels();
    this->DrawBattleMenu();
    this->BattleEndFrame();
    this->battleMessages.Draw();
    this->fieldView.DrawLines();
    this->fieldView.DrawRoleHeads();
    this->talk.Draw();
    this->battleMessages.DrawResistancePanel();
    this->fieldView.DrawTimer(kTimerX, kTimerY, 0);
    this->fieldView.DrawTimer(kSecondTimerX, kTimerY, 1);

    if (this->state.GameFlag(kQuitFlag))
    {
        return kQuitBattle;
    }

    if (this->config.debugMode == 1)
    {
        this->DrawBattleDebug();
    }

    this->CheckBattleEnd();
    this->DrawBattlePrompt();

    if (battle.fieldFade.frames <= 0 && battle.fadingField == 1)
    {
        battle.StartFieldFade(kFadeLight, kFadeLight, kFadeLight, 0, 0, 0, kFadeFrames);
        battle.fadingField = 0;
    }

    this->FieldFadeFrame(1);

    if (battle.quaking == 1 || (battle.panelFlags & kQuakePanel) != 0)
    {
        this->QuakeFrame();
    }
    else
    {
        this->ShowFrame();
    }

    if (battle.unk53BFE8 == 1)
    {
        const std::string name = WinFormat(kSnapName, {static_cast<i32>(++battle.unk53C49A + kSnapBase)});
        const std::span<const u16> pixels(
            this->drawSurface,
            static_cast<std::size_t>(this->painter.ScreenPitch()) * Display::Height
        );
        SaveScreenshot(
            this->painter,
            pixels,
            Display::Width,
            Display::Height,
            name,
            [this]
            {
                this->AudioPump();
            }
        );
        battle.unk53BFE8 = 0;
    }

    return battle.unk53BDC8;
}

}  // namespace swd3
