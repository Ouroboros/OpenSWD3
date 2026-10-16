#pragma once

#include <cstdint>

#include "pic/rle.h"

namespace swd3 {

// Modes of ShiftPicture.
enum ShiftMode { kShiftUp = 0, kShiftDown = 1, kShiftLeft = 2, kShiftRight = 3 };

int PictureHitTest(const Picture* picture, int x, int y, int originX, int originY);
void ShiftPicture(std::uint16_t* data, int mode, int amount);

}  // namespace swd3
