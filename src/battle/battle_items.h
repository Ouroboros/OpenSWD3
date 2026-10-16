#pragma once

#include <cstdint>

// Item lists of the battle and shop screens and the battle work surfaces.
namespace swd3 {

int TotalItemValue();
void CountListB();
void SortBattleItems(std::uint8_t** source);
void FreeBattleItems();
int CountItemTotal(int id);
std::uint32_t ItemPriority(std::uint32_t kind);
int SetGaugeFull(int speed);
int CreateBattleSurfaces();
void ReleaseBattleSurfaces();

}  // namespace swd3
