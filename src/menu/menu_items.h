#pragma once

#include <cstdint>

// Item lists of the equipment, shop and item menus.
namespace swd3 {

void MenuToggleOption();
void MenuToggleOptionQuiet();
void ReturnPickedItems();
void SortPickedItems();
bool ItemFitsMenu(const std::uint8_t* item, int mode);
void ComputeEquipmentBonus();
void FreeShopBuffers();
void FilterItemsByKind(std::uint8_t** source, std::uint8_t* target, int kind);
void CountListD64();
void ReturnListD70();
int DrawMenuPicture(int id, int frame, int x, int y);
std::int32_t* CopyHeroStats(std::int32_t* stats);
void FilterItemsByType(std::uint8_t** source, std::uint8_t* target, int type);
void CountListFBC();
void ReturnListFC0();
void CountStatusPages();
int ReturnThree();
void MenuPreviousPage();
void MenuNextPage();
void InitNumberList();
void CopyItemsForMenu(int mode);
void FreeRecordBuffers();
int OpenRecordsPage();

}  // namespace swd3
