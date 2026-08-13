#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"

#include <bit>

namespace openswd3::asset_runtime {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) - std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32 field_as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] rendering::LegacyBlitClipRectangle
current_clip(const rendering::LegacyRasterGeometryState& raster) noexcept {
    return rendering::LegacyBlitClipRectangle{
        .left = raster.clip_left,
        .top = raster.clip_top,
        .width = raster.clip_width,
        .height = raster.clip_height,
    };
}

[[nodiscard]] LegacyActionDrawResult load_and_draw(
    const u16 resource_id,
    const u16 frame_index,
    const i32 x,
    const i32 y,
    const u32 flags,
    const i32 opacity_step,
    LegacyActionDrawPorts& ports,
    LegacyActionDrawResult result
) {
    rendering::LegacyFramePiece piece;
    ++result.frame_request_count;
    if (!ports.load_frame_piece(resource_id, frame_index, piece)) {
        result.status = LegacyActionDrawStatus::frame_load_failed;
        return result;
    }

    result.last_blit_status =
        ports.draw_frame_piece(piece, x, y, flags, opacity_step);
    ++result.draw_count;
    if (!accepted_blit_status(result.last_blit_status)) {
        ++result.blit_failure_count;
    }
    return result;
}

[[nodiscard]] LegacyActionDrawResult update_and_draw(
    LegacyActionRecord& record,
    const i32 x,
    const i32 y,
    const u32 caller_flags,
    const bool mask_action_flags,
    LegacyActionDrawPorts& ports
) {
    LegacyActionDrawResult result;
    ++result.action_update_count;
    if (ports.update_action_record(record) !=
        LegacyActionUpdateStatus::completed) {
        result.status = LegacyActionDrawStatus::action_update_failed;
        return result;
    }

    const u32 flags = mask_action_flags
        ? caller_flags | (record.mode_flags & 0x80000003U)
        : record.mode_flags;
    return load_and_draw(
        record.field_4a,
        record.field_4c,
        wrapping_subtract(x, field_as_i32(record.draw_offset_x)),
        wrapping_subtract(y, field_as_i32(record.draw_offset_y)),
        flags,
        static_cast<i32>(record.field_8a),
        ports,
        result
    );
}

}  // namespace

LegacyActionDrawRuntimePorts::LegacyActionDrawRuntimePorts(
    LegacyActionUpdater& action_updater,
    LegacyTswRuntime& tsw_runtime,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    const rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter
) noexcept
    : action_updater_(action_updater), tsw_runtime_(tsw_runtime),
      framebuffer_(framebuffer), raster_(raster), effects_(effects),
      jitter_(jitter) {}

LegacyActionUpdateStatus
LegacyActionDrawRuntimePorts::update_action_record(LegacyActionRecord& record) {
    return action_updater_.update(record).status;
}

bool LegacyActionDrawRuntimePorts::load_frame_piece(
    const u16 resource_id,
    const u16 frame_index,
    rendering::LegacyFramePiece& piece
) {
    const LegacyTswQueryResult loaded =
        tsw_runtime_.query_cached(resource_id, frame_index);
    if (loaded.status != LegacyTswRuntimeStatus::ready) {
        piece = rendering::LegacyFramePiece{};
        return false;
    }

    piece = rendering::LegacyFramePiece{
        .source =
            rendering::LegacyBlitSource{
                .bytes = loaded.frame.primary_stream,
                .layout = rendering::LegacyBlitSourceLayout::direct_16,
                .palette = {},
            },
        .width = loaded.frame.width,
        .height = loaded.frame.height,
    };
    return true;
}

rendering::LegacyBlitExecutionStatus
LegacyActionDrawRuntimePorts::draw_frame_piece(
    const rendering::LegacyFramePiece& piece,
    const i32 destination_x,
    const i32 destination_y,
    const u32 flags,
    const i32 opacity_step
) noexcept {
    return rendering::blit_legacy_copy_paths(
               framebuffer_,
               current_clip(raster_),
               piece.source,
               rendering::LegacyBlitRequest{
                   .destination_x = destination_x,
                   .destination_y = destination_y,
                   .source_width = piece.width,
                   .source_height = piece.height,
                   .target_height = piece.height,
                   .flags = flags,
                   .opacity_step = opacity_step,
               },
               effects_,
               jitter_
    )
        .status;
}

LegacyActionDrawResult update_draw_legacy_action(
    LegacyActionRecord& record,
    const i32 x,
    const i32 y,
    LegacyActionDrawPorts& ports
) {
    return update_and_draw(record, x, y, 0U, false, ports);
}

LegacyActionDrawResult draw_legacy_tsw_frame(
    const u32 resource_id_slot,
    const u32 frame_index_slot,
    const i32 x,
    const i32 y,
    LegacyActionDrawPorts& ports
) {
    return load_and_draw(
        static_cast<u16>(resource_id_slot),
        static_cast<u16>(frame_index_slot),
        x,
        y,
        0U,
        0,
        ports,
        LegacyActionDrawResult{}
    );
}

LegacyActionDrawResult update_draw_legacy_action_with_flags(
    LegacyActionRecord& record,
    const i32 x,
    const i32 y,
    const u32 caller_flags,
    LegacyActionDrawPorts& ports
) {
    return update_and_draw(record, x, y, caller_flags, true, ports);
}

}  // namespace openswd3::asset_runtime
