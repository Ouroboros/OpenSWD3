#include "openswd3/rendering/legacy_blitter.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace openswd3::rendering {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::array<LegacyBlitterRoutine, 256> make_blitter_table() {
    std::array<LegacyBlitterRoutine, 256> table{};

    table[0x00] = LegacyBlitterRoutine::rle_copy_forward;
    table[0x01] = LegacyBlitterRoutine::rle_copy_reverse;
    table[0x02] = LegacyBlitterRoutine::rle_copy_forward;
    table[0x03] = LegacyBlitterRoutine::rle_copy_reverse;
    table[0x04] = LegacyBlitterRoutine::rle_saturated_add_forward;
    table[0x05] = LegacyBlitterRoutine::rle_saturated_add_reverse;
    table[0x08] = LegacyBlitterRoutine::rle_coverage_forward;
    table[0x09] = LegacyBlitterRoutine::rle_coverage_reverse;
    table[0x0C] = LegacyBlitterRoutine::rle_shifted_resample_forward;
    table[0x0D] = LegacyBlitterRoutine::rle_shifted_resample_reverse;
    table[0x0E] = LegacyBlitterRoutine::rle_shifted_resample_forward;
    table[0x0F] = LegacyBlitterRoutine::rle_shifted_resample_reverse;
    table[0x10] = LegacyBlitterRoutine::rle_destination_offset_forward;
    table[0x11] = LegacyBlitterRoutine::rle_destination_offset_reverse;
    table[0x14] = LegacyBlitterRoutine::rle_opacity_forward;
    table[0x15] = LegacyBlitterRoutine::rle_opacity_reverse;
    table[0x16] = LegacyBlitterRoutine::rle_opacity_forward;
    table[0x17] = LegacyBlitterRoutine::rle_opacity_reverse;
    table[0x18] = LegacyBlitterRoutine::rle_copy_with_edges_forward;
    table[0x19] = LegacyBlitterRoutine::rle_copy_with_edges_reverse;
    table[0x1C] = LegacyBlitterRoutine::rle_vertical_opacity_fade;
    table[0x20] = LegacyBlitterRoutine::rle_saturated_resample_forward;
    table[0x21] = LegacyBlitterRoutine::rle_saturated_resample_reverse;
    table[0x24] = LegacyBlitterRoutine::rle_constant_fill_forward;
    table[0x25] = LegacyBlitterRoutine::rle_constant_fill_reverse;
    table[0x26] = LegacyBlitterRoutine::rle_constant_fill_forward;
    table[0x27] = LegacyBlitterRoutine::rle_constant_fill_reverse;
    table[0x28] = LegacyBlitterRoutine::rle_grayscale_forward;
    table[0x29] = LegacyBlitterRoutine::rle_grayscale_reverse;
    table[0x2A] = LegacyBlitterRoutine::rle_grayscale_forward;
    table[0x2B] = LegacyBlitterRoutine::rle_grayscale_reverse;
    table[0x2C] = LegacyBlitterRoutine::rle_saturated_subtract_forward;
    table[0x2D] = LegacyBlitterRoutine::rle_saturated_subtract_reverse;
    table[0x30] = LegacyBlitterRoutine::rle_smear_forward;
    table[0x31] = LegacyBlitterRoutine::rle_smear_reverse;
    table[0x32] = LegacyBlitterRoutine::rle_smear_forward;
    table[0x33] = LegacyBlitterRoutine::rle_smear_reverse;
    table[0x80] = LegacyBlitterRoutine::raw_copy_forward;
    table[0x81] = LegacyBlitterRoutine::raw_copy_reverse;
    table[0x84] = LegacyBlitterRoutine::raw_color_key_copy_forward;
    table[0x85] = LegacyBlitterRoutine::raw_color_key_copy_reverse;
    table[0x88] = LegacyBlitterRoutine::raw_constant_vertical_fade;
    table[0x94] = LegacyBlitterRoutine::raw_opacity_forward;

    return table;
}

inline constexpr std::array<LegacyBlitterRoutine, 256> kBlitterTable =
    make_blitter_table();

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

[[nodiscard]] constexpr u32 wrapping_byte_offset(
    const i32 value
) noexcept {
    return to_bits(value);
}

[[nodiscard]] bool read_u8(
    const std::span<const u8> bytes,
    const std::size_t offset,
    u8& value
) noexcept {
    if (offset >= bytes.size()) {
        return false;
    }

    value = bytes[offset];
    return true;
}

[[nodiscard]] bool read_u16(
    const std::span<const u8> bytes,
    const std::size_t offset,
    u16& value
) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 2U) {
        return false;
    }

    value = static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
    return true;
}

[[nodiscard]] bool write_u16(
    LegacyFramebuffer& framebuffer,
    const u32 byte_offset,
    const u16 value
) noexcept {
    if ((byte_offset & 1U) != 0U) {
        return false;
    }

    const std::size_t pixel_offset =
        static_cast<std::size_t>(byte_offset / 2U);
    std::span<u16> pixels = framebuffer.physical_pixels();
    if (pixel_offset >= pixels.size()) {
        return false;
    }

    pixels[pixel_offset] = value;
    return true;
}

[[nodiscard]] bool read_framebuffer_u16(
    const LegacyFramebuffer& framebuffer,
    const u32 byte_offset,
    u16& value
) noexcept {
    if ((byte_offset & 1U) != 0U) {
        return false;
    }

    const std::size_t pixel_offset =
        static_cast<std::size_t>(byte_offset / 2U);
    const std::span<const u16> pixels = framebuffer.physical_pixels();
    if (pixel_offset >= pixels.size()) {
        return false;
    }

    value = pixels[pixel_offset];
    return true;
}

