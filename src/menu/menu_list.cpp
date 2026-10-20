#include "menu/menu_list.h"

#include "config/config.h"
#include "core/debug_log.h"
#include "crt/msvc.h"
#include "game/databases.h"
#include "game/game.h"
#include "menu/menu.h"
#include "platform/display.h"

namespace swd3
{

namespace
{

constexpr i32 kItemsBackground = 0x43;  // Menu::background action of the items page
constexpr int kItemRows = 0xD;
constexpr u16 kSaveItem = 0x2D9;
constexpr u8 kUnkSettings = 0x64;  // written as unk14 by the settings
constexpr int kBarBoxLeft = 4;     // DrawScrollBar
constexpr int kBarBoxUp = 0xC;
constexpr int kBarBoxWidth = 8;
constexpr int kBarBoxExtra = 0x18;
constexpr int kBarBoxMode = 4;
constexpr int kBarClipWidth = 0x20;
constexpr int kThumbLeft = 3;
constexpr int kArrowHeight = 0x10;
constexpr i32 kUpArrowAction = 0x1A;
constexpr i32 kDownArrowAction = 0x1B;
constexpr i32 kUpPressedAction = 0x1E;
constexpr i32 kDownPressedAction = 0x1F;
constexpr int kUpPressed = 1;
constexpr int kDownPressed = 2;

}  // namespace

// `node` inserted into `list` before the first node of an id not below its own that follows one below it (inline in
// 0x0043F0D0, 0x0043F160 and 0x00441F70; the head counts as id 0)
void InsertItemById(ItemLink& list, std::unique_ptr<ItemNode> node)
{
    ItemLink* link = &list;
    u16 previous = 0;

    while (*link != nullptr && !((*link)->id >= node->id && previous < node->id))
    {
        previous = (*link)->id;
        link = &(*link)->next;
    }

    node->next = std::move(*link);
    *link = std::move(node);
}

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

// swd32.exe 0x0043BB40: a row past the rows shown goes back to the last row and the list scrolls by one (while
// entries remain)
void FitListRow(i32 count, i32& top, i32& row, i32 shown)
{
    if (row < shown)
    {
        return;
    }

    row = 0;

    if (shown >= 1)
    {
        row = shown - 1;
    }

    if (top + shown < count)
    {
        ++top;
    }
}

// swd32.exe 0x0043BB80: the next row of a list; past the rows shown it scrolls by one (while entries remain)
void ListDown(i32 count, i32& top, i32& row, i32 shown)
{
    ++row;
    FitListRow(count, top, row, shown);
}

// swd32.exe 0x0043BBC0: the row before; before the first it scrolls back by one
void ListUp(i32& top, i32& row)
{
    if (--row < 0)
    {
        row = 0;

        if (top > 0)
        {
            --top;
        }
    }
}

// swd32.exe 0x0043BBE0: Page Down: to the last row shown, from there a page of `page` further (to the last page,
// where the rows shown shrink to the entries left)
void ListPageDown(i32 count, i32& top, i32& row, i32& shown, i32 page)
{
    if (row != shown - 1)
    {
        row = 0;

        if (shown >= 1)
        {
            row = shown - 1;
        }

        return;
    }

    top += page;

    if (top + page < count)
    {
        if (row > count - top - 1)
        {
            row = count - top - 1;
        }

        return;
    }

    top = count - page;

    if (top < 0)
    {
        top = 0;
    }

    shown = count - top;
    row = shown - 1;
}

// swd32.exe 0x0043BC60: Page Up: to the first row shown, from there a page of `page` back (to the start)
void ListPageUp(i32& top, i32& row, i32 page)
{
    if (row != 0)
    {
        row = 0;
        return;
    }

    top -= page;

    if (top < 0)
    {
        row = 0;
        top = 0;
    }
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

// swd32.exe 0x0044B010: the options page left: its list freed and the settings written (to [game], Env.dat in the
// original)
void Menu::OptionsLeave()
{
    this->options.notes.clear();
    this->game.config.Update(this->CurrentSettings());
    this->storedSettings.reset();
}

// The settings the settings pages write (with unk14 as they write it).
Config::Settings Menu::CurrentSettings() const
{
    const Game& owner = this->game;
    return Config::Settings{
        static_cast<u8>(owner.config.soundVolume),
        static_cast<u8>(owner.config.musicVolume),
        static_cast<u8>(owner.config.cacheSize),
        kUnkSettings,
        static_cast<u8>(owner.state.GameFlag(GameState::PlainScreen) ? 1 : 0),
        static_cast<u8>(owner.config.textSpeed),
        static_cast<u8>(owner.config.battleSpeed),
    };
}

// The rebuild (deviations.md): a settings page writes a setting once it changed (the original only when the page is
// left); the first call of a page notes what is set.
void Menu::StoreChangedSettings()
{
    const Config::Settings settings = this->CurrentSettings();

    if (this->storedSettings.has_value() && *this->storedSettings != settings)
    {
        this->game.config.Update(settings);
    }

    this->storedSettings = settings;
}

// swd32.exe 0x0043AE40: the scroll bar of a list from `y` down `height` pixels at `x`: its box, the bar picture (act
// unk4FC328) repeated down it, the thumb (act unk4FC6E8) from `from` to `to` of the height, the arrows above and below
// (pressed when bit 0 or bit 1 of `arrows` is set). `bar` keeps the edges for the clicks.
void Menu::DrawScrollBar(int x, int y, int height, int arrows, float from, float to, MenuScrollBar& bar)
{
    Game& owner = this->game;
    this->DrawBox(x + kBarBoxLeft, y - kBarBoxUp, kBarBoxWidth, height + kBarBoxExtra, kBarBoxMode, 0, 0, 0);
    const int bottom = y + height;
    owner.painter.SetClipClamped(x, y, x + kBarClipWidth, y + height);
    bar.top = y;
    bar.bottom = y + height;

    // A failed act is reported and still drawn with the picture it had (format 0x0049FD0C).
    auto tile = [&owner](ActState& act, std::u8string_view failed, int left, int from, int to)
    {
        if (owner.acts.Run(act) == 0)
        {
            DebugLog(std::u8string(failed) + u8" 設定錯誤 \nact[%d] qq[%d] dir[%d]", act.id, act.action, act.direction);
        }

        const Picture* picture = owner.archive.GetPicture(act.frame, act.picture);

        if (picture == nullptr || picture->height == 0)
        {
            return;
        }

        for (int place = from; place < to; place += picture->height)
        {
            owner.painter.SetSource(picture->data.data());
            owner.painter.DrawPicture(left, place, picture->width, picture->height, act.flags);
        }
    };

    tile(this->unk4FC328, u8"Act Err(Menu:DrawBar)", x, y, bottom);
    const auto length = static_cast<double>(height);
    const int thumbTop = y + static_cast<i32>(crt::Ftol(length * static_cast<double>(from)));
    bar.thumbTop = thumbTop;
    const int thumbBottom = y + static_cast<i32>(crt::Ftol(length * static_cast<double>(to)));
    bar.thumbBottom = thumbBottom;
    owner.painter.SetClipClamped(x, y, x + kBarClipWidth, thumbBottom);
    tile(this->unk4FC6E8, u8"Act Err(Menu:Prop:Render:bar)", x + kThumbLeft, thumbTop, thumbBottom);
    owner.painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->unk4FBA18.action = kUpArrowAction;
    owner.actDrawer.Draw(this->unk4FBA18, x, y - kArrowHeight);
    this->unk4FB8D8.action = kDownArrowAction;
    owner.actDrawer.Draw(this->unk4FB8D8, x, bottom);

    if ((arrows & kUpPressed) != 0)
    {
        this->unk4FBA18.action = kUpPressedAction;
        owner.actDrawer.Draw(this->unk4FBA18, x, y - kArrowHeight);
    }

    if ((arrows & kDownPressed) != 0)
    {
        this->unk4FB8D8.action = kDownPressedAction;
        owner.actDrawer.Draw(this->unk4FB8D8, x, bottom);
    }
}
}  // namespace swd3
