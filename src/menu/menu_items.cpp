#include "menu/menu_items.h"

#include <cstring>

#include "audio/audio.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "data/image.h"
#include "draw/draw.h"
#include "res/tsw.h"
#include "game/game_state.h"
#include "swd3/w32.h"

namespace swd3 {

using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

namespace {

void toggleOption(uint16_t firstSound) {
    if (G.dword_4FCD20 == 1) {
        if (++G.dword_4FCAB8 > 1) {
            G.dword_4FCAB8 = 0;
        }
        PlaySound(firstSound, G.soundVolume);
        return;
    }
    if (G.dword_4FCD20 == 2) {
        if (++G.dword_4FCA8C > 1) {
            G.dword_4FCA8C = 0;
        }
        if (GameFlag(0x1BA9)) {
            G.dword_4FCA8C = 1;
        }
        if ((G.dword_4FCAAC & 1) == 0) {
            G.dword_4FCA8C = 1;
        }
        if ((G.dword_4FCAAC & 2) == 0) {
            G.dword_4FCA8C = 0;
        }
        PlaySound(0x107, G.soundVolume);
        return;
    }
    if (G.dword_4FCD20 == 3) {
        G.dword_4FCAB4 = 0xC8;
    }
}

// insertion before the first node with an id not below the item's, the node before it having a smaller id; the
// search starts with `head` itself as the previous node
void insertById(uint8_t* head, uint8_t* item) {
    uint8_t* previous = head;
    uint8_t* current = mem::ptr<uint8_t>(head, 0);
    const uint16_t id = mem::u16(item, 4);
    while (current != nullptr) {
        if (mem::u16(current, 4) >= id && mem::u16(previous, 4) < id) {
            break;
        }
        current = mem::ptr<uint8_t>(current, 0);
        previous = mem::ptr<uint8_t>(previous, 0);
    }
    mem::putPtr(item, 0, mem::ptr<uint8_t>(previous, 0));
    mem::putPtr(previous, 0, item);
}

}  // namespace

// swd32.exe 0x0043E250: switches the option of the current page (+0x4FCD20: 1 and 2 toggle, 3 sets 200); page 2 is
// forced by game flag 0x1BA9 and the option mask. The return value is unused by callers.
void MenuToggleOption() { toggleOption(0x107); }

// swd32.exe 0x0043E310: as MenuToggleOption with sound 0x2E on page 1
void MenuToggleOptionQuiet() { toggleOption(0x2E); }

// swd32.exe 0x0043F080: the picked items go back into the inventory (front), empty items are freed. The return
// value is unused by callers.
void ReturnPickedItems() {
    for (auto* item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCAE0)); item != nullptr;
         item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCAE0))) {
        G.dword_4FCAE0 = mem::s32(item, 0);
        if (mem::u16(item, 4) == 0xFFDC) {
            crt::free(mem::ptr<uint8_t>(item, 0xAC));
            crt::free(item);
        } else {
            mem::putPtr(item, 0, G.inventory);
            G.inventory = item;
        }
    }
}

// swd32.exe 0x0043F160: sorts the picked items by id through a zeroed local head node
void SortPickedItems() {
    uint8_t head[0xB0] = {};
    for (auto* item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCAE0)); item != nullptr;
         item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCAE0))) {
        G.dword_4FCAE0 = mem::s32(item, 0);
        mem::putPtr(item, 0, static_cast<uint8_t*>(nullptr));
        insertById(head, item);
    }
    G.word_4FCAEA = 0;
    G.word_4FCAE8 = 0;
    G.dword_4FCAE0 = mem::s32(head, 0);
}

// swd32.exe 0x0043F7C0: whether `item` belongs on menu page `mode`: target types 0..9 and 15..19 only, kinds
// 1, 2, 4, 8, 0x10, 0x1000 on page 0, 0x100, 0x200, 0x400 on page 1, 0x800 on page 2 (bits 15 and 28..31 of the
// kind ignored). The name is compared with an empty text first and the result dropped.
bool ItemFitsMenu(const uint8_t* item, int mode) {
    w32::lstrcmpA(reinterpret_cast<const char*>(item + 0xC), reinterpret_cast<const char*>(&G.byte_4A6D10));
    const uint16_t type = mem::u16(item, 0x5E);
    const uint32_t kind = mem::u32(item, 0x2C) & 0x0FFF7FFF;
    if (type > 9 && (type < 0xF || type > 0x13)) {
        return false;
    }
    switch (kind) {
        case 1:
        case 2:
        case 4:
        case 8:
        case 0x10:
        case 0x1000:
            return mode == 0;
        case 0x100:
        case 0x200:
        case 0x400:
            return mode == 1;
        case 0x800:
            return mode == 2;
        default:
            return false;
    }
}

