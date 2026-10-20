#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <vector>

#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_text.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// The stages of the page (OptionsPage::stage).
constexpr i32 kOnTabs = 0;
constexpr i32 kOpened = 1;
constexpr i32 kConfirming = 2;
constexpr i32 kKeys = 5;
constexpr i32 kKeyChosen = 6;
constexpr i32 kKeyWaiting = 7;
constexpr i32 kCannotSave = 0xA;
constexpr i32 kDisplay = 8;  // the rebuild's display setup (deviations.md)

// The tabs (OptionsPage::entry).
constexpr i32 kSaveEntry = 0;
constexpr i32 kLoadEntry = 1;
constexpr i32 kNotesEntry = 2;
constexpr i32 kSettingsEntry = 3;
constexpr i32 kQuitEntry = 4;

// The rows of the settings and of the quit box (OptionsPage::row).
constexpr i32 kSoundRow = 0;
constexpr i32 kMusicRow = 1;
constexpr i32 kCacheRow = 2;
constexpr i32 kEffectsRow = 3;
constexpr i32 kTextSpeedRow = 4;
constexpr i32 kBattleSpeedRow = 5;
constexpr i32 kKeysRow = 6;
constexpr i32 kDisplayRow = 7;  // the rebuild's display setup, below the key setup
constexpr i32 kEndRow = 0;
constexpr i32 kRestartRow = 1;
constexpr i32 kQuitRows = 2;

// The key setup (OptionsPage::keyRow): sixteen key rows, then the three buttons.
constexpr i32 kApplyButton = 0x10;
constexpr i32 kDiscardButton = 0x11;
constexpr i32 kDefaultsButton = 0x12;
constexpr i32 kKeyRows = 0x13;
constexpr i32 kLastKeyRow = 0xF;  // Page Down on the key setup: the last key row
constexpr i32 kNoKey = 0xFF;      // a key row emptied by a swap
constexpr int kBackspace = 0xE;   // DirectInput DIK_BACK: cancels the key wait
// Keys that cannot be set (DirectInput codes: Caps Lock, Num Lock, Scroll Lock, F9..F12, F13..F15, SysRq, the Windows
// keys, the menu key and P).
constexpr std::array<i32, 15> kReservedKeys =
    {0x3A, 0x45, 0x46, 0x43, 0x44, 0x57, 0x58, 0x64, 0x65, 0x66, 0xB7, 0xDB, 0xDC, 0xDD, 0x19};

// The notes.
constexpr i32 kFirstNoteFlag = 0xE75;
constexpr i32 kNoteFlagsEnd = 0xFA0;
constexpr i32 kNoteBase = 0xE74;  // a note's number is its flag less this
constexpr i32 kNotesPerPage = 5;
constexpr u8 kNoteMark = '%';  // a note starting with it gets a mark before it
constexpr i32 kNoteMarkAction = 0x17;
constexpr int kNoteMarkLeft = 0x1C;
constexpr int kNoteLines = 2;
constexpr int kNoteWidth = 0x154;
constexpr u32 kUpArrowFrames = 3;  // OptionsPage::arrows: bits 0..3 the up arrow, 4..7 the down arrow
constexpr u32 kDownArrowFrames = 0x30;
constexpr u32 kUpArrowMask = 0xF;
constexpr u32 kDownArrowMask = 0xF0;
constexpr u32 kDownArrowStep = 0x10;
constexpr int kUpArrowPressed = 1;
constexpr int kDownArrowPressed = 2;

// The settings.
constexpr i32 kMinCache = 0x3C;
constexpr i32 kCacheStep = 0x28;
constexpr i32 kCacheSteps = 2;
constexpr i32 kMaxCache = kMinCache + kCacheSteps * kCacheStep;
constexpr i32 kSlowestText = 4;
constexpr i32 kSampleDelayFactor = 2;
constexpr i32 kEffectsOffFlag = GameState::PlainScreen;
constexpr i32 kSaveBlockedFlag = GameState::TimerEvent;
constexpr i32 kNoSaveMap = 0x16;

// The rows of the display setup (OptionsPage::displayRow), and the frame rates it offers (0: automatic, no limit
// besides the refresh with vertical sync).
constexpr i32 kSmoothRow = 0;
constexpr i32 kVSyncRow = 1;
constexpr i32 kFrameRateRow = 2;
constexpr i32 kRenderScaleRow = 3;
constexpr i32 kWideMapRow = 4;
constexpr i32 kDisplayBackRow = 5;
constexpr std::array<i64, 4> kFrameRates = {0, 60, 144, 240};
constexpr u32 kGameSavedFlag = 2;  // Menu::flags: saved since the menu opened, quitting needs no confirmation

// The quit fade.
constexpr i32 kQuitFadeFrom = 0x1E;
constexpr i32 kQuitFadeEnd = 0x3C;
constexpr i32 kEndGame = 1;  // OptionsPage::quitKind
constexpr i32 kRestart = 2;
constexpr int kQuitMusicFade = 0x1E;

// The sounds.
constexpr int kMoveSound = 0x2E;
constexpr int kChosenSound = 0x8B;
constexpr int kRefusedSound = 0x8C;
constexpr int kSwapSound = 0xB8;
constexpr int kKeyRowSound = 0x107;

// The clicks (Menu::clicks).
constexpr u32 kLeftClicks = 3;
constexpr u32 kSingleClick = 1;
constexpr u32 kRightClicks = 0xC;
constexpr u32 kAnyClicks = 0xF;

// The mouse areas.
constexpr u32 kTabsLeft = 0xD4;
constexpr u32 kTabsRight = 0x200;
constexpr u32 kTabsTop = 0x3A;
constexpr u32 kTabsBottom = 0x50;
constexpr u32 kTabWidth = 0x3C;
constexpr u32 kBarLeft = 0x264;
constexpr u32 kBarRight = 0x274;
constexpr u32 kUpArrowTop = 0x74;
constexpr u32 kUpArrowBottom = 0x82;
constexpr u32 kDownArrowTop = 0x1C0;
constexpr u32 kDownArrowBottom = 0x1CE;
constexpr u32 kKeyRowsTop = 0x68;
constexpr u32 kKeyRowsBottom = 0x1A7;
constexpr u32 kKeyRowsLeft = 0x142;
constexpr u32 kKeyRowsRight = 0x200;
constexpr u32 kKeyRowHeight = 0x14;
constexpr u32 kButtonsTop = 0x1BC;
constexpr u32 kButtonsBottom = 0x1D4;
constexpr std::array<std::pair<u32, u32>, 3> kButtonAreas = {{{0x189, 0x1B5}, {0x1B6, 0x1E2}, {0x1E3, 0x245}}};
constexpr u32 kChoicesLeft = 0x1E3;
constexpr u32 kChoicesRight = 0x267;
constexpr u32 kChoicesTop = 0xE0;
constexpr u32 kChoicesBottom = 0xFE;
constexpr u32 kChoiceWidth = 0x42;
constexpr u32 kQuitLeft = 0x1DC;
constexpr u32 kQuitRight = 0x248;
constexpr u32 kQuitTop = 0x68;
constexpr u32 kQuitBottom = 0x9C;
constexpr u32 kQuitRowHeight = 0x1A;
constexpr u32 kSettingsRight = 0x1FC;
constexpr u32 kStepsFrom = 0xD6;  // left of this only the key setup row is clicked
constexpr int kStepsLeft = 0x13D;
constexpr int kStepWidth = 0x10;
constexpr u32 kKeysRowFrom = 0x5E;
// The rows of the settings between these y (the last one the display setup's).
constexpr std::array<std::pair<u32, u32>, 8> kSettingsRows = {
    {{0x82, 0xA1},
     {0xA2, 0xC1},
     {0xC2, 0xE1},
     {0xE2, 0x101},
     {0x102, 0x121},
     {0x122, 0x141},
     {0x142, 0x161},
     {0x162, 0x181}}
};

