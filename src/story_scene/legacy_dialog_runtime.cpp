#include "openswd3/story_scene/legacy_dialog_runtime.hpp"

namespace openswd3::story_scene {
namespace {

[[nodiscard]] constexpr bool requires_role_anchor(
    const LegacyDialogRecord32 &record) noexcept {
  return (record.flags & (kLegacyDialogFlagDirectRectangle |
                          kLegacyDialogFlagExplicitAnchor)) == 0U &&
         record.role_index != 0xFFFDU;
}

[[nodiscard]] constexpr bool accepted_text_status(
    const LegacyDialogTextFrameStatus status) noexcept {
  return status == LegacyDialogTextFrameStatus::completed ||
         status == LegacyDialogTextFrameStatus::page_limit_reached ||
         status == LegacyDialogTextFrameStatus::page_boundary_reached ||
         status == LegacyDialogTextFrameStatus::terminator_reached;
}

[[nodiscard]] constexpr bool should_draw_indicator(
    const compat::u32 flags) noexcept {
  if ((flags & (kLegacyDialogFlagPageBoundary |
                kLegacyDialogFlagTerminated)) == 0U) {
    return false;
  }

  const bool accepted_without_initial_selection =
      (flags & 0x00000002U) != 0U && (flags & 0x00000001U) == 0U;
  return accepted_without_initial_selection ||
         (flags & kLegacyDialogFlagPressResetsSelection) == 0U;
}

class TextSurfaceGuard final {
public:
  explicit TextSurfaceGuard(LegacyDialogRuntimePorts &ports) noexcept
      : ports_(ports) {}

  TextSurfaceGuard(const TextSurfaceGuard &) = delete;
  TextSurfaceGuard &operator=(const TextSurfaceGuard &) = delete;

  ~TextSurfaceGuard() {
    if (active_) {
      ports_.end_text_surface();
    }
  }

