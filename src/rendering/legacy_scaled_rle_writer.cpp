#include "openswd3/rendering/legacy_scaled_rle_writer.hpp"

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

enum class Direction : u8 {
    forward,
    reverse,
};

struct RowBounds {
    std::size_t start{};
    std::size_t end{};
    u16 encoded_length{};
};

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) + to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) - to_bits(right));
}

[[nodiscard]] constexpr i32
wrapping_multiply(const i32 left, const i32 right) noexcept {
    return from_bits(to_bits(left) * to_bits(right));
}

[[nodiscard]] constexpr i32
truncate_10_10_product(const i32 left, const i32 right) noexcept {
    return wrapping_multiply(left, right) / 0x400;
}

[[nodiscard]] constexpr i32 signed_remainder_10_10(const i32 value) noexcept {
    return value % 0x400;
}

[[nodiscard]] bool read_u16(
    const std::span<const u8> bytes, const std::size_t offset, u16& value
) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < 2U) {
        return false;
    }
    value = static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
    return true;
}

[[nodiscard]] bool row_bounds(
    const LegacyScaledRleSource& source,
    const std::size_t row_offset,
    RowBounds& bounds
) noexcept {
    u16 encoded_length{};
    if (!read_u16(source.bytes, row_offset, encoded_length)) {
        return false;
    }
    if (encoded_length == 0U) {
        bounds = RowBounds{row_offset, row_offset, 0U};
        return true;
    }

    const std::size_t length = encoded_length & 0x3FFFU;
    if (length < 2U || length > source.bytes.size() - row_offset) {
        return false;
    }
    bounds = RowBounds{
        .start = row_offset,
        .end = row_offset + length,
        .encoded_length = encoded_length,
    };
    return true;
}

[[nodiscard]] bool valid_geometry(
    const LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const LegacyScaledRleRequest& request
) noexcept {
    const LegacySurfaceGeometry geometry = framebuffer.geometry().surface;
    if (source.row_stream_offset >= source.bytes.size() ||
        request.source_width < 0 || request.source_height < 0 ||
        clip.left < 0 || clip.top < 0 || clip.width <= 0 || clip.height <= 0) {
        return false;
    }

    const std::int64_t right =
        static_cast<std::int64_t>(clip.left) + clip.width;
    const std::int64_t bottom =
        static_cast<std::int64_t>(clip.top) + clip.height;
    return right <= geometry.width && bottom <= geometry.height;
}

[[nodiscard]] LegacyScaledRleWriteStatus write_pixel(
    LegacyFramebuffer& framebuffer, const i32 destination_word, const u16 pixel
) noexcept {
    if (destination_word < 0 ||
        static_cast<std::size_t>(destination_word) >=
            framebuffer.physical_pixels().size()) {
        return LegacyScaledRleWriteStatus::destination_out_of_bounds;
    }
    framebuffer.physical_pixels()[static_cast<std::size_t>(destination_word)] =
        pixel;
    return LegacyScaledRleWriteStatus::completed;
}

[[nodiscard]] bool run_write_bounds(
    const Direction direction,
    const i32 destination_word,
    const i32 scaled_run,
    const i32 clip_left_word,
    const i32 clip_right_word,
    const i32 clip_width,
    i32& lower_bound,
    i32& upper_bound
) noexcept {
    if (direction == Direction::forward) {
        const i32 left_distance =
            wrapping_subtract(destination_word, clip_left_word);
        const i32 right_overflow = wrapping_subtract(
            wrapping_add(destination_word, scaled_run), clip_right_word
        );
        if (left_distance >= 0) {
            if (left_distance >= clip_width) {
                return false;
            }
            lower_bound = destination_word;
        } else {
            if (wrapping_add(left_distance, scaled_run) <= 0) {
                return false;
            }
            lower_bound = clip_left_word;
        }
        upper_bound = right_overflow > 0
            ? wrapping_subtract(clip_right_word, 1)
            : wrapping_add(destination_word, scaled_run);
        return true;
    }

    const i32 left_distance = wrapping_subtract(
        wrapping_subtract(destination_word, scaled_run), clip_left_word
    );
    const i32 right_distance =
        wrapping_subtract(destination_word, clip_right_word);
    if (left_distance >= 0) {
        if (left_distance >= clip_width) {
            return false;
        }
        lower_bound = wrapping_subtract(destination_word, scaled_run);
        upper_bound = right_distance > 0 ? wrapping_subtract(clip_right_word, 1)
                                         : destination_word;
        return true;
    }

    if (wrapping_add(left_distance, scaled_run) <= 0) {
        return false;
    }
    lower_bound = clip_left_word;
    upper_bound = right_distance > 0
        ? wrapping_subtract(wrapping_add(clip_right_word, scaled_run), 1)
        : wrapping_add(destination_word, scaled_run);
    return true;
}

