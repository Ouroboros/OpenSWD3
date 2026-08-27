#include "openswd3/battle/legacy_battle_menu_page_retreat.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

inline constexpr u32 kSelectionSample = 0x2EU;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

}  // namespace

LegacyBattleMenuPageRetreatResult retreat_legacy_battle_menu_page(
    LegacyBattleMenuPageRetreatBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuPageRetreatRequest& request
) {
    LegacyBattleMenuPageRetreatResult result;
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
    const auto stop = [&](const LegacyBattleMenuPageRetreatStatus status) {
        result.status = status;
        return finish();
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

    eax -= 2U;
    bindings.final_actor.pre_frame_gate_b = 0U;
    if (eax == 0U) {
        play_sample();
        ecx = input.menu_action;
        eax = 1U;
        if (ecx == 0U && frame.grid_selection != eax) {
            frame.grid_selection = eax;
            return finish();
        }
        ecx = frame.panel_scroll_b - 7U;
        frame.panel_scroll_b = ecx;
        if (signed_bits(ecx) < 0) {
            frame.panel_scroll_b = 0U;
        }
        input.mouse_action_gate = eax;
        return finish();
    }

    eax -= 2U;
    if (eax == 0U) {
        play_sample();
        ecx = input.menu_action;
        const bool menu_action_is_zero = ecx == 0U;
        ecx = frame.grid_selection;
        eax = 1U;
        if (menu_action_is_zero && ecx != eax) {
            frame.grid_selection = eax;
            return finish();
        }
        edx = frame.panel_scroll_b - 7U;
        frame.panel_scroll_b = edx;
        if (signed_bits(edx) < 0) {
            frame.panel_scroll_b = 0U;
        }
        edx = frame.panel_scroll_b;
        input.mouse_action_gate = eax;
        eax = frame.current_equipment_selection;
        if (eax >= frame.equipment_grid_selections.size()) {
            return stop(
                LegacyBattleMenuPageRetreatStatus::
                    equipment_selection_typed_stop
            );
        }
        frame.equipment_grid_selections[eax] = ecx;
        if (eax >= bindings.startup_reset.values_52544c.size()) {
            return stop(
                LegacyBattleMenuPageRetreatStatus::equipment_scroll_typed_stop
            );
        }
        bindings.startup_reset.values_52544c[eax] = edx;
        return finish();
    }

    eax -= 0x17U;
    if (eax != 0U) {
        return finish();
    }

    play_sample();
    ecx = input.menu_action;
    eax = 1U;
    if (ecx == 0U && frame.list_selection != eax) {
        frame.list_selection = eax;
        return finish();
    }
    ecx = frame.panel_scroll_a - 7U;
    frame.panel_scroll_a = ecx;
    if (signed_bits(ecx) < 0) {
        frame.panel_scroll_a = 0U;
    }
    input.mouse_action_gate = eax;
    return finish();
}

}  // namespace openswd3::battle
