#include "openswd3/asset_runtime/legacy_ani_streak_effect.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <span>

namespace openswd3::asset_runtime {
namespace {

constexpr compat::u32 kTriggerRandomBound = 1000U;
constexpr compat::u32 kTriggerThreshold = 900U;
constexpr compat::u32 kInitialXRandomBound = 640U;
constexpr compat::u32 kHorizontalStepRandomBound = 3U;
constexpr compat::u32 kVerticalStepRandomBound = 30U;
constexpr compat::u32 kVerticalStepMinimum = 16U;
constexpr compat::u32 kTrailLimitRandomBound = 16U;
constexpr compat::u32 kTrailLimitMinimum = 8U;
constexpr compat::i16 kScreenCenterFixedX = 0x1400;
constexpr compat::i16 kShortLifetime = 0x10;
constexpr compat::i16 kLongLifetime = 0x20;

[[nodiscard]] constexpr compat::i16 wrapping_add_i16(
    const compat::i16 left,
    const compat::i16 right
) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u16>(left) +
        static_cast<compat::u16>(right)
    ));
}

[[nodiscard]] constexpr compat::i16 wrapping_multiply_i16(
    const compat::i16 left,
    const compat::i16 right
) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u32>(static_cast<compat::u16>(left)) *
        static_cast<compat::u32>(static_cast<compat::u16>(right))
    ));
}

[[nodiscard]] constexpr bool active(
    const LegacyAniStreakSlot& slot
) noexcept {
    return (static_cast<compat::u16>(slot.active_flags) & 1U) != 0U;
}

void initialize_slot(
    LegacyAniStreakSlot& slot,
    input_time_rng::LegacySecondaryRng& random
) noexcept {
    slot.fixed_x = static_cast<compat::i16>(
        random.next_bounded(kInitialXRandomBound) << 4U
    );
    slot.fixed_y = 0;

    compat::i32 horizontal = static_cast<compat::i32>(
        random.next_bounded(kHorizontalStepRandomBound)
    );
    if (slot.fixed_x > kScreenCenterFixedX) {
        horizontal = -horizontal;
    }
    slot.horizontal_step = static_cast<compat::i16>(horizontal);

    slot.vertical_step = static_cast<compat::i16>(
        random.next_bounded(kVerticalStepRandomBound) +
        kVerticalStepMinimum
    );
    slot.trail_limit = static_cast<compat::i16>(
        random.next_bounded(kTrailLimitRandomBound) +
        kTrailLimitMinimum
    );
    slot.remaining_frames = kShortLifetime;
    if (slot.vertical_step > 0x0F) {
        slot.remaining_frames = kLongLifetime;
    }
    slot.field_c = 0;
    slot.active_flags = 1;
}

}  // namespace

LegacyAniStreakEffect::LegacyAniStreakEffect() noexcept {
    reset();
}

void LegacyAniStreakEffect::reset() noexcept {
    std::fill_n(
        state_.slots.begin(),
        kLegacyAniStreakResetSlotCount,
        LegacyAniStreakSlot{}
    );
    state_.previous_live_count = 0;
    state_.target_spawn_count = 0;
}