[[nodiscard]] constexpr u32 legacy_shift_left(
    const u32 value,
    const u32 count
) noexcept {
    return value << (count & 31U);
}

[[nodiscard]] constexpr u32 legacy_shift_right(
    const u32 value,
    const u32 count
) noexcept {
    return value >> (count & 31U);
}

[[nodiscard]] constexpr u32 adjusted_channel(
    const u16 pixel,
    const u32 mask,
    const u32 shift,
    const i32 offset
) noexcept {
    const u32 step = legacy_shift_left(1U, shift);
    const u32 candidate = (static_cast<u32>(pixel) & mask) +
        step * to_bits(offset);
    if (((~mask) & candidate) != 0U) {
        return offset >= 0 ? mask : 0U;
    }

    return candidate;
}

[[nodiscard]] constexpr u16 destination_offset_pixel(
    const u16 pixel,
    const LegacyBlitEffectState& effects
) noexcept {
    const LegacyPixelConversionState& format = effects.pixel_conversion;
    return static_cast<u16>(
        adjusted_channel(
            pixel,
            format.effective_masks.red,
            format.red_shift,
            effects.red_offset
        ) |
        adjusted_channel(
            pixel,
            format.effective_masks.green,
            format.green_shift,
            effects.green_offset
        ) |
        adjusted_channel(
            pixel,
            format.effective_masks.blue,
            format.blue_shift,
            effects.blue_offset
        )
    );
}

[[nodiscard]] constexpr u16 grayscale_pixel(
    const u16 pixel,
    const LegacyPixelConversionState& format
) noexcept {
    u32 intensity = legacy_shift_right(
        static_cast<u32>(pixel) & format.effective_masks.red,
        format.red_shift
    );
    intensity += legacy_shift_right(
        static_cast<u32>(pixel) & format.effective_masks.green,
        format.green_shift
    );
    intensity += legacy_shift_right(
        static_cast<u32>(pixel) & format.effective_masks.blue,
        format.blue_shift
    );
    intensity >>= 2U;

    return static_cast<u16>(
        legacy_shift_left(intensity, format.red_shift) +
        legacy_shift_left(intensity, format.green_shift) +
        legacy_shift_left(intensity, format.blue_shift)
    );
}

[[nodiscard]] u32 duplicated_transparent_pixel(
    const LegacyPixelConversionState& format
) noexcept {
    u16 pixel = 0x026BU;
    legacy_convert_pixels_forward(format, &pixel, 1);
    return static_cast<u32>(pixel) |
        (static_cast<u32>(pixel) << 16U);
}

[[nodiscard]] u16 run_edge_pixel(
    const LegacyPixelConversionState& format
) noexcept {
    u16 pixel = static_cast<u16>(format.effective_masks.green);
    legacy_convert_pixels_forward(format, &pixel, 1);
    return pixel;
}

[[nodiscard]] constexpr u32 opacity_shift_mask(
    const LegacyPixelConversionState& format,
    const u32 shift
) noexcept {
    return ((format.effective_masks.red >> shift) &
            format.effective_masks.red) |
        ((format.effective_masks.green >> shift) &
         format.effective_masks.green) |
        ((format.effective_masks.blue >> shift) &
         format.effective_masks.blue);
}

[[nodiscard]] constexpr u16 opacity_pixel(
    const u16 source,
    const u16 destination,
    const i32 step,
    const LegacyPixelConversionState& format
) noexcept {
    if (step <= 0) {
        return destination;
    }
    if (step >= 15) {
        return source;
    }

    u32 result{};
    for (u32 shift = 1U; shift <= 4U; ++shift) {
        const u32 unit = 1U << (4U - shift);
        const u16 selected =
            (static_cast<u32>(step) & unit) != 0U
            ? source
            : destination;
        result += (static_cast<u32>(selected) >> shift) &
            opacity_shift_mask(format, shift);
    }

    return static_cast<u16>(result);
}

struct ClippedBlit {
    i32 destination_x{};
    i32 destination_y{};
    i32 visible_width{};
    i32 visible_height{};
    i32 source_left_skip{};
    i32 source_top_skip{};
    u32 destination_start_bytes{};
    u32 destination_row_step_bytes{};
};

enum class ClipStatus : u8 {
    visible,
    clipped_out,
    invalid_geometry,
};

[[nodiscard]] bool row_offset(
    const LegacyRasterGeometryState& geometry,
    const i32 row,
    u32& offset
) noexcept {
    if (row < 0 ||
        row >= static_cast<i32>(kLegacyRowOffsetCapacity)) {
        return false;
    }

    offset = geometry.row_byte_offsets[static_cast<std::size_t>(row)];
    return true;
}

