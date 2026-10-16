#include "input/input.h"

#include <cstring>

#include "crt/msvc.h"
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

// swd32.exe 0x004372B0
int Input::PollKeyboard()
{
    this->device.KeyboardState(this->keys);
    return 1;
}

// swd32.exe 0x004372D0
int Input::IsKeyDown(int scanCode) const
{
    return this->keys[static_cast<std::size_t>(scanCode)] & 0x80;
}

// swd32.exe 0x004372E0: the lowest pressed scan code; 0 when none (indistinguishable from code 0)
int Input::FirstKeyDown() const
{
    for (int code = 0; code < 0x100; ++code)
    {
        if ((this->keys[static_cast<std::size_t>(code)] & 0x80) != 0)
        {
            return code;
        }
    }

    return 0;
}

// swd32.exe 0x00437300
int Input::PressKey(int scanCode)
{
    this->keys[static_cast<std::size_t>(scanCode)] |= 0x80;
    return scanCode;
}

// swd32.exe 0x00437310: the screen position (sensitivity * axis / 10 from the origin) clamped to 640x480, the origin
// following the axes at the edges; bit 0 left, bit 1 right button
int Input::ReadMouse(int& x, int& y)
{
    const MouseState state = this->device.Mouse();
    int screenX = (state.x - this->mouseOriginX) * this->sensitivity / 10;
    int screenY = (state.y - this->mouseOriginY) * this->sensitivity / 10;
    int buttons = 0;

    if (state.left)
    {
        buttons = 1;
    }

    if (state.right)
    {
        buttons |= 2;
    }

    if (screenX < 0)
    {
        screenX = 0;
        this->mouseOriginX = state.x;
    }
    else if (screenX > Input::kScreenRight)
    {
        screenX = Input::kScreenRight;
        this->mouseOriginX = state.x - 0x18F6 / this->sensitivity;
    }

    if (screenY < 0)
    {
        screenY = 0;
        this->mouseOriginY = state.y;
    }
    else if (screenY > Input::kScreenBottom)
    {
        screenY = Input::kScreenBottom;
        this->mouseOriginY = state.y - 0x12B6 / this->sensitivity;
    }

    x = screenX;
    y = screenY;
    return buttons;
}

// swd32.exe 0x00437430: moves the origin so that the current axes map to (x, y) clamped to the screen. The original
// returns its last computed value, which no caller reads.
void Input::SetMousePosition(int x, int y)
{
    if (x >= 0x280)
    {
        x = Input::kScreenRight;
    }
    else if (x <= 0)
    {
        x = 0;
    }

    if (y >= 0x1E0)
    {
        y = Input::kScreenBottom;
    }
    else if (y <= 0)
    {
        y = 0;
    }

    const MouseState state = this->device.Mouse();
    this->mouseOriginX = state.x - 10 * (x / this->sensitivity);
    this->mouseOriginY = state.y - 10 * (y / this->sensitivity);
}

// swd32.exe 0x004374C0: tenths, truncated
void Input::SetSensitivity(double sensitivity)
{
    this->sensitivity = static_cast<i32>(crt::Ftol(sensitivity * 10.0));
}

// swd32.exe 0x004374E0
void Input::Release()
{
    this->acquired = false;
}

}  // namespace swd3