LegacyAniStreakResult LegacyAniStreakEffect::update(
    input_time_rng::LegacySecondaryRng& random,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& pixel_format,
    LegacyAniStreakServicePort& services
) noexcept {
    LegacyAniStreakResult result;
    if (framebuffer.physical_byte_size() <
        rendering::kLegacyFixedCanvasBytes) {
        result.status = LegacyAniStreakStatus::framebuffer_too_small;
        return result;
    }

    if (random.next_bounded(kTriggerRandomBound) > kTriggerThreshold) {
        ++result.service_query_count;
        if (services.service_enabled(kLegacyAniStreakServiceId)) {
            state_.target_spawn_count = wrapping_add_i16(
                state_.target_spawn_count, 1
            );
            if (state_.target_spawn_count >
                kLegacyAniStreakTargetMaximum) {
                state_.target_spawn_count = kLegacyAniStreakTargetMaximum;
            }
        } else {
            state_.target_spawn_count = wrapping_add_i16(
                state_.target_spawn_count, -1
            );
            if (state_.target_spawn_count < 0) {
                state_.target_spawn_count = 0;
            }
        }
    }

    if (state_.target_spawn_count == 0 &&
        state_.previous_live_count == 0) {
        return result;
    }

    result.scanned_slots = true;
    state_.previous_live_count = 0;
    compat::i32 created_this_frame = 0;
    const compat::i32 pitch_words =
        framebuffer.geometry().surface.pitch_bytes >> 1;
    const std::int64_t physical_row_step =
        static_cast<std::int64_t>(pitch_words) * 2;
    const auto pixels = framebuffer.physical_pixels();

    for (LegacyAniStreakSlot& slot : state_.slots) {
        if (!active(slot)) {
            if (created_this_frame >= state_.target_spawn_count) {
                continue;
            }
            initialize_slot(slot, random);
            ++created_this_frame;
            ++result.created_count;
            continue;
        }

        ++result.visited_active_count;
        std::int64_t row_byte_offset =
            static_cast<std::int64_t>(slot.fixed_y) *
            physical_row_step;
        compat::i32 fixed_x = slot.fixed_x;
        compat::i32 intensity = 0;

        if (slot.trail_limit >= 0) {
            for (compat::i32 trail = 0;
                 trail <= slot.trail_limit; ++trail) {
                if (row_byte_offset > 0 &&
                    row_byte_offset <
                        rendering::kLegacyFixedCanvasBytes) {
                    static_cast<void>(rendering::legacy_pack_color_pair(
                        pixel_format, intensity, intensity, intensity
                    ));
                    ++result.packed_color_count;

                    const std::int64_t pixel_index =
                        row_byte_offset / 2 + fixed_x / 16;
                    if (pixel_index < 0 ||
                        pixel_index + 1 >=
                            static_cast<std::int64_t>(pixels.size())) {
                        ++result.pixel_failure_count;
                    } else {
                        const auto pixel_span = pixels.subspan(
                            static_cast<std::size_t>(pixel_index)
                        );
                        const auto status =
                            rendering::adjust_legacy_rgb_channels(
                                pixel_span,
                                1,
                                intensity,
                                intensity,
                                intensity,
                                pixel_format
                            );
                        if (status == rendering::
                                LegacyFrameColorStatus::completed) {
                            ++result.adjusted_pixel_count;
                        } else {
                            ++result.pixel_failure_count;
                        }
                    }
                }

                if (row_byte_offset >=
                    rendering::kLegacyFixedCanvasBytes) {
                    slot.active_flags = 0;
                }
                fixed_x += slot.horizontal_step;
                row_byte_offset += physical_row_step;
                ++intensity;
                if (slot.remaining_frames == kLongLifetime) {
                    ++intensity;
                }
            }
        }

        slot.fixed_x = wrapping_add_i16(
            slot.fixed_x,
            wrapping_multiply_i16(
                slot.horizontal_step, slot.vertical_step
            )
        );
        slot.fixed_y = wrapping_add_i16(
            slot.fixed_y, slot.vertical_step
        );
        slot.remaining_frames = wrapping_add_i16(
            slot.remaining_frames, -1
        );
        if (slot.remaining_frames == 0) {
            slot.active_flags = 0;
        } else {
            state_.previous_live_count = wrapping_add_i16(
                state_.previous_live_count, 1
            );
        }
    }

    return result;
}

LegacyAniStreakState& LegacyAniStreakEffect::state() noexcept {
    return state_;
}

const LegacyAniStreakState& LegacyAniStreakEffect::state() const noexcept {
    return state_;
}

}  // namespace openswd3::asset_runtime
