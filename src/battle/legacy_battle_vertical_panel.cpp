#include "openswd3/battle/legacy_battle_vertical_panel.hpp"

#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u32 to_bits(const compat::i32 value) noexcept {
    return std::bit_cast<compat::u32>(value);
}

[[nodiscard]] compat::i32 from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] compat::i32
wrapping_multiply(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

void publish_blitter_normal_epilogue(
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects
) noexcept {
    shared_request.target_height = 0;
    shared_request.horizontal_resample_displacement = 0;
    shared_request.vertical_resample_phase_10_10 = 0U;
    shared_request.opacity_step = 0;
    shared_effects.red_offset = 0;
    shared_effects.green_offset = 0;
    shared_effects.blue_offset = 0;
    shared_effects.skip_every_third_row = false;
}

void set_panel_clip(
    LegacyBattleVerticalPanelState& state,
    compat::i32 left,
    compat::i32 top,
    compat::i32 right,
    compat::i32 bottom
) noexcept {
    if (left < 0) {
        left = 0;
    }
    if (top < 0) {
        top = 0;
    }
    if (right > state.screen_width) {
        right = state.screen_width;
    }
    if (bottom > state.screen_height) {
        bottom = state.screen_height;
    }
    state.shared_clip = {
        .left = left,
        .top = top,
        .width = wrapping_subtract(right, left),
        .height = wrapping_subtract(bottom, top),
    };
}

[[nodiscard]] bool fill_loop_never_reaches_threshold(
    const compat::i32 scaled_fill_height, const compat::i32 fill_frame_height
) noexcept {
    if (fill_frame_height == 0) {
        return true;
    }
    const compat::u32 step = static_cast<compat::u32>(fill_frame_height);
    const compat::u32 modular_gcd = step & (0U - step);
    const compat::u32 signed_max =
        static_cast<compat::u32>(std::numeric_limits<compat::i32>::max());
    const compat::u32 largest_reachable_positive =
        signed_max - signed_max % modular_gcd;
    return static_cast<compat::u32>(scaled_fill_height) >
        largest_reachable_positive;
}

[[nodiscard]] compat::i32 legacy_scaled_fill_height(
    const compat::i32 maximum_count, const compat::i32 repeated_middle_height
) noexcept {
    if (maximum_count == 0) {
        return 0;
    }
    const volatile long double ratio =
        7.0L / static_cast<long double>(maximum_count);
    const volatile long double product =
        ratio * static_cast<long double>(repeated_middle_height);
    const long double value = product;
    if (!std::isfinite(value) ||
        value > static_cast<long double>(
                    std::numeric_limits<std::int64_t>::max()
                ) ||
        value < static_cast<long double>(
                    std::numeric_limits<std::int64_t>::min()
                )) {
        return 0;
    }
    const std::int64_t converted = static_cast<std::int64_t>(value);
    return from_bits(static_cast<compat::u32>(converted));
}

[[nodiscard]] bool load_panel_phase(
    LegacyBattleVerticalPanelState& state,
    LegacyBattleVerticalPanelResult& result,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 phase,
    const compat::u32 action_id,
    const compat::u32 base_variant,
    const bool clear_record,
    const LegacyBattleActionUpdateRegisterSnapshot& registers,
    const bool resource_from_eax_frame_from_edx
) {
    if (clear_record) {
        std::memset(
            &state.action_record, 0, asset_runtime::kLegacyActionRecordSize
        );
    }
    state.action_record.action_id = action_id;
    state.action_record.base_variant = base_variant;
    state.base_variants[phase] = base_variant;
    state.action_update_attempted[phase] = true;
    state.action_updates[phase] = action_updater.update(state.action_record);
    ++result.action_update_calls;
    if (state.action_updates[phase].return_value == 0U) {
        result.status = LegacyBattleVerticalPanelStatus::action_update_failed;
        result.stopped_phase = phase;
        return false;
    }

    const compat::u32 resource_high =
        resource_from_eax_frame_from_edx ? registers.eax : registers.ecx;
    const compat::u32 frame_high =
        resource_from_eax_frame_from_edx ? registers.edx : registers.eax;
    const compat::u32 resource_id = (resource_high & 0xFFFF0000U) |
        static_cast<compat::u32>(state.action_record.field_4a);
    const compat::u32 frame_index = (frame_high & 0xFFFF0000U) |
        static_cast<compat::u32>(state.action_record.field_4c);
    state.frame_resource_ids[phase] = resource_id;
    state.frame_indices[phase] = frame_index;

    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, frame_index, piece);
    ++result.frame_load_calls;
    state.frame_available[phase] = available;
    if (!available) {
        state.frames[phase] = {};
        result.status = LegacyBattleVerticalPanelStatus::frame_unavailable;
        result.stopped_phase = phase;
        return false;
    }

    state.frames[phase] = piece;
    state.current_source = piece.source;
    state.source_published = true;
    return true;
}

