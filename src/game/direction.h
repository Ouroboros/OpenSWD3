#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

// sin(degree) * 8192 for 0..89 degrees (swd32.exe 0x0049F250; the table goes on, only these entries are read).
constexpr std::array<i16, 90> kSineTable = {
    0,    142,  285,  428,  571,  713,  856,  998,  1140, 1281, 1422, 1563, 1703, 1842, 1981, 2120, 2258, 2395,
    2531, 2667, 2801, 2935, 3068, 3200, 3331, 3462, 3591, 3719, 3845, 3971, 4096, 4219, 4341, 4461, 4580, 4698,
    4815, 4930, 5043, 5155, 5265, 5374, 5481, 5586, 5690, 5792, 5892, 5991, 6087, 6182, 6275, 6366, 6455, 6542,
    6627, 6710, 6791, 6870, 6947, 7021, 7094, 7164, 7233, 7299, 7362, 7424, 7483, 7540, 7595, 7647, 7697, 7745,
    7791, 7834, 7874, 7912, 7948, 7982, 8012, 8041, 8067, 8091, 8112, 8130, 8147, 8160, 8172, 8180, 8187, 8190,
};

int AngleBetween(int fromX, int fromY, int toX, int toY, i32& distance);
int AngleToDirection(int angle);

}  // namespace swd3
