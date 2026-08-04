#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::rendering {

struct LegacyPixelMasks {
    compat::u32 red{};
    compat::u32 green{};
    compat::u32 blue{};
};

enum class LegacyPixelTransform {
    identity,
    shift_whole_word_left,
    rgb555_to_rgb565,
    rgb565_to_rgb555,
    shift_red_field_left,
};

struct LegacyPixelConversionState {
    LegacyPixelMasks reported_masks{};
    LegacyPixelTransform forward{LegacyPixelTransform::identity};
    LegacyPixelTransform reverse{LegacyPixelTransform::identity};
};

void select_legacy_pixel_conversion(
    LegacyPixelConversionState& state,
    const LegacyPixelMasks& masks
) noexcept;

void apply_legacy_pixel_transform(
    LegacyPixelTransform transform,
    compat::u16* pixels,
    compat::i32 pixel_count
) noexcept;

void legacy_convert_pixels_forward(
    const LegacyPixelConversionState& state,
    compat::u16* pixels,
    compat::i32 pixel_count
) noexcept;

void legacy_convert_pixels_reverse(
    const LegacyPixelConversionState& state,
    compat::u16* pixels,
    compat::i32 pixel_count
) noexcept;

}  // namespace openswd3::rendering