  void activate() noexcept { active_ = true; }
  void release() noexcept {
    if (active_) {
      ports_.end_text_surface();
      active_ = false;
    }
  }

private:
  LegacyDialogRuntimePorts &ports_;
  bool active_{};
};

} // namespace

LegacyDialogRuntimeResult update_draw_legacy_dialogs(
    LegacyDialogRuntimeState &state, const LegacyDialogRuntimeInput &input,
    LegacyDialogRuntimePorts &ports) noexcept {
  LegacyDialogRuntimeResult result;
  if (state.messages.empty()) {
    return result;
  }

  if (!ports.begin_text_surface(kLegacyDialogSurfaceWidth,
                                kLegacyDialogSurfaceHeight)) {
    result.status = LegacyDialogRuntimeStatus::surface_unavailable;
    return result;
  }
  TextSurfaceGuard surface_guard{ports};
  surface_guard.activate();
  ports.clear_text_surface();

  const compat::u32 initial_selection_state = state.control.selection_state;
  for (LegacyDialogMessage &message : state.messages) {
    ++result.message_count;
    auto &record = message.record;

    LegacyDialogAnchorInput anchor{
        .scale = kLegacyDialogScale,
        .camera_left = input.camera_left,
        .camera_top = input.camera_top,
        .role_anchor_available = true,
    };
    if (requires_role_anchor(record)) {
      anchor.role_anchor_available = ports.resolve_role_anchor(
          record.role_index, anchor.role_world_x, anchor.role_world_y);
    }

    const LegacyDialogGeometryResult geometry =
        prepare_legacy_dialog_geometry(record, anchor);
    if (geometry.status != LegacyDialogGeometryStatus::completed) {
      result.status = LegacyDialogRuntimeStatus::role_anchor_unavailable;
      return result;
    }
    if (geometry.panel_draw_requested) {
      ports.draw_dialog_panel(LegacyDialogPanelDrawRequest{
          .record = &record,
          .action = message.frame_action,
          .rectangle = geometry.panel,
          .opacity_step = geometry.opacity_step,
      });
      ++result.panel_draw_count;
    }
    ports.set_dialog_clip(geometry.text_clip);

    const LegacyDialogControlResult control = advance_legacy_dialog_control(
        record,
        LegacyDialogControlInput{
            .current_tick = input.current_tick,
            .initial_selection_state = initial_selection_state,
            .primary_press_state = input.primary_press_state,
            .global_confirm_latch_state =
                input.global_confirm_latch_state,
            .choice_chain_active = input.choice_chain_active,
            .transition_in_progress = geometry.transition_in_progress,
        },
        state.control);

    if (control.action == LegacyDialogControlAction::parse_text) {
      const LegacyDialogTextFrameResult text = update_legacy_dialog_text(
          message,
          LegacyDialogTextFrameInput{
              .scale = kLegacyDialogScale,
              .base_character_delay = input.base_character_delay,
              .selected_choice_index = input.selected_choice_index,
              .current_tick = input.current_tick,
              .force_complete = control.force_complete_text,
          },
          ports);
      result.text_status = text.status;
      result.text_draw_failure_count += text.segment_draw_failure_count;
      if (!accepted_text_status(text.status)) {
        result.status = LegacyDialogRuntimeStatus::text_failed;
        return result;
      }
    } else if (control.action !=
               LegacyDialogControlAction::skip_text_during_transition) {
      const LegacyDialogControlApplyResult applied =
          apply_legacy_dialog_control_action(message, control.action,
                                             state.control, state.close,
                                             ports);
      result.control_apply_status = applied.status;
      result.nonfatal_action_failure_count +=
          applied.nonfatal_role_action_failure_count;
      if (applied.status != LegacyDialogControlApplyStatus::completed) {
        result.status = LegacyDialogRuntimeStatus::control_apply_failed;
        return result;
      }
    }

    ports.composite_text_surface(LegacyDialogCompositeRequest{
        .destination_x = static_cast<compat::i32>(record.left),
        .destination_y = static_cast<compat::i32>(record.top),
        .flags = geometry.opacity_step == 0 ? 4U : 0x14U,
        .opacity_step = geometry.opacity_step,
    });
    ++result.composite_count;
    ports.clear_text_surface();

    if (should_draw_indicator(record.flags)) {
      ports.draw_dialog_indicator(LegacyDialogIndicatorRequest{
          .kind = (record.flags & kLegacyDialogFlagTerminated) != 0U
                      ? LegacyDialogIndicatorKind::end_dialog
                      : LegacyDialogIndicatorKind::next_page,
          .panel = geometry.panel,
          .flags = record.flags,
      });
      ++result.indicator_draw_count;
    }

    if (!geometry.transition_in_progress && !message.caption.empty() &&
        record.text_cursor_pointer_32 != 0U) {
      ports.draw_dialog_caption(LegacyDialogCaptionRequest{
          .text = message.caption,
          .panel = geometry.panel,
          .action_pointer_32 = record.caption_action_pointer_32,
          .action = message.caption_action,
          .opacity_step = geometry.opacity_step,
      });
      ++result.caption_draw_count;
    }
  }

  surface_guard.release();
  ports.set_dialog_clip(LegacyDialogRectangle{0, 0, 639, 479});
  state.control.selection_state = 0U;

  for (auto iterator = state.messages.begin();
       iterator != state.messages.end();) {
    if (iterator->active) {
      ++iterator;
      continue;
    }
    if (iterator->record.role_index != 0xFFFDU) {
      ports.release_message_owner(iterator->record.role_index);
    }
    iterator = state.messages.erase(iterator);
    ++result.removed_message_count;
  }

  if (!ports.update_end_dialog_action()) {
    ++result.nonfatal_action_failure_count;
  }
  if (!ports.update_next_page_action()) {
    ++result.nonfatal_action_failure_count;
  }
  ports.restore_text_destination(input.destination_width,
                                 input.destination_height);
  result.status = LegacyDialogRuntimeStatus::completed;
  return result;
}

} // namespace openswd3::story_scene
