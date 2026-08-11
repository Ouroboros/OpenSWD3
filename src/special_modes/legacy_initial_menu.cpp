#include "openswd3/special_modes/legacy_initial_menu.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::special_modes {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr std::array<i32, 4U> kChoiceX{0xD2, 0x107, 0x140, 0x179};
constexpr i32 kChoiceY = 0x7D;
constexpr std::array<u8, 5U> kFirstDefaultName{0xC1U, 0xC9U, 0xAFU, 0x53U, 0U};
constexpr std::array<u8, 5U> kSecondDefaultName{0xA9U, 0x67U, 0xA5U, 0x69U, 0U};

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(std::bit_cast<u32>(left) + std::bit_cast<u32>(right));
}

[[nodiscard]] constexpr i32 wrapping_subtract(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(std::bit_cast<u32>(left) - std::bit_cast<u32>(right));
}

[[nodiscard]] constexpr bool strict_between(
    const i32 value,
    const i32 low,
    const i32 high
) noexcept {
    return value > low && value < high;
}

[[nodiscard]] bool first_press(
    const input_time_rng::LegacyInputRecord* const record
) noexcept {
    return record != nullptr && record->rapid_press_multiplicity != 0U &&
           record->held_sample_count == 1U;
}

[[nodiscard]] bool repeated_press(
    const input_time_rng::LegacyInputRecord* const record
) noexcept {
    if (record == nullptr || record->rapid_press_multiplicity == 0U) {
        return false;
    }
    return record->held_sample_count == 1U ||
           (record->held_sample_count > 7U &&
            (record->held_sample_count & 1U) == 0U);
}

void copy_default_names(LegacyInitialMenuState& state) noexcept {
    std::ranges::fill(state.first_name, u8{});
    std::ranges::fill(state.second_name, u8{});
    std::ranges::copy(kFirstDefaultName, state.first_name.begin());
    std::ranges::copy(kSecondDefaultName, state.second_name.begin());
}

void submit_current_choice(LegacyInitialMenuState& state) noexcept {
    if (state.phase != 1) {
        return;
    }

    state.phase = 2;
    state.counter = kLegacyInitialMenuExitCounter;
    switch (state.selected_choice) {
        case 1U:
            // sub_448EE0 stores 0x61 and then immediately increments it after
            // constructing the first 32-byte text-input object.
            state.counter = kLegacyInitialMenuNameOneCounter;
            copy_default_names(state);
            break;
        case 2U:
            state.phase = 5;
            state.counter = 0;
            break;
        case 3U:
            // The original launches the external helper here and leaves phase
            // two in place. Its platform owner is outside the new-game path.
            break;
        default:
            break;
    }
}

[[nodiscard]] bool process_phase_one_input(
    LegacyInitialMenuState& state,
    const LegacyInitialMenuInput& input
) noexcept {
    if (state.phase != 1) {
        return false;
    }

    if (strict_between(input.mouse_y, 0x6E, 0x104)) {
        constexpr std::array<i32, 4U> kHitHigh{0xE8, 0x11D, 0x156, 0x18F};
        for (std::size_t index = 0U; index < kChoiceX.size(); ++index) {
            if (!strict_between(input.mouse_x, kChoiceX[index], kHitHigh[index])) {
                continue;
            }
            state.selected_choice = static_cast<u32>(index);
            if ((input.mouse_button_mask & 3U) != 0U) {
                submit_current_choice(state);
                return true;
            }
            break;
        }
    }

    // sub_43A470 invokes cancel before the directional callbacks. In phase
    // one cancel selects item three; it does not submit the item.
    if (state.phase == 1 && first_press(input.cancel)) {
        state.selected_choice = 3U;
    }
    if (state.phase == 1 && repeated_press(input.down)) {
        state.selected_choice = std::min(state.selected_choice + 1U, 3U);
    }
    if (state.phase == 1 && repeated_press(input.up)) {
        state.selected_choice = state.selected_choice == 0U
                                    ? 0U
                                    : state.selected_choice - 1U;
    }
    if (state.phase == 1 && repeated_press(input.page_down)) {
        state.selected_choice = 3U;
    }
    if (state.phase == 1 && repeated_press(input.page_up)) {
        state.selected_choice = 0U;
    }
    if (state.phase == 1 && repeated_press(input.left)) {
        state.selected_choice = state.selected_choice == 0U
                                    ? 0U
                                    : state.selected_choice - 1U;
    }
    if (state.phase == 1 && repeated_press(input.right)) {
        state.selected_choice = std::min(state.selected_choice + 1U, 3U);
    }
    if (state.phase == 1 &&
        (first_press(input.primary) || first_press(input.alternate_primary))) {
        submit_current_choice(state);
        return true;
    }
    return false;
}

