#include <algorithm>
#include <cstring>
#include <span>
#include <string>

#include "core/format.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_text.h"
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
constexpr i32 kInGame = 100;

constexpr i32 kFadeOut = 0x64;
constexpr i32 kFirstName = 0x62;
constexpr i32 kNameEntry = 0x61;
constexpr i32 kInGameFade = -0x78;
constexpr i32 kTitleFade = -6;
constexpr i32 kHiddenGlow = -0x10;
constexpr i32 kEffectsOffFlag = 0x48;
constexpr i32 kNameEntryFlag = 0x50;
constexpr int kVolumeSound = 0x2E;
constexpr int kTitleMusic = 0xA;
constexpr int kNameBoxX = 300;
constexpr int kNameBoxY = 0xE6;
constexpr int kNameLength = 8;
constexpr u32 kScreenPixels = 0x4B000;
constexpr std::size_t kScreenBytes = 0x96000;
constexpr int kTextStyle = 4;

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

    if (this->mode == 1 || this->mode == 2)
    {
        this->systemState = kInGame;
        this->systemRow = kInGameFade;
        this->savedScreen.assign(owner.drawSurface, owner.drawSurface + kScreenPixels);
    }

    if (this->mode == 3)
    {
        this->systemState = kEntries;
        this->systemCursor = 1;
        this->SystemSelect();
    }

    owner.unk4A9484 = 0;

    if (this->mode == 0 && owner.audio.IsMusicStopped())
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

    if (this->systemState == kChosen)
    {
        if (this->systemRow >= kNameEntry)
        {
            if (leftButton.heldFrames == 1 && leftButton.press == 1 && y < 0x112 && y > 0x101 && x < 0x198 && x > 0x162)
            {
                this->systemClick = 1;
            }
            else if (rightButton.heldFrames == 1 && rightButton.press == 1)
            {
                this->systemClick = 2;
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
            if (y < kRows[entry].second && y > kRows[entry].first && x < 0x104 && x > 0x6E)
            {
                this->systemCursor = static_cast<i32>(entry);

                if ((this->clicks & 3) != 0)
                {
                    this->SystemSelect();
                }
            }
        }

        return;
    }

    if (this->systemState != kSettings)
    {
        return;
    }

    if (leftButton.press == 0 || x >= 0x1C0 || x <= 0xA8)
    {
        if (rightButton.press != 0)
        {
            this->SystemCancel();
        }

        return;
    }

    u32 step = (x - 0x100) >> 4;
    const bool onBar = (step & 0xFFFF0000u) == 0;
    Config& settings = owner.config;

    if (y < 0x10F && y > 0xF0)
    {
        this->systemRow = 0;

        if (onBar)
        {
            settings.soundVolume = static_cast<i32>(std::min<u32>(step, Config::MaxVolume));
            owner.audio.PlaySound(kVolumeSound, settings.soundVolume);
        }

        return;
    }

    if (y < 0x12F && y > 0x110)
    {
        this->systemRow = 1;

        if (onBar)
        {
            settings.musicVolume = static_cast<i32>(std::min<u32>(step, Config::MaxVolume));
            owner.audio.SetMusicVolume(settings.musicVolume);
        }

        return;
    }

    if (y < 0x14F && y > 0x130)
    {
        this->systemRow = 2;

        if (onBar)
        {
            settings.cacheSize = static_cast<i32>(0x28 * std::min<u32>(step, 2) + 0x3C);
        }

        return;
    }

    if (y < 0x16F && y > 0x150)
    {
        this->systemRow = 3;

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

    if (y < 0x18F && y > 0x170)
    {
        this->systemRow = 4;

        if (onBar)
        {
            step = std::min<u32>(step, 4);
            settings.textSpeed = static_cast<i32>(4 - step);
            this->textSpeedShown = static_cast<i32>(4 - step);
        }

        return;
    }

    if (y < 0x1AF && y > 0x190)
    {
        this->systemRow = 5;

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

// swd32.exe 0x00448BB0: on the settings a step past the last row falls back to row 4
void Menu::SystemDown()
{
    if (this->systemState == kEntries)
    {
        if (++this->systemCursor > 3)
        {
            this->systemCursor = 3;
        }
    }
    else if (this->systemState == kSettings)
    {
        if (++this->systemRow > 5)
        {
            this->systemRow = 4;
        }
    }
}

// swd32.exe 0x00448C00
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
            this->systemRow = 0;
        }
    }
}

// swd32.exe 0x00448C40
void Menu::SystemLast()
{
    if (this->systemState == kEntries)
    {
        this->systemCursor = 3;
    }
    else if (this->systemState == kSettings)
    {
        this->systemRow = 5;
    }
}

