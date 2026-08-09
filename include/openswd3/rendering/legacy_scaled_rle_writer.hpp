#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <cstddef>
#include <span>

namespace openswd3::rendering {

struct LegacyScaledRleSource {
    std::span<const compat::u8> bytes{};
    std::size_t row_stream_offset{8U};
};

struct LegacyScaledRleTransform {
    compat::i32 anchor_x{40};
    compat::i32 anchor_y{72};
    compat::i32 horizontal_step_10_10{0x600};
    compat::i32 vertical_step_10_10{0x400};
};

struct LegacyScaledRleRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 source_width{};
    compat::i32 source_height{};
};

enum class LegacyScaledRleWriteStatus : compat::u8 {
    completed,
    clipped_out,
    malformed_source,
    invalid_geometry,
    destination_out_of_bounds,
};

// sub_422C70: left-to-right two-dimensional 10.10 RLE scaling.
[[nodiscard]] LegacyScaledRleWriteStatus write_legacy_scaled_rle_forward(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const LegacyScaledRleRequest& request,
    const LegacyScaledRleTransform& transform
) noexcept;

// sub_423020: horizontally mirrored right-to-left variant.
[[nodiscard]] LegacyScaledRleWriteStatus write_legacy_scaled_rle_reverse(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const LegacyScaledRleRequest& request,
    const LegacyScaledRleTransform& transform
) noexcept;

}  // namespace openswd3::rendering
