#include "openswd3/asset_runtime/legacy_ani_spark_effect.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <span>

namespace openswd3::asset_runtime {
namespace {

constexpr compat::u32 kTriggerRandomBound = 1000U;
constexpr compat::u32 kTriggerThreshold = 900U;
constexpr compat::u32 kInitialXRandomBound = 640U;
constexpr compat::u32 kHorizontalStepRandomBound = 3U;
constexpr compat::u32 kVerticalStepRandomBound = 3U;
constexpr compat::u32 kRemainingRandomBound = 160U;
constexpr compat::i16 kScreenCenterFixedX = 0x1400;
constexpr compat::i16 kInitialPointCount = 1;
constexpr compat::i16 kMaximumPhase = 0x1C;
constexpr std::int64_t kFirstExcludedRowOffset = 0x500;
constexpr std::int64_t kLastExcludedRowOffset = 0x95B00;
constexpr std::array<std::int8_t, 8> kPhaseOffsets{
    0,
    1,
    1,
    1,
    0,
    -1,
    -1,
    -1,
};

[[nodiscard]] constexpr compat::i16
wrapping_add_i16(const compat::i16 left, const compat::i16 right) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u16>(left) + static_cast<compat::u16>(right)
    ));
}

[[nodiscard]] constexpr compat::i16 wrapping_subtract_i16(
    const compat::i16 left, const compat::i16 right
) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u16>(left) - static_cast<compat::u16>(right)
    ));
}

[[nodiscard]] constexpr compat::i16 wrapping_multiply_i16(
    const compat::i16 left, const compat::i16 right
) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u32>(static_cast<compat::u16>(left)) *
        static_cast<compat::u32>(static_cast<compat::u16>(right))
    ));
}

[[nodiscard]] constexpr bool active(const LegacyAniSparkSlot& slot) noexcept {
    return (static_cast<compat::u16>(slot.active_flags) & 1U) != 0U;
}

void initialize_slot(
    LegacyAniSparkSlot& slot, input_time_rng::LegacySecondaryRng& random
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
        random.next_bounded(kVerticalStepRandomBound) + 1U
    );
    slot.point_count = kInitialPointCount;
    slot.remaining_height = static_cast<compat::i16>(
        static_cast<compat::i32>(slot.vertical_step) * 160 -
        static_cast<compat::i32>(random.next_bounded(kRemainingRandomBound))
    );
    slot.phase = 0;
    slot.active_flags = 1;
}

[[nodiscard]] compat::i32
intensity_for_height(const compat::i16 remaining_height) noexcept {
    const compat::i32 intensity =
        31 - (480 - static_cast<compat::i32>(remaining_height)) / 6;
    if (static_cast<compat::u32>(intensity) > 1000U) {
        return 0;
    }
    return intensity;
}

[[nodiscard]] bool adjust_pixel(
    const std::span<compat::u16> pixels,
    const std::int64_t pixel_index,
    const compat::i32 intensity,
    const rendering::LegacyPixelConversionState& pixel_format,
    LegacyAniSparkResult& result
) noexcept {
    if (pixel_index < 0 ||
        pixel_index + 1 >= static_cast<std::int64_t>(pixels.size())) {
        ++result.pixel_failure_count;
        return false;
    }

    const auto status = rendering::adjust_legacy_rgb_channels(
        pixels.subspan(static_cast<std::size_t>(pixel_index)),
        1,
        intensity,
        intensity,
        intensity,
        pixel_format
    );
    if (status != rendering::LegacyFrameColorStatus::completed) {
        ++result.pixel_failure_count;
        return false;
    }

    ++result.adjusted_pixel_count;
    return true;
}

}  // namespace

void LegacyAniSparkEffect::reset_counters() noexcept {
    state_.previous_live_count = 0;
    state_.target_spawn_count = 0;
}

