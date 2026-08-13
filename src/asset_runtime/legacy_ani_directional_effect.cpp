#include "openswd3/asset_runtime/legacy_ani_directional_effect.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <bit>

namespace openswd3::asset_runtime {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr i32 kMinimumX = -640;
constexpr i32 kMinimumY = -320;
constexpr i32 kHorizontalOuterTiles = 40;
constexpr i32 kVerticalOuterTiles = 20;
constexpr i32 kVerticalSpawnOffset = 118;
constexpr i32 kTopSpawnY = -300;
constexpr u32 kRandomOutputRange = 0x0000FFFFU;

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

[[nodiscard]] constexpr i32
wrapping_multiply(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) * std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32 wrapping_scale_by_16(const i32 value) noexcept {
    return std::bit_cast<i32>(std::bit_cast<u32>(value) << 4U);
}

[[nodiscard]] constexpr i32
arithmetic_shift_right_two(const i32 value) noexcept {
    const u32 bits = std::bit_cast<u32>(value);
    const u32 sign_fill = (bits & 0x80000000U) != 0U ? 0xC0000000U : 0U;
    return std::bit_cast<i32>((bits >> 2U) | sign_fill);
}

[[nodiscard]] constexpr i32 field_as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr bool valid_random_bound(const u32 bound) noexcept {
    return bound != 0U && bound <= kRandomOutputRange;
}

[[nodiscard]] constexpr i32
move_toward(const i32 current, const i32 target) noexcept {
    if (current < target) {
        return wrapping_add(current, 1);
    }
    if (current > target) {
        return wrapping_subtract(current, 1);
    }
    return current;
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

[[nodiscard]] u32 random_bounded(
    input_time_rng::LegacySecondaryRng& random,
    const u32 upper_bound,
    LegacyAniDirectionalInitializationResult& result
) noexcept {
    ++result.random_call_count;
    return random.next_bounded(upper_bound);
}

[[nodiscard]] u32 random_bounded(
    input_time_rng::LegacySecondaryRng& random,
    const u32 upper_bound,
    LegacyAniDirectionalResult& result
) noexcept {
    ++result.random_call_count;
    return random.next_bounded(upper_bound);
}

}  // namespace

LegacyAniDirectionalRuntimePorts::LegacyAniDirectionalRuntimePorts(
    LegacyActionUpdater& action_updater,
    LegacyTswRuntime& tsw_runtime,
    rendering::LegacyFramebuffer& framebuffer,
    rendering::LegacyRasterGeometryState& raster,
    rendering::LegacyBlitEffectState& effects,
    rendering::LegacyRleRowJitterState& jitter
) noexcept
    : action_updater_(action_updater), tsw_runtime_(tsw_runtime),
      framebuffer_(framebuffer), raster_(raster), effects_(effects),
      jitter_(jitter) {}

LegacyActionUpdateStatus LegacyAniDirectionalRuntimePorts::update_action_record(
    LegacyActionRecord& record
) {
    return action_updater_.update(record).status;
}

bool LegacyAniDirectionalRuntimePorts::load_frame_piece(
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
LegacyAniDirectionalRuntimePorts::draw_frame_piece(
    const rendering::LegacyFramePiece& piece,
    const i32 destination_x,
    const i32 destination_y,
    const u32 flags,
    const i32 opacity_step,
    const i32 color_offset
) noexcept {
    effects_.red_offset = color_offset;
    effects_.green_offset = color_offset;
    effects_.blue_offset = color_offset;
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

LegacyAniDirectionalEffect::LegacyAniDirectionalEffect() noexcept {
    reset_motion_block();
}

void LegacyAniDirectionalEffect::reset_motion_block() noexcept {
    const i32 reset_value = std::bit_cast<i32>(kLegacyAniDirectionalResetWord);
    for (LegacyAniDirectionalMotionSlot& slot : state_.motion) {
        slot.world_x = reset_value;
        slot.world_y = reset_value;
        slot.velocity_x = reset_value;
        slot.velocity_y = reset_value;
    }
}

LegacyAniDirectionalInitializationResult
LegacyAniDirectionalEffect::initialize_slots(
    const LegacyAniDirectionalConfiguration& configuration,
    input_time_rng::LegacySecondaryRng& random
) noexcept {
    LegacyAniDirectionalInitializationResult result;
    const i32 map_width_pixels =
        wrapping_scale_by_16(configuration.map_width_tiles);
    const i32 map_height_pixels =
        wrapping_scale_by_16(configuration.map_height_tiles);
    const u32 width_bound = std::bit_cast<u32>(map_width_pixels);
    const u32 height_bound = std::bit_cast<u32>(map_height_pixels);

    for (std::size_t index = 0U; index < state_.motion.size(); ++index) {
        LegacyAniDirectionalMotionSlot& motion = state_.motion[index];
        LegacyAniDirectionalColorSlot& color = state_.color[index];
        LegacyAniDirectionalTimingSlot& timing = state_.timing[index];

        timing.target_interval =
            static_cast<i32>(random_bounded(random, 3U, result) + 1U);
        color.target_offset = 0;

        if (!valid_random_bound(configuration.variant_count)) {
            result.status =
                LegacyAniDirectionalInitializationStatus::invalid_random_bound;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }
        timing.variant = std::bit_cast<i32>(
            random_bounded(random, configuration.variant_count, result) +
            static_cast<u32>(configuration.base_variant)
        );

        if (!valid_random_bound(width_bound)) {
            result.status =
                LegacyAniDirectionalInitializationStatus::invalid_random_bound;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }
        motion.world_x =
            static_cast<i32>(random_bounded(random, width_bound, result));
        if (!valid_random_bound(height_bound)) {
            result.status =
                LegacyAniDirectionalInitializationStatus::invalid_random_bound;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }
        motion.world_y =
            static_cast<i32>(random_bounded(random, height_bound, result));
        motion.velocity_x =
            static_cast<i32>(random_bounded(random, 2U, result)) - 2;
        motion.velocity_y =
            static_cast<i32>(random_bounded(random, 2U, result)) - 2;
        ++result.initialized_slot_count;
    }
    return result;
}

LegacyAniDirectionalResult LegacyAniDirectionalEffect::update(
    const LegacyAniDirectionalConfiguration& configuration,
    const LegacyAniDirectionalFrameInput& frame,
    input_time_rng::LegacySecondaryRng& random,
    LegacyAniDirectionalServicePort& services,
    LegacyActionRecord& shared_action_record,
    LegacyAniDirectionalPorts& ports
) {
    LegacyAniDirectionalResult result;
    ++result.service_query_count;
    if (!services.service_enabled(kLegacyAniDirectionalServiceId)) {
        result.status = LegacyAniDirectionalStatus::disabled;
        return result;
    }

    const i32 maximum_x = wrapping_scale_by_16(
        wrapping_add(configuration.map_width_tiles, kHorizontalOuterTiles)
    );
    const i32 maximum_y = wrapping_scale_by_16(
        wrapping_add(configuration.map_height_tiles, kVerticalOuterTiles)
    );
    const i32 map_width_pixels =
        wrapping_scale_by_16(configuration.map_width_tiles);
    const i32 map_height_pixels =
        wrapping_scale_by_16(configuration.map_height_tiles);

    for (std::size_t index = 0U; index < kLegacyAniDirectionalUpdatedSlotCount;
         ++index) {
        LegacyAniDirectionalMotionSlot& motion = state_.motion[index];
        LegacyAniDirectionalColorSlot& color = state_.color[index];
        LegacyAniDirectionalTimingSlot& timing = state_.timing[index];
        const u32 frame_roll = random_bounded(random, 1000U, result);

        const bool inside = motion.world_x > kMinimumX &&
            motion.world_x < maximum_x && motion.world_y > kMinimumY &&
            motion.world_y < maximum_y;
        if (!inside) {
            if (frame_roll < 990U) {
                ++result.skipped_outside_slot_count;
                continue;
            }

            color.target_offset = 0;
            timing.target_interval = static_cast<i32>(frame_roll / 100U + 1U);
            timing.variant = std::bit_cast<i32>(
                (static_cast<u32>(configuration.variant_count) * frame_roll) /
                    1000U +
                static_cast<u32>(configuration.base_variant)
            );

            if (configuration.spawn_direction > 3U) {
                ++result.invalid_direction_count;
                continue;
            }

            const u32 width_bound = std::bit_cast<u32>(map_width_pixels);
            if (!valid_random_bound(width_bound)) {
                result.status =
                    LegacyAniDirectionalStatus::invalid_random_bound;
                result.failed_slot = static_cast<u32>(index);
                return result;
            }
            motion.world_x =
                static_cast<i32>(random_bounded(random, width_bound, result));

            if (configuration.spawn_direction < 2U) {
                motion.world_y =
                    wrapping_add(map_height_pixels, kVerticalSpawnOffset);
            } else {
                motion.world_y = kTopSpawnY;
            }

            if (configuration.spawn_direction == 0U) {
                motion.velocity_x =
                    static_cast<i32>(random_bounded(random, 1U, result)) - 2;
                motion.velocity_y =
                    static_cast<i32>(random_bounded(random, 1U, result)) - 2;
            } else if (configuration.spawn_direction == 2U) {
                motion.velocity_x =
                    static_cast<i32>(random_bounded(random, 2U, result)) - 2;
                motion.velocity_y =
                    static_cast<i32>(random_bounded(random, 2U, result)) - 2;
            } else {
                motion.velocity_x =
                    static_cast<i32>(random_bounded(random, 2U, result));
                motion.velocity_y =
                    static_cast<i32>(random_bounded(random, 2U, result));
            }
            ++result.respawned_slot_count;
            continue;
        }

        timing.frame_counter = wrapping_add(timing.frame_counter, 1);
        if (timing.frame_counter > timing.current_interval) {
            timing.frame_counter = 0;
            const i32 player_motion_x = arithmetic_shift_right_two(
                wrapping_multiply(frame.movement_scale, frame.player_delta_x)
            );
            const i32 player_motion_y = arithmetic_shift_right_two(
                wrapping_multiply(frame.movement_scale, frame.player_delta_y)
            );
            motion.world_x = wrapping_add(
                wrapping_add(motion.world_x, motion.velocity_x), player_motion_x
            );
            motion.world_y = wrapping_add(
                wrapping_add(motion.world_y, motion.velocity_y), player_motion_y
            );
            ++result.moved_slot_count;

            if (frame_roll < 25U) {
                timing.target_interval =
                    static_cast<i32>(random_bounded(random, 5U, result) + 1U);
            }
            timing.current_interval =
                move_toward(timing.current_interval, timing.target_interval);

            if (frame_roll < 5U) {
                color.target_offset =
                    static_cast<i32>(random_bounded(random, 4U, result)) - 5;
            }
            color.current_offset =
                move_toward(color.current_offset, color.target_offset);
        }

        shared_action_record.action_id = kLegacyAniDirectionalActionId;
        shared_action_record.base_variant = std::bit_cast<u32>(timing.variant);
        ++result.action_update_count;
        if (ports.update_action_record(shared_action_record) !=
            LegacyActionUpdateStatus::completed) {
            result.status = LegacyAniDirectionalStatus::action_update_failed;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }

        rendering::LegacyFramePiece piece;
        ++result.frame_request_count;
        if (!ports.load_frame_piece(
                shared_action_record.field_4a,
                shared_action_record.field_4c,
                piece
            )) {
            result.status = LegacyAniDirectionalStatus::frame_load_failed;
            result.failed_slot = static_cast<u32>(index);
            return result;
        }

        const u32 flags = configuration.variant_count == 4U ? 4U : 0x2CU;
        const i32 destination_x = wrapping_subtract(
            wrapping_subtract(
                motion.world_x, field_as_i32(shared_action_record.draw_offset_x)
            ),
            frame.camera_x
        );
        const i32 destination_y = wrapping_subtract(
            wrapping_subtract(
                motion.world_y, field_as_i32(shared_action_record.draw_offset_y)
            ),
            frame.camera_y
        );
        result.last_blit_status = ports.draw_frame_piece(
            piece,
            destination_x,
            destination_y,
            flags,
            static_cast<i32>(shared_action_record.field_8a),
            color.current_offset
        );
        ++result.draw_count;
        if (!accepted_blit_status(result.last_blit_status)) {
            ++result.blit_failure_count;
        }
    }
    return result;
}

LegacyAniDirectionalState& LegacyAniDirectionalEffect::state() noexcept {
    return state_;
}

const LegacyAniDirectionalState&
LegacyAniDirectionalEffect::state() const noexcept {
    return state_;
}

}  // namespace openswd3::asset_runtime
