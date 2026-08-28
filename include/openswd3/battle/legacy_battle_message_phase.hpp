#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_group_b_frame.hpp"
#include "openswd3/battle/legacy_battle_growth_actor_selection.hpp"
#include "openswd3/battle/legacy_battle_growth_caption.hpp"
#include "openswd3/battle/legacy_battle_growth_item_completion_panel.hpp"
#include "openswd3/battle/legacy_battle_growth_item_result_selection.hpp"
#include "openswd3/battle/legacy_battle_growth_completion_caption.hpp"
#include "openswd3/battle/legacy_battle_level_advancement.hpp"
#include "openswd3/battle/legacy_battle_level_growth_panel.hpp"
#include "openswd3/battle/legacy_battle_level_up_panel.hpp"
#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"
#include "openswd3/battle/legacy_battle_selection_frame.hpp"
#include "openswd3/battle/legacy_battle_target_selection_entry.hpp"
#include "openswd3/battle/legacy_battle_victory_item_list_panel.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleMessagePhaseGroupABaseToken =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleMessagePhaseGroupAElementSize =
    0x2F34U;
inline constexpr compat::u32 kLegacyBattleMessagePhaseGroupBBaseToken =
    0x00525508U;
inline constexpr compat::u32 kLegacyBattleMessagePhaseGroupBElementSize =
    0x2B28U;
inline constexpr compat::u32 kLegacyBattleMessagePhaseSample = 0x160U;

struct LegacyBattleMessagePhaseState {
    compat::u32 entry_list_gate{};       // 0x004ACF48
    compat::u32 transition_mode_gate{};  // 0x0053C004
    compat::u32 group_b_bypass_gate{};   // 0x0053CEAC
};

class LegacyBattleMessagePhaseStatePort {
public:
    [[nodiscard]] virtual LegacyBattleMessagePhaseState&
    battle_message_phase_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleMessagePhaseState&
    battle_message_phase_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleMessagePhaseStatePort() = default;
    ~LegacyBattleMessagePhaseStatePort() = default;

private:
    LegacyBattleMessagePhaseState state_{};
};

enum class LegacyBattleMessagePhaseCall : compat::u8 {
    resolve_group_a_position,
    prepare_message_98,
    reset_actor_state,
    query_actor_completion,
    prepare_transition_control,
    prepare_group_a_actor,
    reset_group_a_actor,
    set_group_a_actor_mode,
    commit_active_actor,
    configure_actor_action,
    query_actor_resource,
    resolve_action_item,
    reserved_advance_message_100_slot,
    select_message_101_actor,
    allocate_actor_transition,
    advance_message_101,
    reserved_advance_message_110_slot,
    reserved_advance_message_111_slot,
    reserved_select_message_112_actor_slot,
    reserved_advance_message_112_slot,
    reserved_select_message_113_actor_slot,
    reserved_advance_message_113_slot,
    reserved_advance_message_102_slot,
    advance_message_103,
};

struct LegacyBattleMessagePhaseCallRequest {
    LegacyBattleMessagePhaseCall call{
        LegacyBattleMessagePhaseCall::resolve_group_a_position
    };
    compat::u32 actor_token{};
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleMessagePhaseCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    bool publish_message_state{};
    compat::u32 message_state{};
    bool publish_transition_actor_index{};
    compat::u8 transition_actor_index{};
    bool publish_transition_control_words{};
    compat::u32 transition_control_words{};
    bool publish_transition_state{};
    compat::u32 transition_state{};
    bool publish_transition_timer{};
    compat::u32 transition_timer{};
    bool publish_transition_sample_word{};
    compat::u16 transition_sample_word{};
    bool publish_transition_aux_byte{};
    compat::u8 transition_aux_byte{};
    bool publish_completion_gate{};
    compat::u32 completion_gate{};
    bool publish_special_action_count{};
    compat::u32 special_action_count{};
    bool publish_target_ready_gate{};
    compat::u32 target_ready_gate{};
    bool publish_transition_mode_gate{};
    compat::u32 transition_mode_gate{};
    bool publish_group_b_bypass_gate{};
    compat::u32 group_b_bypass_gate{};
};

class LegacyBattleMessagePhasePort
    : public virtual LegacyBattleMessagePhaseStatePort,
      public virtual LegacyBattleVictoryRewardPort,
      public virtual LegacyBattleLevelAdvancementPort,
      public virtual LegacyBattleLevelGrowthPanelPort,
      public virtual LegacyBattleGrowthActorSelectionPort,
      public virtual LegacyBattleGrowthCaptionPort,
      public virtual LegacyBattleGrowthItemCompletionPanelPort,
      public virtual LegacyBattleGrowthItemResultSelectionPort,
      public virtual LegacyBattleVictoryItemListPanelPort {
public:
    ~LegacyBattleMessagePhasePort() override = default;

    [[nodiscard]] virtual LegacyBattleMessagePhaseCallReply
    invoke_message_phase(const LegacyBattleMessagePhaseCallRequest& request) {
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }
};

