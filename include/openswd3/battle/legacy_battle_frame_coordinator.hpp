#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_actor_priority.hpp"
#include "openswd3/battle/legacy_battle_actor_frame_sequence.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"
#include "openswd3/battle/legacy_battle_pending_action_commit.hpp"
#include "openswd3/battle/legacy_battle_color_accumulation.hpp"
#include "openswd3/battle/legacy_battle_context_prompt.hpp"
#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_debug_overlay.hpp"
#include "openswd3/battle/legacy_battle_debug_status_panel.hpp"
#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "openswd3/battle/legacy_battle_frame_completion.hpp"
#include "openswd3/battle/legacy_battle_frame_effect.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_message_phase.hpp"
#include "openswd3/battle/legacy_battle_text_message_frame.hpp"
#include "openswd3/battle/legacy_battle_outcome_resolution.hpp"
#include "openswd3/battle/legacy_battle_pre_frame.hpp"
#include "openswd3/battle/legacy_battle_selection_frame.hpp"
#include "openswd3/battle/legacy_battle_transition.hpp"
#include "openswd3/battle/legacy_battle_vertical_shift.hpp"
#include "openswd3/rendering/legacy_action_renderers.hpp"
#include "openswd3/rendering/legacy_bmp_writer.hpp"
#include "openswd3/rendering/legacy_countdown.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_role_head_actions.hpp"

#include <array>
#include <bit>
#include <filesystem>
#include <list>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFrameCoordinatorTargetSurfaceToken =
    0x004ACBA0U;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorSurfaceOwnerToken =
    0x004AB870U;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorSurfaceFormat =
    0x2711U;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorFrameResource =
    0x234DU;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorPanelAction = 0x233BU;
inline constexpr compat::u32 kLegacyBattleFrameCoordinatorStandaloneAction =
    0x2391U;

struct LegacyBattleFrameCoordinatorPosition {
    compat::i32 x{};
    compat::i32 y{};
};

enum class LegacyBattleFrameCoordinatorCall : compat::u8 {
    query_music_gate,
    music_commit,
    reserved_frame_input_resolution_slot,
    reserved_input_dispatch_slot,
    frame_input_validate_option_actor,
    frame_input_configure_actor_selection,
    frame_input_query_group_b_candidate,
    frame_input_prepare_actor_origin,
    frame_input_resolve_actor_surface,
    frame_input_query_actor_mirror,
    reserved_frame_input_query_group_b_action_six_availability_slot,
    frame_input_query_group_a_candidate,
    query_actor_metric,
    lock_target_surface,
    unlock_target_surface,
    reserved_refresh_selection_slot,
    reserved_selection_frame_slot,
    query_actor_pair,
    reserved_frame_completion_slot,
    reserved_pending_action_commit_slot,
    actor_ready_query,
    post_render_stage_1,
    reserved_message_phase_slot,
    reserved_text_message_frame_slot,
    reserved_post_dialog_stage_slot,
    reserved_debug_overlay_slot,
    reserved_outcome_resolution_slot,
    reserved_context_prompt_slot,
    finalize_overlay,
    reserved_vertical_shift_slot,
    pending_action_prepare_actor,
    pending_action_ready_query,
    pending_action_commit_actor,
    reserved_pending_action_remove_actor_record,
    frame_completion_query_actor,
    attack_order_dequeue_query_actor,
    selection_frame_query_group_a_replacement,
    reserved_selection_frame_prepare_selected_actor_slot,
    selection_frame_query_selected_actor_release,
    selection_frame_release_selected_actor,
    selection_frame_reset_actor_selection,
    selection_frame_draw_mouse_anchor,
    selection_frame_configure_text_row,
    selection_frame_configure_text_color,
    selection_frame_query_text_length,
    selection_frame_draw_text,
    reserved_selection_frame_draw_action_summary_slot,
    reserved_selection_frame_draw_list_frame_slot,
    reserved_selection_frame_draw_list_contents_slot,
    reserved_selection_frame_draw_grid_frame_slot,
    reserved_selection_frame_draw_narrow_frame_slot,
    reserved_selection_frame_draw_grid_alternate_slot,
    reserved_selection_frame_draw_grid_mode_slot,
    reserved_selection_frame_draw_message_five_slot,
    reserved_selection_frame_draw_message_seven_slot,
    selection_frame_configure_text_font,
    selection_frame_query_group_b_completion,
    selection_frame_query_group_a_completion,
    selection_frame_build_actor_snapshot,
    selection_frame_query_actor_origin,
    selection_frame_query_target_action_available,
    reserved_selection_frame_draw_selection_hint_slot,
    actor_target_prepare_group_a_actor,
    actor_target_query_group_b_completion,
    action_summary_configure_font_reset,
    action_summary_configure_font_style,
    action_summary_query_actor_special_gate,
    action_summary_draw_text,
    action_summary_query_action_available,
    action_summary_action_mode_query_primary_actor,
    action_summary_action_mode_query_secondary_actor,
    action_summary_action_mode_query_active_actor,
    list_contents_initialize_rows,
    list_contents_refresh_actor,
    list_contents_query_row,
    list_contents_resolve_negative_row,
    list_contents_resolve_regular_row,
    grid_frame_initialize_rows,
    grid_frame_query_row,
    alternate_grid_frame_query_row,
    mode_grid_frame_query_row,
    mode_grid_frame_query_secondary_count,
    narrow_grid_frame_initialize_rows,
    narrow_grid_frame_query_row,
    guard_panel_query_actor_label,
    selection_hint_query_actor_label,
    selection_hint_configure_font_width,
    selection_hint_draw_text,
    selection_hint_query_metric_source,
    selection_hint_resolve_metric_value,
    selection_hint_query_metric_pair,
    selection_hint_query_fade_width,
    selection_hint_query_fade_color,
    control_panel_configure_font_reset,
    control_panel_configure_font_style,
    control_panel_draw_text,
    reserved_control_panel_query_primary_option_slot,
    reserved_control_panel_query_special_option_slot,
    reserved_message_phase_resolve_group_a_position_slot,
    reserved_message_phase_prepare_message_98_slot,
    message_phase_reset_actor_state,
    message_phase_query_actor_completion,
    reserved_message_phase_prepare_transition_control_slot,
    message_phase_prepare_group_a_actor,
    reserved_message_phase_reset_group_a_actor_slot,
    message_phase_set_group_a_actor_mode,
    message_phase_commit_active_actor,
    message_phase_configure_actor_action,
    message_phase_refresh_actor_message_percent,
    reserved_message_phase_resolve_action_item_slot,
    reserved_message_phase_victory_reward_slot,
    victory_begin_music_fade,
    victory_stop_all_samples,
    reserved_victory_query_group_b_item,
    victory_query_group_a_reward_block,
    reserved_victory_apply_group_a_reward,
    victory_prepare_group_a_actor,
    victory_configure_group_a_actor,
    victory_reserved_transition_stage_advance_slot,
    victory_format_level_up_text,
    victory_draw_text,
    reserved_level_advancement_query_requirement,
    level_advancement_query_requirement =
        reserved_level_advancement_query_requirement,
    level_advancement_build_profile,
    level_advancement_stop_sample,
    level_advancement_play_sample,
    level_growth_reserved_transition_stage_advance_slot,
    level_growth_format_integer,
    level_growth_draw_text,
    level_growth_play_sample,
    growth_actor_query_group_a_reward_block,
    reserved_growth_actor_load_item_definition,
    growth_actor_load_item_definition =
        reserved_growth_actor_load_item_definition,
    growth_actor_query_item_presence,
    growth_actor_allocate_item_node,
    growth_item_result_query_actor_completion,
    reserved_growth_item_result_select_item,
    reserved_growth_item_result_load_definition,
    growth_item_result_load_definition =
        reserved_growth_item_result_load_definition,
    growth_item_result_release_description,
    growth_item_result_copy_caption,
    growth_item_completion_format_text,
    growth_item_completion_measure_text,
    growth_item_completion_reserved_transition_stage_advance_slot,
    growth_item_completion_set_font_size,
    growth_item_completion_draw_text,
    growth_caption_format_name,
    growth_caption_reserved_transition_stage_advance_slot,
    growth_caption_draw_text,
    growth_caption_format_detail,
    growth_completion_caption_play_sample,
    message_phase_select_message_101_actor,
    message_phase_allocate_actor_transition,
    message_phase_advance_message_101,
    reserved_message_phase_advance_message_110_slot,
    reserved_message_phase_advance_message_111_slot,
    reserved_message_phase_select_message_112_actor_slot,
    reserved_message_phase_advance_message_112_slot,
    reserved_message_phase_select_message_113_actor_slot,
    reserved_message_phase_advance_message_113_slot,
    reserved_message_phase_advance_message_102_slot,
    reserved_message_phase_advance_message_103_slot,
    message_phase_summon_frame_play_sample,
    message_phase_summon_frame_render,
    victory_item_list_set_font_size,
    victory_item_list_draw_title,
    victory_item_list_reserved_transition_stage_advance_slot,
    victory_item_list_format_row,
    victory_item_list_draw_row,
    defeat_panel_draw_title,
    defeat_panel_reserved_transition_stage_advance_slot,
    defeat_panel_set_font_size,
    defeat_panel_draw_detail,
    talisman_result_reserved_transition_stage_advance_slot,
    talisman_result_draw_success_title,
    talisman_result_format_success_detail,
    talisman_result_draw_success_detail,
    talisman_result_draw_failure_title,
    talisman_result_draw_failure_detail,
    text_message_frame_draw_text,
    text_message_frame_release_node,
    reserved_group_b_action_item_load_definition,
    group_b_action_item_load_definition =
        reserved_group_b_action_item_load_definition,
    group_b_action_item_copy_name,
};

