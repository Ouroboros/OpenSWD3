#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <string>
#include <vector>

#include "core/format.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_text.h"
#include "menu/settings_constants.h"
#include "platform/display.h"
#include "text/text_input.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// The states of the system page.
constexpr i32 kEntries = 1;
constexpr i32 kChosen = 2;
constexpr i32 kSettings = 5;
constexpr i32 kDisplaySettings = 6;  // the rebuild's display setup, opened from the settings (deviations.md)
constexpr i32 kInGame = 100;

constexpr i32 kFadeOut = 0x64;
constexpr i32 kFirstName = 0x62;
constexpr i32 kNameEntry = 0x61;
constexpr i32 kInGameFade = -0x78;
constexpr i32 kTitleFade = -6;
constexpr i32 kHiddenGlow = -0x10;
constexpr i32 kEffectsOffFlag = GameState::PlainScreen;
constexpr i32 kNameEntryFlag = GameState::TextBox;
constexpr int kVolumeSound = 0x2E;
constexpr int kTitleMusic = 0xA;
constexpr int kNameBoxX = 300;
constexpr int kNameBoxY = 0xE6;
constexpr int kNameLength = 8;
constexpr u32 kScreenPixels = Display::Pixels;
constexpr std::size_t kScreenBytes = sizeof(Pixel) * Display::Pixels;
constexpr int kTextStyle = TextFont::BoldShadow;

// The title entries (Menu::systemCursor).
constexpr i32 kContinueEntry = 0;
constexpr i32 kNewGameEntry = 1;
constexpr i32 kSettingsEntry = 2;
constexpr i32 kQuitEntry = 3;
constexpr u32 kEntryLeft = 0x6E;  // the entries are clicked between these x
constexpr u32 kEntryRight = 0x104;
constexpr int kEntryX = 0x7D;
constexpr i32 kChosenFade = 4;  // the chosen entry dims by this a frame down to kChosenGlow
constexpr i32 kChosenGlow = -0x20;
constexpr i32 kGlowRise = 2;  // the others brighten to kEntryGlow
constexpr i32 kEntryGlow = -0xC;
constexpr i32 kFadeInStep = 5;     // the screen fades in from black by this a frame
constexpr i32 kFadeOutEnd = 0x68;  // a chosen entry fades the screen out until this row
constexpr i32 kFadeOutStep = 8;
constexpr int kWithMap = 1;  // Game::StartStory: the first map is entered

// The settings rows (Menu::systemRow); a row is kRowHeight high from kRowsTop. The rebuild adds the launcher switch
// above the original six and the display setup below them (deviations.md): the box grows a row up and down, the
// original rows stay where they were (from 0xF0).
constexpr i32 kLauncherRow = 0;
constexpr i32 kSoundRow = 1;
constexpr i32 kMusicRow = 2;
constexpr i32 kCacheRow = 3;
constexpr i32 kEffectsRow = 4;
constexpr i32 kTextSpeedRow = 5;
constexpr i32 kBattleSpeedRow = 6;
constexpr i32 kDisplayRow = 7;
constexpr u32 kRowsTop = 0xD0;
constexpr u32 kRowHeight = 0x20;
constexpr u32 kBarsLeft = 0xA8;  // a click between these x sets a bar
constexpr u32 kBarsRight = 0x1C0;
constexpr int kStepsLeft = 0x100;  // the steps of a bar, kStepWidth wide
constexpr int kStepWidth = 0x10;
constexpr int kStepShift = 4;
constexpr int kStepsRight = 0x1B0;
constexpr i32 kStepOff = 0x22;  // the actions of a step of a bar
constexpr i32 kStepOn = 0x23;
constexpr i32 kMinCache = 0x3C;  // the cache size in megabytes: kMinCache and kCacheStep more per step
constexpr i32 kCacheStep = 0x28;
constexpr u32 kCacheSteps = 2;
constexpr i32 kMaxCache = kMinCache + static_cast<i32>(kCacheSteps) * kCacheStep;
constexpr i32 kSlowestText = 4;                        // the text speed counts down to 0 from here
constexpr i32 kMusicFallback = Config::MaxVolume - 1;  // past the maximum the music volume falls back here
constexpr int kVolumeLabelX = 0xC4;
constexpr int kLabelX = 0x98;
constexpr int kLineX = 0x168;  // the cache, effects and speed lines
constexpr int kLineDrop = 6;
constexpr int kSampleX = 0x19E;
constexpr int kLabelRed = 0x15;
constexpr int kLabelGreen = 0xF;
constexpr int kLabelBlue = 8;
constexpr int kSampleRed = 0xF;
constexpr int kSampleGreen = 9;
constexpr int kSampleBlue = 2;
constexpr i32 kSampleDelayFactor = 2;
constexpr int kBoxX = 0x98;
constexpr int kBoxTop = 0xCC;  // four pixels above the first row
constexpr int kBoxWidth = 0x190;
constexpr int kSettingsRows = 8;  // the box is as high as its rows (the original's six: 0xC0)
constexpr int kHighlightX = 0x93;
constexpr int kHighlightWidth = 0x19A;
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr i32 kSpeeds = 5;

