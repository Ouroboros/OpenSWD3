#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_frame_deformation.hpp"
#include "openswd3/battle/legacy_battle_retreat_commit.hpp"
#include "openswd3/battle/legacy_battle_color_accumulation.hpp"
#include "openswd3/battle/legacy_battle_reward_scale.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/battle/legacy_battle_particle_frame.hpp"
#include "openswd3/battle/legacy_battle_attack_order_entry.hpp"
#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"
#include "openswd3/battle/legacy_battle_attack_order_remove.hpp"
#include "openswd3/battle/legacy_battle_frame_effect.hpp"
#include "openswd3/battle/legacy_battle_pair_transition.hpp"
#include "openswd3/battle/legacy_battle_frame_refresh.hpp"
#include "openswd3/battle/legacy_battle_outcome_state.hpp"
#include "openswd3/battle/legacy_battle_actor_list_query.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution.hpp"
#include "openswd3/battle/legacy_battle_group_a_actor_cleanup.hpp"
#include "openswd3/battle/legacy_battle_group_a_attribute_effect.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/battle/legacy_battle_group_b_order.hpp"
#include "openswd3/battle/legacy_battle_player_item_quantity.hpp"
#include "openswd3/battle/legacy_battle_scale_scan.hpp"
#include "openswd3/battle/legacy_battle_shared_phase.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"
#include "openswd3/battle/legacy_battle_text_message.hpp"
#include "openswd3/battle/legacy_battle_target_ready.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_countdown.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActionGroupABaseToken = 0x005029D0U;
inline constexpr compat::u32 kLegacyBattleActionGroupBBaseToken = 0x00525508U;
inline constexpr compat::u32 kLegacyBattleActionGroupAStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleActionGroupBStride = 0x2B28U;

