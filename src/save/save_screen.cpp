#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/byte_reader.h"
#include "core/debug_log.h"
#include "core/format.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "map/map_cache.h"
#include "platform/display.h"
#include "platform/system_info.h"
#include "save/save_games.h"
#include "ui/marching_rect.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr i32 kKeyBlockLimit = 0x3E8;  // Game::slotMouseBlock past this is reset
constexpr i32 kKeyBlockFrames = 0x78;  // a key or click blocks the mouse slot choice this long
constexpr i32 kSaveSlotsShown = 1;     // Game::storyState once the save screen runs
constexpr i32 kFlagView = 0;           // Game::saveScreenPage: the flag view (F1)
constexpr i32 kSlotsPage = 1;          // the save slots (F2)
constexpr u32 kKeyPause = 0x1F4;       // milliseconds after F1 or F2
constexpr u32 kLeavePause = 0x96;
constexpr u32 kSavedPause = 0x258;
constexpr u32 kSettledFrames = 0xA;  // frames of the save screen before a slot can be saved or loaded

// The flag view.
constexpr i32 kFlags = 0x2000;
constexpr i32 kFlagsPerPage = 0x78;
constexpr i32 kFlagRowStep = 6;
constexpr int kFlagTop = 0x20;
constexpr int kFlagColumnStep = 0x64;
constexpr int kFlagColumnsEnd = 0x258;
constexpr int kFlagRowHeight = 0x16;
constexpr int kFlagRowsEnd = 0x1D8;
constexpr int kFlagMarkX = 0x40;
constexpr int kHeldStart = 3;  // FlagViewKeys: a direction repeats after this many held frames
constexpr std::string_view kFlagFormat = "[%5d:  ]";
constexpr std::string_view kFlagSet = "@";
constexpr std::string_view kFlagClear = " ";
constexpr Pixel kWhite = 0xFFFF;
constexpr Pixel kFlagSetColor = 0xFF00;
constexpr Pixel kCursorBackground = 0xFFFF;
constexpr Pixel kNoBackground = 0xFFFE;
constexpr int kPlainStyle = 1;

// The slot prompt (Game::slotPrompt).
constexpr i32 kBrowsing = 0;
constexpr i32 kOverwriteYes = 5;
constexpr i32 kOverwriteNo = 6;
constexpr i32 kSlotChosen = 7;

// The mouse areas of the slots page.
constexpr u32 kSlotsLeft = 0x10;
constexpr u32 kSlotsRight = 0x270;
constexpr u32 kSlotsTop = 0x28;
constexpr u32 kSlotsBottom = 0x1CC;
constexpr u32 kSlotHeight = 0x8C;
constexpr u32 kAnswersLeft = 0x190;
constexpr u32 kAnswersRight = 0x1E6;
constexpr u32 kAnswerWidth = 0x2C;
constexpr u32 kAnswersTop = 0x5E;
constexpr u32 kAnswersBottom = 0x76;
constexpr u32 kUpArrowBottom = 0x1D;
constexpr u32 kDownArrowTop = 0x1C2;
constexpr u32 kDownArrowBottom = 0x1E5;
constexpr i32 kUpPointer = 0;  // Scene::pointerRole actions over the arrows
constexpr i32 kDownPointer = 1;
constexpr u8 kArrowFlash = 3;

constexpr u16 kSlotSound = 0x107;
constexpr u16 kPageSound = 0xC2;
constexpr u16 kEmptySlotSound = 0xB8;

// The save file (reverse/analysis/save-format.md).
constexpr std::string_view kSaveDateFormat = "%04d%02d%02d%02d%02d";
constexpr std::size_t kFirstBlockAt =
    SaveSlots::HeaderBytes + sizeof(u16) * SaveSlots::ThumbnailPixels + SaveSlots::LocationBytes;
constexpr std::size_t kBlockHeaderBytes = 2 * sizeof(u32);  // a packed block: its packed and its unpacked size
constexpr u32 kMapsHeaderBytes = 0x200;                     // maps.dat before GameDatabases::mapsData
constexpr int kTileShift = 4;                               // map pixels to tiles
constexpr std::size_t kRoleRecordBytes = 0x16;              // a record of maps.dat's role table
constexpr std::size_t kRecordMap = 0;                       // its words
constexpr std::size_t kRecordId = 1;
constexpr std::size_t kRecordAct = 2;
constexpr std::size_t kRecordAction = 3;
constexpr std::size_t kRecordDirection = 4;
constexpr std::size_t kRecordX = 5;
constexpr std::size_t kRecordY = 6;
constexpr std::size_t kRecordEvent = 7;
constexpr std::size_t kRecordTalk = 8;
constexpr std::size_t kRecordSetBits = 10;
constexpr std::size_t kPlaceMapAt = 4;  // the seven dwords of the place after the role table
constexpr std::size_t kPlaceXAt = 8;
constexpr std::size_t kPlaceYAt = 0xC;
constexpr std::size_t kPlaceActAt = 0x10;
constexpr std::size_t kPlaceActionAt = 0x14;
constexpr std::size_t kPlaceDirectionAt = 0x18;
constexpr std::size_t kPlaceBytes = 0x1C;
constexpr std::size_t kPartyBufferScale = 2;  // the party block is unpacked into twice its size
constexpr u16 kPendingBit = 0x8000;           // an inventory entry of a pending count
constexpr std::size_t kMusicRecordBytes = 8;  // a scene music record of maps.dat
constexpr std::size_t kMusicCueAt = 6;
constexpr u16 kMusicKept = 0x800;  // a record with this cue bit is not loaded back
constexpr std::size_t kReturnBytes = 0x40;
constexpr std::size_t kReturnColumnAt = 0;
constexpr std::size_t kReturnRowAt = 2;
constexpr std::size_t kReturnMapAt = 4;
constexpr std::size_t kReturnUnkAt = 6;
constexpr std::size_t kReturnTimerAt = 8;
constexpr std::size_t kReturnEventAt = 0xC;
constexpr std::size_t kNamesTimeBytes = sizeof(Party::names) + sizeof(u32) + kReturnBytes;
constexpr std::size_t kShakeBytes = 0x84;
constexpr std::size_t kShakeHoldAt = 0;
constexpr std::size_t kShakeFramesAt = 2;
constexpr std::size_t kShakeOffsetsAt = 4;
constexpr std::size_t kFileNameBytes = 0x34;  // Scene::noTownWarpText and noExitWarpText
constexpr u32 kFileNamesBytes = 2 * kFileNameBytes;
constexpr std::size_t kFileNamesBuffer = 0xA8;
constexpr std::array<int, 6> kKeptFlags = {5, 6, 7, 8, 0x13, 0x16};  // keep their saved values across LoadCity
constexpr int kClearedOnLoadFlag = 0xA;
constexpr u32 kLoadedCueMask = 0xFF5CFF00;  // Scene::musicCue bits kept when a game is loaded over story music
constexpr u32 kGameSavedFlag = 2;           // Menu::flags