// The name entry.
constexpr u32 kConfirmLeft = 0x162;  // the confirm button
constexpr u32 kConfirmRight = 0x198;
constexpr u32 kConfirmTop = 0x101;
constexpr u32 kConfirmBottom = 0x112;
constexpr i32 kClickConfirm = 1;  // Menu::systemClick
constexpr i32 kClickCancel = 2;
constexpr u32 kLeftClicks = 3;  // Menu::clicks: a single or double left click
constexpr u32 kCoin = 2;
constexpr i32 kSecondName = kFirstName + 1;
constexpr int kFirstRoleAct = 1;  // the faces of the two names
constexpr int kSecondRoleAct = 2;
constexpr i32 kFaceIdle = 0x46;
constexpr i32 kFaceSmile = 0x3A;
constexpr std::array<i32, 3> kFirstReplies = {0x1C, 0x4D, 0x1E};
constexpr std::array<i32, 3> kSecondReplies = {0x22, 0x39, 0xE};
constexpr i32 kReplyFrames = 0x28;
constexpr i32 kReplyEnd = 2;  // the face goes back to idle when this many frames are left
constexpr int kFaceX = 0xDC;
constexpr int kFaceY = 0xFE;
constexpr int kQuitFadeMode = 0x10;
constexpr int kQuitFadeLength = 0x19;

// In game (modes 1 and 2) the page fades over the saved screen.
constexpr i32 kGrayEnd = -0x5A;  // mode 1: darkened until here, plain, then darkened again from kRedarken
constexpr i32 kRedarken = -0xA;
constexpr i32 kRedarkenStep = 3;
constexpr i32 kInGameFrameAction = 0x44;
constexpr i32 kDarkenEvery = 3;  // mode 2 darkens the saved screen every third frame
constexpr int kDarkenTint = -1;

// Whether `y` is inside settings row `row`.
bool InRow(u32 y, i32 row)
{
    const u32 top = kRowsTop + kRowHeight * static_cast<u32>(row);
    return y < top + kRowHeight - 1 && y > top;
}

// The top of settings row `row`.
int RowY(i32 row)
{
    return static_cast<int>(kRowsTop + kRowHeight * static_cast<u32>(row));
}

}  // namespace

// swd32.exe 0x00448700: the entries hidden, fading in from -6 (in game, mode 1 and 2, the page fades over a copy of the
// screen; mode 3 goes straight to the name entry); the title music in mode 0 when no music plays
void Menu::SystemEnter()
{
    Game& owner = this->game;
    this->glow = {kHiddenGlow, kHiddenGlow, kHiddenGlow, kHiddenGlow};
    this->systemCursor = 0;
    this->systemRow = kTitleFade;
    this->systemState = 0;
    this->savedScreen.clear();

    if (this->mode == Menu::GameModeA || this->mode == Menu::GameModeB)
    {
        this->systemState = kInGame;
        this->systemRow = kInGameFade;
        this->savedScreen.assign(owner.drawSurface, owner.drawSurface + kScreenPixels);
    }

    if (this->mode == Menu::NameEntryMode)
    {
        this->systemState = kEntries;
        this->systemCursor = kNewGameEntry;
        this->SystemSelect();
    }

    owner.unk4A9484 = 0;

    if (this->mode == Menu::TitleMode && owner.audio.IsMusicStopped())
    {
        owner.audio.StopMusic();
        owner.PlayMusic(owner.MusicPath(kTitleMusic));
        owner.audio.SetMusicVolume(owner.config.musicVolume);
    }

    this->sampleDelay = 0;
    this->sampleLength = 0;
    this->textSpeedShown = owner.config.textSpeed;
}

