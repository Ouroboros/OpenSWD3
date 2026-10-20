#pragma once

#include <span>

#include "core/types.h"

namespace swd3
{

class Surface;

// The largest render scale woven (DisplaySettings::MaximumRenderScale).
constexpr int MaxWeaveScale = 8;

// The rebuild's render scale (deviations.md): the N x N logical screens of a render scale N woven into one: screen
// b * N + a gives the pixels of column a and row b of every block (the map and the battle drawn between game frames).
void Weave(std::span<const Surface* const> parts, int scale, std::span<Pixel> to);

}  // namespace swd3
