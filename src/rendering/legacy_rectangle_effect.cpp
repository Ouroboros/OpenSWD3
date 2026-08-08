#include "openswd3/rendering/legacy_rectangle_effect.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <span>

namespace openswd3::rendering {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

struct ClippedRectangle {
    i32 x{};
    i32 y{};
    i32 width{};
    i32 height{};
};

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_add(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_subtract(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32 wrapping_multiply(
    const i32 left,
    const i32 right
) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] ClippedRectangle clip_rectangle(
    const LegacyRasterGeometryState& raster,
    const LegacyRectangleEffectRequest& request
) noexcept {
    ClippedRectangle clipped{
        .x = request.x,
        .y = request.y,
        .width = request.width,
        .height = request.height,
    };

    if (clipped.x < raster.clip_left) {
        const i32 displacement = wrapping_subtract(
            clipped.x,
            raster.clip_left
        );
        clipped.width = wrapping_add(clipped.width, displacement);
        clipped.x = raster.clip_left;
    }

    if (clipped.y < raster.clip_top) {
        const i32 displacement = wrapping_subtract(
            clipped.y,
            raster.clip_top
        );
        clipped.height = wrapping_add(clipped.height, displacement);
        clipped.y = raster.clip_top;
    }

    const i32 clip_right = wrapping_add(
        raster.clip_left,
        raster.clip_width
    );
    if (wrapping_add(clipped.x, clipped.width) > clip_right) {
        clipped.width = wrapping_add(
            wrapping_subtract(raster.clip_width, clipped.x),
            raster.clip_left
        );
    }

    const i32 clip_bottom = wrapping_add(
        raster.clip_top,
        raster.clip_height
    );
    if (wrapping_add(clipped.y, clipped.height) > clip_bottom) {
        clipped.height = wrapping_add(
            wrapping_subtract(raster.clip_height, clipped.y),
            raster.clip_top
        );
    }

    return clipped;
}

[[nodiscard]] constexpr u32 low_word_mask(const u32 value) noexcept {
    return value & 0xFFFFU;
}

[[nodiscard]] constexpr u32 shifted_channel_mask(
    const u32 mask,
    const u32 shift_count
) noexcept {
    const u32 base = low_word_mask(mask);
    u32 shifted = base;
    for (u32 index = 0U; index < shift_count; ++index) {
        shifted = (shifted >> 1U) & base;
    }
    return shifted;
}

[[nodiscard]] constexpr u32 duplicated_shift_mask(
    const LegacyPixelConversionState& format,
    const u32 shift_count
) noexcept {
    const u32 lane = shifted_channel_mask(
        format.effective_masks.red,
        shift_count
    ) | shifted_channel_mask(
        format.effective_masks.green,
        shift_count
    ) | shifted_channel_mask(
        format.effective_masks.blue,
        shift_count
    );
    return lane | (lane << 16U);
}

[[nodiscard]] u32 packed_effect_color(
    const LegacyPixelConversionState& format,
    const i32 red,
    const i32 green,
    const i32 blue
) noexcept {
    const u16 rgb555 = static_cast<u16>(
        ((to_bits(red) & 0x1FU) << 10U) |
        ((to_bits(green) & 0x1FU) << 5U) |
        (to_bits(blue) & 0x1FU)
    );
    std::array<u16, 2> pixels{rgb555, rgb555};
    legacy_convert_pixels_forward(format, pixels.data(), 2);
    return static_cast<u32>(pixels[0]) |
        (static_cast<u32>(pixels[1]) << 16U);
}

[[nodiscard]] constexpr u32 read_pair(
    const std::span<const u16> pixels,
    const std::size_t offset
) noexcept {
    return static_cast<u32>(pixels[offset]) |
        (static_cast<u32>(pixels[offset + 1U]) << 16U);
}

void write_pair(
    const std::span<u16> pixels,
    const std::size_t offset,
    const u32 value
) noexcept {
    pixels[offset] = static_cast<u16>(value);
    pixels[offset + 1U] = static_cast<u16>(value >> 16U);
}

[[nodiscard]] constexpr u32 adjusted_channel(
    const u16 pixel,
    const u32 raw_mask,
    const u32 shift,
    const i32 delta
) noexcept {
    const u32 mask = low_word_mask(raw_mask);
    const u32 unit = 1U << (shift & 31U);
    const u32 increment = unit * to_bits(delta);
    const u32 candidate = (static_cast<u32>(pixel) & mask) + increment;
    if (((~mask) & candidate) != 0U) {
        return delta >= 0 ? mask : 0U;
    }
    return candidate;
}

[[nodiscard]] constexpr u16 offset_pixel(
    const u16 pixel,
    const LegacyPixelConversionState& format,
    const i32 red,
    const i32 green,
    const i32 blue
) noexcept {
    return static_cast<u16>(
        adjusted_channel(
            pixel,
            format.effective_masks.red,
            format.red_shift,
            red
        ) |
        adjusted_channel(
            pixel,
            format.effective_masks.green,
            format.green_shift,
            green
        ) |
        adjusted_channel(
            pixel,
            format.effective_masks.blue,
            format.blue_shift,
            blue
        )
    );
}

[[nodiscard]] constexpr u16 grayscale_pixel(
    const u16 pixel,
    const LegacyPixelConversionState& format
) noexcept {
    u32 intensity =
        (static_cast<u32>(pixel) & low_word_mask(
            format.effective_masks.red
        )) >> (format.red_shift & 31U);
    intensity +=
        (static_cast<u32>(pixel) & low_word_mask(
            format.effective_masks.green
        )) >> (format.green_shift & 31U);
    intensity +=
        (static_cast<u32>(pixel) & low_word_mask(
            format.effective_masks.blue
        )) >> (format.blue_shift & 31U);
    intensity >>= 2U;

    return static_cast<u16>(
        (intensity << (format.red_shift & 31U)) +
        (intensity << (format.green_shift & 31U)) +
        (intensity << (format.blue_shift & 31U))
    );
}

void offset_row(
    const std::span<u16> pixels,
    const std::size_t row_offset,
    const i32 width,
    const LegacyPixelConversionState& format,
    const i32 red,
    const i32 green,
    const i32 blue
) noexcept {
    for (i32 column = 0; column < width; ++column) {
        const std::size_t offset = row_offset +
            static_cast<std::size_t>(column);
        pixels[offset] = offset_pixel(
            pixels[offset],
            format,
            red,
            green,
            blue
        );
    }
}

[[nodiscard]] bool destination_is_addressable(
    const std::span<const u16> pixels,
    const ClippedRectangle& rectangle,
    const i32 pitch_words
) noexcept {
    if (pitch_words <= 0 ||
        rectangle.x < 0 ||
        rectangle.y < 0 ||
        rectangle.width <= 0 ||
        rectangle.height <= 0) {
        return false;
    }

    const auto pitch = static_cast<std::size_t>(pitch_words);
    if (pixels.size() % pitch != 0U) {
        return false;
    }

    const auto x = static_cast<std::size_t>(rectangle.x);
    const auto y = static_cast<std::size_t>(rectangle.y);
    const auto width = static_cast<std::size_t>(rectangle.width);
    const auto height = static_cast<std::size_t>(rectangle.height);
    const std::size_t physical_rows = pixels.size() / pitch;
    return x <= pitch &&
        width <= pitch - x &&
        y <= physical_rows &&
        height <= physical_rows - y;
}

[[nodiscard]] constexpr i32 fixed_increment(
    const i32 component,
    const i32 pair_count
) noexcept {
    return wrapping_multiply(component, 0x400) / pair_count;
}

}  // namespace

LegacyRectangleEffectStatus apply_legacy_rectangle_effect(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyPixelConversionState& format,
    const LegacyRectangleEffectRequest& request
) noexcept {
    const ClippedRectangle rectangle = clip_rectangle(raster, request);
    if (rectangle.width <= 0 || rectangle.height <= 0) {
        return LegacyRectangleEffectStatus::clipped_out;
    }

    const u32 color = packed_effect_color(
        format,
        request.red,
        request.green,
        request.blue
    );
    if (request.mode > 5U) {
        return LegacyRectangleEffectStatus::unsupported_mode;
    }

    if (raster.surface.pitch_bytes <= 0 ||
        (raster.surface.pitch_bytes & 1) != 0 ||
        (request.mode == 0U && rectangle.width < 2)) {
        return LegacyRectangleEffectStatus::invalid_geometry;
    }

    const i32 pitch_words = raster.surface.pitch_bytes / 2;
    std::span<u16> pixels = framebuffer.physical_pixels();
    if (!destination_is_addressable(
            pixels,
            rectangle,
            pitch_words
        )) {
        return LegacyRectangleEffectStatus::destination_out_of_bounds;
    }

    const std::size_t first_offset =
        static_cast<std::size_t>(rectangle.y) *
            static_cast<std::size_t>(pitch_words) +
        static_cast<std::size_t>(rectangle.x);

    switch (request.mode) {
    case 0U: {
        const u32 mask_1 = duplicated_shift_mask(format, 1U);
        const u32 mask_2 = duplicated_shift_mask(format, 2U);
        const u32 mask_3 = duplicated_shift_mask(format, 3U);
        const u32 mask_4 = duplicated_shift_mask(format, 4U);
        const i32 pair_count = rectangle.width / 2;
        for (i32 row = 0; row < rectangle.height; ++row) {
            const std::size_t row_offset = first_offset +
                static_cast<std::size_t>(row) *
                    static_cast<std::size_t>(pitch_words);
            for (i32 pair = 0; pair < pair_count; ++pair) {
                const std::size_t offset = row_offset +
                    static_cast<std::size_t>(pair) * 2U;
                const u32 destination = read_pair(pixels, offset);
                const u32 output =
                    ((destination >> 1U) & mask_1) +
                    ((color >> 2U) & mask_2) +
                    ((destination >> 3U) & mask_3) +
                    ((destination >> 4U) & mask_4);
                write_pair(pixels, offset, output);
            }
        }
        break;
    }

    case 1U:
        for (i32 row = 0; row < rectangle.height; ++row) {
            offset_row(
                pixels,
                first_offset + static_cast<std::size_t>(row) *
                    static_cast<std::size_t>(pitch_words),
                rectangle.width,
                format,
                request.red,
                request.green,
                request.blue
            );
        }
        break;

    case 2U:
    case 4U: {
        const u32 shift = request.mode == 2U ? 2U : 3U;
        const u32 mask = duplicated_shift_mask(format, shift);
        const i32 pair_count = rectangle.width / 2;
        for (i32 row = 0; row < rectangle.height; ++row) {
            const std::size_t row_offset = first_offset +
                static_cast<std::size_t>(row) *
                    static_cast<std::size_t>(pitch_words);
            for (i32 pair = 0; pair < pair_count; ++pair) {
                const std::size_t offset = row_offset +
                    static_cast<std::size_t>(pair) * 2U;
                write_pair(
                    pixels,
                    offset,
                    (read_pair(pixels, offset) >> shift) & mask
                );
            }
        }
        break;
    }

    case 3U:
        for (i32 row = 0; row < rectangle.height; ++row) {
            const std::size_t row_offset = first_offset +
                static_cast<std::size_t>(row) *
                    static_cast<std::size_t>(pitch_words);
            for (i32 column = 0; column < rectangle.width; ++column) {
                const std::size_t offset = row_offset +
                    static_cast<std::size_t>(column);
                pixels[offset] = grayscale_pixel(pixels[offset], format);
            }
        }
        break;

    case 5U: {
        const i32 pair_count = (rectangle.height + 1) / 2;
        const i32 red_increment = fixed_increment(request.red, pair_count);
        const i32 green_increment = fixed_increment(
            request.green,
            pair_count
        );
        const i32 blue_increment = fixed_increment(request.blue, pair_count);
        i32 red_fixed{};
        i32 green_fixed{};
        i32 blue_fixed{};
        std::size_t top_offset = first_offset;
        std::size_t bottom_offset = first_offset +
            static_cast<std::size_t>(rectangle.height - 1) *
                static_cast<std::size_t>(pitch_words);

        for (i32 pair = 0; pair < pair_count; ++pair) {
            const i32 red = red_fixed / 0x400;
            const i32 green = green_fixed / 0x400;
            const i32 blue = blue_fixed / 0x400;
            offset_row(
                pixels,
                top_offset,
                rectangle.width,
                format,
                red,
                green,
                blue
            );
            offset_row(
                pixels,
                bottom_offset,
                rectangle.width,
                format,
                red,
                green,
                blue
            );

            top_offset += static_cast<std::size_t>(pitch_words);
            bottom_offset -= static_cast<std::size_t>(pitch_words);
            red_fixed = wrapping_add(red_fixed, red_increment);
            green_fixed = wrapping_add(green_fixed, green_increment);
            blue_fixed = wrapping_add(blue_fixed, blue_increment);
        }
        break;
    }

    default:
        break;
    }

    return LegacyRectangleEffectStatus::completed;
}

}  // namespace openswd3::rendering