// swd32.exe 0x00448C70
void Menu::SystemFirst()
{
    if (this->systemState == kEntries)
    {
        this->systemCursor = 0;
    }
    else if (this->systemState == kSettings)
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

    if (this->systemState != kSettings)
    {
        return;
    }

    switch (this->systemRow)
    {
        case 0:
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

        case 1:
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

        case 2:
            settings.cacheSize -= 0x28;

            if (settings.cacheSize <= 0x3C)
            {
                settings.cacheSize = 0x3C;
            }

            break;

        case 3:
            owner.state.SetGameFlag(kEffectsOffFlag);
            break;

        case 4:
        {
            int level = ++settings.textSpeed;

            if (settings.textSpeed > 4)
            {
                level = 4;
                settings.textSpeed = 4;
            }

            this->textSpeedShown = level;
            break;
        }

        case 5:
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
        if (++this->systemCursor > 3)
        {
            this->systemCursor = 3;
        }

        return;
    }

    if (this->systemState != kSettings)
    {
        return;
    }

    switch (this->systemRow)
    {
        case 0:
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

        case 1:
        {
            int volume = ++settings.musicVolume;

            if (settings.musicVolume > Config::MaxVolume)
            {
                volume = 0xA;
                settings.musicVolume = 0xA;
            }

            owner.audio.SetMusicVolume(volume);
            break;
        }

        case 2:
            settings.cacheSize += 0x28;

            if (settings.cacheSize >= 0x8C)
            {
                settings.cacheSize = 0x8C;
            }

            break;

        case 3:
            owner.state.ClearGameFlag(kEffectsOffFlag);
            break;

        case 4:
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

        case 5:
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
        if (++this->systemCursor > 3)
        {
            this->systemCursor = 3;
        }
    }
}

// swd32.exe 0x00448EE0: the chosen entry fades out; entry 1 first asks for the two names (flag 0x50 routes the
// keyboard to a name box), entry 2 opens the settings, entry 3 also fades the music out
void Menu::SystemSelect()
{
    Game& owner = this->game;

    if (this->systemState != kEntries)
    {
        return;
    }

    this->systemRow = kFadeOut;
    this->systemState = kChosen;

    if (this->systemCursor == 1)
    {
        this->systemRow = kNameEntry;
        owner.state.SetGameFlag(kNameEntryFlag);
        this->face.id = 1;
        this->face.action = 0x46;
        this->face.direction = 0;
        this->nameReply = 0;
        this->enteredNames.assign(0x20, 0);
        ++this->systemRow;
        owner.textInput = std::make_unique<TextInput>(Party::RoleNames[0], kNameLength, kNameBoxX, kNameBoxY);
    }
    else if (this->systemCursor == 2)
    {
        owner.acts.Run(owner.battleMessages.panelAct);
        this->systemState = kSettings;
        this->systemRow = 0;
        this->gauge.id = 0x232A;
        this->gauge.action = 0x22;
    }
    else if (this->systemCursor == 3)
    {
        owner.audio.SetMusicFade(0x10, 0x19);
        owner.audio.PollMusicFade();
    }
}

// swd32.exe 0x00449050: Escape on the entries points at the last entry; on the settings it goes back and writes them
// to Env.dat
void Menu::SystemCancel()
{
    Game& owner = this->game;

    if (this->systemState == kEntries)
    {
        this->systemCursor = 3;
        return;
    }

    if (this->systemState == kSettings)
    {
        this->systemState = kEntries;
        const Config::Settings settings{
            static_cast<u8>(owner.config.soundVolume),
            static_cast<u8>(owner.config.musicVolume),
            static_cast<u8>(owner.config.cacheSize),
            0x64,
            static_cast<u8>(owner.state.GameFlag(kEffectsOffFlag) ? 1 : 0),
            static_cast<u8>(owner.config.textSpeed),
            static_cast<u8>(owner.config.battleSpeed),
        };

        owner.config.Update(owner.config.installDirectory, owner.config.cdRoot, settings);
    }
}