[[nodiscard]] LegacyScaledRleWriteStatus draw_row(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const RowBounds& row,
    const i32 destination_y,
    const i32 row_start_x,
    const i32 horizontal_step,
    const Direction direction
) noexcept {
    const i32 pitch_words = framebuffer.geometry().surface.pitch_bytes / 2;
    const i32 row_base = wrapping_multiply(destination_y, pitch_words);
    const i32 clip_left_word = wrapping_add(row_base, clip.left);
    const i32 clip_right_word = wrapping_add(clip_left_word, clip.width);
    i32 command_destination = wrapping_add(row_base, row_start_x);
    i32 horizontal_phase{};
    std::size_t command_offset = row.start + 2U;

    while (true) {
        u16 command{};
        if (command_offset >= row.end ||
            !read_u16(source.bytes, command_offset, command)) {
            return LegacyScaledRleWriteStatus::malformed_source;
        }
        command_offset += 2U;

        if (command == 0U) {
            return LegacyScaledRleWriteStatus::completed;
        }
        const i32 run = command & 0x3FFFU;
        const bool literal = (command & 0xC000U) == 0U;
        const i32 scaled_run = truncate_10_10_product(horizontal_step, run);
        i32 lower_bound{};
        i32 upper_bound{};
        const bool can_write = run_write_bounds(
            direction,
            command_destination,
            scaled_run,
            clip_left_word,
            clip_right_word,
            clip.width,
            lower_bound,
            upper_bound
        );

        if (literal) {
            const std::size_t payload_bytes =
                static_cast<std::size_t>(run) * 2U;
            if (payload_bytes > row.end - command_offset) {
                return LegacyScaledRleWriteStatus::malformed_source;
            }

            if (can_write) {
                horizontal_phase = signed_remainder_10_10(horizontal_phase);
                i32 output_destination = command_destination;
                bool reached_terminal_bound = false;
                for (i32 index = 0; index < run && !reached_terminal_bound;
                     ++index) {
                    u16 pixel{};
                    if (!read_u16(
                            source.bytes,
                            command_offset +
                                static_cast<std::size_t>(index) * 2U,
                            pixel
                        )) {
                        return LegacyScaledRleWriteStatus::malformed_source;
                    }

                    horizontal_phase =
                        wrapping_add(horizontal_phase, horizontal_step);
                    while (horizontal_phase >= 0x400) {
                        horizontal_phase =
                            wrapping_subtract(horizontal_phase, 0x400);
                        reached_terminal_bound = direction == Direction::forward
                            ? output_destination > upper_bound
                            : output_destination < lower_bound;
                        if (reached_terminal_bound) {
                            break;
                        }
                        const bool visible = direction == Direction::forward
                            ? output_destination >= lower_bound
                            : output_destination <= upper_bound;
                        if (visible) {
                            const LegacyScaledRleWriteStatus status =
                                write_pixel(
                                    framebuffer, output_destination, pixel
                                );
                            if (status !=
                                LegacyScaledRleWriteStatus::completed) {
                                return status;
                            }
                        }
                        output_destination = direction == Direction::forward
                            ? wrapping_add(output_destination, 1)
                            : wrapping_subtract(output_destination, 1);
                    }
                }
            }
            command_offset += payload_bytes;
        } else if (can_write) {
            horizontal_phase = wrapping_add(
                horizontal_phase, wrapping_multiply(horizontal_step, scaled_run)
            );
        }

        command_destination = direction == Direction::forward
            ? wrapping_add(command_destination, scaled_run)
            : wrapping_subtract(command_destination, scaled_run);
    }
}