// swd32.exe 0x00448840: the mouse on the page: the confirm button and the right button of the name entry, the four
// entries (a click chooses), the bars of the settings (a right click or a click elsewhere leaves them)
void Menu::SystemFrame()
{
    Game& owner = this->game;
    const ButtonState& leftButton = owner.controls.At(Button::MouseLeft);
    const ButtonState& rightButton = owner.controls.At(Button::MouseRight);
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);

    if (this->systemState == kSettings || this->systemState == kDisplaySettings)
    {
        this->StoreChangedSettings();
    }

    if (this->systemState == kChosen)
    {
        if (this->systemRow >= kNameEntry)
        {
            if (leftButton.heldFrames == 1 && leftButton.press == 1 && y < kConfirmBottom && y > kConfirmTop &&
                x < kConfirmRight && x > kConfirmLeft)
            {
                this->systemClick = kClickConfirm;
            }
            else if (rightButton.heldFrames == 1 && rightButton.press == 1)
            {
                this->systemClick = kClickCancel;
            }
        }

        return;
    }

    if (this->systemState == kEntries)
    {
        constexpr std::array<std::pair<u32, u32>, 4> kRows = {
            {{0xD2, 0xE8}, {0x107, 0x11D}, {0x140, 0x156}, {0x179, 0x18F}}
        };

        for (std::size_t entry = 0; entry < kRows.size(); ++entry)
        {
            if (y < kRows[entry].second && y > kRows[entry].first && x < kEntryRight && x > kEntryLeft)
            {
                this->systemCursor = static_cast<i32>(entry);

                if ((this->clicks & kLeftClicks) != 0)
                {
                    this->SystemSelect();
                }
            }
        }

        return;
    }

    if (this->systemState == kDisplaySettings)
    {
        this->DisplaySettingsMouse();
        return;
    }

    if (this->systemState != kSettings)
    {
        return;
    }

    // The rebuild's rows (deviations.md): the launcher switch takes the choice clicked, a click on the display setup
    // opens it.
    if (leftButton.press != 0 && x > kBarsLeft && x < static_cast<u32>(kBoxX + kBoxWidth))
    {
        if (InRow(y, kLauncherRow))
        {
            this->systemRow = kLauncherRow;
            const std::vector<int> lefts = Menu::ChoiceLefts(Menu::OnOffChoices(), owner.largeFont, kStepsLeft);

            for (std::size_t at = 0; at + 1 < lefts.size(); ++at)
            {
                if (x >= static_cast<u32>(lefts[at]) && x < static_cast<u32>(lefts[at + 1]))
                {
                    this->OptionsSetLauncher(at == 0);
                }
            }

            return;
        }

        if (InRow(y, kDisplayRow))
        {
            this->systemRow = kDisplayRow;
            this->SystemSelect();
            return;
        }
    }

    if (leftButton.press == 0 || x >= kBarsRight || x <= kBarsLeft)
    {
        if (rightButton.press != 0)
        {
            this->SystemCancel();
        }

        return;
    }

    u32 step = (x - kStepsLeft) >> kStepShift;
    const bool onBar = HighWord(step) == 0;
    Config& settings = owner.config;

    if (InRow(y, kSoundRow))
    {
        this->systemRow = kSoundRow;

        if (onBar)
        {
            settings.soundVolume = static_cast<i32>(std::min<u32>(step, Config::MaxVolume));
            owner.audio.PlaySound(kVolumeSound, settings.soundVolume);
        }

        return;
    }

    if (InRow(y, kMusicRow))
    {
        this->systemRow = kMusicRow;

        if (onBar)
        {
            settings.musicVolume = static_cast<i32>(std::min<u32>(step, Config::MaxVolume));
            owner.audio.SetMusicVolume(settings.musicVolume);
        }

        return;
    }

    if (InRow(y, kCacheRow))
    {
        this->systemRow = kCacheRow;

        if (onBar)
        {
            settings.cacheSize =
                static_cast<i32>(kCacheStep * static_cast<i32>(std::min<u32>(step, kCacheSteps)) + kMinCache);
        }

        return;
    }

    if (InRow(y, kEffectsRow))
    {
        this->systemRow = kEffectsRow;

        if (onBar)
        {
            owner.state.SetGameFlag(kEffectsOffFlag);

            if (step != 0)
            {
                owner.state.ClearGameFlag(kEffectsOffFlag);
            }
        }

        return;
    }

    if (InRow(y, kTextSpeedRow))
    {
        this->systemRow = kTextSpeedRow;

        if (onBar)
        {
            step = std::min<u32>(step, kSlowestText);
            settings.textSpeed = kSlowestText - static_cast<i32>(step);
            this->textSpeedShown = kSlowestText - static_cast<i32>(step);
        }

        return;
    }

    if (InRow(y, kBattleSpeedRow))
    {
        this->systemRow = kBattleSpeedRow;

        if (onBar)
        {
            settings.battleSpeed = static_cast<i32>(std::min<u32>(step, Config::MaxVolume));
        }

        return;
    }

    if (rightButton.press != 0)
    {
        this->SystemCancel();
    }
}

// swd32.exe 0x00448BB0: on the settings a step past the last row falls back to row 4 (the rebuild goes round to the
// first)
void Menu::SystemDown()
{
    if (this->systemState == kEntries)
    {
        if (++this->systemCursor > kQuitEntry)
        {
            this->systemCursor = kQuitEntry;
        }
    }
    else if (this->systemState == kSettings)
    {
        // The original falls back to the row before its last, so a held key swings between the last two; the rebuild
        // goes round to the first (deviations.md).
        if (++this->systemRow > kDisplayRow)
        {
            this->systemRow = kLauncherRow;
        }
    }
    else if (this->systemState == kDisplaySettings && ++this->systemRow > kDisplayBackRow)
    {
        this->systemRow = 0;
    }

    this->SettingsMoveSound();
}

// swd32.exe 0x00448C00 (on the settings the rebuild goes round from the first row to the last)
void Menu::SystemUp()
{
    if (this->systemState == kEntries)
    {
        if (--this->systemCursor < 0)
        {
            this->systemCursor = 0;
        }
    }
    else if (this->systemState == kSettings)
    {
        if (--this->systemRow < 0)
        {
            this->systemRow = kDisplayRow;
        }
    }
    else if (this->systemState == kDisplaySettings && --this->systemRow < 0)
    {
        this->systemRow = kDisplayBackRow;
    }

    this->SettingsMoveSound();
}

// The rebuild (deviations.md): moving on the settings sounds as on the settings of the system tab (the original's
// title page is silent).
void Menu::SettingsMoveSound()
{
    if (this->systemState == kSettings || this->systemState == kDisplaySettings)
    {
        this->game.audio.PlaySound(kVolumeSound, this->game.config.soundVolume);
    }
}

// swd32.exe 0x00448C40
void Menu::SystemLast()
{
    if (this->systemState == kEntries)
    {
        this->systemCursor = kQuitEntry;
    }
    else if (this->systemState == kSettings)
    {
        this->systemRow = kDisplayRow;
    }
    else if (this->systemState == kDisplaySettings)
    {
        this->systemRow = kDisplayBackRow;
    }
}

