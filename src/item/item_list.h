#pragma once

#include <cstdint>

// Item lists: nodes of 0xB0 bytes (+0 next, +4 id, +8 count, +0xA pending count, +0xC name, +0x2C kind,
// +0xAC description) and the item slots of the heroes.
namespace swd3 {

void CountVisibleEntries();
std::uint32_t* AdvanceByWords(int count, std::uint32_t base, std::uint32_t* out);
void MergePendingCounts(std::uint8_t** list);
std::uint8_t* CopyItemList(std::uint8_t** list);
void FreeItemList(std::uint8_t** list);
std::uint8_t* NewEmptyItem();
std::uint8_t* UnlinkItem(std::uint8_t** list, std::int16_t id);
std::uint8_t* FindHeroItem(std::int16_t id);
std::uint8_t* FindItemOfKind(std::uint8_t** list, std::int16_t id);
std::uint8_t* ItemAt(std::uint8_t** list, std::uint16_t index);

}  // namespace swd3
