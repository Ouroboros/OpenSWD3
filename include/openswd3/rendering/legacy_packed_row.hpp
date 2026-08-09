#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <span>

namespace openswd3::rendering {

enum class LegacyPackedRowBlendStatus : compat::u8 {
    completed,
    invalid_geometry,
    destination_out_of_bounds,
};

// sub_417DE0. The destination span begins at the original pointer argument.
// The helper processes floor(pixel_count / 2) packed pairs and leaves an odd
// final pixel unchanged.
[[nodiscard]] LegacyPackedRowBlendStatus blend_legacy_packed_row(
    std::span<compat::u16> destination,
    compat::u32 color_pattern,
    compat::i32 pixel_count,
    const LegacyPixelConversionState& format
) noexcept;

}  // namespace openswd3::rendering