[[nodiscard]] LegacyScaledRleWriteStatus write_scaled_rle(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const LegacyScaledRleRequest& request,
    const LegacyScaledRleTransform& transform,
    const Direction direction
) noexcept {
    if (!valid_geometry(framebuffer, clip, source, request)) {
        return LegacyScaledRleWriteStatus::invalid_geometry;
    }

    i32 row_start_x{};
    i32 scaled_left{};
    i32 scaled_right{};
    if (direction == Direction::forward) {
        row_start_x = wrapping_add(
            request.destination_x,
            wrapping_subtract(
                transform.anchor_x,
                truncate_10_10_product(
                    transform.horizontal_step_10_10, transform.anchor_x
                )
            )
        );
        scaled_left = row_start_x;
        scaled_right = wrapping_add(
            row_start_x,
            truncate_10_10_product(
                transform.horizontal_step_10_10, request.source_width
            )
        );
    } else {
        scaled_right = wrapping_add(
            request.destination_x,
            wrapping_add(
                transform.anchor_x,
                truncate_10_10_product(
                    wrapping_subtract(request.source_width, transform.anchor_x),
                    transform.horizontal_step_10_10
                )
            )
        );
        row_start_x = scaled_right;
        scaled_left = wrapping_subtract(
            scaled_right,
            truncate_10_10_product(
                transform.horizontal_step_10_10, request.source_width
            )
        );
    }

    const i32 scaled_top = wrapping_add(
        request.destination_y,
        wrapping_subtract(
            transform.anchor_y,
            truncate_10_10_product(
                transform.vertical_step_10_10, transform.anchor_y
            )
        )
    );
    const i32 scaled_bottom = wrapping_add(
        scaled_top,
        truncate_10_10_product(
            transform.vertical_step_10_10, request.source_height
        )
    );
    const i32 clip_right = wrapping_add(clip.left, clip.width);
    const i32 clip_bottom = wrapping_add(clip.top, clip.height);
    const bool vertically_clipped_out = direction == Direction::forward
        ? scaled_top >= clip_bottom || scaled_bottom < clip.top
        : scaled_bottom >= clip_bottom || scaled_top < clip.top;
    if (scaled_left >= clip_right || scaled_right < clip.left ||
        vertically_clipped_out) {
        return LegacyScaledRleWriteStatus::clipped_out;
    }

    std::size_t row_offset = source.row_stream_offset;
    i32 destination_y = scaled_top;
    i32 vertical_phase{};

    while (true) {
        RowBounds row{};
        if (!row_bounds(source, row_offset, row)) {
            return LegacyScaledRleWriteStatus::malformed_source;
        }
        if (row.encoded_length == 0U) {
            return LegacyScaledRleWriteStatus::completed;
        }

        vertical_phase =
            wrapping_add(vertical_phase, transform.vertical_step_10_10);
        while (vertical_phase >= 0x400) {
            if (destination_y >= clip.top) {
                vertical_phase = signed_remainder_10_10(vertical_phase);
                goto render_rows;
            }
            destination_y = wrapping_add(destination_y, 1);
            vertical_phase = wrapping_subtract(vertical_phase, 0x400);
        }
        row_offset = row.end;
    }

render_rows:
    while (true) {
        RowBounds row{};
        if (!row_bounds(source, row_offset, row)) {
            return LegacyScaledRleWriteStatus::malformed_source;
        }
        if (row.encoded_length == 0U) {
            return LegacyScaledRleWriteStatus::completed;
        }

        vertical_phase =
            wrapping_add(vertical_phase, transform.vertical_step_10_10);
        while (vertical_phase >= 0x400) {
            vertical_phase = wrapping_subtract(vertical_phase, 0x400);
            if (destination_y >= clip_bottom) {
                return LegacyScaledRleWriteStatus::completed;
            }
            const LegacyScaledRleWriteStatus status = draw_row(
                framebuffer,
                clip,
                source,
                row,
                destination_y,
                row_start_x,
                transform.horizontal_step_10_10,
                direction
            );
            if (status != LegacyScaledRleWriteStatus::completed) {
                return status;
            }
            destination_y = wrapping_add(destination_y, 1);
        }
        row_offset = row.end;
    }
}

}  // namespace

LegacyScaledRleWriteStatus write_legacy_scaled_rle_forward(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const LegacyScaledRleRequest& request,
    const LegacyScaledRleTransform& transform
) noexcept {
    return write_scaled_rle(
        framebuffer, clip, source, request, transform, Direction::forward
    );
}

LegacyScaledRleWriteStatus write_legacy_scaled_rle_reverse(
    LegacyFramebuffer& framebuffer,
    const LegacyBlitClipRectangle& clip,
    const LegacyScaledRleSource& source,
    const LegacyScaledRleRequest& request,
    const LegacyScaledRleTransform& transform
) noexcept {
    return write_scaled_rle(
        framebuffer, clip, source, request, transform, Direction::reverse
    );
}

}  // namespace openswd3::rendering
