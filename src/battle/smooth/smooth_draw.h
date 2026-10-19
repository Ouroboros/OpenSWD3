#pragma once

#include <array>
#include <vector>

#include "core/types.h"

namespace swd3
{

struct BattleServices;
struct Picture;
class TswArchive;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): what the parts deciding a battle frame hand to the parts drawing it.
namespace swd3::smooth
{

// A picture a view draws: its archive id and frame, taken again when drawn (a cached picture goes when the cache
// trims for another).
struct PictureRef
{
    u32 id = 0;
    u32 frame = 0;
    bool shown = false;

    const Picture* Get(TswArchive& archive) const;
};

// A picture drawn at (x, y) with `mode`, the painter's options set first as the frame set them (each only when
// marked: the others stay as they are); `motion`: the rebuild's smooth battle reckoning.
struct PictureStep
{
    PictureRef picture;
    int x = 0;
    int y = 0;
    u32 mode = 0;
    bool tinted = false;
    int tintRed = 0;
    int tintGreen = 0;
    int tintBlue = 0;
    bool blended = false;
    int alpha = 0;
    std::array<int, 2> motion = {};
};

using PictureSteps = std::vector<PictureStep>;

// The steps drawn in their order (a step that moves by the next frame says so to the smooth battle's record).
void DrawSteps(BattleServices& services, const PictureSteps& steps);

}  // namespace swd3::smooth