[[nodiscard]] ClipStatus clip_request(
    const LegacyRasterGeometryState& geometry,
    const LegacyBlitClipRectangle& clip,
    const LegacyBlitRequest& request,
    ClippedBlit& result
) noexcept {
    i32 visible_height = request.source_height;
    i32 destination_y = request.destination_y;
    i32 source_top_skip{};
    i32 source_bottom_skip{};

    if (clip.top > destination_y) {
        visible_height = wrapping_add(
            wrapping_subtract(destination_y, clip.top),
            request.source_height
        );
        if (visible_height <= 0) {
            return ClipStatus::clipped_out;
        }

        source_top_skip = wrapping_subtract(
            request.source_height,
            visible_height
        );
        destination_y = clip.top;
    }

    const i32 destination_bottom = wrapping_add(
        destination_y,
        visible_height
    );
    const i32 clip_bottom = wrapping_add(clip.top, clip.height);
    if (destination_bottom >= clip_bottom) {
        source_bottom_skip = wrapping_add(
            wrapping_subtract(
                wrapping_subtract(destination_y, clip.height),
                clip.top
            ),
            request.source_height
        );
        visible_height = wrapping_add(
            wrapping_subtract(clip.height, destination_y),
            clip.top
        );
        if (visible_height <= 0) {
            return ClipStatus::clipped_out;
        }
    }

    i32 visible_width = request.source_width;
    i32 destination_x = request.destination_x;
    i32 source_left_skip{};
    if (clip.left > destination_x) {
        visible_width = wrapping_add(
            wrapping_subtract(destination_x, clip.left),
            request.source_width
        );
        if (visible_width <= 0) {
            return ClipStatus::clipped_out;
        }

        source_left_skip = wrapping_subtract(
            request.source_width,
            visible_width
        );
        destination_x = clip.left;
    }

    const i32 destination_right = wrapping_add(
        destination_x,
        visible_width
    );
    const i32 clip_right = wrapping_add(clip.left, clip.width);
    if (destination_right >= clip_right) {
        visible_width = wrapping_add(
            wrapping_subtract(clip.width, destination_x),
            clip.left
        );
        if (visible_width <= 0) {
            return ClipStatus::clipped_out;
        }
    }

    u32 destination_row{};
    u32 visible_span_rows{};
    if (!row_offset(geometry, destination_y, destination_row) ||
        !row_offset(geometry, visible_height, visible_span_rows)) {
        return ClipStatus::invalid_geometry;
    }

    const u32 destination_x_bytes =
        to_bits(destination_x) * static_cast<u32>(sizeof(u16));
    const u32 destination_start = destination_row + destination_x_bytes;
    result = ClippedBlit{
        .destination_x = destination_x,
        .destination_y = destination_y,
        .visible_width = visible_width,
        .visible_height = visible_height,
        .source_left_skip = source_left_skip,
        .source_top_skip = source_top_skip,
        .destination_start_bytes = destination_start,
        .destination_row_step_bytes = to_bits(geometry.surface.pitch_bytes),
    };

    if ((request.flags & 2U) != 0U) {
        result.source_top_skip = source_bottom_skip;
        result.destination_start_bytes =
            destination_start + visible_span_rows;
        result.destination_row_step_bytes =
            0U - result.destination_row_step_bytes;
    }

    return ClipStatus::visible;
}

[[nodiscard]] LegacyBlitExecutionStatus blit_raw_forward(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const ClippedBlit& clipped,
    const LegacyPixelConversionState& format,
    const bool color_key_copy
) noexcept {
    const i32 source_pixel = wrapping_add(
        clipped.source_left_skip,
        wrapping_multiply(request.source_width, clipped.source_top_skip)
    );
    const u32 initial_source_offset = wrapping_byte_offset(source_pixel) *
        (source.layout == LegacyBlitSourceLayout::direct_16 ? 2U : 1U);
    u32 destination_row = clipped.destination_start_bytes;
    u32 source_row = initial_source_offset;
    const u16 transparent_pixel = static_cast<u16>(
        duplicated_transparent_pixel(format)
    );

    for (i32 row = 0; row < clipped.visible_height; ++row) {
        u32 destination = destination_row;
        u32 source_offset = source_row;
        for (i32 column = 0; column < clipped.visible_width; ++column) {
            u16 pixel{};
            bool write_pixel = true;
            if (source.layout == LegacyBlitSourceLayout::direct_16) {
                if (!read_u16(
                        source.bytes,
                        static_cast<std::size_t>(source_offset),
                        pixel
                    )) {
                    return LegacyBlitExecutionStatus::malformed_source;
                }

                source_offset += 2U;
                write_pixel = !color_key_copy ||
                    pixel != transparent_pixel;
            } else {
                u8 palette_index{};
                if (!read_u8(
                        source.bytes,
                        static_cast<std::size_t>(source_offset),
                        palette_index
                    )) {
                    return LegacyBlitExecutionStatus::malformed_source;
                }

                source_offset += 1U;
                if (color_key_copy && palette_index == 1U) {
                    write_pixel = false;
                } else {
                    if (static_cast<std::size_t>(palette_index) >=
                        source.palette.size()) {
                        return LegacyBlitExecutionStatus::palette_out_of_bounds;
                    }

                    pixel = source.palette[palette_index];
                }
            }

            if (write_pixel &&
                !write_u16(framebuffer, destination, pixel)) {
                return LegacyBlitExecutionStatus::destination_out_of_bounds;
            }

            destination += 2U;
        }

        destination_row += clipped.destination_row_step_bytes;
        source_row += to_bits(request.source_width) *
            (source.layout == LegacyBlitSourceLayout::direct_16 ? 2U : 1U);
    }

    return LegacyBlitExecutionStatus::completed;
}

