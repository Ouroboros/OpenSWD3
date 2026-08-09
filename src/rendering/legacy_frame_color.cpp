#include "openswd3/rendering/legacy_frame_color.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::rendering {
namespace {

[[nodiscard]] constexpr compat::u32 wrapped_product(
    const compat::i32 delta,
    const compat::u32 unit
) noexcept {
    return std::bit_cast<compat::u32>(delta) * unit;
}

[[nodiscard]] constexpr compat::u32 adjusted_channel(
    const compat::u32 loaded,
    const compat::u32 mask,
    const compat::u32 wrapped_delta,
    const bool nonnegative
) noexcept {
    const compat::u32 candidate =
        (loaded & mask) + wrapped_delta;
    if ((candidate & ~mask) != 0U) {
        return nonnegative ? mask : 0U;
    }
    return candidate;
}

[[nodiscard]] constexpr compat::u32 read_pair(
    const std::span<const compat::u16> pixels,
    const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(pixels[offset]) |
        (static_cast<compat::u32>(pixels[offset + 1U]) << 16U);
}

void write_pair(
    const std::span<compat::u16> pixels,
    const std::size_t offset,
    const compat::u32 value
) noexcept {
    pixels[offset] = static_cast<compat::u16>(value);
    pixels[offset + 1U] = static_cast<compat::u16>(value >> 16U);
}

[[nodiscard]] LegacyFrameColorStatus adjust_legacy_channel_pairs(
    const std::span<compat::u16> pixels,
    const compat::i32 pixel_count,
    const compat::i32 delta,
    const compat::u32 mask,
    const compat::u32 unit
) noexcept {
    if (pixel_count <= 0 || (pixel_count & 1) != 0) {
        return LegacyFrameColorStatus::invalid_count;
    }

    const auto count = static_cast<std::size_t>(pixel_count);
    if (count > pixels.size()) {
        return LegacyFrameColorStatus::buffer_out_of_bounds;
    }

    const compat::u32 pair_mask = mask | (mask << 16U);
    const compat::u32 outside_pair_mask = ~pair_mask;
    const compat::u32 low_outside_mask = outside_pair_mask & 0xFFFFU;
    const compat::u32 high_outside_mask = low_outside_mask << 16U;
    const bool nonnegative = delta >= 0;
    const compat::u32 raw_delta = wrapped_product(delta, unit);
    const compat::u32 magnitude = nonnegative ? raw_delta : 0U - raw_delta;
    const compat::u32 pair_delta = magnitude | (magnitude << 16U);

    for (std::size_t offset = 0U; offset < count; offset += 2U) {
        const compat::u32 loaded = read_pair(pixels, offset);
        compat::u32 candidate = nonnegative
            ? (loaded & pair_mask) + pair_delta
            : (loaded & pair_mask) - pair_delta;

        if ((candidate & low_outside_mask) != 0U) {
            candidate = (candidate & 0xFFFF0000U) |
                (nonnegative ? mask : 0U);
        }
        if ((candidate & high_outside_mask) != 0U) {
            candidate = (candidate & 0x0000FFFFU) |
                (nonnegative ? (mask << 16U) : 0U);
        }

        write_pair(
            pixels,
            offset,
            (loaded & outside_pair_mask) | candidate
        );
    }

    return LegacyFrameColorStatus::completed;
}

[[nodiscard]] constexpr compat::u32 combined_channel(
    const compat::u16 source,
    const compat::u16 destination,
    const compat::u32 mask,
    const compat::u32 shift
) noexcept {
    const compat::u32 index =
        ((static_cast<compat::u32>(source) & mask) +
         (static_cast<compat::u32>(destination) & mask)) >> shift;
    return index < 32U ? index << shift : 0U;
}

}  // namespace

LegacyFrameColorStatus adjust_legacy_rgb_channels(
    const std::span<compat::u16> pixels,
    const compat::i32 pixel_count,
    const compat::i32 red_delta,
    const compat::i32 green_delta,
    const compat::i32 blue_delta,
    const LegacyPixelConversionState& format
) noexcept {
    if (pixel_count <= 0) {
        return LegacyFrameColorStatus::invalid_count;
    }

    const auto count = static_cast<std::size_t>(pixel_count);
    if (count >= pixels.size()) {
        return LegacyFrameColorStatus::buffer_out_of_bounds;
    }

    const compat::u32 red_delta_field = wrapped_product(
        red_delta,
        1U << format.red_shift
    );
    const compat::u32 green_delta_field = wrapped_product(
        green_delta,
        1U << format.green_shift
    );
    const compat::u32 blue_delta_field = wrapped_product(
        blue_delta,
        1U << format.blue_shift
    );

    for (std::size_t index = 0U; index < count; ++index) {
        const compat::u32 loaded = read_pair(pixels, index);
        const compat::u32 red = adjusted_channel(
            loaded,
            format.effective_masks.red,
            red_delta_field,
            red_delta >= 0
        );
        const compat::u32 green = adjusted_channel(
            loaded,
            format.effective_masks.green,
            green_delta_field,
            green_delta >= 0
        );
        const compat::u32 blue = adjusted_channel(
            loaded,
            format.effective_masks.blue,
            blue_delta_field,
            blue_delta >= 0
        );
        pixels[index] = static_cast<compat::u16>(red | green | blue);
    }

    return LegacyFrameColorStatus::completed;
}

