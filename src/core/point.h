#pragma once

#include "core/types.h"

namespace swd3
{

// A place in pixels, or the distance between two places.
struct Point
{
    i32 x = 0;
    i32 y = 0;

    bool operator==(const Point&) const = default;
};

}  // namespace swd3