// The slots page.
constexpr u16 kUnk4CC2AALimit = 0xD;
constexpr u32 kTextRed = 0x1D;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x10;
constexpr i32 kSlotsAct = 0x232A;
constexpr i32 kSaveTitleAction = 0x4C;
constexpr i32 kLoadTitleAction = 0x4D;
constexpr int kTitleY = 4;
constexpr int kSlotStep = static_cast<int>(kSlotHeight);
constexpr int kInfoX = 0xC0;
constexpr int kInfoTop = 0x32;
constexpr int kInfoWidth = 0x1B8;
constexpr int kInfoHeight = 0x7C;
constexpr int kDateX = 0xC8;
constexpr int kThumbnailX = 0x14;
constexpr int kThumbnailTop = 0x34;
constexpr int kThumbnailWidth = 0xA0;
constexpr int kThumbnailHeight = 0x78;
constexpr int kPlaceTop = 0x9C;
constexpr int kPlaceHeight = 0xE;
constexpr int kPlaceShade = -12;
constexpr int kPlaceCharWidth = 3;
constexpr int kPlaceChars = 0x20;
constexpr std::size_t kPlaceColor = 5;  // swd32.exe 0x0049E0DC: the low word of the converted primary text colour 5
constexpr u16 kAbsentRole = 0xFFFF;     // SavePreviewRole::maxHitPoints of a role not in the party
constexpr int kFaceStep = 0x48;
constexpr int kFaceX = 0xE0;
constexpr int kFaceTop = 0x88;
constexpr int kSecondFaceDrop = 0x10;
constexpr std::u8string_view kLevelFormat = u8"%d級";  // swd32.exe 0x0049FA84
constexpr int kDigitWidth = 4;
constexpr int kLevelStep = 0x12;
constexpr int kLevelX = 0xF0;
constexpr int kGoldRight = 0x26E;
constexpr int kGoldTop = 0x3C;
constexpr int kPlayTimeRight = 0x26C;
constexpr int kPlayTimeCharWidth = 9;
constexpr int kNoSaveX = 0x1FC;
constexpr std::u8string_view kNoSave = u8"沒有存檔資訊";  // swd32.exe 0x0049FA74
constexpr int kFrameLeftA = 0x16;
constexpr int kFrameRightA = 0xB2;
constexpr int kFrameLeftB = 0xC4;
constexpr int kFrameRightB = 0x270;
constexpr int kFrameTop = 0x37;
constexpr int kFrameBottom = 0xA9;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr std::string_view kSlotFormat = "%02d";
constexpr int kPanelStyle = 4;
constexpr int kPromptLeft = 0x190;
constexpr int kPromptRight = 0x1F4;
constexpr int kPromptTop = 0x46;
constexpr int kPromptBottom = 0x78;
constexpr int kPromptStyle = 8;
constexpr std::u8string_view kOverwrite = u8"要覆蓋嗎?";  // swd32.exe 0x0049FA60
constexpr std::u8string_view kAnswers = u8"  是  否";     // swd32.exe 0x0049FA54
constexpr int kAnswerBase = 0xB4;
constexpr int kAnswerHeight = 0x18;
constexpr int kAnswerRed = 0x14;
constexpr int kAnswerGreen = 0xD;
constexpr int kArrowReach = 2;  // the arrows show while slots lie this far beyond the slot chosen
constexpr int kArrowX = 0x130;
constexpr int kDownArrowY = 0x1C0;
constexpr int kUpArrowY = 4;
constexpr u16 kDownArrow = 0x2460;
constexpr u16 kDownArrowLit = 0x2462;
constexpr u16 kUpArrow = 0x245F;
constexpr u16 kUpArrowLit = 0x2461;

u16 ReadWord(std::span<const u8> bytes, std::size_t at)
{
    return ByteReader::Word16At(bytes, at);
}

u32 ReadDword(std::span<const u8> bytes, std::size_t at)
{
    return ByteReader::Word32At(bytes, at);
}

void PutWord(std::vector<u8>& bytes, std::size_t at, u16 value)
{
    bytes[at] = LowByte(value);
    bytes[at + 1] = HighByte(value);
}

void PutDword(std::vector<u8>& bytes, std::size_t at, u32 value)
{
    PutWord(bytes, at, LowWord(value));
    PutWord(bytes, at + sizeof(u16), HighWord(value));
}

}  // namespace

// swd32.exe 0x00406E30: a frame of the save screen (Game::storyState): the flag view (F1) or the save slots (F2) with
// their keys, drawn while it stays open
void Game::SaveScreenFrame()
{
    if (--this->slotMouseBlock < 0 || this->slotMouseBlock > kKeyBlockLimit)
    {
        this->slotMouseBlock = 0;
    }

    ++this->saveFrames;

    if (this->storyState == Game::SaveSlotsState)
    {
        this->storyState = kSaveSlotsShown;
    }

    this->scene.pointerRole.act.action = Scene::ChoicePointer;
    this->SaveScreenKeys();

    if (this->saveScreenPage == kFlagView)
    {
        this->FlagViewKeys();
    }
    else if (this->saveScreenPage == kSlotsPage)
    {
        this->SaveSlotsFrame();
    }

    if (this->storyState != Game::StoryRunning)
    {
        this->DrawSaveScreen();
    }
}

// swd32.exe 0x00406EB0: a right click is Escape; F1 and F2 (only when not opened by a menu) switch to the flag view and
// to the slots; Escape or a Ctrl key leaves
void Game::SaveScreenKeys()
{
    const ButtonState& right = this->controls.At(Button::MouseRight);

    if (right.press != 0 && right.heldFrames == 1)
    {
        this->input.PressKey(Input::Escape);
    }

    if (this->input.IsKeyDown(Input::F1) != 0 && this->menuMode == Menu::NoSlotsMode)
    {
        SleepMilliseconds(kKeyPause);
        this->saveScreenPage = kFlagView;
    }

    if (this->input.IsKeyDown(Input::F2) != 0 && this->menuMode == Menu::NoSlotsMode)
    {
        SleepMilliseconds(kKeyPause);
        this->saveScreenPage = kSlotsPage;
    }

    if (this->input.IsKeyDown(Input::Escape) != 0 || this->input.IsKeyDown(Input::LeftControl) != 0 ||
        this->input.IsKeyDown(Input::RightControl) != 0)
    {
        this->LeaveSaveScreen();
    }
}

// swd32.exe 0x00406F70: the keys of the flag view: Space toggles the flag chosen; the arrows move by one or a row of
// six, the page keys by a page (held: after three frames), wrapping around the 0x2000 flags
void Game::FlagViewKeys()
{
    auto held = [this](Button button)
    {
        const ButtonState& state = this->controls.At(button);
        return state.press > 0 && (state.heldFrames > kHeldStart || state.heldFrames == 1);
    };

    i32 step = 0;
    const ButtonState& space = this->controls.At(Button::Space);

    if (space.press > 0 && space.heldFrames == 1)
    {
        if (this->state.GameFlag(this->flagCursor))
        {
            this->state.ClearGameFlag(this->flagCursor);
        }
        else
        {
            this->state.SetGameFlag(this->flagCursor);
        }
    }

    if (held(Button::Left))
    {
        step = -1;
    }

    if (held(Button::Right))
    {
        step = 1;
    }

    if (held(Button::Up))
    {
        step = -kFlagRowStep;
    }

    if (held(Button::Down))
    {
        step = kFlagRowStep;
    }

    if (held(Button::PageUp))
    {
        step = -kFlagsPerPage;
    }

    if (held(Button::PageDown))
    {
        step = kFlagsPerPage;
    }

    this->flagCursor += step;

    if (this->flagCursor < 0)
    {
        this->flagCursor = kFlags - 1;
    }
    else if (this->flagCursor >= kFlags)
    {
        this->flagCursor = 0;
    }
}

