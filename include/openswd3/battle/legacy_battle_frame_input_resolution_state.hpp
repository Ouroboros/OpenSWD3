#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

struct LegacyBattleFrameInputResolutionState {
    compat::i32 previous_mouse_x{};                 // 0x005028A0
    compat::i32 previous_mouse_y{};                 // 0x005028A4
    compat::u32 pointer_activity_gate{};            // 0x0053BDA8
    compat::u32 list_selection{1U};                 // 0x004A7558
    compat::u32 grid_selection{1U};                 // 0x004A755C
    compat::u32 narrow_list_selection{1U};          // 0x004A7560
    compat::u32 selection_actor_code{0xFFFFFFFFU};  // 0x004A7564
    compat::u32 current_equipment_selection{2U};    // 0x004A7570
    compat::u32 target_cursor{1U};                  // 0x004A7550
    compat::u32 alternate_selection_limit{2U};      // 0x004A7568
    compat::u32 alternate_selection{1U};            // 0x004A756C
    compat::u32 hovered_equipment{0xFFFFFFFFU};     // 0x004A7578
    compat::u32 hovered_secondary{0xFFFFFFFFU};     // 0x004A757C
    compat::u32 target_action_available{};          // 0x004A7640
    compat::u32 panel_scroll_a{};                   // 0x0053BD04
    compat::u32 panel_scroll_b{};                   // 0x0053BD08
    compat::u32 panel_origin_x{};                   // 0x0053BD24
    compat::u32 panel_origin_y{};                   // 0x0053BD28
    compat::u32 group_b_row_selection{};            // 0x0053BCF8
    compat::u32 target_actor_index{};               // 0x0053BCF4
    compat::u32 target_selection_gate{};            // 0x0053C49C
    compat::u32 transition_value_a{};               // 0x0053BD44
    compat::u32 transition_value_b{};               // 0x0053BD48
    compat::u32 transient_selection_a{};            // 0x0053BD6C
    compat::u32 transient_selection_b{};            // 0x0053BD70
    compat::u8 panel_row_limit_a{};                 // 0x0053BDF2
    compat::u8 panel_row_limit_b{};                 // 0x0053BDF3
    compat::u16 panel_row_limit_c{};                // 0x0053BDF4
    compat::u16 selection_block_word{};             // 0x0053BF2A
    compat::u32 target_selection_block{};           // 0x0053BF88
    compat::u32 transient_selection_c{};            // 0x0053BFF8
    compat::u32 selected_target_index{};            // 0x0053C4AC
    compat::u8 target_selection_suppression{};      // 0x0053C4C0
    compat::u32 lower_panel_top{};                  // 0x004FDC5C
    compat::u32 lower_panel_bottom{};               // 0x004FDC58
    compat::u32 lower_panel_aux{};                  // 0x004FDC60
    compat::u32 lower_panel_aux_index{};            // 0x004FDC64
    compat::u32 final_panel_top{};                  // 0x004FD790
    compat::u32 final_panel_bottom{};               // 0x004FD794
    std::array<compat::u16, 8> option_role_ids{};   // 0x004FE5CA view
    // Current-equipment selection cache at 0x004FF578.
    std::array<compat::u32, 4> equipment_grid_selections{1U, 1U, 1U, 1U};
    std::array<compat::u8, 10> target_markers{};  // 0x00524118 prefix
};

class LegacyBattleFrameInputResolutionStatePort {
public:
    [[nodiscard]] virtual LegacyBattleFrameInputResolutionState&
    battle_frame_input_resolution_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleFrameInputResolutionState&
    battle_frame_input_resolution_state() const noexcept {
        return state_;
    }

protected:
    LegacyBattleFrameInputResolutionStatePort() = default;
    ~LegacyBattleFrameInputResolutionStatePort() = default;

private:
    LegacyBattleFrameInputResolutionState state_{};
};

}  // namespace openswd3::battle
