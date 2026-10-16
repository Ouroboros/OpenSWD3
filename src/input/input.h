#pragma once

#include <cstdint>

#include "swd3/dinput.h"

namespace swd3 {

// DirectInput keyboard and mouse (global object at 0x004B8748, size 0x118). Returns of Init: 0 on success.
class Input {
public:
    Input();
    ~Input();
    int Init(w32::HANDLE instance, w32::HWND window);
    int PollKeyboard();
    int IsKeyDown(int scanCode);
    int FirstKeyDown();
    int PressKey(int scanCode);
    int ReadMouse(int* x, int* y);
    void SetMousePosition(int x, int y);
    void SetSensitivity(double sensitivity);
    void Release();

    std::uint8_t m_keys[0x100];                 // +0x000 keyboard state (bit 7 = down)
    w32::IDirectInputA* m_directInput;          // +0x100
    w32::IDirectInputDeviceA* m_keyboard;       // +0x104
    w32::IDirectInputDeviceA* m_mouse;          // +0x108
    std::int32_t m_mouseOriginX;                // +0x10C absolute mouse axis value at screen x 0
    std::int32_t m_mouseOriginY;                // +0x110
    std::int32_t m_sensitivity;                 // +0x114 tenths
};

static_assert(sizeof(Input) == 0x118);

}  // namespace swd3