// swd32.exe 0x004404D0: element bonuses of the two compared items (G.dword_4FCA88, G.dword_4FCBA0): attack words
// at 0x004FCD14 / 0x004FCD18 and defence words at 0x004FCD16 / 0x004FCD1A, each flag word adding a multiple of the
// base (+0x60)
void ComputeEquipmentBonus() {
    const uint8_t* items[2] = {reinterpret_cast<const uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCA88)),
                               reinterpret_cast<const uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCBA0))};
    image::at<uint16_t>(0x004FCD1Au) = 0;
    image::at<uint16_t>(0x004FCD18u) = 0;
    image::at<uint16_t>(0x004FCD16u) = 0;
    image::at<uint16_t>(0x004FCD14u) = 0;
    for (int index = 0; index < 2; ++index) {
        const uint8_t* item = items[index];
        uint16_t& attack = image::at<uint16_t>(0x004FCD14u + 4u * index);
        uint16_t& defence = image::at<uint16_t>(0x004FCD16u + 4u * index);
        const uint16_t base = mem::u16(item, 0x60);
        static const struct {
            uint16_t field;
            uint16_t factor;
            bool defence;
        } kBonuses[] = {{0x72, 2, false}, {0x76, 3, false}, {0x7A, 5, false}, {0x86, 2, false},
                        {0x8A, 4, false}, {0x7E, 3, true},  {0x82, 5, true}};
        for (const auto& bonus : kBonuses) {
            if (mem::u16(item, bonus.field) != 0) {
                uint16_t& total = bonus.defence ? defence : attack;
                total = static_cast<uint16_t>(total + base * bonus.factor);
            }
        }
    }
}

// swd32.exe 0x004405C0: the return value is unused by callers
void FreeShopBuffers() {
    G.dword_4FC320 = 0;
    G.dword_4FCD20 = 4;
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FCB90)));
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FCD1C)));
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FCA9C)));
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FCA98)));
    G.dword_4FCB90 = 0;
    G.dword_4FCD1C = 0;
    G.dword_4FCA9C = 0;
    G.dword_4FCA98 = 0;
    G.dword_4FCAC8 = 0;
}

// swd32.exe 0x00441F70: moves the items of `source` of kind mask `kind` (table 0x0049E344, bit 15 ignored) usable by
// the current hero (word table 0x00499CD4 against +0x46) into the sorted list `target` (+0 head, +8 count cleared).
// The return value is unused by callers.
void FilterItemsByKind(uint8_t** source, uint8_t* target, int kind) {
    auto* link = reinterpret_cast<uint8_t*>(source);
    mem::put32(target, 0, 0);
    mem::put16(target, 8, 0);
    uint8_t* item = *source;
    while (item != nullptr) {
        const uint32_t mask = image::at<uint32_t>(0x0049E344u + 4u * static_cast<uint32_t>(kind));
        const uint32_t kindBits = mem::u32(item, 0x2C) & mask & ~0x8000u;
        const uint16_t heroes =
            image::at<uint16_t>(0x00499CD4u + 2u * static_cast<uint16_t>(G.dword_4FBECC));
        if (kindBits != mask || (mem::u16(item, 0x46) & heroes) == 0) {
            item = mem::ptr<uint8_t>(item, 0);
            link = mem::ptr<uint8_t>(link, 0);
            continue;
        }
        uint8_t* previous = target;
        uint8_t* current = mem::ptr<uint8_t>(target, 0);
        const uint16_t id = mem::u16(item, 4);
        while (current != nullptr) {
            if (mem::u16(current, 4) >= id && mem::u16(previous, 4) < id) {
                break;
            }
            current = mem::ptr<uint8_t>(current, 0);
            previous = mem::ptr<uint8_t>(previous, 0);
        }
        uint8_t* moved = item;
        mem::putPtr(link, 0, mem::ptr<uint8_t>(item, 0));
        item = mem::ptr<uint8_t>(item, 0);
        if (current == nullptr || mem::ptr<uint8_t>(target, 0) != nullptr) {
            mem::putPtr(moved, 0, mem::ptr<uint8_t>(previous, 0));
            mem::putPtr(previous, 0, moved);
        } else {
            mem::putPtr(target, 0, moved);
            mem::putPtr(moved, 0, static_cast<uint8_t*>(nullptr));
        }
    }
}