// swd32.exe 0x00448C70
void Menu::SystemFirst()
{
    if (this->systemState == kEntries)
    {
        this->systemCursor = 0;
    }
    else if (this->systemState == kSettings || this->systemState == kDisplaySettings)
    {
        this->systemRow = 0;
    }
}

// swd32.exe 0x00448CA0: on the settings: sound volume down (with sound 0x2E), music volume down, cache size down to
// 60 MB, effects off, text slower (up to 4), battle speed down
void Menu::SystemLeft()
{
    Game& owner = this->game;
    Config& settings = owner.config;

    if (this->systemState == kEntries)
    {
        if (--this->systemCursor < 0)
        {
            this->systemCursor = 0;
        }

        return;
    }

    if (this->systemState == kDisplaySettings)
    {
        this->OptionsStepDisplay(this->systemRow, false, false);
        return;
    }

    if (this->systemState != kSettings)
    {
        return;
    }

    switch (this->systemRow)
    {
        case kLauncherRow:
            this->OptionsSetLauncher(true);
            break;

        case kSoundRow:
        {
            int volume = --settings.soundVolume;

            if (settings.soundVolume <= 0)
            {
                volume = 0;
                settings.soundVolume = 0;
            }

            owner.audio.PlaySound(kVolumeSound, volume);
            break;
        }

        case kMusicRow:
        {
            int volume = --settings.musicVolume;

            if (settings.musicVolume <= 0)
            {
                volume = 0;
                settings.musicVolume = 0;
            }

            owner.audio.SetMusicVolume(volume);
            break;
        }

        case kCacheRow:
            settings.cacheSize -= kCacheStep;

            if (settings.cacheSize <= kMinCache)
            {
                settings.cacheSize = kMinCache;
            }

            break;

        case kEffectsRow:
            owner.state.SetGameFlag(kEffectsOffFlag);
            break;

        case kTextSpeedRow:
        {
            int level = ++settings.textSpeed;

            if (settings.textSpeed > kSlowestText)
            {
                level = kSlowestText;
                settings.textSpeed = kSlowestText;
            }

            this->textSpeedShown = level;
            break;
        }

        case kBattleSpeedRow:
            if (--settings.battleSpeed < 0)
            {
                settings.battleSpeed = 0;
            }

            break;

        default:
            break;
    }
}

// swd32.exe 0x00448DA0: on the settings: sound volume up to 11, music volume (past 11 it falls back to 10), cache size
// up to 140 MB, effects on, text faster (down to 0), battle speed up to 11
void Menu::SystemRight()
{
    Game& owner = this->game;
    Config& settings = owner.config;

    if (this->systemState == kEntries)
    {
        if (++this->systemCursor > kQuitEntry)
        {
            this->systemCursor = kQuitEntry;
        }

        return;
    }

    if (this->systemState == kDisplaySettings)
    {
        this->OptionsStepDisplay(this->systemRow, true, false);
        return;
    }

    if (this->systemState != kSettings)
    {
        return;
    }

    switch (this->systemRow)
    {
        case kLauncherRow:
            this->OptionsSetLauncher(false);
            break;

        case kSoundRow:
        {
            int volume = ++settings.soundVolume;

            if (settings.soundVolume > Config::MaxVolume)
            {
                volume = Config::MaxVolume;
                settings.soundVolume = Config::MaxVolume;
            }

            owner.audio.PlaySound(kVolumeSound, volume);
            break;
        }

        case kMusicRow:
        {
            int volume = ++settings.musicVolume;

            if (settings.musicVolume > Config::MaxVolume)
            {
                volume = kMusicFallback;
                settings.musicVolume = kMusicFallback;
            }

            owner.audio.SetMusicVolume(volume);
            break;
        }

        case kCacheRow:
            settings.cacheSize += kCacheStep;

            if (settings.cacheSize >= kMaxCache)
            {
                settings.cacheSize = kMaxCache;
            }

            break;

        case kEffectsRow:
            owner.state.ClearGameFlag(kEffectsOffFlag);
            break;

        case kTextSpeedRow:
        {
            int level = --settings.textSpeed;

            if (settings.textSpeed <= 0)
            {
                level = 0;
                settings.textSpeed = 0;
            }

            this->textSpeedShown = level;
            break;
        }

        case kBattleSpeedRow:
            if (++settings.battleSpeed > Config::MaxVolume)
            {
                settings.battleSpeed = Config::MaxVolume;
            }

            break;

        default:
            break;
    }
}

// swd32.exe 0x00448EB0
void Menu::SystemEnd()
{
    if (this->systemState == kEntries)
    {
        if (++this->systemCursor > kQuitEntry)
        {
            this->systemCursor = kQuitEntry;
        }
    }
}