// swd32.exe 0x00409540: the save screen left, as its opener wants: back to the map (keys), to the title (the title's
// load), to the story (talk command) or to the items page; the previews freed
void Game::LeaveSaveScreen()
{
    switch (this->menuMode)
    {
        case Menu::KeySaveMode:
        case Menu::KeyLoadMode:
            this->storyState = Game::StoryRunning;
            this->DrawSaveScreen();
            SleepMilliseconds(kLeavePause);
            this->menuMode = Menu::NoSlotsMode;
            break;

        case Menu::TitleLoadMode:
            this->menu.SystemEnter();
            this->menuState = Game::TitleMenu;
            this->menuMode = Menu::NoSlotsMode;
            this->storyState = Game::StoryRunning;
            break;

        case Menu::ScriptSaveMode:
            this->storyState = Game::StoryRunning;
            this->menuState = Game::NoMenu;
            this->menuMode = Menu::NoSlotsMode;
            break;

        case Menu::ItemSaveMode:
            this->storyState = Game::StoryRunning;
            this->menuMode = Menu::NoSlotsMode;
            this->menu.ItemsAfterSave(0);
            break;

        default:
            break;
    }

    this->FreeSavePreviews();
}

// swd32.exe 0x00406E00: the three previews freed (0x004099C0 three times)
void Game::FreeSavePreviews()
{
    for (SavePreview& preview : this->saveSlots.previews)
    {
        SaveSlots::Free(preview);
    }
}

// The previews of the page of the slot chosen loaded again with their frames (inline in 0x004070A0).
void Game::ReloadSavePage()
{
    this->FreeSavePreviews();
    const int first = SaveSlots::Visible * (this->saveSlots.newest / SaveSlots::Visible);

    for (int index = 0; index < SaveSlots::Visible; ++index)
    {
        SaveSlots::LoadPreview(
            this->saveSlots.previews[static_cast<std::size_t>(index)],
            first + index,
            this->config.SaveDirectory(),
            this->painter
        );
    }

    this->saveSlots.ResetActs();
}

// swd32.exe 0x00408CF0: the save screen drawn and shown: the flag view on black or the slots page
void Game::DrawSaveScreen()
{
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);

    if (this->saveScreenPage == kFlagView)
    {
        std::fill_n(this->drawSurface, static_cast<std::size_t>(Display::Width) * Display::Height, u16{0});
        this->DrawFlagView();
    }
    else if (this->saveScreenPage == kSlotsPage)
    {
        this->DrawSlotsPage();
    }

    this->ShowFrame();
}

// swd32.exe 0x00408D70: the page of 120 flags holding the flag chosen, six to a row: "[number:  ]" (the chosen one
// black on white) and "@" in yellow for a flag set
void Game::DrawFlagView()
{
    int y = kFlagTop;
    i32 flag = kFlagsPerPage * (this->flagCursor / kFlagsPerPage);

    while (true)
    {
        for (int x = 0; x < kFlagColumnsEnd; x += kFlagColumnStep)
        {
            Pixel color = kWhite;

            if (flag == this->flagCursor)
            {
                this->mediumFont.SetBackground(kCursorBackground);
                color = 0;
            }

            const std::string text = WinFormat(kFlagFormat, {flag});
            this->mediumFont.DrawText(this->drawSurface, x, y, text, color, kPlainStyle);

            if (flag == this->flagCursor)
            {
                this->mediumFont.SetBackground(kNoBackground);
            }

            const bool set = this->state.GameFlag(flag);
            this->mediumFont.DrawText(
                this->drawSurface,
                x + kFlagMarkX,
                y,
                set ? kFlagSet : kFlagClear,
                set ? kFlagSetColor : Pixel{0},
                kPlainStyle
            );

            if (++flag >= kFlags)
            {
                return;
            }
        }

        y += kFlagRowHeight;

        if (y >= kFlagRowsEnd)
        {
            return;
        }
    }
}

// swd32.exe 0x004070A0: the save slots page (save or load by Game::slotsLoad): the mouse picks a slot (a double click
// takes it) or answers the overwrite prompt and the arrows page; the keys move by one or a page of three, Space or
// Enter takes the slot (a used slot asks first when saving), Escape or Right Ctrl leaves. A slot taken is saved or
// loaded once the screen ran ten frames; until then the choice stays pending. (The original keeps it only for the frame
// it was taken: taken within the first ten frames, the slot is never saved or loaded and the screen answers nothing but
// Escape; here it stays taken until then.)
// swd32.exe 0x00498010 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498018 (EH handler of 0x004070A0)
void Game::SaveSlotsFrame()
{
    const ButtonState& mouse = this->controls.At(Button::MouseLeft);
    SlotsStep step = SlotsStep::Keys;

    if (this->slotPrompt == kSlotChosen)
    {
        step = SlotsStep::Chosen;
    }
    else if (mouse.press != 0 && mouse.heldFrames != 0)
    {
        this->slotMouseBlock = 0;
        step = this->SlotsMouse();
    }
    else if (this->slotMouseBlock == 0)
    {
        step = this->SlotsMouse();
    }

    if (step == SlotsStep::Leave)
    {
        this->LeaveSaveScreen();
        return;
    }

    if (step == SlotsStep::Done)
    {
        return;
    }

    if (step == SlotsStep::Keys)
    {
        step = this->SlotsKeys();

        if (step == SlotsStep::Done)
        {
            return;
        }
    }

    if (step != SlotsStep::Chosen)
    {
        const ButtonState& escape = this->controls.At(Button::Escape);
        const ButtonState& control = this->controls.At(Button::RightControl);

        if ((escape.press == 1 && escape.heldFrames == 1) || (control.press == 1 && control.heldFrames == 1))
        {
            this->LeaveSaveScreen();
            return;
        }
    }

    if (this->saveFrames <= kSettledFrames)
    {
        return;
    }

    if (step == SlotsStep::Chosen && this->slotPrompt == kSlotChosen && this->slotsLoad == 0)
    {
        this->SaveGame();
        return;
    }

    if (step == SlotsStep::Chosen && this->slotsLoad == 1)
    {
        this->LoadGame();
    }
}