[[nodiscard]] bool draw_panel_phase(
    LegacyBattleVerticalPanelState& state,
    LegacyBattleVerticalPanelResult& result,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    const compat::u32 phase,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(state.frames[phase].width);
    request.source_height =
        static_cast<compat::i32>(state.frames[phase].height);
    request.flags = state.action_record.mode_flags;
    request.auxiliary = {};
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer,
        state.shared_clip,
        call_source,
        request,
        shared_effects,
        jitter
    );
    ++result.frame_draw_calls;
    ++result.phase_draw_calls[phase];
    result.last_blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleVerticalPanelStatus::blit_typed_stop;
        result.stopped_phase = phase;
        return false;
    }
    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return true;
}

}  // namespace

LegacyBattleVerticalPanelResult draw_legacy_battle_vertical_panel(
    LegacyBattleVerticalPanelState& state,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 action_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 middle_count,
    const compat::i32 fill_offset,
    const compat::u32 selector,
    const std::array<LegacyBattleActionUpdateRegisterSnapshot, 4>&
        update_registers,
    const compat::u32 final_blit_eax_snapshot
) {
    LegacyBattleVerticalPanelResult result;
    const compat::u32 selected_action_id =
        static_cast<compat::u32>(static_cast<compat::u16>(action_id));

    const compat::u32 top_variant = selector == 1U ? 0x1EU : 0x1AU;
    if (!load_panel_phase(
            state,
            result,
            action_updater,
            frame_provider,
            0U,
            selected_action_id,
            top_variant,
            false,
            update_registers[0],
            false
        )) {
        return result;
    }
    if (!draw_panel_phase(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            0U,
            x,
            y
        )) {
        return result;
    }

    const compat::i32 top_height =
        static_cast<compat::i32>(state.frames[0].height);
    const compat::i32 panel_content_top = wrapping_add(y, top_height);
    compat::i32 accumulated_height = top_height;

    if (!load_panel_phase(
            state,
            result,
            action_updater,
            frame_provider,
            1U,
            selected_action_id,
            0x18U,
            true,
            update_registers[1],
            true
        )) {
        return result;
    }
    const compat::i32 middle_height =
        static_cast<compat::i32>(state.frames[1].height);
    compat::i32 middle_index = 0;
    while (middle_index < middle_count) {
        const compat::i32 draw_y = wrapping_add(
            wrapping_add(
                wrapping_multiply(middle_height, middle_index), top_height
            ),
            y
        );
        if (!draw_panel_phase(
                state,
                result,
                framebuffer,
                shared_request,
                shared_effects,
                jitter,
                1U,
                wrapping_add(x, 2),
                draw_y
            )) {
            return result;
        }
        middle_index = wrapping_add(middle_index, 1);
    }
    result.middle_draw_count = static_cast<compat::u32>(middle_index);
    result.repeated_middle_height =
        wrapping_multiply(middle_height, middle_count);
    accumulated_height = wrapping_add(
        accumulated_height, wrapping_multiply(middle_height, middle_index)
    );

    if (!load_panel_phase(
            state,
            result,
            action_updater,
            frame_provider,
            2U,
            selected_action_id,
            0x19U,
            true,
            update_registers[2],
            false
        )) {
        return result;
    }

    const compat::i32 scaled_fill_height = legacy_scaled_fill_height(
        state.maximum_count, result.repeated_middle_height
    );
    result.scaled_fill_height = scaled_fill_height;
    const compat::i32 divisor = wrapping_add(state.maximum_count, -7);
    if (divisor == 0) {
        result.status = LegacyBattleVerticalPanelStatus::ratio_divide_by_zero;
        result.stopped_phase = 2U;
        return result;
    }
    const compat::i32 dividend =
        wrapping_subtract(result.repeated_middle_height, scaled_fill_height);
    if (dividend == std::numeric_limits<compat::i32>::min() && divisor == -1) {
        result.status = LegacyBattleVerticalPanelStatus::ratio_divide_overflow;
        result.stopped_phase = 2U;
        return result;
    }
    const compat::i32 quotient = dividend / divisor;
    state.ratio_quotient = quotient;
    result.ratio_quotient = quotient;

    const compat::i32 current_displacement =
        wrapping_multiply(state.current_count, quotient);
    const compat::i32 fill_start = wrapping_add(
        wrapping_add(current_displacement, panel_content_top), fill_offset
    );
    compat::i32 fill_clip_bottom = wrapping_add(
        wrapping_add(
            wrapping_add(current_displacement, scaled_fill_height),
            panel_content_top
        ),
        fill_offset
    );
    if (wrapping_subtract(fill_clip_bottom, fill_start) < 5) {
        fill_clip_bottom = wrapping_add(fill_start, 5);
    }
    if (wrapping_add(state.current_count, 7) >= state.maximum_count) {
        fill_clip_bottom =
            wrapping_add(panel_content_top, result.repeated_middle_height);
    }

    state.panel_content_top = panel_content_top;
    state.fill_start = fill_start;
    state.fill_clip_bottom = fill_clip_bottom;
    state.panel_content_bottom =
        wrapping_add(panel_content_top, result.repeated_middle_height);
    set_panel_clip(state, x, y, wrapping_add(x, 32), fill_clip_bottom);
    ++result.clip_set_calls;

    if (scaled_fill_height > 0) {
        const compat::i32 fill_frame_height =
            static_cast<compat::i32>(state.frames[2].height);
        const bool nonterminating_fill = fill_loop_never_reaches_threshold(
            scaled_fill_height, fill_frame_height
        );
        compat::i32 fill_progress = 0;
        do {
            if (!draw_panel_phase(
                    state,
                    result,
                    framebuffer,
                    shared_request,
                    shared_effects,
                    jitter,
                    2U,
                    wrapping_add(x, 5),
                    wrapping_add(fill_start, fill_progress)
                )) {
                return result;
            }
            ++result.fill_draw_count;
            if (nonterminating_fill) {
                result.status =
                    LegacyBattleVerticalPanelStatus::fill_loop_nonterminating;
                result.stopped_phase = 2U;
                return result;
            }
            fill_progress = wrapping_add(fill_progress, fill_frame_height);
        } while (fill_progress < scaled_fill_height);
    }

    set_panel_clip(state, 0, 0, 640, 480);
    ++result.clip_set_calls;

    const compat::u32 bottom_variant = selector == 2U ? 0x1FU : 0x1BU;
    if (!load_panel_phase(
            state,
            result,
            action_updater,
            frame_provider,
            3U,
            selected_action_id,
            bottom_variant,
            true,
            update_registers[3],
            false
        )) {
        return result;
    }
    result.bottom_draw_y = wrapping_add(y, accumulated_height);
    if (!draw_panel_phase(
            state,
            result,
            framebuffer,
            shared_request,
            shared_effects,
            jitter,
            3U,
            x,
            result.bottom_draw_y
        )) {
        return result;
    }
    result.return_value = final_blit_eax_snapshot;
    return result;
}

}  // namespace openswd3::battle
