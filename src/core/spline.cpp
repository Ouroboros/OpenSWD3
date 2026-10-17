#include "core/spline.h"

#include <algorithm>
#include <cmath>

#include "crt/msvc.h"

namespace swd3
{

namespace
{

constexpr double kSegmentBase = 2.0;  // the first point of a segment is two before it
constexpr double kTalkStep = 0.05;    // swd32.exe 0x00499D20: TalkCurvePoint goes 1/20 along the curve a step

}  // namespace

namespace spline
{

// swd32.exe 0x00437EB0: each column of `matrix` weighted by `vector` (rows 0, 1, 3, 2 summed in that order)
Weights WeighColumns(const Weights& vector, const Matrix& matrix)
{
    Weights out = {};

    for (std::size_t column = 0; column < out.size(); ++column)
    {
        const double first = static_cast<double>(matrix[column]) * vector[0];
        const double second = static_cast<double>(matrix[4 + column]) * vector[1];
        out[column] = static_cast<float>(
            first + second + static_cast<double>(matrix[12 + column]) * vector[3] +
            static_cast<double>(matrix[8 + column]) * vector[2]
        );
    }

    return out;
}

// swd32.exe 0x00437F40: the weights of (t^3, t^2, t, 1) by `matrix`
Weights BasisWeights(float t, const Matrix& matrix)
{
    const double square = static_cast<double>(t) * t;
    const Weights powers = {static_cast<float>(square * t), static_cast<float>(square), t, 1.0F};
    return WeighColumns(powers, matrix);
}

// swd32.exe 0x00437EF0: the four points summed by `weights` (x: points 2, 1, 3, 0; y: 0, 2, 1, 3)
Point Weighted(const Weights& weights, const Points& points)
{
    Point out;
    out.x = static_cast<float>(
        static_cast<double>(points[2].x) * weights[2] + static_cast<double>(points[1].x) * weights[1] +
        static_cast<double>(points[3].x) * weights[3] + static_cast<double>(weights[0]) * points[0].x
    );
    out.y = static_cast<float>(
        static_cast<double>(points[0].y) * weights[0] + static_cast<double>(points[2].y) * weights[2] +
        static_cast<double>(points[1].y) * weights[1] + static_cast<double>(points[3].y) * weights[3]
    );
    return out;
}

// swd32.exe 0x00437F90: the point of the curve through `points` at `t` (its whole part picks the segment of four
// points from the start, the rest the place on it)
Point Evaluate(float t, std::span<const Point> points)
{
    const double whole = std::floor(static_cast<double>(t));
    const auto segment = static_cast<std::size_t>(crt::Ftol(whole + kSegmentBase));
    const auto rest = static_cast<float>(static_cast<double>(t) - std::floor(static_cast<double>(t)));
    const Weights weights = BasisWeights(rest, Basis);
    Points four;
    std::copy_n(points.begin() + static_cast<std::ptrdiff_t>(segment - 2), four.size(), four.begin());
    return Weighted(weights, four);
}

}  // namespace spline

// swd32.exe 0x0046E290: the point of the battle talk's curve through four points at `step` twentieths (the original
// also evaluates the curve at 0 and drops it)
void TalkCurvePoint(float step, i32& x, i32& y, const std::array<std::array<i16, 2>, 4>& points)
{
    spline::Points curve;

    for (std::size_t at = 0; at < curve.size(); ++at)
    {
        curve[at] = {static_cast<float>(points[at][0]), static_cast<float>(points[at][1])};
    }

    spline::Evaluate(0.0F, curve);
    const auto t = static_cast<float>(static_cast<double>(step) * kTalkStep);
    const spline::Point point = spline::Weighted(spline::BasisWeights(t, spline::Basis), curve);
    x = static_cast<i32>(crt::Ftol(point.x));
    y = static_cast<i32>(crt::Ftol(point.y));
}

}  // namespace swd3
