#include "openswd3/battle/legacy_battle_menu_page_advance.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i8;
using compat::i32;
using compat::u32;

inline constexpr u32 kSelectionSample = 0x2EU;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 signed_byte(const compat::u8 value) noexcept {
    return static_cast<i32>(std::bit_cast<i8>(value));
}

}  // namespace

LegacyBattleMenuPageAdvanceResult advance_legacy_battle_menu_page(
    LegacyBattleMenuPageAdvanceBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuPageAdvanceRequest& request
) {
    LegacyBattleMenuPageAdvanceResult result;
    auto& frame = bindings.frame_input_resolution;
    auto& input = bindings.input_dispatch;
    u32 eax = bindings.message_state;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto play_sample = [&]() {
        eax = std::bit_cast<u32>(input.sample_mix_level);
        const auto reply = port.play_input_sample(
            kSelectionSample, input.sample_mix_level, eax, ecx, edx
        );
        ++result.port_calls;
        ++result.sample_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };
    const auto publish_equipment = [&](const u32 grid, const u32 scroll) {
        if (ecx >= frame.equipment_grid_selections.size()) {
            return false;
        }
        frame.equipment_grid_selections[ecx] = grid;
        if (ecx >= bindings.startup_reset.values_52544c.size()) {
            result.status =
                LegacyBattleMenuPageAdvanceStatus::equipment_scroll_typed_stop;
            return false;
        }
        bindings.startup_reset.values_52544c[ecx] = scroll;
        return true;
    };
    const auto normalize_grid = [&]() {
        const u32 limit = static_cast<u32>(frame.panel_row_limit_c);
        eax = 7U;
        if (limit < eax) {
            eax = limit;
        }
        frame.grid_selection = eax;
        ecx = frame.current_equipment_selection;
        edx = frame.panel_scroll_b;
        input.mouse_action_gate = 1U;
        if (!publish_equipment(eax, edx)) {
            if (result.status == LegacyBattleMenuPageAdvanceStatus::completed) {
                result.status = LegacyBattleMenuPageAdvanceStatus::
                    equipment_selection_typed_stop;
            }
            return false;
        }
        return true;
    };

    eax -= 2U;
    bindings.final_actor.pre_frame_gate_b = 0U;
    if (eax == 0U) {
        play_sample();
        ecx = (ecx & 0xFFFFFF00U) | static_cast<u32>(frame.panel_row_limit_a);
        if (signed_byte(frame.panel_row_limit_a) < 7) {
            return finish();
        }
        eax = input.menu_action;
        if (eax == 0U && frame.list_selection != 7U) {
            frame.list_selection = 7U;
            return finish();
        }

        eax = frame.panel_scroll_a + 7U;
        ecx = std::bit_cast<u32>(signed_byte(frame.panel_row_limit_a));
        edx = eax + 7U;
        frame.panel_scroll_a = eax;
        if (signed_bits(edx) > signed_bits(ecx)) {
            eax = ecx - 7U;
            frame.panel_scroll_a = eax;
        }
        if (signed_bits(eax) < 0) {
            frame.panel_scroll_a = 0U;
            frame.list_selection = ecx;
        }
        input.mouse_action_gate = 1U;
        return finish();
    }

    eax -= 2U;
    if (eax == 0U) {
        play_sample();
        eax = input.menu_action;
        edx = frame.grid_selection;
        if (eax == 0U && edx != 7U) {
            if (!normalize_grid()) {
                return finish();
            }
            return finish();
        }

        eax = frame.panel_scroll_b + 7U;
        ecx = static_cast<u32>(frame.panel_row_limit_c);
        frame.panel_scroll_b = eax;
        const u32 next_page_end = eax + 7U;
        if (signed_bits(next_page_end) > signed_bits(ecx)) {
            eax = ecx - 7U;
            frame.panel_scroll_b = eax;
        }
        if (signed_bits(eax) < 0) {
            eax = 0U;
            edx = ecx;
            frame.panel_scroll_b = eax;
            frame.grid_selection = edx;
        }
        ecx = frame.current_equipment_selection;
        input.mouse_action_gate = 1U;
        if (!publish_equipment(edx, eax)) {
            if (result.status == LegacyBattleMenuPageAdvanceStatus::completed) {
                result.status = LegacyBattleMenuPageAdvanceStatus::
                    equipment_selection_typed_stop;
            }
            return finish();
        }
        return finish();
    }

    eax -= 0x17U;
    if (eax != 0U) {
        return finish();
    }

    play_sample();
    eax = input.menu_action;
    if (eax == 0U && frame.grid_selection != 7U) {
        if (!normalize_grid()) {
            return finish();
        }
        return finish();
    }

    ecx = frame.panel_scroll_b;
    eax = static_cast<u32>(frame.panel_row_limit_c);
    ecx += 7U;
    frame.panel_scroll_b = ecx;
    edx = ecx + 7U;
    if (signed_bits(edx) > signed_bits(eax)) {
        ecx = eax - 7U;
        frame.panel_scroll_b = ecx;
    }
    if (signed_bits(ecx) < 0) {
        frame.panel_scroll_b = 0U;
        frame.grid_selection = eax;
    }
    input.mouse_action_gate = 1U;
    return finish();
}

}  // namespace openswd3::battle