// The drawing.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr int kOtherTabTint = -4;
constexpr int kSampleTint = -6;
constexpr int kPanelStyle = 4;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
// The tab labels of swd32.exe 0x004A73B8, 0x004A73B0, 0x004A73A8, 0x004A73A0 and 0x004A7398.
constexpr std::array<std::u8string_view, 5> kTabLabels = {u8"存檔", u8"讀取", u8"記載", u8"設定", u8"離開"};
constexpr std::u8string_view kCannotSaveText = u8"目前無法存檔";        // swd32.exe 0x004A7388
constexpr std::u8string_view kKeySetup = u8"按鍵設定";                  // swd32.exe 0x004A737C
constexpr std::string_view kVersion = "v1.02DVD";                       // swd32.exe 0x004A7374
constexpr std::u8string_view kEndGameText = u8"結束遊戲";               // swd32.exe 0x004A7364
constexpr std::u8string_view kRestartText = u8"重新開始";               // swd32.exe 0x004A7358
constexpr std::u8string_view kProgressLost = u8"目前進度將會消失";      // swd32.exe 0x004A7344
constexpr std::u8string_view kEndQuestion = u8" 確定要結束嗎?";         // swd32.exe 0x004A7334
constexpr std::u8string_view kRestartQuestion = u8" 要重新開始嗎?";     // swd32.exe 0x004A7324
constexpr std::u8string_view kConfirm = u8"確定";                       // swd32.exe 0x004A731C
constexpr std::u8string_view kGiveUp = u8"放棄";                        // swd32.exe 0x004A7314
constexpr std::u8string_view kKeyButtons = u8" 設定 放棄 恢復預設值 ";  // swd32.exe 0x004A72FC
constexpr std::u8string_view kPressKey = u8"請按一個鍵";                // swd32.exe 0x004A72F0
// The texts of the display setup.
constexpr std::u8string_view kDisplaySetup = u8"顯示設定";
constexpr std::array<std::u8string_view, 6> kDisplayLabels = {
    u8"平滑畫面",
    u8"垂直同步",
    u8"幀率上限",
    u8"繪製倍率",
    u8"地圖填滿",
    u8"返回",
};

constexpr std::u8string_view kAutomatic = u8"自動";
constexpr int kChoiceGap = 12;     // between the choices of a display row
constexpr int kUnchosenTint = -6;  // the choices not in use, and all of a row without effect
constexpr int kTabsBoxX = 0xD0;
constexpr int kTabsBoxY = 0x36;
constexpr int kTabsBoxWidth = 0x13C;
constexpr int kTabsBoxHeight = 0x1E;
constexpr int kTabsBoxRed = 0x10;
constexpr int kTabsBoxGreen = 0x10;
constexpr int kTabsBoxBlue = 0x6C;
constexpr int kTabsFrameLeft = 0xD8;
constexpr int kTabsFrameTop = 0x3E;
constexpr int kTabsFrameRight = 0x204;
constexpr int kTabsFrameBottom = 0x50;
constexpr int kTabX = 0xE0;
constexpr int kTabY = 0x3E;
constexpr int kTabStep = 0x3C;
constexpr int kRaised = -1;  // the current tab is drawn a pixel up and left
constexpr int kMessageX = 0xDC;
constexpr int kMessageY = 0x68;
constexpr int kMessageWidth = 0x84;
constexpr int kMessageHeight = 0x16;
constexpr int kListBoxX = 0xD4;
constexpr int kListBoxY = 0x74;
constexpr int kListBoxWidth = 0x190;
constexpr int kNotesBoxHeight = 0x15E;
constexpr int kSettingsBoxHeight = 0xF8;
constexpr int kDisplayRowHeight = 0x20;  // the settings box grown by the display setup's row
constexpr int kDisplayBoxHeight = 0xD8;  // the display setup: its six rows
constexpr int kDisplayFrameBottom = 0x148;
constexpr int kListFrameLeft = 0xD8;
constexpr int kListFrameTop = 0x78;
constexpr int kListFrameRight = 0x25C;
constexpr int kNotesFrameBottom = 0x1CA;
constexpr int kSettingsFrameBottom = 0x168;
constexpr int kNotesBarX = 0x264;
constexpr int kNotesBarY = 0x84;
constexpr int kNotesBarHeight = 0x139;
constexpr int kNoteX = 0xF0;
constexpr int kNotesTop = 0x84;
constexpr int kNoteHeight = 0x41;
constexpr int kVolumeLabelX = 0x108;
constexpr int kLabelX = 0xDC;
constexpr std::array<int, 8> kRowY = {0x82, 0xA2, 0xC2, 0xE2, 0x102, 0x122, 0x142, 0x162};
constexpr int kVersionX = 0x20C;
constexpr int kVersionY = 0x159;
constexpr i32 kStepOff = 0x22;
constexpr i32 kStepOn = 0x23;
constexpr int kBarStepsLeft = 0x13C;
constexpr int kBarStepsRight = 0x1EC;
constexpr int kTextStepsRight = 0x17C;
constexpr int kLineX = 0x1A4;
constexpr int kLineDrop = 6;
constexpr int kSampleX = 0x1DA;
constexpr int kHighlightX = 0xD3;
constexpr int kHighlightTop = 0x7E;
constexpr int kHighlightWidth = 0x18E;
constexpr int kHighlightHeight = 0x20;
// The display setup rows take clicks up to the end of the highlight.
constexpr u32 kDisplayRight = kHighlightX + kHighlightWidth;
constexpr int kQuitBoxX = 0x1DC;
constexpr int kQuitBoxY = 0x60;
constexpr int kQuitBoxWidth = 0x6C;
constexpr int kQuitBoxHeight = 0x40;
constexpr int kQuitFrameLeft = 0x1E0;
constexpr int kQuitFrameTop = 0x64;
constexpr int kQuitFrameRight = 0x244;
constexpr int kQuitFrameBottom = 0x9A;
constexpr int kQuitTextX = 0x1E4;
constexpr int kEndTextY = 0x68;
constexpr int kRestartTextY = 0x82;
constexpr int kQuitHighlightTop = 0x66;
constexpr int kConfirmBoxX = 0x114;
constexpr int kConfirmBoxY = 0xC4;
constexpr int kConfirmBoxWidth = 0x156;
constexpr int kConfirmBoxHeight = 0x38;
constexpr int kConfirmFrameLeft = 0x118;
constexpr int kConfirmFrameTop = 0xC8;
constexpr int kConfirmFrameRight = 0x266;
constexpr int kConfirmFrameBottom = 0xF8;
constexpr int kQuestionX = 0x11C;
constexpr int kQuestionY = 0xCA;
constexpr int kConfirmX = 0x1EE;
constexpr int kGiveUpX = 0x230;
constexpr int kChoicesY = 0xE4;
constexpr int kChoiceHighlightY = 0xE2;
constexpr int kChoiceHighlightHeight = 0x18;
constexpr int kKeysBoxX = 0xD8;
constexpr int kKeysBoxY = 0x78;
constexpr int kKeysBoxWidth = 0x20;
constexpr int kKeysBoxHeight = 0xF0;
constexpr int kKeyListX = 0xE0;
constexpr int kKeyListY = 0x68;
constexpr int kKeyListWidth = 0x184;
constexpr int kKeyListHeight = 0x140;
constexpr int kButtonsLeft = 0x184;
constexpr int kButtonsY = 0x1BE;
constexpr int kButtonsWidth = 0xC6;
constexpr int kButtonsHeight = 0x12;
constexpr int kKeyLabelX = 0xE8;
constexpr int kKeyNameX = 0x142;
constexpr int kKeyRowsY = 0x68;
constexpr int kPressKeyX = 0x1E4;
// The highlight of each button (x and width).
constexpr std::array<std::pair<int, int>, 3> kButtonHighlights = {{{0x189, 0x2C}, {0x1B6, 0x2C}, {0x1E3, 0x62}}};
constexpr int kKeyHighlightX = 0x139;
constexpr int kKeyHighlightWidth = 0xA2;
constexpr int kWaitHighlightWidth = 0x6C;
constexpr int kKeyHighlightUp = 1;

// Whether `value` is strictly between the edges of `range`.
bool Inside(u32 value, std::pair<u32, u32> range)
{
    return value < range.second && value > range.first;
}