struct LegacyBattleMessagePhaseBindings {
    LegacyBattleMessagePhaseState& state;
    LegacyBattleStartupState& startup;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleInputDispatchState& input_dispatch;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleSelectionFrameState& selection_frame;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    compat::u32& target_ready_gate;
    compat::u32& message_state;
    story_scene::LegacyDialogRuntimeState& dialogs;
    compat::u32& one_shot_interaction_state;
    compat::u32& outcome_darkening_gate;
    std::span<input_time_rng::LegacyInputRecord> input_records;
    std::span<const compat::u8> action_profile_bytes;
    LegacyBattleVictoryRewardBindings victory_rewards;
};

struct LegacyBattleMessagePhaseRequest {
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleVictoryRewardRequest victory_reward_request{};
    LegacyBattleLevelUpPanelRequest level_up_panel_request{};
    LegacyBattleLevelAdvancementRequest level_advancement_request{};
    LegacyBattleLevelGrowthPanelRequest level_growth_panel_request{};
    LegacyBattleGrowthCaptionRequest growth_caption_request{};
    LegacyBattleGrowthCaptionRequest growth_completion_caption_request{};
    LegacyBattleGrowthItemCompletionPanelRequest
        growth_item_completion_panel_request{};
    LegacyBattleGrowthItemResultSelectionRequest
        growth_item_result_selection_request{};
    LegacyBattleVictoryItemListPanelRequest victory_item_list_panel_request{};
};

enum class LegacyBattleMessagePhaseStatus : compat::u8 {
    completed,
    group_a_position_typed_stop,
    group_b_actor_typed_stop,
    group_a_actor_typed_stop,
    priority_workspace_typed_stop,
    attack_order_table_typed_stop,
    action_label_typed_stop,
    action_profile_typed_stop,
    player_item_quantity_typed_stop,
    victory_rewards_typed_stop,
    level_up_panel_typed_stop,
    level_advancement_typed_stop,
    level_growth_panel_typed_stop,
    growth_actor_selection_typed_stop,
    growth_caption_typed_stop,
    growth_completion_caption_typed_stop,
    growth_item_completion_panel_typed_stop,
    growth_item_result_selection_typed_stop,
    victory_item_list_panel_typed_stop,
    target_selection_entry_typed_stop,
};

struct LegacyBattleMessagePhaseResult {
    LegacyBattleMessagePhaseStatus status{
        LegacyBattleMessagePhaseStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 group_b_reset_calls{};
    compat::u32 group_b_completion_calls{};
    compat::u32 group_a_reset_calls{};
    compat::u32 group_a_prepare_calls{};
    compat::u32 sample_calls{};
    compat::u32 target_selection_entry_calls{};
    compat::u32 player_item_quantity_calls{};
    compat::u32 victory_reward_calls{};
    compat::u32 level_up_panel_calls{};
    compat::u32 level_advancement_calls{};
    compat::u32 level_growth_panel_calls{};
    compat::u32 growth_actor_selection_calls{};
    compat::u32 growth_caption_calls{};
    compat::u32 growth_completion_caption_calls{};
    compat::u32 growth_item_completion_panel_calls{};
    compat::u32 growth_item_result_selection_calls{};
    LegacyBattleTargetSelectionEntryResult target_selection_entry{};
    LegacyBattlePlayerItemQuantityResult player_item_quantity{};
    LegacyBattleVictoryRewardResult victory_rewards{};
    LegacyBattleLevelUpPanelResult level_up_panel{};
    LegacyBattleLevelAdvancementResult level_advancement{};
    LegacyBattleLevelGrowthPanelResult level_growth_panel{};
    LegacyBattleGrowthActorSelectionResult growth_actor_selection{};
    LegacyBattleGrowthCaptionResult growth_caption{};
    LegacyBattleGrowthCaptionResult growth_completion_caption{};
    LegacyBattleGrowthItemCompletionPanelResult growth_item_completion_panel{};
    LegacyBattleGrowthItemResultSelectionResult growth_item_result_selection{};
    compat::u32 victory_item_list_panel_calls{};
    LegacyBattleVictoryItemListPanelResult victory_item_list_panel{};
    std::vector<LegacyBattleMessagePhaseCall> call_trace;
    compat::u32 call_trace_count{};
};

// Typed closure of legacy 0x00466F70.
[[nodiscard]] LegacyBattleMessagePhaseResult
advance_legacy_battle_message_phase(
    LegacyBattleMessagePhaseBindings bindings,
    LegacyBattleMessagePhasePort& port,
    const LegacyBattleMessagePhaseRequest& request = {}
);

}  // namespace openswd3::battle
