#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

// LST .data:0053C198..0053C497; the following word begins at 0053C498.
using LegacyBattleMusicPath = std::array<compat::u8, 0x300U>;

}  // namespace openswd3::battle
