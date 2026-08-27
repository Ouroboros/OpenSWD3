#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_group_b_frame.hpp"
#include "openswd3/battle/legacy_battle_outcome_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"
#include "openswd3/world_map/legacy_world_player_control.hpp"

#include <array>

namespace openswd3::battle {

enum class LegacyBattleDebugHotkeyCall : compat::u8 {
    suspend_audio_output,
    display_text,
    reset_group_a_primary,
    reset_group_a_secondary,
    configure_group_a,
    publish_actor_value,
    query_special_index,
    reset_special_group_b,
    reset_actor,
    restart_battle_music,
    query_actor_status,
    adjust_actor,
};

struct LegacyBattleDebugHotkeyCallRequest {
    LegacyBattleDebugHotkeyCall call{};
    compat::u32 object_token{};
    std::array<compat::u32, 5> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleDebugHotkeyCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_priority_actor{};
    compat::u32 priority_actor{};
};

struct LegacyBattleDebugHotkeyState {
    compat::u32 developer_tools_enabled{};
    compat::u32 toggle_5244e0{};
    compat::u32 toggle_53af68{};
    compat::u32 message_latch_53ceb8{};
    compat::u32 selection_status_word_53c050{};
    compat::u32 actor_retarget_gate_53bf64{};
    std::array<compat::u32, 6> selection_workspace_tail{};
    compat::u32 text_mode_toggle_53c02c{};
    compat::u32 battle_mode_flags_53bc24{};
    std::array<compat::u32, 10> block_53af30{};
    compat::u32 reset_gate_53bd50{};
    compat::u32 screenshot_request{};
};

class LegacyBattleDebugHotkeyStatePort {
public:
    [[nodiscard]] virtual LegacyBattleDebugHotkeyState&
    battle_debug_hotkey_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleDebugHotkeyState&
    battle_debug_hotkey_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleDebugHotkeyStatePort() = default;
    ~LegacyBattleDebugHotkeyStatePort() = default;

private:
    LegacyBattleDebugHotkeyState state_{};
};

class LegacyBattleDebugHotkeyPort
    : public virtual LegacyBattleDebugHotkeyStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort {
public:
    virtual ~LegacyBattleDebugHotkeyPort() = default;

    [[nodiscard]] virtual LegacyBattleDebugHotkeyCallReply
    invoke_debug_hotkey(const LegacyBattleDebugHotkeyCallRequest& request) {
        static_cast<void>(request);
        return {};
    }
    virtual void delay_milliseconds(compat::u32 milliseconds) {
        static_cast<void>(milliseconds);
    }
};

struct LegacyBattleDebugHotkeyBindings {
    LegacyBattleStartupState& startup;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& actor_metrics;
    LegacyBattleActorPublicationState& actor_publication;
    LegacyBattleEffectCoordinatorState& effect_coordinator;
    LegacyBattleEffectShiftState& effect_shift;
    LegacyBattleGroupBFrameState* actor_frames{};
    world_map::LegacyWorldPlayerControlState& player_control;
    compat::u32& message_state;
};

enum class LegacyBattleDebugHotkeyStatus : compat::u8 {
    completed,
    group_a_runtime_typed_stop,
    group_b_publication_typed_stop,
    actor_frame_state_typed_stop,
};

struct LegacyBattleDebugHotkeyResult {
    LegacyBattleDebugHotkeyStatus status{
        LegacyBattleDebugHotkeyStatus::completed
    };
    compat::u32 return_value{1U};
    compat::u32 raw_key_queries{};
    compat::u32 port_calls{};
    compat::u32 delay_calls{};
    compat::u32 group_a_iterations{};
    compat::u32 group_b_iterations{};
    compat::u32 actor_adjust_iterations{};
    bool control_chord_active{};
    bool early_return_zero{};
    bool full_reset_applied{};
};

[[nodiscard]] LegacyBattleDebugHotkeyResult
coordinate_legacy_battle_debug_hotkeys(
    const input_time_rng::LegacyKeyboardSnapshot& keyboard,
    LegacyBattleDebugHotkeyState& state,
    LegacyBattleDebugHotkeyBindings bindings,
    LegacyBattleDebugHotkeyPort& port
);

}  // namespace openswd3::battle