[[nodiscard]] LegacyBlitExecutionStatus blit_raw_reverse(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const ClippedBlit& clipped,
    const LegacyPixelConversionState& format,
    const bool color_key_copy
) noexcept {
    const i32 reverse_source_x = wrapping_subtract(
        wrapping_subtract(request.source_width, clipped.source_left_skip),
        1
    );
    const i32 source_pixel = wrapping_add(
        reverse_source_x,
        wrapping_multiply(request.source_width, clipped.source_top_skip)
    );
    const u32 initial_source_offset = wrapping_byte_offset(source_pixel) *
        (source.layout == LegacyBlitSourceLayout::direct_16 ? 2U : 1U);
    u32 destination_row = clipped.destination_start_bytes;
    u32 source_row = initial_source_offset;
    const u32 transparent_pixel = duplicated_transparent_pixel(format);

    for (i32 row = 0; row < clipped.visible_height; ++row) {
        u32 destination = destination_row;
        u32 source_offset = source_row;
        for (i32 column = 0; column < clipped.visible_width; ++column) {
            u16 pixel{};
            if (!read_u16(
                    source.bytes,
                    static_cast<std::size_t>(source_offset),
                    pixel
                )) {
                return LegacyBlitExecutionStatus::malformed_source;
            }

            if ((!color_key_copy ||
                    static_cast<u32>(pixel) != transparent_pixel) &&
                !write_u16(framebuffer, destination, pixel)) {
                return LegacyBlitExecutionStatus::destination_out_of_bounds;
            }

            source_offset -= 2U;
            destination += 2U;
        }

        destination_row += clipped.destination_row_step_bytes;
        source_row += to_bits(request.source_width) * 2U;
    }

    return LegacyBlitExecutionStatus::completed;
}

[[nodiscard]] LegacyBlitExecutionStatus blit_raw_opacity_forward(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const ClippedBlit& clipped,
    const LegacyPixelConversionState& format
) noexcept {
    const i32 source_pixel = wrapping_add(
        clipped.source_left_skip,
        wrapping_multiply(request.source_width, clipped.source_top_skip)
    );
    u32 source_row = wrapping_byte_offset(source_pixel) * 2U;
    u32 destination_row = clipped.destination_start_bytes;
    const u16 transparent_pixel = static_cast<u16>(
        duplicated_transparent_pixel(format)
    );
    const i32 internal_step = wrapping_subtract(request.opacity_step, 1);

    for (i32 row = 0; row < clipped.visible_height; ++row) {
        u32 source_offset = source_row;
        u32 destination = destination_row;
        for (i32 column = 0; column < clipped.visible_width; ++column) {
            u16 source_pixel_value{};
            if (!read_u16(
                    source.bytes,
                    static_cast<std::size_t>(source_offset),
                    source_pixel_value
                )) {
                return LegacyBlitExecutionStatus::malformed_source;
            }

            if (source_pixel_value != transparent_pixel) {
                u16 destination_pixel{};
                if (!read_framebuffer_u16(
                        framebuffer,
                        destination,
                        destination_pixel
                    )) {
                    return LegacyBlitExecutionStatus::destination_out_of_bounds;
                }

                if (!write_u16(
                        framebuffer,
                        destination,
                        opacity_pixel(
                            source_pixel_value,
                            destination_pixel,
                            internal_step,
                            format
                        )
                    )) {
                    return LegacyBlitExecutionStatus::destination_out_of_bounds;
                }
            }

            source_offset += 2U;
            destination += 2U;
        }

        source_row += to_bits(request.source_width) * 2U;
        destination_row += clipped.destination_row_step_bytes;
    }

    return LegacyBlitExecutionStatus::completed;
}

struct JitterCursor {
    bool active{};
    u32 base_bytes{};
    u32 current_bytes{};
    u32 end_bytes{};
};

[[nodiscard]] JitterCursor make_jitter_cursor(
    const LegacyRleRowJitterState& state,
    const u32 group_stride_bytes
) noexcept {
    if (state.group == 0) {
        return {};
    }

    const u32 base = (to_bits(state.group) - 1U) * group_stride_bytes;
    return JitterCursor{
        .active = true,
        .base_bytes = base,
        .current_bytes = base + state.phase_bytes,
        .end_bytes = base + 0x84U,
    };
}

[[nodiscard]] bool next_jitter_offset(
    JitterCursor& cursor,
    const LegacyRleRowJitterState& state,
    i32& pixel_offset
) noexcept {
    if (!cursor.active) {
        pixel_offset = 0;
        return true;
    }

    cursor.current_bytes += 4U;
    if (cursor.current_bytes >= cursor.end_bytes) {
        cursor.current_bytes = cursor.base_bytes;
    }

    if ((cursor.current_bytes & 3U) != 0U) {
        return false;
    }

    const std::size_t index =
        static_cast<std::size_t>(cursor.current_bytes / 4U);
    if (index >= state.offsets.size()) {
        return false;
    }

    pixel_offset = state.offsets[index];
    return true;
}

void advance_jitter_phase(LegacyRleRowJitterState& state) noexcept {
    if (state.group == 0) {
        return;
    }

    state.phase_bytes += 4U;
    if (state.phase_bytes >= 0x84U) {
        state.phase_bytes = 0U;
    }
}

enum class RlePixelOperation : u8 {
    copy,
    copy_with_edges,
    opacity,
    vertical_opacity_fade,
    destination_offset,
    constant_fill,
    grayscale,
    saturated_add,
    saturated_subtract,
};

struct RleRoutinePolicy {
    RlePixelOperation operation{RlePixelOperation::copy};
    bool reverse{};
    bool advance_phase_on_exit{};
    bool supports_third_row_skip{};
    u32 jitter_group_stride_bytes{0x84U};
};

[[nodiscard]] constexpr u32 saturated_add_channel(
    const u32 source,
    const u32 destination,
    const u32 mask
) noexcept {
    const u32 sum = source + destination;
    return ((~mask) & sum) != 0U ? mask : sum;
}

[[nodiscard]] constexpr u32 saturated_subtract_channel(
    const u32 minuend,
    const u32 subtrahend,
    const u32 mask
) noexcept {
    const u32 difference = minuend - subtrahend;
    return (difference & 0x80000000U) != 0U
        ? 0U
        : difference & mask;
}

