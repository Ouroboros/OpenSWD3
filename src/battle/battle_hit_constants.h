#pragma once

#include <array>
#include <utility>

#include "core/types.h"

// The constants of the hit frames that battle_hit.cpp and smooth/smooth_hit.cpp share.
namespace swd3
{

constexpr i8 kHitFadeEnd = -0x20;
constexpr i8 kHitFadeStep = 4;
constexpr i32 kRippleHold = 0x28;
constexpr u16 kRippleSound = 0x142;
}  // namespace swd3
