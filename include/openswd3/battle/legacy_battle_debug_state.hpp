#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleDebugHotkeyState {
    compat::u32 developer_tools_enabled{};
    compat::u32 toggle_5244e0{};
    compat::u32 toggle_53af68{};
    compat::u32 message_latch_53ceb8{};
    compat::u32 selection_status_word_53c050{};
    compat::u32 actor_retarget_gate_53bf64{};
    std::array<compat::u32, 6> selection_workspace_tail{};
    compat::u32 text_mode_toggle_53c02c{};
    compat::u32 battle_mode_flags_53bc24{};
    std::array<compat::u32, 10> block_53af30{};
    compat::u32 reset_gate_53bd50{};
    compat::u32 screenshot_request{};
};

class LegacyBattleDebugHotkeyStatePort {
public:
    [[nodiscard]] virtual LegacyBattleDebugHotkeyState&
    battle_debug_hotkey_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleDebugHotkeyState&
    battle_debug_hotkey_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleDebugHotkeyStatePort() = default;
    ~LegacyBattleDebugHotkeyStatePort() = default;

private:
    LegacyBattleDebugHotkeyState state_{};
};

class LegacyBattleDebugOverlayGateStatePort {
public:
    [[nodiscard]] virtual compat::u32& battle_debug_overlay_gate() noexcept {
        return gate_;
    }

    [[nodiscard]] virtual const compat::u32&
    battle_debug_overlay_gate() const noexcept {
        return gate_;
    }

protected:
    LegacyBattleDebugOverlayGateStatePort() = default;
    ~LegacyBattleDebugOverlayGateStatePort() = default;

private:
    compat::u32 gate_{};
};

}  // namespace openswd3::battle
