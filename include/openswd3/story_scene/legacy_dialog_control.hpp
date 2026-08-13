#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/story_scene/legacy_dialog_text.hpp"

namespace openswd3::story_scene {

inline constexpr compat::u32 kLegacyDialogFlagTimedAdvance = 0x00000008U;
inline constexpr compat::u32 kLegacyDialogFlagInteractive = 0x00000020U;
inline constexpr compat::u32 kLegacyDialogFlagSelectionAccepted = 0x00000200U;
inline constexpr compat::u32 kLegacyDialogFlagPressResetsSelection =
    0x00000400U;
inline constexpr compat::u32 kLegacyDialogFlagCloseRoleAction = 0x00000010U;
inline constexpr compat::u32 kLegacyDialogFlagCloseInitialized = 0x20000000U;
inline constexpr compat::u32 kLegacyDialogFlagConfirmArmed = 0x80000000U;

struct LegacyDialogControlInput {
    compat::u32 current_tick{};
    compat::u32 initial_selection_state{};
    compat::u32 primary_press_state{};
    compat::u32 global_confirm_latch_state{};
    bool choice_chain_active{};
    bool transition_in_progress{};
};

// Mutable globals read/written by 0x0042F11E..0x0042F43A. They remain
// explicit instead of being hidden process-wide state.
struct LegacyDialogControlState {
    compat::u32 selection_state{};
    compat::u32 advance_signal_state{};
};

enum class LegacyDialogControlAction : compat::u8 {
    parse_text,
    skip_text_during_transition,
    advance_page,
    close_message,
};

struct LegacyDialogControlResult {
    LegacyDialogControlAction action{LegacyDialogControlAction::parse_text};
    compat::u32 elapsed_deciseconds{};
    bool elapsed_sampled{};
    bool force_complete_text{};
};

struct LegacyDialogCloseState {
    compat::u32 flagged_dialog_counter{};
    compat::u32 close_mode_state{};
    compat::u32 input_hold_state{};
};

class LegacyDialogClosePorts {
public:
    virtual ~LegacyDialogClosePorts() = default;

    [[nodiscard]] virtual bool
    close_role_dialog_action(compat::u16 role_index) noexcept = 0;
    virtual void close_detached_dialog() noexcept = 0;
};

enum class LegacyDialogControlApplyStatus : compat::u8 {
    completed,
    source_out_of_bounds,
};

struct LegacyDialogControlApplyResult {
    LegacyDialogControlApplyStatus status{
        LegacyDialogControlApplyStatus::completed
    };
    compat::u32 nonfatal_role_action_failure_count{};
    bool page_advanced{};
    bool message_closed{};
};

// 0x0042F11E..0x0042F43A: decrement +0x2A and reproduce the input, timeout,
// page and close gates immediately before the text byte protocol.
[[nodiscard]] LegacyDialogControlResult advance_legacy_dialog_control(
    LegacyDialogRecord32& record,
    const LegacyDialogControlInput& input,
    LegacyDialogControlState& state
) noexcept;

// 0x0042FA7C..0x0042FB82: apply the two actions selected by the control gate.
// parse/transition actions are intentional no-ops here.
[[nodiscard]] LegacyDialogControlApplyResult apply_legacy_dialog_control_action(
    LegacyDialogMessage& message,
    LegacyDialogControlAction action,
    LegacyDialogControlState& control_state,
    LegacyDialogCloseState& close_state,
    LegacyDialogClosePorts& ports
) noexcept;

}  // namespace openswd3::story_scene
