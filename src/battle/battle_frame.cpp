#include "game/game.h"

#include <algorithm>
#include <cmath>
#include <span>

#include "battle/battle_items.h"
#include "battle/smooth/smooth_draw.h"
#include "battle/smooth/smooth_messages.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "game/screenshot.h"
#include "platform/display.h"
#include "platform/surface.h"
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
constexpr u16 kIntroSound = 0xC0;  // BattleIntro
constexpr u16 kIntroBottomBar = 0x234D;
constexpr int kIntroBottomBarY = 0x180;
constexpr int kIntroPixelBits = 0x10;
constexpr i32 kIntroBufferBytes = 0x96000;
constexpr u16 kIntroSway = 1;
constexpr u16 kIntroZoom = 2;
constexpr int kIntroDropStart = -0x32;
constexpr int kIntroScaleStart = 9;
constexpr int kIntroZoomUnit = 0x400;
constexpr int kIntroZoomScale = 4;
constexpr int kIntroZoomFrames = 0x100;
constexpr int kIntroRise = -0x20;
constexpr int kIntroZoomGrowth = 0x20;
constexpr int kIntroTint = -12;
constexpr u32 kIntroDrawMode = Painter::ScaledGlow;
constexpr int kIntroStretchStep = 2;
constexpr int kIntroScaleStep = 0x12;
constexpr int kIntroLastX = -0x22;
constexpr int kIntroSwayFrames = 0x21;
constexpr int kIntroFadeScale = 9;
constexpr double kIntroPi = 3.140000104904175;           // swd32.exe 0x00499D08 (3.14f)
constexpr double kIntroSwayRate = 0.030303030303030304;  // swd32.exe 0x00499D00
constexpr double kIntroSwayWidth = 256.0;                // swd32.exe 0x00499CF8
constexpr int kIntroZoomStep = 0x10;
constexpr int kIntroZoomBase = 0x40;
constexpr int kIntroScaleDivisor = 0xA;
constexpr u32 kIntroUnusedDraw = 3;

struct BattleMusicRange
{
    u16 first;
    u16 last;
    std::string_view path;
};

constexpr std::array<BattleMusicRange, 3> kBattleMusic = {{
    {1, 0x70, "music\\Battle_Europa01.mp3"},     // swd32.exe 0x004A776C
    {0x72, 0xB9, "music\\Battle_Arab01.mp3"},    // swd32.exe 0x004A7754
    {0xC6, 0x10E, "music\\Battle_China01.mp3"},  // swd32.exe 0x004A7738
}};

constexpr i32 kNoFirstStrike = 0x40;  // FirstStrike
constexpr u32 kFirstStrikeSides = 2;
constexpr u32 kFirstStrikeRoll = 100;
constexpr u32 kPartyStrikeFirst = 0x1B;
constexpr u32 kPartyStrikeLast = 0x20;
constexpr u32 kEnemyStrikeFirst = 0x37;
constexpr u32 kEnemyStrikeLast = 0x3C;
constexpr int kEnemySlot = 2;
constexpr int kStrikeNoticeX = 0x118;
constexpr int kStrikeNoticeY = 0xA;
constexpr i16 kStrikeNoticeFrames = 0x3C;
constexpr int kStrikeNoticeFlags = BattleMessage::BlackBar | BattleMessage::Centered;
constexpr std::u8string_view kPartyStrikes = u8"我方搶先攻擊!!";  // swd32.exe 0x004A7718
constexpr std::u8string_view kEnemyStrikes = u8"敵方搶先攻擊!!";  // swd32.exe 0x004A7728
constexpr i32 kFacingChanged = 0x80;

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

