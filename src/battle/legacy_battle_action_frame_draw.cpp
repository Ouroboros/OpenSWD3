#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"

#include <bit>
#include <cstring>
#include <span>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u32 to_bits(const compat::i32 value) noexcept {
    return std::bit_cast<compat::u32>(value);
}

[[nodiscard]] compat::i32 from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] compat::i32 wrapping_subtract(
    const compat::i32 value, const compat::u32 displacement
) noexcept {
    return from_bits(to_bits(value) - displacement);
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

[[nodiscard]] std::span<const compat::u8>
palette_bytes(const std::span<const compat::u16> palette) noexcept {
    if (palette.empty()) {
        return {};
    }
    return {
        reinterpret_cast<const compat::u8*>(palette.data()),
        palette.size_bytes(),
    };
}

[[nodiscard]] rendering::LegacyBlitRequest make_frame_request(
    const rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyFramePiece& frame,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 flags
) noexcept {
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(frame.width);
    request.source_height = static_cast<compat::i32>(frame.height);
    request.flags = flags;
    request.auxiliary = palette_bytes(frame.source.palette);
    return request;
}

}  // namespace

LegacyBattleActionFrameDrawResult draw_legacy_battle_action_frame(
    LegacyBattleActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const std::span<const compat::u8> outline_state_by_variant,
    const compat::u32 variant,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 overlay_selector
) {
    LegacyBattleActionFrameDrawResult result;
    const compat::u16 selected_variant =
        static_cast<compat::u16>(variant & 0xFFFFU);

    state.action_record = {};
    state.action_record.action_id = 0x2390U;
    state.action_record.base_variant = selected_variant;
    state.action_update_attempted = true;
    state.action_update = action_updater.update(state.action_record);
    if (state.action_update.return_value == 0U) {
        result.status = LegacyBattleActionFrameDrawStatus::action_update_failed;
        return result;
    }

    const compat::u32 frame_resource = state.action_record.field_4a;
    const compat::u32 frame_index = state.action_record.field_4c;
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(frame_resource, frame_index, piece);
    result.frame_load_calls = 1U;
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleActionFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;

    if (static_cast<std::size_t>(selected_variant) >=
        outline_state_by_variant.size()) {
        result.status =
            LegacyBattleActionFrameDrawStatus::outline_state_out_of_range;
        return result;
    }

    const compat::i32 draw_x =
        wrapping_subtract(x, state.action_record.draw_offset_x);
    const compat::i32 draw_y =
        wrapping_subtract(y, state.action_record.draw_offset_y);
    result.draw_x = draw_x;
    result.draw_y = draw_y;

    if (outline_state_by_variant[selected_variant] == 1U) {
        state.outline_color_slot = {0x07E0U, 0x07E0U};
        rendering::LegacyBlitSource outline_source = state.current_source;
        outline_source.palette = state.outline_color_slot;
        rendering::LegacyBlitRequest outline_request =
            make_frame_request(shared_request, piece, draw_x, draw_y, 0U);
        outline_request.auxiliary = palette_bytes(state.outline_color_slot);
        result.outline = rendering::blit_legacy_outline_copy_paths(
            framebuffer,
            clip,
            outline_source,
            outline_request,
            shared_effects,
            jitter
        );
        result.outline_draw_calls = result.outline.pass_count;
        shared_request.target_height = outline_request.target_height;
        shared_request.horizontal_resample_displacement =
            outline_request.horizontal_resample_displacement;
        shared_request.vertical_resample_phase_10_10 =
            outline_request.vertical_resample_phase_10_10;
        shared_request.opacity_step = outline_request.opacity_step;

        if (result.outline.pass_count != 4U ||
            !accepted_blit_status(result.outline
                                      .passes
                                          [result.outline.pass_count == 0U
                                               ? 0U
                                               : result.outline.pass_count - 1U]
                                      .status)) {
            result.status =
                LegacyBattleActionFrameDrawStatus::outline_typed_stop;
            if (result.outline.pass_count != 0U) {
                result.last_blit_status =
                    result.outline.passes[result.outline.pass_count - 1U]
                        .status;
            }
            return result;
        }
    }

    rendering::LegacyBlitRequest primary_request =
        make_frame_request(shared_request, piece, draw_x, draw_y, 0U);
    const rendering::LegacyBlitResult primary =
        rendering::blit_legacy_copy_paths(
            framebuffer,
            clip,
            state.current_source,
            primary_request,
            shared_effects,
            jitter
        );
    ++result.frame_draw_calls;
    result.last_blit_status = primary.status;
    if (!accepted_blit_status(primary.status)) {
        result.status =
            LegacyBattleActionFrameDrawStatus::primary_blit_typed_stop;
        return result;
    }
    publish_blitter_normal_epilogue(shared_request, shared_effects);

    if (overlay_selector != 1U) {
        return result;
    }

    shared_effects.red_offset = 0;
    shared_effects.green_offset = -10;
    shared_effects.blue_offset = -10;
    rendering::LegacyBlitRequest overlay_request = make_frame_request(
        shared_request,
        piece,
        draw_x,
        draw_y,
        state.action_record.mode_flags | 0x10U
    );
    const rendering::LegacyBlitResult overlay =
        rendering::blit_legacy_copy_paths(
            framebuffer,
            clip,
            state.current_source,
            overlay_request,
            shared_effects,
            jitter
        );
    ++result.frame_draw_calls;
    result.last_blit_status = overlay.status;
    if (!accepted_blit_status(overlay.status)) {
        result.status =
            LegacyBattleActionFrameDrawStatus::overlay_blit_typed_stop;
        return result;
    }
    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

LegacyBattlePreparedActionFrameDrawResult
draw_legacy_battle_prepared_action_frame(
    LegacyBattlePreparedActionFrameDrawState& state,
    const std::span<asset_runtime::LegacyActionRecord> action_records,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 action_id,
    const compat::u32 action_record_index,
    const compat::u32 action_update_ecx_snapshot,
    const compat::i32 x,
    const compat::i32 y
) {
    LegacyBattlePreparedActionFrameDrawResult result{
        .draw_x = x,
        .draw_y = y,
    };
    state.requested_record_index = action_record_index;
    state.wrapped_record_offset = action_record_index *
        static_cast<compat::u32>(asset_runtime::kLegacyActionRecordSize);
    if (state.wrapped_record_offset % asset_runtime::kLegacyActionRecordSize !=
        0U) {
        result.status = LegacyBattlePreparedActionFrameDrawStatus::
            action_record_out_of_range;
        return result;
    }
    const std::size_t resolved_index =
        state.wrapped_record_offset / asset_runtime::kLegacyActionRecordSize;
    if (resolved_index >= action_records.size()) {
        result.status = LegacyBattlePreparedActionFrameDrawStatus::
            action_record_out_of_range;
        return result;
    }
    state.resolved_record_index = static_cast<compat::u32>(resolved_index);

    asset_runtime::LegacyActionRecord& action = action_records[resolved_index];
    action.action_id = action_id;
    action.base_variant = 0U;
    state.action_update_attempted = true;
    state.action_update = action_updater.update(action);
    if (state.action_update.return_value == 0U) {
        result.status =
            LegacyBattlePreparedActionFrameDrawStatus::action_update_failed;
        return result;
    }

    state.frame_resource_id = (action_update_ecx_snapshot & 0xFFFF0000U) |
        static_cast<compat::u32>(action.field_4a);
    state.frame_index = static_cast<compat::u32>(action.field_4c);
    rendering::LegacyFramePiece piece{};
    const bool available = frame_provider.load_frame_piece(
        state.frame_resource_id, state.frame_index, piece
    );
    result.frame_load_calls = 1U;
    if (!available) {
        state.current_frame = {};
        result.status =
            LegacyBattlePreparedActionFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request =
        make_frame_request(shared_request, piece, x, y, action.mode_flags);
    request.auxiliary = {};
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status =
            LegacyBattlePreparedActionFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

compat::u32 clear_legacy_battle_action_record(
    asset_runtime::LegacyActionRecord& action_record
) noexcept {
    std::memset(&action_record, 0, asset_runtime::kLegacyActionRecordSize);
    return 0U;
}

LegacyBattleOffsetActionFrameDrawResult draw_legacy_battle_offset_action_frame(
    LegacyBattleOffsetActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 action_id,
    const compat::u32 base_variant,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 offset_mode,
    const compat::u32 action_update_edx_snapshot
) {
    LegacyBattleOffsetActionFrameDrawResult result;
    state.action_record.action_id = action_id;
    state.action_record.base_variant = base_variant;
    state.action_update_attempted = true;
    state.action_update = action_updater.update(state.action_record);
    if (state.action_update.return_value == 0U) {
        result.status =
            LegacyBattleOffsetActionFrameDrawStatus::action_update_failed;
        return result;
    }

    state.frame_resource_id =
        static_cast<compat::u32>(state.action_record.field_4a);
    state.frame_index = (action_update_edx_snapshot & 0xFFFF0000U) |
        static_cast<compat::u32>(state.action_record.field_4c);
    rendering::LegacyFramePiece piece{};
    const bool available = frame_provider.load_frame_piece(
        state.frame_resource_id, state.frame_index, piece
    );
    result.frame_load_calls = 1U;

    const compat::u32 x_offset = state.action_record.draw_offset_x;
    state.effective_flags = state.action_record.mode_flags;
    if (offset_mode == 1U) {
        state.effective_flags ^= 1U;
    }
    if (!available) {
        state.current_frame = {};
        result.status =
            LegacyBattleOffsetActionFrameDrawStatus::frame_unavailable;
        return result;
    }

    compat::u16 x_correction_word = static_cast<compat::u16>(x_offset);
    if (offset_mode == 1U) {
        x_correction_word = static_cast<compat::u16>(
            piece.width - static_cast<compat::u16>(x_offset)
        );
    }
    const compat::i32 x_correction =
        static_cast<compat::i32>(std::bit_cast<compat::i16>(x_correction_word));
    result.draw_x = from_bits(to_bits(x) - to_bits(x_correction));
    result.draw_y = wrapping_subtract(y, state.action_record.draw_offset_y);

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = make_frame_request(
        shared_request,
        piece,
        result.draw_x,
        result.draw_y,
        state.effective_flags
    );
    request.auxiliary = {};
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status =
            LegacyBattleOffsetActionFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    state.result_latch_read = true;
    result.return_value = state.result_latch == 1U ? 1U : 0U;
    return result;
}

LegacyBattleStandaloneActionFrameDrawResult
draw_legacy_battle_standalone_action_frame(
    LegacyBattleStandaloneActionFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 action_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::u32 action_update_ecx_snapshot,
    const compat::u32 action_update_edx_snapshot
) {
    LegacyBattleStandaloneActionFrameDrawResult result{
        .draw_x = x,
        .draw_y = y,
    };
    state.action_record.action_id = action_id;
    state.action_record.base_variant = 0U;
    state.action_update_attempted = true;
    state.action_update = action_updater.update(state.action_record);
    if (state.action_update.return_value == 0U) {
        result.status =
            LegacyBattleStandaloneActionFrameDrawStatus::action_update_failed;
        return result;
    }

    state.frame_index = (action_update_ecx_snapshot & 0xFFFF0000U) |
        static_cast<compat::u32>(state.action_record.field_4c);
    state.frame_resource_id = (action_update_edx_snapshot & 0xFFFF0000U) |
        static_cast<compat::u32>(state.action_record.field_4a);
    rendering::LegacyFramePiece piece{};
    const bool available = frame_provider.load_frame_piece(
        state.frame_resource_id, state.frame_index, piece
    );
    result.frame_load_calls = 1U;
    if (!available) {
        state.current_frame = {};
        result.status =
            LegacyBattleStandaloneActionFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = make_frame_request(
        shared_request, piece, x, y, state.action_record.mode_flags
    );
    request.auxiliary = {};
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status =
            LegacyBattleStandaloneActionFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

LegacyBattleIndexedActionFrameDrawResult
draw_legacy_battle_indexed_action_frame(
    LegacyBattleIndexedActionFrameDrawState& state,
    const std::span<asset_runtime::LegacyActionRecord> action_records,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 action_record_index
) {
    LegacyBattleIndexedActionFrameDrawResult result;
    state.action_record_index = to_bits(action_record_index);
    if (action_record_index < 0 ||
        static_cast<std::size_t>(action_record_index) >=
            action_records.size()) {
        result.status = LegacyBattleIndexedActionFrameDrawStatus::
            action_record_out_of_range;
        return result;
    }

    asset_runtime::LegacyActionRecord& action =
        action_records[static_cast<std::size_t>(action_record_index)];
    action.action_id = 0x2392U;
    action.base_variant = 0U;
    state.action_update_attempted = true;
    state.action_update = action_updater.update(action);
    if (state.action_update.return_value == 0U) {
        result.status =
            LegacyBattleIndexedActionFrameDrawStatus::action_update_failed;
        return result;
    }

    rendering::LegacyFramePiece piece{};
    const compat::u32 frame_resource = action.field_4a;
    const compat::u32 frame_index = action.field_4c;
    const bool available =
        frame_provider.load_frame_piece(frame_resource, frame_index, piece);
    result.frame_load_calls = 1U;
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status =
            LegacyBattleIndexedActionFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    result.draw_x = wrapping_subtract(x, action.draw_offset_x);
    result.draw_y = wrapping_subtract(y, action.draw_offset_y);

    rendering::LegacyBlitRequest request = make_frame_request(
        shared_request, piece, result.draw_x, result.draw_y, action.mode_flags
    );
    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, state.current_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status =
            LegacyBattleIndexedActionFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

}  // namespace openswd3::battle
