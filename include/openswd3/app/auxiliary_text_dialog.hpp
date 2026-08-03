#pragma once

#include "openswd3/compat/types.hpp"

#include <string>
#include <string_view>

namespace openswd3::app {

inline constexpr compat::u32 kAuxiliaryTextEditControlId = 0x40AU;
inline constexpr compat::u32 kAuxiliaryTextCancelControlId = 0x40CU;
inline constexpr compat::u32 kAuxiliaryTextEditLimitBytes = 8U;
inline constexpr compat::u32 kAuxiliaryTextCopyBufferBytes = 8U;
inline constexpr compat::u32 kAuxiliaryTextInitMessage = 0x46AU;
inline constexpr compat::u32 kAuxiliaryTextInitFirstParameter = 1U;
inline constexpr compat::u32 kAuxiliaryTextInitSecondParameter = 0x42U;

struct AuxiliaryTextDialogState {
    std::string committed_bytes;
};

enum class AuxiliaryTextDialogAction {
    none,
    show_empty_warning,
    end_dialog_0,
    end_dialog_1,
};

[[nodiscard]] AuxiliaryTextDialogAction handle_auxiliary_text_command(
    AuxiliaryTextDialogState& state,
    compat::u32 command_value,
    std::string_view visible_edit_bytes
);

[[nodiscard]] AuxiliaryTextDialogAction auxiliary_text_close_action() noexcept;

class AuxiliaryTextDialogPorts {
public:
    virtual ~AuxiliaryTextDialogPorts() = default;

    virtual void show_empty_warning() = 0;
    virtual void end_dialog(compat::i32 numeric_result) = 0;
};

void execute_auxiliary_text_dialog_action(
    AuxiliaryTextDialogAction action,
    AuxiliaryTextDialogPorts& ports
);

}  // namespace openswd3::app