// The name of DirectInput key `code` (swd32.exe 0x0049E470); the table ends at 0xDD.
std::u8string_view KeyName(i32 code)
{
    if (code < 0 || static_cast<std::size_t>(code) >= menu_text::kKeyNames.size())
    {
        return {};
    }

    return menu_text::kKeyNames[static_cast<std::size_t>(code)];
}

// The left of each of `choices` drawn with `font` from kBarStepsLeft, kChoiceGap apart, and the right of the last.
std::vector<int> ChoiceLefts(const std::vector<std::string>& choices, const TextFont& font)
{
    std::vector<int> lefts;
    int x = kBarStepsLeft;

    for (const std::string& choice : choices)
    {
        lefts.push_back(x);
        x += static_cast<int>(choice.size()) * font.Advance() / 2 + kChoiceGap;
    }

    lefts.push_back(x - kChoiceGap);
    return lefts;
}

}  // namespace

// The choices of display setup row `row` (Big5), and the one in use in `chosen` (-1 for none).
std::vector<std::string> Menu::OptionsDisplayChoices(i32 row, i32& chosen) const
{
    const Game& owner = this->game;

    const auto text = [](std::u8string_view words)
    {
        return WinFormat(words, {});
    };

    std::vector<std::string> choices;
    chosen = -1;

    switch (row)
    {
        case kSmoothRow:
        case kVSyncRow:
        case kWideMapRow:
        {
            const bool on = row == kSmoothRow ? owner.smoothMotion : row == kVSyncRow ? owner.vsync : owner.wideMap;
            choices = {text(menu_text::kOn), text(menu_text::kOff)};
            chosen = on ? 0 : 1;
            break;
        }

        case kFrameRateRow:
            for (std::size_t at = 0; at < kFrameRates.size(); ++at)
            {
                const auto rate = static_cast<i32>(kFrameRates[at]);
                choices.push_back(rate == 0 ? text(kAutomatic) : WinFormat("%d", {rate}));

                if (kFrameRates[at] == owner.frameRate)
                {
                    chosen = static_cast<i32>(at);
                }
            }

            break;

        case kRenderScaleRow:
            choices.push_back(text(kAutomatic));

            for (int scale = 1; scale <= static_cast<int>(DisplaySettings::MaximumRenderScale); ++scale)
            {
                choices.push_back(WinFormat("%d", {static_cast<i32>(scale)}));
            }

            chosen = owner.renderScaleSetting == Game::AutoRenderScale ? 0 : owner.renderScaleSetting;
            break;

        default:
            break;
    }

    return choices;
}

// Choice `choice` of display setup row `row` taken, put to use and written.
void Menu::OptionsChooseDisplay(i32 row, i32 choice)
{
    Game& owner = this->game;

    switch (row)
    {
        case kSmoothRow:
            owner.smoothMotion = choice == 0;
            break;

        case kVSyncRow:
            owner.vsync = choice == 0;
            break;

        case kFrameRateRow:
            owner.frameRate = kFrameRates[static_cast<std::size_t>(choice)];
            break;

        case kRenderScaleRow:
            owner.renderScaleSetting = choice == 0 ? Game::AutoRenderScale : choice;
            break;

        case kWideMapRow:
            owner.wideMap = choice == 0;
            break;

        default:
            return;
    }

    owner.ApplyDisplay();
    owner.StoreDisplay();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// The choice of display setup row `row` moved right (`up`) or left, past the last back to the first when `wrap`.
void Menu::OptionsStepDisplay(i32 row, bool up, bool wrap)
{
    i32 chosen = -1;
    const auto count = static_cast<i32>(this->OptionsDisplayChoices(row, chosen).size());

    if (count == 0)
    {
        return;
    }

    i32 next = chosen < 0 ? 0 : chosen + (up ? 1 : -1);

    if (next >= count)
    {
        next = wrap ? 0 : count - 1;
    }

    next = std::max(next, 0);

    if (next != chosen)
    {
        this->OptionsChooseDisplay(row, next);
    }
}

// swd32.exe 0x0044AF30: the system tab opened: the notes are the flags 0xE75..0xF9F set (their numbers from 1), the
// scroll bars cleared
void Menu::OptionsEnter()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;
    this->unk4FBECC = Menu::NoTab;
    page.entry = 0;
    page.stage = kOnTabs;
    page.locked = 0;
    page.notes.clear();

    for (i32 flag = kFirstNoteFlag; flag < kNoteFlagsEnd; ++flag)
    {
        if (owner.state.GameFlag(flag))
        {
            page.notes.push_back(static_cast<u16>(flag - kNoteBase));
        }
    }

    page.noteCount = static_cast<i32>(page.notes.size());
    this->scrollBar = MenuScrollBar{};
    this->townBar = MenuScrollBar{};
    page.sampleDelay = 0;
    page.sampleLength = 0;
    page.arrows = 0;
    page.textSpeedShown = owner.config.textSpeed;
}

// swd32.exe 0x0044D010: the notes shown from the top one: up to five, to the end of the list
void Menu::CountNotes()
{
    OptionsPage& page = this->options;
    page.notesShown = 0;
    auto at = static_cast<std::size_t>(page.noteTop);

    while (at < page.notes.size() && page.notes[at] != 0)
    {
        if (page.notesShown >= kNotesPerPage)
        {
            break;
        }

        ++page.notesShown;
        ++at;
    }
}

// swd32.exe 0x0044D050: the place of note `index` in the list (kept, never read)
void Menu::FindNote(int index)
{
    this->options.noteAt = static_cast<std::size_t>(std::max(index, 0));
}

// swd32.exe 0x0044D070: note `index` (table 0x11 of maps.dat) wrapped in two lines at (x, y); one starting with '%'
// gets a mark before it
void Menu::DrawNote(int index, int x, int y)
{
    Game& owner = this->game;
    const std::vector<u8>& maps = owner.databases.mapsData;
    const std::size_t number = this->options.notes[static_cast<std::size_t>(index)];
    std::size_t at = MapsDword(maps, MapsTable(maps, MapsDataTable::Notes) + sizeof(u32) * number);
    const std::string_view all(reinterpret_cast<const char*>(maps.data()), maps.size());

    if (at < maps.size() && maps[at] == kNoteMark)
    {
        this->gauge.id = Scene::InterfaceAct;
        this->gauge.action = kNoteMarkAction;
        owner.actDrawer.Draw(this->gauge, x - kNoteMarkLeft, y);
        ++at;
    }

    owner.DrawWrappedText(at < all.size() ? all.substr(at) : std::string_view(), x, y, kNoteLines, kNoteWidth);
}

