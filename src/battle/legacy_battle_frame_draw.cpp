#include "openswd3/battle/legacy_battle_frame_draw.hpp"

namespace openswd3::battle {
namespace {

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

}  // namespace

LegacyBattleFrameDrawResult draw_legacy_battle_frame_zero(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    LegacyBattleFrameDrawResult result{
        .frame_load_calls = 1U,
    };
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, 0U, piece);
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = 0U;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;

    rendering::LegacyBlitSource call_source = state.current_source;
    call_source.palette = {};
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    request.auxiliary = {};

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, call_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

LegacyBattleLayeredFrameDrawResult draw_legacy_battle_layered_resource_frames(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 second_width,
    const compat::u32 second_frame_index,
    const compat::u32 legacy_return_value
) noexcept {
    LegacyBattleLayeredFrameDrawResult result{
        .legacy_return_value = legacy_return_value,
    };
    result.first = draw_legacy_battle_resource_frame(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        0U,
        x,
        y
    );
    result.frame_load_calls = result.first.frame_load_calls;
    result.frame_draw_calls = result.first.frame_draw_calls;
    if (result.first.status != LegacyBattleFrameDrawStatus::completed) {
        result.status =
            LegacyBattleLayeredFrameDrawStatus::first_frame_typed_stop;
        return result;
    }

    result.second = draw_legacy_battle_resource_frame_width(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        second_frame_index,
        x,
        y,
        second_width,
        false
    );
    result.frame_load_calls += result.second.frame_load_calls;
    result.frame_draw_calls += result.second.frame_draw_calls;
    if (result.second.status != LegacyBattleFrameDrawStatus::completed &&
        result.second.status !=
            LegacyBattleFrameDrawStatus::width_nonpositive) {
        result.status =
            LegacyBattleLayeredFrameDrawStatus::second_frame_typed_stop;
    }
    return result;
}

LegacyBattleLayeredFrameDrawResult
draw_legacy_battle_layered_resource_frame_two(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 second_width
) noexcept {
    return draw_legacy_battle_layered_resource_frames(
        state,
        framebuffer,
        clip,
        shared_request,
        shared_effects,
        jitter,
        frame_provider,
        resource_id,
        x,
        y,
        second_width,
        2U,
        1U
    );
}

LegacyBattleFrameDrawResult draw_legacy_battle_resource_frame(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::u32 frame_index,
    const compat::i32 x,
    const compat::i32 y
) noexcept {
    LegacyBattleFrameDrawResult result{
        .frame_load_calls = 1U,
    };
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, frame_index, piece);
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = static_cast<compat::i32>(piece.width);
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    if (piece.source.palette.empty()) {
        request.auxiliary = {};
    } else {
        request.auxiliary = {
            reinterpret_cast<const compat::u8*>(piece.source.palette.data()),
            piece.source.palette.size_bytes(),
        };
    }

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, state.current_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

LegacyBattleFrameDrawResult draw_legacy_battle_resource_frame_width(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    const compat::u32 resource_id,
    const compat::u32 frame_index,
    const compat::i32 x,
    const compat::i32 y,
    const compat::i32 explicit_width,
    const bool skip_nonpositive_width
) noexcept {
    LegacyBattleFrameDrawResult result{
        .frame_load_calls = 1U,
    };
    rendering::LegacyFramePiece piece{};
    const bool available =
        frame_provider.load_frame_piece(resource_id, frame_index, piece);
    state.frame_record_published = true;
    state.frame_record_available = available;
    state.current_frame_index = frame_index;
    if (!available) {
        state.current_frame = {};
        result.status = LegacyBattleFrameDrawStatus::frame_unavailable;
        return result;
    }

    state.current_frame = piece;
    state.current_source = piece.source;
    state.source_published = true;
    if ((skip_nonpositive_width && explicit_width <= 0) ||
        (!skip_nonpositive_width && explicit_width == 0)) {
        result.status = LegacyBattleFrameDrawStatus::width_nonpositive;
        return result;
    }

    rendering::LegacyBlitRequest request = shared_request;
    request.destination_x = x;
    request.destination_y = y;
    request.source_width = explicit_width;
    request.source_height = static_cast<compat::i32>(piece.height);
    request.flags = 0U;
    if (piece.source.palette.empty()) {
        request.auxiliary = {};
    } else {
        request.auxiliary = {
            reinterpret_cast<const compat::u8*>(piece.source.palette.data()),
            piece.source.palette.size_bytes(),
        };
    }

    const rendering::LegacyBlitResult blit = rendering::blit_legacy_copy_paths(
        framebuffer, clip, state.current_source, request, shared_effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request, shared_effects);
    return result;
}

}  // namespace openswd3::battle
