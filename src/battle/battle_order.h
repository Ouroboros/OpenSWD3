#pragma once

#include <cstdint>

namespace swd3 {

std::int32_t CollectUnitDepths();
int SortDrawOrder();
std::int32_t CollectEnemyDrawOrder();
std::uint8_t* SortInventory();
std::uint8_t* SortItemLists();
std::int16_t SetBattleWord(int index, std::int16_t value);
void ReleaseBattleUnits();

}  // namespace swd3
