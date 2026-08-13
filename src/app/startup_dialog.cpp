#include "openswd3/app/startup_dialog.hpp"

namespace openswd3::app {

namespace {

struct HoverRange {
    compat::u16 first_x{};
    compat::u16 past_last_x{};
    StartupDialogAction click_action{};
    bool requires_save{};
};

inline constexpr compat::u16 kFirstHoverY = 7U;
inline constexpr compat::u16 kLastHoverY = 0xBBU;

inline constexpr std::array<HoverRange, 5> kHoverRanges{
    HoverRange{0x2BU, 0x52U, StartupDialogAction::end_dialog_1, true},
    HoverRange{0x57U, 0x7EU, StartupDialogAction::end_dialog_2, false},
    HoverRange{0x1E4U, 0x20BU, StartupDialogAction::open_url, false},
    HoverRange{0x20EU, 0x235U, StartupDialogAction::open_readme, false},
    HoverRange{0x235U, 0x25CU, StartupDialogAction::end_dialog_6, false},
};

}  // namespace

void initialize_startup_dialog(StartupDialogState& state) noexcept {
    state.control_visibility = {};
}

StartupDialogAction
select_startup_dialog_command(const compat::u32 command_value) noexcept {
    switch (static_cast<compat::u16>(command_value)) {
    case 0x3FEU:
        return StartupDialogAction::show_auxiliary_then_end_dialog_2;
    case 0x3FFU:
        return StartupDialogAction::open_url;
    case 0x400U:
        return StartupDialogAction::open_readme;
    case 0x401U:
        return StartupDialogAction::end_dialog_6;
    case 0x402U:
        return StartupDialogAction::end_dialog_3;
    default:
        return StartupDialogAction::none;
    }
}

StartupDialogAction startup_dialog_close_action() noexcept {
    return StartupDialogAction::end_dialog_6;
}

StartupDialogPointerResult update_startup_dialog_pointer(
    StartupDialogState& state,
    const bool any_save_exists,
    const StartupDialogPointerMessage message,
    const compat::u32 packed_position
) noexcept {
    const auto previous_visibility = state.control_visibility;
    state.control_visibility = {};

    const compat::u16 x = static_cast<compat::u16>(packed_position);
    const compat::u16 y = static_cast<compat::u16>(packed_position >> 16U);
    if (y < kFirstHoverY || y > kLastHoverY) {
        return {};
    }

    for (std::size_t index = 0U; index < kHoverRanges.size(); ++index) {
        const HoverRange& range = kHoverRanges[index];
        if (x < range.first_x || x >= range.past_last_x ||
            (range.requires_save && !any_save_exists)) {
            continue;
        }

        state.control_visibility[index] = kStartupDialogControlShown;
        return {
            message == StartupDialogPointerMessage::left_button_down
                ? range.click_action
                : StartupDialogAction::none,
            previous_visibility[index] == kStartupDialogControlHidden,
        };
    }
    return {};
}

void execute_startup_dialog_action(
    const StartupDialogAction action, StartupDialogPorts& ports
) {
    switch (action) {
    case StartupDialogAction::show_auxiliary_then_end_dialog_2:
        ports.show_auxiliary_dialog();
        ports.end_dialog(2);
        return;
    case StartupDialogAction::open_url:
        ports.open_url();
        return;
    case StartupDialogAction::open_readme:
        ports.open_readme();
        return;
    case StartupDialogAction::end_dialog_1:
        ports.end_dialog(1);
        return;
    case StartupDialogAction::end_dialog_2:
        ports.end_dialog(2);
        return;
    case StartupDialogAction::end_dialog_3:
        ports.end_dialog(3);
        return;
    case StartupDialogAction::end_dialog_6:
        ports.end_dialog(6);
        return;
    case StartupDialogAction::none:
        return;
    }
}

}  // namespace openswd3::app
