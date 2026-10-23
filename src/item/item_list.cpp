#include "item/item_list.h"

#include <algorithm>
#include <cstring>

#include "core/debug_log.h"
#include "org/database.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

constexpr u16 kKindIdMask = 0x3FFF;  // an item id without its two top bits

// "無" (swd32.exe 0x0049F9FC).
constexpr std::u8string_view kEmptyName = u8"無";

bool Pending(const ItemNode& node)
{
    return (node.record.data.kind & ItemNode::PendingKind) != 0;
}

// strncpy: the name, then NULs up to the size
void SetName(ItemNode& node, std::string_view name)
{
    auto& field = node.record.data.name;
    field.fill('\0');
    std::copy_n(name.begin(), std::min(name.size(), field.size()), field.begin());
}

// The link holding the first node `matches` accepts, or the link past the end.
template <typename Match>
ItemLink* FindLink(ItemLink& list, Match matches)
{
    ItemLink* link = &list;

    while (*link != nullptr && !matches(**link))
    {
        link = &(*link)->next;
    }

    return link;
}

void Remove(ItemLink& link)
{
    std::unique_ptr<ItemNode> node = std::move(link);
    link = std::move(node->next);
}

void InsertHead(ItemLink& list, std::unique_ptr<ItemNode> node)
{
    node->next = std::move(list);
    list = std::move(node);
}

// A new node of `id` (the empty entry, or the object of mon.dat); null when mon.dat has no such object (logged as in
// 0x0044D0F0 and 0x0044D2D0).
std::unique_ptr<ItemNode> NewNode(u16 id, Database& database)
{
    auto node = std::make_unique<ItemNode>();

    if (id == ItemNode::Empty)
    {
        SetName(*node, Big5(kEmptyName));
    }
    else if (!database.LoadObject(node->record, id))
    {
        DebugLog(u8"增加道具 沒有定義物品號碼(%d)", id);  // 0x004A73C0 + 0x004A6BA0
        return nullptr;
    }

    node->id = id;
    return node;
}

}  // namespace

// swd32.exe 0x0044D4E0: adds the pending counts (at most 99 in all) and clears the pending kind bit
void MergePendingCounts(ItemLink& list)
{
    for (ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        node->record.data.kind &= ~ItemNode::PendingKind;
        node->count = static_cast<i16>(node->count + node->pending);
        node->pending = 0;

        if (node->count > ItemNode::MaxCount)
        {
            node->count = ItemNode::MaxCount;
        }
    }
}

// swd32.exe 0x0044D520: a copy of the list in reverse order, with copies of the descriptions (null for an empty list).
// A node without a description gets an uninitialised one-byte copy in the original; here an empty one.
ItemLink CopyItemList(const ItemLink& list)
{
    ItemLink copy;

    for (const ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        auto duplicate = std::make_unique<ItemNode>();
        duplicate->id = node->id;
        duplicate->workCount = node->workCount;
        duplicate->count = node->count;
        duplicate->pending = node->pending;
        duplicate->record.data = node->record.data;
        duplicate->record.description = node->record.description.value_or(std::string());
        InsertHead(copy, std::move(duplicate));
    }

    return copy;
}

// swd32.exe 0x0044D5A0
void FreeItemList(ItemLink& list)
{
    while (list != nullptr)
    {
        Remove(list);
    }
}

// swd32.exe 0x0044D5D0: the empty entry "無" (id 0xFFDC, count 1)
std::unique_ptr<ItemNode> NewEmptyItem()
{
    auto node = std::make_unique<ItemNode>();
    node->id = ItemNode::Empty;
    node->count = 1;
    SetName(*node, Big5(kEmptyName));
    return node;
}

// swd32.exe 0x0044D620: unlinks the node `id` (null when it is missing)
std::unique_ptr<ItemNode> UnlinkItem(ItemLink& list, i16 id)
{
    ItemLink* link = FindLink(
        list,
        [id](const ItemNode& node)
        {
            return static_cast<i16>(node.id) == id;
        }
    );

    if (*link == nullptr)
    {
        return nullptr;
    }

    std::unique_ptr<ItemNode> node = std::move(*link);
    *link = std::move(node->next);
    return node;
}

// swd32.exe 0x0044D650: the first equipment slot holding `id`
ItemNode* FindRoleItem(std::span<const std::unique_ptr<ItemNode>> slots, i16 id)
{
    for (const std::unique_ptr<ItemNode>& slot : slots)
    {
        if (slot != nullptr && static_cast<i16>(slot->id) == id)
        {
            return slot.get();
        }
    }

    return nullptr;
}

