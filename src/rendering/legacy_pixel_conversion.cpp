#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <bit>

namespace openswd3::rendering {
namespace {

constexpr LegacyPixelMasks kRgb555Masks{0x7C00U, 0x03E0U, 0x001FU};
constexpr LegacyPixelMasks kShiftedWholeWordMasks{
    0xF800U,
    0x07C0U,
    0x003FU,
};
constexpr LegacyPixelMasks kRgb565Masks{0xF800U, 0x07E0U, 0x001FU};
constexpr LegacyPixelMasks kShiftedRedMasks{0xFC00U, 0x03E0U, 0x001FU};

[[nodiscard]] bool masks_equal(
    const LegacyPixelMasks& left,
    const LegacyPixelMasks& right
) noexcept {
    return left.red == right.red &&
        left.green == right.green &&
        left.blue == right.blue;
}

[[nodiscard]] compat::u16 transform_pixel(
    const LegacyPixelTransform transform,
    const compat::u16 pixel
) noexcept {
    switch (transform) {
    case LegacyPixelTransform::identity:
        return pixel;

    case LegacyPixelTransform::shift_whole_word_left:
        return static_cast<compat::u16>(
            static_cast<compat::u32>(pixel) << 1U
        );

    case LegacyPixelTransform::rgb555_to_rgb565:
        return static_cast<compat::u16>(
            ((static_cast<compat::u32>(pixel) & 0xFFE0U) << 1U) +
            (static_cast<compat::u32>(pixel) & 0x001FU)
        );

    case LegacyPixelTransform::rgb565_to_rgb555:
        return static_cast<compat::u16>(
            ((static_cast<compat::u32>(pixel) >> 1U) & 0xFFE0U) +
            (static_cast<compat::u32>(pixel) & 0x001FU)
        );

    case LegacyPixelTransform::shift_red_field_left:
        return static_cast<compat::u16>(
            ((static_cast<compat::u32>(pixel) & 0x7C00U) << 1U) |
            (static_cast<compat::u32>(pixel) & 0x03FFU)
        );
    }

    return pixel;
}

}  // namespace

void select_legacy_pixel_conversion(
    LegacyPixelConversionState& state,
    const LegacyPixelMasks& masks
) noexcept {
    state.reported_masks = masks;

    if (masks_equal(masks, kRgb555Masks)) {
        state.forward = LegacyPixelTransform::identity;
        state.reverse = LegacyPixelTransform::identity;
        return;
    }

    if (masks_equal(masks, kShiftedWholeWordMasks)) {
        state.forward = LegacyPixelTransform::shift_whole_word_left;
        return;
    }

    if (masks_equal(masks, kRgb565Masks)) {
        state.forward = LegacyPixelTransform::rgb555_to_rgb565;
        state.reverse = LegacyPixelTransform::rgb565_to_rgb555;
        return;
    }

    if (masks_equal(masks, kShiftedRedMasks)) {
        state.forward = LegacyPixelTransform::shift_red_field_left;
    }
}

void apply_legacy_pixel_transform(
    const LegacyPixelTransform transform,
    compat::u16* pixels,
    const compat::i32 pixel_count
) noexcept {
    if (transform == LegacyPixelTransform::identity) {
        return;
    }

    compat::u32 remaining = std::bit_cast<compat::u32>(pixel_count);
    do {
        *pixels = transform_pixel(transform, *pixels);
        ++pixels;
        --remaining;
    } while (std::bit_cast<compat::i32>(remaining) > 0);
}

void legacy_convert_pixels_forward(
    const LegacyPixelConversionState& state,
    compat::u16* const pixels,
    const compat::i32 pixel_count
) noexcept {
    apply_legacy_pixel_transform(state.forward, pixels, pixel_count);
}

void legacy_convert_pixels_reverse(
    const LegacyPixelConversionState& state,
    compat::u16* const pixels,
    const compat::i32 pixel_count
) noexcept {
    apply_legacy_pixel_transform(state.reverse, pixels, pixel_count);
}

}  // namespace openswd3::rendering
