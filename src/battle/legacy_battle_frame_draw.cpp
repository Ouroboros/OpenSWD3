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
    rendering::LegacyBlitRequest& shared_request
) noexcept {
    shared_request.target_height = 0;
    shared_request.horizontal_resample_displacement = 0;
    shared_request.vertical_resample_phase_10_10 = 0U;
    shared_request.opacity_step = 0;
}

}  // namespace

LegacyBattleFrameDrawResult draw_legacy_battle_frame_zero(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    const rendering::LegacyBlitEffectState& effects,
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
        framebuffer, clip, call_source, request, effects, jitter
    );
    result.frame_draw_calls = 1U;
    result.blit_status = blit.status;
    if (!accepted_blit_status(blit.status)) {
        result.status = LegacyBattleFrameDrawStatus::blit_typed_stop;
        return result;
    }

    publish_blitter_normal_epilogue(shared_request);
    return result;
}

}  // namespace openswd3::battle