// swd32.exe 0x0044B070: the mouse on the system tab: the notes' scroll bar (while the left button repeats), a right
// click goes back (on the key wait it presses Backspace first), the key rows and buttons, the tabs, the quit choices,
// the quit rows and the steps of the settings (while the left button is down)
void Menu::OptionsFrame()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;
    Config& settings = owner.config;

    if (page.locked != 0)
    {
        return;
    }

    const bool repeating = owner.controls.Repeating(Button::MouseLeft);
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);

    if (repeating && (page.stage == kOpened || page.stage == kConfirming) && page.entry == kNotesEntry &&
        page.noteCount > kNotesPerPage && x < kBarRight && x > kBarLeft)
    {
        const MenuScrollBar& bar = this->scrollBar;
        const auto at = static_cast<i32>(y);

        if (y < kUpArrowBottom && y > kUpArrowTop)
        {
            this->OptionsUp();
            return;
        }

        if (y < kDownArrowBottom && y > kDownArrowTop)
        {
            this->OptionsDown();
            return;
        }

        if (at < bar.thumbTop && at > bar.top)
        {
            this->OptionsPageUp();
            return;
        }

        if (at < bar.bottom && at > bar.thumbBottom)
        {
            this->OptionsPageDown();
            return;
        }
    }

    if ((this->clicks & kRightClicks) != 0)
    {
        if (page.stage == kKeyWaiting)
        {
            owner.input.PressKey(kBackspace);
        }

        this->OptionsCancel();
        return;
    }

    if (page.stage == kCannotSave)
    {
        if ((this->clicks & kAnyClicks) != 0)
        {
            this->OptionsSelect();
        }

        return;
    }

    if (page.stage == kKeys)
    {
        if (y < kKeyRowsBottom && y > kKeyRowsTop && x < kKeyRowsRight && x > kKeyRowsLeft)
        {
            if ((this->clicks & kLeftClicks) != 0)
            {
                page.keyRow = static_cast<i32>((y - kKeyRowsTop) / kKeyRowHeight);
                this->OptionsSelect();
            }

            return;
        }

        for (std::size_t button = 0; button < kButtonAreas.size(); ++button)
        {
            if (Inside(x, kButtonAreas[button]) && y < kButtonsBottom && y > kButtonsTop)
            {
                page.keyRow = kApplyButton + static_cast<i32>(button);

                if ((this->clicks & kLeftClicks) != 0)
                {
                    this->OptionsSelect();
                }

                return;
            }
        }
    }

    if ((this->clicks & kLeftClicks) != 0 && x < kTabsRight && x > kTabsLeft && y < kTabsBottom && y > kTabsTop)
    {
        page.stage = kOnTabs;
        page.entry = static_cast<i32>((x - kTabsLeft) / kTabWidth);
        this->OptionsSelect();
        owner.audio.PlaySound(kMoveSound, settings.soundVolume);
        return;
    }

    if (page.stage == kDisplay)
    {
        for (i32 row = kSmoothRow; row <= kDisplayBackRow; ++row)
        {
            if (x > kStepsFrom && x < kDisplayRight && Inside(y, kSettingsRows[static_cast<std::size_t>(row)]))
            {
                page.displayRow = row;

                if ((this->clicks & kSingleClick) == 0)
                {
                    return;
                }

                if (row == kDisplayBackRow)
                {
                    this->OptionsSelect();
                    return;
                }

                // A click on a choice takes it.
                i32 chosen = -1;
                const std::vector<std::string> choices = this->OptionsDisplayChoices(row, chosen);
                const std::vector<int> lefts = ChoiceLefts(choices, owner.largeFont);

                for (std::size_t at = 0; at < choices.size(); ++at)
                {
                    const auto left = static_cast<u32>(lefts[at]);
                    const auto right = static_cast<u32>(lefts[at + 1]);

                    if (x >= left && x < right && static_cast<i32>(at) != chosen)
                    {
                        this->OptionsChooseDisplay(row, static_cast<i32>(at));
                    }
                }

                return;
            }
        }

        return;
    }

    if (page.stage != kOpened)
    {
        if (page.stage != kConfirming || page.entry < kSettingsEntry)
        {
            return;
        }

        if (x < kChoicesRight && x > kChoicesLeft && y < kChoicesBottom && y > kChoicesTop)
        {
            page.choice = static_cast<i32>((x - kChoicesLeft) / kChoiceWidth);

            if ((this->clicks & kSingleClick) != 0)
            {
                this->OptionsSelect();
            }
        }

        return;
    }

    if (page.entry == kQuitEntry)
    {
        if (x < kQuitRight && x > kQuitLeft && y < kQuitBottom && y > kQuitTop)
        {
            page.entry = kQuitEntry;
            page.row = static_cast<i32>((y - kQuitTop) / kQuitRowHeight);

            if ((this->clicks & kSingleClick) != 0)
            {
                this->OptionsSelect();
            }
        }

        return;
    }

    if (page.entry != kSettingsEntry)
    {
        return;
    }

    if (owner.controls.At(Button::MouseLeft).press == 0 || x >= kSettingsRight)
    {
        return;
    }

    if (x > kStepsFrom)
    {
        int step = (static_cast<int>(x) - kStepsLeft) / kStepWidth;
        const std::array<i32, 6> rows = {kSoundRow, kMusicRow, kCacheRow, kEffectsRow, kTextSpeedRow, kBattleSpeedRow};

        for (const i32 row : rows)
        {
            if (!Inside(y, kSettingsRows[static_cast<std::size_t>(row)]))
            {
                continue;
            }

            page.row = row;

            if (step < 0)
            {
                return;
            }

            switch (row)
            {
                case kSoundRow:
                    step = std::min(step, Config::MaxVolume);
                    settings.soundVolume = step;
                    owner.audio.PlaySound(kMoveSound, step);
                    break;

                case kMusicRow:
                    step = std::min(step, Config::MaxVolume);
                    settings.musicVolume = step;
                    owner.audio.SetMusicVolume(step);
                    break;

                case kCacheRow:
                    settings.cacheSize = kCacheStep * std::min(step, kCacheSteps) + kMinCache;
                    break;

                case kEffectsRow:
                    owner.state.SetGameFlag(kEffectsOffFlag);

                    if (step != 0)
                    {
                        owner.state.ClearGameFlag(kEffectsOffFlag);
                    }

                    break;

                case kTextSpeedRow:
                    step = std::min(step, kSlowestText);
                    settings.textSpeed = kSlowestText - step;
                    page.textSpeedShown = kSlowestText - step;
                    break;

                default:
                    settings.battleSpeed = std::min(step, Config::MaxVolume);
                    break;
            }

            return;
        }
    }

    if (x > kKeysRowFrom && Inside(y, kSettingsRows[kKeysRow]))
    {
        page.row = kKeysRow;
        this->OptionsSelect();
    }
    else if (x > kKeysRowFrom && Inside(y, kSettingsRows[kDisplayRow]))
    {
        page.row = kDisplayRow;
        this->OptionsSelect();
    }
}

// swd32.exe 0x0044B560: on the tabs the next one; the next page of notes (the down arrow pressed for three frames);
// the next settings row (the key setup the last); the restart row; the "give up" choice; the next key row (past the
// buttons back to the first)
void Menu::OptionsDown()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            if (++page.entry > kQuitEntry)
            {
                page.entry = kQuitEntry;
                return;
            }

            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        case kOpened:
            if (page.entry == kNotesEntry)
            {
                page.noteTop += kNotesPerPage;

                if (page.noteTop < page.noteCount)
                {
                    this->FindNote(page.noteTop);
                    this->CountNotes();

                    if (page.noteRow > page.notesShown - 1)
                    {
                        page.noteRow = page.notesShown - 1;
                    }
                }
                else
                {
                    page.noteTop -= kNotesPerPage;
                    page.noteRow = page.notesShown - 1;
                }

                page.arrows |= kDownArrowFrames;
                return;
            }

            if (page.entry == kSettingsEntry)
            {
                if (++page.row > kDisplayRow)
                {
                    page.row = kDisplayRow;
                }

                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
                return;
            }

            if (page.entry == kQuitEntry)
            {
                if (++page.row >= kQuitRows)
                {
                    page.row = kRestartRow;
                }

                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            }

            return;

        case kConfirming:
            if (page.entry == kQuitEntry && ++page.choice >= kQuitRows)
            {
                page.choice = 1;
            }

            return;

        case kKeys:
            if (++page.keyRow >= kKeyRows)
            {
                page.keyRow = 0;
            }

            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        case kDisplay:
            if (++page.displayRow > kDisplayBackRow)
            {
                page.displayRow = 0;
            }

            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        default:
            return;
    }
}

// swd32.exe 0x0044B6E0: on the tabs the one before; the page of notes before (the up arrow pressed for three
// frames); the row before; the "confirm" choice; the key row before (from the first to the last button)
void Menu::OptionsUp()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            if (--page.entry < 0)
            {
                page.entry = 0;
                return;
            }

            break;

        case kOpened:
            if (page.entry == kNotesEntry)
            {
                page.noteTop -= kNotesPerPage;

                if (page.noteTop >= 0)
                {
                    this->FindNote(page.noteTop);
                }
                else
                {
                    page.noteRow = 0;
                    page.noteTop = 0;
                    this->FindNote(0);
                }

                this->CountNotes();
                page.arrows |= kUpArrowFrames;
                return;
            }

            if (page.entry != kSettingsEntry && page.entry != kQuitEntry)
            {
                return;
            }

            if (--page.row <= 0)
            {
                page.row = 0;
            }

            break;

        case kConfirming:
            if (page.entry == kQuitEntry && --page.choice < 0)
            {
                page.choice = 0;
            }

            return;

        case kKeys:
            if (--page.keyRow < 0)
            {
                page.keyRow = kDefaultsButton;
            }

            break;

        case kDisplay:
            if (--page.displayRow < 0)
            {
                page.displayRow = kDisplayBackRow;
            }

            break;

        default:
            return;
    }

    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x0044B840: Page Down: the last tab; the next page of notes; the key setup row; on the quit box row 2
