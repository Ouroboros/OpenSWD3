#include "openswd3/asset_runtime/legacy_ani_drift_effect.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace openswd3::asset_runtime {
namespace {

using compat::i32;
using compat::u32;

constexpr i32 kOuterMargin = 120;
constexpr i32 kSpawnMargin = 64;
constexpr std::array<i32, 8U> kVerticalVariants{
    0x34,
    0x34,
    0x34,
    0x36,
    0x38,
    0x38,
    0x38,
    0x36,
};

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

[[nodiscard]] constexpr i32 wrapping_scale_by_16(const i32 value) noexcept {
    return std::bit_cast<i32>(std::bit_cast<u32>(value) << 4U);
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

}  // namespace

LegacyAniDriftRuntimePorts::LegacyAniDriftRuntimePorts(
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
LegacyAniDriftRuntimePorts::update_action_record(LegacyActionRecord& record) {
    return action_updater_.update(record).status;
}

bool LegacyAniDriftRuntimePorts::load_frame_piece(
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

rendering::LegacyBlitExecutionStatus
LegacyAniDriftRuntimePorts::draw_frame_piece(
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

LegacyAniDriftEffect::LegacyAniDriftEffect() noexcept {
    for (LegacyActionRecord& record : action_records_) {
        initialize_legacy_action_record(record);
        record.action_id = kLegacyAniDriftActionId;
    }
    reset_positions();
}

void LegacyAniDriftEffect::reset_positions() noexcept {
    for (LegacyAniDriftSlot& slot : state_.slots) {
        slot.x = kLegacyAniDriftInactiveX;
    }
}

LegacyAniDriftResult LegacyAniDriftEffect::update(
    const compat::i32 map_width_tiles,
    const compat::i32 map_height_tiles,
    const compat::i32 camera_x,
    const compat::i32 camera_y,
    input_time_rng::LegacySecondaryRng& random,
    LegacyAniDriftServicePort& services,
    LegacyAniDriftPorts& ports
) {
    LegacyAniDriftResult result;
    ++result.service_query_count;
    if (!services.service_enabled(kLegacyAniDriftServiceId)) {
        result.status = LegacyAniDriftStatus::disabled;
        return result;
    }

    const i32 map_width_pixels = wrapping_scale_by_16(map_width_tiles);
    const i32 map_height_pixels = wrapping_scale_by_16(map_height_tiles);
    const i32 maximum_x = wrapping_add(map_width_pixels, kOuterMargin);
    const i32 maximum_y = wrapping_add(map_height_pixels, kOuterMargin);

    for (std::size_t index = 0U; index < state_.slots.size(); ++index) {
        LegacyAniDriftSlot& slot = state_.slots[index];
        LegacyActionRecord& record = action_records_[index];

        if (slot.x <= -kOuterMargin || slot.x >= maximum_x ||
            slot.y <= -kOuterMargin || slot.y >= maximum_y) {
            slot.x = kLegacyAniDriftInactiveX;
        }

        if (slot.x == kLegacyAniDriftInactiveX) {
            ++result.respawn_count;
            slot.x = -kSpawnMargin;
            slot.y = static_cast<i32>(
                random.next_bounded(std::bit_cast<u32>(map_height_pixels))
            );
            slot.velocity_y = static_cast<i32>(random.next_bounded(5U)) - 2;
            slot.velocity_x = static_cast<i32>(random.next_bounded(7U)) - 3;
            if (slot.velocity_x == 0) {
                slot.velocity_x = 2;
            }
            if (slot.velocity_x < 0) {
                slot.x = wrapping_add(map_width_pixels, kSpawnMargin);
            }
        } else if (random.next_bounded(1000U) > 250U) {
            ++result.perturbation_count;
            const i32 horizontal_delta =
                static_cast<i32>(random.next_bounded(3U)) - 1;
            const i32 vertical_delta =
                static_cast<i32>(random.next_bounded(3U)) - 1;

            slot.velocity_y = std::clamp(
                wrapping_add(slot.velocity_y, vertical_delta), -3, 3
            );
            slot.velocity_x = std::clamp(
                wrapping_add(slot.velocity_x, horizontal_delta), -5, 5
            );
            if (slot.velocity_x == 0) {
                slot.velocity_x = wrapping_subtract(0, horizontal_delta);
            }
        }

        const i32 variant_index = slot.velocity_y + 3;
        if (variant_index < 0 ||
            variant_index >= static_cast<i32>(kVerticalVariants.size())) {
            result.status = LegacyAniDriftStatus::invalid_vertical_velocity;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }
        i32 variant =
            kVerticalVariants[static_cast<std::size_t>(variant_index)];
        if (slot.velocity_x > 0) {
            ++variant;
        }
        record.base_variant = static_cast<u32>(variant);

        ++result.action_update_count;
        if (ports.update_action_record(record) !=
            LegacyActionUpdateStatus::completed) {
            result.status = LegacyAniDriftStatus::action_update_failed;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }

        rendering::LegacyFramePiece piece;
        ++result.frame_request_count;
        if (!ports.load_frame_piece(record.field_4a, record.field_4c, piece)) {
            result.status = LegacyAniDriftStatus::frame_load_failed;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }

        const i32 destination_x = wrapping_subtract(
            wrapping_subtract(slot.x, field_as_i32(record.draw_offset_x)),
            camera_x
        );
        const i32 destination_y = wrapping_subtract(
            wrapping_subtract(slot.y, field_as_i32(record.draw_offset_y)),
            camera_y
        );
        result.last_blit_status = ports.draw_frame_piece(
            piece, destination_x, destination_y, record.mode_flags
        );
        ++result.draw_count;
        if (!accepted_blit_status(result.last_blit_status)) {
            ++result.blit_failure_count;
        }

        slot.x = wrapping_add(slot.x, slot.velocity_x);
        slot.y = wrapping_add(slot.y, slot.velocity_y);
    }
    return result;
}

LegacyAniDriftState& LegacyAniDriftEffect::state() noexcept {
    return state_;
}

const LegacyAniDriftState& LegacyAniDriftEffect::state() const noexcept {
    return state_;
}

std::array<LegacyActionRecord, kLegacyAniDriftSlotCount>&
LegacyAniDriftEffect::action_records() noexcept {
    return action_records_;
}

const std::array<LegacyActionRecord, kLegacyAniDriftSlotCount>&
LegacyAniDriftEffect::action_records() const noexcept {
    return action_records_;
}

}  // namespace openswd3::asset_runtime
