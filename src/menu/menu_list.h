#pragma once

#include <string>
#include <vector>

#include "core/types.h"
#include "item/item_list.h"

namespace swd3
{

class Database;

// The view of a scrolled object list of a menu page (for the items page the counts at 0x004FD08C, 0x004FD164,
// 0x004FD098, 0x004FD298 and the first node shown at 0x004FD150).
struct MenuList
{
    i32 count = 0;              // the objects of the list
    i32 top = 0;                // the first one shown
    i32 row = 0;                // the chosen one, counted from `top`
    i32 shown = 0;              // the rows shown
    ItemNode* first = nullptr;  // the node at `top`
};

int CountItems(const ItemLink& list);
void SkipItems(int count, const ItemLink& list, ItemNode*& node);
ItemNode* ItemAfter(int count, const ItemLink& list);
ItemNode* CountShown(ItemNode* node, i32& shown, int rows);
std::string FitMenuList(MenuList& view, ItemLink& list, int rows, Database& database, const std::vector<u8>& mapsData);
void InsertItemById(ItemLink& list, std::unique_ptr<ItemNode> node);
void FitListRow(i32 count, i32& top, i32& row, i32 shown);
void ListDown(i32 count, i32& top, i32& row, i32 shown);
void ListUp(i32& top, i32& row);
void ListPageDown(i32 count, i32& top, i32& row, i32& shown, i32 page);
void ListPageUp(i32& top, i32& row, i32 page);

}  // namespace swd3
