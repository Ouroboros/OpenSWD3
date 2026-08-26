#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleFinalActorStepState {
    std::array<compat::u32, 10> group_a_completion_flags{};
    std::array<compat::u32, 10> group_a_slot_values{};
    std::array<compat::u32, 10> actor_order{};
    std::array<std::array<compat::u32, 5>, 10> actor_runtime_records{};

    compat::u8 removed_group_a_count{};
    compat::u16 excluded_group_a_count{};
    compat::u32 queued_actor_code{};
    compat::u32 active_actor_code{0xFFFFFFFFU};
    compat::u32 secondary_actor_code{};
    compat::u32 published_actor_code{};

    compat::u32 action_execution_active{};
    compat::u32 terminal_mode{};
    compat::u32 frame_gate_a{};
    compat::u32 frame_gate_b{};
    compat::u32 selection_gate{};
    compat::u32 auxiliary_gate{};
    compat::u32 terminal_latch{};

    compat::u16 coordinate_x{};
    compat::u16 coordinate_y{};
    compat::u16 action_delay{};
    compat::u16 group_b_reset_word{};
    compat::u32 actor_descriptor_token{};
};

// Typed closure of legacy 0x0045AA00. The group selector is compared as a
// complete dword; only one selects group A and every other value selects B.
[[nodiscard]] LegacyBattleActionDispatchResult
advance_legacy_battle_final_actor_step(
    LegacyBattleFinalActorStepState& state,
    LegacyBattleActionDispatchState& action,
    LegacyBattleActionDispatchPort& port,
    compat::u32 actor_index,
    compat::u32 actor_group
);

}  // namespace openswd3::battle