// The mouse on the slots page (inline in 0x004070A0): a click on a slot chooses it, a double click takes it (saving
// over a used slot asks first; loading a free slot only sounds), a click on an answer of the prompt answers it, a right
// double click leaves; over the arrows the pointer turns and a click pages.
Game::SlotsStep Game::SlotsMouse()
{
    const ButtonState& mouse = this->controls.At(Button::MouseLeft);
    const ButtonState& right = this->controls.At(Button::MouseRight);
    const i32 click = mouse.press;
    const auto x = static_cast<u32>(this->state.pointerX);
    const auto y = static_cast<u32>(this->state.pointerY);
    i32 prompt = this->slotPrompt;
    const bool inSlots = x < kSlotsRight && x > kSlotsLeft && y < kSlotsBottom && y > kSlotsTop;

    if (prompt == kBrowsing && inSlots)
    {
        if (click == 1 && mouse.heldFrames != 0)
        {
            const i32 delta =
                static_cast<i32>((y - kSlotsTop) / kSlotHeight) - this->saveSlots.newest % SaveSlots::Visible;
            this->saveSlots.newest += delta;

            if (delta != 0)
            {
                this->audio.PlaySound(kSlotSound, this->config.soundVolume);
                this->saveSlots.ResetActs();
            }
        }

        if (click == 2 && mouse.heldFrames != 0)
        {
            const bool free = SaveGames::IsFree(this->config.SaveDirectory(), this->saveSlots.newest);

            if (this->slotsLoad == 0)
            {
                if (!free)
                {
                    this->slotPrompt = kOverwriteYes;
                    this->controls.buttons.fill(ButtonState{});
                    return SlotsStep::Done;
                }

                return this->TakeSlot();
            }

            if (!free)
            {
                return this->TakeSlot();
            }

            this->slotPrompt = kBrowsing;
            this->audio.PlaySound(kEmptySlotSound, this->config.soundVolume);
            prompt = kBrowsing;
        }

        if (right.press == 2 && right.heldFrames != 0)
        {
            return SlotsStep::Leave;
        }
    }

    if (prompt >= kOverwriteYes)
    {
        const u32 row = kSlotHeight * static_cast<u32>(this->saveSlots.newest % SaveSlots::Visible);

        if (x < kAnswersRight && x > kAnswersLeft && y < row + kAnswersBottom && y > row + kAnswersTop)
        {
            const u32 answer = (x - kAnswersLeft) / kAnswerWidth;
            prompt = static_cast<i32>(answer) + kOverwriteYes;
            this->slotPrompt = prompt;

            if (click != 0 && mouse.heldFrames != 0)
            {
                if (answer == 0)
                {
                    return this->TakeSlot();
                }

                this->slotPrompt = kBrowsing;
                this->controls.buttons.fill(ButtonState{});
                return SlotsStep::Done;
            }

            if (right.press != 0 && right.heldFrames != 0)
            {
                this->slotPrompt = kBrowsing;
                this->controls.buttons.fill(ButtonState{});
                return SlotsStep::Done;
            }
        }
    }

    if (x < static_cast<u32>(Display::Width) && x != 0 && prompt == kBrowsing)
    {
        if (y <= kUpArrowBottom)
        {
            this->scene.pointerRole.act.action = kUpPointer;

            if (click != 0 && mouse.heldFrames == 1)
            {
                ButtonState& pageUp = this->controls.At(Button::PageUp);
                pageUp.press = 2;
                pageUp.heldFrames = 1;
            }
        }

        if (y < kDownArrowBottom && y > kDownArrowTop)
        {
            this->scene.pointerRole.act.action = kDownPointer;

            if (click != 0 && mouse.heldFrames == 1)
            {
                ButtonState& pageDown = this->controls.At(Button::PageDown);
                pageDown.press = 1;
                pageDown.heldFrames = 1;
            }
        }
    }

    return SlotsStep::Keys;
}

// A slot taken (inline in 0x004070A0): the prompt marks it.
Game::SlotsStep Game::TakeSlot()
{
    this->slotPrompt = kSlotChosen;
    return SlotsStep::Chosen;
}

// The keys of the slots page (inline in 0x004070A0): Left or Up and Right or Down move by a slot, Page Up and Page Down
// by three (each blocking the mouse choice a while); on the prompt they pick the answer. Space or Enter takes the slot,
// answers the prompt or (loading a free slot) only sounds.
Game::SlotsStep Game::SlotsKeys()
{
    auto repeating = [this](Button button)
    {
        return this->controls.At(button).press > 0 && this->controls.Repeating(button);
    };

    auto move = [this](i32 delta, u16 sound, bool reloadAlways, u8& flash)
    {
        this->slotMouseBlock = kKeyBlockFrames;

        if (this->slotPrompt == kBrowsing)
        {
            this->saveSlots.newest += delta;

            if (delta < 0 ? this->saveSlots.newest >= 0 : this->saveSlots.newest < SaveGames::LastSlot)
            {
                this->audio.PlaySound(sound, this->config.soundVolume);

                if (!reloadAlways)
                {
                    this->ReloadSavePage();
                }
            }
            else
            {
                this->scene.pointerRole.act.action = Scene::ChoicePointer;
                this->saveSlots.newest = delta < 0 ? 0 : SaveGames::LastSlot;
            }

            if (reloadAlways)
            {
                this->ReloadSavePage();
            }

            flash = kArrowFlash;
        }
    };

    auto answer = [this](i32 prompt)
    {
        if (this->slotPrompt >= kOverwriteYes && this->slotPrompt != kSlotChosen)
        {
            this->slotPrompt = prompt;
        }
    };

    if (repeating(Button::Left) || repeating(Button::Up))
    {
        move(-1, kSlotSound, false, this->downArrowPress);
        answer(kOverwriteYes);
    }

    if (repeating(Button::Right) || repeating(Button::Down))
    {
        move(1, kSlotSound, false, this->upArrowPress);
        answer(kOverwriteNo);
    }

    if (repeating(Button::PageUp))
    {
        move(-SaveSlots::Visible, kPageSound, true, this->upArrowPress);
        answer(kOverwriteYes);
    }

    if (repeating(Button::PageDown))
    {
        move(SaveSlots::Visible, kPageSound, true, this->downArrowPress);
        answer(kOverwriteNo);
    }

    const ButtonState& space = this->controls.At(Button::Space);
    const ButtonState& enter = this->controls.At(Button::Enter);

    if (!((space.press != 0 && space.heldFrames == 1) || (enter.press != 0 && enter.heldFrames == 1)))
    {
        return SlotsStep::Keys;
    }

    this->slotMouseBlock = kKeyBlockFrames;

    if (this->slotsLoad != 0)
    {
        if (SaveGames::IsFree(this->config.SaveDirectory(), this->saveSlots.newest))
        {
            this->audio.PlaySound(kEmptySlotSound, this->config.soundVolume);
            this->slotPrompt = kBrowsing;
            return SlotsStep::Keys;
        }

        return this->TakeSlot();
    }

    switch (this->slotPrompt)
    {
        case kBrowsing:
            if (!SaveGames::IsFree(this->config.SaveDirectory(), this->saveSlots.newest))
            {
                this->slotPrompt = kOverwriteYes;
                return SlotsStep::Done;
            }

            return this->TakeSlot();

        case kOverwriteYes:
            return this->TakeSlot();

        case kOverwriteNo:
            this->slotPrompt = kBrowsing;
            return SlotsStep::Done;

        default:
            return SlotsStep::Keys;
    }
}

// The slot chosen saved (inline in 0x004070A0; the layout is in reverse/analysis/save-format.md): Fame.dat first, the
// roles of the map written back, then the file truncated and written; the page shown again for 600 ms and the screen
// left (the items page gets its list back with one save item less).
void Game::SaveGame()
{
    this->SaveFameFile();
    this->menu.flags |= kGameSavedFlag;
    this->slotPrompt = kBrowsing;
    const Role& leader = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    this->SaveRoleStates(
        static_cast<i16>(static_cast<u32>(leader.x) >> kTileShift),
        static_cast<i16>(static_cast<u32>(leader.y) >> kTileShift)
    );
    this->audioDevice.Serve();
    File file;

    if (file.Open(
            SaveGames::SlotPath(this->config.SaveDirectory(), this->saveSlots.newest),
            File::Disposition::OpenAlways,
            File::Access::ReadWrite
        ))
    {
        this->WriteSave(file);
        file.Close();
    }

    this->ReloadSavePage();
    this->saveFrames = 0;
    this->storyState = Game::StoryRunning;
    this->DrawSaveScreen();
    SleepMilliseconds(kSavedPause);

    if (this->menuMode == Menu::ItemSaveMode)
    {
        this->menu.ItemsAfterSave(1);
    }

    this->menuMode = Menu::NoSlotsMode;
    this->FreeSavePreviews();
}

