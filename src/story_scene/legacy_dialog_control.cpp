#include "openswd3/story_scene/legacy_dialog_control.hpp"

namespace openswd3::story_scene {
namespace {

using compat::u16;
using compat::u32;

[[nodiscard]] constexpr bool is_negative_byte(const compat::u8 value) noexcept {
  return (value & 0x80U) != 0U;
}

[[nodiscard]] constexpr u32 decrement_flagged_dialog_counter(
    const u32 value) noexcept {
  const u32 preserved_flag = value & 0x00008000U;
  u32 counter = value & ~0x00008000U;
  --counter;
  if (static_cast<compat::i32>(counter) < 0) {
    counter = 0U;
  }
  return counter | preserved_flag;
}

[[nodiscard]] constexpr u32 elapsed_deciseconds(
    const u32 current_tick, const u32 started_at) noexcept {
  return (current_tick - started_at) / 100U;
}

void accept_selection(LegacyDialogRecord32 &record,
                      LegacyDialogControlState &state) noexcept {
  record.flags |= kLegacyDialogFlagSelectionAccepted;
  state.selection_state = 1U;
}

void filter_selection_through_advance_signal(
    LegacyDialogRecord32 &record,
    LegacyDialogControlState &state) noexcept {
  if (state.advance_signal_state == 1U) {
    return;
  }
  state.selection_state = 0U;
  record.flags &= ~kLegacyDialogFlagSelectionAccepted;
}

[[nodiscard]] u32 sample_elapsed(const LegacyDialogRecord32 &record,
                                 const LegacyDialogControlInput &input,
                                 LegacyDialogControlResult &result) noexcept {
  result.elapsed_deciseconds =
      elapsed_deciseconds(input.current_tick, record.lifetime_started_at);
  result.elapsed_sampled = true;
  return result.elapsed_deciseconds;
}

void apply_timed_advance(LegacyDialogRecord32 &record,
                         const LegacyDialogControlInput &input,
                         LegacyDialogControlState &state,
                         LegacyDialogControlResult &result,
                         const bool reset_start) noexcept {
  if (record.lifetime_limit == 0xFFFFU) {
    return;
  }
  const u32 elapsed = sample_elapsed(record, input, result);
  if (elapsed <= record.lifetime_limit ||
      (record.flags & kLegacyDialogFlagTimedAdvance) == 0U) {
    return;
  }
  if (reset_start) {
    record.lifetime_started_at = 0U;
  }
  accept_selection(record, state);
}

} // namespace

LegacyDialogControlResult advance_legacy_dialog_control(
    LegacyDialogRecord32 &record, const LegacyDialogControlInput &input,
    LegacyDialogControlState &state) noexcept {
  LegacyDialogControlResult result;
  result.force_complete_text = input.primary_press_state != 0U;
  state.selection_state = input.initial_selection_state;

  record.character_countdown =
      static_cast<u16>(record.character_countdown - 1U);

  if (input.primary_press_state != 0U) {
    record.character_countdown = 0x8FFFU;
    state.selection_state = 1U;
    state.advance_signal_state = 1U;
  }
  if (input.global_confirm_latch_state == 1U) {
    state.advance_signal_state = 1U;
  }

  if ((record.flags & kLegacyDialogFlagInteractive) != 0U &&
      state.advance_signal_state == 1U) {
    if ((record.flags & kLegacyDialogFlagConfirmArmed) == 0U) {
      record.flags |= kLegacyDialogFlagConfirmArmed;
    } else {
      record.flags |= kLegacyDialogFlagClosing;
    }
  }

  if ((record.flags & kLegacyDialogFlagClosing) != 0U) {
    record.character_delay = 0xFFFFU;
    if ((record.flags & kLegacyDialogFlagCloseInitialized) == 0U) {
      record.character_countdown = 0xFFFFU;
      record.flags |= kLegacyDialogFlagCloseInitialized;
    }
  }

  if ((record.flags &
       (kLegacyDialogFlagPageBoundary | kLegacyDialogFlagTerminated)) == 0U) {
    record.display_counter = 0x24U;
    if ((record.flags & kLegacyDialogFlagInteractive) != 0U) {
      state.selection_state = 0U;
    } else if (state.selection_state != 0U) {
      record.flags |= kLegacyDialogFlagClosing;
    }
  } else if ((record.flags & kLegacyDialogFlagTerminated) != 0U) {
    if ((record.flags & kLegacyDialogFlagPressResetsSelection) != 0U &&
        input.primary_press_state == 0U) {
      state.selection_state = 0U;
    }
    if (input.initial_selection_state != 0U) {
      record.flags |= 0x00000001U;
    }
    if (state.selection_state != 0U) {
      record.flags |= kLegacyDialogFlagSelectionAccepted;
    }
    filter_selection_through_advance_signal(record, state);
    apply_timed_advance(record, input, state, result, false);
  } else {
    if ((record.flags & kLegacyDialogFlagPressResetsSelection) != 0U &&
        input.primary_press_state == 0U) {
      state.selection_state = 0U;
    }
    if (state.selection_state != 0U) {
      record.flags |= kLegacyDialogFlagSelectionAccepted;
    }
    filter_selection_through_advance_signal(record, state);
    if (state.selection_state != 0U) {
      record.flags |= kLegacyDialogFlagClosing;
    }
    apply_timed_advance(record, input, state, result, true);
    if (record.lifetime_limit == 0xFFFFU &&
        (record.flags & kLegacyDialogFlagPressResetsSelection) != 0U) {
      state.selection_state = input.initial_selection_state;
    }
  }

  if ((record.flags & kLegacyDialogFlagDirectRectangle) != 0U) {
    state.selection_state = 0U;
    const u32 elapsed = sample_elapsed(record, input, result);
    if (record.lifetime_limit != 0xFFFFU) {
      if (elapsed > record.lifetime_limit) {
        record.display_counter =
            static_cast<u16>(record.display_counter - 1U);
        if ((record.display_counter & 0x8000U) != 0U) {
          record.display_counter = 0U;
        }
        if (record.display_counter == 0U) {
          accept_selection(record, state);
        }
      }
    } else if (elapsed < 0x00010018U) {
      record.display_counter = 0x10U;
    }
  }

  if ((record.flags & kLegacyDialogFlagHasChoices) != 0U) {
    record.display_counter = 0x20U;
    if (!input.choice_chain_active) {
      record.display_counter = 1U;
      accept_selection(record, state);
    }
  }

  if (input.transition_in_progress) {
    result.action =
        LegacyDialogControlAction::skip_text_during_transition;
    return result;
  }

  if (state.selection_state == 0U) {
    return result;
  }
  if ((record.flags & kLegacyDialogFlagInteractive) == 0U) {
    record.character_countdown = 0x8FFFU;
  }
  if (input.choice_chain_active) {
    return result;
  }
  if ((record.flags & kLegacyDialogFlagPageBoundary) != 0U &&
      state.selection_state != 2U) {
    result.action = LegacyDialogControlAction::advance_page;
    return result;
  }
  if ((record.flags & kLegacyDialogFlagSelectionAccepted) != 0U) {
    result.action = LegacyDialogControlAction::close_message;
  }
  return result;
}

LegacyDialogControlApplyResult apply_legacy_dialog_control_action(
    LegacyDialogMessage &message, const LegacyDialogControlAction action,
    LegacyDialogControlState &control_state,
    LegacyDialogCloseState &close_state,
    LegacyDialogClosePorts &ports) noexcept {
  LegacyDialogControlApplyResult result;
  auto &record = message.record;

  if (action == LegacyDialogControlAction::advance_page) {
    if (message.page_stop_index >= message.text.size()) {
      result.status =
          LegacyDialogControlApplyStatus::source_out_of_bounds;
      return result;
    }
    message.text_cursor_index = message.page_stop_index;
    const compat::u8 first = message.text[message.page_stop_index];
    if (!is_negative_byte(first) && first != static_cast<compat::u8>('%')) {
      ++message.page_stop_index;
    }
    record.flags &= 0xFFFFBCFFU;
    record.foreground_index = record.saved_foreground_index;
    record.secondary_index = record.saved_secondary_index;
    record.text_style = record.saved_text_style;
    record.saved_text_style = 0U;
    result.page_advanced = true;
    return result;
  }

  if (action != LegacyDialogControlAction::close_message) {
    return result;
  }

  if ((record.flags & kLegacyDialogFlagExplicitAnchor) == 0U) {
    if (record.role_index == 0xFFFDU) {
      ports.close_detached_dialog();
    } else if (!ports.close_role_dialog_action(record.role_index)) {
      ++result.nonfatal_role_action_failure_count;
    }
  }

  if ((record.flags & kLegacyDialogFlagCloseRoleAction) != 0U) {
    close_state.close_mode_state = 0x0CU;
    close_state.flagged_dialog_counter = decrement_flagged_dialog_counter(
        close_state.flagged_dialog_counter);
  }

  record.text_cursor_pointer_32 = 0U;
  message.active = false;
  control_state.selection_state = 0U;
  control_state.advance_signal_state = 0U;
  close_state.input_hold_state = 0U;
  result.message_closed = true;
  return result;
}

} // namespace openswd3::story_scene
