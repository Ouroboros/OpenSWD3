#pragma once

#include <memory>
#include <span>

#include "core/types.h"
#include "org/object_record.h"

namespace swd3
{

class Database;

// An item or skill of a list (0xB0 bytes in the original): the mon.dat object with its counts.
struct ItemNode
{
    // The id of the empty entry ("無") heading every skill list and filling empty equipment slots.
    static constexpr u16 Empty = 0xFFDC;
    // ObjectData::kind bit of a node holding a pending count.
    static constexpr u32 PendingKind = 0x8000;
    static constexpr i16 MaxCount = 0x63;

    std::unique_ptr<ItemNode> next;  // +0x00
    u16 id = 0;                      // +0x04
    i16 unk06 = 0;                   // +0x06
    i16 count = 0;                   // +0x08
    i16 pending = 0;                 // +0x0A
    ObjectRecord record;             // +0x0C (the description at +0xAC)
};

// Where the first node of a list is kept: a list head, or the next of the node a list starts after (a skill list is
// passed as the next of its "無" entry, so its first entry is skipped).
using ItemLink = std::unique_ptr<ItemNode>;

// The ways AddItem changes an entry (0x0044D2D0).
enum class ItemCountMode : i16
{
    Pending = 0,
    Count = 1,
    PendingOnly = 2,
};

void MergePendingCounts(ItemLink& list);
ItemLink CopyItemList(const ItemLink& list);
void FreeItemList(ItemLink& list);
std::unique_ptr<ItemNode> NewEmptyItem();
std::unique_ptr<ItemNode> UnlinkItem(ItemLink& list, i16 id);
ItemNode* FindRoleItem(std::span<const std::unique_ptr<ItemNode>> slots, i16 id);
ItemNode* FindItemOfKind(const ItemLink& list, i16 id);
ItemNode* ItemAt(const ItemLink& list, u16 index);
ItemNode* AddFlaggedItem(ItemLink& list, u16 id, i16 amount, i16 flagged, Database& database);
ItemNode* AddItem(ItemLink& list, u16 id, i16 amount, ItemCountMode mode, Database& database);
void SortItems(ItemLink& list);
void SortSkills(std::span<std::unique_ptr<ItemNode>> heads);

}  // namespace swd3
