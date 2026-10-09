#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleScreenFlashState {
    compat::u32 active{};       // 0x0053BFCC
    compat::u8 intensity{16U};  // 0x004A75FE, original static initializer
};

}  // namespace openswd3::battle
