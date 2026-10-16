#pragma once

#include <cstdint>

// Window frames and filled boxes of the menus, dialogs and battle panels.
namespace swd3 {

// Modes of FillBoxMode.
enum FillMode {
    kFillBlend = 0,     // blended with the colour
    kFillTint = 1,      // TintPixels by (red, green, blue)
    kFillQuarter = 2,   // a quarter of the brightness
    kFillGray = 3,      // GrayPixels
    kFillEighth = 4,    // an eighth of the brightness
    kFillGradient = 5,  // tint growing from the top and bottom edges towards the middle
};

void FillBoxMode(int x, int y, int width, int height, int red, int green, int blue, int mode);
void DrawWindowFrame(int id, int left, int top, int right, int bottom, int alpha, int border);

}  // namespace swd3
