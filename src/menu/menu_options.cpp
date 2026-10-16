#include "menu/menu_options.h"

#include <cstring>
#include <new>

#include "audio/audio.h"
#include "crt/crt.h"
#include "res/act_anim.h"
#include "text/text_input.h"
#include "data/globals.h"
#include "data/image.h"
#include "swd3/w32.h"
#include "game/game_state.h"

namespace swd3 {

// swd32.exe 0x00448BB0: on page 5 the row stops at 5 by falling back to 4. The return value is unused by callers.
void OptionCursorDown() {
    if (G.optionPage == 1) {
        if (++G.optionPageCursor > 3) {
            G.optionPageCursor = 3;
        }
    } else if (G.optionPage == 5) {
        if (++G.optionRow > 5) {
            G.optionRow = 4;
        }
    }
}

// swd32.exe 0x00448C00: the return value is unused by callers
void OptionCursorUp() {
    if (G.optionPage == 1) {
        if (--G.optionPageCursor < 0) {
            G.optionPageCursor = 0;
        }
    } else if (G.optionPage == 5) {
        if (--G.optionRow < 0) {
            G.optionRow = 0;
        }
    }
}

// swd32.exe 0x00448C40: the return value is unused by callers
void OptionCursorLast() {
    if (G.optionPage == 1) {
        G.optionPageCursor = 3;
    } else if (G.optionPage == 5) {
        G.optionRow = 5;
    }
}

// swd32.exe 0x00448C70: the return value is unused by callers
void OptionCursorFirst() {
    if (G.optionPage == 1) {
        G.optionPageCursor = 0;
    } else if (G.optionPage == 5) {
        G.optionRow = 0;
    }
}

// swd32.exe 0x00448CA0: left on the settings: sound volume (sample 0x2E at the new volume), music volume, text
// speed down to 60, game flag 0x48 set, level G.dword_4A9ECC up to 4 (copied to G.dword_4FD2A4), level
// G.dword_4ACE60 down to 0. The return value is unused by callers.
void OptionDecrease() {
    if (G.optionPage == 1) {
        if (--G.optionPageCursor < 0) {
            G.optionPageCursor = 0;
        }
        return;
    }
    if (G.optionPage != 5) {
        return;
    }
    switch (G.optionRow) {
        case 0: {
            int volume = --G.soundVolume;
            if (G.soundVolume <= 0) {
                volume = 0;
                G.soundVolume = 0;
            }
            PlaySound(0x2E, volume);
            break;
        }
        case 1: {
            int volume = --G.musicVolume;
            if (G.musicVolume <= 0) {
                volume = 0;
                G.musicVolume = 0;
            }
            SetMusicVolume(volume);
            break;
        }
        case 2:
            G.dword_4B7ACC -= 0x28;
            if (G.dword_4B7ACC <= 0x3C) {
                G.dword_4B7ACC = 0x3C;
            }
            break;
        case 3:
            SetGameFlag(0x48);
            break;
        case 4: {
            int level = ++G.dword_4A9ECC;
            if (G.dword_4A9ECC > 4) {
                level = 4;
                G.dword_4A9ECC = 4;
            }
            G.dword_4FD2A4 = level;
            break;
        }
        case 5:
            if (--G.dword_4ACE60 < 0) {
                G.dword_4ACE60 = 0;
            }
            break;
        default:
            break;
    }
}

// swd32.exe 0x00448DA0: right on the settings: sound volume up to 11, music volume above 11 falls back to 10, text
// speed up to 140, game flag 0x48 cleared, level G.dword_4A9ECC down to 0, level G.dword_4ACE60 up to 11. The return
// value is unused by callers.
void OptionIncrease() {
    if (G.optionPage == 1) {
        if (++G.optionPageCursor > 3) {
            G.optionPageCursor = 3;
        }
        return;
    }
    if (G.optionPage != 5) {
        return;
    }
    switch (G.optionRow) {
        case 0: {
            int volume = ++G.soundVolume;
            if (G.soundVolume > 0xB) {
                volume = 0xB;
                G.soundVolume = 0xB;
            }
            PlaySound(0x2E, volume);
            break;
        }
        case 1: {
            int volume = ++G.musicVolume;
            if (G.musicVolume > 0xB) {
                volume = 0xA;
                G.musicVolume = 0xA;
            }
            SetMusicVolume(volume);
            break;
        }
        case 2:
            G.dword_4B7ACC += 0x28;
            if (G.dword_4B7ACC >= 0x8C) {
                G.dword_4B7ACC = 0x8C;
            }
            break;
        case 3:
            ClearGameFlag(0x48);
            break;
        case 4: {
            int level = --G.dword_4A9ECC;
            if (G.dword_4A9ECC <= 0) {
                level = 0;
                G.dword_4A9ECC = 0;
            }
            G.dword_4FD2A4 = level;
            break;
        }
        case 5:
            if (++G.dword_4ACE60 > 0xB) {
                G.dword_4ACE60 = 0xB;
            }
            break;
        default:
            break;
    }
}

// swd32.exe 0x00448EB0: the return value is unused by callers
void OptionPageCursorDown() {
    if (G.optionPage == 1) {
        if (++G.optionPageCursor > 3) {
            G.optionPageCursor = 3;
        }
    }
}

// swd32.exe 0x00448EE0: Enter on the page list: 1 opens the name entry (game flag 0x50, a 0x20 byte buffer and a
// TextInput of 8 bytes at (300, 230)), 2 the settings page (panel act 0x232A / 0x22), 3 fades the music out
void OptionSelect() {
    if (G.optionPage != 1) {
        return;
    }
    G.optionRow = 0x64;
    G.optionPage = 2;
    switch (G.optionPageCursor) {
        case 1: {
            G.optionRow = 0x61;
            SetGameFlag(0x50);
            G.dword_4FC208 = 1;
            G.dword_4FC210 = 0x46;
            G.dword_4FC23C = 0;
            G.dword_4FD2A0 = 0;
            auto* buffer = static_cast<std::uint8_t*>(crt::malloc(0x20));
            G.dword_4FD2D0 = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(buffer));
            std::memset(buffer, 0, 0x20);
            ++G.optionRow;
            void* storage = crt::operator_new(0x20);
            if (storage == nullptr) {
                G.dword_4CACD8 = 0;
                return;
            }
            auto* input = new (storage) TextInput(reinterpret_cast<const char*>(&G.sz), 8, 0x12C, 0xE6);
            G.dword_4CACD8 = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(input));
            break;
        }
        case 2:
            ActRun(&G.panelAct);
            G.optionPage = 5;
            G.optionRow = 0;
            G.dword_4FC650 = 0x232A;
            G.dword_4FC658 = 0x22;
            break;
        case 3:
            SetMusicFade(0x10, 0x19);
            PollMusicFade();
            break;
        default:
            break;
    }
}