// swd32.exe 0x00442020: the return value is unused by callers
void CountListD64() {
    G.dword_4FCD34 = 0;
    for (auto* node = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCD64));
         node != nullptr && G.dword_4FCD34 < 0xA; node = mem::ptr<uint8_t>(node, 0)) {
        ++G.dword_4FCD34;
    }
}

// swd32.exe 0x004420F0: the items of the list at 0x004FCD70 go back into the inventory, empty ones are freed
// (without their description). The return value is unused by callers.
void ReturnListD70() {
    auto& head = image::at<std::uint32_t>(0x004FCD70u);
    for (auto* item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(head)); item != nullptr;
         item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(head))) {
        head = mem::u32(item, 0);
        if (mem::u16(item, 4) == 0xFFDC) {
            crt::free(item);
        } else {
            mem::putPtr(item, 0, G.inventory);
            G.inventory = item;
        }
    }
}

// swd32.exe 0x00442960: frame `frame` of picture `id` at (x, y), kept in G.dword_4FCD60
int DrawMenuPicture(int id, int frame, int x, int y) {
    Picture* picture = TswGetPicture(static_cast<uint32_t>(id), static_cast<uint32_t>(frame));
    G.dword_4FCD60 = static_cast<std::int32_t>(reinterpret_cast<std::uintptr_t>(picture));
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    return DrawPicture(x, y, picture->width, picture->height, 0, 0);
}

// swd32.exe 0x00442BC0: the fifteen stat words +4..+0x20 and the words +0x26 / +0x28 of the hero data at
// G.dword_4FCD4C, sign extended
std::int32_t* CopyHeroStats(std::int32_t* stats) {
    const auto* data = reinterpret_cast<const uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCD4C));
    for (int i = 0; i < 0xF; ++i) {
        stats[i] = mem::s16(data, 4 + 2 * i);
    }
    stats[0xF] = mem::s16(data, 0x26);
    stats[0x10] = mem::s16(data, 0x28);
    return stats;
}

// swd32.exe 0x00444DB0: moves the items of `source` whose target type (+0x5E) is table 0x00499CDC[`type`] or 0 into
// the sorted list `target`. The return value is unused by callers.
void FilterItemsByType(uint8_t** source, uint8_t* target, int type) {
    auto* link = reinterpret_cast<uint8_t*>(source);
    mem::put32(target, 0, 0);
    mem::put16(target, 8, 0);
    uint8_t* item = *source;
    while (item != nullptr) {
        const uint16_t itemType = mem::u16(item, 0x5E);
        if (itemType != image::at<uint32_t>(0x00499CDCu + 4u * static_cast<uint32_t>(type)) && itemType != 0) {
            item = mem::ptr<uint8_t>(item, 0);
            link = mem::ptr<uint8_t>(link, 0);
            continue;
        }
        uint8_t* previous = target;
        uint8_t* current = mem::ptr<uint8_t>(target, 0);
        const uint16_t id = mem::u16(item, 4);
        while (current != nullptr) {
            if (mem::u16(current, 4) >= id && mem::u16(previous, 4) < id) {
                break;
            }
            current = mem::ptr<uint8_t>(current, 0);
            previous = mem::ptr<uint8_t>(previous, 0);
        }
        uint8_t* moved = item;
        mem::putPtr(link, 0, mem::ptr<uint8_t>(item, 0));
        item = mem::ptr<uint8_t>(item, 0);
        if (current == nullptr || mem::ptr<uint8_t>(target, 0) != nullptr) {
            mem::putPtr(moved, 0, mem::ptr<uint8_t>(previous, 0));
            mem::putPtr(previous, 0, moved);
        } else {
            mem::putPtr(target, 0, moved);
            mem::putPtr(moved, 0, static_cast<uint8_t*>(nullptr));
        }
    }
}