// swd32.exe 0x004490C0: the page. The entries glow in over the background (the chosen one brighter) while the screen
// fades in from black; a chosen entry fades the screen out and acts (0: continue the newest save, 1: a new game after
// the names, 2: the settings, 3: quit). The name entry shows the box with the face's reply to a key; the settings
// show the six rows with their bars; in game the page fades over the saved screen and back.
void Menu::SystemDraw()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;

    if (this->systemState <= kChosen)
    {
        owner.actDrawer.Draw(this->background, 0, 0);
        const auto chosen = static_cast<std::size_t>(this->systemCursor);
        this->glow[chosen] -= 4;

        if (this->glow[chosen] < -0x20)
        {
            this->glow[chosen] = -0x20;
        }

        for (i32& level : this->glow)
        {
            level += 2;

            if (level > -0xC)
            {
                level = -0xC;
            }
        }

        constexpr std::array<int, 4> kEntryY = {0xD2, 0x107, 0x140, 0x179};

        for (std::size_t entry = 0; entry < this->items.size(); ++entry)
        {
            owner.actDrawer.DrawGlow(this->items[entry], 0x7D, kEntryY[entry], this->glow[entry]);
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

            const int level = 5 * this->systemRow;
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
        else if (++this->systemRow <= 0x68)
        {
            const int level = 8 * (kFadeOut - this->systemRow);
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

    if (this->systemState != kInGame)
    {
        this->systemClick = 0;
        return;
    }

    i32 row = this->systemRow;

    if (this->mode == 1 && row == kInGameFade)
    {
        painter.GrayPixels(this->savedScreen);
    }

    if (this->mode == 1)
    {
        if (row >= kInGameFade)
        {
            if (row < -0x5A)
            {
                this->tint = row + 0x5A;
            }
            else if (row < -0xA)
            {
                this->tint = 0;
            }
            else if (row <= 0)
            {
                this->tint = 3 * (-0xA - row);
            }
        }

        this->gauge.id = 0x232A;
        this->gauge.action = 0x44;
        owner.actDrawer.Draw(this->gauge, 0, 0);
    }
    else if (this->mode == 2)
    {
        std::copy(this->savedScreen.begin(), this->savedScreen.end(), owner.drawSurface);

        if (row % 3 == 0)
        {
            painter.TintPixels(this->savedScreen, -1, -1, -1);
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
                constexpr std::array<i32, 3> kReplies = {0x1C, 0x4D, 0x1E};
                this->face.action = kReplies[static_cast<std::size_t>(owner.random.Below(3))];
            }
            else
            {
                constexpr std::array<i32, 3> kReplies = {0x22, 0x39, 0xE};
                this->face.action = kReplies[static_cast<std::size_t>(owner.random.Below(3))];
            }
        }

        this->nameReply = 0x28;
    }
    else
    {
        if (this->nameReply > 0)
        {
            --this->nameReply;
        }

        if (this->nameReply == 2)
        {
            if (this->systemRow == kFirstName && owner.random.Below(2) == 0)
            {
                this->face.action = 0x3A;
            }
            else
            {
                this->face.action = 0x46;
            }
        }
    }

    owner.DrawTextInput(*owner.textInput);
    owner.actDrawer.Draw(this->face, 0xDC, 0xFE);
    int result = owner.textInput->GetResult();

    if (result == 0 && this->systemClick != 0)
    {
        result = this->systemClick;
    }

    this->systemClick = 0;

    if (result == TextInput::Enter)
    {
        owner.party.names = Party::DefaultNames();
        const std::string name = owner.textInput->CopyText(0x10);
        const auto at = static_cast<std::size_t>(0x10 * (this->systemRow - kFirstName));
        std::copy_n(
            name.begin(),
            std::min<std::size_t>(name.size(), 0x10),
            this->enteredNames.begin() + static_cast<long>(at)
        );
        owner.textInput.reset();

        if (++this->systemRow == 0x63)
        {
            owner.textInput = std::make_unique<TextInput>(Party::RoleNames[1], kNameLength, kNameBoxX, kNameBoxY);
            this->face.id = 2;
            this->face.action = 0x46;
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
        case 0:
            owner.menuState = Game::NoMenu;
            owner.storyState = Game::SaveSlotsState;
            owner.unk4CAEBC = 1;
            owner.unk4CAEC0 = 1;
            owner.menuMode = Menu::TitleLoadMode;
            owner.ResetSaveSlots();
            break;

        case 1:
            owner.unk4ACEF4 = 0;
            owner.unk4AB990 = crt::Time();
            this->systemState = kSettings;
            std::memset(owner.drawSurface, 0, kScreenBytes);
            owner.StartStory(1);
            owner.menuState = Game::NoMenu;
            owner.storyState = Game::StoryRunning;
            owner.unk4CAEBC = 1;
            owner.unk4CAEC0 = 1;
            owner.ResetSaveSlots();
            owner.party.ApplyNameCheats();
            owner.LoadFameFile();
            break;

        case 2:
            this->systemState = kEntries;
            break;

        case 3:
            owner.systemFlags |= 4;
            break;

        default:
            break;
    }
}

// The settings of the system page (part of 0x004490C0): the box, the six labels, a bar of steps for each (the current
// value lit), the cache line, the effects line, the speed with its sample text typing out, and the row highlight.
void Menu::DrawSettings()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    Config& settings = owner.config;
    auto color = static_cast<Pixel>(painter.MakeColor(0x15, 0xF, 8));
    owner.actDrawer.Draw(this->background, 0, 0);
    this->DrawBox(0x98, 0xEC, 0x190, 0xC0, 4, 0, 0, 0);
    this->gauge.id = 0x232A;
    this->gauge.action = 0x22;
    TextFont& large = owner.largeFont;
    TextFont& medium = owner.mediumFont;
    u16* pixels = painter.Target();
    large.DrawText(pixels, 0xC4, 0xF0, menu_text::kSoundVolume, color, kTextStyle);
    large.DrawText(pixels, 0xC4, 0x110, menu_text::kMusicVolume, color, kTextStyle);
    large.DrawText(pixels, 0x98, 0x130, menu_text::kCacheSize, color, kTextStyle);
    large.DrawText(pixels, 0x98, 0x150, menu_text::kMapEffects, color, kTextStyle);
    large.DrawText(pixels, 0x98, 0x170, menu_text::kTextSpeed, color, kTextStyle);
    large.DrawText(pixels, 0x98, 0x190, menu_text::kBattleSpeed, color, kTextStyle);

    auto bar = [this, &owner](int y, auto lit)
    {
        int step = 0;

        for (int x = 0x100; x <= 0x1B0; x += 0x10)
        {
            this->gauge.action = lit(step) ? 0x23 : 0x22;
            owner.actDrawer.Draw(this->gauge, x, y);
            ++step;
        }
    };

    bar(0xF0,
        [&settings](int step)
        {
            return step <= settings.soundVolume;
        });

    bar(0x110,
        [&settings](int step)
        {
            return step <= settings.musicVolume;
        });

    int size = 0x3C;

    for (int x = 0x100; x < 0x130; x += 0x10)
    {
        this->gauge.action = 0x22;

        if (size == settings.cacheSize)
        {
            this->gauge.action = 0x23;
            const std::string line = WinFormat(
                menu_text::kCacheLine,
                {static_cast<i32>(
                     static_cast<u8>(settings.installDirectory.empty() ? 0 : settings.installDirectory[0])
                 ),
                 settings.cacheSize}
            );
            color = static_cast<Pixel>(painter.MakeColor(0x15, 0xF, 8));
            medium.DrawText(pixels, 0x168, 0x136, line, color, kTextStyle);
        }

        owner.actDrawer.Draw(this->gauge, x, 0x130);
        size += 0x28;
    }

    const int effectsOff = owner.state.GameFlag(kEffectsOffFlag) ? 1 : 0;

    for (int step = 0, x = 0x100; x < 0x120; x += 0x10, ++step)
    {
        this->gauge.action = step == effectsOff ? 0x22 : 0x23;
        owner.actDrawer.Draw(this->gauge, x, 0x150);
    }

    const std::string effects =
        WinFormat(menu_text::kEffectsLine, {effectsOff != 0 ? menu_text::kOff : menu_text::kOn});
    medium.DrawText(pixels, 0x168, 0x156, effects, color, kTextStyle);

    for (int step = 0, x = 0x140; step < 5; ++step, x -= 0x10)
    {
        this->gauge.action = step == settings.textSpeed ? 0x23 : 0x22;
        owner.actDrawer.Draw(this->gauge, x, 0x170);
    }

    std::string speed = settings.textSpeed >= 0 && settings.textSpeed < 5
                            ? Big5(menu_text::kSpeeds[static_cast<std::size_t>(settings.textSpeed)])
                            : std::string();
    std::string sample = Big5(menu_text::kSample);
    medium.DrawText(pixels, 0x19E, 0x176, sample, static_cast<Pixel>(painter.MakeColor(0xF, 9, 2)), kTextStyle);

    // The original then copies the first sampleLength words of the sample over the speed name in place.
    const auto words = static_cast<std::size_t>(this->sampleLength);

    if (words != 0)
    {
        speed.resize(std::max(speed.size(), 2 + 2 * words), '\0');
        std::copy_n(sample.begin(), std::min(sample.size(), 2 * words), speed.begin() + 2);
    }

    if (--this->sampleDelay <= 0)
    {
        ++this->sampleLength;
        this->sampleDelay = 2 * this->textSpeedShown + 1;
    }

    medium.DrawText(pixels, 0x168, 0x176, speed, color, kTextStyle);

    if (2 * static_cast<std::size_t>(this->sampleLength) >= sample.size() ||
        sample[2 * static_cast<std::size_t>(this->sampleLength)] == 0)
    {
        this->sampleLength = 0;
    }

    bar(0x190,
        [&settings](int step)
        {
            return step <= settings.battleSpeed;
        });

    FillBox(painter, 0x93, 0x20 * this->systemRow + 0xEC, 0x19A, 0x20, 0x14, 0xD, 0, FillMode::Gradient);
}

}  // namespace swd3
