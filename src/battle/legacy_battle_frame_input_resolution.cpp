#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::i8;
using compat::u16;
using compat::u32;
using compat::u8;

inline constexpr u32 kGroupABase = 0x005029D0U;
inline constexpr u32 kGroupAStride = 0x2F34U;
inline constexpr u32 kGroupBBase = 0x00525508U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kSelectionSample = 0x2EU;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 unsigned_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kGroupABase + index * kGroupAStride;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kGroupBBase + index * kGroupBStride;
}

[[nodiscard]] constexpr bool
strict_inside(const u32 value, const u32 lower, const u32 upper) noexcept {
    return value > lower && value < upper;
}

[[nodiscard]] constexpr u8 permission_byte(
    const LegacyBattleStartupResetBlocks& reset, const std::size_t index
) noexcept {
    const u32 word = index < 4U ? reset.value_524414 : reset.value_524418;
    const std::size_t byte_index = index < 4U ? index : index - 4U;
    return static_cast<u8>((word >> (byte_index * 8U)) & 0xFFU);
}

}  // namespace

LegacyBattleFrameInputResolutionResult
coordinate_legacy_battle_frame_input_resolution(
    LegacyBattleFrameInputResolutionBindings bindings,
    LegacyBattleFrameInputResolutionPort& port,
    const LegacyBattleFrameInputResolutionRequest& request
) {
    LegacyBattleFrameInputResolutionResult result;
    auto& state = port.battle_frame_input_resolution_state();
    auto& input_state = bindings.input_dispatch;
    u32 eax = request.entry_eax;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto return_zero = [&]() {
        eax = 0U;
        result.returned_early = true;
        return finish();
    };
    const auto return_one = [&]() {
        eax = 1U;
        result.returned_early = true;
        return finish();
    };
    const auto stop = [&](const LegacyBattleFrameInputResolutionStatus status) {
        result.status = status;
        return finish();
    };
    const auto call = [&](const LegacyBattleFrameInputResolutionCall operation,
                          const u32 actor_token,
                          const std::array<u32, 4> arguments = {}) {
        const auto reply = port.invoke_frame_input_resolution({
            .call = operation,
            .actor_token = actor_token,
            .arguments = arguments,
            .eax = eax,
            .ecx = actor_token,
            .edx = edx,
        });
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        return reply;
    };
    const auto play_selection_sample = [&]() {
        edx = std::bit_cast<u32>(input_state.sample_mix_level);
        const auto reply = port.play_input_sample(
            kSelectionSample, input_state.sample_mix_level, eax, ecx, edx
        );
        ++result.sample_calls;
        ++result.port_calls;
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };

    u32 mouse_x = unsigned_bits(bindings.input.current_mouse.logical_x);
    u32 mouse_y = unsigned_bits(bindings.input.current_mouse.logical_y);
    eax = unsigned_bits(state.previous_mouse_x);
    if (eax == mouse_x && unsigned_bits(state.previous_mouse_y) == mouse_y) {
        eax = bindings.final_actor.pre_frame_gate_b;
        if (eax == 0U) {
            return return_zero();
        }
    } else {
        bindings.final_actor.pre_frame_gate_b = 1U;
        state.pointer_activity_gate = 0U;
    }
    state.previous_mouse_x = bindings.input.current_mouse.logical_x;
    state.previous_mouse_y = bindings.input.current_mouse.logical_y;

    if (!bindings.choice_hotspots.empty()) {
        const auto hit = world_map::find_legacy_world_choice_hotspot(
            bindings.choice_hotspots, mouse_x, mouse_y
        );
        ++result.hotspot_queries;
        eax = hit.index;
        input_state.choice_guard = hit.hotspot == nullptr ? 0U : hit.index + 1U;
        if (input_state.choice_guard != 0U) {
            input_state.choice_selection_index = eax;
        }
        mouse_x = unsigned_bits(bindings.input.current_mouse.logical_x);
        mouse_y = unsigned_bits(bindings.input.current_mouse.logical_y);
    }

    const auto party_hover = [&](const bool stop_on_first_match) {
        ecx = 0U;
        const i32 count = signed_bits(bindings.metrics.group_a_count);
        if (count <= 0) {
            return true;
        }
        while (signed_bits(ecx) < count) {
            if (ecx >= bindings.startup.party_source_indices.size()) {
                result.status = LegacyBattleFrameInputResolutionStatus::
                    party_source_index_typed_stop;
                return false;
            }
            const u32 source = bindings.startup.party_source_indices[ecx];
            if (source >= bindings.startup.party_offsets.size()) {
                result.status = LegacyBattleFrameInputResolutionStatus::
                    party_offset_typed_stop;
                return false;
            }
            eax = unsigned_bits(bindings.startup.party_offsets[source]);
            const bool hit = strict_inside(mouse_x, eax - 0x18U, eax + 0x74U);
            if (hit) {
                input_state.selected_option_word = static_cast<u16>(ecx + 8U);
                if (stop_on_first_match) {
                    ecx += 8U;
                    return true;
                }
            }
            ++ecx;
        }
        return true;
    };

    const auto select_row = [&](const u32 first_bottom,
                                const u32 step,
                                const u32 terminal,
                                const u32 lower_x,
                                const u32 upper_x,
                                u32& output,
                                const bool signed_limit,
                                const u32 limit,
                                const u32 comparison_offset,
                                const bool one_based) {
        if (!strict_inside(mouse_x, lower_x, upper_x)) {
            return false;
        }
        ecx = 0U;
        eax = first_bottom;
        while (!strict_inside(mouse_y, eax - step, eax)) {
            eax += step;
            ++ecx;
            if (eax >= terminal) {
                return false;
            }
        }
        const u32 selected = ecx + (one_based ? 1U : 0U);
        const u32 compared = selected + comparison_offset;
        const bool accepted = signed_limit
            ? signed_bits(compared) <= signed_bits(limit)
            : compared <= limit;
        if (!accepted) {
            return false;
        }
        if (output != selected) {
            play_selection_sample();
        }
        output = selected;
        return true;
    };

    const auto set_common_success = [&]() {
        bindings.final_actor.pre_frame_gate_b = 1U;
        input_state.mouse_action_gate = 1U;
    };

    const auto actor_surface_hit = [&](const u32 actor_token,
                                       const u32 outer_step,
                                       const bool require_present,
                                       const bool accept_visible,
                                       bool& typed_stop) {
        typed_stop = false;
        const auto origin = call(
            LegacyBattleFrameInputResolutionCall::prepare_actor_origin,
            actor_token
        );
        const i32 origin_x = origin.origin_x;
        const i32 origin_y = origin.origin_y;
        const auto resolved = call(
            LegacyBattleFrameInputResolutionCall::resolve_actor_surface,
            actor_token
        );
        const auto& surface = resolved.surface;
        if (require_present) {
            if (!surface.command_stream_present) {
                return false;
            }
            state.target_action_available = 1U;
        }
        for (u32 outer = 0U; outer < 8U; outer += outer_step) {
            for (u32 inner = 0U; inner < 8U; inner += outer_step) {
                const auto mirror = call(
                    LegacyBattleFrameInputResolutionCall::query_actor_mirror,
                    actor_token
                );
                u32 point_x = mouse_x + inner;
                if (mirror.eax == 1U) {
                    point_x = static_cast<u32>(surface.width) +
                        2U * unsigned_bits(origin_x) - mouse_x - inner;
                }
                const u32 point_y = mouse_y + outer;
                const auto query =
                    rendering::query_legacy_image_command_stream_point(
                        surface.command_stream,
                        surface.width,
                        surface.height,
                        std::bit_cast<i32>(point_x),
                        std::bit_cast<i32>(point_y),
                        origin_x,
                        origin_y
                    );
                ++result.image_queries;
                eax = query.return_value;
                if (query.status ==
                    rendering::LegacyImagePointQueryStatus::source_exhausted) {
                    typed_stop = true;
                    result.status = LegacyBattleFrameInputResolutionStatus::
                        image_source_typed_stop;
                    return false;
                }
                if (query.return_value != 0U) {
                    if (accept_visible) {
                        return true;
                    }
                    continue;
                }
                input_state.mouse_action_gate = 0U;
            }
        }
        return false;
    };

    constexpr std::array<u8, 31> kSwitchIndices{
        0U, 1U, 2U, 3U, 4U, 5U, 9U, 9U, 6U, 9U, 9U, 9U, 9U, 9U, 9U, 9U,
        9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 7U, 9U, 9U, 8U,
    };
    if (bindings.message_state <= 30U) {
        ecx = kSwitchIndices[bindings.message_state];
    }

    switch (bindings.message_state) {
    case 0U: {
        edx = static_cast<u32>(
            static_cast<u8>(bindings.final_actor.excluded_group_a_count)
        );
        eax = bindings.metrics.group_b_count;
        if (edx >= eax || mouse_y <= 0x182U || mouse_y >= 0x1E0U) {
            input_state.selected_option_word = 0xFFFFU;
            return return_zero();
        }
        if (!party_hover(true)) {
            return stop(result.status);
        }
        if (ecx < 8U) {
            input_state.selected_option_word = 0xFFFFU;
            return return_zero();
        }
        return return_one();
    }
    case 1U: {
        const u32 active_actor = bindings.final_actor.active_actor_code;
        if (active_actor == 0U) {
            return return_zero();
        }
        const u32 origin_x = state.panel_origin_x;
        const u32 origin_y = state.panel_origin_y;
        if (strict_inside(mouse_x, origin_x + 0x0AU, origin_x + 0x76U) &&
            strict_inside(mouse_y, origin_y + 0x28U, origin_y + 0x88U)) {
            const u32 column = (mouse_x - origin_x - 0x0AU) / 0x36U;
            const u32 row = (mouse_y - origin_y - 0x28U) / 0x18U;
            u32 selected = row + 4U * column;
            const u32 maximum =
                static_cast<u32>(bindings.startup.reset.value_53bf22) + 5U;
            if (signed_bits(selected) < signed_bits(maximum)) {
                if (selected >= 8U) {
                    return stop(
                        LegacyBattleFrameInputResolutionStatus::
                            permission_typed_stop
                    );
                }
                if (permission_byte(bindings.startup.reset, selected) == 0U) {
                    return return_zero();
                }
                if (selected >= 4U) {
                    if (selected >= state.option_role_ids.size()) {
                        return stop(
                            LegacyBattleFrameInputResolutionStatus::
                                option_role_typed_stop
                        );
                    }
                    const u32 actor_token = group_a_token(active_actor - 8U);
                    const auto validation = call(
                        LegacyBattleFrameInputResolutionCall::
                            validate_option_actor,
                        actor_token,
                        {state.option_role_ids[selected]}
                    );
                    if (validation.eax == 0U ||
                        permission_byte(bindings.startup.reset, selected) ==
                            0U) {
                        return return_zero();
                    }
                }
                ++selected;
                eax = input_state.selection_index;
                if (eax != selected) {
                    play_selection_sample();
                }
                ecx = selected;
                input_state.selection_index = selected;
                eax =
                    static_cast<u32>(bindings.startup.reset.value_53bf22) + 5U;
                if (ecx > eax) {
                    input_state.selection_index = eax;
                }
                set_common_success();
                return return_one();
            }
        }
        ecx = 0U;
        input_state.mouse_action_gate = 0U;
        input_state.selected_option_word = 0xFFFFU;
        if (mouse_y > 0x182U && mouse_y < 0x1E0U &&
            signed_bits(bindings.metrics.group_a_count) > 0) {
            if (!party_hover(false)) {
                return stop(result.status);
            }
        }
        return return_zero();
    }
    case 2U: {
        state.hovered_equipment = 0xFFFFFFFFU;
        if (strict_inside(mouse_y, 0x82U, 0xA0U)) {
            ecx = 0U;
            eax = 0x10AU;
            while (!strict_inside(mouse_x, eax - 0x2AU, eax)) {
                eax += 0x2AU;
                ++ecx;
                if (eax >= 0x188U) {
                    break;
                }
            }
            if (eax < 0x188U) {
                state.hovered_equipment = ecx;
                bindings.final_actor.pre_frame_gate_b = 1U;
            }
        }
        const u32 limit = unsigned_bits(
            static_cast<i32>(static_cast<i8>(state.panel_row_limit_a))
        );
        if (select_row(
                0xBEU,
                0x14U,
                0x14AU,
                0xE0U,
                0x180U,
                state.list_selection,
                true,
                limit,
                state.panel_scroll_a,
                true
            )) {
            input_state.interaction_mode = 0U;
            set_common_success();
            return return_one();
        }
        if (strict_inside(mouse_x, 0x192U, 0x1A4U)) {
            if (strict_inside(mouse_y, 0x9EU, 0xAEU)) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 1U;
                return return_one();
            }
            if (strict_inside(mouse_y, 0x12AU, 0x13AU)) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 2U;
                return return_one();
            }
            if (strict_inside(
                    mouse_y, state.lower_panel_top, state.lower_panel_bottom
                )) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 3U;
                return return_one();
            }
            if (mouse_x < 0x1A4U &&
                strict_inside(
                    mouse_y, state.final_panel_top, state.final_panel_bottom
                )) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 4U;
                return return_one();
            }
        }
        input_state.interaction_mode = 0U;
        input_state.menu_action = 0U;
        input_state.mouse_action_gate = 0U;
        return return_zero();
    }
    case 3U:
        break;
    case 4U: {
        state.hovered_secondary = 0xFFFFFFFFU;
        if (strict_inside(mouse_y, 0x82U, 0xA0U)) {
            ecx = 0U;
            eax = 0x10AU;
            while (!strict_inside(mouse_x, eax - 0x2AU, eax)) {
                eax += 0x2AU;
                ++ecx;
                if (eax >= 0x1B2U) {
                    break;
                }
            }
            if (eax < 0x1B2U) {
                if (state.current_equipment_selection != ecx) {
                    state.hovered_secondary = ecx;
                }
                bindings.final_actor.pre_frame_gate_b = 1U;
            }
        }
        if (select_row(
                0xBEU,
                0x14U,
                0x14AU,
                0xE0U,
                0x19CU,
                state.grid_selection,
                true,
                static_cast<u32>(state.panel_row_limit_c),
                state.panel_scroll_b,
                true
            )) {
            input_state.interaction_mode = 0U;
            set_common_success();
            return return_one();
        }
        input_state.mouse_action_gate = 0U;
        if (signed_bits(bindings.input.records[15U].held_sample_count) >= 1) {
            return return_zero();
        }
        input_state.menu_action = 0U;
        state.transient_selection_a = 0U;
        state.transient_selection_b = 0U;
        state.transient_selection_c = 0U;
        if (state.panel_row_limit_c <= 7U) {
            return return_zero();
        }
        if (strict_inside(mouse_x, 0x19EU, 0x1B0U)) {
            if (strict_inside(mouse_y, 0x9EU, 0xAEU)) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 1U;
                return return_one();
            }
            if (strict_inside(mouse_y, 0x12EU, 0x13EU)) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 2U;
                return return_one();
            }
            if (strict_inside(
                    mouse_y, state.lower_panel_top, state.lower_panel_bottom
                )) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 3U;
                return return_one();
            }
            if (mouse_x < 0x1B0U &&
                strict_inside(
                    mouse_y, state.final_panel_top, state.final_panel_bottom
                )) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 4U;
                return return_one();
            }
        }
        input_state.interaction_mode = 0U;
        input_state.menu_action = 0U;
        input_state.mouse_action_gate = 0U;
        return return_zero();
    }
    case 5U: {
        if (select_row(
                0xECU,
                0x16U,
                0x118U,
                0xC4U,
                0x178U,
                state.group_b_row_selection,
                true,
                0x7FFFFFFFU,
                0U,
                true
            )) {
            set_common_success();
            return return_one();
        }
        input_state.mouse_action_gate = 0U;
        return return_zero();
    }
    case 8U: {
        if (select_row(
                0xBAU,
                0x18U,
                0x162U,
                0xE0U,
                0x199U,
                state.narrow_list_selection,
                true,
                unsigned_bits(
                    static_cast<i32>(static_cast<i8>(state.panel_row_limit_b))
                ),
                0U,
                true
            )) {
            set_common_success();
            return return_one();
        }
        input_state.mouse_action_gate = 0U;
        return return_zero();
    }
    case 27U: {
        if (select_row(
                0xBEU,
                0x14U,
                0x14AU,
                0xE0U,
                0x194U,
                state.grid_selection,
                true,
                static_cast<u32>(state.panel_row_limit_c),
                state.panel_scroll_b,
                true
            )) {
            input_state.interaction_mode = 0U;
            set_common_success();
            return return_one();
        }
        input_state.mouse_action_gate = 0U;
        if (signed_bits(bindings.input.records[15U].held_sample_count) >= 1) {
            return return_zero();
        }
        input_state.menu_action = 0U;
        state.transient_selection_a = 0U;
        state.transient_selection_b = 0U;
        state.transient_selection_c = 0U;
        if (state.panel_row_limit_c <= 7U) {
            return return_zero();
        }
        if (strict_inside(mouse_x, 0x192U, 0x1A4U)) {
            if (strict_inside(mouse_y, 0x9EU, 0xAEU)) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 1U;
                return return_one();
            }
            if (strict_inside(mouse_y, 0x12EU, 0x140U)) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 2U;
                return return_one();
            }
            if (strict_inside(
                    mouse_y, state.lower_panel_top, state.lower_panel_bottom
                )) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 3U;
                return return_one();
            }
            if (mouse_x < 0x1A4U &&
                strict_inside(
                    mouse_y, state.final_panel_top, state.final_panel_bottom
                )) {
                input_state.mouse_action_gate = 1U;
                input_state.interaction_mode = 4U;
                return return_one();
            }
        }
        input_state.interaction_mode = 0U;
        input_state.menu_action = 0U;
        input_state.mouse_action_gate = 0U;
        return return_zero();
    }
    case 30U: {
        u32 column = 0U;
        edx = 0x15AU;
        while (true) {
            if (strict_inside(mouse_x, 0xEAU, edx)) {
                ecx = 0U;
                eax = 0xBAU;
                while (!strict_inside(mouse_y, eax - 0x14U, eax)) {
                    eax += 0x14U;
                    ++ecx;
                    if (eax >= 0x11EU) {
                        break;
                    }
                }
                if (eax < 0x11EU) {
                    u32 selected = column + 4U * column + ecx + 1U;
                    eax = state.grid_selection;
                    if (eax != selected) {
                        play_selection_sample();
                    }
                    state.grid_selection = selected;
                    if (selected > 10U) {
                        selected = 10U;
                        state.grid_selection = selected;
                    }
                    set_common_success();
                    return return_one();
                }
            }
            edx += 0x70U;
            ++column;
            if (edx >= 0x23AU) {
                input_state.mouse_action_gate = 0U;
                input_state.selected_option_word = 0xFFFFU;
                return return_zero();
            }
        }
    }
    default:
        return return_zero();
    }

    if (state.target_selection_block == 1U ||
        state.target_selection_suppression == 1U ||
        state.selection_block_word != 0U) {
        return return_zero();
    }
    const u32 active_actor = bindings.final_actor.active_actor_code;
    const u32 startup_mode_index = active_actor * 5U - 40U;
    if (startup_mode_index >= bindings.startup.reset.block_520e90.size()) {
        return stop(
            LegacyBattleFrameInputResolutionStatus::startup_mode_typed_stop
        );
    }

    const auto publish_target = [&](const u32 actor_code,
                                    const u32 actor_index,
                                    const u32 actor_token) {
        bindings.final_actor.published_actor_code = actor_code;
        state.selected_target_index = actor_index;
        static_cast<void>(call(
            LegacyBattleFrameInputResolutionCall::configure_actor_selection,
            actor_token,
            {1U}
        ));
        input_state.mouse_action_gate = 1U;
        bindings.final_actor.pre_frame_gate_b = 1U;
        eax = 1U;
    };

    if (bindings.startup.reset.block_520e90[startup_mode_index] == 0U) {
        i32 index = 0;
        while (index < signed_bits(bindings.metrics.group_b_count)) {
            static_cast<void>(call(
                LegacyBattleFrameInputResolutionCall::configure_actor_selection,
                group_b_token(static_cast<u32>(index)),
                {0U}
            ));
            ++index;
            ++result.actor_iterations;
        }
        i32 candidate = signed_bits(bindings.metrics.group_b_count - 1U);
        while (candidate >= 0) {
            const u32 actor_index = static_cast<u32>(candidate);
            const u32 actor_token = group_b_token(actor_index);
            const auto blocked = call(
                LegacyBattleFrameInputResolutionCall::query_group_b_candidate,
                actor_token
            );
            if (blocked.eax != 1U) {
                bool typed_stop = false;
                if (actor_surface_hit(
                        actor_token, 2U, true, true, typed_stop
                    )) {
                    publish_target(actor_index + 1U, actor_index, actor_token);
                    if (input_state.selection_index == 6U) {
                        const auto mode = call(
                            LegacyBattleFrameInputResolutionCall::
                                query_group_b_mode,
                            actor_token
                        );
                        if (mode.eax == 0U) {
                            state.target_action_available = 0U;
                        }
                    }
                    return return_one();
                }
                if (typed_stop) {
                    return stop(result.status);
                }
            }
            --candidate;
            ++result.actor_iterations;
        }
        return return_zero();
    }

    i32 reset_index = 0;
    while (reset_index < signed_bits(bindings.metrics.group_a_count)) {
        static_cast<void>(call(
            LegacyBattleFrameInputResolutionCall::configure_actor_selection,
            group_a_token(static_cast<u32>(reset_index)),
            {0U}
        ));
        ++reset_index;
        ++result.actor_iterations;
    }
    const u32 selected_actor_index = state.selected_target_index;
    static_cast<void>(call(
        LegacyBattleFrameInputResolutionCall::configure_actor_selection,
        group_a_token(selected_actor_index),
        {0U}
    ));

    u32 remaining = bindings.metrics.group_a_count;
    remaining -= bindings.startup.final_subtract_word;
    remaining -= bindings.startup.supplemental_count_word;
    if (remaining >= 4U) {
        u32 order_index = remaining - 1U;
        while (signed_bits(order_index) >= 0) {
            if (order_index >= bindings.final_actor.actor_order.size()) {
                return stop(
                    LegacyBattleFrameInputResolutionStatus::
                        actor_order_typed_stop
                );
            }
            const u32 actor_index =
                bindings.final_actor.actor_order[order_index];
            if (actor_index >=
                    bindings.final_actor.group_a_completion_flags.size() ||
                actor_index >=
                    bindings.final_actor.group_a_slot_values.size()) {
                return stop(
                    LegacyBattleFrameInputResolutionStatus::
                        group_a_actor_typed_stop
                );
            }
            const u32 actor_token = group_a_token(actor_index);
            if (bindings.final_actor.group_a_completion_flags[actor_index] !=
                    1U &&
                bindings.final_actor.group_a_slot_values[actor_index] != 1U) {
                const auto blocked = call(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_a_candidate,
                    actor_token
                );
                if (blocked.eax != 1U) {
                    bool typed_stop = false;
                    const bool allowed = active_actor - 8U != actor_index ||
                        input_state.selection_index == 2U ||
                        input_state.selection_index == 3U;
                    if (actor_surface_hit(
                            actor_token, 1U, false, allowed, typed_stop
                        )) {
                        publish_target(
                            actor_index + 1U, actor_index, actor_token
                        );
                        return return_one();
                    }
                    if (typed_stop) {
                        return stop(result.status);
                    }
                    if (state.selected_target_index >=
                        state.target_markers.size()) {
                        return stop(
                            LegacyBattleFrameInputResolutionStatus::
                                target_marker_typed_stop
                        );
                    }
                    state.target_markers[state.selected_target_index] = 0U;
                    if (mouse_y >= 0x16AU && mouse_y <= 0x1B6U) {
                        if (actor_index >=
                            bindings.startup.party_offsets.size()) {
                            return stop(
                                LegacyBattleFrameInputResolutionStatus::
                                    party_offset_typed_stop
                            );
                        }
                        const u32 left = unsigned_bits(
                            bindings.startup.party_offsets[actor_index]
                        );
                        if (mouse_x >= left && mouse_x <= left + 0x7CU) {
                            if (actor_index >= state.target_markers.size()) {
                                return stop(
                                    LegacyBattleFrameInputResolutionStatus::
                                        target_marker_typed_stop
                                );
                            }
                            state.target_markers[actor_index] = 1U;
                            publish_target(
                                actor_index + 1U, actor_index, actor_token
                            );
                            return return_one();
                        }
                    }
                    input_state.mouse_action_gate = 0U;
                }
            }
            --order_index;
            ++result.actor_iterations;
        }
        return return_zero();
    }

    i32 candidate = signed_bits(bindings.metrics.group_a_count - 1U);
    while (candidate >= 0) {
        const u32 actor_index = static_cast<u32>(candidate);
        if (actor_index >=
                bindings.final_actor.group_a_completion_flags.size() ||
            actor_index >= bindings.final_actor.group_a_slot_values.size()) {
            return stop(
                LegacyBattleFrameInputResolutionStatus::group_a_actor_typed_stop
            );
        }
        const u32 actor_token = group_a_token(actor_index);
        if (bindings.final_actor.group_a_completion_flags[actor_index] != 1U &&
            bindings.final_actor.group_a_slot_values[actor_index] != 1U) {
            const auto blocked = call(
                LegacyBattleFrameInputResolutionCall::query_group_a_candidate,
                actor_token
            );
            if (blocked.eax != 1U) {
                bool typed_stop = false;
                const bool allowed = active_actor - 8U != actor_index ||
                    input_state.selection_index == 2U ||
                    input_state.selection_index == 3U;
                if (actor_surface_hit(
                        actor_token, 1U, false, allowed, typed_stop
                    )) {
                    publish_target(actor_index + 1U, actor_index, actor_token);
                    for (std::size_t index = 0U; index < 4U; ++index) {
                        state.target_markers[index] = 0U;
                    }
                    return return_one();
                }
                if (typed_stop) {
                    return stop(result.status);
                }
            }
        }
        --candidate;
        ++result.actor_iterations;
    }

    if (state.selected_target_index >= state.target_markers.size()) {
        return stop(
            LegacyBattleFrameInputResolutionStatus::target_marker_typed_stop
        );
    }
    state.target_markers[state.selected_target_index] = 0U;
    if (mouse_y >= 0x16AU && mouse_y <= 0x1B6U) {
        u32 party_index = 0U;
        while (signed_bits(party_index) <
               signed_bits(bindings.metrics.group_a_count)) {
            if (party_index >= bindings.startup.party_source_indices.size()) {
                return stop(
                    LegacyBattleFrameInputResolutionStatus::
                        party_source_index_typed_stop
                );
            }
            const u32 source =
                bindings.startup.party_source_indices[party_index];
            if (party_index >=
                    bindings.final_actor.group_a_completion_flags.size() ||
                party_index >=
                    bindings.final_actor.group_a_slot_values.size()) {
                return stop(
                    LegacyBattleFrameInputResolutionStatus::
                        group_a_actor_typed_stop
                );
            }
            const u32 actor_token = group_a_token(party_index);
            if (bindings.final_actor.group_a_completion_flags[party_index] !=
                    1U &&
                bindings.final_actor.group_a_slot_values[party_index] != 1U) {
                const auto blocked = call(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_a_candidate,
                    actor_token
                );
                if (blocked.eax != 1U) {
                    if (source >= bindings.startup.party_offsets.size()) {
                        return stop(
                            LegacyBattleFrameInputResolutionStatus::
                                party_offset_typed_stop
                        );
                    }
                    const u32 left =
                        unsigned_bits(bindings.startup.party_offsets[source]);
                    if (mouse_x >= left && mouse_x <= left + 0x7CU) {
                        if (source >= state.target_markers.size()) {
                            return stop(
                                LegacyBattleFrameInputResolutionStatus::
                                    target_marker_typed_stop
                            );
                        }
                        state.selected_target_index = source;
                        state.target_markers[source] = 1U;
                        bindings.final_actor.published_actor_code =
                            party_index + 1U;
                        input_state.mouse_action_gate = 1U;
                        bindings.final_actor.pre_frame_gate_b = 1U;
                        return return_one();
                    }
                }
            }
            ++party_index;
            ++result.actor_iterations;
        }
    }
    input_state.mouse_action_gate = 0U;
    return return_zero();
}

}  // namespace openswd3::battle
