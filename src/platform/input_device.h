#pragma once

#include <array>

#include "core/types.h"

union SDL_Event;

namespace swd3
{

class Display;

// The state the game reads from DirectInput: the keyboard as 256 bytes indexed by DirectInput scan code (bit 7 set
// while down) and the mouse as absolute axes with two buttons.
struct MouseState
{
    i32 x = 0;
    i32 y = 0;
    bool left = false;
    bool right = false;
};

class InputDevice
{
public:
    static constexpr std::size_t KeyCount = 256;  // a DIK_* key state per scan code
    static constexpr u8 KeyDown = 0x80;
    using KeyState = std::array<u8, KeyCount>;

    virtual ~InputDevice() = default;
    virtual void KeyboardState(KeyState& keys) = 0;
    virtual MouseState Mouse() = 0;
};

// Keyboard and mouse of the game window. The mouse axes are the pointer position on the logical screen, so that at
// the default sensitivity the game's cursor stays under the pointer (DirectInput reports accumulated motion).
class WindowInputDevice : public InputDevice
{
public:
    explicit WindowInputDevice(const Display& display);

    void HandleEvent(const SDL_Event& event);
    void KeyboardState(KeyState& keys) override;
    MouseState Mouse() override;

private:
    const Display& display;
    KeyState keys{};
    MouseState mouse;
};

// The DirectInput scan code (DIK_*) of an SDL scancode, 0 for keys without one.
u8 ScanCodeOf(int sdlScancode);

}  // namespace swd3
