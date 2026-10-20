#pragma once

#include "core/types.h"

namespace swd3
{

// The quotient rounded down (towards minus infinity) and up.
inline i32 FloorDivide(i32 value, i32 divisor)
{
    const i32 quotient = value / divisor;
    return value % divisor != 0 && (value < 0) != (divisor < 0) ? quotient - 1 : quotient;
}

inline i32 CeilDivide(i32 value, i32 divisor)
{
    return -FloorDivide(-value, divisor);
}

}  // namespace swd3
