#pragma once

#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleFrameInputResolutionState {
    compat::i32 previous_mouse_x{};                // 0x005028A0
    compat::i32 previous_mouse_y{};                // 0x005028A4
    compat::u32 pointer_activity_gate{};           // 0x0053BDA8
    compat::u32 list_selection{1U};                // 0x004A7558
    compat::u32 grid_selection{1U};                // 0x004A755C
    compat::u32 narrow_list_selection{1U};         // 0x004A7560
    compat::u32 selection_actor_code{0xFFFFFFFFU}; // 0x004A7564
    compat::u32 current_equipment_selection{2U};   // 0x004A7570
    compat::u32 target_cursor{1U};                 // 0x004A7550
    compat::u32 alternate_selection_limit{2U};     // 0x004A7568
    compat::u32 alternate_selection{1U};           // 0x004A756C
    compat::u32 hovered_equipment{0xFFFFFFFFU};    // 0x004A7578
    compat::u32 hovered_secondary{0xFFFFFFFFU};    // 0x004A757C
    compat::u32 target_action_available{};         // 0x004A7640
    compat::u32 panel_scroll_a{};                  // 0x0053BD04
    compat::u32 panel_scroll_b{};                  // 0x0053BD08
    compat::u32 panel_origin_x{};                  // 0x0053BD24
    compat::u32 panel_origin_y{};                  // 0x0053BD28
    compat::u32 group_b_row_selection{};           // 0x0053BCF8
    compat::u32 target_actor_index{};              // 0x0053BCF4
    compat::u32 target_selection_gate{};           // 0x0053C49C
    compat::u32 transition_value_a{};              // 0x0053BD44
    compat::u32 transition_value_b{};              // 0x0053BD48
    compat::u32 transient_selection_a{};           // 0x0053BD6C
    compat::u32 transient_selection_b{};           // 0x0053BD70
    compat::u8 panel_row_limit_a{};                // 0x0053BDF2
    compat::u8 panel_row_limit_b{};                // 0x0053BDF3
    compat::u16 panel_row_limit_c{};               // 0x0053BDF4
    compat::u16 selection_block_word{};            // 0x0053BF2A
    compat::u32 target_selection_block{};          // 0x0053BF88
    compat::u32 transient_selection_c{};           // 0x0053BFF8
    compat::u32 selected_target_index{};           // 0x0053C4AC
    compat::u8 target_selection_suppression{};     // 0x0053C4C0
    compat::u32 lower_panel_top{};                 // 0x004FDC5C
    compat::u32 lower_panel_bottom{};              // 0x004FDC58
    compat::u32 lower_panel_aux{};                 // 0x004FDC60
    compat::u32 lower_panel_aux_index{};           // 0x004FDC64
    compat::u32 final_panel_top{};                 // 0x004FD790
    compat::u32 final_panel_bottom{};              // 0x004FD794
    std::array<compat::u16, 8> option_role_ids{};  // 0x004FE5CA view
    // Current-equipment selection cache at 0x004FF578.
    std::array<compat::u32, 4> equipment_grid_selections{1U};
    std::array<compat::u8, 10> target_markers{};  // 0x00524118 prefix
};

class LegacyBattleFrameInputResolutionStatePort {
public:
    [[nodiscard]] virtual LegacyBattleFrameInputResolutionState&
    battle_frame_input_resolution_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleFrameInputResolutionState&
    battle_frame_input_resolution_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleFrameInputResolutionStatePort() = default;
    ~LegacyBattleFrameInputResolutionStatePort() = default;

private:
    LegacyBattleFrameInputResolutionState state_{};
};

enum class LegacyBattleFrameInputResolutionCall : compat::u8 {
    validate_option_actor,
    configure_actor_selection,
    query_group_b_candidate,
    prepare_actor_origin,
    resolve_actor_surface,
    query_actor_mirror,
    reserved_query_group_b_action_six_target_availability_slot,
    query_group_a_candidate,
};

struct LegacyBattleFrameInputSurface {
    compat::u32 object_token{};
    bool command_stream_present{};
    std::span<const compat::u8> command_stream{};
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyBattleFrameInputResolutionCallRequest {
    LegacyBattleFrameInputResolutionCall call{
        LegacyBattleFrameInputResolutionCall::configure_actor_selection
    };
    compat::u32 actor_token{};
    std::array<compat::u32, 4> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleFrameInputResolutionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::i32 origin_x{};
    compat::i32 origin_y{};
    LegacyBattleFrameInputSurface surface{};
};

class LegacyBattleFrameInputResolutionPort
    : public virtual LegacyBattleFrameInputResolutionStatePort,
      public virtual LegacyBattleInputDispatchPort {
public:
    virtual ~LegacyBattleFrameInputResolutionPort() = default;

    [[nodiscard]] virtual LegacyBattleFrameInputResolutionCallReply
    invoke_frame_input_resolution(
        const LegacyBattleFrameInputResolutionCallRequest& request
    ) = 0;
};

struct LegacyBattleFrameInputResolutionBindings {
    LegacyBattleStartupState& startup;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleInputDispatchState& input_dispatch;
    input_time_rng::LegacyInputNormalizationState& input;
    compat::u32& message_state;
    std::vector<world_map::LegacyWorldInteractionHotspot>& choice_hotspots;
};

struct LegacyBattleFrameInputResolutionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleFrameInputResolutionStatus : compat::u8 {
    completed,
    party_source_index_typed_stop,
    party_offset_typed_stop,
    permission_typed_stop,
    option_role_typed_stop,
    startup_mode_typed_stop,
    actor_order_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    target_marker_typed_stop,
    image_source_typed_stop,
};

struct LegacyBattleFrameInputResolutionResult {
    LegacyBattleFrameInputResolutionStatus status{
        LegacyBattleFrameInputResolutionStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 sample_calls{};
    compat::u32 hotspot_queries{};
    compat::u32 image_queries{};
    compat::u32 actor_iterations{};
    compat::u32 action_six_availability_queries{};
    LegacyBattleGroupBActionSixTargetAvailabilityResult
        action_six_availability{};
    bool returned_early{};
};

[[nodiscard]] LegacyBattleFrameInputResolutionResult
coordinate_legacy_battle_frame_input_resolution(
    LegacyBattleFrameInputResolutionBindings bindings,
    LegacyBattleFrameInputResolutionPort& port,
    const LegacyBattleFrameInputResolutionRequest& request = {}
);

}  // namespace openswd3::battle