[[nodiscard]] constexpr u16 saturated_add_pixel(
    const u16 source,
    const u16 destination,
    const LegacyBlitEffectState& effects
) noexcept {
    const LegacyPixelConversionState& format = effects.pixel_conversion;
    return static_cast<u16>(
        saturated_add_channel(
            adjusted_channel(
                source,
                format.effective_masks.red,
                format.red_shift,
                effects.red_offset
            ),
            static_cast<u32>(destination) & format.effective_masks.red,
            format.effective_masks.red
        ) |
        saturated_add_channel(
            adjusted_channel(
                source,
                format.effective_masks.green,
                format.green_shift,
                effects.green_offset
            ),
            static_cast<u32>(destination) & format.effective_masks.green,
            format.effective_masks.green
        ) |
        saturated_add_channel(
            adjusted_channel(
                source,
                format.effective_masks.blue,
                format.blue_shift,
                effects.blue_offset
            ),
            static_cast<u32>(destination) & format.effective_masks.blue,
            format.effective_masks.blue
        )
    );
}

[[nodiscard]] constexpr u16 saturated_subtract_pixel(
    const u16 source,
    const u16 destination,
    const LegacyBlitEffectState& effects,
    const bool reverse
) noexcept {
    const LegacyPixelConversionState& format = effects.pixel_conversion;
    const auto channel = [&](
        const u32 mask,
        const u32 shift,
        const i32 offset
    ) constexpr {
        if (reverse) {
            return saturated_subtract_channel(
                adjusted_channel(destination, mask, shift, offset),
                static_cast<u32>(source) & mask,
                mask
            );
        }

        return saturated_subtract_channel(
            static_cast<u32>(destination) & mask,
            adjusted_channel(source, mask, shift, offset),
            mask
        );
    };

    return static_cast<u16>(
        channel(
            format.effective_masks.red,
            format.red_shift,
            effects.red_offset
        ) |
        channel(
            format.effective_masks.green,
            format.green_shift,
            effects.green_offset
        ) |
        channel(
            format.effective_masks.blue,
            format.blue_shift,
            effects.blue_offset
        )
    );
}

[[nodiscard]] LegacyBlitExecutionStatus apply_rle_literal_pixel(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const std::size_t source_offset,
    const u32 destination,
    const LegacyBlitEffectState& effects,
    const RleRoutinePolicy& policy,
    const u16 constant_fill,
    const i32 opacity_step
) noexcept {
    u16 pixel{};
    switch (policy.operation) {
    case RlePixelOperation::copy:
    case RlePixelOperation::copy_with_edges:
        if (!read_u16(source.bytes, source_offset, pixel)) {
            return LegacyBlitExecutionStatus::malformed_source;
        }
        break;

    case RlePixelOperation::opacity:
    case RlePixelOperation::vertical_opacity_fade: {
        u16 destination_pixel{};
        if (!read_u16(source.bytes, source_offset, pixel)) {
            return LegacyBlitExecutionStatus::malformed_source;
        }
        if (!read_framebuffer_u16(
                framebuffer,
                destination,
                destination_pixel
            )) {
            return LegacyBlitExecutionStatus::destination_out_of_bounds;
        }

        pixel = opacity_pixel(
            pixel,
            destination_pixel,
            opacity_step,
            effects.pixel_conversion
        );
        break;
    }

    case RlePixelOperation::destination_offset:
        if (!read_framebuffer_u16(framebuffer, destination, pixel)) {
            return LegacyBlitExecutionStatus::destination_out_of_bounds;
        }

        pixel = destination_offset_pixel(pixel, effects);
        break;

    case RlePixelOperation::constant_fill:
        pixel = constant_fill;
        break;

    case RlePixelOperation::grayscale:
        if (!read_framebuffer_u16(framebuffer, destination, pixel)) {
            return LegacyBlitExecutionStatus::destination_out_of_bounds;
        }

        pixel = grayscale_pixel(pixel, effects.pixel_conversion);
        break;

    case RlePixelOperation::saturated_add: {
        u16 destination_pixel{};
        if (!read_u16(source.bytes, source_offset, pixel)) {
            return LegacyBlitExecutionStatus::malformed_source;
        }
        if (!read_framebuffer_u16(
                framebuffer,
                destination,
                destination_pixel
            )) {
            return LegacyBlitExecutionStatus::destination_out_of_bounds;
        }

        pixel = saturated_add_pixel(pixel, destination_pixel, effects);
        break;
    }

    case RlePixelOperation::saturated_subtract: {
        u16 destination_pixel{};
        if (!read_u16(source.bytes, source_offset, pixel)) {
            return LegacyBlitExecutionStatus::malformed_source;
        }
        if (!read_framebuffer_u16(
                framebuffer,
                destination,
                destination_pixel
            )) {
            return LegacyBlitExecutionStatus::destination_out_of_bounds;
        }

        pixel = saturated_subtract_pixel(
            pixel,
            destination_pixel,
            effects,
            policy.reverse
        );
        break;
    }
    }

    if (!write_u16(framebuffer, destination, pixel)) {
        return LegacyBlitExecutionStatus::destination_out_of_bounds;
    }

    return LegacyBlitExecutionStatus::completed;
}