// swd32.exe 0x00453200: one frame of the battle; it returns frameResult (1, or the end CheckBattleEnd set), 3 when game
// flag 0x11 leaves it, or the debug keys' 0 (the frame not drawn). The battle music starts again when it stopped; the
// pointer, the keys and the auto battle are handled and the units measured and sorted. Unless paused: an idle battle
// takes the next slot record every kSlotRecordWait frames as the acting unit (enemyGaugeBoost lets the gauges run while
// none acts and no panel is open), the background, the units (the acting ones sorted around it), the hits, the bottom
// bar, the current role's panel frame and cursor, the role panels, the menu, the end frame, the messages, the field's
// lines and heads, the talk, the resistance panel and the timers are drawn; the end is checked, a finished fade of
// the stone sides lights the field again, the frame is shown (shaken while quaking) and saved when asked.
int Game::BattleFrame()
{
    BattleState& battle = this->battleState;
    BattleHud& hud = this->battleHud;
    battle.frameResult = 1;

    if (this->audio.IsMusicStopped() && battle.unk53C4C0 == 0)
    {
        this->PlayMusic(battle.battleMusic);
        this->audio.SetMusicVolume(this->config.musicVolume);
    }

    // The rebuild (deviations.md): a press is a role's own; when another role (or the same one's next turn) is to be
    // given a command, what is held from before counts only once let go and pressed again.
    if (battle.currentUnit != battle.keysUnit)
    {
        if (battle.currentUnit != 0)
        {
            this->input.SpendHeld();
            this->controls.SpendHeld();
        }

        battle.keysUnit = battle.currentUnit;
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
        return battle.frameResult;
    }

    if (battle.unk53BFC0 == 1 && battle.actingUnit == -1)
    {
        battle.unk53BFC0 = 0;
    }

    if (battle.slotRecords[0].unit != -1 && battle.unk53BF74 == 0 && this->fightTalk.battleRuns == 1 &&
        battle.unk53BFC0 == 0)
    {
        if (battle.slotRecordFrames < kSlotRecordWait)
        {
            ++battle.slotRecordFrames;
        }
        else
        {
            BattleSlotRecord acting{
                battle.actingUnit,
                battle.sweepTotal,
                battle.unk53AE78,
                static_cast<u16>(battle.slideBackPicture),
                battle.unk53AE7C,
                battle.unk53AE80,
                battle.unk53AE84,
                battle.unk53AE88
            };

            this->TakeSlotRecord(acting);
            battle.actingUnit = acting.unit;
            battle.sweepTotal = acting.targetCount;
            battle.unk53AE78 = acting.kind;
            battle.slideBackPicture = static_cast<i16>(acting.unk0A);
            battle.unk53AE7C = acting.unk0C;
            battle.unk53AE80 = acting.unk10;
            battle.unk53AE84 = acting.unk14;
            battle.unk53AE88 = acting.unk18;

            if (battle.actingUnit != -1)
            {
                battle.slotRecordFrames = 0;
                battle.unk53BF74 = 1;
                this->fightTalk.battleActingUnit = battle.actingUnit;
            }
        }
    }

    battle.enemyGaugeBoost = battle.actingUnit == -1 && battle.currentUnit == 0 ? 1 : 0;
    smooth::Scene frameScene;
    this->battleServices.frame = this->smoothMotion || this->battleDeferred ? &frameScene : nullptr;
    this->DrawBattleBackground(battle.shake);

    if (battle.unk53BF64 == 1 || battle.hitsAllRoles == 1)
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
        smooth::Scene scene;
        smooth::WindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            x,
            kCurrentPanelTop,
            x + kCurrentPanelWidth,
            kCurrentPanelBottom,
            0,
            kCurrentPanelBorder,
            scene
        );
        scene.Show(this->battleServices);

        if (battle.grayBandShown == 0 && this->battleRoles[unit].Flag26B8() == 0)
        {
            hud.DrawCursorAnimation(kPanelCursor, battle.panelPlaces[2 * unit], battle.panelPlaces[2 * unit + 1]);
        }
    }

    this->DrawRolePanels();
    this->DrawBattleMenu();

    this->BattleEndFrame();
    {
        smooth::Scene scene;
        smooth::MessagesFrame(this->battleMessages).Frame(scene);
        scene.Show(this->battleServices);
    }

    this->fieldView.DrawLines();
    this->fieldView.DrawRoleHeads();
    this->talk.Draw();
    this->battleMessages.DrawResistancePanel();
    this->fieldView.DrawTimer(kTimerX, kTimerY, 0);
    this->fieldView.DrawTimer(kSecondTimerX, kTimerY, 1);

    if (this->state.GameFlag(kQuitFlag))
    {
        this->battleServices.frame = nullptr;
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
    this->battleServices.frame = nullptr;

    if (this->smoothMotion)
    {
        this->battleStage.Draw(std::move(frameScene), this->state.pointerX, this->state.pointerY);
    }
    else
    {
        frameScene.DrawAll(this->battleServices);
    }

    const bool smooth = this->smoothMotion;

    if (battle.quaking == 1 || (battle.panelFlags & kQuakePanel) != 0)
    {
        this->QuakeFrame();
    }
    else if (smooth)
    {
        this->CopyToPrimary();
        this->battleFrameShown = true;
    }
    else
    {
        this->ShowFrame();
    }

    if (battle.snapNext == 1)
    {
        const std::u8string name = Utf8Format(kSnapName, {static_cast<i32>(++battle.snapNumber + kSnapBase)});
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
                this->audioDevice.Serve();
            }
        );
        battle.snapNext = 0;
    }

    return battle.frameResult;
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
    battle.roleOrder.fill(0);
    battle.unk524200.fill(0);
    battle.unk53AF30 = 0;
    battle.unk53AF34 = 0;
    std::fill_n(battle.turnTaken.begin(), kClearedTurnUnits, 0);
    battle.unk502950 = 0;
    battle.unk5244E8.fill(-1);
    battle.enemyOrders.fill(0);
    battle.enemyPlaces.fill(UnitPlacement{});
    battle.rolePlaces.fill(UnitPlacement{});
    battle.slotRecords.fill(BattleSlotRecord{0, 0, 0, 0, 0, 0, 0, 0});
    battle.ClearActing();
    battle.hitActs.fill(ActState{});
    battle.fadeActs.fill(ActState{});
    battle.projector = Projector();
    battle.pointerX = 0;
    battle.pointerY = 0;
    battle.formation = BattleFormation{};
    battle.unk4FF168.fill(0);
    battle.roleListScrolls.fill(0);
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
    battle.roleListRows.fill(1);
    battle.controlRows = kAnswersStart;
    battle.listRole = kHealPage;
    this->talk.choice = 0;
    battle.panelFlags = 0;
    battle.unk53BCDC = 0;
    battle.enemyCount = 0;
    battle.roleCount = 0;
    battle.guestCount = 0;
    battle.turnCommand = 0;
    battle.result = 0;
    battle.menuCommand = 1;
    battle.aimTarget = 1;
    battle.actingEnemy = 0;
    battle.orderStep = 1;
    battle.pageListRow = 1;
    battle.roleListRow = 1;
    battle.controlCursor = 1;
    battle.darkenedAt = -1;
    battle.listScroll = 0;
    battle.roleListScroll = 0;
    battle.unk53BD0C = 0;
    battle.specialCursor = 0;
    battle.listCursor = 0;
    battle.menuX = 0;
    battle.menuY = 0;
    battle.enemyHitAct = 0;
    battle.unk4A7580 = kOpeningPlaces[0];
    battle.unk53BD4C = 0;
    battle.turnUnit = 0;
    battle.currentUnit = 0;
    battle.heldEnemy = 0;
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
    battle.darkenUnit = -1;
    battle.autoTarget = -1;
    battle.controlledEnemy = kNoUnit;
    battle.anyStateRole = kNoUnit;

    // 0x00521374..0x00521385 in the original: fadeActLevels whole and the first five words of escortCalled, which share
    // the bytes of its upper half.
    battle.fadeActLevels.fill(0);
    std::fill_n(battle.escortCalled.begin(), kSharedEscortWords, u16{0});
    battle.lootIds = {};
    battle.unk520FBC = 0;
    battle.unk520FC0 = 0;
    battle.unk520FC4 = 0;
    battle.unk53BD78 = 0;
    battle.sweepTargets = 0;
    battle.unk53BD80 = 0;
    battle.sweepDefeated = 0;
    battle.unk53BD88 = 0;
    battle.unk53BD9C = 0;
    battle.hoverScrollPart = 0;
    battle.unk53BF12 = 0;
    battle.unk53BF14 = 0;
    battle.unk53BF16 = 0;
    battle.unk53BF1C = 0;
    battle.unk53BDAC = 0;
    battle.pendingActionRow = 0;
    battle.pendingAction = 0;
    battle.flashLevel = BattleState::FlashStart;
    battle.defeatedRoles = 0;
    battle.enemiesGone = 0;
    battle.unk53BF02 = 0;
    battle.unk53BF04 = 0;
    battle.unk53BF08 = 0;
    battle.lastRoleHit = kNoUnit;
    battle.lastEnemyHit = kNoUnit;
    battle.lastRoleDefeated = kNoUnit;
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
    battle.listPanelFrames = 0;
    battle.menuGrow = 0;
    battle.listRows = 0;
    battle.listedCommands = 0;
    battle.callingEscort = 0;
    battle.escortCount = 0;
    battle.endFrames = 0;
    battle.unk53BDEE = 0;
    battle.fieldFade = ColorFade{};
    battle.unk53BF28 = 0;
    battle.unk53BF2A = 0;
    battle.quakeStepFrames = 0;
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
    battle.hitFading.fill(0);
    battle.hitsAllRoles = 0;
    battle.unk53BF6C = 0;
    battle.unk53BF70 = 0;
    battle.unk53BF74 = 0;
    battle.unk53BF78 = 0;
    battle.sweepHit = 0;
    battle.unk53BF80 = 0;
    battle.unk53BF88 = 0;
    battle.gatheringSweep = 0;
    battle.unk53BF90 = 0;
    battle.unk53BF94 = 0;
    battle.unk53BF98 = 0;
    battle.unk53BF9C = 0;
    battle.unk53BFA0 = 0;
    battle.unk53BFA4 = 0;
    battle.hitLanded = 0;
    battle.unk53BFAC = 0;
    battle.unk53BFB0 = 0;
    battle.unk53BFB4 = 0;
    battle.unk53BFB8 = 0;
    battle.unk53BFBC = 0;
    battle.unk53BFC0 = 0;
    battle.unk53BFC4 = 0;
    battle.flashing = 0;
    battle.unk53BFD0 = 0;
    battle.enemyAimsAtEnemies = 0;
    battle.enemyFaded = 0;
    battle.damageShown = 0;
    battle.unk53BFDC = 0;
    battle.endFade = 0;
    battle.unk53BFF0 = 0;
    battle.enemyHitsAll = 0;
    battle.unk4A763C = 1;
    battle.learnedPending = 0;
    battle.mirrored = 0;
    battle.spellStage = 0;
    battle.victoryWithoutPanel = 0;
    battle.autoBattle = 0;
    battle.unk53C028 = 0;
    battle.darkenByTint = 0;
    this->fightTalk.wonByTalk = 0;
    battle.levelUpPending = 0;
    battle.orderSorted = 0;
    battle.unk53C048 = 0;
    this->archive.CloseAll();
    this->acts.CloseAll();
    this->audio.StopAllSounds();
    this->audio.StopMusic();
    this->database.CloseFiles();
    battle.learnedPending = 0;
    battle.lastLearned = LearnedSkill{};
}

