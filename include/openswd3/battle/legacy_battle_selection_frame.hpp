#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/battle/legacy_battle_action_summary.hpp"
#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_alternate_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_actor_target_preparation.hpp"
#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"
#include "openswd3/battle/legacy_battle_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_guard_panel_frame.hpp"
#include "openswd3/battle/legacy_battle_group_b_frame.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_list_contents.hpp"
#include "openswd3/battle/legacy_battle_list_frame.hpp"
#include "openswd3/battle/legacy_battle_mode_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_narrow_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_scale_fill_panel.hpp"
#include "openswd3/battle/legacy_battle_selection_hint_frame.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_vertical_panel.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleSelectionFrameState {
    std::array<compat::u32, 2> pointer_origin{};  // 0x004A9924
    compat::u32 display_gate{};                   // 0x0053BF5C
    compat::u32 secondary_actor_gate{};           // 0x0053BF68
    LegacyBattleScaleFillPanelState scale_fill_panel{};
    LegacyBattleVerticalPanelState vertical_panel{};
    LegacyBattlePreparedActionFrameDrawState prepared_action_frame{};
    LegacyBattleListContentsState list_contents{};
    LegacyBattleGridFrameState grid_frame{};
    LegacyBattleAlternateGridFrameState alternate_grid_frame{};
    LegacyBattleModeGridFrameState mode_grid_frame{};
    LegacyBattleNarrowGridFrameState narrow_grid_frame{};
    LegacyBattleSelectionHintFrameState selection_hint_frame{};
    // Records 0..7 occupy 0x004FD798..0x004FDC57. Record 8 starts at
    // 0x004FDC58 and aliases the four lower-panel dwords for its first
    // 0x10 bytes; the remaining bytes and record 9 use the tail below.
    std::array<asset_runtime::LegacyActionRecord, 8> prepared_action_records{};
    std::array<compat::u8, 0x120> prepared_action_overlap_tail{};
};

class LegacyBattleSelectionFrameStatePort {
public:
    [[nodiscard]] virtual LegacyBattleSelectionFrameState&
    battle_selection_frame_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleSelectionFrameState&
    battle_selection_frame_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleSelectionFrameStatePort() = default;
    ~LegacyBattleSelectionFrameStatePort() = default;

private:
    LegacyBattleSelectionFrameState state_{};
};

enum class LegacyBattleSelectionFrameCall : compat::u8 {
    query_group_a_replacement,
    reserved_prepare_selected_actor_slot,
    query_selected_actor_release,
    release_selected_actor,
    reset_actor_selection,
    draw_mouse_anchor,
    configure_text_row,
    configure_text_color,
    query_text_length,
    draw_text,
    reserved_draw_action_summary_slot,
    reserved_draw_list_frame_slot,
    reserved_draw_list_contents_slot,
    reserved_draw_grid_frame_slot,
    reserved_draw_narrow_frame_slot,
    reserved_draw_grid_alternate_slot,
    reserved_draw_grid_mode_slot,
    reserved_draw_message_five_slot,
    draw_message_seven,
    configure_text_font,
    query_group_b_completion,
    query_group_a_completion,
    build_actor_snapshot,
    query_actor_origin,
    query_target_action_available,
    reserved_draw_selection_hint_slot,
};