// The save written to the open slot file (inline in 0x004070A0).
void Game::WriteSave(File& file)
{
    auto write = [&file](const void* data, std::size_t size)
    {
        u32 bytes = static_cast<u32>(size);
        file.Write(data, bytes);
    };

    auto writeBlock = [&file, &write](std::span<const u8> data, u32 raw)
    {
        const std::vector<u8> packed = MapCache::Pack(data);
        file.WriteDword(static_cast<u32>(packed.size()));
        file.WriteDword(raw);
        write(packed.data(), packed.size());
    };

    file.SeekBegin(0);
    file.Truncate();
    const LocalDateTime now = LocalTime();
    this->audioDevice.Serve();
    const std::string date = WinFormat(kSaveDateFormat, {now.year, now.month, now.day, now.hour, now.minute})
                                 .substr(0, SaveSlots::HeaderBytes);
    write(date.data(), SaveSlots::HeaderBytes);
    this->BeginMapCanvas();
    this->fieldView.Paint();
    this->EndMapCanvas();
    this->audioDevice.Serve();
    ShrinkScreenQuarter(*this->offscreen);
    const std::span<u16> thumbnail(this->drawSurface, SaveSlots::ThumbnailPixels);
    this->painter.ConvertPixelsBack(thumbnail);
    write(thumbnail.data(), thumbnail.size_bytes());
    std::array<char, SaveSlots::LocationBytes> location = {};
    const auto& name = this->state.map.name;
    std::copy_n(name.begin(), std::min(strnlen(name.data(), name.size()), location.size()), location.begin());
    write(location.data(), location.size());
    this->audioDevice.Serve();
    writeBlock(this->state.flags, GameState::FlagBytes);
    const std::vector<u8>& maps = this->databases.mapsData;
    const std::size_t rolesAt = std::min<std::size_t>(this->scene.rolesTable, maps.size());
    const std::size_t rolesBytes = std::min<std::size_t>(this->scene.rolesTableBytes, maps.size() - rolesAt);
    writeBlock(std::span<const u8>(maps).subspan(rolesAt, rolesBytes), this->scene.rolesTableBytes);
    const Role& leader = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    file.WriteDword(static_cast<u32>(this->partyLeaderId));
    file.WriteDword(static_cast<u32>(this->currentMap));
    file.WriteDword(static_cast<u32>(leader.x) >> kTileShift);
    file.WriteDword(static_cast<u32>(leader.y) >> kTileShift);
    file.WriteDword(static_cast<u32>(leader.act.id));
    file.WriteDword(static_cast<u32>(leader.act.action));
    file.WriteDword(static_cast<u32>(leader.act.direction));
    const std::vector<u8> party = this->PackParty();
    this->audioDevice.Serve();
    writeBlock(party, static_cast<u32>(party.size()));
    write(this->party.names.data(), sizeof(this->party.names));
    this->playSeconds += static_cast<i32>(crt::Ftol(crt::Difftime(crt::Time(), this->playClockStart)));
    file.WriteDword(static_cast<u32>(this->playSeconds));
    this->playClockStart = crt::Time();
    std::vector<u8> back(kReturnBytes, 0);
    PutWord(back, kReturnColumnAt, static_cast<u16>(this->scene.returnColumn));
    PutWord(back, kReturnRowAt, static_cast<u16>(this->scene.returnRow));
    PutWord(back, kReturnMapAt, static_cast<u16>(this->scene.returnMap));
    PutWord(back, kReturnUnkAt, static_cast<u16>(this->scene.warpLeaderAct));
    PutDword(back, kReturnTimerAt, static_cast<u32>(this->scene.timerFrames));
    PutDword(back, kReturnEventAt, static_cast<u32>(this->scene.timerEvent));
    write(back.data(), back.size());
    write(this->party.battleBlocks.data(), sizeof(this->party.battleBlocks));
    std::vector<u8> fame;
    this->party.fame.Save(fame, this->errors);
    file.WriteDword(static_cast<u32>(fame.size()));
    write(fame.data(), fame.size());
    std::vector<u8> shake(kShakeBytes, 0);
    PutWord(shake, kShakeHoldAt, static_cast<u16>(this->scene.shakeHold));
    PutWord(shake, kShakeFramesAt, static_cast<u16>(this->scene.shakeFrames));
    std::memcpy(shake.data() + kShakeOffsetsAt, this->scene.shake.data(), sizeof(this->scene.shake));
    write(shake.data(), shake.size());
    std::vector<u8> names(kFileNamesBytes, 0);

    auto putName = [&names](const std::optional<std::string>& name, std::size_t at)
    {
        if (name.has_value())
        {
            std::copy_n(
                name->begin(),
                std::min(name->size(), kFileNameBytes),
                names.begin() + static_cast<std::ptrdiff_t>(at)
            );
        }
    };

    putName(this->scene.noTownWarpText, 0);
    putName(this->scene.noExitWarpText, kFileNameBytes);
    writeBlock(names, kFileNamesBytes);
}

// The party data of a save (inline in 0x004070A0): the clock, the variables, the role records (the equipment
// bonuses written as 0), the equipment, the skills after their heads, the inventory and the scene music records of
// maps.dat.
std::vector<u8> Game::PackParty() const
{
    std::vector<u8> data;

    auto word = [&data](u16 value)
    {
        data.push_back(static_cast<u8>(value));
        data.push_back(static_cast<u8>(value >> CHAR_BIT));
    };

    auto bytes = [&data](const void* from, std::size_t size)
    {
        const auto* first = static_cast<const u8*>(from);
        data.insert(data.end(), first, first + size);
    };

    bytes(&this->party.clock, sizeof(this->party.clock));
    bytes(this->party.variables.data(), sizeof(this->party.variables));

    for (RoleRecord record : this->party.roles)
    {
        record.attackBonus = 0;
        record.defenseBonus = 0;
        bytes(&record, sizeof(record));
    }

    for (const std::unique_ptr<ItemNode>& slot : this->party.equipment)
    {
        word(slot != nullptr ? slot->id : ItemNode::Empty);
    }

    for (const std::unique_ptr<ItemNode>& head : this->party.skills)
    {
        for (const ItemNode* node = head != nullptr ? head->next.get() : nullptr; node != nullptr;
             node = node->next.get())
        {
            word(node->id);
        }

        word(0);
    }

    for (const ItemNode* node = this->party.inventory.get(); node != nullptr; node = node->next.get())
    {
        if (node->count > 0)
        {
            word(node->id);
            word(static_cast<u16>(node->count));
        }

        if (node->pending > 0)
        {
            word(static_cast<u16>(node->id | kPendingBit));
            word(static_cast<u16>(node->pending));
        }
    }

    word(0);
    const std::vector<u8>& maps = this->databases.mapsData;
    const std::size_t list = MapsDword(maps, MapsTable(maps, MapsDataTable::Music) + sizeof(u32));

    for (std::size_t entry = list; entry + sizeof(u32) <= maps.size(); entry += sizeof(u32))
    {
        const std::size_t record = MapsDword(maps, entry);

        if (record == 0)
        {
            break;
        }

        for (std::size_t index = 0; index < kMusicRecordBytes; ++index)
        {
            data.push_back(record + index < maps.size() ? maps[record + index] : u8{0});
        }
    }

    word(0);
    return data;
}