// The battle field of the first frame drawn into the second battle surface (inline in 0x004527E0): the background,
// the units in order, the bottom bar and the role panels on the frame, copied.
void Game::DrawIntroField()
{
    BattleState& battle = this->battleState;
    this->DrawBattleBackground(battle.shake);
    battle.SortDrawOrder();
    this->UpdateUnits();
    this->battleHud.DrawPictureFrame0(kIntroBottomBar, 0, kIntroBottomBarY);
    this->DrawRolePanels();
    CopySurface(*this->offscreen, *battle.surfaces[0]);
}

// A surface packed as a picture (inline in 0x004527E0): its rows (screenPitch apart) copied into a 640 x 480 buffer
// and encoded with the RLE codec.
std::vector<u8> Game::PackSurface(Surface& surface)
{
    this->drawSurface = this->painter.LockSurface(surface);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(surface);
    std::vector<u16> pixels(static_cast<std::size_t>(Display::Width) * Display::Height);
    const auto pitch = static_cast<std::size_t>(this->painter.ScreenPitch());

    for (std::size_t row = 0; row < static_cast<std::size_t>(Display::Height); ++row)
    {
        std::copy_n(
            this->drawSurface + row * pitch,
            Display::Width,
            pixels.begin() + static_cast<std::ptrdiff_t>(row * Display::Width)
        );
    }

    return this->codec.Encode(pixels.data(), Display::Width, Display::Height, kIntroPixelBits);
}

