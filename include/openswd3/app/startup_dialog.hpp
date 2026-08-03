#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::app {

inline constexpr compat::u32 kStartupDialogWidth = 640U;
inline constexpr compat::u32 kStartupDialogHeight = 480U;
inline constexpr compat::u8 kStartupDialogControlHidden = 0U;
inline constexpr compat::u8 kStartupDialogControlShown = 5U;

struct StartupDialogControlLayout {
    compat::u32 control_id{};
    compat::u32 x{};
    compat::u32 y{};
    compat::u32 width{};
    compat::u32 height{};

    bool operator==(const StartupDialogControlLayout&) const = default;
};

inline constexpr std::array<StartupDialogControlLayout, 5>
    kStartupDialogControlLayouts{
        StartupDialogControlLayout{0x40EU, 48U, 4U, 39U, 180U},
        StartupDialogControlLayout{0x40FU, 92U, 4U, 39U, 180U},
        StartupDialogControlLayout{0x410U, 489U, 4U, 39U, 180U},
        StartupDialogControlLayout{0x411U, 531U, 4U, 39U, 180U},
        StartupDialogControlLayout{0x412U, 570U, 4U, 39U, 180U},
    };

struct StartupDialogState {
    std::array<compat::u8, 5> control_visibility{};
};

void initialize_startup_dialog(StartupDialogState& state) noexcept;

enum class StartupDialogAction {
    none,
    show_auxiliary_then_end_dialog_2,
    open_url,
    open_readme,
    end_dialog_1,
    end_dialog_2,
    end_dialog_3,
    end_dialog_6,
};

[[nodiscard]] StartupDialogAction select_startup_dialog_command(
    compat::u32 command_value
) noexcept;

[[nodiscard]] StartupDialogAction startup_dialog_close_action() noexcept;

enum class StartupDialogPointerMessage {
    move,
    left_button_down,
};

struct StartupDialogPointerResult {
    StartupDialogAction action{};
    bool play_hover_sound{};
};

[[nodiscard]] StartupDialogPointerResult update_startup_dialog_pointer(
    StartupDialogState& state,
    bool any_save_exists,
    StartupDialogPointerMessage message,
    compat::u32 packed_position
) noexcept;

class StartupDialogPorts {
public:
    virtual ~StartupDialogPorts() = default;

    virtual void show_auxiliary_dialog() = 0;
    virtual void open_url() = 0;
    virtual void open_readme() = 0;
    virtual void end_dialog(compat::i32 numeric_result) = 0;
};

void execute_startup_dialog_action(
    StartupDialogAction action,
    StartupDialogPorts& ports
);

}  // namespace openswd3::app