struct LegacyBattleActionCallRequest {
    compat::u32 callee_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 argument_8{};
    bool has_argument_8{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActionFourOhTwoParticleCallRequest {
    compat::u32 callee_token{};
    std::array<compat::u32, 9> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleActionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, 8> outputs{};
    compat::u32 output_write_mask{};
    bool publish_accumulator{};
    compat::u32 accumulator{};
    bool publish_selection_word{};
    compat::u16 selection_word{};
    bool publish_selection_high_word{};
    compat::u16 selection_high_word{};
    bool publish_opponent_special_action{};
    compat::u16 opponent_special_action{};
    bool publish_opponent_spawn_count{};
    compat::u16 opponent_spawn_count{};
    compat::u32 object_flags{};
    bool publish_metric_byte{};
    compat::u8 metric_byte{};
    bool publish_metric_word{};
    compat::u16 metric_word{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    std::span<compat::u16> resource_words{};
};

class LegacyBattleSummonFramePort {
public:
    virtual ~LegacyBattleSummonFramePort() = default;

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_summon_frame(const LegacyBattleActionCallRequest& request) = 0;
};

class LegacyBattleActionDispatchPort
    : public virtual LegacyBattleSummonFramePort,
      public virtual LegacyBattleRetreatCommitPort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattlePairTransitionPort,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual LegacyBattleOutcomeResolutionStatePort,
      public virtual LegacyBattleFrameRefreshStatePort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public virtual LegacyBattleGroupASummonMaterializationPort,
      public virtual world_map::LegacyWorldItemListStatePort {
public:
    virtual ~LegacyBattleActionDispatchPort() = default;

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) = 0;

    [[nodiscard]] virtual bool group_b_action_configuration_typed_stop(
        compat::u32 callee_token
    ) const noexcept {
        static_cast<void>(callee_token);
        return false;
    }

    [[nodiscard]] virtual std::shared_ptr<const std::array<compat::u8, 0xA4>>
    group_b_action_resource_bytes() const {
        return nullptr;
    }

    [[nodiscard]] virtual std::shared_ptr<const std::array<std::byte, 0x28>>
    group_b_action_profile_buffer() const {
        return nullptr;
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_action_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply invoke_special_turn_frame(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_oh_five_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_oh_six_effect_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_oh_six_secondary_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_hundred_primary_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record,
        compat::u32& frame_token,
        compat::u32& render_flags,
        compat::u16& draw_x,
        compat::u16& draw_y
    ) {
        static_cast<void>(record);
        static_cast<void>(frame_token);
        static_cast<void>(render_flags);
        static_cast<void>(draw_x);
        static_cast<void>(draw_y);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_hundred_workspace_update(
        const LegacyBattleActionCallRequest& request,
        std::span<compat::u8> workspace
    ) {
        static_cast<void>(workspace);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_oh_nine_coordinate_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_action_four_oh_two_coordinate_update(
        const LegacyBattleActionCallRequest& request,
        compat::u32& coordinate_x,
        compat::u32& coordinate_y
    ) {
        static_cast<void>(coordinate_x);
        static_cast<void>(coordinate_y);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_action_four_oh_two_particle(
        const LegacyBattleActionFourOhTwoParticleCallRequest& request,
        compat::u16& spawn_count
    ) {
        LegacyBattleActionCallRequest call{
            .callee_token = request.callee_token,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        for (std::size_t index = 0; index < call.arguments.size(); ++index) {
            call.arguments[index] = request.arguments[index];
        }
        call.argument_8 = request.arguments[8U];
        call.has_argument_8 = true;
        static_cast<void>(spawn_count);
        return invoke(call);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_action_four_oh_two_completion(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record
    ) {
        static_cast<void>(record);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_target_ready_particle(
        const LegacyBattleActionFourOhTwoParticleCallRequest& request
    ) {
        LegacyBattleActionCallRequest call{
            .callee_token = request.callee_token,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        for (std::size_t index = 0; index < call.arguments.size(); ++index) {
            call.arguments[index] = request.arguments[index];
        }
        call.argument_8 = request.arguments[8U];
        call.has_argument_8 = true;
        return invoke(call);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_target_ready_completion(
        const LegacyBattleActionCallRequest& request,
        LegacyBattleGroupAActionExecutionState& actor
    ) {
        static_cast<void>(actor);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_action_four_direct_effect_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record,
        compat::u32& frame_token,
        compat::u32& render_flags,
        compat::u16& draw_x,
        compat::u16& draw_y
    ) {
        static_cast<void>(record);
        static_cast<void>(frame_token);
        static_cast<void>(render_flags);
        static_cast<void>(draw_x);
        static_cast<void>(draw_y);
        return invoke(request);
    }

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke_special_four_hundred_effect_update(
        const LegacyBattleActionCallRequest& request,
        asset_runtime::LegacyActionRecord& record,
        compat::u32& frame_token,
        compat::u32& render_flags,
        compat::u16& draw_x,
        compat::u16& draw_y
    ) {
        static_cast<void>(record);
        static_cast<void>(frame_token);
        static_cast<void>(render_flags);
        static_cast<void>(draw_x);
        static_cast<void>(draw_y);
        return invoke(request);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke_summon_frame(const LegacyBattleActionCallRequest& request) override {
        return invoke(request);
    }

    [[nodiscard]] LegacyBattleGroupASummonMaterializationCallReply
    invoke_group_a_summon_materialization(
        const LegacyBattleGroupASummonMaterializationCallRequest& request
    ) override;

    [[nodiscard]] LegacyBattleTextMessageCallReply invoke_text_message(
        const LegacyBattleTextMessageCallRequest& request
    ) override {
        LegacyBattleActionCallRequest call_request{};
        call_request.callee_token =
            request.call == LegacyBattleTextMessageCall::allocate
            ? kLegacyBattleTextMessageAllocateCallToken
            : kLegacyBattleTextMessageLengthCallToken;
        call_request.arguments[0U] = request.argument;
        call_request.eax = request.eax;
        call_request.ecx = request.ecx;
        call_request.edx = request.edx;
        const auto reply = invoke(call_request);
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
};

struct LegacyBattleTargetPhaseState {
    compat::u32 resource_token{};              // actor + 0x255C
    compat::u32 decoded_resource_token{};      // actor + 0x0E14 token view
    LegacyBattleImageParticleEmitter emitter;  // actor + 0x0E14 physical owner
    compat::u16 tick{};                        // actor + 0x2F26
    compat::u32 active_gate{};                 // actor + 0x2AFC
    compat::u8 mode_flags{};                   // actor + 0x2A87
    compat::u32 runtime_gate{};                // actor + 0x2680
    compat::u32 render_toggle_gate{};          // actor + 0x2B08
    std::array<compat::u32, 5> spawn_counters{};  // actor + 0x2EF8
    std::array<compat::u32, 8> block_0df4{};
    asset_runtime::LegacyActionRecord action_record{};  // actor + 0x0500
    std::array<asset_runtime::LegacyActionRecord, 5>
        spawn_action_records{};  // actor + 0x2BC8
};

struct LegacyBattleActionMessageProfile {
    compat::u32 phase_flags{};           // target profile + 0x0020
    compat::u16 phase_limit{};           // target profile + 0x0052
    compat::u16 level{};                 // target profile + 0x0054
    compat::u16 message_code{};          // target profile + 0x0086
    compat::u16 acceptance_threshold{};  // target profile + 0x0088
};

struct LegacyBattleTargetPhaseCheckRequest {
    compat::u32 target_token{};
};

enum class LegacyBattleTargetPhaseCheckStatus : compat::u8 {
    completed,
    target_profile_typed_stop,
    actor_profile_typed_stop,
};

struct LegacyBattleTargetPhaseCheckResult {
    LegacyBattleTargetPhaseCheckStatus status{
        LegacyBattleTargetPhaseCheckStatus::completed
    };
    compat::u32 value_query_calls{};
    compat::u32 random_calls{};
    compat::i32 sampled_metric{};
    compat::i32 sampled_argument{};
    compat::i32 level_delta{};
    compat::u32 return_eax{};
};

struct LegacyBattleTargetPhaseStartRequest {
    compat::u32 target_token{};
    compat::i32 surface_width{};
    compat::i32 surface_height{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTargetPhaseStartStatus : compat::u8 {
    completed,
    target_object_typed_stop,
    resource_object_typed_stop,
    host_surface_typed_stop,
};

struct LegacyBattleTargetPhaseStartResult {
    LegacyBattleTargetPhaseStartStatus status{
        LegacyBattleTargetPhaseStartStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 resource_query_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 decode_calls{};
    compat::u32 property_query_calls{};
    compat::u32 presentation_dwords_zeroed{};
    compat::u32 tail_dwords_zeroed{};
    LegacyBattleHostSurfaceResult host_surface{};
    compat::u32 host_surface_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTargetPhaseAdvanceRequest {
    compat::u32 target_token{};
    compat::u32 time_seed{};
    LegacyBattleImageParticleStackSnapshot spawn_stack_snapshot{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTargetPhaseAdvanceStatus : compat::u8 {
    completed,
    target_object_typed_stop,
    particle_frame_typed_stop,
    spawn_frame_typed_stop,
};

struct LegacyBattleActionThirteenRequest {
    compat::u32 actor_token{};
    compat::u32 opponent_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionThirteenStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleActionThirteenResult {
    LegacyBattleActionThirteenStatus status{
        LegacyBattleActionThirteenStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 line_raster_calls{};
    compat::u32 sample_calls{};
    compat::u32 render_calls{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTargetPhaseSpawnFrameRequest {
    compat::u32 actor_token{};
    compat::u32 action_id{};
    compat::u32 action_variant{};
    compat::u32 slot{};
    compat::u32 target_x{};
    compat::u32 target_y{};
    compat::u32 iterations{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

using LegacyBattleTargetPhaseSpawnFrameStatus =
    LegacyBattleActionThirteenStatus;
using LegacyBattleTargetPhaseSpawnFrameResult =
    LegacyBattleActionThirteenResult;

struct LegacyBattleSummonFrameRequest {
    compat::u32 actor_token{};
    compat::u32 position_x{};
    compat::u32 position_y{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

using LegacyBattleSummonFrameStatus = LegacyBattleActionThirteenStatus;
using LegacyBattleSummonFrameResult = LegacyBattleActionThirteenResult;

using LegacyBattleActionFourteenRequest = LegacyBattleActionThirteenRequest;
using LegacyBattleActionFourteenStatus = LegacyBattleActionThirteenStatus;
using LegacyBattleActionFourteenResult = LegacyBattleActionThirteenResult;

struct LegacyBattleActionTwentyThreeRequest {
    compat::u32 actor_token{};
    compat::u32 opponent_token{};
    compat::u32 skip_primary{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionTwentyThreeStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    phase_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleActionTwentyThreeResult {
    LegacyBattleActionTwentyThreeStatus status{
        LegacyBattleActionTwentyThreeStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActionTwentyThreeMessageRequest {
    compat::u32 actor_token{};
    compat::u32 profile_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionTwentyThreeMessageStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    profile_state_typed_stop,
};

struct LegacyBattleActionTwentyThreeMessageResult {
    LegacyBattleActionTwentyThreeMessageStatus status{
        LegacyBattleActionTwentyThreeMessageStatus::completed
    };
    compat::u32 percent_refresh_calls{};
    compat::u32 random_calls{};
    compat::u32 message_code_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

enum class LegacyBattleActionTwentyFiveReadyStatus : compat::u8 {
    completed,
    target_profile_typed_stop,
};

struct LegacyBattleActionTwentyFiveReadyResult {
    LegacyBattleActionTwentyFiveReadyStatus status{
        LegacyBattleActionTwentyFiveReadyStatus::completed
    };
    compat::u32 return_eax{};
};

struct LegacyBattleActionTwentyFourRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionTwentyFourStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    phase_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleActionTwentyFourResult {
    LegacyBattleActionTwentyFourStatus status{
        LegacyBattleActionTwentyFourStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActionTwentySevenRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionTwentySevenStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    phase_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleActionTwentySevenResult {
    LegacyBattleActionTwentySevenStatus status{
        LegacyBattleActionTwentySevenStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    compat::u32 target_refresh_calls{};
    compat::u32 effect_compute_calls{};
    compat::u32 effect_publish_calls{};
    compat::u32 secondary_record_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::i32 effect_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleDualRecordActionRequest {
    compat::u32 actor_token{};
    compat::u32 coordinate_token{};
    compat::u32 secondary_action_id{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleDualRecordActionStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    phase_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleDualRecordActionResult {
    LegacyBattleDualRecordActionStatus status{
        LegacyBattleDualRecordActionStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleSpecialFiveHundredRequest {
    compat::u32 actor_token{};
    compat::u32 source_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleSpecialFiveHundredStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleSpecialFiveHundredResult {
    LegacyBattleSpecialFiveHundredStatus status{
        LegacyBattleSpecialFiveHundredStatus::completed
    };
    compat::u32 special_update_calls{};
    compat::u32 turn_frame_calls{};
    LegacyBattleColorInitializationResult color_initialization{};
    compat::u32 color_initialization_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleSpecialFourOhFiveRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleSpecialFourOhFiveStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
    phase_state_typed_stop,
};

struct LegacyBattleSpecialFourOhFiveResult {
    LegacyBattleSpecialFourOhFiveStatus status{
        LegacyBattleSpecialFourOhFiveStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    LegacyBattleFrameRefreshResult frame_refresh{};
    compat::u32 frame_refresh_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 effect_update_calls{};
    compat::u32 target_refresh_calls{};
    compat::u32 effect_compute_calls{};
    compat::u32 effect_publish_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::i32 effect_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleSpecialFourOhSixRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u16 stale_stack_word_8{};
    compat::u16 stale_stack_word_6{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleSpecialFourOhSixStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleSpecialFourOhSixResult {
    LegacyBattleSpecialFourOhSixStatus status{
        LegacyBattleSpecialFourOhSixStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    compat::u32 effect_update_calls{};
    compat::u32 target_refresh_calls{};
    compat::u32 effect_compute_calls{};
    compat::u32 effect_publish_calls{};
    compat::u32 secondary_update_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::i32 effect_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTargetEffectRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 mode{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTargetEffectStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleTargetEffectResult {
    LegacyBattleTargetEffectStatus status{
        LegacyBattleTargetEffectStatus::completed
    };
    compat::u32 curve_query_calls{};
    compat::u32 skip_gate_calls{};
    compat::u32 target_refresh_calls{};
    compat::u32 effect_compute_calls{};
    compat::u32 effect_apply_calls{};
    compat::u32 effect_property_calls{};
    compat::u32 port_calls{};
    compat::i32 effect_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleSpecialFourHundredRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleSpecialFourHundredStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    progress_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleSpecialFourHundredResult {
    LegacyBattleSpecialFourHundredStatus status{
        LegacyBattleSpecialFourHundredStatus::completed
    };
    compat::u32 special_update_calls{};
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 workspace_update_calls{};
    compat::u32 effect_update_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 render_calls{};
    compat::u32 target_event_calls{};
    compat::u32 action_record_clears{};
    compat::u32 workspace_bytes_cleared{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActionFourEffectRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionFourEffectStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    progress_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleActionFourEffectResult {
    LegacyBattleActionFourEffectStatus status{
        LegacyBattleActionFourEffectStatus::completed
    };
    compat::u32 special_update_calls{};
    compat::u32 turn_frame_calls{};
    LegacyBattleColorInitializationResult color_initialization{};
    compat::u32 color_initialization_calls{};
    LegacyBattleFrameRefreshResult frame_refresh{};
    compat::u32 frame_refresh_calls{};
    compat::u32 effect_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 render_calls{};
    compat::u32 target_event_calls{};
    compat::u32 action_record_clears{};
    compat::u32 workspace_bytes_cleared{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTargetPropertyChanceRequest {
    compat::u32 value{};
};

struct LegacyBattleTargetPropertyChanceResult {
    compat::u32 random_calls{};
    compat::u32 sampled_value{};
    compat::i32 scaled_value{};
    compat::i32 quotient{};
    compat::u16 threshold{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActionFourOhTwoRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleActionFourOhTwoStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
};

struct LegacyBattleActionFourOhTwoResult {
    LegacyBattleActionFourOhTwoStatus status{
        LegacyBattleActionFourOhTwoStatus::completed
    };
    compat::u32 special_update_calls{};
    compat::u32 turn_frame_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 coordinate_update_calls{};
    compat::u32 particle_spawn_calls{};
    compat::u32 particle_commit_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 completion_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleSpecialFourOhNineRequest {
    compat::u32 actor_token{};
    compat::u32 target_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleSpecialFourOhNineStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleSpecialFourOhNineResult {
    LegacyBattleSpecialFourOhNineStatus status{
        LegacyBattleSpecialFourOhNineStatus::completed
    };
    compat::u32 special_update_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 coordinate_update_calls{};
    compat::u32 stage_two_calls{};
    compat::u32 action_record_clears{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTargetPhaseAdvanceResult {
    LegacyBattleTargetPhaseAdvanceStatus status{
        LegacyBattleTargetPhaseAdvanceStatus::completed
    };
    LegacyBattleImageParticleFrameResult particle_frame{};
    compat::u32 particle_frame_calls{};
    compat::u32 spawn_calls{};
    std::array<LegacyBattleTargetPhaseSpawnFrameResult, 5> spawn_frames{};
    compat::u32 spawn_frame_calls{};
    compat::u32 resource_release_calls{};
    compat::u32 presentation_dwords_zeroed{};
    compat::u32 spawn_counter_clears{};
    compat::u32 tail_dwords_zeroed{};
    compat::u32 port_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTurnCommitChanceRequest {
    compat::u16 candidate{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTurnCommitChanceStatus : compat::u8 {
    completed,
    actor_record_typed_stop,
};

struct LegacyBattleTurnCommitChanceResult {
    LegacyBattleTurnCommitChanceStatus status{
        LegacyBattleTurnCommitChanceStatus::completed
    };
    compat::u32 random_calls{};
    compat::u8 actor_level{};
    compat::i32 difference{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleTurnAdvanceRequest {
    compat::u32 actor_token{};
    compat::u32 argument{};
    compat::u32 sample_handle{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleTurnAdvanceStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    frame_owner_typed_stop,
    shared_state_typed_stop,
};

struct LegacyBattleTurnAdvanceResult {
    LegacyBattleTurnAdvanceStatus status{
        LegacyBattleTurnAdvanceStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 queue_completion_calls{};
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 sample_play_calls{};
    compat::u32 sample_pan_calls{};
    compat::u32 coordinate_query_calls{};
    compat::u32 coordinate_publish_calls{};
    compat::u32 render_calls{};
    compat::u32 action_record_clears{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleActionDispatchState {
    compat::u32 side_mode{};
    compat::i32 group_a_count{};
    compat::i32 group_b_count{};
    compat::i32 available_actor_count{};
    compat::u32 blocking_effect{};

    compat::u16 current_actor_index{0xFFFFU};
    compat::u16 selected_target_index{0xFFFFU};
    compat::u16 stored_group_b_index{0xFFFFU};
    compat::u16 stored_group_a_index{0xFFFFU};
    compat::u32 packed_action_state{};

    compat::u16 selection_word{};
    compat::u16 selection_high_word{};
    compat::u32 selection_source{};
    compat::u32 selection_context{};
    compat::u16 computed_selection_word{};
    compat::i32 signed_action_value{};
    compat::u32 action_pending{};
    compat::u32 action_pending_aux{};
    compat::u32 frame_refresh_pending{};

    compat::u32 action_runtime_flags{};
    compat::u16 side_selection_word{};
    compat::u32 scene_value{};
    compat::u32 input_mode{};
    compat::u32 battle_flags{};
    compat::u32 active_actor_count{};
    compat::u16 active_actor_snapshot{};
    compat::u32 phase_counter{};
    compat::u16 special_phase{};
    compat::u32 phase_condition{};
    compat::u32 phase_condition_aux{};
    compat::u32 phase_terminal{};
    compat::u32 scan_runtime{};
    compat::u32 scan_push_state{};
    compat::u32 scan_dialog_state{};
    compat::u16 scan_word{};
    compat::u32 battle_mode{};
    compat::u32 battle_submode{};

    compat::u32 message_gate{};
    compat::u32 message_aux{};
    compat::u32 choice_state{};
    compat::u32 choice_cursor{};
    compat::u32 choice_commit{};
    compat::u32 overlay_gate{};
    compat::u32 result_mode{};

    compat::u32 packed_actor_counter{};
    compat::u32 group_a_special_count{};
    compat::u16 opponent_special_action{};
    compat::u16 opponent_spawn_count{};
    compat::u32 opponent_processed_counter{};
    compat::u32 mirror_group_b_spawn{};
    std::array<LegacyBattleActionMessageProfile, 8> group_b_message_profiles{};
    std::array<LegacyBattleRewardScaleActorState, 8> group_b_reward_scale{};
    std::array<std::unique_ptr<LegacyBattleGroupAActionExecutionState>, 8>
        group_b_action_execution{};
    std::array<std::unique_ptr<LegacyBattleTargetPhaseState>, 8>
        group_b_target_phases{};
    LegacyBattleImageParticleNodePool target_phase_particle_nodes;
    LegacyBattleImageParticleSharedState target_phase_particle_shared;
    LegacyBattleImageParticleDiagnostics target_phase_particle_diagnostics;
    input_time_rng::LegacyCrtRng target_phase_particle_rng;
    std::array<std::byte, 0xA4> opponent_scratch{};
    std::array<compat::u32, 0x7E> opponent_workspace{};
    compat::u32 active_target_code{};
    compat::u32 active_effect_target{};
    compat::u32 active_effect_gate{};
    compat::u32 post_battle_counter{};
    compat::u32 current_summon_index{};
    compat::u32 summon_packed{};
    compat::u32 summon_gate{};
    compat::u32 summon_x{};
    compat::u32 summon_y{};
    compat::u8 summon_status{};

    compat::u32 temporary_record_flags{};
    compat::u8 temporary_record_mode{};
    std::array<compat::u8, 0x28> temporary_record{};
    asset_runtime::LegacyActionRecord persistent_action_record{};

    std::array<compat::u32, 10> group_a_to_actor{};
    std::array<LegacyBattleGroupAActionExecutionState, 10>
        group_a_action_execution{};
    std::array<LegacyBattleTargetPhaseState, 10> group_a_target_phases{};
    LegacyBattleGroupAActionExecutionSharedState group_a_action_shared{};
    std::array<compat::u32, 18> target_identity{};
    std::array<compat::u32, 18> selection_workspace{};
    std::array<compat::u32, 18> selected_group_b_identity{};
    std::array<compat::u32, 10> actor_effect_score{};
    std::array<compat::u32, 10> summon_runtime{};
    std::array<compat::u16, 10> summon_target_x{};
    std::array<compat::u16, 10> summon_target_y{};
    std::array<compat::u16, 60> group_a_event_slots_tail{};
    std::array<compat::u16, 8> group_b_status_words{};
    std::array<compat::u16, 10> group_a_status_words{};

    std::unique_ptr<asset_runtime::LegacyDeformationNode> deformation;
    compat::u32 deformation_owner_token{};
    bool deformation_active{};

    LegacyBattleFrameEffectState frame_effect{};
    LegacyBattleStatusIndicatorState status_indicator{};
    LegacyBattleScaleScanState scale_scan{};
    rendering::LegacyCountdownState countdown{};
};

struct LegacyBattleTargetSelectionRuntimeState;
struct LegacyBattleFinalActorStepState;
struct LegacyBattleStartupState;

struct LegacyBattleActionDispatchContext {
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    LegacyBattleBoundedRandomPort& bounded_random;
    LegacyBattleIndicatorSoundPort& indicator_sound;
    rendering::LegacyCountdownFlagPorts& countdown_flags;
    std::span<compat::u8> internal_flags;
    LegacyBattleStartupState* startup{};
    LegacyBattleStartupResetBlocks* startup_reset{};
    LegacyBattleTextMessageState* text_messages{};
    std::span<LegacyBattleStartupResetRecord> attack_order_records;
    std::span<compat::u32> attack_order_party_sources;
    compat::u32* attack_order_primary_gate{};
    compat::u32* attack_order_secondary_gate{};
    LegacyBattleIntensityEffectRecord* attack_order_adjacent_record{};
    compat::u32 status_indicator_action_eax_snapshot{};
    LegacyBattleActionDispatchState* shared_action_dispatch{};
    LegacyBattleFinalActorStepState* shared_final_actor{};
    LegacyBattleTargetSelectionRuntimeState* target_selection_runtime{};
    std::span<const compat::u32> group_a_skip_primary;
    std::span<const compat::u32> group_a_skip_secondary;
    compat::u32 target_phase_time_seed{};
    LegacyBattleImageParticleStackSnapshot target_phase_spawn_stack_snapshot{};
    bool scripted_resource_release_test_compat{};
};

enum class LegacyBattleActionDispatchStatus : compat::u8 {
    completed,
    group_a_index_typed_stop,
    group_b_index_typed_stop,
    actor_map_typed_stop,
    target_table_typed_stop,
    effect_score_typed_stop,
    target_object_typed_stop,
    event_slot_typed_stop,
    framebuffer_typed_stop,
    status_indicator_typed_stop,
    scale_scan_typed_stop,
    internal_flag_typed_stop,
    effect_record_typed_stop,
    actor_metric_typed_stop,
    actor_order_typed_stop,
    group_b_order_typed_stop,
    final_actor_workspace_typed_stop,
    final_actor_record_typed_stop,
    group_b_coordinate_offset_typed_stop,
    final_actor_descriptor_typed_stop,
    player_item_typed_stop,
    text_message_typed_stop,
    actor_target_preparation_typed_stop,
    attack_order_typed_stop,
    attack_order_insert_typed_stop,
    attack_order_remove_typed_stop,
    summon_materialization_typed_stop,
    group_a_attribute_effect_typed_stop,
    group_a_action_execution_typed_stop,
    group_a_actor_cleanup_typed_stop,
    group_a_final_processing_typed_stop,
    group_a_actor_list_action_typed_stop,
    group_a_mode_four_finalization_typed_stop,
    target_phase_check_typed_stop,
    target_phase_start_typed_stop,
    target_phase_advance_typed_stop,
    action_thirteen_typed_stop,
    action_fourteen_typed_stop,
    action_twenty_three_typed_stop,
    action_twenty_three_message_typed_stop,
    action_twenty_four_typed_stop,
    action_twenty_five_ready_typed_stop,
    action_twenty_seven_typed_stop,
    dual_record_action_typed_stop,
    special_five_hundred_typed_stop,
    special_four_oh_five_typed_stop,
    special_four_oh_six_typed_stop,
    special_four_hundred_typed_stop,
    action_four_effect_typed_stop,
    action_four_oh_two_typed_stop,
    special_four_oh_nine_typed_stop,
    target_ready_typed_stop,
    summon_frame_typed_stop,
    turn_commit_chance_typed_stop,
    turn_advance_typed_stop,
    group_b_progress_typed_stop,
    group_b_action_configuration_typed_stop,
};

struct LegacyBattleActionDispatchResult {
    LegacyBattleActionDispatchStatus status{
        LegacyBattleActionDispatchStatus::completed
    };
    compat::u32 return_value{};
    compat::u16 action_code{};
    compat::u32 port_calls{};
    compat::u32 framebuffer_clear_calls{};
    compat::u32 group_a_iterations{};
    compat::u32 group_b_iterations{};
    compat::u32 terminal_resets{};
    LegacyBattleStatusIndicatorResult status_indicator{};
    compat::u32 status_indicator_calls{};
    LegacyBattleScaleScanResult scale_scan{};
    compat::u32 scale_scan_calls{};
    compat::u32 action_record_clear_calls{};
    LegacyBattlePlayerItemQuantityResult player_item{};
    compat::u32 player_item_calls{};
    std::vector<LegacyBattleTextMessageResult> text_messages;
    compat::u32 text_message_calls{};
    compat::u32 actor_target_preparation_calls{};
    LegacyBattlePairTransitionResult pair_transition{};
    compat::u32 pair_transition_calls{};
    LegacyBattleRetreatCommitResult retreat_commit{};
    compat::u32 retreat_commit_calls{};
    LegacyBattleAttackOrderEntryResult attack_order{};
    compat::u32 attack_order_calls{};
    LegacyBattleAttackOrderInsertResult attack_order_insert{};
    compat::u32 attack_order_insert_calls{};
    LegacyBattleAttackOrderRemoveResult attack_order_remove{};
    compat::u32 attack_order_remove_calls{};
    LegacyBattleGroupASummonMaterializationResult summon_materialization{};
    compat::u32 summon_materialization_calls{};
    LegacyBattleGroupAAttributeEffectResult group_a_attribute_effect{};
    compat::u32 group_a_attribute_effect_calls{};
    LegacyBattleGroupAActionExecutionResult group_a_action_execution{};
    compat::u32 group_a_action_execution_calls{};
    LegacyBattleGroupAActorCleanupResult group_a_actor_cleanup{};
    compat::u32 group_a_actor_cleanup_calls{};
    LegacyBattleGroupAFinalProcessingResult group_a_final_processing{};
    compat::u32 group_a_final_processing_calls{};
    LegacyBattleActorListActionResult group_a_actor_list_action{};
    compat::u32 group_a_actor_list_action_calls{};
    LegacyBattleActorResourceReleaseResult actor_resource_release{};
    compat::u32 actor_resource_release_calls{};
    LegacyBattleActorModeFourFinalizationResult
        group_a_mode_four_finalization{};
    compat::u32 group_a_mode_four_finalization_calls{};
    LegacyBattleTargetPhaseCheckResult target_phase_check{};
    compat::u32 target_phase_check_calls{};
    LegacyBattleTargetPhaseStartResult target_phase_start{};
    compat::u32 target_phase_start_calls{};
    LegacyBattleTargetPhaseAdvanceResult target_phase_advance{};
    compat::u32 target_phase_advance_calls{};
    LegacyBattleActionThirteenResult action_thirteen{};
    compat::u32 action_thirteen_calls{};
    LegacyBattleActionFourteenResult action_fourteen{};
    compat::u32 action_fourteen_calls{};
    LegacyBattleActionTwentyThreeResult action_twenty_three{};
    compat::u32 action_twenty_three_calls{};
    LegacyBattleActionTwentyThreeMessageResult action_twenty_three_message{};
    compat::u32 action_twenty_three_message_calls{};
    LegacyBattleActionTwentyFourResult action_twenty_four{};
    compat::u32 action_twenty_four_calls{};
    LegacyBattleActionTwentyFiveReadyResult action_twenty_five_ready{};
    compat::u32 action_twenty_five_ready_calls{};
    LegacyBattleActionTwentySevenResult action_twenty_seven{};
    compat::u32 action_twenty_seven_calls{};
    LegacyBattleDualRecordActionResult dual_record_action{};
    compat::u32 dual_record_action_calls{};
    LegacyBattleSpecialFiveHundredResult special_five_hundred{};
    compat::u32 special_five_hundred_calls{};
    LegacyBattleSpecialFourOhFiveResult special_four_oh_five{};
    compat::u32 special_four_oh_five_calls{};
    LegacyBattleSpecialFourOhSixResult special_four_oh_six{};
    compat::u32 special_four_oh_six_calls{};
    LegacyBattleSpecialFourHundredResult special_four_hundred{};
    compat::u32 special_four_hundred_calls{};
    LegacyBattleActionFourEffectResult action_four_effect{};
    compat::u32 action_four_effect_calls{};
    LegacyBattleTargetPropertyChanceResult target_property_chance{};
    compat::u32 target_property_chance_calls{};
    LegacyBattleActionFourOhTwoResult action_four_oh_two{};
    compat::u32 action_four_oh_two_calls{};
    LegacyBattleSpecialFourOhNineResult special_four_oh_nine{};
    compat::u32 special_four_oh_nine_calls{};
    LegacyBattleTargetReadyResult target_ready{};
    compat::u32 target_ready_calls{};
    LegacyBattleSummonFrameResult summon_frame{};
    compat::u32 summon_frame_calls{};
    LegacyBattleTurnCommitChanceResult turn_commit_chance{};
    compat::u32 turn_commit_chance_calls{};
    LegacyBattleTurnAdvanceResult turn_advance{};
    compat::u32 turn_advance_calls{};
};

// sub_4731A0.
[[nodiscard]] LegacyBattleSpecialFourOhFiveResult
advance_legacy_battle_special_four_oh_five(
    LegacyBattleActionDispatchState& dispatch,
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleSpecialFourOhFiveRequest& request
);

// sub_474E60.
[[nodiscard]] LegacyBattleSpecialFourOhNineResult
advance_legacy_battle_special_four_oh_nine(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleSpecialFourOhNineRequest& request
);

// sub_474BA0.
[[nodiscard]] LegacyBattleActionFourOhTwoResult
advance_legacy_battle_action_four_oh_two(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleActionFourOhTwoRequest& request
);

// sub_474B60.
[[nodiscard]] LegacyBattleTargetPropertyChanceResult
check_legacy_battle_target_property_chance(
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleTargetPropertyChanceRequest& request
);

// sub_4745B0.
[[nodiscard]] LegacyBattleActionFourEffectResult
advance_legacy_battle_action_four_effect(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorProgressState* progress,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionFourEffectRequest& request
);

// sub_474FC0.
[[nodiscard]] LegacyBattleTargetEffectResult apply_legacy_battle_target_effect(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetEffectRequest& request
);

// sub_473C10.
[[nodiscard]] LegacyBattleSpecialFourHundredResult
advance_legacy_battle_special_four_hundred(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorProgressState* progress,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleSpecialFourHundredRequest& request
);

// sub_4735B0.
[[nodiscard]] LegacyBattleSpecialFourOhSixResult
advance_legacy_battle_special_four_oh_six(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleSpecialFourOhSixRequest& request
);

// sub_472730.
[[nodiscard]] LegacyBattleTargetPhaseCheckResult
check_legacy_battle_target_phase(
    const LegacyBattleGroupAActionExecutionState* actor,
    const LegacyBattleActionMessageProfile* target_profile,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseCheckRequest& request
);

// sub_4710D0.
[[nodiscard]] LegacyBattleTargetPhaseStartResult
start_legacy_battle_target_phase(
    LegacyBattleTargetPhaseState* phase,
    const LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleRenderGeometry* render_geometry,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseStartRequest& request
);

// sub_471270.
[[nodiscard]] LegacyBattleTargetPhaseAdvanceResult
advance_legacy_battle_target_phase(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* action_shared,
    asset_runtime::LegacyActionUpdater* action_updater,
    rendering::LegacyFramePieceProvider* frame_provider,
    LegacyBattleImageParticleNodePool* nodes,
    input_time_rng::LegacyCrtRng* rng,
    LegacyBattleImageParticleSharedState* shared,
    LegacyBattleImageParticleDiagnostics* diagnostics,
    const LegacyBattleImageParticleSurface& surface,
    rendering::LegacyPixelConversionState* pixel_format,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleTargetPhaseAdvanceRequest& request
);

// sub_4717F0.
[[nodiscard]] LegacyBattleActionThirteenResult
advance_legacy_battle_action_thirteen(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionThirteenRequest& request
);

// sub_471AD0.
[[nodiscard]] LegacyBattleActionFourteenResult
advance_legacy_battle_action_fourteen(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionFourteenRequest& request
);

// sub_4721F0.
[[nodiscard]] LegacyBattleActionTwentyThreeResult
advance_legacy_battle_action_twenty_three(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentyThreeRequest& request
);

// sub_472430.
[[nodiscard]] LegacyBattleActionTwentyThreeMessageResult
consume_legacy_battle_action_twenty_three_message(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActionMessageProfile* profile,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleActionTwentyThreeMessageRequest& request
);

// sub_4724D0.
[[nodiscard]] LegacyBattleActionTwentyFourResult
advance_legacy_battle_action_twenty_four(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentyFourRequest& request
);

// sub_4728E0.
[[nodiscard]] LegacyBattleActionTwentySevenResult
advance_legacy_battle_action_twenty_seven(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleActionTwentySevenRequest& request
);

// sub_472CE0.
[[nodiscard]] LegacyBattleDualRecordActionResult
advance_legacy_battle_dual_record_action(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    const LegacyBattleDualRecordActionRequest& request
);

// sub_473010.
[[nodiscard]] LegacyBattleSpecialFiveHundredResult
advance_legacy_battle_special_five_hundred(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleActionDispatchPort& port,
    const LegacyBattleSpecialFiveHundredRequest& request
);

// sub_472710.
[[nodiscard]] LegacyBattleActionTwentyFiveReadyResult
query_legacy_battle_action_twenty_five_ready(
    const LegacyBattleActionMessageProfile* target_profile
) noexcept;

// sub_471FC0.
[[nodiscard]] LegacyBattleTargetPhaseSpawnFrameResult
advance_legacy_battle_target_phase_spawn_frame(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleSummonFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleTargetPhaseSpawnFrameRequest& request
);

// sub_471D60.
[[nodiscard]] LegacyBattleSummonFrameResult advance_legacy_battle_summon_frame(
    LegacyBattleTargetPhaseState* phase,
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleGroupAActionExecutionSharedState* shared,
    LegacyBattleSummonFramePort& port,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleSummonFrameRequest& request
);

// sub_4539B0: dispatch one action code for the selected group-A actor and
// group-B target, preserving the original special-code predispatch and the
// sparse 1..36 ordinary switch.
[[nodiscard]] LegacyBattleActionDispatchResult dispatch_legacy_battle_action(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    compat::u32 group_a_index,
    compat::u32 group_b_index
);

}  // namespace openswd3::battle