struct LegacyBattleFrameCoordinatorCallRequest {
    LegacyBattleFrameCoordinatorCall call{
        LegacyBattleFrameCoordinatorCall::reserved_frame_input_resolution_slot
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 list_text_token{};
    std::array<compat::u8, 10> list_text_bytes{};
    compat::u32 list_text_length{};
    compat::u32 grid_text_token{};
    std::array<compat::u8, 20> grid_text_bytes{};
    compat::u32 grid_text_length{};
    compat::u32 control_text_token{};
    std::array<compat::u8, 24> control_text_bytes{};
    compat::u32 control_text_length{};
    std::array<compat::u8, 64> victory_text_bytes{};
    compat::u32 victory_text_length{};
    std::array<compat::u8, 64> growth_text_bytes{};
    compat::u32 growth_text_length{};
    std::array<compat::u8, 64> caption_text_bytes{};
    compat::u32 caption_text_length{};
};

struct LegacyBattleFrameCoordinatorCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 published_value{};
    bool publish_metric_byte{};
    compat::u8 metric_byte{};
    bool publish_metric_word{};
    compat::u16 metric_word{};
    bool publish_group_b_count{};
    compat::u32 group_b_count{};
    bool publish_group_a_count{};
    compat::u32 group_a_count{};
    compat::i32 origin_x{};
    compat::i32 origin_y{};
    compat::i32 selection_snapshot_x{};
    compat::i32 selection_snapshot_y{};
    compat::i32 selection_snapshot_width{};
    compat::i32 selection_snapshot_height{};
    compat::u16 selection_origin_x{};
    compat::u16 selection_origin_y{};
    bool publish_list_panel_row_limit{};
    compat::u8 list_panel_row_limit{};
    bool publish_list_row_value{};
    compat::u32 list_row_value{};
    bool publish_list_limit_word{};
    compat::u16 list_limit_word{};
    bool publish_list_limit_byte{};
    compat::u8 list_limit_byte{};
    bool publish_grid_panel_row_limit{};
    compat::u16 grid_panel_row_limit{};
    bool publish_grid_row_flags{};
    compat::u32 grid_row_flags{};
    bool publish_grid_row_value{};
    compat::u32 grid_row_value{};
    bool publish_grid_row_text{};
    std::array<compat::u8, 20> grid_row_text{};
    compat::u32 selection_hint_text_length{};
    bool publish_selection_hint_metric_pair{};
    compat::u32 selection_hint_metric_current{};
    compat::u32 selection_hint_metric_limit{};
    bool publish_control_text{};
    std::array<compat::u8, 24> control_text{};
    bool publish_control_primary_value{};
    compat::u32 control_primary_value{};
    bool publish_message_phase_message{};
    compat::u32 message_phase_message{};
    bool publish_message_phase_actor_index{};
    compat::u8 message_phase_actor_index{};
    bool publish_message_phase_control_words{};
    compat::u32 message_phase_control_words{};
    bool publish_message_phase_transition_state{};
    compat::u32 message_phase_transition_state{};
    bool publish_message_phase_timer{};
    compat::u32 message_phase_timer{};
    bool publish_message_phase_sample_word{};
    compat::u16 message_phase_sample_word{};
    bool publish_message_phase_aux_byte{};
    compat::u8 message_phase_aux_byte{};
    bool publish_message_phase_completion_gate{};
    compat::u32 message_phase_completion_gate{};
    bool publish_message_phase_special_count{};
    compat::u32 message_phase_special_count{};
    bool publish_message_phase_target_ready{};
    compat::u32 message_phase_target_ready{};
    bool publish_message_phase_mode_gate{};
    compat::u32 message_phase_mode_gate{};
    bool publish_message_phase_group_b_bypass{};
    compat::u32 message_phase_group_b_bypass{};
    bool message_phase_action_item_typed_stop{};
    std::shared_ptr<const std::array<compat::u8, 0xA4>>
        message_phase_action_item_definition{};
    bool publish_victory_item_count{};
    compat::u16 victory_item_count{};
    bool publish_victory_reward_words{};
    compat::u16 victory_committed_money_word{};
    compat::u16 victory_experience_per_party_member{};
    compat::u16 victory_reward_experience{};
    std::array<compat::u8, 64> victory_formatted_text{};
    compat::u32 victory_formatted_text_length{};
    bool publish_victory_item_list_text{};
    std::array<compat::u8, 64> victory_item_list_text{};
    compat::u32 victory_item_list_text_length{};
    bool publish_level_profile{};
    world_map::LegacyWorldStoryPartyMemberResources level_profile{};
    bool publish_level_transition_mode{};
    compat::u32 level_transition_mode{};
    bool publish_growth_transition_actor_index{};
    compat::u8 growth_transition_actor_index{};
    bool publish_growth_item_definition{};
    std::array<compat::u8, world_map::kLegacyItemDefinitionSnapshotBytes>
        growth_item_definition{};
    std::array<compat::u8, 256U> growth_item_description{};
    compat::u32 growth_item_description_length{};
    bool growth_item_allocation_failed{};
    bool publish_growth_item_allocation_token{};
    compat::u32 growth_item_allocation_token{};
    bool publish_growth_item_formatted_text{};
    std::array<compat::u8, 64U> growth_item_formatted_text{};
    compat::u32 growth_item_formatted_text_length{};
    bool publish_growth_item_measured_length{};
    compat::u32 growth_item_measured_length{};
    bool publish_growth_transition_stage{};
    compat::u32 growth_transition_stage{};
    bool publish_growth_formatted_text{};
    std::array<compat::u8, 64> growth_formatted_text{};
    compat::u32 growth_formatted_text_length{};
    bool publish_caption_transition_actor_index{};
    compat::u8 caption_transition_actor_index{};
    bool publish_caption_transition_stage{};
    compat::u32 caption_transition_stage{};
    bool publish_caption_formatted_text{};
    std::array<compat::u8, 64> caption_formatted_text{};
    compat::u32 caption_formatted_text_length{};
    LegacyBattleFrameInputSurface actor_surface{};
};

