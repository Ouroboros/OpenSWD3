#include <cstring>

#include "battle/battle_message.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "test.h"

using namespace swd3;

TEST_CASE("battle.messages append in order and rising ones start above the screen") {
    G.battleMessages = nullptr;
    AddBattleMessage(10, 20, 30, "abcd", kMessageCentered);
    AddBattleMessage(1, 2, 3, "xy", kMessageRise);
    BattleMessage* first = G.battleMessages;
    CHECK(first != nullptr);
    CHECK_EQ(first->x, 10);
    CHECK_EQ(first->y, 20);
    CHECK_EQ(first->frames, 30);
    CHECK_EQ(first->length, 4);
    CHECK_EQ(first->flags, kMessageCentered);
    BattleMessage* second = first->next;
    CHECK(second != nullptr);
    CHECK(second->next == nullptr);
    CHECK_EQ(second->y, 1);
    CHECK_EQ(second->slideY, -0x20);
    crt::free(second);
    crt::free(first);
    G.battleMessages = nullptr;
}

TEST_CASE("battle.panel grows by a third of the rest until it stops") {
    G.panelHeight = 0;
    CHECK_EQ(GrowPanel(0xD4, 0xF4, 3), 0);
    CHECK_EQ(G.panelHeight, 10);
    CHECK_EQ(GrowPanel(0xD4, 0xF4, 3), 0);
    CHECK_EQ(G.panelHeight, 17);
    while (GrowPanel(0xD4, 0xF4, 3) == 0) {
    }
    CHECK_EQ(G.panelHeight, 30);
    G.panelHeight = 0;
}

TEST_CASE("battle.pending action is the first queued entry") {
    std::memset(G.actionQueue, 0, sizeof(G.actionQueue));
    G.actionQueue[2 * 10 + 3] = 7;
    G.actionQueue[3 * 10 + 0] = 9;
    G.pendingAction = 0;
    TakePendingAction();
    CHECK_EQ(G.pendingAction, 7);
    CHECK_EQ(G.pendingActionRow, 2);
    CHECK_EQ(G.actionQueue[23], 0);
    TakePendingAction();
    CHECK_EQ(G.pendingAction, 7);
    G.pendingAction = 0;
    TakePendingAction();
    CHECK_EQ(G.pendingAction, 9);
    CHECK_EQ(G.pendingActionRow, 3);
    G.pendingAction = 0;
    G.pendingActionRow = 0;
}
