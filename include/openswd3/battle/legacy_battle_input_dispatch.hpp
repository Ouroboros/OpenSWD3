#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_retreat_ready.hpp"
#include "openswd3/battle/legacy_battle_actor_frame_snapshot.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_context_prompt.hpp"
#include "openswd3/battle/legacy_battle_debug_state.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_item_option.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"
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
    compat::u32 input_gate{};                // 0x0053C024
    compat::u32 input_latch{};               // 0x0053BDA4
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
    compat::u16 selected_group_b_index{0xFFFFU};            // 0x004A762C
    compat::u16 selected_group_a_index{0xFFFFU};            // 0x004A762E
    compat::u16 target_transition_word{};                   // 0x0053BDEA
    compat::u32 fallback_action_kind{};                     // 0x0053BCF0
    compat::u32 selected_actor_cleanup_gate{};              // 0x0053C018
    compat::u32 selection_runtime_gate{};                   // 0x0053BFB0
    compat::u32 selection_cache_gate_c{};                   // 0x0053BFC8
    compat::u32 selection_animation_frame_a{};              // 0x0053BD90
    compat::u32 selection_animation_frame_b{};              // 0x0053BD94
    compat::u32 selection_animation_phase{};                // 0x0053BD98
    compat::u32 selection_target_cache{};                   // 0x0053BFF0
    compat::u32 selected_actor_reset_gate{};                // 0x0053C02C
    std::array<compat::u32, 6> selection_text_workspace{};  // 0x0053C16C
    std::array<compat::u32, 5> selection_workspace{};       // 0x0053C184
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
    reserved_query_retreat_actor_slot,
    reserved_display_retreat_warning_slot,
    menu_retreat_query_group_b_candidate,
    reserved_menu_retreat_prepare_actor_origin_slot,
    menu_retreat_configure_actor_selection,
    menu_retreat_query_group_a_candidate,
    menu_advance_query_group_b_candidate,
    reserved_menu_advance_prepare_actor_origin_slot,
    menu_advance_configure_actor_selection,
    menu_advance_query_group_a_candidate,
    reserved_menu_finalize_reset_active_group_a_actor_slot,
    menu_finalize_reset_actor,
    reserved_target_selection_current_coordinates_slot,
    reserved_target_selection_scan_primary_slot,
    reserved_target_selection_scan_secondary_slot,
    reserved_target_selection_refresh_state_slot,
    reserved_available_actor_cycle_slot,
    reserved_actor_action_commit_nested_slot,
    reserved_available_actor_reverse_cycle_slot,
    action_mode_query_primary_actor,
    action_mode_query_secondary_actor,
    action_mode_query_active_actor,
    text_message_allocate,
    text_message_measure,
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
    : public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleInputDispatchStatePort,
      public virtual LegacyBattleFrameEffectControlStatePort,
      public virtual LegacyBattleTargetSelectionRuntimePort,
      public virtual LegacyBattleGroupBActionItemOptionPort {
public:
    virtual ~LegacyBattleInputDispatchPort() = default;

    [[nodiscard]] virtual LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(const LegacyBattleInputDispatchCallRequest& request) {
        static_cast<void>(request);
        return {};
    }

    [[nodiscard]] LegacyBattleGroupBActionItemNameCopyReply
    copy_action_item_name(
        const LegacyBattleGroupBActionItemNameCopyRequest& request
    ) override {
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
    LegacyBattleStartupState& startup;
    LegacyBattleStartupResetBlocks& startup_reset;
    LegacyBattleTextMessageState& text_messages;
    const LegacyBattleActionModeSourceState& action_mode_source;
    const std::array<compat::u8, 4>& startup_party_presence;
    const compat::u32& startup_mode_flags;
    std::span<LegacyBattlePartyStartupRecord> party{};
    compat::u16& startup_supplemental_count_word;
    compat::u32& startup_mirror_mode;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    std::span<LegacyBattleActorGroupBElementState> group_b_actors;
    LegacyBattleContextPromptState& context_prompt;
    compat::u32& message_state;
    compat::u32& terminal_latch;
    compat::u32& one_shot_interaction_state;
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
    LegacyBattleActorFrameSnapshotRequest menu_actor_frame_snapshot{};
};

enum class LegacyBattleInputDispatchStatus : compat::u8 {
    completed,
    input_record_typed_stop,
    workspace_typed_stop,
    actor_availability_block_typed_stop,
    text_message_typed_stop,
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
    actor_retreat_ready_typed_stop,
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
    LegacyBattleActorAvailabilityBlockResult actor_availability_block{};
    compat::u32 actor_availability_block_calls{};
    std::vector<LegacyBattleTextMessageResult> text_messages;
    compat::u32 text_message_calls{};
    LegacyBattleActorRetreatReadyResult actor_retreat_ready{};
    compat::u32 actor_retreat_ready_calls{};
    compat::u32 menu_selection_retreat_calls{};
    compat::u32 menu_selection_advance_calls{};
    compat::u32 menu_actor_frame_snapshot_queries{};
    LegacyBattleActorFrameSnapshotResult menu_actor_frame_snapshot{};
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

enum class LegacyBattleInputDispatchEntryStatus : compat::u8 {
    returned_render_abort_latch,
    continue_at_message_state,
};

// 0x0045F2A0..BE: read the render-abort latch, clear the shared menu word,
// then return only when the latch is exactly one.
[[nodiscard]] LegacyBattleInputDispatchEntryStatus
run_legacy_battle_input_dispatch_entry_prefix(
    LegacyBattleInputDispatchState& state, compat::u32 render_abort_latch
) noexcept;

// 0x0045F2BE..EF: signed message comparison, actor code and dialog-chain
// gate before the first keyboard query; no state write in this interval.
[[nodiscard]] bool should_legacy_battle_input_dispatch_query_keyboard(
    compat::u32 message_state,
    compat::u32 queued_actor_code,
    bool dialog_chain_empty
) noexcept;

struct LegacyBattleInputDispatchKeyboardProbeResult {
    compat::u32 raw_key_queries{};
    compat::u32 first_pressed_dik{};
    const char* stop_boundary{"0x0045F5A3 -> input record state"};
};

// 0x0045F2F5..0x0045F59D: query DIK 2..9 in physical order. Stop before
// the first pressed-key mutation, or before the input-record state read.
[[nodiscard]] LegacyBattleInputDispatchKeyboardProbeResult
probe_legacy_battle_input_dispatch_keyboard_prefix(
    const input_time_rng::LegacyKeyboardSnapshot& keyboard
) noexcept;

// 0x0045F5A3..C4: read input gate and record 1, then OR bit 0 of the
// shared input latch only when the three physical conditions all hold.
void run_legacy_battle_input_dispatch_record_one_prefix(
    LegacyBattleInputDispatchState& state,
    const input_time_rng::LegacyInputRecord& record_one
) noexcept;

// 0x0045F5C4..E0: a nonzero record-9 rapid dword and signed positive held
// count publish rapid=1 and the held count to record 0 in this order.
[[nodiscard]] bool run_legacy_battle_input_dispatch_record_nine_prefix(
    input_time_rng::LegacyInputRecord& record_zero,
    const input_time_rng::LegacyInputRecord& record_nine
) noexcept;

enum class LegacyBattleInputRecordTwoStatus : compat::u8 {
    continue_at_record_eighteen,
    return_from_message_gate,
    call_actor_action_cycle,
};

struct LegacyBattleInputRecordTwoPrefixResult {
    LegacyBattleInputRecordTwoStatus status{
        LegacyBattleInputRecordTwoStatus::continue_at_record_eighteen
    };
    bool loaded_held_count{};
    bool divided_held_count{};
    compat::u32 held_count{};
    compat::u32 quotient_bits{};
    compat::u32 remainder_bits{};
};

// 0x0045F5E0..0x0045F629: signed repeat gate, signed message gate and
// the two writes before sub_462320. A triggered repeat writes option zero
// even when its held count is not one (BP was cleared at 0x0045F602).
[[nodiscard]] LegacyBattleInputRecordTwoPrefixResult
run_legacy_battle_input_dispatch_record_two_prefix(
    LegacyBattleInputDispatchState& state,
    const input_time_rng::LegacyInputRecord& record_two,
    compat::u32 message_state
) noexcept;

enum class LegacyBattleInputRecordEighteenStatus : compat::u8 {
    continue_at_record_seventeen,
    return_from_pre_debug_gate,
    read_actor_retarget_gate,
};

struct LegacyBattleInputRecordEighteenPrefixResult {
    LegacyBattleInputRecordEighteenStatus status{
        LegacyBattleInputRecordEighteenStatus::continue_at_record_seventeen
    };
    bool loaded_held_count{};
    bool divided_held_count{};
    compat::u32 held_count{};
    compat::u32 quotient_bits{};
    compat::u32 remainder_bits{};
};

// 0x0045F629..0x0045F672. The debug gate at 0x0045F672 requires its
// actual owner; the prefix stops before reading it on the active route.
[[nodiscard]] LegacyBattleInputRecordEighteenPrefixResult
run_legacy_battle_input_dispatch_record_eighteen_prefix(
    const LegacyBattleInputDispatchState& state,
    const input_time_rng::LegacyInputRecord& record_eighteen
) noexcept;

struct LegacyBattleInputIdleSuffixResult {
    const char* stop_boundary{"0x0045FC5B -> input dispatch RET"};
    compat::u32 inspected_records{};
    compat::u32 first_active_record{input_time_rng::kLegacyInputRecordCount};
};

// Follow only the zero-rapid edges from 0x0045F806 to 0x0045FC5B;
// report the first nonzero read, never execute its unbound action branch.
[[nodiscard]] LegacyBattleInputIdleSuffixResult
scan_legacy_battle_input_dispatch_idle_suffix(
    const std::array<
        input_time_rng::LegacyInputRecord,
        input_time_rng::kLegacyInputRecordCount>& records
) noexcept;

// Typed closure of legacy 0x0045F2A0.
[[nodiscard]] LegacyBattleInputDispatchResult
coordinate_legacy_battle_input_dispatch(
    LegacyBattleInputDispatchBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleInputDispatchRequest& request
);

}  // namespace openswd3::battle
