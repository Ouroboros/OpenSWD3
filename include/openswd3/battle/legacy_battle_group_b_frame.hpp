#pragma once

#include "openswd3/battle/legacy_battle_group_a_frame.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleGroupBFrameState {
    LegacyBattleGroupAFrameState shared{};

    compat::u32 frame_enabled{};
    std::array<compat::u32, 8> post_update_gate{};
    compat::u32 update_gate_argument{};

    compat::u32 selection_initialized{};
    compat::u32 phase_mode{};
    compat::u32 phase_progress{};
    compat::u32 random_target_index{};
    std::array<compat::u16, 8> status_words{};
    std::vector<compat::u8> action_profile_bytes{};
    compat::u32 action_profile_index{};
    compat::u32 stale_action_profile_edx{};
    compat::u32 status_action_value{};
    compat::u32 status_misc{};
    compat::u32 branch_misc{};
    compat::u32 special_selection_pending{};
    compat::u32 special_action_latch{};

    std::array<compat::u8, 8> opponent_text_present{};
    compat::u32 opponent_text_token_base{0x00527B38U};

    std::array<compat::u16, 10> group_a_completion_words{};
    std::array<compat::u32, 10> group_a_completion_slots{};
    std::array<compat::u16, 161> completion_value_table{};
    compat::u32 completion_resource_token{};
    compat::i32 completion_rect_right{};
    compat::i32 completion_rect_bottom{};
    compat::u32 completion_surface_token{};
    std::span<compat::u16> completion_surface{};
    compat::u32 completion_selected{0xFFFFFFFFU};
    compat::u32 completion_gate{};

    std::array<compat::u32, 8> pending_effect_ids{};
    compat::u32 pending_effect_argument{};
    std::array<compat::u32, 8> final_actor_state{};
    std::array<compat::u32, 8> final_actor_targets{};
    compat::u32 final_gate{};

    LegacyBattleGroupBFrameState() noexcept;
};

// Typed closure of legacy 0x004576A0. One call advances the selected group-B
// actor and preserves the original complete EAX return from its final actor
// step unless an earlier direct-return or typed boundary is reached.
[[nodiscard]] LegacyBattleActionDispatchResult
advance_legacy_battle_group_b_frame(
    LegacyBattleGroupBFrameState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    compat::u32 group_b_index
);

}  // namespace openswd3::battle