// swd32.exe 0x00448EE0: the chosen entry fades out; entry 1 first asks for the two names (flag 0x50 routes the
// keyboard to a name box), entry 2 opens the settings, entry 3 also fades the music out
// swd32.exe 0x00498310 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x0049831B (EH handler of 0x00448EE0)
void Menu::SystemSelect()
{
    Game& owner = this->game;

    // The rebuild's rows (deviations.md): Enter switches the launcher, opens the display setup; there it steps a row's
    // choice round, or goes back.
    if (this->systemState == kSettings && this->systemRow == kLauncherRow)
    {
        this->OptionsSetLauncher(!owner.config.showLauncher);
        return;
    }

    if (this->systemState == kSettings && this->systemRow == kDisplayRow)
    {
        this->systemState = kDisplaySettings;
        this->systemRow = 0;
        owner.audio.PlaySound(kVolumeSound, owner.config.soundVolume);
        return;
    }

    if (this->systemState == kDisplaySettings)
    {
        if (this->systemRow == kDisplayBackRow)
        {
            this->SystemCancel();
            return;
        }

        this->OptionsStepDisplay(this->systemRow, true, true);
        return;
    }

    if (this->systemState != kEntries)
    {
        return;
    }

    this->systemRow = kFadeOut;
    this->systemState = kChosen;

    if (this->systemCursor == kNewGameEntry)
    {
        this->systemRow = kNameEntry;
        owner.state.SetGameFlag(kNameEntryFlag);
        this->face.id = kFirstRoleAct;
        this->face.action = kFaceIdle;
        this->face.direction = 0;
        this->nameReply = 0;
        this->enteredNames.assign(2 * Party::NameBytes, 0);
        ++this->systemRow;
        owner.textInput = std::make_unique<TextInput>(Party::RoleNames[0], kNameLength, kNameBoxX, kNameBoxY);
    }
    else if (this->systemCursor == kSettingsEntry)
    {
        owner.acts.Run(owner.battleMessages.panelAct);
        this->systemState = kSettings;
        this->systemRow = kLauncherRow;
        this->gauge.id = Scene::InterfaceAct;
        this->gauge.action = kStepOff;
    }
    else if (this->systemCursor == kQuitEntry)
    {
        owner.audio.SetMusicFade(kQuitFadeMode, kQuitFadeLength);
        owner.audio.PollMusicFade();
    }
}

// swd32.exe 0x00449050: Escape on the entries points at the last entry; on the settings it goes back and writes them
// (to [game], Env.dat in the original)
void Menu::SystemCancel()
{
    Game& owner = this->game;

    if (this->systemState == kEntries)
    {
        this->systemCursor = kQuitEntry;
        return;
    }

    if (this->systemState == kDisplaySettings)
    {
        this->systemState = kSettings;
        this->systemRow = kDisplayRow;
        owner.audio.PlaySound(kVolumeSound, owner.config.soundVolume);
        return;
    }

    if (this->systemState == kSettings)
    {
        this->systemState = kEntries;
        owner.config.Update(this->CurrentSettings());
        this->storedSettings.reset();
    }
}

// swd32.exe 0x004490C0: the page. The entries glow in over the background (the chosen one brighter) while the screen
// fades in from black; a chosen entry fades the screen out and acts (0: continue the newest save, 1: a new game after
// the names, 2: the settings, 3: quit). The name entry shows the box with the face's reply to a key; the settings
// show the six rows with their bars; in game the page fades over the saved screen and back.
// swd32.exe 0x00498330 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x0049833B (EH handler of 0x004490C0)
void Menu::SystemDraw()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;

    if (this->systemState <= kChosen)
    {
        owner.actDrawer.Draw(this->background, 0, 0);
        const auto chosen = static_cast<std::size_t>(this->systemCursor);
        this->glow[chosen] -= kChosenFade;

        if (this->glow[chosen] < kChosenGlow)
        {
            this->glow[chosen] = kChosenGlow;
        }

        for (i32& level : this->glow)
        {
            level += kGlowRise;

            if (level > kEntryGlow)
            {
                level = kEntryGlow;
            }
        }

        constexpr std::array<int, 4> kEntryY = {0xD2, 0x107, 0x140, 0x179};

        for (std::size_t entry = 0; entry < this->items.size(); ++entry)
        {
            owner.actDrawer.DrawGlow(this->items[entry], kEntryX, kEntryY[entry], this->glow[entry]);
        }

        if (this->systemRow < 0)
        {
            if (++this->systemRow >= 0)
            {
                this->systemRow = 0;
                this->systemState = kEntries;
                this->systemClick = 0;
                return;
            }

            const int level = kFadeInStep * this->systemRow;
            painter.TintPixels(std::span<u16>(owner.drawSurface, kScreenPixels), level, level, level);
        }
    }

    if (this->systemState == kChosen)
    {
        if (this->systemRow < kFadeOut)
        {
            if (!this->DrawNameEntry())
            {
                return;
            }
        }
        else if (++this->systemRow <= kFadeOutEnd)
        {
            const int level = kFadeOutStep * (kFadeOut - this->systemRow);
            painter.TintPixels(std::span<u16>(owner.drawSurface, kScreenPixels), level, level, level);
        }
        else
        {
            this->ActOnEntry();
            std::memset(owner.drawSurface, 0, kScreenBytes);
        }
    }

    if (this->systemState == kSettings)
    {
        this->DrawSettings();
    }
    else if (this->systemState == kDisplaySettings)
    {
        this->DrawDisplaySettings();
    }

    if (this->systemState != kInGame)
    {
        this->systemClick = 0;
        return;
    }

    i32 row = this->systemRow;

    if (this->mode == Menu::GameModeA && row == kInGameFade)
    {
        painter.GrayPixels(this->savedScreen);
    }

    if (this->mode == Menu::GameModeA)
    {
        if (row >= kInGameFade)
        {
            if (row < kGrayEnd)
            {
                this->tint = row - kGrayEnd;
            }
            else if (row < kRedarken)
            {
                this->tint = 0;
            }
            else if (row <= 0)
            {
                this->tint = kRedarkenStep * (kRedarken - row);
            }
        }

        this->gauge.id = Scene::InterfaceAct;
        this->gauge.action = kInGameFrameAction;
        owner.actDrawer.Draw(this->gauge, 0, 0);
    }
    else if (this->mode == Menu::GameModeB)
    {
        std::copy(this->savedScreen.begin(), this->savedScreen.end(), owner.drawSurface);

        if (row % kDarkenEvery == 0)
        {
            painter.TintPixels(this->savedScreen, kDarkenTint, kDarkenTint, kDarkenTint);
        }
    }

    this->systemRow = row + 1;

    if (row + 1 < 0)
    {
        this->systemClick = 0;
        return;
    }

    owner.menuState = Game::TitleMenu;
    owner.storyState = Game::StoryRunning;
    owner.unk4CAEBC = 0;
    owner.unk4CAEC0 = 0;
    owner.menuMode = Menu::NoSlotsMode;
    owner.scene.playerHold = 0;
    this->savedScreen.clear();
    std::memset(owner.drawSurface, 0, kScreenBytes);
}

