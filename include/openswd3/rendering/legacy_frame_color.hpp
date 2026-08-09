#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <span>

namespace openswd3::rendering {

enum class LegacyFrameColorStatus : compat::u8 {
    completed,
    invalid_count,
    buffer_out_of_bounds,
    source_out_of_bounds,
    destination_out_of_bounds,
};

// sub_420490. The original performs a dword read for every logical u16 pixel,
// so the span must include one readable look-ahead pixel after the range.
[[nodiscard]] LegacyFrameColorStatus adjust_legacy_rgb_channels(
    std::span<compat::u16> pixels,
    compat::i32 pixel_count,
    compat::i32 red_delta,
    compat::i32 green_delta,
    compat::i32 blue_delta,
    const LegacyPixelConversionState& format
) noexcept;

// sub_420560. This has the same final dword-read look-ahead contract.
[[nodiscard]] LegacyFrameColorStatus adjust_legacy_red_channel(
    std::span<compat::u16> pixels,
    compat::i32 pixel_count,
    compat::i32 delta,
    const LegacyPixelConversionState& format
) noexcept;

// sub_420600/sub_4206F0. These helpers preserve the original two-u16 packed
// dword arithmetic and therefore accept only the positive even count domain
// used by every current caller.
[[nodiscard]] LegacyFrameColorStatus adjust_legacy_green_channel_pairs(
    std::span<compat::u16> pixels,
    compat::i32 pixel_count,
    compat::i32 delta,
    const LegacyPixelConversionState& format
) noexcept;

[[nodiscard]] LegacyFrameColorStatus adjust_legacy_blue_channel_pairs(
    std::span<compat::u16> pixels,
    compat::i32 pixel_count,
    compat::i32 delta,
    const LegacyPixelConversionState& format
) noexcept;

// sub_4207E0. Destination is overwritten. The effective masks are truncated
// to 16 bits before the zero-count check, matching the original global writes.
[[nodiscard]] LegacyFrameColorStatus combine_legacy_channels_overflow_to_zero(
    std::span<const compat::u16> source,
    std::span<compat::u16> destination,
    compat::i32 pixel_count,
    LegacyPixelConversionState& format
) noexcept;

// sub_421FB0. q=(r+g+b)>>2, deliberately not an arithmetic mean.
[[nodiscard]] LegacyFrameColorStatus convert_legacy_quarter_sum_grayscale(
    std::span<compat::u16> pixels,
    compat::i32 pixel_count,
    const LegacyPixelConversionState& format
) noexcept;

}  // namespace openswd3::rendering
