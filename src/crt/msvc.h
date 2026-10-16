#pragma once

#include "core/types.h"

// The parts of the statically linked MSVC runtime whose results the game can observe.
namespace swd3::crt
{

i64 Ftol(double value);
double Difftime(i32 end, i32 start);

}  // namespace swd3::crt