// (past the two rows); the "confirm" choice; the last key row
void Menu::OptionsPageDown()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            if (page.entry != kQuitEntry)
            {
                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            }

            page.entry = kQuitEntry;
            return;

        case kOpened:
            if (page.entry == kNotesEntry)
            {
                this->OptionsDown();
            }
            else if (page.entry == kSettingsEntry)
            {
                page.row = kDisplayRow;
                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            }
            else if (page.entry == kQuitEntry)
            {
                // The original sets row 2 here, below the two rows of the box.
                page.row = kQuitRows;
                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            }

            return;

        case kConfirming:
            if (page.entry == kQuitEntry && --page.choice < 0)
            {
                page.choice = 0;
            }

            return;

        case kKeys:
            page.keyRow = kLastKeyRow;
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        case kDisplay:
            page.displayRow = kDisplayBackRow;
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        default:
            return;
    }
}

// swd32.exe 0x0044B930: Page Up: the first tab; the page of notes before; the first row; the "give up" choice; the
// first key row
void Menu::OptionsPageUp()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            if (page.entry != 0)
            {
                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            }

            page.entry = 0;
            return;

        case kOpened:
            if (page.entry == kNotesEntry)
            {
                this->OptionsUp();
            }
            else if (page.entry == kSettingsEntry || page.entry == kQuitEntry)
            {
                page.row = 0;
                owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            }

            return;

        case kConfirming:
            if (page.entry == kQuitEntry && ++page.choice >= kQuitRows)
            {
                page.choice = 1;
            }

            return;

        case kKeys:
            page.keyRow = 0;
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        case kDisplay:
            page.displayRow = 0;
            owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
            return;

        default:
            return;
    }
}

// swd32.exe 0x0044BA20: on the tabs the one before; the page of notes before; on the settings the row's value down
// (sound and music volume, cache size to 60 MB, effects off, text slower); the quit row before (no sound); the
// "confirm" choice; the key row before
void Menu::OptionsLeft()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;
    Config& settings = owner.config;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            if (--page.entry < 0)
            {
                page.entry = 0;
                return;
            }

            break;

        case kOpened:
            if (page.entry == kNotesEntry)
            {
                this->OptionsUp();
                return;
            }

            if (page.entry == kQuitEntry)
            {
                if (--page.row <= 0)
                {
                    page.row = 0;
                }

                return;
            }

            if (page.entry != kSettingsEntry)
            {
                return;
            }

            switch (page.row)
            {
                case kSoundRow:
                {
                    int volume = --settings.soundVolume;

                    if (settings.soundVolume <= 0)
                    {
                        volume = 0;
                        settings.soundVolume = 0;
                    }

                    owner.audio.PlaySound(kMoveSound, volume);
                    return;
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
                    return;
                }

                case kCacheRow:
                    settings.cacheSize -= kCacheStep;

                    if (settings.cacheSize <= kMinCache)
                    {
                        settings.cacheSize = kMinCache;
                    }

                    return;

                case kEffectsRow:
                    owner.state.SetGameFlag(kEffectsOffFlag);
                    return;

                case kTextSpeedRow:
                {
                    int level = ++settings.textSpeed;

                    if (settings.textSpeed > kSlowestText)
                    {
                        level = kSlowestText;
                        settings.textSpeed = kSlowestText;
                    }

                    page.textSpeedShown = level;
                    return;
                }

                case kBattleSpeedRow:
                    if (--settings.battleSpeed < 0)
                    {
                        settings.battleSpeed = 0;
                    }

                    return;

                default:
                    return;
            }

        case kConfirming:
            if (page.entry == kQuitEntry && --page.choice < 0)
            {
                page.choice = 0;
            }

            return;

        case kKeys:
            if (--page.keyRow < 0)
            {
                page.keyRow = kDefaultsButton;
            }

            break;

        case kDisplay:
            this->OptionsStepDisplay(page.displayRow, false, false);
            return;

        default:
            return;
    }

    owner.audio.PlaySound(kMoveSound, settings.soundVolume);
}

// swd32.exe 0x0044BBD0: on the tabs the next one; the next page of notes; on the settings the row's value up (sound
// and music volume to 11, cache size to 140 MB, effects on, text faster); the restart row (no sound); the "give up"
// choice; the next key row
void Menu::OptionsRight()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;
    Config& settings = owner.config;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            if (++page.entry > kQuitEntry)
            {
                page.entry = kQuitEntry;
                return;
            }

            break;

        case kOpened:
            if (page.entry == kNotesEntry)
            {
                this->OptionsDown();
                return;
            }

            if (page.entry == kQuitEntry)
            {
                if (++page.row >= kQuitRows)
                {
                    page.row = kRestartRow;
                }

                return;
            }

            if (page.entry != kSettingsEntry)
            {
                return;
            }

            switch (page.row)
            {
                case kSoundRow:
                {
                    int volume = ++settings.soundVolume;

                    if (settings.soundVolume > Config::MaxVolume)
                    {
                        volume = Config::MaxVolume;
                        settings.soundVolume = Config::MaxVolume;
                    }

                    owner.audio.PlaySound(kMoveSound, volume);
                    return;
                }

                case kMusicRow:
                {
                    int volume = ++settings.musicVolume;

                    if (settings.musicVolume > Config::MaxVolume)
                    {
                        volume = Config::MaxVolume;
                        settings.musicVolume = Config::MaxVolume;
                    }

                    owner.audio.SetMusicVolume(volume);
                    return;
                }

                case kCacheRow:
                    settings.cacheSize += kCacheStep;

                    if (settings.cacheSize >= kMaxCache)
                    {
                        settings.cacheSize = kMaxCache;
                    }

                    return;

                case kEffectsRow:
                    owner.state.ClearGameFlag(kEffectsOffFlag);
                    return;

                case kTextSpeedRow:
                    if (--settings.textSpeed < 0)
                    {
                        settings.textSpeed = 0;
                    }

                    page.textSpeedShown = settings.textSpeed;
                    return;

                case kBattleSpeedRow:
                    if (++settings.battleSpeed > Config::MaxVolume)
                    {
                        settings.battleSpeed = Config::MaxVolume;
                    }

                    return;

                default:
                    return;
            }

        case kConfirming:
            if (page.entry == kQuitEntry && ++page.choice >= kQuitRows)
            {
                page.choice = 1;
            }

            return;

        case kKeys:
            if (++page.keyRow >= kKeyRows)
            {
                page.keyRow = 0;
            }

            break;

        case kDisplay:
            this->OptionsStepDisplay(page.displayRow, true, false);
            return;

        default:
            return;
    }

    owner.audio.PlaySound(kMoveSound, settings.soundVolume);
}

