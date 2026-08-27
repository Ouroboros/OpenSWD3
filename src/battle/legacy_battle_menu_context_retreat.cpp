#include "openswd3/battle/legacy_battle_menu_context_retreat.hpp"

#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u32;

inline constexpr u32 kSelectionSample = 0x2EU;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] std::array<u8, 9>
permission_bytes(const LegacyBattleStartupResetBlocks& reset) noexcept {
    return {
        reset.value_524413,
        static_cast<u8>(reset.value_524414),
        static_cast<u8>(reset.value_524414 >> 8U),
        static_cast<u8>(reset.value_524414 >> 16U),
        static_cast<u8>(reset.value_524414 >> 24U),
        static_cast<u8>(reset.value_524418),
        static_cast<u8>(reset.value_524418 >> 8U),
        static_cast<u8>(reset.value_524418 >> 16U),
        static_cast<u8>(reset.value_524418 >> 24U),
    };
}

}  // namespace

LegacyBattleMenuContextRetreatResult retreat_legacy_battle_menu_context(
    const LegacyBattleMenuContextRetreatBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleMenuContextRetreatRequest& request
) {
    LegacyBattleMenuContextRetreatResult result;
    auto& reset = bindings.startup_reset;
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
    const auto stop = [&](const LegacyBattleMenuContextRetreatStatus status) {
        result.status = status;
        return finish();
    };
    const auto play_sample = [&]() {
        const auto reply = port.play_input_sample(
            kSelectionSample, input.sample_mix_level, eax, ecx, edx
        );
        ++result.sample_calls;
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };

    bindings.final_actor.pre_frame_gate_b = 0U;

    if (eax == 1U) {
        eax = input.action_kind;
        ecx = eax - 4U;
        if (signed_bits(ecx) >= 1) {
            eax = ecx;
            input.action_kind = eax;
        }
        const auto permissions = permission_bytes(reset);
        if (eax >= permissions.size()) {
            return stop(
                LegacyBattleMenuContextRetreatStatus::permission_typed_stop
            );
        }
        const u32 permission = permissions[eax];
        ++result.permission_reads;
        ecx = (ecx & 0xFFFFFF00U) | permission;
        if (permission == 0U) {
            eax += 4U;
            input.action_kind = eax;
        }
        eax = std::bit_cast<u32>(input.sample_mix_level);
        play_sample();
    }

    ecx = bindings.message_state;
    eax = 2U;
    if (ecx == eax) {
        ecx = input.action_category_index - 1U;
        input.action_category_index = ecx;
        if (signed_bits(ecx) < 0) {
            input.action_category_index = eax;
        }
        ecx = std::bit_cast<u32>(input.sample_mix_level);
        frame.list_selection = 1U;
        play_sample();
    }

    if (bindings.message_state == 4U) {
        eax = frame.current_equipment_selection - 1U;
        frame.current_equipment_selection = eax;
        if (signed_bits(eax) < 0) {
            eax = 3U;
            frame.current_equipment_selection = eax;
        }
        ecx = std::bit_cast<u32>(input.sample_mix_level);
        if (eax >= frame.equipment_grid_selections.size()) {
            return stop(
                LegacyBattleMenuContextRetreatStatus::
                    equipment_selection_typed_stop
            );
        }
        edx = frame.equipment_grid_selections[eax];
        ++result.equipment_selection_reads;
        if (eax >= reset.values_52544c.size()) {
            return stop(
                LegacyBattleMenuContextRetreatStatus::
                    equipment_scroll_typed_stop
            );
        }
        eax = reset.values_52544c[eax];
        ++result.equipment_scroll_reads;
        frame.grid_selection = edx;
        frame.panel_scroll_b = eax;
        play_sample();
    }

    if (bindings.message_state == 30U) {
        eax = frame.grid_selection - 5U;
        frame.grid_selection = eax;
        if (signed_bits(eax) < 1) {
            frame.grid_selection = 1U;
        }
        edx = std::bit_cast<u32>(input.sample_mix_level);
        play_sample();
    }

    return finish();
}

}  // namespace openswd3::battle
