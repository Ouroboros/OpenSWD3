#pragma once

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_context_prompt.hpp"
#include "openswd3/battle/legacy_battle_debug_state.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
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
    compat::u32 action_kind{};
    compat::u32 selection_index{1U};
    compat::u32 input_gate{};
    compat::u32 input_latch{};
    compat::u16 retreat_block_word{};
    compat::u32 action_block_gate{};
    compat::u16 retreat_target_word{0xFFFFU};
    compat::u16 selected_option_word{0xFFFFU};
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
    refresh_action_mode,
    commit_selection,
    reserved_menu_selection_retreat_slot,
    mode_two,
    mode_three,
    mode_four,
    confirm_primary,
    confirm_secondary,
    commit_selected_option,
    commit_left,
    commit_right,
    commit_final,
    query_active_actor,
    query_retreat_actor,
    configure_retreat_actor,
    display_retreat_warning,
    menu_retreat_query_group_b_candidate,
    menu_retreat_prepare_actor_origin,
    menu_retreat_configure_actor_selection,
    menu_retreat_query_group_a_candidate,
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
};

class LegacyBattleInputDispatchPort
    : public virtual LegacyBattleInputDispatchStatePort {
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
    compat::u16& startup_supplemental_count_word;
    LegacyBattleFrameInputResolutionState& frame_input_resolution;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleContextPromptState& context_prompt;
    compat::u32& message_state;
    compat::u32& terminal_latch;
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
