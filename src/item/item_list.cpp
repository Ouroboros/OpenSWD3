#include "item/item_list.h"

#include <cstring>

#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "swd3/w32.h"

namespace swd3 {

using std::uint16_t;
using std::uint8_t;

// swd32.exe 0x0044D010: counts the non-zero entries from G.menuFirst (at most 5) into G.menuVisible. The return
// value is unused by callers.
void CountVisibleEntries() {
    const uint16_t* entry = G.menuEntries + G.menuFirst;
    int visible = 0;
    G.menuVisible = 0;
    while (*entry != 0 && visible < 5) {
        ++visible;
        ++entry;
        G.menuVisible = visible;
    }
}

// swd32.exe 0x0044D050
std::uint32_t* AdvanceByWords(int count, std::uint32_t base, std::uint32_t* out) {
    *out = base;
    for (int i = count; i > 0; --i) {
        *out += 2;
    }
    return out;
}

// swd32.exe 0x0044D4E0: adds the pending counts (at most 99 in all) and clears bit 15 of the kind. The return
// value is unused by callers.
void MergePendingCounts(uint8_t** list) {
    for (uint8_t* item = *list; item != nullptr; item = mem::ptr<uint8_t>(item, 0)) {
        mem::ref<std::uint32_t>(item, 0x2C) &= 0xFFFF7FFF;
        mem::put16(item, 8, static_cast<uint16_t>(mem::u16(item, 8) + mem::u16(item, 0xA)));
        mem::put16(item, 0xA, 0);
        if (mem::s16(item, 8) > 0x63) {
            mem::put16(item, 8, 0x63);
        }
    }
}

// swd32.exe 0x0044D520: copies the list (in reverse order) with its descriptions; null for an empty list
uint8_t* CopyItemList(uint8_t** list) {
    uint8_t* copy = nullptr;
    uint8_t* previous = nullptr;
    for (uint8_t* item = *list; item != nullptr; item = mem::ptr<uint8_t>(item, 0)) {
        copy = static_cast<uint8_t*>(crt::malloc(0xB0));
        std::memcpy(copy, item, 0xB0);
        const char* text = mem::ptr<const char>(item, 0xAC);
        auto* description = static_cast<char*>(crt::malloc(static_cast<std::uint32_t>(w32::lstrlenA(text) + 1)));
        mem::putPtr(copy, 0xAC, description);
        w32::lstrcpyA(description, mem::ptr<const char>(item, 0xAC));
        mem::putPtr(copy, 0, previous);
        previous = copy;
    }
    return copy;
}

// swd32.exe 0x0044D5A0: the return value is unused by callers
void FreeItemList(uint8_t** list) {
    for (uint8_t* item = *list; *list != nullptr; item = *list) {
        *list = mem::ptr<uint8_t>(item, 0);
        crt::free(mem::ptr<uint8_t>(item, 0xAC));
        crt::free(item);
    }
}

// swd32.exe 0x0044D5D0: an empty item (id 0xFFDC, count 1, the empty name)
uint8_t* NewEmptyItem() {
    auto* item = static_cast<uint8_t*>(crt::malloc(0xB0));
    std::memset(item, 0, 0xB0);
    mem::put16(item, 6, 0);
    mem::put32(item, 0, 0);
    mem::put16(item, 4, 0xFFDC);
    mem::put16(item, 8, 1);
    w32::lstrcpyA(reinterpret_cast<char*>(item + 0xC), G.emptyItemName);
    return item;
}

// swd32.exe 0x0044D620: unlinks and returns the item `id` (null when missing)
uint8_t* UnlinkItem(uint8_t** list, std::int16_t id) {
    auto* link = reinterpret_cast<uint8_t*>(list);
    uint8_t* item = *list;
    if (item == nullptr) {
        return nullptr;
    }
    while (mem::s16(item, 4) != id) {
        item = mem::ptr<uint8_t>(item, 0);
        link = mem::ptr<uint8_t>(link, 0);
        if (item == nullptr) {
            return nullptr;
        }
    }
    mem::putPtr(link, 0, mem::ptr<uint8_t>(item, 0));
    return item;
}

// swd32.exe 0x0044D650: the first hero item slot holding `id` (every slot must hold an item)
uint8_t* FindHeroItem(std::int16_t id) {
    for (uint8_t* item : G.heroItemSlots) {
        if (mem::s16(item, 4) == id) {
            return item;
        }
    }
    return nullptr;
}

// swd32.exe 0x0044D680: the first item whose id without its two top bits is `id`
uint8_t* FindItemOfKind(uint8_t** list, std::int16_t id) {
    for (uint8_t* item = *list; item != nullptr; item = mem::ptr<uint8_t>(item, 0)) {
        if ((mem::u16(item, 4) & 0x3FFF) == static_cast<uint16_t>(id)) {
            return item;
        }
    }
    return nullptr;
}

// swd32.exe 0x0044D6B0
uint8_t* ItemAt(uint8_t** list, uint16_t index) {
    uint8_t* item = *list;
    for (uint16_t position = 0; item != nullptr && position != index; ++position) {
        item = mem::ptr<uint8_t>(item, 0);
    }
    return item;
}

}  // namespace swd3