// swd32.exe 0x0044BDA0: Enter: a tab opens (saving is refused while the timer event runs, on a map with an exit warp
// and on map 0x16; saving and loading open the save slots); the key setup opens on a copy of the keys; a quit row
// asks for confirmation (none after a save); confirming fades the music and starts the quit fade; on the key setup
// the buttons apply (and write) the keys, give up, or restore the defaults, a key row waits for a key
void Menu::OptionsSelect()
{
    Game& owner = this->game;
    OptionsPage& page = this->options;
    Config& settings = owner.config;

    if (page.locked != 0)
    {
        return;
    }

    auto startQuit = [this, &page](i32 kind)
    {
        ++page.stage;
        page.quitKind = kind;
        page.choice = 0;

        if ((this->flags & kGameSavedFlag) != 0)
        {
            page.locked = 1;
            page.choice = kQuitFadeFrom;
        }
    };

    switch (page.stage)
    {
        case kOnTabs:
            page.row = 0;
            ++page.stage;

            if (page.entry == kSaveEntry)
            {
                if (owner.state.GameFlag(kSaveBlockedFlag) || this->FindExitWarp(owner.currentMap) != 0 ||
                    owner.currentMap == kNoSaveMap)
                {
                    page.stage = kCannotSave;
                    owner.audio.PlaySound(kChosenSound, settings.soundVolume);
                    return;
                }

                owner.unk4CAEC0 = 0;
                owner.menuMode = Menu::KeySaveMode;
                owner.unk4CAEBC = 1;
                owner.storyState = Game::SaveSlotsState;
                owner.ResetSaveSlots();
                --page.stage;
                return;
            }

            if (page.entry == kLoadEntry)
            {
                owner.menuMode = Menu::KeyLoadMode;
                owner.unk4CAEC0 = 1;
                owner.unk4CAEBC = 1;
                owner.storyState = Game::SaveSlotsState;
                owner.ResetSaveSlots();
                --page.stage;
                return;
            }

            if (page.entry == kNotesEntry)
            {
                page.notesShown = 0;
                page.noteRow = 0;
                page.noteTop = 0;
                this->CountNotes();
            }

            return;

        case kOpened:
            if (page.entry == kSettingsEntry)
            {
                if (page.row == kKeysRow)
                {
                    page.keyRow = 0;
                    page.stage = kKeys;
                    page.keys = settings.keys;
                    owner.audio.PlaySound(kMoveSound, settings.soundVolume);
                }
                else if (page.row == kDisplayRow)
                {
                    page.displayRow = 0;
                    page.stage = kDisplay;
                    owner.audio.PlaySound(kMoveSound, settings.soundVolume);
                }

                return;
            }

            if (page.entry == kQuitEntry)
            {
                if (page.row == kEndRow)
                {
                    startQuit(kEndGame);
                }
                else if (page.row == kRestartRow)
                {
                    startQuit(kRestart);
                }
            }

            return;

        case kConfirming:
            if (page.entry != kQuitEntry)
            {
                return;
            }

            if (page.choice != 0)
            {
                page.stage = kOpened;
                return;
            }

            page.locked = 1;
            page.choice = kQuitFadeFrom;
            owner.audio.SetMusicFade(AudioManager::FadeOverLength, kQuitMusicFade);
            owner.audio.UpdateMusicFade();
            owner.scene.musicTunes = {};
            return;

        case kKeys:
            if (page.keyRow == kApplyButton)
            {
                settings.keys = page.keys;
                settings.SaveKeys();
            }
            else if (page.keyRow == kDefaultsButton)
            {
                settings.keys = {};
                settings.SetDefaultKeys();
                page.keys = settings.keys;
                owner.audio.PlaySound(kMoveSound, settings.soundVolume);
                return;
            }
            else if (page.keyRow != kDiscardButton)
            {
                owner.audio.PlaySound(kKeyRowSound, settings.soundVolume);
                page.stage = kKeyChosen;
                return;
            }

            page.stage = kOpened;
            owner.audio.PlaySound(kMoveSound, settings.soundVolume);
            return;

        case kKeyChosen:
            page.stage = kKeyWaiting;
            owner.audio.PlaySound(kMoveSound, settings.soundVolume);
            return;

        case kDisplay:
            if (page.displayRow == kDisplayBackRow)
            {
                page.stage = kOpened;
                owner.audio.PlaySound(kMoveSound, settings.soundVolume);
                return;
            }

            this->OptionsStepDisplay(page.displayRow, true, true);
            return;

        case kCannotSave:
            page.stage = kOnTabs;
            return;

        default:
            return;
    }
}

// swd32.exe 0x0044C0E0: Escape: from the tabs the menu goes back a page; an opened tab or the confirmation closes;
// on the key setup the "give up" button; the refusal to save closes
void Menu::OptionsCancel()
{
    OptionsPage& page = this->options;

    if (page.locked != 0)
    {
        return;
    }

    switch (page.stage)
    {
        case kOnTabs:
            --this->code;
            this->OptionsLeave();
            this->SetPage(this->code);
            return;

        case kOpened:
        case kConfirming:
            --page.stage;
            return;

        case kKeys:
            page.keyRow = kDiscardButton;
            return;

        case kCannotSave:
            page.stage = kOnTabs;
            return;

        case kDisplay:
            page.stage = kOpened;
            return;

        default:
            return;
    }
}

// The settings of the system tab (part of 0x0044C160): the labels, a bar of steps for each (the cache line, the
// effects line, the speed with its sample typing out), the version and the row highlight.
void Menu::OptionsDrawSettings(Pixel color)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    OptionsPage& page = this->options;
    Config& settings = owner.config;
    ActState& panel = owner.battleMessages.panelAct;
    TextFont& large = owner.largeFont;
    TextFont& medium = owner.mediumFont;
    this->gauge.id = Scene::InterfaceAct;
    this->gauge.action = kStepOff;
    FillBox(
        painter,
        kListBoxX,
        kListBoxY,
        kListBoxWidth,
        kSettingsBoxHeight + kDisplayRowHeight,
        0,
        0,
        0,
        FillMode::Quarter
    );
    DrawWindowFrame(
        painter,
        owner.archive,
        panel.frame,
        kListFrameLeft,
        kListFrameTop,
        kListFrameRight,
        kSettingsFrameBottom + kDisplayRowHeight,
        0,
        kFrameBorder
    );
    large.DrawText(owner.drawSurface, kVolumeLabelX, kRowY[kSoundRow], menu_text::kSoundVolume, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kVolumeLabelX, kRowY[kMusicRow], menu_text::kMusicVolume, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kLabelX, kRowY[kCacheRow], menu_text::kCacheSize, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kLabelX, kRowY[kEffectsRow], menu_text::kMapEffects, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kLabelX, kRowY[kTextSpeedRow], menu_text::kTextSpeed, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kLabelX, kRowY[kBattleSpeedRow], menu_text::kBattleSpeed, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kLabelX, kRowY[kKeysRow], kKeySetup, color, kPanelStyle);
    large.DrawText(owner.drawSurface, kLabelX, kRowY[kDisplayRow], kDisplaySetup, color, kPanelStyle);
    medium.DrawText(owner.drawSurface, kVersionX, kVersionY + kDisplayRowHeight, kVersion, color, kPanelStyle);

    auto bar = [this, &owner](int y, i32 value)
    {
        int step = 0;

        for (int x = kBarStepsLeft; x <= kBarStepsRight; x += kStepWidth)
        {
            this->gauge.action = step > value ? kStepOff : kStepOn;
            owner.actDrawer.Draw(this->gauge, x, y);
            ++step;
        }
    };

    bar(kRowY[kSoundRow], settings.soundVolume);
    bar(kRowY[kMusicRow], settings.musicVolume);
    int size = kMinCache;

    for (int x = kBarStepsLeft; x < kBarStepsLeft + (kCacheSteps + 1) * kStepWidth; x += kStepWidth)
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
            medium.DrawText(owner.drawSurface, kLineX, kRowY[kCacheRow] + kLineDrop, line, color, kPanelStyle);
        }

        owner.actDrawer.Draw(this->gauge, x, kRowY[kCacheRow]);
        size = static_cast<u16>(size + kCacheStep);
    }

    const int effectsOff = owner.state.GameFlag(kEffectsOffFlag) ? 1 : 0;

    for (int step = 0, x = kBarStepsLeft; x < kBarStepsLeft + 2 * kStepWidth; x += kStepWidth, ++step)
    {
        this->gauge.action = step == effectsOff ? kStepOff : kStepOn;
        owner.actDrawer.Draw(this->gauge, x, kRowY[kEffectsRow]);
    }

    const std::string effects =
        WinFormat(menu_text::kEffectsLine, {effectsOff != 0 ? menu_text::kOff : menu_text::kOn});
    medium.DrawText(owner.drawSurface, kLineX, kRowY[kEffectsRow] + kLineDrop, effects, color, kPanelStyle);

    for (int step = 0, x = kTextStepsRight; x > kBarStepsLeft - kStepWidth; x -= kStepWidth, ++step)
    {
        this->gauge.action = step == settings.textSpeed ? kStepOn : kStepOff;
        owner.actDrawer.Draw(this->gauge, x, kRowY[kTextSpeedRow]);
    }

    auto sampleColor = color;
    painter.TintPixels(std::span<u16>(&sampleColor, 1), kSampleTint, kSampleTint, kSampleTint);
    medium.DrawText(
        owner.drawSurface,
        kSampleX,
        kRowY[kTextSpeedRow] + kLineDrop,
        menu_text::kSample,
        sampleColor,
        kPanelStyle
    );
    const std::string speed =
        menu_text::SpeedSample(settings.textSpeed, static_cast<std::size_t>(std::max(page.sampleLength, 0)));

    if (--page.sampleDelay <= 0)
    {
        ++page.sampleLength;
        page.sampleDelay = kSampleDelayFactor * page.textSpeedShown + 1;
    }

    medium.DrawText(owner.drawSurface, kLineX, kRowY[kTextSpeedRow] + kLineDrop, speed, color, kPanelStyle);

    if (menu_text::SampleEnds(static_cast<std::size_t>(std::max(page.sampleLength, 0))))
    {
        page.sampleLength = 0;
    }

    bar(kRowY[kBattleSpeedRow], settings.battleSpeed);
    FillBox(
        painter,
        kHighlightX,
        kHighlightHeight * page.row + kHighlightTop,
        kHighlightWidth,
        kHighlightHeight,
        kHighlightRed,
        kHighlightGreen,
        0,
        FillMode::Gradient
    );
}

