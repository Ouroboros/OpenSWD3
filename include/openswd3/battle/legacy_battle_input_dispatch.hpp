#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_context_prompt.hpp"
#include "openswd3/battle/legacy_battle_debug_state.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_interaction.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleFrameInputResolutionState;

inline constexpr compat::u32 kLegacyBattleInputWarningTextToken = 0x004A7954U;
inline constexpr compat::u32 kLegacyBattleInputWarningSample = 0x8CU;

struct LegacyBattleInputDispatchState {
    compat::u32 menu_action{};
    compat::u32 action_kind{};                // 0x004A7548
    compat::u32 action_lookup_auxiliary{1U};  // 0x004A7554
    compat::u32 action_category_index{};      // 0x0053BD18
    compat::u32 selection_index{1U};
    compat::u32 input_gate{};  // 0x0053C024
    compat::u32 input_latch{};
    compat::u16 retreat_block_word{};        // 0x0053BF1C
    compat::u16 selection_actor_origin_x{};  // 0x0053BF4A
    compat::u16 selection_actor_origin_y{};  // 0x0053BF4E
    compat::u32 action_block_gate{};
    compat::u16 retreat_target_word{0xFFFFU};   // 0x004A7626
    compat::u16 selected_option_word{0xFFFFU};  // 0x004A7644
    compat::u16 action_word{};
    compat::u32 frame_gate_c{};
    compat::u32 frame_value_a{};
    compat::u32 frame_value_b{};
    compat::u32 interaction_mode{};
    compat::u32 captured_mouse_y{};
    compat::u32 captured_mouse_aux{};
    compat::u32 mouse_action_gate{};
    compat::u32 signed_status{};
    compat::u32 choice_guard{};
    compat::u32 choice_selection_index{};
    compat::u32 final_value_a{};
    compat::u32 final_value_b{};
    compat::u16 selected_group_b_index{0xFFFFU};       // 0x004A762C
    compat::u16 selected_group_a_index{0xFFFFU};       // 0x004A762E
    compat::u16 target_transition_word{};              // 0x0053BDEA
    compat::u32 fallback_action_kind{};                // 0x0053BCF0
    compat::u32 selected_actor_cleanup_gate{};         // 0x0053C018
    compat::u32 selection_runtime_gate{};              // 0x0053BFB0
    compat::u32 selection_cache_gate_a{};              // 0x0053BFC0
    compat::u32 selection_cache_gate_b{};              // 0x0053BFC4
    compat::u32 selection_cache_gate_c{};              // 0x0053BFC8
    compat::u32 selection_animation_frame_a{};         // 0x0053BD90
    compat::u32 selection_animation_frame_b{};         // 0x0053BD94
    compat::u32 selection_animation_phase{};           // 0x0053BD98
    compat::u32 selection_mode_cache{};                // 0x0053BF94
    compat::u32 selection_target_cache{};              // 0x0053BFF0
    compat::u32 selected_actor_reset_gate{};           // 0x0053C02C
    std::array<compat::u32, 5> selection_workspace{};  // 0x0053C184
    compat::i32 sample_mix_level{};
};

class LegacyBattleInputDispatchStatePort {
public:
    [[nodiscard]] virtual LegacyBattleInputDispatchState&
    battle_input_dispatch_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleInputDispatchState&
    battle_input_dispatch_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleInputDispatchStatePort() = default;
    ~LegacyBattleInputDispatchStatePort() = default;

private:
    LegacyBattleInputDispatchState state_{};
};

enum class LegacyBattleInputDispatchCall : compat::u8 {
    reserved_action_mode_refresh_slot,
    reserved_target_selection_entry_slot,
    reserved_menu_selection_retreat_slot,
    reserved_menu_selection_advance_slot,
    reserved_menu_page_retreat_slot,
    reserved_menu_page_advance_slot,
    reserved_actor_action_cycle_slot,
    reserved_actor_action_reverse_cycle_slot,
    reserved_actor_action_commit_direct_slot,
    reserved_menu_context_retreat_slot,
    reserved_menu_context_advance_slot,
    reserved_menu_input_finalize_slot,
    query_active_actor,
    query_retreat_actor,
    configure_retreat_actor,
    display_retreat_warning,
    menu_retreat_query_group_b_candidate,
    menu_retreat_prepare_actor_origin,
    menu_retreat_configure_actor_selection,
    menu_retreat_query_group_a_candidate,
    menu_advance_query_group_b_candidate,
    menu_advance_prepare_actor_origin,
    menu_advance_configure_actor_selection,
    menu_advance_query_group_a_candidate,
    menu_finalize_reset_active_group_a_actor,
    menu_finalize_reset_actor,
    target_selection_configure_actor,
    target_selection_scan_primary,
    target_selection_scan_secondary,
    reserved_target_selection_refresh_state_slot,
    reserved_available_actor_cycle_slot,
    reserved_actor_action_commit_nested_slot,
    reserved_available_actor_reverse_cycle_slot,
    action_mode_query_primary_actor,
    action_mode_query_secondary_actor,
    action_mode_query_active_actor,
};

