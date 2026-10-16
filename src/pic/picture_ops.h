#pragma once

#include <vector>

#include "core/types.h"

namespace swd3
{

struct Picture;

// The directions of ShiftPicture.
enum class ShiftMode
{
    Up = 0,
    Down = 1,
    Left = 2,
    Right = 3,
};

int PictureHitTest(const Picture& picture, int x, int y, int originX, int originY);
void ShiftPicture(std::vector<u8>& data, ShiftMode mode, int amount);

}  // namespace swd3
