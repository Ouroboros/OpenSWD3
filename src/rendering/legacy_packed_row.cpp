#include "openswd3/rendering/legacy_packed_row.hpp"

#include <cstddef>

namespace openswd3::rendering {
namespace {

[[nodiscard]] constexpr compat::u32 low_word(
    const compat::u32 value
) noexcept {
    return value & 0xFFFFU;
}

[[nodiscard]] constexpr compat::u32 shifted_channel_mask(
    const compat::u32 mask,
    const compat::u32 shift_count
) noexcept {
    const compat::u32 base = low_word(mask);
    compat::u32 shifted = base;
    for (compat::u32 index = 0U; index < shift_count; ++index) {
        shifted = (shifted >> 1U) & base;
    }
    return shifted;
}

[[nodiscard]] constexpr compat::u32 duplicated_shift_mask(
    const LegacyPixelConversionState& format,
    const compat::u32 shift_count
) noexcept {
    const compat::u32 lane = shifted_channel_mask(
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

}  // namespace

LegacyPackedRowBlendStatus blend_legacy_packed_row(
    const std::span<compat::u16> destination,
    const compat::u32 color_pattern,
    const compat::i32 pixel_count,
    const LegacyPixelConversionState& format
) noexcept {
    if (pixel_count < 2) {
        return LegacyPackedRowBlendStatus::invalid_geometry;
    }

    const auto pair_count = static_cast<std::size_t>(pixel_count / 2);
    if (pair_count > destination.size() / 2U) {
        return LegacyPackedRowBlendStatus::destination_out_of_bounds;
    }

    const compat::u32 mask_1 = duplicated_shift_mask(format, 1U);
    const compat::u32 mask_2 = duplicated_shift_mask(format, 2U);
    const compat::u32 mask_3 = duplicated_shift_mask(format, 3U);
    const compat::u32 mask_4 = duplicated_shift_mask(format, 4U);
    const compat::u32 color_term = (color_pattern >> 2U) & mask_2;

    for (std::size_t pair = 0U; pair < pair_count; ++pair) {
        const std::size_t offset = pair * 2U;
        const compat::u32 current = read_pair(destination, offset);
        write_pair(
            destination,
            offset,
            ((current >> 1U) & mask_1) + color_term +
                ((current >> 3U) & mask_3) +
                ((current >> 4U) & mask_4)
        );
    }

    return LegacyPackedRowBlendStatus::completed;
}

}  // namespace openswd3::rendering