// The rebuild's display setup of the system tab (deviations.md), in the box of the settings: the labels, the choices
// of each row (the one in use bright), and the row highlight.
void Menu::OptionsDrawDisplay(Pixel color)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    TextFont& large = owner.largeFont;
    FillBox(painter, kListBoxX, kListBoxY, kListBoxWidth, kDisplayBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        painter,
        owner.archive,
        owner.battleMessages.panelAct.frame,
        kListFrameLeft,
        kListFrameTop,
        kListFrameRight,
        kDisplayFrameBottom,
        0,
        kFrameBorder
    );

    for (std::size_t row = 0; row < kDisplayLabels.size(); ++row)
    {
        large.DrawText(owner.drawSurface, kLabelX, kRowY[row], kDisplayLabels[row], color, kPanelStyle);
    }

    Pixel unchosen = color;
    painter.TintPixels(std::span<u16>(&unchosen, 1), kUnchosenTint, kUnchosenTint, kUnchosenTint);

    for (i32 row = kSmoothRow; row < kDisplayBackRow; ++row)
    {
        // The frame rate and the render scale only count with smooth motion.
        const bool counts = owner.smoothMotion || (row != kFrameRateRow && row != kRenderScaleRow);
        i32 chosen = -1;
        const std::vector<std::string> choices = this->OptionsDisplayChoices(row, chosen);
        const std::vector<int> lefts = ChoiceLefts(choices, large);

        for (std::size_t at = 0; at < choices.size(); ++at)
        {
            const bool shown = counts && static_cast<i32>(at) == chosen;
            large.DrawText(
                owner.drawSurface,
                lefts[at],
                kRowY[static_cast<std::size_t>(row)],
                choices[at],
                shown ? color : unchosen,
                kPanelStyle
            );
        }
    }

    FillBox(
        painter,
        kHighlightX,
        kHighlightHeight * this->options.displayRow + kHighlightTop,
        kHighlightWidth,
        kHighlightHeight,
        kHighlightRed,
        kHighlightGreen,
        0,
        FillMode::Gradient
    );
}

// The key setup of the system tab (part of 0x0044C160): the sixteen key rows with the keys of the copy, the buttons,
// the highlight; while waiting for a key the first key down is taken: Backspace gives up (on a row holding a key),
// a key held from before is let go first, Enter counts only on the frame it went down, the same key gives up, the
// reserved keys are refused, a key of another row moves there (that row is emptied and chosen next)
void Menu::OptionsDrawKeys(Pixel color)
{
    Game& owner = this->game;
    OptionsPage& page = this->options;
    Config& settings = owner.config;
    TextFont& medium = owner.mediumFont;
    this->DrawBox(kKeysBoxX, kKeysBoxY, kKeysBoxWidth, kKeysBoxHeight, static_cast<int>(FillMode::Eighth), 0, 0, 0);
    this->DrawBox(kKeyListX, kKeyListY, kKeyListWidth, kKeyListHeight, static_cast<int>(FillMode::Eighth), 0, 0, 0);
    this->DrawBox(kButtonsLeft, kButtonsY, kButtonsWidth, kButtonsHeight, static_cast<int>(FillMode::Eighth), 0, 0, 0);
    medium.DrawText(owner.drawSurface, kButtonsLeft, kButtonsY, kKeyButtons, color, kPanelStyle);

    for (std::size_t row = 0; row < Menu::KeyRowSlots.size(); ++row)
    {
        const int y = kKeyRowsY + static_cast<int>(kKeyRowHeight) * static_cast<int>(row);
        medium.DrawText(owner.drawSurface, kKeyLabelX, y, menu_text::kKeyRows[row], color, kPanelStyle);
        i32 key = page.keys[Menu::KeyRowSlots[row]];

        if (key == kNoKey)
        {
            key = 0;
        }

        medium.DrawText(owner.drawSurface, kKeyNameX, y, KeyName(key), color, kPanelStyle);
    }

    if (page.keyRow >= kApplyButton)
    {
        const auto& [x, width] = kButtonHighlights[static_cast<std::size_t>(page.keyRow - kApplyButton)];
        FillBox(
            owner.painter,
            x,
            kButtonsY,
            width,
            static_cast<int>(kKeyRowHeight),
            kHighlightRed,
            kHighlightGreen,
            0,
            FillMode::Gradient
        );
    }
    else
    {
        const int y = static_cast<int>(kKeyRowHeight) * page.keyRow + kKeyRowsY;

        if (page.stage == kKeyWaiting)
        {
            medium.DrawText(owner.drawSurface, kPressKeyX, y, kPressKey, color, kPanelStyle);
        }

        const int left = page.stage == kKeyWaiting ? kKeyHighlightWidth : 0;
        const int width = page.stage == kKeyWaiting ? kWaitHighlightWidth : kKeyHighlightWidth;
        FillBox(
            owner.painter,
            left + kKeyHighlightX,
            y - kKeyHighlightUp,
            width,
            static_cast<int>(kKeyRowHeight),
            kHighlightRed,
            kHighlightGreen,
            0,
            FillMode::Gradient
        );
    }

    if (page.stage != kKeyWaiting)
    {
        if (page.stage == kKeyChosen)
        {
            page.stage = kKeyWaiting;
        }

        return;
    }

    i32 key = owner.input.FirstKeyDown();
    const std::size_t slot = Menu::KeyRowSlots[static_cast<std::size_t>(page.keyRow)];

    if (owner.input.IsKeyDown(kBackspace) != 0)
    {
        if (page.keys[slot] != kNoKey)
        {
            page.stage = kKeys;
            owner.audio.PlaySound(kChosenSound, settings.soundVolume);
            return;
        }

        key = 0;
    }

    if (page.heldKey != 0)
    {
        page.heldKey = key;
        return;
    }

    if (key == settings.keys[static_cast<std::size_t>(Button::Enter)] &&
        owner.controls.At(Button::Enter).heldFrames != 1)
    {
        return;
    }

    if (key == page.keys[slot])
    {
        page.stage = kKeys;
        return;
    }

    if (key != 0)
    {
        if (std::find(kReservedKeys.begin(), kReservedKeys.end(), key) != kReservedKeys.end())
        {
            owner.audio.PlaySound(kRefusedSound, settings.soundVolume);
            owner.audio.PlaySound(kRefusedSound, settings.soundVolume);
        }
        else
        {
            std::size_t other = 0;

            while (other == static_cast<std::size_t>(page.keyRow) || page.keys[Menu::KeyRowSlots[other]] != key)
            {
                if (++other >= Menu::KeyRowSlots.size())
                {
                    break;
                }
            }

            if (other >= Menu::KeyRowSlots.size())
            {
                page.keys[slot] = key;
                page.stage = kKeys;
                owner.audio.PlaySound(kChosenSound, settings.soundVolume);
            }
            else
            {
                page.keyRow = static_cast<i32>(other);
                const std::size_t otherSlot = Menu::KeyRowSlots[other];
                page.heldKey = key;
                page.swappedKey = page.keys[otherSlot];
                page.keys[otherSlot] = kNoKey;
                page.keys[slot] = key;
                owner.audio.PlaySound(kSwapSound, settings.soundVolume);
                return;
            }
        }
    }

    page.heldKey = key;
}