struct LegacyBattleInputDispatchCallRequest {
    LegacyBattleInputDispatchCall call{};
    std::array<compat::u32, 5> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleInputDispatchCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u16 output_word_a{};
    compat::u16 output_word_b{};
};

class LegacyBattleInputDispatchPort
    : public virtual LegacyBattleInputDispatchStatePort,
      public virtual LegacyBattleTargetSelectionRuntimePort {
public:
    virtual ~LegacyBattleInputDispatchPort() = default;

    [[nodiscard]] virtual LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(const LegacyBattleInputDispatchCallRequest& request) {
        static_cast<void>(request);
        return {};
    }
    virtual void delay_input_milliseconds(compat::u32 milliseconds) {
        static_cast<void>(milliseconds);
    }
    [[nodiscard]] virtual LegacyBattleInputDispatchCallReply play_input_sample(
        compat::u32 sound_id,
        compat::i32 mix_level,
        compat::u32 eax,
        compat::u32 ecx,
        compat::u32 edx
    ) {
        static_cast<void>(sound_id);
        static_cast<void>(mix_level);
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }
};

struct LegacyBattleInputDispatchBindings {
    compat::u32& render_abort_latch;
    LegacyBattleStartupResetBlocks& startup_reset;
    const LegacyBattleActionModeSourceState& action_mode_source;
    const std::array<compat::u8, 4>& startup_party_presence;
    const compat::u32& startup_mode_flags;
    compat::u16& startup_supplemental_count_word;
    compat::u32& startup_mirror_mode;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleContextPromptState& context_prompt;
    compat::u32& message_state;
    compat::u32& terminal_latch;
    compat::u32& one_shot_interaction_state;
    compat::u32& target_ready_gate;
    compat::u32& outcome_darkening_gate;
    std::span<input_time_rng::LegacyInputRecord> input_records;
    const input_time_rng::LegacyKeyboardSnapshot& keyboard;
    story_scene::LegacyDialogRuntimeState& dialogs;
    std::vector<world_map::LegacyWorldInteractionHotspot>& choice_hotspots;
};

struct LegacyBattleInputDispatchRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::i32 mouse_y{};
    compat::u32 mouse_lower_bound{};
    compat::u32 mouse_upper_bound{480U};
};

enum class LegacyBattleInputDispatchStatus : compat::u8 {
    completed,
    input_record_typed_stop,
    workspace_typed_stop,
    menu_selection_retreat_typed_stop,
    menu_selection_advance_typed_stop,
    menu_page_retreat_typed_stop,
    menu_page_advance_typed_stop,
    menu_input_finalize_typed_stop,
    action_mode_refresh_typed_stop,
    target_selection_entry_typed_stop,
    actor_action_commit_typed_stop,
    actor_action_cycle_typed_stop,
    actor_action_reverse_cycle_typed_stop,
    menu_context_advance_typed_stop,
    menu_context_retreat_typed_stop,
};

struct LegacyBattleInputDispatchResult {
    LegacyBattleInputDispatchStatus status{
        LegacyBattleInputDispatchStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 raw_key_queries{};
    compat::u32 input_record_reads{};
    compat::u32 input_record_writes{};
    compat::u32 port_calls{};
    compat::u32 delay_calls{};
    compat::u32 menu_selection_retreat_calls{};
    compat::u32 menu_selection_advance_calls{};
    compat::u32 menu_page_retreat_calls{};
    compat::u32 menu_page_advance_calls{};
    compat::u32 menu_input_finalize_calls{};
    compat::u32 action_mode_refresh_calls{};
    compat::u32 target_selection_entry_calls{};
    compat::u32 target_selection_refresh_calls{};
    compat::u32 actor_action_cycle_calls{};
    compat::u32 actor_action_reverse_cycle_calls{};
    compat::u32 actor_action_commit_calls{};
    compat::u32 menu_context_advance_calls{};
    compat::u32 menu_context_retreat_calls{};
    bool returned_early{};
};

// Typed closure of legacy 0x0045F2A0.
[[nodiscard]] LegacyBattleInputDispatchResult
coordinate_legacy_battle_input_dispatch(
    LegacyBattleInputDispatchBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleInputDispatchRequest& request
);

}  // namespace openswd3::battle
