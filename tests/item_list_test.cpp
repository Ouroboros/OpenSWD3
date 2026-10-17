#include <array>
#include <cstdio>
#include <string>
#include <string_view>

#include "game_data.h"
#include "item/item_list.h"
#include "org/database.h"
#include "test.h"
#include "text/big5.h"

using swd3::ItemCountMode;
using swd3::ItemLink;
using swd3::ItemNode;

namespace
{

int Length(const ItemLink& list)
{
    int length = 0;

    for (const ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        ++length;
    }

    return length;
}

constexpr swd3::u16 kSwordId = 1;
constexpr swd3::i16 kSwords = 5;
constexpr swd3::i16 kTooMany = 200;  // more than ItemNode::MaxCount
constexpr swd3::u16 kHerbId = 2;
constexpr swd3::i16 kHerbs = 3;
constexpr swd3::u16 kNoneId = 3;
constexpr swd3::i16 kEmptyCount = 7;  // the empty entry counts 1 regardless
constexpr swd3::i16 kPlainCount = 4;
constexpr swd3::i16 kFlaggedCount = 2;
constexpr swd3::i16 kTaken = 3;
constexpr int kEntries = 2;  // the plain and the flagged entry

// Ids with the two top bits FindItemOfKind ignores.
constexpr swd3::u16 kIdBit14 = 0x4000;
constexpr swd3::u16 kIdBit15 = 0x8000;
constexpr swd3::u16 kFirstKind = 5;
constexpr swd3::u16 kThirdKind = 7;
constexpr std::array<swd3::u16, 4> kIds = {kIdBit14 | kFirstKind, 6, kIdBit15 | kThirdKind, 8};
constexpr int kSecondIdAt = 2;
constexpr swd3::i16 kMergedCount = 60;  // with the pending count past ItemNode::MaxCount
constexpr swd3::i16 kMergedPending = 50;
constexpr swd3::u32 kPlainKind = 1;
constexpr std::size_t kSlots = 3;
constexpr swd3::u16 kMissingId = 9;

}  // namespace

TEST_CASE("item list: counts, pending counts and the empty entry")
{
    if (swd3test::ReadGameFile("mon.dat").empty())
    {
        std::printf("  (mon.dat not found, skipped)\n");
        return;
    }

    const std::string directory = swd3test::GameDirectory() + "/";
    swd3::Database database(directory);
    ItemLink list;
    ItemNode* sword = swd3::AddItem(list, kSwordId, kSwords, ItemCountMode::Count, database);
    CHECK(sword != nullptr);
    CHECK_EQ(sword->count, kSwords);
    CHECK(sword->record.data.name[0] != '\0');
    CHECK(swd3::AddItem(list, kSwordId, kTooMany, ItemCountMode::Count, database) == sword);
    CHECK_EQ(sword->count, ItemNode::MaxCount);
    ItemNode* herb = swd3::AddItem(list, kHerbId, kHerbs, ItemCountMode::Pending, database);
    CHECK_EQ(herb->pending, kHerbs);
    CHECK_EQ(herb->count, 0);
    CHECK((herb->record.data.kind & ItemNode::PendingKind) != 0);
    CHECK(list.get() == herb);
    swd3::AddItem(list, kHerbId, -1, ItemCountMode::Pending, database);
    CHECK_EQ(herb->pending, kHerbs - 1);
    CHECK(swd3::AddItem(list, kHerbId, -kHerbs - 1, ItemCountMode::Pending, database) == nullptr);
    CHECK_EQ(Length(list), 1);
    CHECK(swd3::AddItem(list, kNoneId, 0, ItemCountMode::Count, database) == nullptr);
    ItemNode* empty = swd3::AddItem(list, ItemNode::Empty, kEmptyCount, ItemCountMode::Count, database);
    CHECK_EQ(empty->count, 1);
    swd3::AddItem(list, ItemNode::Empty, kEmptyCount, ItemCountMode::Count, database);
    CHECK_EQ(empty->count, 1);
    swd3::FreeItemList(list);
    CHECK(list == nullptr);
}

TEST_CASE("item list: flagged and plain entries of one id")
{
    if (swd3test::ReadGameFile("mon.dat").empty())
    {
        std::printf("  (mon.dat not found, skipped)\n");
        return;
    }

    const std::string directory = swd3test::GameDirectory() + "/";
    swd3::Database database(directory);
    ItemLink list;
    ItemNode* plain = swd3::AddFlaggedItem(list, kSwordId, kPlainCount, 0, database);
    ItemNode* flagged = swd3::AddFlaggedItem(list, kSwordId, kFlaggedCount, 1, database);
    CHECK(plain != flagged);
    CHECK_EQ(Length(list), kEntries);
    CHECK_EQ(flagged->count, kFlaggedCount);
    CHECK(swd3::AddFlaggedItem(list, kSwordId, -kTaken, 0, database) == plain);
    CHECK_EQ(plain->count, kPlainCount + kFlaggedCount - kTaken);  // the flagged entry is folded into the plain one
    CHECK_EQ(Length(list), 1);
    CHECK(swd3::AddFlaggedItem(list, kSwordId, -kTaken, 0, database) == nullptr);
    CHECK(list == nullptr);
}

TEST_CASE("item list: merge, reverse copy, unlink, lookups")
{
    ItemLink list;

    for (const swd3::u16 id : kIds)
    {
        std::unique_ptr<ItemNode> node = swd3::NewEmptyItem();
        node->id = id;
        node->count = kMergedCount;
        node->pending = kMergedPending;
        node->record.data.kind = ItemNode::PendingKind | kPlainKind;
        node->next = std::move(list);
        list = std::move(node);
    }

    CHECK_EQ(list->count, kMergedCount);
    swd3::MergePendingCounts(list);
    CHECK_EQ(list->count, ItemNode::MaxCount);
    CHECK_EQ(list->pending, 0);
    CHECK_EQ(list->record.data.kind, kPlainKind);
    ItemLink copy = swd3::CopyItemList(list);
    CHECK_EQ(copy->id, kIds.front());
    CHECK(copy->record.description.has_value());
    // The list holds the ids in reverse.
    CHECK_EQ(swd3::ItemAt(list, kSecondIdAt)->id, kIds[1]);
    CHECK(swd3::ItemAt(list, static_cast<int>(kIds.size())) == nullptr);
    CHECK_EQ(swd3::FindItemOfKind(list, kThirdKind)->id, kIdBit15 | kThirdKind);
    CHECK_EQ(swd3::FindItemOfKind(list, kFirstKind)->id, kIdBit14 | kFirstKind);
    std::unique_ptr<ItemNode> taken = swd3::UnlinkItem(list, kIds[1]);
    CHECK(taken != nullptr);
    CHECK(taken->next == nullptr);
    CHECK_EQ(Length(list), static_cast<int>(kIds.size()) - 1);
    CHECK(swd3::UnlinkItem(list, kIds[1]) == nullptr);
    std::array<std::unique_ptr<ItemNode>, kSlots> slots = {swd3::NewEmptyItem(), nullptr, std::move(taken)};
    CHECK(swd3::FindRoleItem(slots, kIds[1]) == slots.back().get());
    CHECK(swd3::FindRoleItem(slots, kMissingId) == nullptr);
    const std::string emptyName = swd3::Big5(u8"無");
    CHECK(std::string_view(swd3::NewEmptyItem()->record.data.name.data()).starts_with(emptyName));
}