namespace {

bool nameIs(std::uint32_t name, std::uint32_t code) {
    return w32::lstrcmpA(&image::at<char>(name), &image::at<char>(code)) == 0;
}

void setWord(std::uint32_t address, int value) { image::at<std::int16_t>(address) = static_cast<std::int16_t>(value); }

void fillBytes(std::uint32_t address, std::uint8_t value) {
    for (std::uint32_t i = 0; i < 9; ++i) {
        image::at<std::uint8_t>(address + i) = value;
    }
}

}  // namespace

// swd32.exe 0x00449D80: name cheats after the name entry; each code compares the first (0x0049E148) or second
// (0x0049E158) hero name with a string at 0x004A70F4..0x004A715C and overwrites stat words of the hero data. The
// return value is unused by callers.
void ApplyNameCheats() {
    constexpr std::uint32_t first = 0x0049E148u;
    constexpr std::uint32_t second = 0x0049E158u;
    if (nameIs(first, 0x004A715Cu)) {
        setWord(0x004AB7A0u, 0x19);
        setWord(0x004AB7A2u, 0x19);
        setWord(0x004AB7A4u, 0x19);
        setWord(0x004AB7A6u, 0x19);
    }
    if (nameIs(second, 0x004A7154u)) {
        setWord(0x004AB7DCu, 0x64);
        setWord(0x004AB7CEu, 0x12C);
        setWord(0x004AB7D4u, 0x12C);
    }
    if (nameIs(first, 0x004A714Cu)) {
        setWord(0x004AB7A0u, 0x32);
        setWord(0x004AB794u, 0xC8);
        setWord(0x004AB79Au, 0xC8);
    }
    if (nameIs(first, 0x004A7144u)) {
        fillBytes(0x004AB7BDu, 0xFB);
    }
    if (nameIs(first, 0x004A713Cu)) {
        setWord(0x004AB7A0u, 0x1E);
        setWord(0x004AB7A2u, 0x1E);
        setWord(0x004AB7A4u, 0x1E);
        setWord(0x004AB7A6u, 0x1E);
    }
    if (nameIs(second, 0x004A7134u)) {
        fillBytes(0x004AB7F5u, 0xF9);
    }
    if (nameIs(first, 0x004A712Cu)) {
        setWord(0x004AB7A2u, 0x64);
        setWord(0x004AB798u, 0x12C);
        setWord(0x004AB79Eu, 0x12C);
    }
    if (nameIs(first, 0x004A7124u)) {
        setWord(0x004AB7A4u, 0x46);
        setWord(0x004AB796u, 0xFA);
        setWord(0x004AB79Cu, 0xFA);
    }
    if (nameIs(first, 0x004A711Cu)) {
        setWord(0x004AB7A4u, 0x64);
        fillBytes(0x004AB7BDu, 2);
    }
    if (nameIs(second, 0x004A7114u)) {
        setWord(0x004AB7DAu, 0x64);
        setWord(0x004AB7D0u, 0x12C);
        setWord(0x004AB7D6u, 0x12C);
    }
    if (nameIs(second, 0x004A710Cu)) {
        setWord(0x004AB7D8u, 0x32);
        setWord(0x004AB7CCu, 0xC8);
        setWord(0x004AB7D2u, 0xC8);
    }
    if (nameIs(first, 0x004A7104u)) {
        setWord(0x004AB7A2u, image::at<std::int16_t>(0x004AB7A2u) - 5);
        setWord(0x004AB7A4u, image::at<std::int16_t>(0x004AB7A4u) - 5);
        setWord(0x004AB7A6u, image::at<std::int16_t>(0x004AB7A6u) - 5);
        image::at<std::int32_t>(0x004ACBD0u) = 0x1F4;
        setWord(0x004AB7A0u, image::at<std::int16_t>(0x004AB7A0u) - 10);
    }
    if (nameIs(first, 0x004A70FCu)) {
        setWord(0x004AB7A2u, 0);
        setWord(0x004AB798u, 0);
        setWord(0x004AB79Eu, 0);
    }
    if (nameIs(second, 0x004A70F4u)) {
        setWord(0x004AB7DCu, 5);
        setWord(0x004AB7CEu, 5);
        setWord(0x004AB7D4u, 5);
    }
}

}  // namespace swd3
