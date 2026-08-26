#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

class LegacyBattleSharedPhaseStatePort {
public:
    [[nodiscard]] virtual compat::u32& battle_message_state() noexcept {
        return battle_message_state_;
    }

    [[nodiscard]] virtual const compat::u32&
    battle_message_state() const noexcept {
        return battle_message_state_;
    }

    [[nodiscard]] virtual compat::u32& battle_terminal_latch() noexcept {
        return battle_terminal_latch_;
    }

    [[nodiscard]] virtual const compat::u32&
    battle_terminal_latch() const noexcept {
        return battle_terminal_latch_;
    }

protected:
    LegacyBattleSharedPhaseStatePort() = default;
    ~LegacyBattleSharedPhaseStatePort() = default;

private:
    compat::u32 battle_message_state_{};
    compat::u32 battle_terminal_latch_{};
};

}  // namespace openswd3::battle