struct LegacyBattleSelectionFrameCallRequest {
    LegacyBattleSelectionFrameCall call{
        LegacyBattleSelectionFrameCall::query_group_a_replacement
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleSelectionFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::i32 snapshot_x{};
    compat::i32 snapshot_y{};
    compat::i32 snapshot_width{};
    compat::i32 snapshot_height{};
    compat::u16 origin_x{};
    compat::u16 origin_y{};
    compat::u32 text_length{};
};

class LegacyBattleSelectionFramePort
    : public virtual LegacyBattleSelectionFrameStatePort,
      public virtual LegacyBattleActorTargetPreparationPort,
      public virtual LegacyBattleActionSummaryPort,
      public virtual LegacyBattleListFramePort,
      public virtual LegacyBattleListContentsPort,
      public virtual LegacyBattleGridFramePort,
      public virtual LegacyBattleSelectionHintFramePort {
public:
    virtual ~LegacyBattleSelectionFramePort() = default;

    [[nodiscard]] virtual LegacyBattleSelectionFrameCallReply
    invoke_selection_frame(
        const LegacyBattleSelectionFrameCallRequest& request
    ) = 0;
};

struct LegacyBattleSelectionFrameBindings {
    LegacyBattleStartupState& startup;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleActorMetricState& metrics;
    const std::array<compat::u32, 10>& actor_label_indices;
    LegacyBattleActionDispatchState& action;
    LegacyBattleInputDispatchState& input_dispatch;
    LegacyBattleFrameInputResolutionState& frame_input;
    LegacyBattleTargetSelectionRuntimeState& target_runtime;
    LegacyBattleDebugHotkeyState& debug_hotkeys;
    LegacyBattleGroupBFrameState* actor_frames;
    compat::u32& message_state;
    compat::u32& target_ready_gate;
    asset_runtime::LegacyActionRecord& panel_action_record;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    LegacyBattleBoundedRandomPort& bounded_random;
    std::span<const compat::u8> maps_payload{};
    std::span<compat::u8> shared_text{};
};

struct LegacyBattleSelectionFrameRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    std::array<LegacyBattleActionUpdateRegisterSnapshot, 4>
        vertical_panel_update_registers{};
    compat::u32 vertical_panel_final_blit_eax{};
    compat::u32 prepared_action_update_ecx{};
    compat::u32 prepared_action_return_eax{};
    LegacyBattleListFrameRequest list_frame{};
    LegacyBattleListContentsRequest list_contents{};
    LegacyBattleGridFrameRequest grid_frame{};
    LegacyBattleAlternateGridFrameRequest alternate_grid_frame{};
    LegacyBattleModeGridFrameRequest mode_grid_frame{};
    LegacyBattleNarrowGridFrameRequest narrow_grid_frame{};
    LegacyBattleGuardPanelFrameRequest guard_panel_frame{};
    LegacyBattleSelectionHintFrameRequest selection_hint_frame{};
};

enum class LegacyBattleSelectionFrameStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    actor_order_typed_stop,
    target_actor_index_typed_stop,
    action_workspace_typed_stop,
    actor_label_typed_stop,
    actor_frame_context_typed_stop,
    actor_target_preparation_typed_stop,
    scale_fill_panel_typed_stop,
    vertical_panel_typed_stop,
    prepared_action_frame_typed_stop,
    action_summary_typed_stop,
    list_frame_typed_stop,
    list_contents_typed_stop,
    grid_frame_typed_stop,
    alternate_grid_frame_typed_stop,
    mode_grid_frame_typed_stop,
    narrow_grid_frame_typed_stop,
    guard_panel_frame_typed_stop,
    selection_hint_frame_typed_stop,
};

struct LegacyBattleSelectionFrameResult {
    LegacyBattleSelectionFrameStatus status{
        LegacyBattleSelectionFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 group_a_calls{};
    compat::u32 group_b_calls{};
    compat::u32 action_frame_draw_calls{};
    compat::u32 action_summary_calls{};
    compat::u32 list_frame_calls{};
    compat::u32 list_contents_calls{};
    compat::u32 grid_frame_calls{};
    compat::u32 alternate_grid_frame_calls{};
    compat::u32 mode_grid_frame_calls{};
    compat::u32 narrow_grid_frame_calls{};
    compat::u32 guard_panel_frame_calls{};
    compat::u32 selection_hint_frame_calls{};
    LegacyBattleActionSummaryResult action_summary{};
    LegacyBattleListFrameResult list_frame{};
    LegacyBattleListContentsResult list_contents{};
    LegacyBattleGridFrameResult grid_frame{};
    LegacyBattleAlternateGridFrameResult alternate_grid_frame{};
    LegacyBattleModeGridFrameResult mode_grid_frame{};
    LegacyBattleNarrowGridFrameResult narrow_grid_frame{};
    LegacyBattleGuardPanelFrameResult guard_panel_frame{};
    LegacyBattleSelectionHintFrameResult selection_hint_frame{};
    LegacyBattleActorTargetPreparationResult actor_target_preparation{};
    LegacyBattleScaleFillPanelResult scale_fill_panel{};
    LegacyBattleVerticalPanelResult vertical_panel{};
    LegacyBattlePreparedActionFrameDrawResult prepared_action_frame{};
};

// Typed closure of legacy 0x00464270.
[[nodiscard]] LegacyBattleSelectionFrameResult
draw_legacy_battle_selection_frame(
    LegacyBattleSelectionFrameBindings bindings,
    LegacyBattleSelectionFramePort& port,
    const LegacyBattleSelectionFrameRequest& request = {}
);

}  // namespace openswd3::battle
