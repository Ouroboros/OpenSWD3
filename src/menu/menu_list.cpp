#include "menu/menu_list.h"

#include "config/config.h"
#include "game/databases.h"
#include "game/game.h"
#include "menu/menu.h"

namespace swd3
{

namespace
{

constexpr i32 kItemsBackground = 0x43;  // Menu::background action of the items page
constexpr int kItemRows = 0xD;
constexpr u16 kSaveItem = 0x2D9;
constexpr u8 kUnkSettings = 0x64;  // the fourth byte written to Env.dat

}  // namespace

// swd32.exe 0x0043B980: the nodes of `list`
int CountItems(const ItemLink& list)
{
    int count = 0;

    for (const ItemNode* node = list.get(); node != nullptr; node = node->next.get())
    {
        ++count;
    }

    return count;
}

// swd32.exe 0x0043B9A0: `node` set to the node `count` places after the head of `list` (the original reads through a
// null node past the end; here it stops there)
void SkipItems(int count, const ItemLink& list, ItemNode*& node)
{
    node = list.get();

    for (int step = count; step > 0 && node != nullptr; --step)
    {
        node = node->next.get();
    }
}

// swd32.exe 0x0043B9C0: the node `count` places after the head of `list` (nullptr past the end, where the original
// reads through a null node)
ItemNode* ItemAfter(int count, const ItemLink& list)
{
    ItemNode* node = list.get();

    for (int step = count; step > 0 && node != nullptr; --step)
    {
        node = node->next.get();
    }

    return node;
}

// swd32.exe 0x0043BC90: `shown` counts the nodes from `node` up to `rows`; the node after them
ItemNode* CountShown(ItemNode* node, i32& shown, int rows)
{
    shown = 0;

    while (node != nullptr && shown < rows)
    {
        ++shown;
        node = node->next.get();
    }

    return node;
}

// swd32.exe 0x0043BCC0: the view fitted to `list` after it changed (an empty list gets the empty entry "無"; a view
// past its end moves back), its first node and rows shown (at most `rows`); the description of the chosen object
// (none past the end, where the original reads through a null node)
std::string FitMenuList(MenuList& view, ItemLink& list, int rows, Database& database, const std::vector<u8>& mapsData)
{
    const int count = CountItems(list);
    view.count = count;

    if (count == 0)
    {
        AddItem(list, ItemNode::Empty, 1, ItemCountMode::Pending, database);
    }

    if (count <= view.top + view.shown)
    {
        if (count > view.top)
        {
            if (count < view.row + view.top + 1)
            {
                view.row = count - view.top - 1;
            }
        }
        else
        {
            view.top = count - 1;

            if (view.top < 0)
            {
                view.top = 0;
            }

            view.row = 0;
        }
    }

    SkipItems(view.top, list, view.first);
    CountShown(view.first, view.shown, rows);
    const ItemNode* chosen = ItemAfter(view.row + view.top, list);
    return chosen != nullptr ? ObjectDescription(mapsData, chosen->id) : std::string();
}

// swd32.exe 0x00448650: back on the items page after its save slots: the inventory and the page's list kept by
// 0x004485F0 come back, the view is fitted (13 rows) and the chosen object described; `saved` 1 adds one less of
// object 0x2D9 to the list (inferred: the item spent on the save)
void Menu::ItemsAfterSave(int saved)
{
    Game& owner = this->game;
    this->background.action = kItemsBackground;
    FreeItemList(owner.party.inventory);
    owner.party.inventory = std::move(this->savedInventory);
    this->itemObjects = std::move(this->savedItems);
    FitMenuList(this->itemList, this->itemObjects, kItemRows, owner.database, owner.databases.mapsData);
    const ItemNode* chosen = ItemAfter(this->itemList.row + this->itemList.top, this->itemObjects);
    this->description = chosen != nullptr ? ObjectDescription(owner.databases.mapsData, chosen->id) : std::string();

    if (saved == 1)
    {
        AddItem(this->itemObjects, kSaveItem, -1, ItemCountMode::Pending, owner.database);
    }
}

// swd32.exe 0x0044B010: the options page left: its list freed and the settings written to Env.dat
void Menu::OptionsLeave()
{
    Game& owner = this->game;
    this->unk4FD318.clear();
    const Config::Settings settings{
        static_cast<u8>(owner.config.soundVolume),
        static_cast<u8>(owner.config.musicVolume),
        static_cast<u8>(owner.config.cacheSize),
        kUnkSettings,
        static_cast<u8>(owner.state.GameFlag(GameState::PlainScreen) ? 1 : 0),
        static_cast<u8>(owner.config.textSpeed),
        static_cast<u8>(owner.config.battleSpeed),
    };

    owner.config.Update(owner.config.installDirectory, owner.config.cdRoot, settings);
}

}  // namespace swd3
