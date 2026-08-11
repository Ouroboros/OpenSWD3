#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/resource_io/legacy_dbcs_text_buffer.hpp"

#include <array>
#include <optional>

namespace openswd3::special_modes {

inline constexpr compat::i32 kLegacyInitialMenuEntryCounter = -10;
inline constexpr compat::i32 kLegacyInitialMenuNameOneCounter = 98;
inline constexpr compat::i32 kLegacyInitialMenuNameTwoCounter = 99;
inline constexpr compat::i32 kLegacyInitialMenuExitCounter = 100;
inline constexpr compat::i32 kLegacyInitialMenuCommitCounter = 105;

enum class LegacyInitialMenuEvent : compat::u8 {
    none,
    commit_choice_0_00449291,
    commit_new_game_004492ba,
    commit_choice_2_00449318,
    commit_choice_3_00449320,
};

struct LegacyInitialMenuInput {
    compat::i32 mouse_x{};
    compat::i32 mouse_y{};
    compat::u32 mouse_button_mask{};
    const input_time_rng::LegacyInputRecord* cancel{};
    const input_time_rng::LegacyInputRecord* primary{};
    const input_time_rng::LegacyInputRecord* alternate_primary{};
    const input_time_rng::LegacyInputRecord* left{};
    const input_time_rng::LegacyInputRecord* up{};
    const input_time_rng::LegacyInputRecord* right{};
    const input_time_rng::LegacyInputRecord* down{};
    const input_time_rng::LegacyInputRecord* page_up{};
    const input_time_rng::LegacyInputRecord* page_down{};
    const input_time_rng::LegacyInputRecord* mouse_left{};
    const input_time_rng::LegacyInputRecord* mouse_right{};
};

struct LegacyInitialMenuState {
    bool initialized{};
    compat::i32 phase{};
    compat::i32 counter{};
    compat::u32 selected_choice{};
    std::array<compat::i32, 4U> slide_offsets{};
    std::array<compat::u8, 16U> first_name{};
    std::array<compat::u8, 16U> second_name{};
    std::optional<resource_io::LegacyDbcsTextBuffer> name_input{};
    asset_runtime::LegacyActionRecord background_action{};
    std::array<asset_runtime::LegacyActionRecord, 4U> choice_actions{};
    asset_runtime::LegacyActionRecord name_button_action{};
};

enum class LegacyInitialMenuDrawStatus : compat::u8 {
    completed,
    background_failed,
    choice_failed,
    name_panel_failed,
    name_button_failed,
};

struct LegacyInitialMenuFrameResult {
    LegacyInitialMenuEvent event{LegacyInitialMenuEvent::none};
    LegacyInitialMenuDrawStatus draw_status{
        LegacyInitialMenuDrawStatus::completed
    };
    compat::u32 action_update_count{};
    compat::u32 frame_request_count{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
};

void initialize_legacy_initial_menu(LegacyInitialMenuState& state) noexcept;

// Mode 3 input callbacks and terminal callback for the path
// 0x00448840..0x00449311. The callback order and strict mouse hit boxes match
// sub_43A470; the returned new-game event is emitted only when the original
// phase-two counter advances past 104.
[[nodiscard]] LegacyInitialMenuFrameResult run_legacy_initial_menu_frame(
    LegacyInitialMenuState& state,
    const LegacyInitialMenuInput& input,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    rendering::LegacyBlitEffectState& effects
) noexcept;

}  // namespace openswd3::special_modes
