#include "battle/battle_items.h"

#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "data/image.h"
#include "game/game_state.h"
#include "gfx/graphics.h"
#include "swd3/w32.h"

namespace swd3 {

using std::uint32_t;
using std::uint8_t;

// swd32.exe 0x0044F770: value of the battle item list: price (+0x52) times count (+6), 60% in mode bit 0
int TotalItemValue() {
    const int rate = (G.battleMode & 1) == 1 ? 0x3C : 0x64;
    int total = 0;
    for (uint8_t* item = G.battleItems; item != nullptr; item = mem::ptr<uint8_t>(item, 0)) {
        total += static_cast<int>(mem::u16(item, 0x52) * mem::u16(item, 6) * rate) / 100;
    }
    return total;
}

// swd32.exe 0x0044F7D0: the return value is unused by callers
void CountListB() {
    G.battleListBCount = 0;
    for (uint8_t* entry = G.battleListB; entry != nullptr && G.battleListBCount < 0xD;
         entry = mem::ptr<uint8_t>(entry, 0)) {
        ++G.battleListBCount;
    }
}

// swd32.exe 0x0044FF90: sort priority of the kind bits: by default the kind shifted up 16; bit 8 gives 1, bit 9
// the top byte, bit 10 0x800
uint32_t ItemPriority(uint32_t kind) {
    uint32_t priority = kind << 16;
    if ((kind & 0x100) != 0) {
        priority = 1;
    }
    if ((kind & 0x200) != 0) {
        priority = kind >> 24;
    }
    if ((kind & 0x400) != 0) {
        return 0x800;
    }
    return priority;
}

// swd32.exe 0x0044F800: moves the items of `source` (+6 cleared) into G.battleItems ordered by priority;
// in mode 1 items without price or with kind bit 6 stay. The insertion compares with the node before the
// position, which for the first position is the list variable read as a node (its +0x2C is the global at
// 0x004FD5AC).
void SortBattleItems(uint8_t** source) {
    auto* link = reinterpret_cast<uint8_t*>(source);
    uint8_t* item = *source;
    G.battleItems = nullptr;
    G.battleItemCursor = 0;
    while (item != nullptr) {
        mem::put16(item, 6, 0);
        if ((G.battleMode & 3) == 1 && (mem::u16(item, 0x52) == 0 || (item[0x2C] & 0x40) != 0)) {
            item = mem::ptr<uint8_t>(item, 0);
            link = mem::ptr<uint8_t>(link, 0);
            continue;
        }
        uint8_t* position = G.battleItems;
        auto* before = reinterpret_cast<uint8_t*>(&G.battleItems);
        while (position != nullptr) {
            const uint32_t here = ItemPriority(mem::u32(position, 0x2C));
            const uint32_t previous = ItemPriority(mem::u32(before, 0x2C));
            const uint32_t own = ItemPriority(mem::u32(item, 0x2C));
            if (here >= own && previous < own) {
                break;
            }
            position = mem::ptr<uint8_t>(position, 0);
            before = mem::ptr<uint8_t>(before, 0);
        }
        uint8_t* moved = item;
        mem::putPtr(link, 0, mem::ptr<uint8_t>(item, 0));
        item = mem::ptr<uint8_t>(item, 0);
        if (position == nullptr || G.battleItems != nullptr) {
            mem::putPtr(moved, 0, mem::ptr<uint8_t>(before, 0));
            mem::putPtr(before, 0, moved);
        } else {
            G.battleItems = moved;
            mem::putPtr(moved, 0, static_cast<uint8_t*>(nullptr));
        }
    }
}

// swd32.exe 0x0044F8E0: the return value is unused by callers
void FreeBattleItems() {
    for (uint8_t* item = G.battleItems; G.battleItems != nullptr; item = G.battleItems) {
        G.battleItems = mem::ptr<uint8_t>(item, 0);
        crt::free(mem::ptr<uint8_t>(item, 0xAC));
        crt::free(item);
    }
}

// swd32.exe 0x0044FA40: count (+8 and +0xA) of item `id` in the inventory; outside the shop modes also the
// counts (+8) in the slots of the heroes present (game flags 0x1E..0x21)
int CountItemTotal(int id) {
    int total = 0;
    for (uint8_t* item = G.inventory; item != nullptr; item = mem::ptr<uint8_t>(item, 0)) {
        if (mem::u16(item, 4) == static_cast<uint32_t>(id)) {
            total += mem::s16(item, 8) + mem::s16(item, 0xA);
        }
    }
    if ((G.battleMode & 3) != 0) {
        return total;
    }
    for (int hero = 0; hero < 4; ++hero) {
        if (GameFlag(hero + 0x1E) == 0) {
            continue;
        }
        for (int slot = 0; slot < 0x10; ++slot) {
            const uint8_t* item = G.heroItemSlots[hero * 0x10 + slot];
            if (mem::u16(item, 4) == static_cast<uint32_t>(id)) {
                total += mem::s16(item, 8);
            }
        }
    }
    return total;
}

// swd32.exe 0x0044FFC0: the gauge fills at 100 * (20 - speed)
int SetGaugeFull(int speed) {
    const int full = 0x64 * (0x14 - speed);
    G.gaugeFull = full;
    return full;
}

// swd32.exe 0x00451A90: two screen sized offscreen surfaces; the three words at 0x004A7632 become 0xFFFF
int CreateBattleSurfaces() {
    for (w32::IDirectDrawSurface4*& surface : G.battleSurfaces) {
        const int height = w32::GetSystemMetrics(1);
        const int width = w32::GetSystemMetrics(0);
        surface = G.graphics.CreateOffscreen(width, height);
    }
    G.word_4A7632 = -1;
    G.word_4A7634 = -1;
    G.word_4A7636 = -1;
    return -1;
}

// swd32.exe 0x00451AE0: the return value is unused by callers
void ReleaseBattleSurfaces() {
    for (w32::IDirectDrawSurface4*& surface : G.battleSurfaces) {
        if (surface != nullptr) {
            surface->Release();
            surface = nullptr;
        }
    }
}

}  // namespace swd3
