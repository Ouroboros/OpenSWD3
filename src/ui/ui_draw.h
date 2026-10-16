#pragma once

#include "core/types.h"

namespace swd3
{

class Painter;
class TswArchive;

// The modes of FillBox.
enum class FillMode
{
    Blend = 0,     // blended with the colour
    Tint = 1,      // tinted by (red, green, blue)
    Quarter = 2,   // a quarter of the brightness
    Gray = 3,      // grey
    Eighth = 4,    // an eighth of the brightness
    Gradient = 5,  // a tint growing from the top and bottom edges towards the middle
};

void FillBox(Painter& painter, int x, int y, int width, int height, int red, int green, int blue, FillMode mode);
void DrawWindowFrame(
    Painter& painter,
    TswArchive& archive,
    int id,
    int left,
    int top,
    int right,
    int bottom,
    int alpha,
    int border
);

}  // namespace swd3
