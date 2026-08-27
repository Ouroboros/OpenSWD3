#pragma once

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

namespace openswd3::battle {

struct LegacyBattleMenuPageAdvanceBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleMenuPageAdvanceRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleMenuPageAdvanceStatus : compat::u8 {
    completed,
    equipment_selection_typed_stop,
    equipment_scroll_typed_stop,
};

struct LegacyBattleMenuPageAdvanceResult {
    LegacyBattleMenuPageAdvanceStatus status{
        LegacyBattleMenuPageAdvanceStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 sample_calls{};
};

// Typed closure of legacy 0x00461A30.
[[nodiscard]] LegacyBattleMenuPageAdvanceResult advance_legacy_battle_menu_page(
    LegacyBattleMenuPageAdvanceBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuPageAdvanceRequest& request
);

}  // namespace openswd3::battle
