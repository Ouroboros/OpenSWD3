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

}  // namespace swd3