// swd32.exe 0x0044D680: the first node whose id without its two top bits is `id`
ItemNode* FindItemOfKind(const ItemLink& list, i16 id)
{
    for (ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        if ((node->id & kKindIdMask) == static_cast<u16>(id))
        {
            return node;
        }
    }

    return nullptr;
}

// swd32.exe 0x0044D6B0
ItemNode* ItemAt(const ItemLink& list, u16 index)
{
    ItemNode* node = list.get();

    for (u16 position = 0; node != nullptr && position != index; ++position)
    {
        node = node->next.get();
    }

    return node;
}

// swd32.exe 0x0044D0F0: changes the count of `id` by `amount`. The nodes of a flagged entry (the pending kind bit)
// and of a plain one are kept apart: a flagged addition or any removal takes the flagged node first, and a removal
// larger than it goes on with the plain node. A count reaching 0 removes the node; a missing entry is created for a
// positive amount (flagged when `flagged` is 1). The empty entry always keeps a count of 1.
ItemNode* AddFlaggedItem(ItemLink& list, u16 id, i16 amount, i16 flagged, Database& database)
{
    i16 remaining = amount;

    if (flagged == 1 || amount < 0)
    {
        ItemLink* link = FindLink(
            list,
            [id](const ItemNode& node)
            {
                return node.id == id && Pending(node);
            }
        );

        if (*link != nullptr)
        {
            ItemNode& node = **link;
            node.count = static_cast<i16>(node.count + amount);

            if (node.count > ItemNode::MaxCount)
            {
                node.count = ItemNode::MaxCount;
            }

            remaining = node.count;

            if (remaining > 0)
            {
                if (node.id == ItemNode::Empty)
                {
                    node.count = 1;
                }

                return &node;
            }

            Remove(*link);

            if (remaining >= 0)
            {
                return nullptr;
            }
        }
    }

    if (flagged == 0 || remaining < 0)
    {
        ItemLink* link = FindLink(
            list,
            [id](const ItemNode& node)
            {
                return node.id == id && !Pending(node);
            }
        );

        if (*link != nullptr)
        {
            ItemNode& node = **link;
            node.count = static_cast<i16>(node.count + remaining);

            if (node.count > ItemNode::MaxCount)
            {
                node.count = ItemNode::MaxCount;
            }

            if (node.count <= 0)
            {
                Remove(*link);
                return nullptr;
            }

            if (node.id == ItemNode::Empty)
            {
                node.count = 1;
            }

            return &node;
        }

        if (remaining < 0)
        {
            return nullptr;
        }
    }

    std::unique_ptr<ItemNode> node = NewNode(id, database);

    if (node == nullptr)
    {
        return nullptr;
    }

    if (id == ItemNode::Empty)
    {
        remaining = 1;
    }

    if (flagged == 1)
    {
        node->record.data.kind |= ItemNode::PendingKind;
    }

    node->count = remaining;
    InsertHead(list, std::move(node));
    return list.get();
}

