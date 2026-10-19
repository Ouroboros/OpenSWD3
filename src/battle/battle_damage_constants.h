#pragma once

#include "core/types.h"

// The constants of the spell and attack frames that battle_damage.cpp and smooth/smooth_damage.cpp share.
namespace swd3
{

constexpr i32 kSpellMaxDamage = 0x270F;
constexpr i16 kMagicFadeEnd = -0x20;
constexpr i16 kMagicFadeStep = 8;
}  // namespace swd3