[[nodiscard]] bool process_name_input(
    LegacyInitialMenuState& state,
    const LegacyInitialMenuInput& input
) noexcept {
    if (state.phase != 2 ||
        (state.counter != kLegacyInitialMenuNameOneCounter &&
         state.counter != kLegacyInitialMenuNameTwoCounter)) {
        return false;
    }

    const bool mouse_accept = first_press(input.mouse_left) &&
                              strict_between(input.mouse_x, 0x101, 0x112) &&
                              strict_between(input.mouse_y, 0x162, 0x198);
    const bool accepted = mouse_accept || first_press(input.primary) ||
                          first_press(input.alternate_primary);
    const bool cancelled = first_press(input.mouse_right) ||
                           first_press(input.cancel);
    if (cancelled) {
        state.phase = 1;
        return true;
    }
    if (accepted) {
        ++state.counter;
        return true;
    }
    return false;
}

void advance_slide_offsets(LegacyInitialMenuState& state) noexcept {
    if (state.phase > 2) {
        return;
    }
    const std::size_t selected =
        std::min<std::size_t>(state.selected_choice, 3U);
    state.slide_offsets[selected] =
        std::max(wrapping_add(state.slide_offsets[selected], -4), -32);
    for (i32& offset : state.slide_offsets) {
        offset = std::min(wrapping_add(offset, 2), -12);
    }
}

void accumulate_draw_result(
    LegacyInitialMenuFrameResult& destination,
    const asset_runtime::LegacyActionDrawResult& source
) noexcept {
    destination.action_update_count += source.action_update_count;
    destination.frame_request_count += source.frame_request_count;
    destination.draw_count += source.draw_count;
    destination.blit_failure_count += source.blit_failure_count;
}

[[nodiscard]] bool accepted_blit(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
           status == rendering::LegacyBlitExecutionStatus::clipped_out ||
           status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] bool draw_choice(
    asset_runtime::LegacyActionRecord& action,
    const i32 x,
    const i32 slide_offset,
    asset_runtime::LegacyActionDrawPorts& ports,
    rendering::LegacyBlitEffectState& effects,
    LegacyInitialMenuFrameResult& result
) {
    ++result.action_update_count;
    if (ports.update_action_record(action) !=
        asset_runtime::LegacyActionUpdateStatus::completed) {
        return false;
    }

    rendering::LegacyFramePiece piece;
    ++result.frame_request_count;
    if (!ports.load_frame_piece(action.field_4a, action.field_4c, piece)) {
        return false;
    }

    const u32 flags = 4U | (action.mode_flags & 0x80000003U);
    const i32 draw_x = wrapping_subtract(x, from_bits(action.draw_offset_x));
    const i32 draw_y = wrapping_subtract(kChoiceY, from_bits(action.draw_offset_y));
    const auto draw = [&](const i32 destination_x) {
        const auto status = ports.draw_frame_piece(
            piece,
            destination_x,
            draw_y,
            flags,
            0
        );
        ++result.draw_count;
        if (!accepted_blit(status)) {
            ++result.blit_failure_count;
        }
    };

    effects.red_offset = wrapping_subtract(-25, slide_offset);
    effects.green_offset = effects.red_offset;
    effects.blue_offset = effects.red_offset;
    draw(draw_x);

    i32 trail_color = slide_offset;
    i32 trail_distance = wrapping_subtract(-12, slide_offset);
    while (trail_color < -12) {
        effects.red_offset = trail_color;
        effects.green_offset = trail_color;
        effects.blue_offset = trail_color;
        draw(wrapping_add(draw_x, trail_distance));

        const i32 signed_bias = trail_distance < 0 ? 1 : 0;
        const i32 half = (trail_distance - signed_bias) / 2;
        draw(wrapping_subtract(draw_x, half));
        ++trail_color;
        --trail_distance;
    }
    return true;
}