[[nodiscard]] LegacyBlitExecutionStatus apply_rle_literal_run_edges(
    LegacyFramebuffer& framebuffer,
    const u32 first_destination,
    const u32 last_destination,
    const LegacyBlitEffectState& effects,
    const RleRoutinePolicy& policy
) noexcept {
    if (policy.operation != RlePixelOperation::copy_with_edges) {
        return LegacyBlitExecutionStatus::completed;
    }

    const u16 pixel = run_edge_pixel(effects.pixel_conversion);
    if (!write_u16(framebuffer, first_destination, pixel)) {
        return LegacyBlitExecutionStatus::destination_out_of_bounds;
    }

    const u32 second_destination = policy.reverse
        ? last_destination - 2U  // 0x0041D206 preserves the original BUG.
        : last_destination;
    if (!write_u16(framebuffer, second_destination, pixel)) {
        return LegacyBlitExecutionStatus::destination_out_of_bounds;
    }

    return LegacyBlitExecutionStatus::completed;
}

void finish_rle_success(
    LegacyRleRowJitterState& jitter,
    const RleRoutinePolicy& policy
) noexcept {
    if (policy.advance_phase_on_exit) {
        advance_jitter_phase(jitter);
    }
}

[[nodiscard]] LegacyBlitExecutionStatus blit_rle(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const ClippedBlit& clipped,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter,
    const RleRoutinePolicy& policy
) noexcept {
    if (policy.operation == RlePixelOperation::destination_offset &&
        effects.red_offset == 0 &&
        effects.green_offset == 0 &&
        effects.blue_offset == 0) {
        return LegacyBlitExecutionStatus::completed;
    }

    u16 constant_fill{};
    if (policy.operation == RlePixelOperation::constant_fill) {
        if (request.auxiliary.size() < sizeof(u32) ||
            !read_u16(request.auxiliary, 0U, constant_fill)) {
            return LegacyBlitExecutionStatus::auxiliary_out_of_bounds;
        }
    }

    const bool vertical_opacity_fade =
        policy.operation == RlePixelOperation::vertical_opacity_fade;
    const i32 fade_group = vertical_opacity_fade
        ? from_bits((to_bits(request.source_height) + 16U)) >> 4
        : 0;
    i32 fade_remaining = fade_group;
    i32 opacity_step = vertical_opacity_fade ? 15 : request.opacity_step;
    const auto advance_vertical_opacity = [&]() noexcept {
        if (!vertical_opacity_fade) {
            return;
        }

        fade_remaining = wrapping_subtract(fade_remaining, 1);
        if (fade_remaining == 0) {
            fade_remaining = fade_group;
            opacity_step = wrapping_subtract(opacity_step, 1);
        }
    };

    u16 source_flags{};
    if (!read_u16(source.bytes, 6U, source_flags)) {
        return LegacyBlitExecutionStatus::malformed_source;
    }

    if ((source_flags & 0x10U) == 0U) {
        finish_rle_success(jitter, policy);
        return LegacyBlitExecutionStatus::completed;
    }

    std::size_t row_pointer = 8U;
    for (i32 skipped = 0; skipped < clipped.source_top_skip; ++skipped) {
        u16 row_length{};
        if (!read_u16(source.bytes, row_pointer, row_length)) {
            return LegacyBlitExecutionStatus::malformed_source;
        }

        row_pointer += static_cast<std::size_t>(row_length & 0x7FFFU);
        if (row_pointer > source.bytes.size()) {
            return LegacyBlitExecutionStatus::malformed_source;
        }

        advance_vertical_opacity();
    }

    i32 source_window_start = clipped.source_left_skip;
    if (policy.reverse) {
        source_window_start = wrapping_subtract(
            wrapping_subtract(
                request.source_width,
                clipped.visible_width
            ),
            clipped.source_left_skip
        );
    }

    if (source_window_start < 0) {
        return LegacyBlitExecutionStatus::malformed_source;
    }

    const std::uint64_t window_start =
        static_cast<std::uint64_t>(source_window_start);
    const std::uint64_t window_end = window_start +
        static_cast<std::uint64_t>(clipped.visible_width);
    JitterCursor jitter_cursor = make_jitter_cursor(
        jitter,
        policy.jitter_group_stride_bytes
    );
    u32 destination_row = clipped.destination_start_bytes;
    i32 processed_rows{};
    i32 third_row_phase = wrapping_add(clipped.destination_y, 480) % 3;

    while (true) {
        u16 row_length{};
        if (!read_u16(source.bytes, row_pointer, row_length)) {
            return LegacyBlitExecutionStatus::malformed_source;
        }

        i32 jitter_pixels{};
        if (!next_jitter_offset(jitter_cursor, jitter, jitter_pixels)) {
            return LegacyBlitExecutionStatus::jitter_table_out_of_bounds;
        }

        advance_vertical_opacity();

        const u32 jitter_bytes = to_bits(jitter_pixels) * 2U;
        const u32 jittered_destination_row = destination_row + jitter_bytes;
        if (row_length == 0U) {
            finish_rle_success(jitter, policy);
            return LegacyBlitExecutionStatus::completed;
        }

        processed_rows = wrapping_add(processed_rows, 1);
        if (processed_rows > clipped.visible_height) {
            finish_rle_success(jitter, policy);
            return LegacyBlitExecutionStatus::completed;
        }

        bool skip_row{};
        if (policy.supports_third_row_skip &&
            effects.skip_every_third_row) {
            third_row_phase = wrapping_add(third_row_phase, 1);
            if (third_row_phase >= 3) {
                third_row_phase = 0;
                skip_row = true;
            }
        }

        std::size_t command_pointer = row_pointer + 2U;
        std::uint64_t source_x{};
        while (!skip_row) {
            u16 command{};
            if (!read_u16(source.bytes, command_pointer, command)) {
                return LegacyBlitExecutionStatus::malformed_source;
            }

            command_pointer += 2U;
            const std::uint64_t run =
                static_cast<std::uint64_t>(command & 0x3FFFU);
            if (run == 0U) {
                break;
            }

            const std::uint64_t run_start = source_x;
            const std::uint64_t run_end = run_start + run;
            const bool literal = (command & 0xC000U) == 0U;

            if (run_end <= window_start) {
                if (literal) {
                    const std::uint64_t skipped_bytes = run * 2U;
                    if (skipped_bytes >
                        std::numeric_limits<std::size_t>::max() -
                            command_pointer) {
                        return LegacyBlitExecutionStatus::malformed_source;
                    }

                    command_pointer +=
                        static_cast<std::size_t>(skipped_bytes);
                    if (command_pointer > source.bytes.size()) {
                        return LegacyBlitExecutionStatus::malformed_source;
                    }
                }

                source_x = run_end;
                continue;
            }

            if (run_start >= window_end) {
                break;
            }

            const std::uint64_t visible_start =
                run_start < window_start ? window_start : run_start;
            const std::uint64_t visible_end =
                run_end < window_end ? run_end : window_end;
            const std::uint64_t visible_count =
                visible_end - visible_start;
            const std::uint64_t prefix = visible_start - run_start;

            if (literal) {
                const std::uint64_t payload_offset =
                    static_cast<std::uint64_t>(command_pointer) + prefix * 2U;
                const std::uint64_t payload_end = payload_offset +
                    visible_count * 2U;
                if (payload_offset >
                        std::numeric_limits<std::size_t>::max() ||
                    payload_end > source.bytes.size()) {
                    return LegacyBlitExecutionStatus::malformed_source;
                }

                const auto destination_for = [&](const std::uint64_t index) {
                    const std::uint64_t logical_x =
                        visible_start - window_start + index;
                    const std::uint64_t destination_x = policy.reverse
                        ? static_cast<std::uint64_t>(clipped.visible_width) -
                              logical_x - 1U
                        : logical_x;
                    return jittered_destination_row +
                        static_cast<u32>(destination_x * 2U);
                };
                const u32 first_destination = destination_for(0U);
                const u32 last_destination = destination_for(
                    visible_count - 1U
                );

                for (std::uint64_t index = 0U;
                     index < visible_count;
                     ++index) {
                    const std::uint64_t source_offset =
                        payload_offset + index * 2U;
                    if (source_offset >
                        std::numeric_limits<std::size_t>::max()) {
                        return LegacyBlitExecutionStatus::malformed_source;
                    }

                    const u32 destination = destination_for(index);
                    const LegacyBlitExecutionStatus pixel_status =
                        apply_rle_literal_pixel(
                            framebuffer,
                            source,
                            static_cast<std::size_t>(source_offset),
                            destination,
                            effects,
                            policy,
                            constant_fill,
                            opacity_step
                        );
                    if (pixel_status != LegacyBlitExecutionStatus::completed) {
                        return pixel_status;
                    }
                }

                const LegacyBlitExecutionStatus edge_status =
                    apply_rle_literal_run_edges(
                        framebuffer,
                        first_destination,
                        last_destination,
                        effects,
                        policy
                    );
                if (edge_status != LegacyBlitExecutionStatus::completed) {
                    return edge_status;
                }
            }

            if (run_end >= window_end) {
                break;
            }

            if (literal) {
                const std::uint64_t payload_bytes = run * 2U;
                if (payload_bytes >
                    std::numeric_limits<std::size_t>::max() -
                        command_pointer) {
                    return LegacyBlitExecutionStatus::malformed_source;
                }

                command_pointer += static_cast<std::size_t>(payload_bytes);
                if (command_pointer > source.bytes.size()) {
                    return LegacyBlitExecutionStatus::malformed_source;
                }
            }

            source_x = run_end;
        }

        const std::size_t next_row = row_pointer +
            static_cast<std::size_t>(row_length & 0x7FFFU);
        if (next_row > source.bytes.size()) {
            return LegacyBlitExecutionStatus::malformed_source;
        }

        row_pointer = next_row;
        destination_row += clipped.destination_row_step_bytes;
    }
}

}  // namespace

