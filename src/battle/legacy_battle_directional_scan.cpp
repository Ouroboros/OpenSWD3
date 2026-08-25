#include "openswd3/battle/legacy_battle_directional_scan.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_multiply(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) * std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
arithmetic_shift_right_ten(const compat::i32 value) noexcept {
    compat::u32 shifted = std::bit_cast<compat::u32>(value) >> 10U;
    if (value < 0) {
        shifted |= 0xFFC00000U;
    }
    return std::bit_cast<compat::i32>(shifted);
}

[[nodiscard]] bool read_source_pixel(
    const std::span<const compat::u8> source,
    const compat::u32 byte_offset,
    compat::u16& pixel
) noexcept {
    if (byte_offset >= source.size() ||
        source.size() - byte_offset < sizeof(compat::u16)) {
        return false;
    }
    pixel = static_cast<compat::u16>(source[byte_offset]) |
        static_cast<compat::u16>(
                static_cast<compat::u16>(source[byte_offset + 1U]) << 8U
        );
    return true;
}

}  // namespace

LegacyBattleDirectionalScanResult scan_legacy_battle_directional_surface(
    const LegacyBattleDirectionVectors& vectors,
    const LegacyBattleDirectionalScanSource& source,
    const LegacyBattleDirectionalSurface& destination,
    LegacyBattleDirectionalScanSharedState& shared,
    rendering::LegacyPixelConversionState& pixel_format
) noexcept {
    LegacyBattleDirectionalScanResult result;

    shared.published_value_2c = source.published_value_2c;

    const compat::u32 doubled_height_minus_two =
        static_cast<compat::u32>(source.height) * 2U - 2U;
    const compat::u32 source_base_byte_offset =
        doubled_height_minus_two * static_cast<compat::u32>(source.width);

    shared.published_value_30 = source.published_value_30;
    shared.published_value_34 = source.published_value_34;

    const compat::i32 horizontal_numerator = std::bit_cast<compat::i32>(
        static_cast<compat::u32>(source.width) << 10U
    );
    if (source.horizontal_divisor == 0) {
        result.status =
            LegacyBattleDirectionalScanStatus::horizontal_divisor_zero;
        result.legacy_return_value = horizontal_numerator;
        return result;
    }
    const compat::i32 horizontal_fixed_step =
        horizontal_numerator / source.horizontal_divisor;

    const compat::i32 vertical_numerator = std::bit_cast<compat::i32>(
        static_cast<compat::u32>(source.height) << 10U
    );
    if (source.vertical_divisor == 0) {
        result.status =
            LegacyBattleDirectionalScanStatus::vertical_divisor_zero;
        result.legacy_return_value = vertical_numerator;
        return result;
    }
    const compat::i32 vertical_fixed_step =
        vertical_numerator / source.vertical_divisor;

    LegacyBattleDirectionRaster outer_raster{
        .direction_index = source.direction_index,
        .current_x = source.start_x,
        .current_y = source.start_y,
    };
    LegacyBattleDirectionRaster inner_raster{
        .direction_index = wrapping_add(source.direction_index, 90) % 360,
        .current_x = source.start_x,
        .current_y = source.start_y,
    };

    if (vertical_fixed_step <= 0) {
        result.legacy_return_value = source.start_y;
        return result;
    }

    compat::i32 surface_y = wrapping_add(source.start_y, vertical_fixed_step);
    compat::i32 outer_remaining = vertical_fixed_step;
    compat::u32 row_table_byte_offset = std::bit_cast<compat::u32>(surface_y)
        << 2U;
    compat::i32 vertical_fixed_accumulator{};
    compat::i32 source_row_offset{};

    while (true) {
        const auto outer_status =
            advance_legacy_battle_direction_raster(vectors, outer_raster);
        if (outer_status != LegacyBattleDirectionStepStatus::completed) {
            result.status =
                LegacyBattleDirectionalScanStatus::direction_index_out_of_range;
            return result;
        }

        inner_raster.current_x = outer_raster.current_x;
        inner_raster.current_y = outer_raster.current_y;
        inner_raster.x_error = 0;
        inner_raster.y_error = 0;

        compat::i32 source_x_fixed_accumulator{};
        if (horizontal_fixed_step > 0) {
            compat::i32 destination_x = source.start_x;
            compat::i32 source_x{};
            compat::i32 inner_remaining = horizontal_fixed_step;

            while (true) {
                const auto inner_status =
                    advance_legacy_battle_direction_raster(
                        vectors, inner_raster
                    );
                if (inner_status !=
                    LegacyBattleDirectionStepStatus::completed) {
                    result.status = LegacyBattleDirectionalScanStatus::
                        direction_index_out_of_range;
                    return result;
                }

                const bool in_bounds = destination_x >= 0 &&
                    destination_x < destination.width && surface_y >= 0 &&
                    surface_y < destination.height;
                if (!in_bounds) {
                    ++result.bounds_skips;
                } else {
                    compat::i32 source_index{};
                    if ((static_cast<compat::u8>(source.flags) & 0x01U) != 0U) {
                        source_index = wrapping_subtract(
                            wrapping_subtract(
                                static_cast<compat::i32>(source.width), source_x
                            ),
                            source_row_offset
                        );
                    } else {
                        source_index =
                            wrapping_subtract(source_x, source_row_offset);
                    }

                    const compat::u32 source_byte_offset =
                        source_base_byte_offset +
                        std::bit_cast<compat::u32>(source_index) * 2U;
                    compat::u16 source_pixel{};
                    if (!read_source_pixel(
                            source.pixels, source_byte_offset, source_pixel
                        )) {
                        result.status = LegacyBattleDirectionalScanStatus::
                            source_out_of_range;
                        return result;
                    }

                    if (source_pixel == shared.first_transparent_color ||
                        source_pixel == shared.second_transparent_color) {
                        ++result.transparent_skips;
                    } else {
                        const compat::u32 row_table_index =
                            row_table_byte_offset / 4U;
                        if (row_table_index >= destination.row_offsets.size()) {
                            result.status = LegacyBattleDirectionalScanStatus::
                                row_table_out_of_range;
                            return result;
                        }
                        const compat::u32 destination_index =
                            destination.row_offsets[row_table_index] +
                            std::bit_cast<compat::u32>(destination_x);
                        if (destination_index >= destination.pixels.size()) {
                            result.status = LegacyBattleDirectionalScanStatus::
                                destination_out_of_range;
                            return result;
                        }

                        if ((static_cast<compat::u8>(source.flags) & 0x16U) !=
                            0U) {
                            const std::span<const compat::u16> source_span{
                                &source_pixel, 1U
                            };
                            const std::span<compat::u16> destination_span =
                                destination.pixels.subspan(
                                    destination_index, 1U
                                );
                            result.frame_color_status = rendering::
                                combine_legacy_channels_overflow_to_zero(
                                    source_span,
                                    destination_span,
                                    1,
                                    pixel_format
                                );
                            if (result.frame_color_status !=
                                rendering::LegacyFrameColorStatus::completed) {
                                result.status =
                                    LegacyBattleDirectionalScanStatus::
                                        frame_color_failed;
                                return result;
                            }
                            ++result.combined_writes;
                        } else {
                            destination.pixels[destination_index] =
                                source_pixel;
                            ++result.direct_writes;
                        }
                    }
                }

                source_x_fixed_accumulator = wrapping_add(
                    source_x_fixed_accumulator, source.horizontal_divisor
                );
                source_x =
                    arithmetic_shift_right_ten(source_x_fixed_accumulator);
                destination_x = wrapping_add(destination_x, 1);
                --inner_remaining;
                ++result.inner_iterations;
                if (inner_remaining == 0) {
                    break;
                }
            }
        }

        vertical_fixed_accumulator =
            wrapping_add(vertical_fixed_accumulator, source.vertical_divisor);
        source_row_offset = wrapping_multiply(
            arithmetic_shift_right_ten(vertical_fixed_accumulator),
            static_cast<compat::i32>(source.width)
        );
        surface_y = wrapping_subtract(surface_y, 1);
        row_table_byte_offset -= 4U;
        --outer_remaining;
        ++result.outer_iterations;
        if (outer_remaining == 0) {
            break;
        }
    }

    result.legacy_return_value = 0;
    return result;
}

}  // namespace openswd3::battle