// The slot chosen loaded (inline in 0x004070A0; the layout is in reverse/analysis/save-format.md): the scene music
// fade stopped, the scene effects and the music ended, the screen cleared, maps.dat read again, then the save read
// section by section while the file lasts and the map entered at the saved place. Flags 5..8, 0x13 and 0x16 keep the
// values the save has across the map's start; flag 0xA is cleared. A slot that does not open leaves the screen only.
void Game::LoadGame()
{
    Scene& view = this->scene;

    if ((view.musicCue & Scene::StoryMusic) != 0)
    {
        this->audio.SetFadeMode(AudioManager::NoFade);
        this->audio.SetFadeLength(0);
        this->audio.UpdateMusicFade();
        view.musicTunes[Scene::SceneFade] = static_cast<i32>(Scene::FadePending | AudioManager::StopAtOnce);
        view.musicTunes[Scene::StoryFade] = 0;
        view.musicTunes[Scene::StoryFirst] = 0;
        view.musicTunes[Scene::StorySecond] = 0;
        view.musicCue &= kLoadedCueMask;
    }

    view.ResetEffects();
    this->audio.StopMusic();
    this->slotPrompt = kBrowsing;
    std::fill_n(this->drawSurface, static_cast<std::size_t>(Display::Width) * Display::Height, u16{0});
    this->ShowFrame();
    File& maps = this->databases.maps;
    u32 mapsBytes = maps.Size() - kMapsHeaderBytes;
    maps.SeekBegin(kMapsHeaderBytes);
    this->audioDevice.Serve();
    this->databases.mapsData.assign(mapsBytes, 0);
    maps.Read(this->databases.mapsData.data(), mapsBytes);
    view.rolesTable = MapsDword(this->databases.mapsData, sizeof(u32));
    File file;

    if (file.Open(
            SaveGames::SlotPath(this->config.SaveDirectory(), this->saveSlots.newest),
            File::Disposition::OpenExisting,
            File::Access::Read
        ))
    {
        this->ReadSave(file);
    }

    this->saveFrames = 0;
    this->storyState = Game::StoryRunning;

    if (this->menuMode == Menu::KeyLoadMode)
    {
        this->menu.OptionsLeave();
        this->menuState = Game::NoMenu;
    }

    this->FreeSavePreviews();
}

// The save read from the open slot file and the map entered (inline in 0x004070A0).
void Game::ReadSave(File& file)
{
    this->ResetBattle();
    this->scene.noTownWarpText.reset();
    this->scene.noExitWarpText.reset();
    const u32 size = file.Size();
    file.CreateMapping();
    const u8* mapped = file.MapView(0, 0);
    const std::span<const u8> save(mapped, mapped != nullptr ? size : 0);
    std::size_t at = kFirstBlockAt;

    auto unpack = [&save](std::size_t from, std::span<u8> output)
    {
        const u32 packed = ReadDword(save, from);

        if (from + kBlockHeaderBytes + packed <= save.size())
        {
            MapCache::Unpack(save.subspan(from + kBlockHeaderBytes, packed), output);
        }

        return from + kBlockHeaderBytes + packed;
    };

    std::size_t used = kFirstBlockAt;
    at = unpack(at, this->state.flags);
    used = at;
    u32 kept = 0;

    for (std::size_t index = 0; index < kKeptFlags.size(); ++index)
    {
        if (this->state.GameFlag(kKeptFlags[index]))
        {
            kept |= 1u << index;
        }
    }

    this->state.SetGameFlag(GameState::Music);
    this->state.SetGameFlag(GameState::SoundEffects);

    if ((this->deviceFlags & 1) != 0)
    {
        this->state.ClearGameFlag(GameState::Music);
        this->state.ClearGameFlag(GameState::SoundEffects);
    }

    std::vector<u8> roles(ReadDword(save, at + sizeof(u32)), 0);
    at = unpack(at, roles);

    for (std::size_t record = 0; record + kRoleRecordBytes <= roles.size(); record += kRoleRecordBytes)
    {
        auto word = [&roles, record](std::size_t index)
        {
            return static_cast<i16>(ReadWord(roles, record + sizeof(u16) * index));
        };

        this->UpdateRoleRecord(
            static_cast<u16>(word(kRecordId)),
            word(kRecordAct),
            word(kRecordAction),
            word(kRecordDirection),
            word(kRecordX),
            word(kRecordY),
            word(kRecordEvent),
            word(kRecordTalk),
            word(kRecordSetBits),
            0,
            word(kRecordMap)
        );
    }

    Role& leader = this->state.roles[static_cast<std::size_t>(this->state.playerRole)];
    this->partyLeaderId = static_cast<i32>(ReadDword(save, at));
    this->currentMap = static_cast<i32>(ReadDword(save, at + kPlaceMapAt));
    leader.x = static_cast<i32>(ReadDword(save, at + kPlaceXAt));
    leader.y = static_cast<i32>(ReadDword(save, at + kPlaceYAt));
    leader.act.id = static_cast<i32>(ReadDword(save, at + kPlaceActAt));
    leader.act.action = static_cast<i32>(ReadDword(save, at + kPlaceActionAt));
    leader.act.direction = static_cast<i32>(ReadDword(save, at + kPlaceDirectionAt));
    used = at + kPlaceBytes;
    at = used;

    if (used < size)
    {
        std::vector<u8> party(kPartyBufferScale * static_cast<std::size_t>(ReadDword(save, at + sizeof(u32))), 0);
        at = unpack(at, party);
        used = at;
        this->UnpackParty(party);

        if (used < size)
        {
            std::copy_n(
                save.begin() + static_cast<std::ptrdiff_t>(at),
                sizeof(this->party.names),
                reinterpret_cast<u8*>(this->party.names.data())
            );
            this->playSeconds = static_cast<i32>(ReadDword(save, at + sizeof(this->party.names)));
            this->playClockStart = crt::Time();
            const std::size_t back = at + sizeof(this->party.names) + sizeof(u32);
            this->scene.returnColumn = static_cast<i16>(ReadWord(save, back + kReturnColumnAt));
            this->scene.returnRow = static_cast<i16>(ReadWord(save, back + kReturnRowAt));
            this->scene.returnMap = static_cast<i16>(ReadWord(save, back + kReturnMapAt));
            this->scene.warpLeaderAct = static_cast<i16>(ReadWord(save, back + kReturnUnkAt));
            this->scene.timerFrames = static_cast<i32>(ReadDword(save, back + kReturnTimerAt));
            this->scene.timerEvent = static_cast<i32>(ReadDword(save, back + kReturnEventAt));

            if (this->scene.returnMap == 0)
            {
                this->scene.warpLeaderAct = 0;
                this->scene.returnMap = this->currentMap;
                this->scene.returnColumn = static_cast<i32>(static_cast<u32>(leader.x) >> kTileShift);
                this->scene.returnRow = static_cast<i32>(static_cast<u32>(leader.y) >> kTileShift);
            }

            at = back + kReturnBytes;
            used += kNamesTimeBytes;

            if (used < size)
            {
                std::memcpy(this->party.battleBlocks.data(), save.data() + at, sizeof(this->party.battleBlocks));
                at += sizeof(this->party.battleBlocks);
                used += sizeof(this->party.battleBlocks);

                if (used < size)
                {
                    const u32 length = ReadDword(save, at);
                    this->party.fame.Free();
                    this->party.fame.Load(
                        save.subspan(at + sizeof(u32), std::min<std::size_t>(length, save.size() - at - sizeof(u32))),
                        this->errors
                    );
                    at += sizeof(u32) + length;
                    used += length + sizeof(u32);
                }
            }
        }
    }

    this->scene.shake.fill(Scene::ShakeEnd);

    if (used < size)
    {
        this->scene.shakeHold = static_cast<i16>(ReadWord(save, at + kShakeHoldAt));
        this->scene.shakeFrames = static_cast<i16>(ReadWord(save, at + kShakeFramesAt));
        std::memcpy(this->scene.shake.data(), save.data() + at + kShakeOffsetsAt, sizeof(this->scene.shake));

        if (used + kShakeBytes < size)
        {
            std::vector<u8> names(kFileNamesBuffer, 0);
            unpack(at + kShakeBytes, names);

            auto name = [&names](std::size_t from) -> std::optional<std::string>
            {
                if (names[from] == 0)
                {
                    return std::nullopt;
                }

                const auto* text = reinterpret_cast<const char*>(names.data() + from);
                return std::string(text, strnlen(text, kFileNameBytes));
            };

            this->scene.noTownWarpText = name(0);
            this->scene.noExitWarpText = name(kFileNameBytes);
        }
    }

    file.UnmapView(mapped);
    file.CloseMapping();
    file.Close();
    this->state.leader.FillUnused();
    this->LoadFameFile();
    this->LoadCity(
        this->currentMap,
        leader.x,
        leader.y,
        static_cast<i16>(leader.act.id),
        static_cast<i16>(leader.act.action),
        static_cast<i16>(leader.act.direction),
        0
    );

    for (const int flag : kKeptFlags)
    {
        this->state.ClearGameFlag(flag);
    }

    for (std::size_t index = 0; index < kKeptFlags.size(); ++index)
    {
        if ((kept & (1u << index)) != 0)
        {
            this->state.SetGameFlag(kKeptFlags[index]);
        }
    }

    this->state.ClearGameFlag(kClearedOnLoadFlag);
}

