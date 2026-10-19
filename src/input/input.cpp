#include "input/input.h"

#include <cstring>

#include "crt/msvc.h"
#include "platform/display.h"
#include "platform/input_device.h"

namespace swd3
{

// swd32.exe 0x00436F70: no devices, sensitivity 10 tenths
Input::Input(InputDevice& device)
    : device(device)
{
}

// swd32.exe 0x00436FA0
Input::~Input()
{
    this->Release();
}

// swd32.exe 0x00436FB0: 0 on success. The original's last GetDeviceState asks for a size DirectInput rejects, so the
// mouse origin comes from the first eight bytes of the key state instead of the mouse axes.
int Input::Init()
{
    this->acquired = true;
    i32 origin[2] = {};
    std::memcpy(origin, this->keys.data(), sizeof(origin));
    this->mouseOriginX = origin[0];
    this->mouseOriginY = origin[1];
    return 0;
}

// swd32.exe 0x004372B0 (the rebuild also keeps the previous state for IsKeyPressed)
int Input::PollKeyboard()
{
    this->previousKeys = this->keys;
    this->device.KeyboardState(this->keys);
    return 1;
}

// swd32.exe 0x004372D0
int Input::IsKeyDown(int scanCode) const
{
    return this->keys[static_cast<std::size_t>(scanCode)] & Input::KeyDown;
}

// Whether the key went down since the previous poll (the rebuild's walk/run toggle, see deviations.md 0x00402F80).
bool Input::IsKeyPressed(int scanCode) const
{
    const auto code = static_cast<std::size_t>(scanCode);
    return (this->keys[code] & Input::KeyDown) != 0 && (this->previousKeys[code] & Input::KeyDown) == 0;
}

// swd32.exe 0x004372E0: the lowest pressed scan code; 0 when none (indistinguishable from code 0)
int Input::FirstKeyDown() const
{
    for (int code = 0; code < static_cast<int>(Input::KeyCount); ++code)
    {
        if ((this->keys[static_cast<std::size_t>(code)] & Input::KeyDown) != 0)
        {
            return code;
        }
    }

    return 0;
}

// swd32.exe 0x00437300
int Input::PressKey(int scanCode)
{
    this->keys[static_cast<std::size_t>(scanCode)] |= Input::KeyDown;
    return scanCode;
}

// swd32.exe 0x00437310: the screen position (sensitivity * axis / 10 from the origin) clamped to 640x480, the origin
// following the axes at the edges; bit 0 left, bit 1 right button
int Input::ReadMouse(int& x, int& y)
{
    const MouseState state = this->device.Mouse();
    int screenX = (state.x - this->mouseOriginX) * this->sensitivity / Input::Tenths;
    int screenY = (state.y - this->mouseOriginY) * this->sensitivity / Input::Tenths;
    int buttons = 0;

    if (state.left)
    {
        buttons = Input::LeftButton;
    }

    if (state.right)
    {
        buttons |= Input::RightButton;
    }

    if (screenX < 0)
    {
        screenX = 0;
        this->mouseOriginX = state.x;
    }
    else if (screenX > Input::ScreenRight)
    {
        screenX = Input::ScreenRight;
        this->mouseOriginX = state.x - Input::Tenths * Input::ScreenRight / this->sensitivity;
    }

    if (screenY < 0)
    {
        screenY = 0;
        this->mouseOriginY = state.y;
    }
    else if (screenY > Input::ScreenBottom)
    {
        screenY = Input::ScreenBottom;
        this->mouseOriginY = state.y - Input::Tenths * Input::ScreenBottom / this->sensitivity;
    }

    x = screenX;
    y = screenY;
    return buttons;
}

// swd32.exe 0x00437430: moves the origin so that the current axes map to (x, y) clamped to the screen. The original
// returns its last computed value, which no caller reads.
void Input::SetMousePosition(int x, int y)
{
    if (x >= Display::Width)
    {
        x = Input::ScreenRight;
    }
    else if (x <= 0)
    {
        x = 0;
    }

    if (y >= Display::Height)
    {
        y = Input::ScreenBottom;
    }
    else if (y <= 0)
    {
        y = 0;
    }

    const MouseState state = this->device.Mouse();
    this->mouseOriginX = state.x - Input::Tenths * (x / this->sensitivity);
    this->mouseOriginY = state.y - Input::Tenths * (y / this->sensitivity);
}

// swd32.exe 0x004374C0: tenths, truncated
void Input::SetSensitivity(double sensitivity)
{
    this->sensitivity = static_cast<i32>(crt::Ftol(sensitivity * Input::Tenths));
}

// swd32.exe 0x004374E0
void Input::Release()
{
    this->acquired = false;
}

}  // namespace swd3