// swd32.exe 0x00444E50: counts the list at G.dword_4FCFBC (at most 24) into G.dword_4FCFA0. The return value is
// unused by callers.
void CountListFBC() {
    G.dword_4FCFA0 = 0;
    for (auto* node = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCFBC));
         node != nullptr && G.dword_4FCFA0 < 0x18; node = mem::ptr<uint8_t>(node, 0)) {
        ++G.dword_4FCFA0;
    }
}

// swd32.exe 0x00444F00: the items of the list at 0x004FCFC0 go to the front of the item list of the current hero
// (sentinel G.itemLists[G.dword_4FBECC]); empty ones are freed. The return value is unused by callers.
void ReturnListFC0() {
    auto& head = image::at<std::uint32_t>(0x004FCFC0u);
    for (auto* item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(head)); item != nullptr;
         item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(head))) {
        head = mem::u32(item, 0);
        if (mem::u16(item, 4) == 0xFFDC) {
            crt::free(item);
            continue;
        }
        uint8_t* sentinel = G.itemLists[static_cast<uint16_t>(G.dword_4FBECC)];
        mem::putPtr(item, 0, mem::ptr<uint8_t>(sentinel, 0));
        mem::putPtr(G.itemLists[static_cast<uint16_t>(G.dword_4FBECC)], 0, item);
    }
}

// swd32.exe 0x00444F60: 3 pages, one more for each of the game flags 2 * G.dword_4FCFAC + 0x15 / + 0x16. The return
// value is unused by callers.
void CountStatusPages() {
    G.dword_4FCFB0 = 3;
    if (GameFlag(2 * G.dword_4FCFAC + 0x15)) {
        ++G.dword_4FCFB0;
    }
    if (GameFlag(2 * G.dword_4FCFAC + 0x16)) {
        ++G.dword_4FCFB0;
    }
}

// swd32.exe 0x00444FB0
int ReturnThree() { return 3; }

namespace {

void showPage(uint16_t page) {
    const auto offset = static_cast<std::int16_t>(page * 6 - 0x24);
    G.word_4FB8A8 = offset;
    G.word_4FC200 = offset;
    G.dword_4FB978 = page + 0x29;
    if (GameFlag(0x49) == 1) {
        if (G.dword_4FB978 == 0x38) {
            G.dword_4FB978 = 0x39;
        } else if (G.dword_4FB978 == 0x39) {
            G.dword_4FB978 = 0x38;
        }
    }
    PlaySound(0x8B, G.soundVolume);
}

}  // namespace

// swd32.exe 0x00445210: previous page (not below 11; with game flag 0x49 pages 0x38 / 0x39 swap acts). The return
// value is unused by callers.
void MenuPreviousPage() {
    auto page = static_cast<uint16_t>(G.dword_4FC3C4 - 1);
    if (page <= 0xA) {
        page = 0xB;
    }
    G.dword_4FC3C4 = static_cast<std::int32_t>((static_cast<uint32_t>(G.dword_4FC3C4) & 0xFFFF0000u) | page);
    showPage(page);
}

// swd32.exe 0x004452B0: next page (not above 15, 16 with game flag 0x49). The return value is unused by callers.
void MenuNextPage() {
    uint16_t last = 0xF;
    if (GameFlag(0x49)) {
        last = 0x10;
    }
    auto page = static_cast<uint16_t>(G.dword_4FC3C4 + 1);
    if (page > last) {
        page = last;
    }
    G.dword_4FC3C4 = static_cast<std::int32_t>((static_cast<uint32_t>(G.dword_4FC3C4) & 0xFFFF0000u) | page);
    showPage(page);
}

// swd32.exe 0x00447FD0: numbers 100.. in the 13 words at 0x004A6A00, then the list at G.dword_4FD150 is counted (at
// most 13) into G.dword_4FD298 and its end marked with 0. The return value is unused by callers.
void InitNumberList() {
    for (int i = 0; i < 0xD; ++i) {
        image::at<uint16_t>(0x004A6A00u + 2u * static_cast<uint32_t>(i)) = static_cast<uint16_t>(i + 0x64);
    }
    G.dword_4FD298 = 0;
    for (auto* node = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FD150));
         node != nullptr && G.dword_4FD298 < 0xD; node = mem::ptr<uint8_t>(node, 0)) {
        ++G.dword_4FD298;
    }
    image::at<uint16_t>(0x004A6A00u + 2u * static_cast<uint32_t>(G.dword_4FD298)) = 0;
}

}  // namespace swd3