// The name entry of the system page (part of 0x004490C0): a key makes the face reply (at random) for 40 frames; Enter
// or the confirm button keeps a name and asks for the next, Escape or the right button gives up. False once the step
// is done for the frame.
bool Menu::DrawNameEntry()
{
    Game& owner = this->game;
    const bool keyDown = owner.input.FirstKeyDown() != 0;

    if (keyDown)
    {
        if (this->nameReply == 0)
        {
            if (this->systemRow == kFirstName)
            {
                this->face.action = kFirstReplies[static_cast<std::size_t>(owner.random.Below(kFirstReplies.size()))];
            }
            else
            {
                this->face.action = kSecondReplies[static_cast<std::size_t>(owner.random.Below(kSecondReplies.size()))];
            }
        }

        this->nameReply = kReplyFrames;
    }
    else
    {
        if (this->nameReply > 0)
        {
            --this->nameReply;
        }

        if (this->nameReply == kReplyEnd)
        {
            if (this->systemRow == kFirstName && owner.random.Below(kCoin) == 0)
            {
                this->face.action = kFaceSmile;
            }
            else
            {
                this->face.action = kFaceIdle;
            }
        }
    }

    owner.DrawTextInput(*owner.textInput);
    owner.actDrawer.Draw(this->face, kFaceX, kFaceY);
    int result = owner.textInput->GetResult();

    if (result == 0 && this->systemClick != 0)
    {
        result = this->systemClick;
    }

    this->systemClick = 0;

    if (result == TextInput::Enter)
    {
        owner.party.names = Party::DefaultNames();
        const std::string name = owner.textInput->CopyText(Party::NameBytes);
        const auto at = Party::NameBytes * static_cast<std::size_t>(this->systemRow - kFirstName);
        std::copy_n(
            name.begin(),
            std::min<std::size_t>(name.size(), Party::NameBytes),
            this->enteredNames.begin() + static_cast<std::ptrdiff_t>(at)
        );
        owner.textInput.reset();

        if (++this->systemRow == kSecondName)
        {
            owner.textInput = std::make_unique<TextInput>(Party::RoleNames[1], kNameLength, kNameBoxX, kNameBoxY);
            this->face.id = kSecondRoleAct;
            this->face.action = kFaceIdle;
        }

        if (this->systemRow == kFadeOut)
        {
            std::copy_n(this->enteredNames.begin(), Party::NameBytes, owner.party.names[0].begin());
            std::copy_n(this->enteredNames.begin() + Party::NameBytes, Party::NameBytes, owner.party.names[1].begin());
            this->enteredNames.clear();
            owner.state.ClearGameFlag(kNameEntryFlag);
        }

        return false;
    }

    if (result == TextInput::Escape)
    {
        this->enteredNames.clear();
        this->systemState = kEntries;
        owner.state.ClearGameFlag(kNameEntryFlag);
        return false;
    }

    return true;
}

// What a faded out entry does (part of 0x004490C0).
void Menu::ActOnEntry()
{
    Game& owner = this->game;

    switch (this->systemCursor)
    {
        case kContinueEntry:
            owner.menuState = Game::NoMenu;
            owner.storyState = Game::SaveSlotsState;
            owner.unk4CAEBC = 1;
            owner.unk4CAEC0 = 1;
            owner.menuMode = Menu::TitleLoadMode;
            owner.ResetSaveSlots();
            break;

        case kNewGameEntry:
            owner.unk4ACEF4 = 0;
            owner.unk4AB990 = crt::Time();
            this->systemState = kSettings;
            std::memset(owner.drawSurface, 0, kScreenBytes);
            owner.StartStory(kWithMap);
            owner.menuState = Game::NoMenu;
            owner.storyState = Game::StoryRunning;
            owner.unk4CAEBC = 1;
            owner.unk4CAEC0 = 1;
            owner.ResetSaveSlots();
            owner.party.ApplyNameCheats();
            owner.LoadFameFile();
            break;

        case kSettingsEntry:
            this->systemState = kEntries;
            break;

        case kQuitEntry:
            owner.systemFlags |= Game::QuitRequested;
            break;

        default:
            break;
    }
}