void draw_menu(
    LegacyInitialMenuState& state,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    rendering::LegacyBlitEffectState& effects,
    LegacyInitialMenuFrameResult& result
) {
    const auto background = asset_runtime::update_draw_legacy_action(
        state.background_action,
        0,
        0,
        action_ports
    );
    accumulate_draw_result(result, background);
    if (background.status != asset_runtime::LegacyActionDrawStatus::ready) {
        result.draw_status = LegacyInitialMenuDrawStatus::background_failed;
        return;
    }

    if (state.phase > 2) {
        return;
    }
    for (std::size_t index = 0U; index < state.choice_actions.size(); ++index) {
        if (!draw_choice(
                state.choice_actions[index],
                kChoiceX[index],
                state.slide_offsets[index],
                action_ports,
                effects,
                result
            )) {
            result.draw_status = LegacyInitialMenuDrawStatus::choice_failed;
            return;
        }
    }
}

[[nodiscard]] LegacyInitialMenuEvent commit_event(const u32 choice) noexcept {
    switch (choice) {
        case 0U: return LegacyInitialMenuEvent::commit_choice_0_00449291;
        case 1U: return LegacyInitialMenuEvent::commit_new_game_004492ba;
        case 2U: return LegacyInitialMenuEvent::commit_choice_2_00449318;
        case 3U: return LegacyInitialMenuEvent::commit_choice_3_00449320;
        default: return LegacyInitialMenuEvent::none;
    }
}

}  // namespace

void initialize_legacy_initial_menu(LegacyInitialMenuState& state) noexcept {
    state = {};
    state.initialized = true;
    state.phase = 0;
    state.counter = kLegacyInitialMenuEntryCounter;
    state.slide_offsets.fill(-16);
    copy_default_names(state);

    asset_runtime::initialize_legacy_action_record(state.background_action);
    state.background_action.action_id = 0x232AU;
    state.background_action.base_variant = 0x4EU;
    for (std::size_t index = 0U; index < state.choice_actions.size(); ++index) {
        auto& action = state.choice_actions[index];
        asset_runtime::initialize_legacy_action_record(action);
        action.action_id = 0x232BU;
        action.base_variant = static_cast<u32>(0x2CU + index);
    }
}

LegacyInitialMenuFrameResult run_legacy_initial_menu_frame(
    LegacyInitialMenuState& state,
    const LegacyInitialMenuInput& input,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    rendering::LegacyBlitEffectState& effects
) noexcept {
    if (!state.initialized) {
        initialize_legacy_initial_menu(state);
    }

    LegacyInitialMenuFrameResult result;
    try {
        const bool submitted = process_phase_one_input(state, input);
        const bool name_input_handled =
            !submitted && process_name_input(state, input);
        advance_slide_offsets(state);
        draw_menu(state, action_ports, effects, result);
        if (submitted || name_input_handled) {
            return result;
        }
        if (state.phase <= 2 && state.counter < 0) {
            ++state.counter;
            if (state.counter >= 0) {
                state.counter = 0;
                state.phase = 1;
            }
            return result;
        }
        if (state.phase == 2 && state.counter >= kLegacyInitialMenuExitCounter) {
            ++state.counter;
            if (state.counter >= kLegacyInitialMenuCommitCounter) {
                result.event = commit_event(state.selected_choice);
            }
        }
    } catch (...) {
        result.draw_status = LegacyInitialMenuDrawStatus::choice_failed;
    }
    return result;
}

}  // namespace openswd3::special_modes
