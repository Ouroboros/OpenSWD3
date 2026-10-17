#include "battle/battle_items.h"

#include "battle/battle_state.h"
#include "game/game_state.h"
#include "gfx/graphics.h"
#include "party/party.h"
#include "platform/display.h"
#include "platform/surface.h"

namespace swd3
{

namespace
{

constexpr int kShopRate = 0x3C;
constexpr int kFullRate = 0x64;
constexpr i32 kListBLimit = 0xD;
constexpr u32 kNoPriceKind = 0x40;
constexpr int kKindShift = 16;     // ItemPriority: the kind moved up by default
constexpr u32 kFirstKind = 0x100;  // a kind with this bit sorts first
constexpr u32 kFirstPriority = 1;
constexpr u32 kTopByteKind = 0x200;  // a kind with this bit sorts by its top byte
constexpr int kTopByteShift = 24;
constexpr u32 kLastKind = 0x400;  // a kind with this bit sorts last
constexpr u32 kLastPriority = 0x800;
constexpr int kGaugePerSpeed = 100;  // the gauge fills at kGaugePerSpeed * (kSpeeds - speed)
constexpr int kSpeeds = 20;
// The kind the sort reads for the list variable itself (taken as the node before the first position): the global at
// 0x004FD5AC, which nothing writes.
constexpr u32 kHeadKind = 0;

}  // namespace

// swd32.exe 0x0044F770: the value of the item list: price times the count (+6) of every item, 60% in mode bit 0
int TotalItemValue(const BattleState& battle)
{
    const int rate = (battle.mode & BattleState::SellMode) == BattleState::SellMode ? kShopRate : kFullRate;
    int total = 0;

    for (const ItemNode* item = battle.items.get(); item != nullptr; item = item->next.get())
    {
        total += static_cast<int>(item->record.data.price * static_cast<u16>(item->unk06) * rate) / kFullRate;
    }

    return total;
}

// swd32.exe 0x0044F7D0: the entries of listB, at most 13
void CountListB(BattleState& battle)
{
    battle.listBCount = 0;

    for (const ItemNode* entry = battle.listB.get(); entry != nullptr && battle.listBCount < kListBLimit;
         entry = entry->next.get())
    {
        ++battle.listBCount;
    }
}

// swd32.exe 0x0044FF90: the sort priority of the kind bits: by default the kind shifted up 16; bit 8 gives 1, bit 9 the
// top byte, bit 10 0x800
u32 ItemPriority(u32 kind)
{
    u32 priority = kind << kKindShift;

    if ((kind & kFirstKind) != 0)
    {
        priority = kFirstPriority;
    }

    if ((kind & kTopByteKind) != 0)
    {
        priority = kind >> kTopByteShift;
    }

    if ((kind & kLastKind) != 0)
    {
        return kLastPriority;
    }

    return priority;
}

// swd32.exe 0x0044F800: moves the items of `source` (their +6 cleared) into the item list ordered by priority: each one
// before the first node of a priority not below its own whose predecessor ranks below it, else at the end. In mode 1
// the items without a price or with kind bit 6 stay in `source`.
void SortBattleItems(BattleState& battle, ItemLink& source)
{
    battle.items.reset();
    battle.itemCursor = 0;
    ItemLink* link = &source;

    while (*link != nullptr)
    {
        ItemNode& item = **link;
        item.unk06 = 0;

        if ((battle.mode & BattleState::ListModes) == BattleState::SellMode &&
            (item.record.data.price == 0 || (item.record.data.kind & kNoPriceKind) != 0))
        {
            link = &item.next;
            continue;
        }

        const u32 own = ItemPriority(item.record.data.kind);
        ItemLink* before = &battle.items;
        u32 previous = ItemPriority(kHeadKind);

        while (*before != nullptr)
        {
            const u32 here = ItemPriority((*before)->record.data.kind);

            if (here >= own && previous < own)
            {
                break;
            }

            previous = here;
            before = &(*before)->next;
        }

        std::unique_ptr<ItemNode> moved = std::move(*link);
        *link = std::move(moved->next);
        moved->next = std::move(*before);
        *before = std::move(moved);
    }
}

// swd32.exe 0x0044F8E0
void FreeBattleItems(BattleState& battle)
{
    FreeItemList(battle.items);
}

// swd32.exe 0x0044FA40: the count (count and pending) of item `id` in the inventory; outside the shop modes also the
// counts in the slots of the roles present (game flags 0x1E..0x21)
int CountItemTotal(const BattleState& battle, const Party& party, const GameState& state, int id)
{
    int total = 0;

    for (const ItemNode* item = party.inventory.get(); item != nullptr; item = item->next.get())
    {
        if (item->id == static_cast<u32>(id))
        {
            total += item->count + item->pending;
        }
    }

    if ((battle.mode & BattleState::ListModes) != 0)
    {
        return total;
    }

    for (int role = 0; role < Party::Roles; ++role)
    {
        if (!state.GameFlag(role + Party::FirstRoleFlag))
        {
            continue;
        }

        for (int slot = 0; slot < Party::SlotsPerRole; ++slot)
        {
            const ItemNode* item = party.equipment[static_cast<std::size_t>(role * Party::SlotsPerRole + slot)].get();

            if (item != nullptr && item->id == static_cast<u32>(id))
            {
                total += item->count;
            }
        }
    }

    return total;
}

// swd32.exe 0x0044FFC0: the gauge fills at 100 * (20 - speed)
int SetGaugeFull(BattleState& battle, int speed)
{
    battle.gaugeFull = kGaugePerSpeed * (kSpeeds - speed);
    return battle.gaugeFull;
}

// swd32.exe 0x00451A90: two screen sized off-screen surfaces; the three words at 0x004A7632 become -1
int CreateBattleSurfaces(BattleState& battle, Graphics& graphics)
{
    for (std::unique_ptr<Surface>& surface : battle.surfaces)
    {
        surface = graphics.CreateOffscreen(Display::Width, Display::Height);
    }

    battle.darkCopyTint = {-1, -1, -1};
    return -1;
}

// swd32.exe 0x00451AE0
void ReleaseBattleSurfaces(BattleState& battle)
{
    for (std::unique_ptr<Surface>& surface : battle.surfaces)
    {
        surface.reset();
    }
}

}  // namespace swd3