LegacyBlitterRoutine legacy_blitter_routine(
    const u32 table_slot
) noexcept {
    if (table_slot >= kBlitterTable.size()) {
        return LegacyBlitterRoutine::unassigned;
    }

    return kBlitterTable[static_cast<std::size_t>(table_slot)];
}

LegacyBlitterSelection select_legacy_blitter(
    const u16 source_first_word,
    const bool palette_pointer_nonzero,
    u32 flags,
    const i32 opacity_step
) noexcept {
    if (source_first_word == 0xFFFFU && !palette_pointer_nonzero) {
        flags |= 0x80000000U;
    }

    if ((flags & 0x0000FFFCU) == 0x14U) {
        if (opacity_step <= 0) {
            return LegacyBlitterSelection{
                .status = LegacyBlitterSelectionStatus::opacity_disabled,
                .effective_flags = flags,
                .table_slot = 0U,
                .routine = LegacyBlitterRoutine::unassigned,
                .rle_family = (flags & 0x80000000U) != 0U,
            };
        }

        if (opacity_step > 15) {
            flags &= 0x80000003U;
        }
    }

    const bool rle_family = (flags & 0x80000000U) != 0U;
    const u32 table_slot = (flags & 0xFFFFU) +
        (rle_family ? 0U : 0x80U);
    const LegacyBlitterRoutine routine = legacy_blitter_routine(table_slot);
    return LegacyBlitterSelection{
        .status = routine == LegacyBlitterRoutine::unassigned
            ? LegacyBlitterSelectionStatus::unassigned
            : LegacyBlitterSelectionStatus::selected,
        .effective_flags = flags,
        .table_slot = table_slot,
        .routine = routine,
        .rle_family = rle_family,
    };
}