// swd32.exe 0x0044C160: the system tab: the five tabs (the current one raised and brighter), the refusal to save, the
// notes with their scroll bar, the settings, the quit box with its confirmation and the quit fade (the game ends, or
// the title opens), the key setup
void Menu::OptionsDraw()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    OptionsPage& page = this->options;
    ActState& panel = owner.battleMessages.panelAct;
    const auto color = static_cast<Pixel>(painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    owner.acts.Run(panel);
    FillBox(
        painter,
        kTabsBoxX,
        kTabsBoxY,
        kTabsBoxWidth,
        kTabsBoxHeight,
        kTabsBoxRed,
        kTabsBoxGreen,
        kTabsBoxBlue,
        FillMode::Quarter
    );
    DrawWindowFrame(
        painter,
        owner.archive,
        panel.frame,
        kTabsFrameLeft,
        kTabsFrameTop,
        kTabsFrameRight,
        kTabsFrameBottom,
        0,
        kFrameBorder
    );
    Pixel tabColor = color;

    for (std::size_t tab = 0; tab < kTabLabels.size(); ++tab)
    {
        int raise = kRaised;
        tabColor = color;

        if (static_cast<i32>(tab) != page.entry)
        {
            painter.TintPixels(std::span<u16>(&tabColor, 1), kOtherTabTint, kOtherTabTint, kOtherTabTint);
            raise = 0;
        }

        owner.mediumFont.DrawText(
            owner.drawSurface,
            kTabStep * static_cast<int>(tab) + raise + kTabX,
            raise + kTabY,
            kTabLabels[tab],
            tabColor,
            kPanelStyle
        );
    }

    if (page.stage == kCannotSave)
    {
        this->DrawBox(
            kMessageX,
            kMessageY,
            kMessageWidth,
            kMessageHeight,
            static_cast<int>(FillMode::Quarter),
            0,
            0,
            0
        );
        // The original draws it in the colour of the last tab.
        owner.largeFont.DrawText(owner.drawSurface, kMessageX, kMessageY, kCannotSaveText, tabColor, kPanelStyle);
        return;
    }

    if (page.stage == kOpened || page.stage == kConfirming)
    {
        if (page.entry == kNotesEntry)
        {
            FillBox(painter, kListBoxX, kListBoxY, kListBoxWidth, kNotesBoxHeight, 0, 0, 0, FillMode::Quarter);
            DrawWindowFrame(
                painter,
                owner.archive,
                panel.frame,
                kListFrameLeft,
                kListFrameTop,
                kListFrameRight,
                kNotesFrameBottom,
                0,
                kFrameBorder
            );

            if (page.noteCount > kNotesPerPage)
            {
                int arrows = 0;

                if ((page.arrows & kUpArrowMask) != 0)
                {
                    page.arrows = (page.arrows & ~kUpArrowMask) | ((page.arrows & kUpArrowMask) - 1);
                    arrows = kUpArrowPressed;
                }

                if ((page.arrows & kDownArrowMask) != 0)
                {
                    page.arrows -= kDownArrowStep;
                    arrows |= kDownArrowPressed;
                }

                const auto count = static_cast<double>(page.noteCount);
                const auto to = static_cast<float>(static_cast<double>(page.noteTop + page.notesShown) / count);
                const auto from = static_cast<float>(static_cast<double>(page.noteTop) / count);
                this->DrawScrollBar(kNotesBarX, kNotesBarY, kNotesBarHeight, arrows, from, to, this->scrollBar);
            }

            for (int shown = 0; shown < page.notesShown; ++shown)
            {
                this->DrawNote(shown + page.noteTop, kNoteX, kNotesTop + kNoteHeight * shown);
            }
        }

        if (page.entry == kSettingsEntry)
        {
            this->OptionsDrawSettings(color);
        }

        if (page.entry == kQuitEntry)
        {
            FillBox(painter, kQuitBoxX, kQuitBoxY, kQuitBoxWidth, kQuitBoxHeight, 0, 0, 0, FillMode::Quarter);
            DrawWindowFrame(
                painter,
                owner.archive,
                panel.frame,
                kQuitFrameLeft,
                kQuitFrameTop,
                kQuitFrameRight,
                kQuitFrameBottom,
                0,
                kFrameBorder
            );
            owner.largeFont.DrawText(owner.drawSurface, kQuitTextX, kEndTextY, kEndGameText, color, kPanelStyle);
            owner.largeFont.DrawText(owner.drawSurface, kQuitTextX, kRestartTextY, kRestartText, color, kPanelStyle);
            FillBox(
                painter,
                kQuitBoxX,
                static_cast<int>(kQuitRowHeight) * page.row + kQuitHighlightTop,
                kQuitBoxWidth,
                static_cast<int>(kQuitRowHeight),
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }

        if (page.stage == kConfirming)
        {
            if (page.entry != kQuitEntry)
            {
                return;
            }

            if ((this->flags & kGameSavedFlag) == 0)
            {
                FillBox(
                    painter,
                    kConfirmBoxX,
                    kConfirmBoxY,
                    kConfirmBoxWidth,
                    kConfirmBoxHeight,
                    0,
                    0,
                    0,
                    FillMode::Quarter
                );
                DrawWindowFrame(
                    painter,
                    owner.archive,
                    panel.frame,
                    kConfirmFrameLeft,
                    kConfirmFrameTop,
                    kConfirmFrameRight,
                    kConfirmFrameBottom,
                    0,
                    kFrameBorder
                );
                std::string question = Big5(kProgressLost);
                question += Big5(page.quitKind == kEndGame ? kEndQuestion : kRestartQuestion);
                owner.largeFont.DrawText(owner.drawSurface, kQuestionX, kQuestionY, question, color, kPanelStyle);
                owner.largeFont.DrawText(owner.drawSurface, kConfirmX, kChoicesY, kConfirm, color, kPanelStyle);
                owner.largeFont.DrawText(owner.drawSurface, kGiveUpX, kChoicesY, kGiveUp, color, kPanelStyle);
                FillBox(
                    painter,
                    static_cast<int>(kChoiceWidth) * page.choice + static_cast<int>(kChoicesLeft),
                    kChoiceHighlightY,
                    static_cast<int>(kChoiceWidth),
                    kChoiceHighlightHeight,
                    kHighlightRed,
                    kHighlightGreen,
                    0,
                    FillMode::Gradient
                );
            }

            if (page.choice >= kQuitFadeFrom)
            {
                // The rebuild's continuous motion: the fade ends by leaving the game, which is not painted again.
                this->KeepFrameFrom(false);
                this->tint = kQuitFadeFrom - page.choice++;

                if (page.choice >= kQuitFadeEnd)
                {
                    this->OptionsLeave();

                    if (page.quitKind == kEndGame)
                    {
                        owner.state.ClearFrameInterval();
                        owner.menuState = Game::NoMenu;
                        owner.inactive = 1;
                        owner.state.ClearGameFlag(GameState::Running);
                        owner.systemFlags |= Game::QuitRequested;
                        return;
                    }

                    if (page.quitKind == kRestart)
                    {
                        owner.menuState = Game::TitleMenu;
                        owner.storyState = 0;
                        owner.unk4CAEBC = 0;
                        owner.unk4CAEC0 = 0;
                    }
                }
            }
        }
    }

    if (page.stage == kKeys || page.stage == kKeyChosen || page.stage == kKeyWaiting)
    {
        this->OptionsDrawKeys(color);
    }

    if (page.stage == kDisplay)
    {
        this->OptionsDrawDisplay(color);
    }
}

}  // namespace swd3
