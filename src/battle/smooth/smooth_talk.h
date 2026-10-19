#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class Surface;
class Talk;
struct TalkWindow;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the talk windows (Talk::Draw) split into the part deciding the frame
// (the windows advanced and their text drawn into the talk surface) and the part drawing it on the screen.
namespace swd3::smooth
{

class TalkFrame
{
public:
    explicit TalkFrame(Talk& talk);

    void DrawWindowAdvance(TalkWindow& window, Surface& surface, u16* pixels, i32 heldAdvance, Scene& scene);

    void DrawAdvance(Scene& scene);

private:
    Talk& talk;
};

}  // namespace swd3::smooth