// The settings of the system page (part of 0x004490C0): the box, the six labels (with the rebuild's two around them),
// a bar of steps for each (the current value lit), the cache line, the effects line, the speed with its sample text
// typing out, and the row highlight.
void Menu::DrawSettings()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    Config& settings = owner.config;
    auto color = static_cast<Pixel>(painter.MakeColor(kLabelRed, kLabelGreen, kLabelBlue));
    owner.actDrawer.Draw(this->background, 0, 0);
    this->DrawBox(
        kBoxX,
        kBoxTop,
        kBoxWidth,
        kSettingsRows * static_cast<int>(kRowHeight),
        static_cast<int>(FillMode::Eighth),
        0,
        0,
        0
    );
    this->gauge.id = Scene::InterfaceAct;
    this->gauge.action = kStepOff;
    TextFont& large = owner.largeFont;
    TextFont& medium = owner.mediumFont;
    u16* pixels = painter.Target();
    // The rebuild's rows (deviations.md): the launcher switch with its choices (the one in use bright), the display
    // setup.
    large.DrawText(pixels, kLabelX, RowY(kLauncherRow), kLauncherSetup, color, kTextStyle);
    this->DrawSystemChoices(kLauncherRow, Menu::OnOffChoices(), owner.config.showLauncher ? 0 : 1, true);
    large.DrawText(pixels, kLabelX, RowY(kDisplayRow), kDisplaySetup, color, kTextStyle);
    large.DrawText(pixels, kVolumeLabelX, RowY(kSoundRow), menu_text::kSoundVolume, color, kTextStyle);
    large.DrawText(pixels, kVolumeLabelX, RowY(kMusicRow), menu_text::kMusicVolume, color, kTextStyle);
    large.DrawText(pixels, kLabelX, RowY(kCacheRow), menu_text::kCacheSize, color, kTextStyle);
    large.DrawText(pixels, kLabelX, RowY(kEffectsRow), menu_text::kMapEffects, color, kTextStyle);
    large.DrawText(pixels, kLabelX, RowY(kTextSpeedRow), menu_text::kTextSpeed, color, kTextStyle);
    large.DrawText(pixels, kLabelX, RowY(kBattleSpeedRow), menu_text::kBattleSpeed, color, kTextStyle);

    auto bar = [this, &owner](int y, auto lit)
    {
        int step = 0;

        for (int x = kStepsLeft; x <= kStepsRight; x += kStepWidth)
        {
            this->gauge.action = lit(step) ? kStepOn : kStepOff;
            owner.actDrawer.Draw(this->gauge, x, y);
            ++step;
        }
    };

    bar(RowY(kSoundRow),
        [&settings](int step)
        {
            return step <= settings.soundVolume;
        });

    bar(RowY(kMusicRow),
        [&settings](int step)
        {
            return step <= settings.musicVolume;
        });

    int size = kMinCache;

    for (int x = kStepsLeft; x < kStepsLeft + static_cast<int>(kCacheSteps + 1) * kStepWidth; x += kStepWidth)
    {
        this->gauge.action = kStepOff;

        if (size == settings.cacheSize)
        {
            this->gauge.action = kStepOn;
            const std::string line = WinFormat(
                menu_text::kCacheLine,
                {static_cast<i32>(
                     static_cast<u8>(settings.installDirectory.empty() ? 0 : settings.installDirectory[0])
                 ),
                 settings.cacheSize}
            );
            color = static_cast<Pixel>(painter.MakeColor(kLabelRed, kLabelGreen, kLabelBlue));
            medium.DrawText(pixels, kLineX, RowY(kCacheRow) + kLineDrop, line, color, kTextStyle);
        }

        owner.actDrawer.Draw(this->gauge, x, RowY(kCacheRow));
        size += kCacheStep;
    }

    const int effectsOff = owner.state.GameFlag(kEffectsOffFlag) ? 1 : 0;

    for (int step = 0, x = kStepsLeft; x < kStepsLeft + 2 * kStepWidth; x += kStepWidth, ++step)
    {
        this->gauge.action = step == effectsOff ? kStepOff : kStepOn;
        owner.actDrawer.Draw(this->gauge, x, RowY(kEffectsRow));
    }

    const std::string effects =
        WinFormat(menu_text::kEffectsLine, {effectsOff != 0 ? menu_text::kOff : menu_text::kOn});
    medium.DrawText(pixels, kLineX, RowY(kEffectsRow) + kLineDrop, effects, color, kTextStyle);

    for (int step = 0, x = kStepsLeft + kSlowestText * kStepWidth; step < kSpeeds; ++step, x -= kStepWidth)
    {
        this->gauge.action = step == settings.textSpeed ? kStepOn : kStepOff;
        owner.actDrawer.Draw(this->gauge, x, RowY(kTextSpeedRow));
    }

    const std::string speed =
        menu_text::SpeedSample(settings.textSpeed, static_cast<std::size_t>(std::max(this->sampleLength, 0)));
    const std::string sample = Big5(menu_text::kSample);
    medium.DrawText(
        pixels,
        kSampleX,
        RowY(kTextSpeedRow) + kLineDrop,
        sample,
        static_cast<Pixel>(painter.MakeColor(kSampleRed, kSampleGreen, kSampleBlue)),
        kTextStyle
    );

    if (--this->sampleDelay <= 0)
    {
        ++this->sampleLength;
        this->sampleDelay = kSampleDelayFactor * this->textSpeedShown + 1;
    }

    medium.DrawText(pixels, kLineX, RowY(kTextSpeedRow) + kLineDrop, speed, color, kTextStyle);

    if (menu_text::SampleEnds(static_cast<std::size_t>(std::max(this->sampleLength, 0))))
    {
        this->sampleLength = 0;
    }

    bar(RowY(kBattleSpeedRow),
        [&settings](int step)
        {
            return step <= settings.battleSpeed;
        });

    FillBox(
        painter,
        kHighlightX,
        static_cast<int>(kRowHeight) * this->systemRow + kBoxTop,
        kHighlightWidth,
        static_cast<int>(kRowHeight),
        kHighlightRed,
        kHighlightGreen,
        0,
        FillMode::Gradient
    );
}

