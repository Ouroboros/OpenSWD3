#pragma once

#include "openswd3/battle/legacy_battle_actor_frame_resource.hpp"
#include "openswd3/battle/legacy_battle_actor_frame_snapshot.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution_state.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <span>
#include <vector>

namespace openswd3::battle {

enum class LegacyBattleFrameInputResolutionCall : compat::u8 {
    validate_option_actor,
    configure_actor_selection,
    query_group_b_candidate,
    reserved_prepare_actor_origin_slot,
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
    LegacyBattleActionDispatchState& action;
    LegacyBattleActorMetricState& metrics;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    LegacyBattleInputDispatchState& input_dispatch;
    input_time_rng::LegacyInputNormalizationState& input;
    compat::u32& message_state;
    std::vector<world_map::LegacyWorldInteractionHotspot>& choice_hotspots;
};

struct LegacyBattleFrameInputResolutionRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 actor_frame_output_token{};
    compat::u32 action_updater_return_ecx{};
    compat::u32 action_updater_return_edx{};
    LegacyBattleActorCoordinateFlags action_updater_flags{};
    compat::u32 frame_provider_return_eax{1U};
    compat::u32 frame_provider_return_ecx{};
    compat::u32 frame_provider_return_edx{};
    LegacyBattleActorCoordinateFlags frame_provider_flags{};
    std::array<compat::u32, 4> actor_frame_initial_output{};
    bool action_updater_flags_known{};
    bool frame_provider_flags_known{};
    bool actor_frame_overlapping_frame_dword_readable{true};
    bool actor_frame_overlapping_resource_dword_readable{true};
    bool actor_frame_output_pointer_readable{true};
    std::array<bool, 4> actor_frame_output_writable{true, true, true, true};
    bool actor_frame_first_token_readable{true};
    bool actor_frame_second_token_readable{true};
    bool actor_frame_width_readable{true};
    bool actor_frame_height_readable{true};
    std::array<LegacyBattleActorFrameResourceRequest, 3>
        actor_frame_resource_requests{};
    std::array<bool, 3> actor_frame_resource_object_readable{true, true, true};
};

enum class LegacyBattleFrameInputGateStatus : compat::u8 {
    returned_zero,
    continue_at_hotspot_head,
};

struct LegacyBattleFrameInputGateResult {
    LegacyBattleFrameInputGateStatus status{
        LegacyBattleFrameInputGateStatus::returned_zero
    };
    compat::u32 eax{};
};

[[nodiscard]] LegacyBattleFrameInputGateResult
run_legacy_battle_frame_input_gate_prefix(
    LegacyBattleFrameInputResolutionState& state,
    LegacyBattleFinalActorStepState& final_actor,
    const input_time_rng::LegacyInputNormalizationState& input
) noexcept;

struct LegacyBattleFrameInputHotspotResult {
    compat::u32 eax{};
    compat::u32 mouse_x{};
    compat::u32 mouse_y{};
    compat::u32 hotspot_queries{};
};

[[nodiscard]] LegacyBattleFrameInputHotspotResult
run_legacy_battle_frame_input_hotspot_prefix(
    LegacyBattleFrameInputResolutionState& state,
    LegacyBattleInputDispatchState& input_dispatch,
    const input_time_rng::LegacyInputNormalizationState& input,
    std::span<const world_map::LegacyWorldInteractionHotspot> choice_hotspots,
    compat::u32 gate_eax
) noexcept;

// 0x0045FCEF..0x0045FD00: values mapped to the no-side-effect
// 0x004602A3 default return (including values above the 31-entry table).
[[nodiscard]] bool is_legacy_battle_frame_input_default_message(
    compat::u32 message_state
) noexcept;

// 0x00460527..0x0046054D: ESI=1 and EDI=0 after the mouse/hotspot prefix.
[[nodiscard]] bool is_legacy_battle_frame_input_case_three_blocked(
    const LegacyBattleFrameInputResolutionState& state
) noexcept;

// Case 5/8 horizontal guards; call only for these two message values.
[[nodiscard]] bool is_legacy_battle_frame_input_row_x_outside(
    compat::u32 message_state, compat::u32 mouse_x
) noexcept;

// Case 2/4 reset their separate hovered slots before the vertical branch.
void reset_legacy_battle_frame_input_case_two_hover_prefix(
    LegacyBattleFrameInputResolutionState& state
) noexcept;
void reset_legacy_battle_frame_input_case_four_hover_prefix(
    LegacyBattleFrameInputResolutionState& state
) noexcept;

enum class LegacyBattleFrameInputCaseZeroGateStatus : compat::u8 {
    returned_zero_preserving_selection,
    returned_zero_clearing_selection,
    continue_at_party_source,
};

[[nodiscard]] LegacyBattleFrameInputCaseZeroGateStatus
run_legacy_battle_frame_input_case_zero_gate_prefix(
    const LegacyBattleActorMetricState& metrics,
    const LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleInputDispatchState& input_dispatch,
    compat::u32 mouse_y
) noexcept;

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
    actor_frame_snapshot_typed_stop,
    actor_frame_resource_typed_stop,
    actor_frame_resource_object_typed_stop,
    image_source_typed_stop,
};

struct LegacyBattleFrameInputCaseZeroPartyResult {
    LegacyBattleFrameInputResolutionStatus status{
        LegacyBattleFrameInputResolutionStatus::completed
    };
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

// Call only after the case-zero count/Y gate. The physical offset read begins
// at 0x004A75A8: indices 8..17 alias the adjacent party-source dwords.
[[nodiscard]] LegacyBattleFrameInputCaseZeroPartyResult
run_legacy_battle_frame_input_case_zero_party_prefix(
    const LegacyBattleStartupState& startup,
    const LegacyBattleActorMetricState& metrics,
    LegacyBattleInputDispatchState& input_dispatch,
    compat::u32 mouse_x,
    compat::u32 entry_eax
) noexcept;

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
    compat::u32 actor_frame_snapshot_queries{};
    compat::u32 actor_frame_resource_queries{};
    compat::u32 actor_frame_resource_caller{};
    compat::u32 action_six_availability_queries{};
    LegacyBattleActorFrameSnapshotResult actor_frame_snapshot{};
    LegacyBattleActorFrameResourceResult actor_frame_resource{};
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