class LegacyBattleFrameCoordinatorPort
    : public LegacyBattleHudCallPort,
      public LegacyBattleEffectCallPort,
      public LegacyBattlePreFramePort,
      public LegacyBattleDebugHotkeyPort,
      public LegacyBattleDebugOverlayPort,
      public LegacyBattleOutcomeResolutionPort,
      public LegacyBattleContextPromptPort,
      public LegacyBattleVerticalShiftPort,
      public LegacyBattleAttackOrderDequeuePort,
      public LegacyBattlePendingActionPort,
      public LegacyBattleFrameCompletionPort,
      public LegacyBattleFrameInputResolutionPort,
      public LegacyBattleSelectionFramePort,
      public LegacyBattleMessagePhasePort,
      public LegacyBattleTextMessageFramePort,
      public virtual LegacyBattleEffectCoordinatorStatePort {
public:
    using LegacyBattleEffectCallPort::invoke;
    using LegacyBattleOutcomeFinalizationPort::invoke;

    virtual ~LegacyBattleFrameCoordinatorPort() = default;

    [[nodiscard]] LegacyBattleTextMessageFrameCallReply
    invoke_text_message_frame(
        const LegacyBattleTextMessageFrameCallRequest& request
    ) override {
        const auto call =
            request.call == LegacyBattleTextMessageFrameCall::draw_text
            ? LegacyBattleFrameCoordinatorCall::text_message_frame_draw_text
            : LegacyBattleFrameCoordinatorCall::text_message_frame_release_node;
        const auto reply = invoke({
            .call = call,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

    [[nodiscard]] virtual LegacyBattleFrameCoordinatorCallReply
    invoke(const LegacyBattleFrameCoordinatorCallRequest& request) = 0;

    [[nodiscard]] LegacyBattleGroupBActionItemNameCopyReply
    copy_action_item_name(
        const LegacyBattleGroupBActionItemNameCopyRequest& request
    ) override {
        const auto reply = invoke({
            .call =
                LegacyBattleFrameCoordinatorCall::group_b_action_item_copy_name,
            .object_token = request.source_token,
            .arguments = {request.destination_token, request.source_token},
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = reply.message_phase_action_item_typed_stop,
        };
    }

    [[nodiscard]] LegacyBattleSelectionFrameCallReply invoke_selection_frame(
        const LegacyBattleSelectionFrameCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                selection_frame_query_group_a_replacement;
        switch (request.call) {
        case LegacyBattleSelectionFrameCall::query_group_a_replacement:
            break;
        case LegacyBattleSelectionFrameCall::
            reserved_prepare_selected_actor_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_prepare_selected_actor_slot;
            break;
        case LegacyBattleSelectionFrameCall::query_selected_actor_release:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_query_selected_actor_release;
            break;
        case LegacyBattleSelectionFrameCall::release_selected_actor:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_release_selected_actor;
            break;
        case LegacyBattleSelectionFrameCall::reset_actor_selection:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_reset_actor_selection;
            break;
        case LegacyBattleSelectionFrameCall::draw_mouse_anchor:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_draw_mouse_anchor;
            break;
        case LegacyBattleSelectionFrameCall::configure_text_row:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_row;
            break;
        case LegacyBattleSelectionFrameCall::configure_text_color:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_color;
            break;
        case LegacyBattleSelectionFrameCall::query_text_length:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_query_text_length;
            break;
        case LegacyBattleSelectionFrameCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::selection_frame_draw_text;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_action_summary_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_action_summary_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_list_frame_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_list_frame_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_list_contents_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_list_contents_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_grid_frame_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_grid_frame_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_narrow_frame_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_narrow_frame_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_grid_alternate_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_grid_alternate_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_grid_mode_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_grid_mode_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_message_five_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_message_five_slot;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_message_seven_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_message_seven_slot;
            break;
        case LegacyBattleSelectionFrameCall::configure_text_font:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_font;
            break;
        case LegacyBattleSelectionFrameCall::query_group_b_completion:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_query_group_b_completion;
            break;
        case LegacyBattleSelectionFrameCall::query_group_a_completion:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_query_group_a_completion;
            break;
        case LegacyBattleSelectionFrameCall::build_actor_snapshot:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_build_actor_snapshot;
            break;
        case LegacyBattleSelectionFrameCall::query_actor_origin:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_query_actor_origin;
            break;
        case LegacyBattleSelectionFrameCall::query_target_action_available:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_query_target_action_available;
            break;
        case LegacyBattleSelectionFrameCall::reserved_draw_selection_hint_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_selection_frame_draw_selection_hint_slot;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .snapshot_x = reply.selection_snapshot_x,
            .snapshot_y = reply.selection_snapshot_y,
            .snapshot_width = reply.selection_snapshot_width,
            .snapshot_height = reply.selection_snapshot_height,
            .origin_x = reply.selection_origin_x,
            .origin_y = reply.selection_origin_y,
            .text_length = reply.eax,
        };
    }
    [[nodiscard]] LegacyBattleActionSummaryCallReply invoke_action_summary(
        const LegacyBattleActionSummaryCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                action_summary_configure_font_reset;
        switch (request.call) {
        case LegacyBattleActionSummaryCall::configure_font_reset:
            break;
        case LegacyBattleActionSummaryCall::configure_font_style:
            call = LegacyBattleFrameCoordinatorCall::
                action_summary_configure_font_style;
            break;
        case LegacyBattleActionSummaryCall::query_actor_special_gate:
            call = LegacyBattleFrameCoordinatorCall::
                action_summary_query_actor_special_gate;
            break;
        case LegacyBattleActionSummaryCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::action_summary_draw_text;
            break;
        case LegacyBattleActionSummaryCall::query_action_available:
            call = LegacyBattleFrameCoordinatorCall::
                action_summary_query_action_available;
            break;
        case LegacyBattleActionSummaryCall::action_mode_query_primary_actor:
            call = LegacyBattleFrameCoordinatorCall::
                action_summary_action_mode_query_primary_actor;
            break;
        case LegacyBattleActionSummaryCall::action_mode_query_secondary_actor:
            call = LegacyBattleFrameCoordinatorCall::
                action_summary_action_mode_query_secondary_actor;
            break;
        case LegacyBattleActionSummaryCall::action_mode_query_active_actor:
            call = LegacyBattleFrameCoordinatorCall::
                action_summary_action_mode_query_active_actor;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleListFrameCallReply invoke_list_frame(
        const LegacyBattleListFrameCallRequest& request
    ) override {
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_color,
            .arguments =
                {
                    request.arguments[0],
                    request.arguments[1],
                    request.arguments[2],
                    request.arguments[3],
                },
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleListContentsCallReply invoke_list_contents(
        const LegacyBattleListContentsCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_row;
        switch (request.call) {
        case LegacyBattleListContentsCall::configure_font_mode:
            break;
        case LegacyBattleListContentsCall::configure_font_style:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_color;
            break;
        case LegacyBattleListContentsCall::configure_font_width:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_font;
            break;
        case LegacyBattleListContentsCall::initialize_rows:
            call =
                LegacyBattleFrameCoordinatorCall::list_contents_initialize_rows;
            break;
        case LegacyBattleListContentsCall::refresh_actor:
            call =
                LegacyBattleFrameCoordinatorCall::list_contents_refresh_actor;
            break;
        case LegacyBattleListContentsCall::query_row:
            call = LegacyBattleFrameCoordinatorCall::list_contents_query_row;
            break;
        case LegacyBattleListContentsCall::resolve_negative_row:
            call = LegacyBattleFrameCoordinatorCall::
                list_contents_resolve_negative_row;
            break;
        case LegacyBattleListContentsCall::resolve_regular_row:
            call = LegacyBattleFrameCoordinatorCall::
                list_contents_resolve_regular_row;
            break;
        case LegacyBattleListContentsCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::selection_frame_draw_text;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .list_text_token = request.text_token,
            .list_text_bytes = request.text_bytes,
            .list_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_panel_row_limit = reply.publish_list_panel_row_limit,
            .panel_row_limit = reply.list_panel_row_limit,
            .publish_row_value = reply.publish_list_row_value,
            .row_value = reply.list_row_value,
            .publish_limit_word = reply.publish_list_limit_word,
            .limit_word = reply.list_limit_word,
            .publish_limit_byte = reply.publish_list_limit_byte,
            .limit_byte = reply.list_limit_byte,
        };
    }
    [[nodiscard]] LegacyBattleGridFrameCallReply invoke_grid_frame(
        const LegacyBattleGridFrameCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_row;
        switch (request.call) {
        case LegacyBattleGridFrameCall::configure_font_mode:
            break;
        case LegacyBattleGridFrameCall::configure_font_style:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_color;
            break;
        case LegacyBattleGridFrameCall::configure_font_width:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_configure_text_font;
            break;
        case LegacyBattleGridFrameCall::initialize_rows:
            call = LegacyBattleFrameCoordinatorCall::grid_frame_initialize_rows;
            break;
        case LegacyBattleGridFrameCall::refresh_actor:
            call =
                LegacyBattleFrameCoordinatorCall::list_contents_refresh_actor;
            break;
        case LegacyBattleGridFrameCall::query_row:
            call = LegacyBattleFrameCoordinatorCall::grid_frame_query_row;
            break;
        case LegacyBattleGridFrameCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::selection_frame_draw_text;
            break;
        case LegacyBattleGridFrameCall::query_alternate_row:
            call = LegacyBattleFrameCoordinatorCall::
                alternate_grid_frame_query_row;
            break;
        case LegacyBattleGridFrameCall::query_mode_row:
            call = LegacyBattleFrameCoordinatorCall::mode_grid_frame_query_row;
            break;
        case LegacyBattleGridFrameCall::query_mode_secondary_count:
            call = LegacyBattleFrameCoordinatorCall::
                mode_grid_frame_query_secondary_count;
            break;
        case LegacyBattleGridFrameCall::initialize_narrow_rows:
            call = LegacyBattleFrameCoordinatorCall::
                narrow_grid_frame_initialize_rows;
            break;
        case LegacyBattleGridFrameCall::query_narrow_row:
            call =
                LegacyBattleFrameCoordinatorCall::narrow_grid_frame_query_row;
            break;
        case LegacyBattleGridFrameCall::query_guard_actor_label:
            call =
                LegacyBattleFrameCoordinatorCall::guard_panel_query_actor_label;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .grid_text_token = request.text_token,
            .grid_text_bytes = request.text_bytes,
            .grid_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_panel_row_limit = reply.publish_grid_panel_row_limit,
            .panel_row_limit = reply.grid_panel_row_limit,
            .publish_row_flags = reply.publish_grid_row_flags,
            .row_flags = reply.grid_row_flags,
            .publish_row_value = reply.publish_grid_row_value,
            .row_value = reply.grid_row_value,
            .publish_row_text = reply.publish_grid_row_text,
            .row_text = reply.grid_row_text,
        };
    }
    [[nodiscard]] LegacyBattleSelectionHintFrameCallReply
    invoke_selection_hint_frame(
        const LegacyBattleSelectionHintFrameCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::selection_hint_query_actor_label;
        switch (request.call) {
        case LegacyBattleSelectionHintFrameCall::query_actor_label:
            break;
        case LegacyBattleSelectionHintFrameCall::configure_font_width:
            call = LegacyBattleFrameCoordinatorCall::
                selection_hint_configure_font_width;
            break;
        case LegacyBattleSelectionHintFrameCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::selection_hint_draw_text;
            break;
        case LegacyBattleSelectionHintFrameCall::query_metric_source:
            call = LegacyBattleFrameCoordinatorCall::
                selection_hint_query_metric_source;
            break;
        case LegacyBattleSelectionHintFrameCall::resolve_metric_value:
            call = LegacyBattleFrameCoordinatorCall::
                selection_hint_resolve_metric_value;
            break;
        case LegacyBattleSelectionHintFrameCall::query_metric_pair:
            call = LegacyBattleFrameCoordinatorCall::
                selection_hint_query_metric_pair;
            break;
        case LegacyBattleSelectionHintFrameCall::query_fade_width:
            call = LegacyBattleFrameCoordinatorCall::
                selection_hint_query_fade_width;
            break;
        case LegacyBattleSelectionHintFrameCall::query_fade_color:
            call = LegacyBattleFrameCoordinatorCall::
                selection_hint_query_fade_color;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .grid_text_token = request.text_token,
            .grid_text_bytes = request.text_bytes,
            .grid_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .text_length = reply.selection_hint_text_length,
            .publish_metric_pair = reply.publish_selection_hint_metric_pair,
            .metric_current = reply.selection_hint_metric_current,
            .metric_limit = reply.selection_hint_metric_limit,
        };
    }
    [[nodiscard]] LegacyBattleControlPanelFrameCallReply
    invoke_control_panel_frame(
        const LegacyBattleControlPanelFrameCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                control_panel_configure_font_reset;
        switch (request.call) {
        case LegacyBattleControlPanelFrameCall::configure_font_reset:
            break;
        case LegacyBattleControlPanelFrameCall::configure_font_style:
            call = LegacyBattleFrameCoordinatorCall::
                control_panel_configure_font_style;
            break;
        case LegacyBattleControlPanelFrameCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::control_panel_draw_text;
            break;
        case LegacyBattleControlPanelFrameCall::
            reserved_query_primary_option_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_control_panel_query_primary_option_slot;
            break;
        case LegacyBattleControlPanelFrameCall::
            reserved_query_special_option_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_control_panel_query_special_option_slot;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .control_text_token = request.text_token,
            .control_text_bytes = request.text_bytes,
            .control_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_text = reply.publish_control_text,
            .text = reply.control_text,
            .publish_primary_value = reply.publish_control_primary_value,
            .primary_value = reply.control_primary_value,
        };
    }
    [[nodiscard]] LegacyBattleMessagePhaseCallReply invoke_message_phase(
        const LegacyBattleMessagePhaseCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_resolve_group_a_position_slot;
        switch (request.call) {
        case LegacyBattleMessagePhaseCall::
            reserved_resolve_group_a_position_slot:
            break;
        case LegacyBattleMessagePhaseCall::reserved_prepare_message_98_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_prepare_message_98_slot;
            break;
        case LegacyBattleMessagePhaseCall::reset_actor_state:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_reset_actor_state;
            break;
        case LegacyBattleMessagePhaseCall::query_actor_completion:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_query_actor_completion;
            break;
        case LegacyBattleMessagePhaseCall::
            reserved_prepare_transition_control_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_prepare_transition_control_slot;
            break;
        case LegacyBattleMessagePhaseCall::prepare_group_a_actor:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_prepare_group_a_actor;
            break;
        case LegacyBattleMessagePhaseCall::reserved_reset_group_a_actor_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_reset_group_a_actor_slot;
            break;
        case LegacyBattleMessagePhaseCall::set_group_a_actor_mode:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_set_group_a_actor_mode;
            break;
        case LegacyBattleMessagePhaseCall::commit_active_actor:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_commit_active_actor;
            break;
        case LegacyBattleMessagePhaseCall::configure_actor_action:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_configure_actor_action;
            break;
        case LegacyBattleMessagePhaseCall::refresh_actor_message_percent:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_refresh_actor_message_percent;
            break;
        case LegacyBattleMessagePhaseCall::reserved_resolve_action_item_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_resolve_action_item_slot;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_100_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_victory_reward_slot;
            break;
        case LegacyBattleMessagePhaseCall::select_message_101_actor:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_select_message_101_actor;
            break;
        case LegacyBattleMessagePhaseCall::allocate_actor_transition:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_allocate_actor_transition;
            break;
        case LegacyBattleMessagePhaseCall::advance_message_101:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_advance_message_101;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_110_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_advance_message_110_slot;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_111_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_advance_message_111_slot;
            break;
        case LegacyBattleMessagePhaseCall::
            reserved_select_message_112_actor_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_select_message_112_actor_slot;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_112_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_advance_message_112_slot;
            break;
        case LegacyBattleMessagePhaseCall::
            reserved_select_message_113_actor_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_select_message_113_actor_slot;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_113_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_advance_message_113_slot;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_102_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_advance_message_102_slot;
            break;
        case LegacyBattleMessagePhaseCall::reserved_advance_message_103_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_message_phase_advance_message_103_slot;
            break;
        case LegacyBattleMessagePhaseCall::summon_frame_play_sample:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_summon_frame_play_sample;
            break;
        case LegacyBattleMessagePhaseCall::summon_frame_render:
            call = LegacyBattleFrameCoordinatorCall::
                message_phase_summon_frame_render;
            break;
        case LegacyBattleMessagePhaseCall::reserved_load_action_item_definition:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_group_b_action_item_load_definition;
            break;
        }
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.actor_token,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_group_b_count = reply.publish_group_b_count,
            .group_b_count = reply.group_b_count,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
            .publish_message_state = reply.publish_message_phase_message,
            .message_state = reply.message_phase_message,
            .publish_transition_actor_index =
                reply.publish_message_phase_actor_index,
            .transition_actor_index = reply.message_phase_actor_index,
            .publish_transition_control_words =
                reply.publish_message_phase_control_words,
            .transition_control_words = reply.message_phase_control_words,
            .publish_transition_state =
                reply.publish_message_phase_transition_state,
            .transition_state = reply.message_phase_transition_state,
            .publish_transition_timer = reply.publish_message_phase_timer,
            .transition_timer = reply.message_phase_timer,
            .publish_transition_sample_word =
                reply.publish_message_phase_sample_word,
            .transition_sample_word = reply.message_phase_sample_word,
            .publish_transition_aux_byte = reply.publish_message_phase_aux_byte,
            .transition_aux_byte = reply.message_phase_aux_byte,
            .publish_completion_gate =
                reply.publish_message_phase_completion_gate,
            .completion_gate = reply.message_phase_completion_gate,
            .publish_special_action_count =
                reply.publish_message_phase_special_count,
            .special_action_count = reply.message_phase_special_count,
            .publish_target_ready_gate =
                reply.publish_message_phase_target_ready,
            .target_ready_gate = reply.message_phase_target_ready,
            .publish_transition_mode_gate =
                reply.publish_message_phase_mode_gate,
            .transition_mode_gate = reply.message_phase_mode_gate,
            .publish_group_b_bypass_gate =
                reply.publish_message_phase_group_b_bypass,
            .group_b_bypass_gate = reply.message_phase_group_b_bypass,
            .typed_stop = reply.message_phase_action_item_typed_stop,
            .group_b_action_item_definition =
                reply.message_phase_action_item_definition,
        };
    }
    [[nodiscard]] LegacyBattleVictoryRewardCallReply invoke_victory_reward(
        const LegacyBattleVictoryRewardCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                reserved_victory_query_group_b_item;
        switch (request.call) {
        case LegacyBattleVictoryRewardCall::reserved_query_group_b_item:
            break;
        case LegacyBattleVictoryRewardCall::query_group_a_reward_block:
            call = LegacyBattleFrameCoordinatorCall::
                victory_query_group_a_reward_block;
            break;
        case LegacyBattleVictoryRewardCall::reserved_apply_group_a_reward:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_victory_apply_group_a_reward;
            break;
        case LegacyBattleVictoryRewardCall::prepare_group_a_actor:
            call =
                LegacyBattleFrameCoordinatorCall::victory_prepare_group_a_actor;
            break;
        case LegacyBattleVictoryRewardCall::configure_group_a_actor:
            call = LegacyBattleFrameCoordinatorCall::
                victory_configure_group_a_actor;
            break;
        case LegacyBattleVictoryRewardCall::
            reserved_transition_stage_advance_slot:
            call = LegacyBattleFrameCoordinatorCall::
                victory_reserved_transition_stage_advance_slot;
            break;
        case LegacyBattleVictoryRewardCall::format_level_up_text:
            call =
                LegacyBattleFrameCoordinatorCall::victory_format_level_up_text;
            break;
        case LegacyBattleVictoryRewardCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::victory_draw_text;
            break;
        }
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.actor_token,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .victory_text_bytes = request.text,
            .victory_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_group_b_count = reply.publish_group_b_count,
            .group_b_count = reply.group_b_count,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
            .publish_collected_item_count = reply.publish_victory_item_count,
            .collected_item_count = reply.victory_item_count,
            .publish_reward_words = reply.publish_victory_reward_words,
            .committed_money_word = reply.victory_committed_money_word,
            .experience_per_party_member =
                reply.victory_experience_per_party_member,
            .reward_experience = reply.victory_reward_experience,
            .publish_transition_actor_index =
                reply.publish_message_phase_actor_index,
            .transition_actor_index = reply.message_phase_actor_index,
            .formatted_text = reply.victory_formatted_text,
            .formatted_text_length = reply.victory_formatted_text_length,
        };
    }
    [[nodiscard]] LegacyBattleVictoryRewardRegisters begin_music_fade(
        const compat::u32 eax, const compat::u32 ecx, const compat::u32 edx
    ) override {
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::victory_begin_music_fade,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleVictoryRewardRegisters stop_all_samples(
        const compat::u32 eax, const compat::u32 ecx, const compat::u32 edx
    ) override {
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::victory_stop_all_samples,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        return {.eax = 1U, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleLevelAdvancementCallReply
    invoke_level_advancement(
        const LegacyBattleLevelAdvancementCallRequest& request
    ) override {
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::
                level_advancement_build_profile,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_profile = reply.publish_level_profile,
            .profile = reply.level_profile,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
            .publish_transition_mode = reply.publish_level_transition_mode,
            .transition_mode = reply.level_transition_mode,
        };
    }
    [[nodiscard]] LegacyBattleLevelAdvancementRegisters stop_level_sample(
        const compat::u32 eax,
        const compat::u32 ecx,
        const compat::u32 edx,
        const compat::u32 sound_id
    ) override {
        const auto reply = invoke({
            .call =
                LegacyBattleFrameCoordinatorCall::level_advancement_stop_sample,
            .arguments = {sound_id},
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        return {.eax = 1U, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleLevelAdvancementRegisters play_level_sample(
        const compat::u32 eax,
        const compat::u32 ecx,
        const compat::u32 edx,
        const compat::u32 sound_id,
        const compat::i32 mix_level
    ) override {
        const auto reply = invoke({
            .call =
                LegacyBattleFrameCoordinatorCall::level_advancement_play_sample,
            .arguments =
                {
                    sound_id,
                    std::bit_cast<compat::u32>(mix_level),
                },
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleLevelGrowthPanelCallReply
    invoke_level_growth_panel(
        const LegacyBattleLevelGrowthPanelCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                level_growth_reserved_transition_stage_advance_slot;
        switch (request.call) {
        case LegacyBattleLevelGrowthPanelCall::
            reserved_transition_stage_advance_slot:
            break;
        case LegacyBattleLevelGrowthPanelCall::format_integer:
            call =
                LegacyBattleFrameCoordinatorCall::level_growth_format_integer;
            break;
        case LegacyBattleLevelGrowthPanelCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::level_growth_draw_text;
            break;
        }
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = call,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .growth_text_bytes = request.text,
            .growth_text_length = request.text_length,
        });
        const bool fallback_format =
            request.call == LegacyBattleLevelGrowthPanelCall::format_integer &&
            !reply.publish_growth_formatted_text;
        return {
            .eax = fallback_format ? request.text_length : reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_transition_actor_index =
                reply.publish_growth_transition_actor_index,
            .transition_actor_index = reply.growth_transition_actor_index,
            .publish_transition_stage = reply.publish_growth_transition_stage,
            .transition_stage = reply.growth_transition_stage,
            .publish_formatted_text =
                reply.publish_growth_formatted_text || fallback_format,
            .formatted_text =
                fallback_format ? request.text : reply.growth_formatted_text,
            .formatted_text_length = fallback_format
                ? request.text_length
                : reply.growth_formatted_text_length,
        };
    }
    [[nodiscard]] LegacyBattleLevelGrowthPanelRegisters
    play_level_growth_sample(
        const compat::u32 eax,
        const compat::u32 ecx,
        const compat::u32 edx,
        const compat::u32 sound_id,
        const compat::i32 mix_level
    ) override {
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::level_growth_play_sample,
            .arguments =
                {
                    sound_id,
                    std::bit_cast<compat::u32>(mix_level),
                },
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleGrowthActorSelectionCallReply
    invoke_growth_actor_selection(
        const LegacyBattleGrowthActorSelectionCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                growth_actor_query_group_a_reward_block;
        switch (request.call) {
        case LegacyBattleGrowthActorSelectionCall::query_group_a_reward_block:
            break;
        case LegacyBattleGrowthActorSelectionCall::
            reserved_load_item_definition:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_growth_actor_load_item_definition;
            break;
        case LegacyBattleGrowthActorSelectionCall::query_item_presence:
            call = LegacyBattleFrameCoordinatorCall::
                growth_actor_query_item_presence;
            break;
        case LegacyBattleGrowthActorSelectionCall::allocate_item_node:
            call = LegacyBattleFrameCoordinatorCall::
                growth_actor_allocate_item_node;
            break;
        }
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.actor_token,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
            .publish_definition = reply.publish_growth_item_definition,
            .definition = reply.growth_item_definition,
            .description = reply.growth_item_description,
            .description_length = reply.growth_item_description_length,
            .allocation_failed = reply.growth_item_allocation_failed,
            .publish_allocation_token =
                reply.publish_growth_item_allocation_token,
            .allocation_token = reply.growth_item_allocation_token,
        };
    }
    [[nodiscard]] LegacyBattleGrowthItemResultSelectionCallReply
    invoke_growth_item_result_selection(
        const LegacyBattleGrowthItemResultSelectionCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                growth_item_result_query_actor_completion;
        switch (request.call) {
        case LegacyBattleGrowthItemResultSelectionCall::query_actor_completion:
            break;

        case LegacyBattleGrowthItemResultSelectionCall::
            reserved_select_growth_item:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_growth_item_result_select_item;
            break;

        case LegacyBattleGrowthItemResultSelectionCall::
            reserved_load_item_definition:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_growth_item_result_load_definition;
            break;

        case LegacyBattleGrowthItemResultSelectionCall::
            release_item_description:
            call = LegacyBattleFrameCoordinatorCall::
                growth_item_result_release_description;
            break;

        case LegacyBattleGrowthItemResultSelectionCall::copy_caption:
            call = LegacyBattleFrameCoordinatorCall::
                growth_item_result_copy_caption;
            break;
        }
        std::array<compat::u32, 8U> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        std::array<compat::u8, 64U> text{};
        const std::size_t text_length =
            std::min<std::size_t>(request.text_length, text.size());
        std::copy_n(request.text.begin(), text_length, text.begin());
        const auto reply = invoke({
            .call = call,
            .object_token = request.actor_token,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .caption_text_bytes = text,
            .caption_text_length = static_cast<compat::u32>(text_length),
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
            .publish_definition = reply.publish_growth_item_definition,
            .definition = reply.growth_item_definition,
            .description = reply.growth_item_description,
            .description_length = reply.growth_item_description_length,
        };
    }
    [[nodiscard]] LegacyBattleDefeatPanelCallReply invoke_defeat_panel(
        const LegacyBattleDefeatPanelCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::defeat_panel_draw_title;
        switch (request.call) {
        case LegacyBattleDefeatPanelCall::draw_title:
            break;
        case LegacyBattleDefeatPanelCall::
            reserved_transition_stage_advance_slot:
            call = LegacyBattleFrameCoordinatorCall::
                defeat_panel_reserved_transition_stage_advance_slot;
            break;
        case LegacyBattleDefeatPanelCall::set_font_size:
            call = LegacyBattleFrameCoordinatorCall::defeat_panel_set_font_size;
            break;
        case LegacyBattleDefeatPanelCall::draw_detail:
            call = LegacyBattleFrameCoordinatorCall::defeat_panel_draw_detail;
            break;
        }
        std::array<compat::u8, 64U> text{};
        const std::size_t text_length =
            std::min<std::size_t>(request.text_length, text.size());
        std::copy_n(request.text.begin(), text_length, text.begin());
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .victory_text_bytes = text,
            .victory_text_length = static_cast<compat::u32>(text_length),
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_stage = reply.publish_growth_transition_stage,
            .stage = reply.growth_transition_stage,
        };
    }
    [[nodiscard]] LegacyBattleTalismanResultPanelCallReply
    invoke_talisman_result_panel(
        const LegacyBattleTalismanResultPanelCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                talisman_result_reserved_transition_stage_advance_slot;
        switch (request.call) {
        case LegacyBattleTalismanResultPanelCall::
            reserved_transition_stage_advance_slot:
            break;
        case LegacyBattleTalismanResultPanelCall::draw_success_title:
            call = LegacyBattleFrameCoordinatorCall::
                talisman_result_draw_success_title;
            break;
        case LegacyBattleTalismanResultPanelCall::format_success_detail:
            call = LegacyBattleFrameCoordinatorCall::
                talisman_result_format_success_detail;
            break;
        case LegacyBattleTalismanResultPanelCall::draw_success_detail:
            call = LegacyBattleFrameCoordinatorCall::
                talisman_result_draw_success_detail;
            break;
        case LegacyBattleTalismanResultPanelCall::draw_failure_title:
            call = LegacyBattleFrameCoordinatorCall::
                talisman_result_draw_failure_title;
            break;
        case LegacyBattleTalismanResultPanelCall::draw_failure_detail:
            call = LegacyBattleFrameCoordinatorCall::
                talisman_result_draw_failure_detail;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .victory_text_bytes = request.text,
            .victory_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_stage = reply.publish_growth_transition_stage,
            .stage = reply.growth_transition_stage,
            .publish_result_mode = reply.publish_message_phase_aux_byte,
            .result_mode = reply.message_phase_aux_byte,
            .publish_formatted_text = reply.publish_victory_item_list_text,
            .formatted_text = reply.victory_item_list_text,
            .formatted_text_length = reply.victory_item_list_text_length,
        };
    }
    [[nodiscard]] LegacyBattleVictoryItemListPanelCallReply
    invoke_victory_item_list_panel(
        const LegacyBattleVictoryItemListPanelCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::victory_item_list_set_font_size;
        switch (request.call) {
        case LegacyBattleVictoryItemListPanelCall::set_font_size:
            break;
        case LegacyBattleVictoryItemListPanelCall::draw_title:
            call =
                LegacyBattleFrameCoordinatorCall::victory_item_list_draw_title;
            break;
        case LegacyBattleVictoryItemListPanelCall::
            reserved_transition_stage_advance_slot:
            call = LegacyBattleFrameCoordinatorCall::
                victory_item_list_reserved_transition_stage_advance_slot;
            break;
        case LegacyBattleVictoryItemListPanelCall::format_item_row:
            call =
                LegacyBattleFrameCoordinatorCall::victory_item_list_format_row;
            break;
        case LegacyBattleVictoryItemListPanelCall::draw_item_row:
            call = LegacyBattleFrameCoordinatorCall::victory_item_list_draw_row;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .victory_text_bytes = request.text,
            .victory_text_length = request.text_length,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_item_count = reply.publish_victory_item_count,
            .item_count = reply.victory_item_count,
            .publish_formatted_text = reply.publish_victory_item_list_text,
            .formatted_text = reply.victory_item_list_text,
            .formatted_text_length = reply.victory_item_list_text_length,
        };
    }
    [[nodiscard]] LegacyBattleGrowthItemCompletionPanelCallReply
    invoke_growth_item_completion_panel(
        const LegacyBattleGrowthItemCompletionPanelCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                growth_item_completion_format_text;
        switch (request.call) {
        case LegacyBattleGrowthItemCompletionPanelCall::format_text:
            break;
        case LegacyBattleGrowthItemCompletionPanelCall::measure_text:
            call = LegacyBattleFrameCoordinatorCall::
                growth_item_completion_measure_text;
            break;
        case LegacyBattleGrowthItemCompletionPanelCall::
            reserved_transition_stage_advance_slot:
            call = LegacyBattleFrameCoordinatorCall::
                growth_item_completion_reserved_transition_stage_advance_slot;
            break;
        case LegacyBattleGrowthItemCompletionPanelCall::set_font_size:
            call = LegacyBattleFrameCoordinatorCall::
                growth_item_completion_set_font_size;
            break;
        case LegacyBattleGrowthItemCompletionPanelCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::
                growth_item_completion_draw_text;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .arguments = request.arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .caption_text_bytes = request.text,
            .caption_text_length = request.text_length,
        });
        const bool fallback_format = request.call ==
                LegacyBattleGrowthItemCompletionPanelCall::format_text &&
            !reply.publish_growth_item_formatted_text;
        const bool fallback_measure = request.call ==
                LegacyBattleGrowthItemCompletionPanelCall::measure_text &&
            !reply.publish_growth_item_measured_length;
        return {
            .eax = fallback_format || fallback_measure ? request.text_length
                : request.call ==
                    LegacyBattleGrowthItemCompletionPanelCall::measure_text
                ? reply.growth_item_measured_length
                : reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_transition_stage = reply.publish_growth_transition_stage,
            .transition_stage = reply.growth_transition_stage,
            .publish_formatted_text =
                reply.publish_growth_item_formatted_text || fallback_format,
            .formatted_text = fallback_format
                ? request.text
                : reply.growth_item_formatted_text,
            .formatted_text_length = fallback_format
                ? request.text_length
                : reply.growth_item_formatted_text_length,
        };
    }
    [[nodiscard]] LegacyBattleGrowthCaptionCallReply invoke_growth_caption(
        const LegacyBattleGrowthCaptionCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::growth_caption_format_name;
        switch (request.call) {
        case LegacyBattleGrowthCaptionCall::format_name:
            break;
        case LegacyBattleGrowthCaptionCall::
            reserved_transition_stage_advance_slot:
            call = LegacyBattleFrameCoordinatorCall::
                growth_caption_reserved_transition_stage_advance_slot;
            break;
        case LegacyBattleGrowthCaptionCall::draw_text:
            call = LegacyBattleFrameCoordinatorCall::growth_caption_draw_text;
            break;
        case LegacyBattleGrowthCaptionCall::format_detail:
            call =
                LegacyBattleFrameCoordinatorCall::growth_caption_format_detail;
            break;
        }
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = call,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .caption_text_bytes = request.text,
            .caption_text_length = request.text_length,
        });
        const bool is_format =
            request.call == LegacyBattleGrowthCaptionCall::format_name ||
            request.call == LegacyBattleGrowthCaptionCall::format_detail;
        const bool fallback_format =
            is_format && !reply.publish_caption_formatted_text;
        return {
            .eax = fallback_format ? request.text_length : reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .publish_transition_actor_index =
                reply.publish_caption_transition_actor_index,
            .transition_actor_index = reply.caption_transition_actor_index,
            .publish_transition_stage = reply.publish_caption_transition_stage,
            .transition_stage = reply.caption_transition_stage,
            .publish_formatted_text =
                reply.publish_caption_formatted_text || fallback_format,
            .formatted_text =
                fallback_format ? request.text : reply.caption_formatted_text,
            .formatted_text_length = fallback_format
                ? request.text_length
                : reply.caption_formatted_text_length,
        };
    }
    [[nodiscard]] LegacyBattleGrowthCaptionRegisters
    play_growth_completion_sample(
        const compat::u32 eax,
        const compat::u32 ecx,
        const compat::u32 edx,
        const compat::u32 sound_id,
        const compat::i32 mix_level
    ) override {
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::
                growth_completion_caption_play_sample,
            .arguments =
                {
                    sound_id,
                    std::bit_cast<compat::u32>(mix_level),
                },
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }
    [[nodiscard]] LegacyBattleActorTargetPreparationCallReply
    invoke_actor_target_preparation(
        const LegacyBattleActorTargetPreparationCallRequest& request
    ) override {
        const auto call = request.call ==
                LegacyBattleActorTargetPreparationCall::prepare_group_a_actor
            ? LegacyBattleFrameCoordinatorCall::
                  actor_target_prepare_group_a_actor
            : LegacyBattleFrameCoordinatorCall::
                  actor_target_query_group_b_completion;
        std::array<compat::u32, 8> arguments{};
        for (std::size_t index = 0U; index < request.arguments.size();
             ++index) {
            arguments[index] = request.arguments[index];
        }
        const auto reply = invoke({
            .call = call,
            .object_token = request.object_token,
            .arguments = arguments,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
        };
    }
    [[nodiscard]] LegacyBattleFrameInputResolutionCallReply
    invoke_frame_input_resolution(
        const LegacyBattleFrameInputResolutionCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::
                frame_input_configure_actor_selection;
        switch (request.call) {
        case LegacyBattleFrameInputResolutionCall::validate_option_actor:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_validate_option_actor;
            break;
        case LegacyBattleFrameInputResolutionCall::configure_actor_selection:
            break;
        case LegacyBattleFrameInputResolutionCall::query_group_b_candidate:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_query_group_b_candidate;
            break;
        case LegacyBattleFrameInputResolutionCall::prepare_actor_origin:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_prepare_actor_origin;
            break;
        case LegacyBattleFrameInputResolutionCall::resolve_actor_surface:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_resolve_actor_surface;
            break;
        case LegacyBattleFrameInputResolutionCall::query_actor_mirror:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_query_actor_mirror;
            break;
        case LegacyBattleFrameInputResolutionCall::
            reserved_query_group_b_action_six_target_availability_slot:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_frame_input_query_group_b_action_six_availability_slot;
            break;
        case LegacyBattleFrameInputResolutionCall::query_group_a_candidate:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_query_group_a_candidate;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .arguments =
                {
                    request.actor_token,
                    request.arguments[0],
                    request.arguments[1],
                    request.arguments[2],
                    request.arguments[3],
                },
            .eax = request.eax,
            .ecx = request.actor_token,
            .edx = request.edx,
        });
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .origin_x = reply.origin_x,
            .origin_y = reply.origin_y,
            .surface = reply.actor_surface,
        };
    }

    [[nodiscard]] virtual compat::u32
    start_music(const std::filesystem::path& path, compat::u32 mode) = 0;
    [[nodiscard]] virtual compat::u32
    create_temporary_surface(compat::u32 owner_token, compat::u32 format) = 0;
    [[nodiscard]] virtual compat::u32
    operate_surface(compat::u32 object_token, compat::u32 source_token) = 0;

    [[nodiscard]] LegacyBattleAttackOrderDequeueActorReply query_actor(
        const LegacyBattleAttackOrderDequeueActorRequest& request
    ) override {
        const auto reply = invoke({
            .call = LegacyBattleFrameCoordinatorCall::
                attack_order_dequeue_query_actor,
            .arguments =
                {
                    request.actor_token,
                    request.actor_code,
                    request.actor_index,
                    request.stale_eax,
                    request.stale_edx,
                },
            .eax = request.stale_eax,
            .ecx = request.actor_token,
            .edx = request.stale_edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

    [[nodiscard]] LegacyBattlePendingActionCallReply invoke_pending_action(
        const LegacyBattlePendingActionCallRequest& request
    ) override {
        LegacyBattleFrameCoordinatorCall call =
            LegacyBattleFrameCoordinatorCall::pending_action_prepare_actor;
        switch (request.call) {
        case LegacyBattlePendingActionCall::prepare_actor:
            break;
        case LegacyBattlePendingActionCall::commit_actor:
            call =
                LegacyBattleFrameCoordinatorCall::pending_action_commit_actor;
            break;
        case LegacyBattlePendingActionCall::reserved_remove_actor_record:
            call = LegacyBattleFrameCoordinatorCall::
                reserved_pending_action_remove_actor_record;
            break;
        }
        const auto reply = invoke({
            .call = call,
            .arguments =
                {
                    request.actor_token,
                    request.actor_code,
                    request.actor_index,
                    request.actor_group,
                    request.arguments[0],
                    request.arguments[1],
                },
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

    [[nodiscard]] LegacyBattleActorReadyCallReply
    query_ready(const LegacyBattleActorReadyRequest& request) override {
        const auto reply = invoke({
            .call =
                LegacyBattleFrameCoordinatorCall::pending_action_ready_query,
            .arguments =
                {
                    request.actor_token,
                    request.stale_eax,
                    request.stale_edx,
                },
            .eax = request.stale_eax,
            .ecx = request.actor_token,
            .edx = request.stale_edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

    [[nodiscard]] LegacyBattleFrameCompletionCallReply invoke_frame_completion(
        const LegacyBattleFrameCompletionCallRequest& request
    ) override {
        const auto reply = invoke({
            .call =
                LegacyBattleFrameCoordinatorCall::frame_completion_query_actor,
            .arguments =
                {
                    request.actor_token,
                    request.actor_index,
                    request.actor_group,
                    request.mask,
                },
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

    [[nodiscard]] compat::u32 resolve_vertical_shift_surface(
        const compat::u32 owner_token, const compat::u32 selector
    ) override {
        return create_temporary_surface(owner_token, selector);
    }
};

struct LegacyBattleFrameCoordinatorState {
    compat::u32 active{};
    compat::u8 music_suppression{};
    std::filesystem::path music_path;
    compat::u32 music_runtime_handle{};
    compat::u32 target_surface_token{
        kLegacyBattleFrameCoordinatorTargetSurfaceToken
    };
    compat::u32 current_target_pointer_token{};
    compat::u32 render_abort_latch{};
    compat::u32 selection_enable{1U};
    compat::u16 selection_delay{};
    compat::u32 selection_auxiliary{};
    compat::u32 interaction_available{};
    compat::u32 conditional_mode{};
    compat::u32 conditional_submode{};
    compat::u32 ui_state{};
    compat::u32 special_panel_suppression{};
    compat::u32 special_surface_gate{};
    compat::u16 screenshot_counter{};
    std::filesystem::path screenshot_path;
    asset_runtime::LegacyActionRecord panel_action_record{};
    LegacyBattleStandaloneActionFrameDrawState standalone_action;
    LegacyBattleFrameEffectState frame_effect{};
    LegacyBattleHudFrameState hud{};
    LegacyBattleContextPromptState context_prompt{};
    LegacyBattleDebugOverlayState debug_overlay{};
};

struct LegacyBattleFrameCoordinatorRequest {
    std::span<const compat::u32> role_index_map;
    std::span<const LegacyBattleFrameCoordinatorPosition> role_positions;
    compat::u16 gameplay_word{};
    compat::u32 actor_priority_eax_snapshot{};
    compat::u32 actor_priority_ecx_snapshot{};
    compat::u32 actor_priority_edx_snapshot{};
    compat::u32 attack_order_dequeue_edx_snapshot{};
    LegacyBattleSelectionFrameRequest selection_frame_request{};
    compat::u32 post_actor_frame_ecx_snapshot{};
    compat::u32 post_actor_frame_edx_snapshot{};
    compat::u32 post_frame_zero_ecx_snapshot{};
    compat::u32 post_tiled_frame_ecx_snapshot{};
    compat::u32 standalone_action_update_ecx_snapshot{};
    compat::u32 standalone_action_update_edx_snapshot{};
    compat::u32 post_standalone_frame_ecx_snapshot{};
    compat::u32 message_phase_entry_ecx_snapshot{};
    compat::u32 message_phase_entry_edx_snapshot{};
    LegacyBattleVictoryRewardRequest victory_reward_request{};
    LegacyBattleTextMessageFrameRequest text_message_frame_request{};
    LegacyBattleDebugStatusPanelRequest debug_status_panel_request{};
    compat::u32 debug_vitality_stack_snapshot{};
    compat::i32 mouse_x{};
    compat::i32 mouse_y{};
    compat::u32 input_mouse_lower_bound{};
    compat::u32 input_mouse_upper_bound{480U};
    compat::u32 context_prompt_action_update_edx_snapshot{};
};

struct LegacyBattleFrameCoordinatorContext {
    LegacyBattleFrameZeroContext& frame_zero;
    rendering::LegacyRasterGeometryState& raster;
    LegacyBattleFrameEffectPort& frame_effect_port;
    LegacyBattleFrameEffectSource& frame_effect_source;
    std::span<const compat::u32> frame_effect_surfaces;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    std::list<rendering::LegacyPackedRowEffect>& packed_row_effects;
    std::span<const compat::u32> packed_row_colors;
    input_time_rng::LegacySecondaryRng& secondary_rng;
    rendering::LegacyPackedRowDrawPorts& packed_row_draw_ports;
    world_map::LegacyRoleHeadActionList& role_head_actions;
    asset_runtime::LegacyActionDrawPorts& role_head_action_ports;
    story_scene::LegacyDialogRuntimeState& dialogs;
    const story_scene::LegacyDialogRuntimeInput& dialog_input;
    story_scene::LegacyDialogRuntimePorts& dialog_ports;
    const rendering::LegacyCountdownState& countdown;
    rendering::LegacyCountdownFlagQueryPorts& countdown_flags;
    rendering::LegacyCountdownPieceProvider& countdown_provider;
    std::span<const compat::u8> internal_flags;
    const rendering::LegacyPixelConversionState& pixel_conversion;
    rendering::LegacyBmpWriterPorts& bmp_ports;
    LegacyBattleFinalActorStepState& final_actor_step;
    LegacyBattleActionDispatchState& action_dispatch;
    LegacyBattleStartupState& startup;
    input_time_rng::LegacyInputNormalizationState& input_normalization;
    const input_time_rng::LegacyKeyboardSnapshot& keyboard;
    std::vector<world_map::LegacyWorldInteractionHotspot>& choice_hotspots;
    world_map::LegacyWorldPlayerControlState& player_control;
    world_map::LegacyWorldStoryVmState& story_vm;
    compat::u32& target_ready_gate;
    LegacyBattleActorFrameAdvanceContext* actor_frames{};
    std::span<const compat::u8> maps_payload{};
    std::span<compat::u8> shared_text{};
};

enum class LegacyBattleFrameCoordinatorStatus : compat::u8 {
    completed,
    pre_frame_returned_zero,
    frame_input_resolution_typed_stop,
    actor_metric_typed_stop,
    actor_order_typed_stop,
    actor_priority_typed_stop,
    attack_order_dequeue_typed_stop,
    actor_frame_typed_stop,
    frame_completion_typed_stop,
    pending_action_typed_stop,
    effect_coordinator_typed_stop,
    render_aborted,
    fixed_frame_typed_stop,
    role_map_typed_stop,
    role_actor_typed_stop,
    tiled_frame_typed_stop,
    standalone_frame_typed_stop,
    dialog_typed_stop,
    debug_status_panel_typed_stop,
    countdown_typed_stop,
    frame_effect_typed_stop,
    selection_frame_typed_stop,
    internal_flag_typed_stop,
    input_return_three,
    temporary_surface_typed_stop,
    hud_typed_stop,
    message_phase_typed_stop,
    text_message_frame_typed_stop,
    color_accumulation_typed_stop,
    pre_frame_typed_stop,
    debug_hotkey_typed_stop,
    input_dispatch_typed_stop,
    debug_overlay_typed_stop,
    outcome_resolution_typed_stop,
    context_prompt_typed_stop,
    vertical_shift_typed_stop,
};

struct LegacyBattleFrameCoordinatorResult {
    LegacyBattleFrameCoordinatorStatus status{
        LegacyBattleFrameCoordinatorStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 port_calls{};
    bool music_started{};
    compat::u32 music_commit_calls{};
    compat::u32 lock_calls{};
    compat::u32 unlock_calls{};
    compat::u32 selection_refresh_calls{};
    LegacyBattleAttackOrderDequeueResult attack_order_dequeue{};
    LegacyBattleFrameInputResolutionResult frame_input_resolution{};
    compat::u32 frame_input_resolution_calls{};
    LegacyBattleInputDispatchResult input_dispatch{};
    compat::u32 input_dispatch_calls{};
    LegacyBattlePreFrameResult pre_frame{};
    compat::u32 pre_frame_calls{};
    LegacyBattleDebugHotkeyResult debug_hotkeys{};
    compat::u32 debug_hotkey_calls{};
    LegacyBattleDebugOverlayResult debug_overlay{};
    compat::u32 debug_overlay_calls{};
    LegacyBattleOutcomeResolutionResult outcome_resolution{};
    compat::u32 outcome_resolution_calls{};
    LegacyBattleContextPromptResult context_prompt{};
    compat::u32 context_prompt_calls{};
    LegacyBattleVerticalShiftResult vertical_shift{};
    compat::u32 vertical_shift_calls{};
    LegacyBattleFrameEffectResult frame_effect{};
    compat::u32 frame_effect_calls{};
    LegacyBattleSelectionFrameResult selection_frame{};
    compat::u32 selection_frame_calls{};
    LegacyBattleActorPriorityResult actor_priority{};
    compat::u32 actor_priority_calls{};
    LegacyBattleActorFrameSequenceResult actor_frame_sequence{};
    compat::u32 actor_frame_sequence_calls{};
    LegacyBattleFrameCompletionResult frame_completion{};
    compat::u32 frame_completion_calls{};
    LegacyBattlePendingActionResult pending_actions{};
    compat::u32 pending_action_calls{};
    LegacyBattleEffectCoordinatorResult effect_coordinator{};
    compat::u32 effect_coordinator_calls{};
    LegacyBattleHudFrameResult hud_frame{};
    compat::u32 hud_frame_calls{};
    LegacyBattleMessagePhaseResult message_phase{};
    compat::u32 message_phase_calls{};
    LegacyBattleTextMessageFrameResult text_message_frame{};
    compat::u32 text_message_frame_calls{};
    LegacyBattleFrameDrawResult fixed_frame{};
    compat::u32 fixed_frame_calls{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    compat::u32 panel_action_update_calls{};
    rendering::LegacyTiledFrameResult panel_frame{};
    compat::u32 panel_frame_calls{};
    LegacyBattleStandaloneActionFrameDrawResult standalone_frame{};
    compat::u32 standalone_frame_calls{};
    rendering::LegacyPackedRowEffectResult packed_rows{};
    world_map::LegacyRoleHeadActionResult role_heads{};
    story_scene::LegacyDialogRuntimeResult dialogs{};
    LegacyBattleDebugStatusPanelResult debug_status_panel{};
    compat::u32 debug_status_panel_calls{};
    std::array<rendering::LegacyCountdownDisplayResult, 2> countdowns{};
    compat::u32 countdown_calls{};
    compat::u32 input_queries{};
    LegacyBattleColorInitializationResult color_initialization{};
    compat::u32 color_initialization_calls{};
    rendering::LegacyFrameColorTransitionResult color_accumulation{};
    compat::u32 color_accumulation_calls{};
    compat::u32 temporary_surface_calls{};
    compat::u32 surface_operation_calls{};
    rendering::LegacyBmpWriteResult screenshot{};
    compat::u32 screenshot_calls{};
    compat::u32 gameplay_word_argument{};
};

[[nodiscard]] LegacyBattleFrameCoordinatorResult
run_legacy_battle_frame_coordinator(
    LegacyBattleFrameCoordinatorState& state,
    LegacyBattleFrameCoordinatorPort& port,
    LegacyBattleFrameCoordinatorContext& context,
    const LegacyBattleFrameCoordinatorRequest& request
);

}  // namespace openswd3::battle
