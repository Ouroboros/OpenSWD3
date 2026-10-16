#include <array>
#include <cstdio>
#include <string>

#include "game_data.h"
#include "item/item_list.h"
#include "org/database.h"
#include "test.h"

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
    ItemNode* sword = swd3::AddItem(list, 1, 5, ItemCountMode::Count, database);
    CHECK(sword != nullptr);
    CHECK_EQ(sword->count, 5);
    CHECK(sword->record.data.name[0] != '\0');
    CHECK(swd3::AddItem(list, 1, 200, ItemCountMode::Count, database) == sword);
    CHECK_EQ(sword->count, 99);
    ItemNode* herb = swd3::AddItem(list, 2, 3, ItemCountMode::Pending, database);
    CHECK_EQ(herb->pending, 3);
    CHECK_EQ(herb->count, 0);
    CHECK((herb->record.data.kind & ItemNode::kPendingKind) != 0);
    CHECK(list.get() == herb);
    swd3::AddItem(list, 2, -1, ItemCountMode::Pending, database);
    CHECK_EQ(herb->pending, 2);
    CHECK(swd3::AddItem(list, 2, -5, ItemCountMode::Pending, database) == nullptr);
    CHECK_EQ(Length(list), 1);
    CHECK(swd3::AddItem(list, 3, 0, ItemCountMode::Count, database) == nullptr);
    ItemNode* empty = swd3::AddItem(list, ItemNode::kEmpty, 7, ItemCountMode::Count, database);
    CHECK_EQ(empty->count, 1);
    swd3::AddItem(list, ItemNode::kEmpty, 7, ItemCountMode::Count, database);
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
    ItemNode* plain = swd3::AddFlaggedItem(list, 1, 4, 0, database);
    ItemNode* flagged = swd3::AddFlaggedItem(list, 1, 2, 1, database);
    CHECK(plain != flagged);
    CHECK_EQ(Length(list), 2);
    CHECK_EQ(flagged->count, 2);
    CHECK(swd3::AddFlaggedItem(list, 1, -3, 0, database) == plain);
    CHECK_EQ(plain->count, 3);
    CHECK_EQ(Length(list), 1);
    CHECK(swd3::AddFlaggedItem(list, 1, -3, 0, database) == nullptr);
    CHECK(list == nullptr);
}

TEST_CASE("item list: merge, reverse copy, unlink, lookups")
{
    ItemLink list;
    std::array<swd3::u16, 4> ids = {0x4005, 0x0006, 0x8007, 0x0008};

    for (const swd3::u16 id : ids)
    {
        std::unique_ptr<ItemNode> node = swd3::NewEmptyItem();
        node->id = id;
        node->count = 60;
        node->pending = 50;
        node->record.data.kind = ItemNode::kPendingKind | 1;
        node->next = std::move(list);
        list = std::move(node);
    }

    CHECK_EQ(list->count, 60);
    swd3::MergePendingCounts(list);
    CHECK_EQ(list->count, 99);
    CHECK_EQ(list->pending, 0);
    CHECK_EQ(list->record.data.kind, 1u);
    ItemLink copy = swd3::CopyItemList(list);
    CHECK_EQ(copy->id, 0x4005);
    CHECK(copy->record.description.has_value());
    CHECK_EQ(swd3::ItemAt(list, 2)->id, 0x0006);
    CHECK(swd3::ItemAt(list, 9) == nullptr);
    CHECK_EQ(swd3::FindItemOfKind(list, 7)->id, 0x8007);
    CHECK_EQ(swd3::FindItemOfKind(list, 5)->id, 0x4005);
    std::unique_ptr<ItemNode> taken = swd3::UnlinkItem(list, 0x0006);
    CHECK(taken != nullptr);
    CHECK(taken->next == nullptr);
    CHECK_EQ(Length(list), 3);
    CHECK(swd3::UnlinkItem(list, 0x0006) == nullptr);
    std::array<std::unique_ptr<ItemNode>, 3> slots = {swd3::NewEmptyItem(), nullptr, std::move(taken)};
    CHECK(swd3::FindRoleItem(slots, 6) == slots[2].get());
    CHECK(swd3::FindRoleItem(slots, 9) == nullptr);
    CHECK(swd3::NewEmptyItem()->record.data.name[0] == '\265');
}
