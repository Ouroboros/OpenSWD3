#include "openswd3/battle/legacy_battle_frame_effect.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(left) + static_cast<u32>(right));
}

[[nodiscard]] constexpr i32
wrapping_multiply(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(left) * static_cast<u32>(right));
}

[[nodiscard]] constexpr i32 wrapping_negate(const i32 value) noexcept {
    return std::bit_cast<i32>(0U - static_cast<u32>(value));
}

[[nodiscard]] constexpr i32
arithmetic_shift_right_one(const i32 value) noexcept {
    const std::int64_t wide = value;
    return static_cast<i32>(wide >= 0 ? wide / 2 : -(((-wide) + 1) / 2));
}

[[nodiscard]] bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] bool accepted_rotation_status(
    const LegacyBattleImageRotationStatus status
) noexcept {
    return status == LegacyBattleImageRotationStatus::completed ||
        status == LegacyBattleImageRotationStatus::shift_not_positive ||
        status == LegacyBattleImageRotationStatus::magic_mismatch ||
        status ==
        LegacyBattleImageRotationStatus::first_row_flags_unsupported ||
        status == LegacyBattleImageRotationStatus::mode_out_of_range;
}

[[nodiscard]] bool accepted_rotation_draw_status(
    const LegacyBattleActionRotationDrawStatus status
) noexcept {
    return status == LegacyBattleActionRotationDrawStatus::completed;
}