LegacyAniSparkResult LegacyAniSparkEffect::update(
    input_time_rng::LegacySecondaryRng& random,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& pixel_format,
    LegacyAniSparkServicePort& services
) noexcept {
    LegacyAniSparkResult result;
    if (framebuffer.physical_byte_size() < rendering::kLegacyFixedCanvasBytes) {
        result.status = LegacyAniSparkStatus::framebuffer_too_small;
        return result;
    }

    if (random.next_bounded(kTriggerRandomBound) > kTriggerThreshold) {
        ++result.service_query_count;
        if (services.service_enabled(kLegacyAniSparkServiceId)) {
            state_.target_spawn_count =
                wrapping_add_i16(state_.target_spawn_count, 1);
            if (state_.target_spawn_count > kLegacyAniSparkTargetMaximum) {
                state_.target_spawn_count = kLegacyAniSparkTargetMaximum;
            }
        } else {
            state_.target_spawn_count =
                wrapping_add_i16(state_.target_spawn_count, -1);
            if (state_.target_spawn_count < 0) {
                state_.target_spawn_count = 0;
            }
        }
    }

    if (state_.target_spawn_count == 0 && state_.previous_live_count == 0) {
        return result;
    }

    result.scanned_slots = true;
    state_.previous_live_count = 0;
    compat::i32 created_this_frame = 0;
    const compat::i32 pitch_words =
        framebuffer.geometry().surface.pitch_bytes >> 1;
    const std::int64_t row_step_bytes =
        static_cast<std::int64_t>(pitch_words) * 2;
    const auto pixels = framebuffer.physical_pixels();

    for (LegacyAniSparkSlot& slot : state_.slots) {
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
        const compat::i32 phase_group =
            static_cast<compat::i32>(slot.phase) / 4;
        compat::i32 phase_offset = 0;
        if (phase_group < 0 ||
            phase_group >= static_cast<compat::i32>(kPhaseOffsets.size())) {
            ++result.invalid_phase_count;
        } else {
            phase_offset = kPhaseOffsets[static_cast<std::size_t>(phase_group)];
        }

        compat::i32 fixed_x = slot.fixed_x;
        std::int64_t row_byte_offset =
            static_cast<std::int64_t>(slot.fixed_y) * row_step_bytes;
        compat::i32 intensity = intensity_for_height(slot.remaining_height);

        if (slot.point_count > 0) {
            for (compat::i32 point = 0; point < slot.point_count; ++point) {
                if (row_byte_offset > kFirstExcludedRowOffset &&
                    row_byte_offset < kLastExcludedRowOffset) {
                    static_cast<void>(rendering::legacy_pack_color_pair(
                        pixel_format, intensity, intensity, intensity
                    ));
                    ++result.packed_color_count;

                    const std::int64_t center =
                        row_byte_offset / 2 + fixed_x / 16 + phase_offset;
                    const compat::i32 half_intensity = intensity >> 1;
                    const compat::i32 quarter_intensity = intensity >> 2;
                    static_cast<void>(adjust_pixel(
                        pixels, center, intensity, pixel_format, result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels, center + 1, half_intensity, pixel_format, result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels, center - 1, half_intensity, pixel_format, result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels,
                        center + pitch_words,
                        half_intensity,
                        pixel_format,
                        result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels,
                        center - pitch_words,
                        half_intensity,
                        pixel_format,
                        result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels,
                        center + pitch_words + 1,
                        quarter_intensity,
                        pixel_format,
                        result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels,
                        center + pitch_words - 1,
                        quarter_intensity,
                        pixel_format,
                        result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels,
                        center - pitch_words + 1,
                        quarter_intensity,
                        pixel_format,
                        result
                    ));
                    static_cast<void>(adjust_pixel(
                        pixels,
                        center - pitch_words - 1,
                        quarter_intensity,
                        pixel_format,
                        result
                    ));
                }

                if (row_byte_offset >= kLastExcludedRowOffset) {
                    slot.active_flags = 0;
                }
                fixed_x += slot.horizontal_step;
                row_byte_offset += row_step_bytes;
                ++intensity;
                if (slot.remaining_height == 0x20) {
                    ++intensity;
                }
            }
        }

        slot.fixed_x = wrapping_add_i16(
            slot.fixed_x,
            wrapping_multiply_i16(slot.horizontal_step, slot.vertical_step)
        );
        slot.fixed_y = wrapping_add_i16(slot.fixed_y, slot.vertical_step);
        slot.phase = wrapping_add_i16(slot.phase, 1);
        if (slot.phase > kMaximumPhase) {
            slot.phase = 0;
        }
        slot.remaining_height =
            wrapping_subtract_i16(slot.remaining_height, slot.vertical_step);
        if (slot.remaining_height <= 0) {
            slot.active_flags = 0;
        } else {
            state_.previous_live_count =
                wrapping_add_i16(state_.previous_live_count, 1);
        }
    }

    return result;
}

LegacyAniSparkState& LegacyAniSparkEffect::state() noexcept {
    return state_;
}

const LegacyAniSparkState& LegacyAniSparkEffect::state() const noexcept {
    return state_;
}

}  // namespace openswd3::asset_runtime
