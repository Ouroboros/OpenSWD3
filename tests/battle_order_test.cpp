#include <cstring>

#include "battle/battle_order.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "test.h"

using namespace swd3;

TEST_CASE("battle.draw order sorts units by depth and lists enemies") {
    const std::uint32_t savedEnemies = G.enemyCount;
    const std::uint32_t savedHeroes = G.heroCount;
    G.enemyCount = 2;
    G.heroCount = 1;
    std::memset(G.unitDepths, 0, sizeof(G.unitDepths));
    G.unitDepths[0] = 50;
    G.unitDepths[1] = 30;
    G.unitDepths[8] = 40;
    CHECK_EQ(SortDrawOrder(), 0);
    CHECK_EQ(G.drawOrder[0], 1);
    CHECK_EQ(G.drawOrder[1], 8);
    CHECK_EQ(G.drawOrder[2], 0);
    CHECK_EQ(G.drawOrderUsed[0], 0);
    for (int i = 3; i < 18; ++i) {
        G.drawOrder[i] = 99;
    }
    CollectEnemyDrawOrder();
    CHECK_EQ(G.enemyDrawOrder[0], 1);
    CHECK_EQ(G.enemyDrawOrder[1], 0);
    G.enemyCount = savedEnemies;
    G.heroCount = savedHeroes;
}

TEST_CASE("item.lists sort by id") {
    std::uint8_t nodes[3][0x10] = {};
    mem::put16(nodes[0], 4, 9);
    mem::put16(nodes[1], 4, 3);
    mem::put16(nodes[2], 4, 5);
    mem::put16(nodes[0], 6, 7);
    mem::putPtr(nodes[0], 0, nodes[1]);
    mem::putPtr(nodes[1], 0, nodes[2]);
    std::uint8_t* saved = G.inventory;
    G.inventory = nodes[0];
    SortInventory();
    CHECK(G.inventory == nodes[1]);
    CHECK(mem::ptr<std::uint8_t>(nodes[1], 0) == nodes[2]);
    CHECK(mem::ptr<std::uint8_t>(nodes[2], 0) == nodes[0]);
    CHECK(mem::ptr<std::uint8_t>(nodes[0], 0) == nullptr);
    CHECK_EQ(mem::u16(nodes[0], 6), 0);
    G.inventory = saved;
}
