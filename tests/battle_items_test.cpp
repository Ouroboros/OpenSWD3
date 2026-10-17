#include <array>
#include <cstdio>
#include <vector>

#include "battle/battle_items.h"
#include "battle/battle_state.h"
#include "game/game_state.h"
#include "game_data.h"
#include "party/party.h"
#include "test.h"

using swd3::ItemLink;
using swd3::ItemNode;
using swd3::u16;
using swd3::u32;

namespace
{

constexpr u16 kDefaultPrice = 10;
constexpr swd3::i16 kUnk06 = 7;  // the sort clears it

// ItemPriority: a kind moved up by default; kind bits that sort first, by the top byte or last.
constexpr u32 kPlainKind = 3;
constexpr int kKindShift = 16;
constexpr u32 kFirstKind = 0x100;
constexpr u32 kFirstPriority = 1;
constexpr u32 kTopByteKind = 0x200;
constexpr u32 kTopByte = 0x7F;
constexpr int kTopByteShift = 24;
constexpr u32 kLastKind = 0x400;
constexpr u32 kLastPriority = 0x800;
constexpr u32 kNoPriceKind = 0x40;  // the shop leaves items of this kind

constexpr std::array<u32, 5> kSortKinds = {5, 3, 4, 4, 1};  // of items 1..5
constexpr std::array<u16, 5> kSortedIds = {5, 2, 4, 3, 1};
constexpr swd3::i16 kCount = 3;
constexpr u16 kPrice = 50;
constexpr int kShopPercent = 60;
constexpr int kPercent = 100;
constexpr u16 kSoldId = 1;
constexpr u16 kFreeId = 2;
constexpr u16 kNotSoldId = 3;

constexpr u16 kCountedId = 9;
constexpr u16 kOtherId = 8;
constexpr swd3::i16 kCarried = 4;
constexpr swd3::i16 kPending = 2;
constexpr std::size_t kWornSlot = 3;
constexpr int kListBEntries = 20;
constexpr swd3::i32 kListBLimit = 13;  // CountListB counts no further
constexpr int kGaugePerSpeed = 100;    // the gauge fills at kGaugePerSpeed * (kSpeeds - speed)
constexpr int kSpeeds = 20;
constexpr int kSpeed = 11;

void Push(ItemLink& list, u16 id, u32 kind, u16 price = kDefaultPrice, swd3::i16 count = 1)
{
    auto node = std::make_unique<ItemNode>();
    node->id = id;
    node->record.data.kind = kind;
    node->record.data.price = price;
    node->count = count;
    node->unk06 = kUnk06;
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
    CHECK_EQ(swd3::ItemPriority(kPlainKind), kPlainKind << kKindShift);
    CHECK_EQ(swd3::ItemPriority(kFirstKind), kFirstPriority);
    CHECK_EQ(swd3::ItemPriority(kTopByteKind | kTopByte << kTopByteShift), kTopByte);
    CHECK_EQ(swd3::ItemPriority(kLastKind | kFirstKind), kLastPriority);
}

TEST_CASE("battle items: the sort inserts before a node ranking not below whose predecessor ranks below")
{
    swd3::BattleState battle;
    ItemLink source;

    // Read in order: 1 (kind 5), 2 (kind 3), 3 (kind 4), 4 (kind 4), 5 (kind 1).
    for (std::size_t index = kSortKinds.size(); index-- > 0;)
    {
        Push(source, static_cast<u16>(index + 1), kSortKinds[index]);
    }

    swd3::SortBattleItems(battle, source);
    CHECK(source == nullptr);
    // 1: [1]; 2 (3 < 5, predecessor 0): [2 1]; 3 (4): 2 ranks 3 < 4, 1 ranks 5 >= 4 with 3 < 4: [2 3 1];
    // 4 (4): at 3 (4 >= 4, 3 < 4): [2 4 3 1]; 5 (1): at 2: [5 2 4 3 1].
    CHECK(Ids(battle.itemsHead.next) == (std::vector<u16>(kSortedIds.begin(), kSortedIds.end())));
    CHECK_EQ(battle.itemsHead.next->unk06, 0);
    battle.itemsHead.next->unk06 = kCount;
    battle.itemsHead.next->record.data.price = kPrice;
    CHECK_EQ(swd3::TotalItemValue(battle), kCount * kPrice);
    battle.mode = swd3::BattleState::SellMode;
    CHECK_EQ(swd3::TotalItemValue(battle), kCount * kPrice * kShopPercent / kPercent);
    swd3::FreeBattleItems(battle);
    CHECK(battle.itemsHead.next == nullptr);
}

TEST_CASE("battle items: the shop mode leaves items without a price or with kind bit 6")
{
    swd3::BattleState battle;
    battle.mode = swd3::BattleState::SellMode;
    ItemLink source;
    Push(source, kNotSoldId, kNoPriceKind);
    Push(source, kFreeId, 1, 0);
    Push(source, kSoldId, 1);
    swd3::SortBattleItems(battle, source);
    CHECK(Ids(battle.itemsHead.next) == (std::vector<u16>{kSoldId}));
    CHECK(Ids(source) == (std::vector<u16>{kFreeId, kNotSoldId}));
    CHECK_EQ(source->unk06, 0);
}

TEST_CASE("battle items: counts of the inventory and the equipment, the list limit, the gauge")
{
    using swd3::Party;
    swd3::BattleState battle;
    Party party;
    swd3::GameState state;
    Push(party.inventory, kCountedId, 0, kDefaultPrice, kCarried);
    party.inventory->pending = kPending;
    Push(party.inventory, kOtherId, 0, kDefaultPrice, 1);

    for (auto& slot : party.equipment)
    {
        slot = swd3::NewEmptyItem();
    }

    constexpr std::size_t kFirstRoleSlot = kWornSlot;
    constexpr std::size_t kSecondRoleSlot = Party::SlotsPerRole + kWornSlot;
    party.equipment[kFirstRoleSlot]->id = kCountedId;
    party.equipment[kFirstRoleSlot]->count = 1;
    party.equipment[kSecondRoleSlot]->id = kCountedId;
    party.equipment[kSecondRoleSlot]->count = 1;
    constexpr int kInInventory = kCarried + kPending;
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, kCountedId), kInInventory);
    state.SetGameFlag(Party::FirstRoleFlag);
    const int withFirst = kInInventory + 1;  // the slot of the first role counts once it has joined
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, kCountedId), withFirst);
    state.SetGameFlag(Party::FirstRoleFlag + 1);
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, kCountedId), withFirst + 1);
    battle.mode = swd3::BattleState::UnkMode2;
    CHECK_EQ(swd3::CountItemTotal(battle, party, state, kCountedId), kInInventory);

    ItemLink shown;

    for (int i = 0; i < kListBEntries; ++i)
    {
        Push(shown, static_cast<u16>(i), 0);
    }

    battle.listB = shown.get();
    swd3::CountListB(battle);
    CHECK_EQ(battle.listBCount, kListBLimit);
    CHECK_EQ(battle.gaugeFull, swd3::BattleState::InitialGaugeFull);
    CHECK_EQ(swd3::SetGaugeFull(battle, kSpeed), kGaugePerSpeed * (kSpeeds - kSpeed));
    CHECK_EQ(swd3::SetGaugeFull(battle, 0), kGaugePerSpeed * kSpeeds);
}

TEST_CASE("party: the object kinds of the equipment slots match swd32.exe")
{
    constexpr swd3::u32 kSlotKindsAt = 0x0049E344;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kSlotKindsAt, swd3::Party::SlotKinds));
}