LegacyFrameColorStatus adjust_legacy_red_channel(
    const std::span<compat::u16> pixels,
    const compat::i32 pixel_count,
    const compat::i32 delta,
    const LegacyPixelConversionState& format
) noexcept {
    if (pixel_count <= 0) {
        return LegacyFrameColorStatus::invalid_count;
    }

    const auto count = static_cast<std::size_t>(pixel_count);
    if (count >= pixels.size()) {
        return LegacyFrameColorStatus::buffer_out_of_bounds;
    }

    const compat::u32 mask = format.effective_masks.red;
    const compat::u32 outside_mask = ~mask;
    const bool nonnegative = delta >= 0;
    const compat::u32 raw_delta = wrapped_product(
        delta,
        1U << format.red_shift
    );
    const compat::u32 magnitude = nonnegative ? raw_delta : 0U - raw_delta;

    for (std::size_t index = 0U; index < count; ++index) {
        const compat::u32 loaded = read_pair(pixels, index);
        compat::u32 candidate = nonnegative
            ? (loaded & mask) + magnitude
            : (loaded & mask) - magnitude;
        if ((candidate & outside_mask) != 0U) {
            candidate = nonnegative ? mask : 0U;
        }
        pixels[index] = static_cast<compat::u16>(
            (loaded & outside_mask) | candidate
        );
    }

    return LegacyFrameColorStatus::completed;
}

LegacyFrameColorStatus adjust_legacy_green_channel_pairs(
    const std::span<compat::u16> pixels,
    const compat::i32 pixel_count,
    const compat::i32 delta,
    const LegacyPixelConversionState& format
) noexcept {
    return adjust_legacy_channel_pairs(
        pixels,
        pixel_count,
        delta,
        format.effective_masks.green,
        1U << format.green_shift
    );
}

LegacyFrameColorStatus adjust_legacy_blue_channel_pairs(
    const std::span<compat::u16> pixels,
    const compat::i32 pixel_count,
    const compat::i32 delta,
    const LegacyPixelConversionState& format
) noexcept {
    return adjust_legacy_channel_pairs(
        pixels,
        pixel_count,
        delta,
        format.effective_masks.blue,
        1U << format.blue_shift
    );
}

LegacyFrameColorStatus combine_legacy_channels_overflow_to_zero(
    const std::span<const compat::u16> source,
    const std::span<compat::u16> destination,
    const compat::i32 pixel_count,
    LegacyPixelConversionState& format
) noexcept {
    format.effective_masks.red &= 0xFFFFU;
    format.effective_masks.green &= 0xFFFFU;
    format.effective_masks.blue &= 0xFFFFU;

    if (pixel_count == 0) {
        return LegacyFrameColorStatus::completed;
    }
    if (pixel_count < 0) {
        return LegacyFrameColorStatus::invalid_count;
    }

    const auto count = static_cast<std::size_t>(pixel_count);
    if (count > source.size()) {
        return LegacyFrameColorStatus::source_out_of_bounds;
    }
    if (count > destination.size()) {
        return LegacyFrameColorStatus::destination_out_of_bounds;
    }

    for (std::size_t index = 0U; index < count; ++index) {
        const compat::u16 source_pixel = source[index];
        const compat::u16 destination_pixel = destination[index];
        destination[index] = static_cast<compat::u16>(
            combined_channel(
                source_pixel,
                destination_pixel,
                format.effective_masks.red,
                format.red_shift
            ) +
            combined_channel(
                source_pixel,
                destination_pixel,
                format.effective_masks.green,
                format.green_shift
            ) +
            combined_channel(
                source_pixel,
                destination_pixel,
                format.effective_masks.blue,
                format.blue_shift
            )
        );
    }

    return LegacyFrameColorStatus::completed;
}

LegacyFrameColorStatus convert_legacy_quarter_sum_grayscale(
    const std::span<compat::u16> pixels,
    const compat::i32 pixel_count,
    const LegacyPixelConversionState& format
) noexcept {
    if (pixel_count <= 0) {
        return LegacyFrameColorStatus::invalid_count;
    }

    const auto count = static_cast<std::size_t>(pixel_count);
    if (count > pixels.size()) {
        return LegacyFrameColorStatus::buffer_out_of_bounds;
    }

    for (std::size_t index = 0U; index < count; ++index) {
        const compat::u32 pixel = pixels[index];
        const compat::u32 red =
            (pixel & format.effective_masks.red) >> format.red_shift;
        const compat::u32 green =
            (pixel & format.effective_masks.green) >> format.green_shift;
        const compat::u32 blue =
            (pixel & format.effective_masks.blue) >> format.blue_shift;
        const compat::u32 quarter_sum = (red + green + blue) >> 2U;
        pixels[index] = static_cast<compat::u16>(
            (quarter_sum << format.red_shift) +
            (quarter_sum << format.green_shift) +
            (quarter_sum << format.blue_shift)
        );
    }

    return LegacyFrameColorStatus::completed;
}

}  // namespace openswd3::rendering
