#include "openswd3/world_map/legacy_world_facing.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr std::array<u16, 91> kLegacySine8192{
    0,    142,  285,  428,  571,  713,  856,  998,  1140, 1281,
    1422, 1563, 1703, 1842, 1981, 2120, 2258, 2395, 2531, 2667,
    2801, 2935, 3068, 3200, 3331, 3462, 3591, 3719, 3845, 3971,
    4096, 4219, 4341, 4461, 4580, 4698, 4815, 4930, 5043, 5155,
    5265, 5374, 5481, 5586, 5690, 5792, 5892, 5991, 6087, 6182,
    6275, 6366, 6455, 6542, 6627, 6710, 6791, 6870, 6947, 7021,
    7094, 7164, 7233, 7299, 7362, 7424, 7483, 7540, 7595, 7647,
    7697, 7745, 7791, 7834, 7874, 7912, 7948, 7982, 8012, 8041,
    8067, 8091, 8112, 8130, 8147, 8160, 8172, 8180, 8187, 8190,
    8192,
};

constexpr std::array<u32, 17> kLegacyAngleSectors{
    3, 7, 7, 0, 0, 4, 4, 2, 2, 6, 6, 1, 1, 5, 5, 3, 0,
};

[[nodiscard]] constexpr i32 wrapping_subtract(
    const u32 left,
    const u32 right
) noexcept {
    return std::bit_cast<i32>(left - right);
}

[[nodiscard]] constexpr i32 wrapping_absolute(const i32 value) noexcept {
    const u32 bits = std::bit_cast<u32>(value);
    const u32 sign = value < 0 ? 0xFFFFFFFFU : 0U;
    return std::bit_cast<i32>((bits ^ sign) - sign);
}

[[nodiscard]] constexpr u32 floor_square_root(u32 value) noexcept {
    u32 root = 0U;
    u32 bit = 1U << 30U;
    while (bit > value) {
        bit >>= 2U;
    }
    while (bit != 0U) {
        if (value >= root + bit) {
            value -= root + bit;
            root = (root >> 1U) + bit;
        } else {
            root >>= 1U;
        }
        bit >>= 2U;
    }
    return root;
}

[[nodiscard]] u32 legacy_distance(const i32 delta_x, const i32 delta_y) noexcept {
    const u32 square_sum =
        std::bit_cast<u32>(delta_x) * std::bit_cast<u32>(delta_x) +
        std::bit_cast<u32>(delta_y) * std::bit_cast<u32>(delta_y);
    const i32 signed_square_sum = std::bit_cast<i32>(square_sum);
    if (signed_square_sum < 0) {
        // x87 fsqrt followed by the original truncating fistp helper yields
        // the integer-indefinite value; its low dword is zero.
        return 0U;
    }
    return floor_square_root(square_sum);
}

[[nodiscard]] u32 quantize_legacy_angle(
    const i32 delta_x,
    const i32 delta_y,
    const u32 distance
) noexcept {
    if (distance == 0U) {
        return 0U;
    }

    const i32 shifted_y = std::bit_cast<i32>(
        std::bit_cast<u32>(delta_y) << 13U
    );
    const i32 normalized = wrapping_absolute(
        shifted_y / static_cast<i32>(distance)
    );

    i32 start_angle = 180;
    i32 table_index = 0;
    i32 table_step = 5;
    if (delta_y >= 0) {
        if (delta_x >= 0) {
            start_angle = 270;
            table_index = 89;
            table_step = -5;
        }
    } else {
        start_angle = 90;
        table_index = 89;
        table_step = -5;
        if (delta_x >= 0) {
            start_angle = 0;
            table_index = 0;
            table_step = 5;
        }
    }

    i32 best_difference = 9000;
    u32 best_angle = 0U;
    for (i32 angle = start_angle; angle < start_angle + 90;
         angle += 5, table_index += table_step) {
        const i32 difference = wrapping_absolute(std::bit_cast<i32>(
            std::bit_cast<u32>(normalized) -
            static_cast<u32>(kLegacySine8192[static_cast<std::size_t>(
                table_index
            )])
        ));
        if (difference <= best_difference) {
            best_difference = difference;
            best_angle = static_cast<u32>(angle);
        }
    }
    return best_angle;
}

[[nodiscard]] constexpr u32 map_legacy_angle_to_direction(
    const u32 angle_degrees
) noexcept {
    const u32 sector = (angle_degrees * 16U) / 360U;
    return kLegacyAngleSectors[sector];
}

}  // namespace

LegacyWorldFacingResult measure_legacy_world_facing(
    const u32 source_x,
    const u32 source_y,
    const u32 target_x,
    const u32 target_y
) noexcept {
    const i32 delta_x = wrapping_subtract(source_x, target_x);
    const i32 delta_y = wrapping_subtract(source_y, target_y);
    const u32 distance = legacy_distance(delta_x, delta_y);
    const u32 angle = quantize_legacy_angle(delta_x, delta_y, distance);
    return {
        .distance = distance,
        .angle_degrees = angle,
        .direction = map_legacy_angle_to_direction(angle),
    };
}

}  // namespace openswd3::world_map
