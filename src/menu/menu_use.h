#pragma once

#include <cstdint>

// The item use page keys: mode 1 moves in the two column item grid (24 per screen), mode 2 picks the hero among the
// valid entries of the table at 0x004A6A38, mode 15 moves in the unlocked entry list.
namespace swd3 {

void UseKeyDown();
void UseKeyUp();
void UseKeyPageDown();
void UseKeyPageUp();
void UseKeyLeft();
void UseKeyRight();
void SwapEquipment(std::uint8_t** list, std::uint8_t* slot, int index);

}  // namespace swd3
