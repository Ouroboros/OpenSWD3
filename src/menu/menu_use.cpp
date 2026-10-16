#include "menu/menu_use.h"

#include <cstring>

#include "audio/audio.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "data/image.h"
#include "item/item_list.h"
#include "menu/menu_items.h"
#include "menu/menu_util.h"
#include "swd3/w32.h"

namespace swd3 {

namespace {

std::uint8_t** useList() { return reinterpret_cast<std::uint8_t**>(&image::at<std::uint8_t>(0x004FCFC0u)); }

std::uint8_t** useTop() { return reinterpret_cast<std::uint8_t**>(&G.dword_4FCFBC); }

bool heroInvalid(int hero) { return image::at<std::int16_t>(0x004A6A38u + 0x1Cu * static_cast<std::uint32_t>(hero)) == -1; }

void showCursorItem(int arrows) {
    LoadItemName(ListNodeAt(G.dword_4FD070 + G.dword_4FCF98, useList()));
    G.dword_4FD080 = arrows;
    PlaySound(0x2E, G.soundVolume);
}

int firstHero() {
    int hero = -1;
    do {
        ++hero;
    } while (heroInvalid(hero));
    return hero;
}

int lastHero() {
    int hero = 4;
    do {
        --hero;
    } while (heroInvalid(hero));
    return hero;
}

}  // namespace

// swd32.exe 0x00443450: down a row (scrolls by a row while more items follow)
void UseKeyDown() {
    if (G.dword_4FD078 == 1) {
        G.dword_4FD070 += 2;
        if (G.dword_4FD070 >= G.dword_4FCFA0) {
            if (G.dword_4FCFA0 + G.dword_4FCF98 < G.dword_4FCFB8) {
                G.dword_4FCF98 += 2;
            }
            G.dword_4FD070 -= 2;
        }
        ListNodeAtInto(G.dword_4FCF98, useList(), useTop());
        CountListUpTo(*useTop(), &G.dword_4FCFA0, 0x18);
        if (G.dword_4FD070 >= G.dword_4FCFA0) {
            G.dword_4FD070 -= 2;
        }
        LoadItemName(ListNodeAt(G.dword_4FD070 + G.dword_4FCF98, useList()));
        G.dword_4FD080 = 0x30;
        PlaySound(0x2E, G.soundVolume);
    } else if (G.dword_4FD078 == 2) {
        int hero = G.dword_4FCFAC;
        do {
            if (++hero >= 4) {
                hero = 0;
            }
        } while (heroInvalid(hero));
        G.dword_4FCFAC = hero;
    } else if (G.dword_4FD078 == 0xF) {
        MenuCursorDown(&G.unlockedCount, &G.dword_4FBED4, &G.dword_4FC510, &G.dword_4FB8D4);
        G.dword_4FD080 |= 0x3000;
    }
}

// swd32.exe 0x00443570: up a row (scrolls back by a row at the top)
void UseKeyUp() {
    if (G.dword_4FD078 == 1) {
        G.dword_4FD070 -= 2;
        if (G.dword_4FD070 < 0) {
            G.dword_4FD070 += 2;
            if (G.dword_4FCF98 > 0) {
                G.dword_4FCF98 -= 2;
            }
        }
        ListNodeAtInto(G.dword_4FCF98, useList(), useTop());
        CountListUpTo(*useTop(), &G.dword_4FCFA0, 0x18);
        showCursorItem(3);
    } else if (G.dword_4FD078 == 2) {
        int hero = G.dword_4FCFAC;
        do {
            if (--hero < 0) {
                hero = 3;
            }
        } while (heroInvalid(hero));
        G.dword_4FCFAC = hero;
    } else if (G.dword_4FD078 == 0xF) {
        MenuCursorUp(static_cast<int>(reinterpret_cast<std::uintptr_t>(&G.unlockedCount)), &G.dword_4FBED4,
                     &G.dword_4FC510);
        G.dword_4FD080 |= 0x300;
    }
}

// swd32.exe 0x00443670: to the last row of the screen in the same column, else a screen further (the cursor stays
// on the last row when no screen follows; no sound when only the cursor moved)
void UseKeyPageDown() {
    if (G.dword_4FD078 == 1) {
        const int lastRow = G.dword_4FCFA0 - ((G.dword_4FCFA0 & 1) == 0 ? 1 : 0);
        int cursor = lastRow - 1;
        if (G.dword_4FD070 < cursor) {
            G.dword_4FD070 = (G.dword_4FD070 & 1) + lastRow - 1;
            if (G.dword_4FD070 >= G.dword_4FCFA0) {
                G.dword_4FD070 -= 2;
            }
            return;
        }
        G.dword_4FCF98 += 0x18;
        if (G.dword_4FCF98 >= G.dword_4FCFB8) {
            G.dword_4FD070 = cursor;
            G.dword_4FCF98 -= 0x18;
        } else {
            ListNodeAtInto(G.dword_4FCF98, useList(), useTop());
            CountListFBC();
            cursor = G.dword_4FD070;
            if (cursor >= G.dword_4FCFA0) {
                G.dword_4FD070 =
                    (cursor & 1) - ((G.dword_4FCFA0 & 1) == 0 ? 1 : 0) + G.dword_4FCFA0 - 1;
                if (G.dword_4FD070 >= G.dword_4FCFA0) {
                    G.dword_4FD070 -= 2;
                }
                return;
            }
        }
        LoadItemName(ListNodeAt(cursor + G.dword_4FCF98, useList()));
        PlaySound(0x2E, G.soundVolume);
        G.dword_4FD080 = 0x30;
    } else if (G.dword_4FD078 == 2) {
        G.dword_4FCFAC = lastHero();
    } else if (G.dword_4FD078 == 0xF) {
        MenuPageDown(&G.unlockedCount, &G.dword_4FBED4, &G.dword_4FC510, &G.dword_4FB8D4, 8);
        G.dword_4FD080 |= 0x3000;
    }
}

// swd32.exe 0x004437C0: to the first row in the same column, from there a screen back (at most to the start)
void UseKeyPageUp() {
    if (G.dword_4FD078 == 1) {
        if (G.dword_4FD070 > 1) {
            G.dword_4FD070 &= 1;
            return;
        }
        G.dword_4FCF98 -= 0x18;
        if (G.dword_4FCF98 >= 0) {
            ListNodeAtInto(G.dword_4FCF98, useList(), useTop());
            CountListFBC();
            G.dword_4FD070 &= 1;
        } else {
            G.dword_4FD070 &= 1;
            G.dword_4FCF98 = 0;
            ListNodeAtInto(0, useList(), useTop());
            CountListFBC();
        }
        showCursorItem(3);
    } else if (G.dword_4FD078 == 2) {
        G.dword_4FCFAC = firstHero();
    } else if (G.dword_4FD078 == 0xF) {
        MenuPageUp(static_cast<int>(reinterpret_cast<std::uintptr_t>(&G.unlockedCount)), &G.dword_4FBED4,
                   &G.dword_4FC510, static_cast<int>(reinterpret_cast<std::uintptr_t>(&G.dword_4FB8D4)), 8);
        G.dword_4FD080 |= 0x300;
    }
}

// swd32.exe 0x004438E0: to the other column (not past the last item)
void UseKeyLeft() {
    if (G.dword_4FD078 == 1) {
        G.dword_4FD070 += (G.dword_4FD070 & 1) != 1 ? 1 : -1;
        if (G.dword_4FD070 >= G.dword_4FCFA0) {
            G.dword_4FD070 = G.dword_4FCFA0 - 1;
        }
        LoadItemName(ListNodeAt(G.dword_4FD070 + G.dword_4FCF98, useList()));
        PlaySound(0x2E, G.soundVolume);
        G.dword_4FD080 = 3;
    } else if (G.dword_4FD078 == 2) {
        G.dword_4FCFAC = firstHero();
    } else if (G.dword_4FD078 == 0xF) {
        MenuCursorUp(static_cast<int>(reinterpret_cast<std::uintptr_t>(&G.unlockedCount)), &G.dword_4FBED4,
                     &G.dword_4FC510);
        G.dword_4FD080 |= 0x300;
    }
}

// swd32.exe 0x004439A0: to the other column (not past the last item)
void UseKeyRight() {
    if (G.dword_4FD078 == 1) {
        G.dword_4FD070 += (G.dword_4FD070 & 1) != 1 ? 1 : -1;
        if (G.dword_4FD070 >= G.dword_4FCFA0) {
            G.dword_4FD070 = G.dword_4FCFA0 - 1;
        }
        LoadItemName(ListNodeAt(G.dword_4FD070 + G.dword_4FCF98, useList()));
        PlaySound(0x2E, G.soundVolume);
        G.dword_4FD080 = 0x30;
    } else if (G.dword_4FD078 == 2) {
        G.dword_4FCFAC = lastHero();
    } else if (G.dword_4FD078 == 0xF) {
        MenuCursorDown(&G.unlockedCount, &G.dword_4FBED4, &G.dword_4FC510, &G.dword_4FB8D4);
        G.dword_4FD080 |= 0x3000;
    }
}

// swd32.exe 0x0043F940: equips entry `index` of `list` into the hero slot item `slot`: the slot gets a copy (count
// 1, own description, one taken from the entry) or the empty marker 0xFFDC when the entry has no count; the item that
// was in the slot gains a count in `list` if it suits menu mode G.dword_4FCADC, else in the inventory. The
// return value is unused by callers.
void SwapEquipment(std::uint8_t** list, std::uint8_t* slot, int index) {
    std::uint8_t* entry;
    ListNodeAtInto(index, list, &entry);
    const bool fits = ItemFitsMenu(slot, G.dword_4FCADC);
    const std::uint16_t previous = mem::u16(slot, 4);
    if (mem::u16(entry, 6) != 0) {
        crt::free(mem::ptr<std::uint8_t>(slot, 0xAC));
        std::memcpy(slot, entry, 0xB0);
        mem::put32(slot, 0xAC, 0);
        if (mem::u16(entry, 4) != 0xFFDC) {
            auto* text = static_cast<char*>(
                crt::malloc(static_cast<std::uint32_t>(w32::lstrlenA(mem::ptr<const char>(entry, 0xAC)) + 1)));
            mem::putPtr(slot, 0xAC, text);
            w32::lstrcpyA(text, mem::ptr<const char>(entry, 0xAC));
        }
        mem::put16(slot, 8, 0);
        mem::put16(slot, 0xA, 0);
        mem::put16(slot, 6, 1);
        mem::put16(entry, 6, static_cast<std::uint16_t>(mem::u16(entry, 6) - 1));
    } else {
        crt::free(mem::ptr<std::uint8_t>(slot, 0xAC));
        mem::put32(slot, 0xAC, 0);
        mem::put16(slot, 4, 0xFFDC);
        mem::put16(slot, 8, 0);
        mem::put16(slot, 0xA, 0);
        mem::put16(slot, 6, 1);
    }
    if (previous == 0xFFDC) {
        return;
    }
    std::uint8_t* owner = FindItemOfKind(fits ? list : &G.inventory, static_cast<std::int16_t>(previous));
    mem::put16(owner, 6, static_cast<std::uint16_t>(mem::u16(owner, 6) + 1));
}

}  // namespace swd3
