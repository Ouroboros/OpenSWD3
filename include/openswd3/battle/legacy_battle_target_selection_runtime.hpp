#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleTargetSelectionRuntimeState {
    compat::u32 selection_input_gate{};        // 0x0053BFB8
    compat::u32 selected_action_kind{};        // 0x0053BCE8
    compat::u32 actor_commit_gate{};           // 0x0053BF60
    compat::u32 action_mode_flags{};           // 0x0053BCDC
    compat::u32 selection_aux_gate{};          // 0x0053BDA4
    compat::u32 candidate_gate_a{};            // 0x0053BD9C
    compat::u32 candidate_gate_b{};            // 0x0053BDA0
    compat::u32 candidate_argument{};          // 0x0053BD10
    compat::u32 target_argument{};             // 0x0053BD14
    compat::u32 target_effect_value{};         // 0x0053BF24
    compat::u32 actor_special_gate{};          // 0x0053C01C
    compat::u32 special_action_count{};        // 0x0053C4B0
    compat::u32 completion_gate{};             // 0x0053BFE4
    compat::u32 transition_timer{};            // 0x0053BD20
    compat::u32 transition_stage{};            // 0x0053C4BC
    compat::u32 debug_status_profile_token{};  // 0x0053C4B8
    // Platform-owned snapshot of the nine signed bytes at profile + 0x92.
    std::array<compat::i8, 9> debug_status_values{};
    compat::u32 transition_state{};          // 0x0053C038
    compat::u32 transition_mode{};           // 0x0053BFFC
    compat::u32 transition_control_words{};  // 0x0053BF1E..0x0053BF21
    compat::u32 transition_packed_value{};   // 0x0053BDE8
    compat::u16 transition_sample_word{};    // 0x0053C498
    compat::u8 transition_actor_index{};     // 0x004A7646
    compat::u8 transition_aux_byte{};        // 0x0053C4B4
    std::array<compat::u32, 9> target_actor_indices{};  // 0x00520DF4
    std::array<compat::u16, 18> actor_result_words{};   // 0x0052137C

    // Physical bytes around 0x004FE5C0 not owned by the startup reset object.
    std::array<compat::u8, 12> action_remap_prefix{};  // 0x004FE5C0..CB
    std::array<compat::u8, 2> action_remap_gap{};      // 0x004FE5D2..D3
    std::array<compat::u8, 14> action_remap_suffix{};  // 0x004FE5FC..0x004FE609
};

class LegacyBattleTargetSelectionRuntimeStatePort {
public:
    [[nodiscard]] virtual LegacyBattleTargetSelectionRuntimeState&
    battle_target_selection_runtime_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleTargetSelectionRuntimeState&
    battle_target_selection_runtime_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleTargetSelectionRuntimeStatePort() = default;
    ~LegacyBattleTargetSelectionRuntimeStatePort() = default;

private:
    LegacyBattleTargetSelectionRuntimeState state_{};
};

enum class LegacyBattleTargetSelectionRuntimeCall : compat::u8 {
    set_cursor_position,
    draw_target_panel,
    commit_actor_action,
    refresh_actor_selection,
    reserved_input_record_priming_slot,
    reserved_group_b_target_cycle_slot,
    reserved_group_a_target_cycle_slot,
    validate_primary_action,
    query_primary_target,
    apply_special_actor_action,
    resolve_action_target,
    resolve_action_effect_value,
    reserved_query_action_thirty_override_slot,
    reserved_query_action_four_override_slot,
    reserved_display_warning_text_slot,
    reset_actor_selection,
    build_selection_snapshot,
    query_actor_cleanup,
    query_group_b_completion,
    query_actor_property_a,
    query_actor_property_b,
    query_actor_property_c,
    apply_actor_action,
    stop_effect_sample,
    start_effect_sample,
    finalize_actor_effect,
    set_actor_mode,
    resource_profile_load,
    resource_missing_word_diagnostic,
};

struct LegacyBattleTargetSelectionRuntimeCallRequest {
    LegacyBattleTargetSelectionRuntimeCall call{
        LegacyBattleTargetSelectionRuntimeCall::
            reserved_input_record_priming_slot
    };
    compat::u32 actor_token{};
    std::array<compat::u32, 5> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleTargetSelectionRuntimeCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 output_value{};
};

class LegacyBattleTargetSelectionRuntimePort
    : public virtual LegacyBattleTargetSelectionRuntimeStatePort {
public:
    virtual ~LegacyBattleTargetSelectionRuntimePort() = default;

    [[nodiscard]] virtual LegacyBattleTargetSelectionRuntimeCallReply
    invoke_target_selection_runtime(
        const LegacyBattleTargetSelectionRuntimeCallRequest& request
    ) {
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }
};

}  // namespace openswd3::battle