// The frame target set to the frame surface (inline in 0x004527E0).
void Game::TargetFrame()
{
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);
}

// The frame cleared (inline in 0x004527E0): 0x96000 bytes from its start.
void Game::ClearFrame()
{
    std::fill_n(this->drawSurface, static_cast<std::size_t>(Display::Width) * Display::Height, u16{0});
}

// The sine sway of transition 1 (inline in 0x004527E0): the picture drawn twice, swung by step `step` either way,
// zoomed by `scale` around the middle of the screen.
void Game::SwayFrame(int step, int scale)
{
    const auto sway =
        static_cast<i32>(crt::Ftol(std::sin(static_cast<double>(step) * kIntroPi * kIntroSwayRate) * kIntroSwayWidth));
    const int zoom = kIntroZoomStep * (kIntroZoomBase - scale / kIntroScaleDivisor);
    this->painter.SetZoom(Display::Width / 2, Display::Height / 2, zoom, zoom);
    this->ClearFrame();
    this->painter.DrawZoomed(sway, 0, Display::Width, Display::Height);
    this->painter.DrawZoomed(-sway, 0, Display::Width, Display::Height);
}

// swd32.exe 0x004527E0: the opening of battle with transition `style` (kIntroSound): the screen shown is kept as a
// picture and the first battle frame drawn into the battle surfaces (style 0: after the transition). Over 34 frames
// the old screen dims and stretches down (0), sways apart (1) or zooms away upwards (2); then the battle sways in (1),
// the frame is cleared (2) or slides in row by row (0). The battle music of the region starts (Europa for battles
// 1..0x70, Arab 0x72..0xB9, China 0xC6..0x10E, nothing else: the install directory itself) unless one plays. Unless
// panel flag 0x40 forbids it, a roll may give a first strike: the party's (one in 2, then 27..32 of 100: the roles
// standing that are not guests get full gauges and queue, the enemies turn away; 我方搶先攻擊!!) or the enemies'
// (55..60 of 100: every enemy standing queues, the roles turn away; 敵方搶先攻擊!!), marking the facing change (0x80).
void Game::BattleIntro(u16 style)
{
    BattleState& battle = this->battleState;
    Painter& painter = this->painter;
    DrawOptions& options = painter.Options();
    this->audio.PlaySound(kIntroSound, this->config.soundVolume);
    battle.unk53BFC0 = 1;
    Surface* primary = this->graphics.GetSurface(Graphics::SurfaceId::Primary);
    Surface& field = *battle.surfaces[0];
    Surface& shown = *battle.surfaces[1];
    CopySurface(*primary, shown);
    battle.unk53BF08 = kIntroBufferBytes;
    const std::vector<u8> oldScreen = this->PackSurface(shown);
    this->TargetFrame();
    this->DrawIntroField();
    std::vector<u8> newScreen;

    if (style != 0)
    {
        newScreen = this->PackSurface(field);
    }

    this->TargetFrame();
    painter.SetSource(oldScreen.data());
    painter.DrawPicture(0, 0, Display::Width, Display::Height, 0);
    int frame = 0;
    int x = 0;
    int stretch = 1;
    int y = kIntroDropStart;
    int scale = kIntroScaleStart;
    int zoomY = kIntroZoomUnit;

    do
    {
        if (style == kIntroZoom)
        {
            painter
                .SetZoom(Display::Width / 2, Display::Height / 2, kIntroZoomScale * (kIntroZoomFrames - frame), zoomY);
            this->ClearFrame();
            painter.DrawZoomed(0, kIntroRise * frame++, Display::Width, Display::Height);
            zoomY += kIntroZoomGrowth;
        }
        else if (style == kIntroSway)
        {
            this->SwayFrame(frame++, scale);
            zoomY += kIntroZoomGrowth;
        }
        else if (style == 0)
        {
            options.tintRed = kIntroTint;
            options.tintGreen = kIntroTint;
            options.tintBlue = kIntroTint;
            options.scaleHeight = stretch + Display::Width;
            painter.DrawPicture(x, y, Display::Width, Display::Height, kIntroDrawMode);
        }

        --x;
        stretch += kIntroStretchStep;
        y -= kIntroStretchStep;
        scale += kIntroScaleStep;
        this->ShowFrame();
    } while (x > kIntroLastX);

    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->TargetFrame();

    if (style == kIntroSway)
    {
        this->ClearFrame();
        painter.SetSource(newScreen.data());
        int fade = kIntroFadeScale * stretch;

        for (int step = 0; step < kIntroSwayFrames; ++step)
        {
            const auto sway = static_cast<i32>(
                crt::Ftol(std::sin(static_cast<double>(step) * kIntroPi * kIntroSwayRate) * kIntroSwayWidth)
            );
            const int zoom = kIntroZoomStep * (kIntroZoomBase - fade / kIntroScaleDivisor);
            painter.SetZoom(Display::Width / 2, Display::Height / 2, zoom, zoom);
            this->ClearFrame();
            painter.DrawZoomed(-sway, 0, Display::Width, Display::Height);
            painter.DrawZoomed(sway, 0, Display::Width, Display::Height);
            fade -= kIntroScaleStep;
            this->ShowFrame();
        }
    }
    else if (style == kIntroZoom)
    {
        this->ClearFrame();
    }
    else if (style == 0)
    {
        CopySurface(*primary, shown);
        this->DrawIntroField();
        CopySurface(shown, *primary);
        this->graphics.Present();
        this->random.Below(kIntroUnusedDraw);
        this->SlideBattleIn(field, shown);
    }

    battle.unk53BFC0 = 0;
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->StartBattleMusic();
    this->FirstStrike();
}