// swd32.exe 0x0044D2D0: changes the count (mode 1) or the pending count (modes 0 and 2) of `id` by `amount`, each at
// most 99 (reaching it clears the other). A pending count dropping to 0 or below is moved into the count with mode 0
// and just cleared with mode 2; the node is removed once both are used up. A missing entry is created for a positive
// amount (its pending count also sets the pending kind bit); other modes insert it without counts. The empty entry
// always keeps a count of 1.
ItemNode* AddItem(ItemLink& list, u16 id, i16 amount, ItemCountMode mode, Database& database)
{
    ItemLink* link = FindLink(
        list,
        [id](const ItemNode& node)
        {
            return node.id == id;
        }
    );

    if (*link != nullptr)
    {
        ItemNode& node = **link;

        if (mode == ItemCountMode::Count)
        {
            node.count = static_cast<i16>(node.count + amount);

            if (node.count > ItemNode::MaxCount)
            {
                node.count = ItemNode::MaxCount;
                node.pending = 0;
                return &node;
            }

            if (node.count <= 0)
            {
                const bool none = node.pending <= 0;
                node.count = 0;

                if (none)
                {
                    Remove(*link);
                    return nullptr;
                }
            }
        }
        else if (mode == ItemCountMode::Pending)
        {
            node.pending = static_cast<i16>(node.pending + amount);

            if (node.pending > ItemNode::MaxCount)
            {
                node.pending = ItemNode::MaxCount;
                node.count = 0;
                return &node;
            }

            if (node.pending <= 0)
            {
                const i16 left = node.pending;
                node.pending = 0;
                node.count = static_cast<i16>(node.count + left);

                if (node.count <= 0)
                {
                    Remove(*link);
                    return nullptr;
                }
            }
        }
        else if (mode == ItemCountMode::PendingOnly)
        {
            node.pending = static_cast<i16>(node.pending + amount);

            if (node.pending > ItemNode::MaxCount)
            {
                node.pending = ItemNode::MaxCount;
                node.count = 0;
                return &node;
            }

            if (node.pending <= 0)
            {
                const bool none = node.count == 0;
                node.pending = 0;

                if (none)
                {
                    Remove(*link);
                    return nullptr;
                }
            }
        }

        if (node.id == ItemNode::Empty)
        {
            node.count = 1;
            node.pending = 0;
        }

        return &node;
    }

    if (amount <= 0)
    {
        return nullptr;
    }

    std::unique_ptr<ItemNode> node = NewNode(id, database);

    if (node == nullptr)
    {
        return nullptr;
    }

    const i16 added = id == ItemNode::Empty ? i16{1} : amount;

    if (mode == ItemCountMode::Pending || mode == ItemCountMode::PendingOnly)
    {
        node->pending = added;
        node->count = 0;
        node->record.data.kind |= ItemNode::PendingKind;
    }
    else if (mode == ItemCountMode::Count)
    {
        node->count = added;
        node->pending = 0;
    }

    InsertHead(list, std::move(node));
    return list.get();
}

// swd32.exe 0x0045D180: one of item `id` added (to the pending count when `pending` is 1) unless the entry holds
// MaxCount in all; a new entry goes first, its object loaded from mon.dat even when it has none. The record of the
// entry is returned. The original takes the inventory list directly; its head is a static node (0x004A9940) whose
// record (0x004A994C, all zero) id 0 returns (0x0045D18A): nullptr here, read by the callers as an empty name.
ObjectRecord* AddOneItem(ItemLink& list, u16 id, int pending, Database& database)
{
    if (id == 0)
    {
        return nullptr;
    }

    for (ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        if (node->id != id)
        {
            continue;
        }

        if (node->pending + node->count < ItemNode::MaxCount)
        {
            if (pending == 1)
            {
                ++node->pending;
            }
            else
            {
                ++node->count;
            }
        }

        return &node->record;
    }

    auto node = std::make_unique<ItemNode>();
    node->id = id;
    database.LoadObject(node->record, id);

    if (pending == 1)
    {
        node->pending = 1;
        node->record.data.kind |= ItemNode::PendingKind;
    }
    else
    {
        node->count = 1;
    }

    InsertHead(list, std::move(node));
    return &list->record;
}

namespace
{

// The node at `link` swapped with the node after it.
void SwapWithNext(ItemLink& link)
{
    std::unique_ptr<ItemNode> first = std::move(link);
    std::unique_ptr<ItemNode> second = std::move(first->next);
    first->next = std::move(second->next);
    second->next = std::move(first);
    link = std::move(second);
}

}  // namespace

// swd32.exe 0x0045D250: the items sorted by id: a pair out of order is swapped and the pass starts over from the head;
// every node passed has its +6 word cleared (the last node keeps it)
void SortItems(ItemLink& list)
{
    if (list == nullptr || list->next == nullptr)
    {
        return;
    }

    ItemLink* link = &list;

    while ((*link)->next != nullptr)
    {
        ItemNode& node = **link;
        node.workCount = 0;

        if (node.id <= node.next->id)
        {
            link = &node.next;
        }
        else
        {
            SwapWithNext(*link);
            link = &list;
        }
    }
}

// swd32.exe 0x0045D2A0: the skill lists (the entries after each "無" head) sorted by id the same way, nothing cleared
void SortSkills(std::span<std::unique_ptr<ItemNode>> heads)
{
    for (std::unique_ptr<ItemNode>& head : heads)
    {
        if (head == nullptr || head->next == nullptr || head->next->next == nullptr)
        {
            continue;
        }

        ItemLink& list = head->next;
        ItemLink* link = &list;

        while ((*link)->next != nullptr)
        {
            ItemNode& node = **link;

            if (node.id <= node.next->id)
            {
                link = &node.next;
            }
            else
            {
                SwapWithNext(*link);
                link = &list;
            }
        }
    }
}

}  // namespace swd3
