#include "game/game.h"

#include <span>

#include "battle/battle_items.h"
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
constexpr std::size_t kClearedTurnUnits = 8;                    // EndBattle: 0x0053AF30..0x0053AF57
constexpr std::size_t kSharedEscortWords = 5;
constexpr std::array<i32, 6> kOpeningPlaces = {0x60, 0x60, 0xF0, 0xFA, 0xF2, 0x64};
constexpr u16 kOpeningPanels = 6;
constexpr i32 kAnswersStart = 2;
constexpr i32 kHealPage = 2;
constexpr u16 kNoUnit = 0xFFFF;

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

// swd32.exe 0x0045B630: the battle is over: its surfaces, the background animation, the projector and the background
// picture freed, the state of the battle cleared field by field (the menu and list cursors back to 1, the hit
// records to -1, unk4A7580..unk4A7594 to their opening places 0x60, 0x60, 0xF0, 0xFA, 0xF2, 0x64: the second
// differs from the exe's 0x180), the archives closed, the sounds and music stopped, the database files closed.
void Game::EndBattle()
{
    BattleState& battle = this->battleState;
    BattleMessages& messages = this->battleMessages;
    ReleaseBattleSurfaces(battle);
    this->battleHud.FreeAnimation(battle.backgroundAnimation);
    battle.projector.Release();
    battle.background = Picture{};
    battle.unk502944 = 0;
    battle.unk502948 = 0;
    battle.unk5240C4 = 0;
    battle.unk50294C = 0;
    battle.unk520DD0.fill(0);
    battle.unk524200.fill(0);
    battle.unk53AF30 = 0;
    battle.unk53AF34 = 0;
    std::fill_n(battle.unk53AF38.begin(), kClearedTurnUnits, 0);
    battle.unk502950 = 0;
    battle.unk5244E8.fill(-1);
    battle.enemyOrders.fill(0);
    battle.enemyPlaces.fill(UnitPlacement{});
    battle.rolePlaces.fill(UnitPlacement{});
    battle.unk524788.fill(BattleSlotRecord{0, 0, 0, 0, 0, 0, 0, 0});
    battle.ClearActing();
    battle.unk5202A8.fill(ActState{});
    battle.unk524980.fill(ActState{});
    battle.projector = Projector();
    battle.pointerX = 0;
    battle.pointerY = 0;
    battle.formation = BattleFormation{};
    battle.unk4FF168.fill(0);
    battle.unk52544C.fill(0);
    battle.unk524324.fill(0);
    battle.unk4FE5D4.fill(0);
    battle.actionQueue.fill(0);
    battle.commandNames = {};
    battle.commandKinds = {};
    messages.messages.reset();
    messages.unk5214FC.fill(0);
    battle.hitPointsRolling.fill(0);
    battle.hitPoints.fill(0);
    battle.nameFlashes.fill(0);
    battle.unk52100C.fill(0);
    battle.unk4FF318.fill(0);
    battle.unk53AED8 = 0;
    battle.unk53AEDC = 0;
    battle.unk53AEE0 = 0;
    battle.unk4FF588.fill(0);
    battle.unk525470 = ActState{};
    battle.spiritRolling.fill(0);
    battle.spirit.fill(0);
    battle.unk53AEE4 = 0;
    battle.unk4FF578.fill(1);
    battle.unk4A7568 = kAnswersStart;
    battle.unk4A7570 = kHealPage;
    this->talk.choice = 0;
    battle.panelFlags = 0;
    battle.unk53BCDC = 0;
    battle.enemyCount = 0;
    battle.roleCount = 0;
    battle.guestCount = 0;
    battle.unk53BCE8 = 0;
    battle.result = 0;
    battle.unk4A7548 = 1;
    battle.unk4A754C = 1;
    battle.unk53BCF4 = 0;
    battle.unk4A7550 = 1;
    battle.unk4A7558 = 1;
    battle.unk4A755C = 1;
    battle.unk4A756C = 1;
    battle.unk4A7574 = -1;
    battle.unk53BD04 = 0;
    battle.unk53BD08 = 0;
    battle.unk53BD0C = 0;
    battle.unk53BD10 = 0;
    battle.unk53BD14 = 0;
    battle.unk53BD24 = 0;
    battle.unk53BD28 = 0;
    battle.unk53BD40 = 0;
    battle.unk4A7580 = kOpeningPlaces[0];
    battle.unk53BD4C = 0;
    battle.unk53BD50 = 0;
    battle.currentUnit = 0;
    battle.unk53BD58 = 0;
    battle.unk4A7584 = kOpeningPlaces[1];
    battle.shake = 0;
    battle.unk4A7588 = kOpeningPlaces[2];
    battle.quakePhase = 0;
    battle.unk4A758C = kOpeningPlaces[3];
    battle.unk4A7590 = kOpeningPlaces[4];
    battle.unk4A7594 = kOpeningPlaces[5];
    battle.unk53BF46 = 0;
    battle.unk53BF48 = 0;
    battle.unk53BD74 = 0;
    battle.darkCopyTint.fill(-1);
    battle.unk4A7630 = -1;
    battle.unk4A7564 = -1;
    battle.unk4A762C = kNoUnit;
    battle.unk4A7644 = kNoUnit;

    // 0x00521374..0x00521385 in the original: unk521374 whole and the first five words of unk52137C, which share
    // the bytes of its upper half.
    battle.unk521374.fill(0);
    std::fill_n(battle.unk52137C.begin(), kSharedEscortWords, u16{0});
    battle.lootIds = {};
    battle.unk520FBC = 0;
    battle.unk520FC0 = 0;
    battle.unk520FC4 = 0;
    battle.unk53BD78 = 0;
    battle.unk53BD7C = 0;
    battle.unk53BD80 = 0;
    battle.unk53BD84 = 0;
    battle.unk53BD88 = 0;
    battle.unk53BD9C = 0;
    battle.unk53BDA0 = 0;
    battle.unk53BF12 = 0;
    battle.unk53BF14 = 0;
    battle.unk53BF16 = 0;
    battle.unk53BF1C = 0;
    battle.unk53BDAC = 0;
    battle.pendingActionRow = 0;
    battle.pendingAction = 0;
    battle.flashLevel = BattleState::FlashStart;
    battle.defeatedRoles = 0;
    battle.unk53BF00 = 0;
    battle.unk53BF02 = 0;
    battle.unk53BF04 = 0;
    battle.unk53BF08 = 0;
    battle.unk4A7620 = kNoUnit;
    battle.unk4A7622 = kNoUnit;
    battle.unk4A7624 = kNoUnit;
    battle.unk4A7626 = kNoUnit;
    battle.unk4FDF7C = 0;
    battle.unk5242F8 = 0;
    battle.unk4A7628 = kOpeningPanels;
    battle.hitDamage = 0;
    battle.lastDamage = 0;
    battle.darkRed = 0;
    battle.darkGreen = 0;
    battle.darkBlue = 0;
    battle.proficiencyBonus = 0;
    battle.unk53BD90 = 0;
    battle.unk53BD94 = 0;
    battle.unk53BD98 = 0;
    battle.unk53BF22 = 0;
    battle.unk53BF24 = 0;
    battle.unk53BF26 = 0;
    battle.unk53BD20 = 0;
    battle.unk53BDEE = 0;
    battle.fieldFade = ColorFade{};
    battle.unk53BF28 = 0;
    battle.unk53BF2A = 0;
    battle.unk53BD68 = 0;
    battle.unk53C4C0 = 0;
    messages.lootCount = 0;
    messages.lootCounts = {};
    battle.unk53BF5C = 0;
    battle.unk53BF60 = 0;
    battle.unk53BF64 = 0;
    messages.lootNames = {};
    battle.unitY.fill(0);
    battle.drawOrder.fill(0);
    battle.drawTaken.fill(0);
    battle.unk524268.fill(0);
    battle.unk52411C.fill(0);
    battle.unk53BF68 = 0;
    battle.unk53BF6C = 0;
    battle.unk53BF70 = 0;
    battle.unk53BF74 = 0;
    battle.unk53BF78 = 0;
    battle.unk53BF7C = 0;
    battle.unk53BF80 = 0;
    battle.unk53BF88 = 0;
    battle.unk53BF8C = 0;
    battle.unk53BF90 = 0;
    battle.unk53BF94 = 0;
    battle.unk53BF98 = 0;
    battle.unk53BF9C = 0;
    battle.unk53BFA0 = 0;
    battle.unk53BFA4 = 0;
    battle.unk53BFA8 = 0;
    battle.unk53BFAC = 0;
    battle.unk53BFB0 = 0;
    battle.unk53BFB4 = 0;
    battle.unk53BFB8 = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFC0 = 0;
    battle.unk53BFC4 = 0;
    battle.flashing = 0;
    battle.unk53BFD0 = 0;
    battle.unk53BFD4 = 0;
    battle.unk53BFE0 = 0;
    battle.unk53BFD8 = 0;
    battle.unk53BFDC = 0;
    battle.unk53BFE4 = 0;
    battle.unk53BFF0 = 0;
    battle.unk53BFF4 = 0;
    battle.unk4A763C = 1;
    battle.unk53BFFC = 0;
    battle.mirrored = 0;
    battle.spellStage = 0;
    battle.unk53C004 = 0;
    battle.autoBattle = 0;
    battle.unk53C028 = 0;
    battle.darkenByTint = 0;
    this->fightTalk.unk53CEAC = 0;
    battle.unk53C038 = 0;
    battle.unk53C040 = 0;
    battle.unk53C048 = 0;
    this->archive.CloseAll();
    this->acts.CloseAll();
    this->audio.StopAllSounds();
    this->audio.StopMusic();
    this->database.CloseFiles();
    battle.unk53BFFC = 0;
    battle.lastLearned = LearnedSkill{};
}
}  // namespace swd3