[[nodiscard]] bool accepted_rotation_playback_status(
    const LegacyBattleActionRotationPlaybackStatus status
) noexcept {
    return status == LegacyBattleActionRotationPlaybackStatus::completed ||
        status ==
        LegacyBattleActionRotationPlaybackStatus::
            initial_action_update_stopped ||
        status ==
        LegacyBattleActionRotationPlaybackStatus::action_update_stopped;
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

void set_clip(
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectResult& result,
    const i32 left,
    const i32 top,
    const i32 right,
    const i32 bottom
) noexcept {
    rendering::set_legacy_clip_rectangle(
        context.raster, left, top, right, bottom
    );
    ++result.clip_calls;
}

[[nodiscard]] rendering::LegacyBlitClipRectangle
current_clip(const rendering::LegacyRasterGeometryState& raster) noexcept {
    return {
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

[[nodiscard]] bool draw_source(
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectResult& result,
    const LegacyBattleFrameEffectSource source,
    const u32 flags
) noexcept {
    rendering::LegacyBlitRequest request = context.shared_request;
    request.destination_x = 0;
    request.destination_y = 0;
    request.source_width = static_cast<i32>(source.width);
    request.source_height = static_cast<i32>(source.height);
    request.flags = flags;
    request.auxiliary = {};
    const rendering::LegacyBlitSource blit_source{
        .bytes = source.bytes,
        .layout = source.layout,
        .palette = {},
    };
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        context.framebuffer,
        current_clip(context.raster),
        blit_source,
        request,
        context.shared_effects,
        context.jitter
    );
    ++result.source_blit_calls;
    if (!accepted_blit_status(blit.status)) {
        return false;
    }
    publish_blitter_normal_epilogue(
        context.shared_request, context.shared_effects
    );
    return true;
}

[[nodiscard]] bool rotate_source(
    LegacyBattleFrameEffectResult& result,
    const LegacyBattleFrameEffectSource source,
    const i32 rotation_amount
) noexcept {
    const LegacyBattleImageRotationMode mode = rotation_amount > 0
        ? LegacyBattleImageRotationMode::pixels_right
        : LegacyBattleImageRotationMode::pixels_left;
    const i32 shift = rotation_amount > 0 ? rotation_amount
                                          : wrapping_negate(rotation_amount);
    result.source_rotation =
        rotate_legacy_battle_literal_image(source.bytes, mode, shift);
    ++result.source_rotation_calls;
    return accepted_rotation_status(result.source_rotation.status);
}

[[nodiscard]] bool draw_rotation_frame(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectPort& port,
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectResult& result
) noexcept {
    result.rotation_frame = draw_legacy_battle_action_rotation_frame(
        state.rotation_cache,
        port,
        context.framebuffer,
        current_clip(context.raster),
        context.shared_request,
        context.shared_effects,
        context.jitter
    );
    ++result.rotation_frame_calls;
    return accepted_rotation_draw_status(result.rotation_frame.status);
}

[[nodiscard]] bool play_rotation_frames(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectPort& port,
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectResult& result,
    const i32 rotation_amount
) noexcept {
    result.rotation_playback = play_legacy_battle_action_rotation_cache(
        state.rotation_cache,
        port,
        context.framebuffer,
        current_clip(context.raster),
        context.shared_request,
        context.shared_effects,
        context.jitter,
        0x0053B0B8U,
        rotation_amount
    );
    ++result.rotation_playback_calls;
    return accepted_rotation_playback_status(result.rotation_playback.status);
}

[[nodiscard]] bool adjust_color_cycle(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectResult& result
) noexcept {
    const i32 delta =
        static_cast<i32>(std::bit_cast<compat::i8>(state.color_cycle_delta));
    state.published_red_delta = delta;
    state.published_green_delta = delta;
    state.published_blue_delta = delta;
    result.applied_red_delta = delta;
    result.applied_green_delta = delta;
    result.applied_blue_delta = delta;

    auto pixels = context.framebuffer.physical_pixels_with_read_guard();
    result.color_status = rendering::adjust_legacy_red_channel(
        pixels,
        kLegacyBattleFrameEffectPixelCount,
        delta,
        context.shared_effects.pixel_conversion
    );
    ++result.color_adjustment_calls;
    if (result.color_status != rendering::LegacyFrameColorStatus::completed) {
        return false;
    }
    result.color_status = rendering::adjust_legacy_green_channel_pairs(
        pixels,
        kLegacyBattleFrameEffectPixelCount,
        delta,
        context.shared_effects.pixel_conversion
    );
    ++result.color_adjustment_calls;
    if (result.color_status != rendering::LegacyFrameColorStatus::completed) {
        return false;
    }
    result.color_status = rendering::adjust_legacy_blue_channel_pairs(
        pixels,
        kLegacyBattleFrameEffectPixelCount,
        delta,
        context.shared_effects.pixel_conversion
    );
    ++result.color_adjustment_calls;
    if (result.color_status != rendering::LegacyFrameColorStatus::completed) {
        return false;
    }

    state.color_cycle_delta = static_cast<compat::u8>(
        static_cast<u32>(state.color_cycle_delta) + 0xFCU
    );
    if (state.color_cycle_delta == 0U) {
        state.color_cycle_delta = 0x10U;
        state.color_cycle_active = 0U;
    }
    return true;
}

[[nodiscard]] bool adjust_stage_colors(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectContext& context,
    LegacyBattleFrameEffectResult& result
) noexcept {
    const i32 stage = state.stage;
    const i32 red =
        wrapping_multiply(arithmetic_shift_right_one(state.red_factor), stage);
    const i32 green = wrapping_multiply(
        arithmetic_shift_right_one(state.green_factor), stage
    );
    const i32 blue =
        wrapping_multiply(arithmetic_shift_right_one(state.blue_factor), stage);
    result.applied_red_delta = red;
    result.applied_green_delta = green;
    result.applied_blue_delta = blue;
    auto pixels = context.framebuffer.physical_pixels_with_read_guard();
    result.color_status = rendering::adjust_legacy_rgb_channels(
        pixels,
        kLegacyBattleFrameEffectPixelCount,
        red,
        green,
        blue,
        context.shared_effects.pixel_conversion
    );
    ++result.color_adjustment_calls;
    return result.color_status == rendering::LegacyFrameColorStatus::completed;
}

[[nodiscard]] bool invoke_staged_surface(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectPort& port,
    LegacyBattleFrameEffectResult& result,
    const std::span<const u32> staged_surface_tokens,
    const i16 index,
    const u32 effect_flags
) {
    if (index < 0 ||
        static_cast<std::size_t>(index) >= staged_surface_tokens.size()) {
        return false;
    }
    static_cast<void>(port.surface_operation({
        .object_token = state.surface_object_token,
        .source_token = staged_surface_tokens[static_cast<std::size_t>(index)],
        .effect_flags = effect_flags,
    }));
    ++result.surface_operation_calls;
    return true;
}

void reset_effect_state(
    LegacyBattleFrameEffectState& state, LegacyBattleFrameEffectResult& result
) noexcept {
    state.red_factor = 0;
    state.green_factor = 0;
    state.blue_factor = 0;
    state.stage = 0;
    state.current_encounter_id = -1;
    state.secondary_suppression = 0U;
    state.primary_suppression = 0U;
    state.alternate_surface_mode = 0U;
    state.fade_active = 0U;
    ++result.reset_calls;
}

}  // namespace

LegacyBattleFrameEffectResult update_legacy_battle_frame_effect(
    LegacyBattleFrameEffectState& state,
    LegacyBattleFrameEffectPort& port,
    LegacyBattleFrameEffectContext& context,
    const LegacyBattleFrameEffectSource source,
    const std::span<const u32> staged_surface_tokens,
    const i32 rotation_amount
) noexcept {
    LegacyBattleFrameEffectResult result;
    state.published_source_token = source.token;
    set_clip(context, result, 0, 0, 640, 480);

    if (state.primary_suppression == 0U && state.secondary_suppression == 0U) {
        if (rotation_amount == 0) {
            if (!draw_source(context, result, source, 0U)) {
                result.status =
                    LegacyBattleFrameEffectStatus::source_blit_typed_stop;
                return result;
            }
            if (!draw_rotation_frame(state, port, context, result)) {
                result.status =
                    LegacyBattleFrameEffectStatus::rotation_frame_typed_stop;
                return result;
            }
            if (state.split_suppression != 1U) {
                u16 extent = state.split_extent;
                if (extent < 0xC0U) {
                    extent = extent < 0x14U ? static_cast<u16>(extent << 1U)
                                            : static_cast<u16>(extent + 0x16U);
                    state.split_extent = extent;
                }
                set_clip(
                    context,
                    result,
                    0,
                    std::bit_cast<i32>(0xC0U - static_cast<u32>(extent)),
                    640,
                    192
                );
                if (!draw_source(context, result, source, 0x28U)) {
                    result.status =
                        LegacyBattleFrameEffectStatus::source_blit_typed_stop;
                    return result;
                }
                set_clip(
                    context,
                    result,
                    0,
                    192,
                    640,
                    std::bit_cast<i32>(0xC0U + static_cast<u32>(extent))
                );
                if (!draw_source(context, result, source, 0x28U)) {
                    result.status =
                        LegacyBattleFrameEffectStatus::source_blit_typed_stop;
                    return result;
                }
            }
        } else {
            if (!rotate_source(result, source, rotation_amount)) {
                result.status =
                    LegacyBattleFrameEffectStatus::source_rotation_typed_stop;
                return result;
            }
            if (!draw_source(context, result, source, 0U)) {
                result.status =
                    LegacyBattleFrameEffectStatus::source_blit_typed_stop;
                return result;
            }
            if (!play_rotation_frames(
                    state, port, context, result, rotation_amount
                )) {
                result.status =
                    LegacyBattleFrameEffectStatus::rotation_playback_typed_stop;
                return result;
            }
        }

        state.pending_rotation = 0;
        if (state.color_cycle_active == 1U &&
            !adjust_color_cycle(state, context, result)) {
            result.status =
                LegacyBattleFrameEffectStatus::color_adjustment_typed_stop;
            return result;
        }
    }

    set_clip(context, result, 0, 0, 640, 480);
    i16 stage = state.stage;
    if (static_cast<i32>(state.current_encounter_id) ==
        state.expected_encounter_id) {
        if (state.primary_suppression == 1U ||
            state.secondary_suppression == 1U) {
            if (state.alternate_surface_mode == 0U) {
                stage = state.stage;
                if (stage >= 1) {
                    if (!invoke_staged_surface(
                            state,
                            port,
                            result,
                            staged_surface_tokens,
                            stage,
                            0x01000000U
                        )) {
                        result.status = LegacyBattleFrameEffectStatus::
                            staged_surface_typed_stop;
                        return result;
                    }
                }
            } else {
                if (rotation_amount != 0) {
                    if (!rotate_source(result, source, rotation_amount)) {
                        result.status = LegacyBattleFrameEffectStatus::
                            source_rotation_typed_stop;
                        return result;
                    }
                    if (!play_rotation_frames(
                            state, port, context, result, rotation_amount
                        )) {
                        result.status = LegacyBattleFrameEffectStatus::
                            rotation_playback_typed_stop;
                        return result;
                    }
                    state.pending_rotation = 0;
                }
                if (!draw_source(context, result, source, 0U)) {
                    result.status =
                        LegacyBattleFrameEffectStatus::source_blit_typed_stop;
                    return result;
                }
                if (!adjust_stage_colors(state, context, result)) {
                    result.status = LegacyBattleFrameEffectStatus::
                        color_adjustment_typed_stop;
                    return result;
                }
                stage = state.stage;
            }

            i32 cadence = state.cadence;
            if (cadence > 1) {
                cadence = 0;
                stage = std::bit_cast<i16>(
                    static_cast<u16>(static_cast<u16>(stage) + 1U)
                );
                if (stage > 2) {
                    stage = 2;
                }
                state.stage = stage;
            }
            state.cadence = wrapping_add(cadence, 1);
            ++result.cadence_updates;
        }
    }

    if (state.fade_active != 1U || state.fade_block != 0U) {
        return result;
    }
    if (stage < 1) {
        reset_effect_state(state, result);
        return result;
    }

    stage = std::bit_cast<i16>(static_cast<u16>(static_cast<u16>(stage) - 1U));
    state.stage = stage;
    if (state.selected_surface_index != -1 &&
        state.alternate_surface_mode == 0U) {
        if (!invoke_staged_surface(
                state, port, result, staged_surface_tokens, stage, 0U
            )) {
            result.status =
                LegacyBattleFrameEffectStatus::staged_surface_typed_stop;
        }
        return result;
    }

    if (!draw_source(context, result, source, 0U)) {
        result.status = LegacyBattleFrameEffectStatus::source_blit_typed_stop;
    }
    return result;
}

}  // namespace openswd3::battle
