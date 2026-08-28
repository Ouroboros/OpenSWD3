#include "openswd3/battle/legacy_battle_script_curve.hpp"

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

constexpr float kNegativeOneSixth = std::bit_cast<float>(0xBE2AAAABU);
constexpr float kOneSixth = std::bit_cast<float>(0x3E2AAAABU);
constexpr float kTwoThirds = std::bit_cast<float>(0x3F2AAAABU);

[[nodiscard]] float basis_value(
    const float cubic,
    const float square,
    const float linear,
    const float cubic_factor,
    const float square_factor,
    const float linear_factor,
    const float constant
) noexcept {
    const long double value = static_cast<long double>(cubic_factor) * cubic +
        static_cast<long double>(square_factor) * square + constant +
        static_cast<long double>(linear_factor) * linear;
    return static_cast<float>(value);
}

[[nodiscard]] i32 truncate_x87_low(const float value) noexcept {
    if (!std::isfinite(value)) {
        return 0;
    }
    const long double truncated = std::trunc(static_cast<long double>(value));
    constexpr long double minimum =
        static_cast<long double>(std::numeric_limits<std::int64_t>::min());
    constexpr long double maximum =
        static_cast<long double>(std::numeric_limits<std::int64_t>::max());
    if (truncated < minimum || truncated >= maximum) {
        return 0;
    }
    const auto wide = static_cast<std::int64_t>(truncated);
    return std::bit_cast<i32>(static_cast<u32>(wide));
}

}  // namespace

LegacyBattleScriptCurveResult sample_legacy_battle_script_curve(
    const float frame,
    const std::array<LegacyBattleScriptCurvePoint, 4>& control_points
) noexcept {
    const float parameter =
        static_cast<float>(static_cast<long double>(frame) * 0.05L);
    const float square = parameter * parameter;
    const float cubic = square * parameter;
    const std::array<float, 4> basis{
        basis_value(
            cubic, square, parameter, kNegativeOneSixth, 0.5F, -0.5F, kOneSixth
        ),
        basis_value(cubic, square, parameter, 0.5F, -1.0F, 0.0F, kTwoThirds),
        basis_value(cubic, square, parameter, -0.5F, 0.5F, 0.5F, kOneSixth),
        basis_value(cubic, square, parameter, kOneSixth, 0.0F, 0.0F, 0.0F),
    };

    const long double x =
        static_cast<long double>(control_points[2].x) * basis[2] +
        static_cast<long double>(control_points[1].x) * basis[1] +
        static_cast<long double>(control_points[3].x) * basis[3] +
        static_cast<long double>(control_points[0].x) * basis[0];
    const long double y =
        static_cast<long double>(control_points[0].y) * basis[0] +
        static_cast<long double>(control_points[2].y) * basis[2] +
        static_cast<long double>(control_points[1].y) * basis[1] +
        static_cast<long double>(control_points[3].y) * basis[3];
    const i32 output_x = truncate_x87_low(static_cast<float>(x));
    const i32 output_y = truncate_x87_low(static_cast<float>(y));
    return {
        .x = output_x,
        .y = output_y,
        .return_value = std::bit_cast<u32>(output_y),
    };
}

}  // namespace openswd3::battle
