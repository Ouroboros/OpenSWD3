#include <vector>

#include "battle/battle_items.h"
#include "battle/battle_state.h"
#include "game/game_state.h"
#include "party/party.h"
#include "test.h"

using swd3::ItemLink;
using swd3::ItemNode;
using swd3::u16;
using swd3::u32;

namespace
{

void Push(ItemLink& list, u16 id, u32 kind, u16 price = 10, swd3::i16 count = 1)
{
    auto node = std::make_unique<ItemNode>();
    node->id = id;
    node->record.data.kind = kind;
    node->record.data.price = price;
    node->count = count;
    node->unk06 = 7;
    node->next = std::move(list);
    list = std::move(node);
}

std::vector<u16> Ids(const ItemLink& list)
{
    std::vector<u16> ids;

    for (const ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        ids.push_back(node->id);
    }

    return ids;
}

}  // namespace

TEST_CASE("battle items: priorities of the kind bits")
{
    CHECK_EQ(swd3::ItemPriority(0x3), 0x30000u);
    CHECK_EQ(swd3::ItemPriority(0x100), 1u);
    CHECK_EQ(swd3::ItemPriority(0x7F000200), 0x7Fu);
    CHECK_EQ(swd3::ItemPriority(0x500), 0x800u);
}

TEST_CASE("battle items: the sort inserts before a node ranking not below whose predecessor ranks below")
{
    swd3::BattleState battle;
    ItemLink source;
    // Read in order: 1 (kind 5), 2 (kind 3), 3 (kind 4), 4 (kind 4), 5 (kind 1).
    Push(source, 5, 1);
    Push(source, 4, 4);
    Push(source, 3, 4);
    Push(source, 2, 3);
    Push(source, 1, 5);
    swd3::SortBattleItems(battle, source);
    CHECK(source == nullptr);
    // 1: [1]; 2 (3 < 5, predecessor 0): [2 1]; 3 (4): 2 ranks 3 < 4, 1 ranks 5 >= 4 with 3 < 4: [2 3 1];
    // 4 (4): at 3 (4 >= 4, 3 < 4): [2 4 3 1]; 5 (1): at 2: [5 2 4 3 1].
    CHECK(Ids(battle.items) == (std::vector<u16>{5, 2, 4, 3, 1}));
    CHECK_EQ(battle.items->unk06, 0);
    battle.items->unk06 = 3;
    battle.items->record.data.price = 50;
    CHECK_EQ(swd3::TotalItemValue(battle), 3 * 50);
    battle.mode = 1;
    CHECK_EQ(swd3::TotalItemValue(battle), 3 * 50 * 60 / 100);
    swd3::FreeBattleItems(battle);
    CHECK(battle.items == nullptr);
}

TEST_CASE("battle items: the shop mode leaves items without a price or with kind bit 6")
{
    swd3::BattleState battle;
    battle.mode = 1;
    ItemLink source;
    Push(source, 3, 0x40);
    Push(source, 2, 1, 0);
    Push(source, 1, 1);
    swd3::SortBattleItems(battle, source);
    CHECK(Ids(battle.items) == (std::vector<u16>{1}));
    CHECK(Ids(source) == (std::vector<u16>{2, 3}));
    CHECK_EQ(source->unk06, 0);
}

TEST_CASE("battle items: counts of the inventory and the equipment, the list limit, the gauge")
{
    swd3::BattleState battle;
    swd3::Party party;
    swd3::GameState state;
    Push(party.inventory, 9, 0, 10, 4);
    party.inventory->pending = 2;
    Push(party.inventory, 8, 0, 10, 1);

    for (auto& slot : party.equipment)
    {
        slot = swd3::NewEmptyItem();
    }

    party.equipment[3]->id = 9;
    party.equipment[3]->count = 1;
    party.equipment[20]->id = 9;
    party.equipment[20]->count = 1;
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, 9), 6);
    state.SetGameFlag(0x1E);
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, 9), 7);
    state.SetGameFlag(0x1F);
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, 9), 8);
    battle.mode = 2;
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, 9), 6);

    for (int i = 0; i < 20; ++i)
    {
        Push(battle.listB, static_cast<u16>(i), 0);
    }

    swd3::CountListB(battle);
    CHECK_EQ(battle.listBCount, 13);
    CHECK_EQ(battle.gaugeFull, 900);
    CHECK_EQ(swd3::SetGaugeFull(battle, 11), 900);
    CHECK_EQ(swd3::SetGaugeFull(battle, 0), 2000);
}
