#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

namespace openswd3::rendering {

struct LegacyRectangleEffectRequest {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 width{};
    compat::i32 height{};
    compat::i32 red{};
    compat::i32 green{};
    compat::i32 blue{};
    compat::u32 mode{};
};

enum class LegacyRectangleEffectStatus : compat::u8 {
    completed,
    clipped_out,
    unsupported_mode,
    invalid_geometry,
    destination_out_of_bounds,
};

[[nodiscard]] LegacyRectangleEffectStatus apply_legacy_rectangle_effect(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyPixelConversionState& format,
    const LegacyRectangleEffectRequest& request
) noexcept;

}  // namespace openswd3::rendering
