#include "battle/battle_order.h"

#include <cstring>

#include "battle/battle_enemy.h"
#include "battle/battle_hero.h"
#include "core/mem.h"
#include "data/globals.h"
#include "data/image.h"

namespace swd3 {

using std::int32_t;
using std::uint32_t;
using std::uint8_t;

// swd32.exe 0x0045B0E0: draw y of every enemy (0..) and hero (8..); returns the last hero y or 8 + hero count
int32_t CollectUnitDepths() {
    std::memset(G.unitDepths, 0, sizeof(G.unitDepths));
    std::memset(G.drawOrder, 0, sizeof(G.drawOrder));
    std::uint16_t x = 0;
    std::uint16_t y = 0;
    for (uint32_t enemy = 0; enemy < G.enemyCount; ++enemy) {
        G.battleEnemies[enemy].GetDrawPosition(&x, &y);
        G.unitDepths[enemy] = static_cast<std::int16_t>(y);
    }
    int32_t result = static_cast<int32_t>(G.heroCount + 8);
    for (uint32_t unit = 8; unit < G.heroCount + 8; ++unit) {
        G.battleHeroes[unit - 8].GetDrawPosition(&x, &y);
        result = static_cast<std::int16_t>(y);
        G.unitDepths[unit] = result;
    }
    return result;
}

// swd32.exe 0x0045B190: selection sort of the units by draw y into G.drawOrder; the search starts at the first
// unit with a non-zero y not taken yet (the slot after the table when there is none) and only scans the enemy
// count among enemies. Always returns 0.
int SortDrawOrder() {
    const uint32_t total = G.enemyCount + G.heroCount;
    for (uint32_t placed = 0; placed < total; ++placed) {
        uint32_t first = 0;
        while (first < 0x12 && (G.unitDepths[first] == 0 || G.drawOrderUsed[first] == 1)) {
            ++first;
        }
        int32_t best = image::at<int32_t>(0x00520E40u + 4u * first);
        uint32_t chosen = first;
        for (uint32_t unit = first + 1; unit < G.enemyCount; ++unit) {
            if (G.drawOrderUsed[unit] != 1 && G.unitDepths[unit] < best) {
                best = G.unitDepths[unit];
                chosen = unit;
            }
        }
        for (uint32_t unit = 8; unit < G.heroCount + 8; ++unit) {
            if (G.drawOrderUsed[unit] != 1 && G.unitDepths[unit] < best) {
                best = G.unitDepths[unit];
                chosen = unit;
            }
        }
        image::at<int32_t>(0x00520D5Cu + 4u * chosen) = 1;
        G.drawOrder[placed] = static_cast<int32_t>(chosen);
    }
    std::memset(G.drawOrderUsed, 0, sizeof(G.drawOrderUsed));
    return 0;
}

// swd32.exe 0x0045B5A0: the enemies (indices below 8) of the draw order, as many as there are enemies
int32_t CollectEnemyDrawOrder() {
    const uint32_t count = G.enemyCount;
    uint32_t taken = 0;
    int32_t result = 0;
    for (int32_t unit : G.drawOrder) {
        result = unit;
        if (unit < 8) {
            G.enemyDrawOrder[taken] = unit;
            if (++taken == count) {
                break;
            }
        }
    }
    return result;
}

namespace {

// bubble sort by id (+4, unsigned) restarting from the list head after every swap; +6 is cleared on the way
uint8_t* sortById(uint8_t** list, bool clearPending) {
    auto* link = reinterpret_cast<uint8_t*>(list);
    uint8_t* result = *list;
    if (*list == nullptr || mem::ptr<uint8_t>(*list, 0) == nullptr) {
        return result;
    }
    for (;;) {
        uint8_t* node = mem::ptr<uint8_t>(link, 0);
        uint8_t* next = mem::ptr<uint8_t>(node, 0);
        result = next;
        if (next == nullptr) {
            return result;
        }
        if (clearPending) {
            mem::put16(node, 6, 0);
        }
        if (mem::u16(node, 4) <= mem::u16(next, 4)) {
            link = node;
            continue;
        }
        mem::putPtr(node, 0, mem::ptr<uint8_t>(next, 0));
        mem::putPtr(next, 0, mem::ptr<uint8_t>(link, 0));
        mem::putPtr(link, 0, next);
        link = reinterpret_cast<uint8_t*>(list);
    }
}

}  // namespace

// swd32.exe 0x0045D250: sorts the inventory by id, clearing the reserved counts (+6)
uint8_t* SortInventory() { return sortById(&G.inventory, true); }

// swd32.exe 0x0045D2A0: sorts each of the four item lists by id; these lists start with a sentinel node
uint8_t* SortItemLists() {
    uint8_t* result = nullptr;
    for (uint8_t* sentinel : G.itemLists) {
        result = sortById(reinterpret_cast<uint8_t**>(sentinel), false);
    }
    return result;
}

// swd32.exe 0x0045EC60
std::int16_t SetBattleWord(int index, std::int16_t value) {
    image::at<std::int16_t>(0x004FF2EAu + 2u * static_cast<uint32_t>(index)) = value;
    return value;
}

// swd32.exe 0x0045EA30: releases the projector, the hero buffers and the enemy data. The return value is unused by
// callers.
void ReleaseBattleUnits() {
    G.projector.Release();
    for (BattleHero& hero : G.battleHeroes) {
        hero.Release();
    }
    for (BattleEnemy& enemy : G.battleEnemies) {
        enemy.FreeData();
    }
}

}  // namespace swd3