// The party data of a save read back (inline in 0x004070A0; see PackParty): an equipment or skill object mon.dat lacks
// leaves the slot empty or is dropped; the skills go in after their heads one by one, so their order turns around.
// A scene music record goes back unless its cue bit 0x800 is set.
void Game::UnpackParty(std::span<const u8> data)
{
    std::size_t at = 0;
    this->party.clock = static_cast<i32>(ReadDword(data, at));
    at += sizeof(this->party.clock);
    std::memcpy(this->party.variables.data(), data.data() + at, sizeof(this->party.variables));
    at += sizeof(this->party.variables);
    std::memcpy(this->party.roles.data(), data.data() + at, sizeof(this->party.roles));
    at += sizeof(this->party.roles);

    for (RoleRecord& record : this->party.roles)
    {
        record.attackBonus = 0;
        record.defenseBonus = 0;
    }

    for (std::size_t index = 0; index < this->party.equipment.size(); ++index)
    {
        std::unique_ptr<ItemNode>& slot = this->party.equipment[index];
        const u16 id = ReadWord(data, at);
        at += sizeof(u16);

        if (id == ItemNode::Empty)
        {
            slot = NewEmptyItem();
        }
        else
        {
            slot = std::make_unique<ItemNode>();

            if (this->database.LoadObject(slot->record, id))
            {
                slot->id = id;
            }
            else
            {
                DebugLog(u8"載入道具 位置(%d)裝備了沒有定義的物品號碼(%d)", static_cast<i32>(index), id);
                slot->record.description.reset();
                slot->id = ItemNode::Empty;
            }
        }

        slot->count = 1;
    }

    for (std::size_t list = 0; list < this->party.skills.size(); ++list)
    {
        std::unique_ptr<ItemNode>& head = this->party.skills[list];

        if (head != nullptr)
        {
            head->next.reset();
        }

        for (u16 id = ReadWord(data, at); id != 0; id = ReadWord(data, at))
        {
            at += sizeof(u16);

            if (id == ItemNode::Empty || head == nullptr)
            {
                continue;
            }

            auto node = std::make_unique<ItemNode>();

            if (this->database.LoadObject(node->record, id))
            {
                node->count = 1;
                node->id = id;
                node->next = std::move(head->next);
                head->next = std::move(node);
            }
            else
            {
                DebugLog(u8"載入法術 角色(%d)擁有未定義的法術(%d)", static_cast<i32>(list), id);
            }
        }

        at += sizeof(u16);
    }

    FreeItemList(this->party.inventory);

    for (u16 id = ReadWord(data, at); id != 0; id = ReadWord(data, at))
    {
        const auto amount = static_cast<i16>(ReadWord(data, at + sizeof(u16)));

        if ((id & kPendingBit) != 0)
        {
            AddItem(this->party.inventory, id & ~kPendingBit, amount, ItemCountMode::PendingOnly, this->database);
        }
        else
        {
            AddItem(this->party.inventory, id, amount, ItemCountMode::Count, this->database);
        }

        at += 2 * sizeof(u16);
    }

    at += sizeof(u16);
    std::vector<u8>& maps = this->databases.mapsData;
    const std::size_t list = MapsDword(maps, MapsTable(maps, MapsDataTable::Music) + sizeof(u32));

    for (u16 map = ReadWord(data, at); map != 0; map = ReadWord(data, at))
    {
        for (std::size_t entry = list; entry + sizeof(u32) <= maps.size(); entry += sizeof(u32))
        {
            const std::size_t record = MapsDword(maps, entry);
            const u16 recordMap = MapsWord(maps, record);

            if (recordMap == 0)
            {
                break;
            }

            if (recordMap == map)
            {
                if ((MapsWord(maps, record + kMusicCueAt) & kMusicKept) == 0 &&
                    record + kMusicRecordBytes <= maps.size())
                {
                    std::copy_n(
                        data.begin() + static_cast<std::ptrdiff_t>(at),
                        kMusicRecordBytes,
                        maps.begin() + static_cast<std::ptrdiff_t>(record)
                    );
                }

                break;
            }
        }

        at += kMusicRecordBytes;
    }
}

