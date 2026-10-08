#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleScreenFlashState {
    compat::u32 active{};       // 0x0053BFCC
    compat::u8 intensity{16U};  // 0x004A75FE, original static initializer
};

class LegacyBattleScreenFlashStatePort {
public:
    [[nodiscard]] virtual LegacyBattleScreenFlashState&
    screen_flash_state() noexcept {
        return screen_flash_state_;
    }

    [[nodiscard]] virtual const LegacyBattleScreenFlashState&
    screen_flash_state() const noexcept {
        return screen_flash_state_;
    }

protected:
    LegacyBattleScreenFlashStatePort() = default;
    ~LegacyBattleScreenFlashStatePort() = default;

private:
    LegacyBattleScreenFlashState screen_flash_state_{};
};

}  // namespace openswd3::battle