// The battle music of BattleIntro (inline in the original); see there.
void Game::StartBattleMusic()
{
    BattleState& battle = this->battleState;

    if (!this->audio.IsMusicStopped())
    {
        return;
    }

    battle.battleMusic = this->config.installDirectory;

    for (const auto& [first, last, path] : kBattleMusic)
    {
        if (battle.battle >= first && battle.battle <= last)
        {
            battle.battleMusic += Utf8Bytes(path);
        }
    }

    this->PlayMusic(battle.battleMusic);
    this->audio.SetMusicVolume(this->config.musicVolume);
}

// The first strike of BattleIntro (inline in the original); see there.
void Game::FirstStrike()
{
    BattleState& battle = this->battleState;
    BattleServices& services = this->battleServices;

    if ((battle.panelFlags & kNoFirstStrike) != 0)
    {
        return;
    }

    if (this->random.Below(kFirstStrikeSides) != 0)
    {
        const u32 roll = this->random.Below(kFirstStrikeRoll);

        if (roll < kPartyStrikeFirst || roll > kPartyStrikeLast)
        {
            return;
        }

        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];

            if (role.IsDefeated() || role.guestControl == 1)
            {
                continue;
            }

            role.ResetGauge(services);
            role.StepGauge(services, 1);
            auto free = std::find(battle.roleOrder.begin(), battle.roleOrder.end(), 0);

            if (free != battle.roleOrder.end())
            {
                *free = static_cast<i32>(index) + BattleState::FirstRoleUnit;
            }
        }

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            BattleEnemy& enemy = this->battleEnemies[index];
            enemy.SetFacingIfAble(1);
            enemy.ResetTurn(services);
        }

        this->battleMessages
            .Add(kStrikeNoticeX, kStrikeNoticeY, kStrikeNoticeFrames, kPartyStrikes, kStrikeNoticeFlags);
    }
    else
    {
        const u32 roll = this->random.Below(kFirstStrikeRoll);

        if (roll < kEnemyStrikeFirst || roll > kEnemyStrikeLast)
        {
            return;
        }

        for (u32 index = 0; index < battle.enemyCount; ++index)
        {
            if (!this->battleEnemies[index].IsDefeated())
            {
                battle.AddSlotRecord(kEnemySlot, static_cast<i32>(index));
            }
        }

        for (u32 index = 0; index < battle.roleCount; ++index)
        {
            BattleRole& role = this->battleRoles[index];
            role.SetFacingIfAble(1);
            role.ResetTurn(services);
        }

        this->battleMessages
            .Add(kStrikeNoticeX, kStrikeNoticeY, kStrikeNoticeFrames, kEnemyStrikes, kStrikeNoticeFlags);
    }

    battle.panelFlags |= kFacingChanged;
}
}  // namespace swd3
