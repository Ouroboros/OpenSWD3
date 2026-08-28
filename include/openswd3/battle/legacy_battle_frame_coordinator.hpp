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
#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "openswd3/battle/legacy_battle_frame_completion.hpp"
#include "openswd3/battle/legacy_battle_frame_effect.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
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
    frame_input_query_group_b_mode,
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
    post_render_stage_2,
    post_render_stage_3,
    post_dialog_stage,
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
    selection_frame_draw_message_seven,
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
      public virtual LegacyBattleEffectCoordinatorStatePort {
public:
    using LegacyBattleEffectCallPort::invoke;
    using LegacyBattleOutcomeFinalizationPort::invoke;

    virtual ~LegacyBattleFrameCoordinatorPort() = default;

    [[nodiscard]] virtual LegacyBattleFrameCoordinatorCallReply
    invoke(const LegacyBattleFrameCoordinatorCallRequest& request) = 0;
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
        case LegacyBattleSelectionFrameCall::draw_message_seven:
            call = LegacyBattleFrameCoordinatorCall::
                selection_frame_draw_message_seven;
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
        case LegacyBattleFrameInputResolutionCall::query_group_b_mode:
            call = LegacyBattleFrameCoordinatorCall::
                frame_input_query_group_b_mode;
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
    countdown_typed_stop,
    frame_effect_typed_stop,
    selection_frame_typed_stop,
    internal_flag_typed_stop,
    input_return_three,
    temporary_surface_typed_stop,
    hud_typed_stop,
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
