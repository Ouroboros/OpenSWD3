#pragma once

#include <array>
#include <span>

#include "core/types.h"

namespace swd3
{

// The cubic B-spline of the battle talk: four weights from a basis matrix, and points weighted by them.
namespace spline
{

using Matrix = std::array<float, 16>;  // row major, 4 x 4
using Weights = std::array<float, 4>;

struct Point
{
    float x = 0;
    float y = 0;
};

using Points = std::array<Point, 4>;

// swd32.exe 0x004A6474: the uniform cubic B-spline basis
constexpr Matrix Basis = {
    -1.0F / 6,
    0.5F,
    -0.5F,
    1.0F / 6,
    0.5F,
    -1.0F,
    0.5F,
    0,
    -0.5F,
    0,
    0.5F,
    0,
    1.0F / 6,
    2.0F / 3,
    1.0F / 6,
    0,
};

Weights WeighColumns(const Weights& vector, const Matrix& matrix);
Weights BasisWeights(float t, const Matrix& matrix);
Point Weighted(const Weights& weights, const Points& points);
Point Evaluate(float t, std::span<const Point> points);

}  // namespace spline

void TalkCurvePoint(float step, i32& x, i32& y, const std::array<std::array<i16, 2>, 4>& points);

}  // namespace swd3
