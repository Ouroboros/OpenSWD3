#pragma once

#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"

namespace openswd3::battle {

struct LegacyBattleMenuContextRetreatBindings {
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleInputDispatchState& input_dispatch;
    compat::u32& message_state;
};

struct LegacyBattleMenuContextRetreatRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleMenuContextRetreatStatus : compat::u8 {
    completed,
    permission_typed_stop,
    equipment_selection_typed_stop,
    equipment_scroll_typed_stop,
};

struct LegacyBattleMenuContextRetreatResult {
    LegacyBattleMenuContextRetreatStatus status{
        LegacyBattleMenuContextRetreatStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 permission_reads{};
    compat::u32 equipment_selection_reads{};
    compat::u32 equipment_scroll_reads{};
    compat::u32 sample_calls{};
    compat::u32 port_calls{};
};

// Typed closure of legacy 0x00462630.
[[nodiscard]] LegacyBattleMenuContextRetreatResult
retreat_legacy_battle_menu_context(
    LegacyBattleMenuContextRetreatBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuContextRetreatRequest& request
);

}  // namespace openswd3::battle
