#include "openswd3/asset_runtime/legacy_ani_follower_effect.hpp"

#include <bit>

namespace openswd3::asset_runtime {
namespace {

using compat::i32;
using compat::u32;

constexpr i32 kSecondClipRadius = 0xC0;
constexpr u32 kSecondDrawFlags = 0x2CU;

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) + std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) - std::bit_cast<u32>(right)
    );
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

}  // namespace

LegacyAniFollowerRuntimePorts::LegacyAniFollowerRuntimePorts(
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

LegacyActionUpdateStatus LegacyAniFollowerRuntimePorts::update_action_record(
    LegacyActionRecord& record
) {
    return action_updater_.update(record).status;
}

bool LegacyAniFollowerRuntimePorts::load_frame_piece(
    const compat::u16 resource_id,
    const compat::u16 frame_index,
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

void LegacyAniFollowerRuntimePorts::set_clip_rectangle(
    const compat::i32 left,
    const compat::i32 top,
    const compat::i32 right,
    const compat::i32 bottom
) noexcept {
    rendering::set_legacy_clip_rectangle(raster_, left, top, right, bottom);
}

rendering::LegacyBlitExecutionStatus
LegacyAniFollowerRuntimePorts::draw_frame_piece(
    const rendering::LegacyFramePiece& piece,
    const compat::i32 destination_x,
    const compat::i32 destination_y,
    const compat::u32 flags
) noexcept {
    return rendering::blit_legacy_copy_paths(
               framebuffer_,
               current_clip(raster_),
               piece.source,
               rendering::LegacyBlitRequest{
                   .destination_x = destination_x,
                   .destination_y = destination_y,
                   .source_width = static_cast<compat::i32>(piece.width),
                   .source_height = static_cast<compat::i32>(piece.height),
                   .flags = flags,
                   .opacity_step = 0,
               },
               effects_,
               jitter_
    )
        .status;
}

LegacyAniFollowerResult update_draw_legacy_ani_follower(
    const bool enabled,
    LegacyAniFollowerState& state,
    LegacyActionRecord& action_record,
    LegacyAniFollowerPorts& ports
) {
    LegacyAniFollowerResult result;
    if (!enabled) {
        result.status = LegacyAniFollowerStatus::disabled;
        return result;
    }

    const auto prepare_frame = [&](const u32 variant,
                                   rendering::LegacyFramePiece& piece) -> bool {
        action_record.action_id = kLegacyAniFollowerActionId;
        action_record.base_variant = variant;
        ++result.action_update_count;
        if (ports.update_action_record(action_record) !=
            LegacyActionUpdateStatus::completed) {
            result.status = LegacyAniFollowerStatus::action_update_failed;
            result.failed_variant = variant;
            return false;
        }

        ++result.frame_request_count;
        if (!ports.load_frame_piece(
                action_record.field_4a, action_record.field_4c, piece
            )) {
            result.status = LegacyAniFollowerStatus::frame_load_failed;
            result.failed_variant = variant;
            return false;
        }
        return true;
    };

    const auto draw_frame = [&](const rendering::LegacyFramePiece& piece,
                                const u32 flags) {
        const i32 half_width = static_cast<i32>(piece.width >> 1U);
        const i32 half_height = static_cast<i32>(piece.height >> 1U);
        result.last_blit_status = ports.draw_frame_piece(
            piece,
            wrapping_subtract(state.current_x, half_width),
            wrapping_subtract(state.current_y, half_height),
            flags
        );
        ++result.draw_count;
        if (!accepted_blit_status(result.last_blit_status)) {
            ++result.blit_failure_count;
        }
    };

    rendering::LegacyFramePiece first;
    if (!prepare_frame(kLegacyAniFollowerFirstVariant, first)) {
        return result;
    }
    const i32 first_half_width = static_cast<i32>(first.width >> 1U);
    const i32 first_half_height = static_cast<i32>(first.height >> 1U);
    ports.set_clip_rectangle(
        wrapping_subtract(state.current_x, first_half_width),
        wrapping_subtract(state.current_y, first_half_width),
        wrapping_add(state.current_x, first_half_height),
        wrapping_add(state.current_y, first_half_height)
    );
    draw_frame(first, 0U);

    rendering::LegacyFramePiece second;
    if (!prepare_frame(kLegacyAniFollowerSecondVariant, second)) {
        return result;
    }
    ports.set_clip_rectangle(
        wrapping_subtract(state.current_x, kSecondClipRadius),
        wrapping_subtract(state.current_y, kSecondClipRadius),
        wrapping_add(state.current_x, kSecondClipRadius),
        wrapping_add(state.current_y, kSecondClipRadius)
    );
    draw_frame(second, kSecondDrawFlags);

    if (state.current_x == state.target_x &&
        state.current_y == state.target_y) {
        return result;
    }

    state.current_x = wrapping_add(state.current_x, state.velocity_x);
    state.current_y = wrapping_add(state.current_y, state.velocity_y);
    if (state.current_x == state.target_x) {
        state.velocity_x = 0;
    }
    if (state.current_y == state.target_y) {
        state.velocity_y = 0;
    }
    return result;
}

}  // namespace openswd3::asset_runtime