// swd32.exe 0x00408E70: the slots page: the background and the title (saving or loading), the three slots of the page
// (the chosen one darker with frames: date, thumbnail, place, the roles' faces and levels, the gold and the play time;
// "沒有存檔資訊" for a free slot; the number), the overwrite prompt when saving, the arrows of the pages beyond (lit
// under the pointer, pressed a few frames after paging) and the pointer
void Game::DrawSlotsPage()
{
    if (++this->unk4CC2AA > kUnk4CC2AALimit)
    {
        this->unk4CC2AA = 0;
    }

    const auto color = static_cast<Pixel>(this->painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    this->menu.background.id = kSlotsAct;
    this->menu.background.action = 0;
    this->actDrawer.Draw(this->menu.background, 0, 0);
    this->slotsTitle.id = kSlotsAct;
    this->slotsTitle.action = this->slotsLoad != 0 ? kLoadTitleAction : kSaveTitleAction;
    this->slotsTitle.direction = 0;
    this->actDrawer.Draw(this->slotsTitle, 0, kTitleY);
    const int first = SaveSlots::Visible * (this->saveSlots.newest / SaveSlots::Visible);
    ActState& panel = this->battleMessages.panelAct;
    const int pitch = this->painter.ScreenPitch();

    for (int slot = first; slot < first + SaveSlots::Visible; ++slot)
    {
        const int index = slot - first;
        const int y = kSlotStep * index;
        FillBox(
            this->painter,
            kInfoX,
            y + kInfoTop,
            kInfoWidth,
            kInfoHeight,
            0,
            0,
            0,
            slot == this->saveSlots.newest ? FillMode::Eighth : FillMode::Quarter
        );
        this->acts.Run(panel);
        const SavePreview& preview = this->saveSlots.previews[static_cast<std::size_t>(index)];

        if (preview.state == SavePreview::Loaded)
        {
            this->mediumFont.DrawText(this->drawSurface, kDateX, y + kThumbnailTop, preview.date, color, kPanelStyle);

            for (int row = 0; row < kThumbnailHeight; ++row)
            {
                const std::size_t from = static_cast<std::size_t>(row) * kThumbnailWidth;

                if (from + kThumbnailWidth <= preview.thumbnail.size())
                {
                    std::copy_n(
                        preview.thumbnail.begin() + static_cast<std::ptrdiff_t>(from),
                        kThumbnailWidth,
                        this->drawSurface + static_cast<std::ptrdiff_t>(y + kThumbnailTop + row) * pitch + kThumbnailX
                    );
                }
            }

            if (slot != this->saveSlots.newest)
            {
                FillBox(
                    this->painter,
                    kThumbnailX,
                    y + kThumbnailTop,
                    kThumbnailWidth,
                    kThumbnailHeight,
                    0,
                    0,
                    0,
                    FillMode::Gray
                );
            }

            const auto length = static_cast<int>(preview.location.size());
            FillBox(
                this->painter,
                kThumbnailX,
                y + kPlaceTop,
                kThumbnailWidth,
                kPlaceHeight,
                kPlaceShade,
                kPlaceShade,
                kPlaceShade,
                FillMode::Tint
            );
            this->smallFont.DrawText(
                this->drawSurface,
                kPlaceCharWidth * (kPlaceChars - length),
                y + kPlaceTop + 1,
                preview.location,
                static_cast<Pixel>(this->textColors.primary[kPlaceColor]),
                kPlainStyle
            );

            for (int role = 0; role < Party::Roles; ++role)
            {
                const SavePreviewRole& entry = preview.roles[static_cast<std::size_t>(role)];

                if (entry.maxHitPoints == kAbsentRole)
                {
                    continue;
                }

                this->actDrawer.Draw(
                    this->saveSlots.acts[static_cast<std::size_t>(index)][static_cast<std::size_t>(role)],
                    kFaceStep * role + kFaceX,
                    (role == 1 ? kSecondFaceDrop : 0) + y + kFaceTop
                );
                const std::string level = WinFormat(kLevelFormat, {static_cast<i32>(entry.level)});
                this->mediumFont.DrawText(
                    this->drawSurface,
                    kDigitWidth * (kLevelStep * role - static_cast<int>(level.size())) + kLevelX,
                    y + kPlaceTop,
                    level,
                    color,
                    kPanelStyle
                );
            }

            this->actDrawer.DrawCounter(kGoldRight, y + kGoldTop, static_cast<u32>(preview.gold), this->counterAlpha);
            this->mediumFont.DrawText(
                this->drawSurface,
                kPlayTimeRight - kPlayTimeCharWidth * static_cast<int>(preview.playTime.size()),
                y + kPlaceTop,
                preview.playTime,
                color,
                kPanelStyle
            );
        }
        else
        {
            this->mediumFont.DrawText(this->drawSurface, kNoSaveX, y + kPlaceTop, kNoSave, color, kPanelStyle);
        }

        if (slot == this->saveSlots.newest)
        {
            DrawWindowFrame(
                this->painter,
                this->archive,
                panel.frame,
                kFrameLeftA,
                y + kFrameTop,
                kFrameRightA,
                y + kFrameBottom,
                0,
                kFrameBorder
            );
            DrawWindowFrame(
                this->painter,
                this->archive,
                panel.frame,
                kFrameLeftB,
                y + kFrameTop,
                kFrameRightB,
                y + kFrameBottom,
                0,
                kFrameBorder
            );
        }

        const std::string number = WinFormat(kSlotFormat, {slot});
        this->smallFont.DrawText(this->drawSurface, kThumbnailX, y + kThumbnailTop, number, color, kPanelStyle);
    }

    if (this->slotsLoad == 0 && (this->slotPrompt == kOverwriteYes || this->slotPrompt == kOverwriteNo))
    {
        int y = kSlotStep * (this->saveSlots.newest % SaveSlots::Visible);
        DrawWindowFrame(
            this->painter,
            this->archive,
            panel.frame,
            kPromptLeft,
            y + kPromptTop,
            kPromptRight,
            y + kPromptBottom,
            0,
            kFrameBorder
        );
        this->largeFont.DrawText(this->drawSurface, kPromptLeft, y + kPromptTop, kOverwrite, color, kPromptStyle);
        y += kAnswersTop;
        FillBox(
            this->painter,
            static_cast<int>(kAnswerWidth) * this->slotPrompt + kAnswerBase,
            y,
            static_cast<int>(kAnswerWidth),
            kAnswerHeight,
            kAnswerRed,
            kAnswerGreen,
            0,
            FillMode::Gradient
        );
        this->largeFont.DrawText(this->drawSurface, kPromptLeft, y, kAnswers, color, kPromptStyle);
    }

    if (this->saveSlots.newest + kArrowReach <= SaveGames::LastSlot)
    {
        this->actDrawer.DrawPicture(kDownArrow, 0, kArrowX, kDownArrowY);

        if (this->state.pointerY < static_cast<i32>(kDownArrowBottom) &&
            this->state.pointerY > static_cast<i32>(kDownArrowTop))
        {
            this->actDrawer.DrawPicture(kDownArrowLit, 0, kArrowX, this->downArrowPress + kDownArrowY);
        }
    }

    if (this->saveSlots.newest - kArrowReach >= 0)
    {
        this->actDrawer.DrawPicture(kUpArrow, 0, kArrowX, kUpArrowY);

        if (static_cast<u32>(this->state.pointerY) <= kUpArrowBottom)
        {
            this->actDrawer.DrawPicture(kUpArrowLit, 0, kArrowX, kUpArrowY - this->upArrowPress);
        }
    }

    if (this->downArrowPress != 0)
    {
        --this->downArrowPress;
    }

    if (this->upArrowPress != 0)
    {
        --this->upArrowPress;
    }

    ActState& pointer = this->scene.pointerRole.act;

    if (this->acts.Run(pointer) == 0)
    {
        DebugLog(
            u8"Act Err(RefrashCursor) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
            this->scene.pointerRole.id,
            pointer.id,
            pointer.action,
            pointer.direction
        );
    }

    this->actDrawer.DrawPicture(
        pointer.frame,
        pointer.picture,
        this->state.pointerX - pointer.x,
        this->state.pointerY - pointer.y
    );
}

}  // namespace swd3
