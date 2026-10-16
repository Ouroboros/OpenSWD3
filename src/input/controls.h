#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

class Config;
class GameState;
class Input;

// The state of a key slot or mouse button (0x10 bytes in the original).
struct ButtonState
{
    i32 press = 0;       // +0x00 the click count in the frame the button went down, otherwise 0
    i32 releasedAt = 0;  // +0x04 the frame time it was released, 0 while held
    i32 clicks = 0;      // +0x08 clicks in a row (released for at most 150 ms in between)
    i32 heldFrames = 0;  // +0x0C frames held down
};

// The key slots (named after their default keys, Config::SetDefaultKeys) and the mouse buttons.
enum class Button
{
    Escape = 0,
    Space = 1,
    RightShift = 2,
    Left = 3,
    Up = 4,
    Right = 5,
    Down = 6,
    PageUp = 7,
    PageDown = 8,
    RightControl = 9,
    End = 10,
    Enter = 12,
    MouseRight = 14,
    MouseLeft = 15,
    KeyR = 16,
    KeyA = 17,
    KeyG = 18,
    F1 = 19,
};

// The buttons the game reads each frame: the configured keys and the mouse (0x004B7CB0..0x004B7EAF and the frame
// bookkeeping around them).
class Controls
{
public:
    static constexpr int kButtons = 32;
    static constexpr int kIdleFlag = 9;
    static constexpr u32 kIdleFrames = 0x1C2;
    static constexpr u32 kClickGap = 0x96;
    // The next click count after a press (swd32.exe 0x0049940C).
    static constexpr std::array<i32, 4> kClickSequence = {1, 2, 3, 3};

    void Update(ButtonState& button, int down);
    void Poll(Input& input, const Config& config, GameState& state, i32 frameTime);
    ButtonState& At(Button button);
    const ButtonState& At(Button button) const;

    std::array<ButtonState, kButtons> buttons = {};
    i32 now = 0;              // 0x004CB224 the frame time of this poll
    i32 lastPoll = 0;         // 0x004CB23C
    i32 mouseSuppressed = 0;  // 0x004CAE94 polls left with the left button ignored
    u32 idleFrames = 0;       // 0x004C8BFC polls without pointer movement or buttons
    i32 lastPointerX = 0;     // 0x004ACE68
    i32 lastPointerY = 0;     // 0x004ACE6C
};

}  // namespace swd3