// The rebuild's display setup on the title page (deviations.md): its rows in the settings box, each with its choices
// (the one in use bright; a row without effect all dim), and the row highlight.
void Menu::DrawDisplaySettings()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const auto color = static_cast<Pixel>(painter.MakeColor(kLabelRed, kLabelGreen, kLabelBlue));
    owner.actDrawer.Draw(this->background, 0, 0);
    const auto rows = static_cast<int>(kDisplayLabels.size());
    this->DrawBox(
        kBoxX,
        kBoxTop,
        kBoxWidth,
        rows * static_cast<int>(kRowHeight),
        static_cast<int>(FillMode::Eighth),
        0,
        0,
        0
    );

    for (i32 row = 0; row < rows; ++row)
    {
        owner.largeFont.DrawText(
            painter.Target(),
            kLabelX,
            RowY(row),
            kDisplayLabels[static_cast<std::size_t>(row)],
            color,
            kTextStyle
        );

        if (row != kDisplayBackRow)
        {
            i32 chosen = -1;
            const std::vector<std::string> choices = this->OptionsDisplayChoices(row, chosen);
            this->DrawSystemChoices(row, choices, chosen, DisplayRowCounts(row, owner.smoothMotion));
        }
    }

    FillBox(
        painter,
        kHighlightX,
        static_cast<int>(kRowHeight) * this->systemRow + kBoxTop,
        kHighlightWidth,
        static_cast<int>(kRowHeight),
        kHighlightRed,
        kHighlightGreen,
        0,
        FillMode::Gradient
    );
}

// The choices of a rebuild's row of the title page from the steps' left: `chosen` bright (unless the row has no
// effect, `counts` false), the others dim.
void Menu::DrawSystemChoices(i32 row, const std::vector<std::string>& choices, i32 chosen, bool counts)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const auto color = static_cast<Pixel>(painter.MakeColor(kLabelRed, kLabelGreen, kLabelBlue));
    Pixel unchosen = color;
    painter.TintPixels(std::span<u16>(&unchosen, 1), kUnchosenTint, kUnchosenTint, kUnchosenTint);
    const std::vector<int> lefts = Menu::ChoiceLefts(choices, owner.largeFont, kStepsLeft);

    for (std::size_t at = 0; at < choices.size(); ++at)
    {
        const bool shown = counts && static_cast<i32>(at) == chosen;
        owner.largeFont
            .DrawText(painter.Target(), lefts[at], RowY(row), choices[at], shown ? color : unchosen, kTextStyle);
    }
}

// The mouse on the title page's display setup (deviations.md): a click on a row's choice takes it, on its last row
// goes back; a right click goes back.
void Menu::DisplaySettingsMouse()
{
    Game& owner = this->game;
    const ButtonState& leftButton = owner.controls.At(Button::MouseLeft);
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);

    if (owner.controls.At(Button::MouseRight).press != 0)
    {
        this->SystemCancel();
        return;
    }

    if (leftButton.press == 0 || x <= static_cast<u32>(kBoxX) || x >= static_cast<u32>(kBoxX + kBoxWidth))
    {
        return;
    }

    for (i32 row = 0; row <= kDisplayBackRow; ++row)
    {
        if (!InRow(y, row))
        {
            continue;
        }

        this->systemRow = row;

        if (row == kDisplayBackRow)
        {
            this->SystemCancel();
            return;
        }

        i32 chosen = -1;
        const std::vector<std::string> choices = this->OptionsDisplayChoices(row, chosen);
        const std::vector<int> lefts = Menu::ChoiceLefts(choices, owner.largeFont, kStepsLeft);

        for (std::size_t at = 0; at + 1 < lefts.size(); ++at)
        {
            if (x >= static_cast<u32>(lefts[at]) && x < static_cast<u32>(lefts[at + 1]) &&
                static_cast<i32>(at) != chosen)
            {
                this->OptionsChooseDisplay(row, static_cast<i32>(at));
            }
        }

        return;
    }
}
}  // namespace swd3
