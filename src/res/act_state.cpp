#include "res/act_state.h"

#include <type_traits>

namespace swd3
{

// swd32.exe 0x0040DC00
void ResetPlayback(ActState& state)
{
    state.unknown1C = -1;
    state.savedAction = -1;
    state.savedDirection = -1;
    state.speed = 0;
    state.frameDelay = 0;
    state.delay = 0;
    state.position = 0;
    state.hold = 0;
}

// A state memset to 0xFF in the original (every field all ones; the script stays unloaded).
void FillUnused(ActState& state)
{
    auto ones = [](auto& field)
    {
        using Field = std::remove_cvref_t<decltype(field)>;
        field = static_cast<Field>(-1);
    };

    ones(state.id);
    ones(state.lastId);
    ones(state.action);
    ones(state.lastAction);
    ones(state.x);
    ones(state.y);
    ones(state.flags);
    ones(state.unknown1C);
    ones(state.savedAction);
    ones(state.effectA);
    ones(state.effectQ);
    ones(state.width);
    ones(state.height);
    ones(state.direction);
    ones(state.lastDirection);
    ones(state.savedDirection);
    ones(state.counter);
    ones(state.position);
    ones(state.delay);
    ones(state.frameDelay);
    ones(state.speed);
    ones(state.frame);
    ones(state.picture);
    ones(state.rotation);
    ones(state.actionCode);
    ones(state.unknown52);
    ones(state.view);
    ones(state.target);
    ones(state.unknown5C);
    ones(state.offsetX);
    ones(state.offsetY);
    ones(state.drawLevel);
    ones(state.red);
    ones(state.green);
    ones(state.blue);
    ones(state.red2);
    ones(state.green2);
    ones(state.blue2);
    ones(state.anchorX);
    ones(state.anchorY);
    ones(state.wait);
    ones(state.unknown89);
    ones(state.signal);
    ones(state.unknown8B);
    ones(state.waiting);
    ones(state.hold);
    ones(state.mask);

    for (u16& value : state.unknown6A)
    {
        ones(value);
    }

    for (u16& value : state.lights)
    {
        ones(value);
    }

    state.script.reset();
}

}  // namespace swd3
