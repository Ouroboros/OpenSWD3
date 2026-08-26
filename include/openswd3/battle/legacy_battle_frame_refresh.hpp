#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
class LegacyBattleEffectCallPort;

struct LegacyBattleFrameRefreshState {
    std::array<compat::u32, 2> surface_tokens{};
    compat::u16 snapshot_word_36{};
    compat::u16 snapshot_word_38{};
    compat::u16 snapshot_word_3a{};

    compat::u32 source_pitch{};
    compat::u32 viewport_token{};
    compat::u32 final_surface_token{};
    compat::u32 last_lock_token{};
    compat::u32 captured_pitch{};
    compat::u32 active_surface_token{};
    compat::u16 refresh_pending{};

    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

class LegacyBattleFrameRefreshStatePort {
public:
    [[nodiscard]] LegacyBattleFrameRefreshState&
    frame_refresh_state() noexcept {
        return frame_refresh_state_;
    }

    [[nodiscard]] const LegacyBattleFrameRefreshState&
    frame_refresh_state() const noexcept {
        return frame_refresh_state_;
    }

protected:
    LegacyBattleFrameRefreshStatePort() = default;
    ~LegacyBattleFrameRefreshStatePort() = default;

private:
    LegacyBattleFrameRefreshState frame_refresh_state_{};
};

struct LegacyBattleFrameRefreshResult {
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 port_calls{};
    compat::u32 surface_iterations{};
    bool refreshed{};
};

[[nodiscard]] LegacyBattleFrameRefreshResult refresh_legacy_battle_frame(
    LegacyBattleActionDispatchPort& port,
    compat::u16 current_word_36,
    compat::u16 current_word_38,
    compat::u16 current_word_3a
);

[[nodiscard]] LegacyBattleFrameRefreshResult refresh_legacy_battle_frame(
    LegacyBattleEffectCallPort& port,
    compat::u16 current_word_36,
    compat::u16 current_word_38,
    compat::u16 current_word_3a
);

}  // namespace openswd3::battle
