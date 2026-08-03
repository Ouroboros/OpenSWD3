#include "openswd3/app/auxiliary_text_dialog.hpp"

#include <algorithm>
#include <utility>

namespace openswd3::app {

namespace {

[[nodiscard]] std::string copy_legacy_edit_bytes(
    const std::string_view visible_edit_bytes
) {
    const std::size_t nul = visible_edit_bytes.find('\0');
    const std::size_t source_size =
        nul == std::string_view::npos ? visible_edit_bytes.size() : nul;
    const std::size_t copied_size = std::min(
        source_size,
        static_cast<std::size_t>(kAuxiliaryTextCopyBufferBytes - 1U)
    );
    return std::string{visible_edit_bytes.substr(0U, copied_size)};
}

}  // namespace

AuxiliaryTextDialogAction handle_auxiliary_text_command(
    AuxiliaryTextDialogState& state,
    const compat::u32 command_value,
    const std::string_view visible_edit_bytes
) {
    switch (static_cast<compat::u16>(command_value)) {
    case 1U: {
        std::string submitted = copy_legacy_edit_bytes(visible_edit_bytes);
        if (submitted.empty()) {
            return AuxiliaryTextDialogAction::show_empty_warning;
        }
        state.committed_bytes = std::move(submitted);
        return AuxiliaryTextDialogAction::end_dialog_1;
    }
    case kAuxiliaryTextCancelControlId:
        return AuxiliaryTextDialogAction::end_dialog_0;
    default:
        return AuxiliaryTextDialogAction::none;
    }
}

AuxiliaryTextDialogAction auxiliary_text_close_action() noexcept {
    return AuxiliaryTextDialogAction::end_dialog_0;
}

void execute_auxiliary_text_dialog_action(
    const AuxiliaryTextDialogAction action,
    AuxiliaryTextDialogPorts& ports
) {
    switch (action) {
    case AuxiliaryTextDialogAction::show_empty_warning:
        ports.show_empty_warning();
        return;
    case AuxiliaryTextDialogAction::end_dialog_0:
        ports.end_dialog(0);
        return;
    case AuxiliaryTextDialogAction::end_dialog_1:
        ports.end_dialog(1);
        return;
    case AuxiliaryTextDialogAction::none:
        return;
    }
}

}  // namespace openswd3::app