LegacyBlitResult blit_legacy_copy_paths(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyBlitSource& source,
    const LegacyBlitRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept {
    u16 source_first_word{};
    if (!read_u16(source.bytes, 0U, source_first_word)) {
        return LegacyBlitResult{
            .status = LegacyBlitExecutionStatus::malformed_source,
        };
    }

    const bool palette_pointer_nonzero =
        source.layout == LegacyBlitSourceLayout::indexed_8;
    const LegacyBlitterSelection selection = select_legacy_blitter(
        source_first_word,
        palette_pointer_nonzero,
        request.flags,
        request.opacity_step
    );
    if (selection.status ==
        LegacyBlitterSelectionStatus::opacity_disabled) {
        return LegacyBlitResult{
            .status = LegacyBlitExecutionStatus::opacity_disabled,
            .selection = selection,
        };
    }

    ClippedBlit clipped{};
    const ClipStatus clip_status = clip_request(
        framebuffer.geometry(),
        clip,
        request,
        clipped
    );
    if (clip_status == ClipStatus::clipped_out) {
        return LegacyBlitResult{
            .status = LegacyBlitExecutionStatus::clipped_out,
            .selection = selection,
        };
    }

    if (clip_status == ClipStatus::invalid_geometry) {
        return LegacyBlitResult{
            .status = LegacyBlitExecutionStatus::invalid_geometry,
            .selection = selection,
        };
    }

    if (selection.status == LegacyBlitterSelectionStatus::unassigned) {
        return LegacyBlitResult{
            .status = LegacyBlitExecutionStatus::unassigned_routine,
            .selection = selection,
        };
    }

    const auto run_rle = [&](const RleRoutinePolicy& policy) {
        return blit_rle(
            framebuffer,
            source,
            request,
            clipped,
            effects,
            jitter,
            policy
        );
    };

    LegacyBlitExecutionStatus status{};
    switch (selection.routine) {
    case LegacyBlitterRoutine::raw_copy_forward:
        status = blit_raw_forward(
            framebuffer,
            source,
            request,
            clipped,
            effects.pixel_conversion,
            false
        );
        break;

    case LegacyBlitterRoutine::raw_copy_reverse:
        status = blit_raw_reverse(
            framebuffer,
            source,
            request,
            clipped,
            effects.pixel_conversion,
            false
        );
        break;

    case LegacyBlitterRoutine::raw_color_key_copy_forward:
        status = blit_raw_forward(
            framebuffer,
            source,
            request,
            clipped,
            effects.pixel_conversion,
            true
        );
        break;

    case LegacyBlitterRoutine::raw_color_key_copy_reverse:
        status = blit_raw_reverse(
            framebuffer,
            source,
            request,
            clipped,
            effects.pixel_conversion,
            true
        );
        break;

    case LegacyBlitterRoutine::raw_opacity_forward:
        status = blit_raw_opacity_forward(
            framebuffer,
            source,
            request,
            clipped,
            effects.pixel_conversion
        );
        break;

    case LegacyBlitterRoutine::rle_copy_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::copy,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_copy_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::copy,
            .reverse = true,
        });
        break;

    case LegacyBlitterRoutine::rle_saturated_add_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::saturated_add,
            .advance_phase_on_exit = true,
            .supports_third_row_skip = true,
        });
        break;

    case LegacyBlitterRoutine::rle_saturated_add_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::saturated_add,
            .reverse = true,
            .advance_phase_on_exit = true,
            .supports_third_row_skip = true,
        });
        break;

    case LegacyBlitterRoutine::rle_opacity_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::opacity,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_opacity_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::opacity,
            .reverse = true,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_vertical_opacity_fade:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::vertical_opacity_fade,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_copy_with_edges_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::copy_with_edges,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_copy_with_edges_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::copy_with_edges,
            .reverse = true,
        });
        break;

    case LegacyBlitterRoutine::rle_destination_offset_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::destination_offset,
            .advance_phase_on_exit = true,
            .jitter_group_stride_bytes = 0x528U,
        });
        break;

    case LegacyBlitterRoutine::rle_destination_offset_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::destination_offset,
            .reverse = true,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_constant_fill_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::constant_fill,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_constant_fill_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::constant_fill,
            .reverse = true,
        });
        break;

    case LegacyBlitterRoutine::rle_grayscale_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::grayscale,
            .advance_phase_on_exit = true,
            .jitter_group_stride_bytes = 0x528U,
        });
        break;

    case LegacyBlitterRoutine::rle_grayscale_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::grayscale,
            .reverse = true,
            .advance_phase_on_exit = true,
        });
        break;

    case LegacyBlitterRoutine::rle_saturated_subtract_forward:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::saturated_subtract,
            .advance_phase_on_exit = true,
            .supports_third_row_skip = true,
        });
        break;

    case LegacyBlitterRoutine::rle_saturated_subtract_reverse:
        status = run_rle(RleRoutinePolicy{
            .operation = RlePixelOperation::saturated_subtract,
            .reverse = true,
            .advance_phase_on_exit = true,
            .supports_third_row_skip = true,
        });
        break;

    default:
        status = LegacyBlitExecutionStatus::unsupported_routine;
        break;
    }

    return LegacyBlitResult{
        .status = status,
        .selection = selection,
    };
}

}  // namespace openswd3::rendering
