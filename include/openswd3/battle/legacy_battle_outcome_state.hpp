#pragma once

#include "openswd3/battle/legacy_battle_full_frame_darkening.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleOutcomeResolutionState {
    compat::u32 resolution_latch{};
    compat::u32 darkening_gate{};
    compat::u32 force_group_b_resolution{};
    LegacyBattleFullFrameDarkeningState darkening{};
};

class LegacyBattleOutcomeResolutionStatePort {
public:
    [[nodiscard]] virtual LegacyBattleOutcomeResolutionState&
    outcome_resolution_state() noexcept {
        return outcome_resolution_state_;
    }

    [[nodiscard]] virtual const LegacyBattleOutcomeResolutionState&
    outcome_resolution_state() const noexcept {
        return outcome_resolution_state_;
    }

protected:
    LegacyBattleOutcomeResolutionStatePort() = default;
    ~LegacyBattleOutcomeResolutionStatePort() = default;

private:
    LegacyBattleOutcomeResolutionState outcome_resolution_state_{};
};

}  // namespace openswd3::battle
