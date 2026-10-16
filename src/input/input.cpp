#include "input/input.h"

#include <cstring>

#include "crt/crt.h"
#include "data/globals.h"

namespace swd3 {

using namespace w32;

namespace {

constexpr DWORD kDirectInputVersion = 0x500;
constexpr DWORD kCooperativeFlags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
constexpr DWORD kKeyboardStateSize = 0x100;
constexpr DWORD kMouseStateSize = 0x1C;
constexpr DWORD kAnyAxis = 0xFFFF03;
constexpr DWORD kAnyButton = 0xFFFF0C;
constexpr DWORD kRelativeAxes = 2;

// c_dfDIKeyboard of dinput.lib (0x00487AC0, in the code section with its object table at 0x00486AC0, which the
// emulated keyboard does not read).
const DIDATAFORMAT kKeyboardFormat{sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), kRelativeAxes,
                                   kKeyboardStateSize, kKeyboardStateSize, nullptr};

// DIPROP_BUFFERSIZE
const GUID* const kPropertyBufferSize = reinterpret_cast<const GUID*>(static_cast<std::uintptr_t>(1));

}  // namespace

// swd32.exe 0x00436F70
Input::Input() {
    m_keyboard = nullptr;
    m_mouse = nullptr;
    m_directInput = nullptr;
    m_sensitivity = 10;
}

// swd32.exe 0x00436FA0
Input::~Input() { Release(); }

// swd32.exe 0x00436FB0: keyboard (c_dfDIKeyboard) and mouse (absolute x/y at 0/4, buttons at 0x0C/0x10, 0x1C
// bytes), both non-exclusive background. The trailing GetDeviceState asks for 0x10 bytes, which DirectInput
// rejects without writing, so the mouse origin is taken from the first 8 key state bytes. Failing to set the
// event notification or the buffer size shows a message and still returns 0 (success).
int Input::Init(HANDLE instance, HWND window) {
    DIOBJECTDATAFORMAT objects[4] = {
        {&G.guidXAxis, 0, kAnyAxis, 0},
        {&G.guidYAxis, 4, kAnyAxis, 0},
        {&G.guidButton, 0xC, kAnyButton, 0},
        {&G.guidButton, 0x10, kAnyButton, 0},
    };
    const DIDATAFORMAT mouseFormat{sizeof(DIDATAFORMAT), sizeof(DIOBJECTDATAFORMAT), DIDF_ABSAXIS, kMouseStateSize, 4,
                                   objects};
    DIPROPDWORD bufferSize{};
    bufferSize.diph.dwSize = sizeof(DIPROPDWORD);
    bufferSize.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    bufferSize.diph.dwObj = 0;
    bufferSize.diph.dwHow = 0;
    bufferSize.dwData = 0x10;
    if (DirectInputCreateA(instance, kDirectInputVersion, &m_directInput, nullptr) < 0) {
        MessageBoxA(window, "Direct Input Failure!", "cDINPUT", MB_OK);
        return 1;
    }
    if (m_directInput->CreateDevice(G.guidSysKeyboard, &m_keyboard, nullptr) < 0 ||
        m_keyboard->SetDataFormat(&kKeyboardFormat) < 0 ||
        m_keyboard->SetCooperativeLevel(window, kCooperativeFlags) < 0 || m_keyboard->Acquire() < 0 ||
        m_directInput->CreateDevice(G.guidSysMouse, &m_mouse, nullptr) < 0 ||
        m_mouse->SetDataFormat(&mouseFormat) < 0 || m_mouse->SetCooperativeLevel(window, kCooperativeFlags) < 0) {
        Release();
        return 1;
    }
    HANDLE event = CreateEventA(nullptr, 0, 0, nullptr);
    if (event == nullptr) {
        MessageBoxA(window, "CreateEvent Error", "CreateEvent", MB_OK);
        return 1;
    }
    if (m_mouse->SetEventNotification(event) < 0) {
        MessageBoxA(window, "SetEventNotification", "CreateEvent", MB_OK);
        return 0;
    }
    if (m_mouse->SetProperty(kPropertyBufferSize, &bufferSize.diph) < 0) {
        MessageBoxA(window, "Set buffer size(SysMouse)", "CreateEvent", MB_OK);
        return 0;
    }
    if (m_mouse->Acquire() < 0) {
        Release();
        return 1;
    }
    m_mouse->GetDeviceState(0x10, m_keys);
    std::int32_t origin[2];
    std::memcpy(origin, m_keys, sizeof(origin));
    m_mouseOriginX = origin[0];
    m_mouseOriginY = origin[1];
    CloseHandle(event);
    return 0;
}

// swd32.exe 0x004372B0
int Input::PollKeyboard() {
    m_keyboard->GetDeviceState(kKeyboardStateSize, m_keys);
    return 1;
}

// swd32.exe 0x004372D0
int Input::IsKeyDown(int scanCode) { return m_keys[scanCode] & 0x80; }

// swd32.exe 0x004372E0: lowest pressed scan code (0 when none, indistinguishable from code 0)
int Input::FirstKeyDown() {
    for (int code = 0; code < 0x100; ++code) {
        if (static_cast<std::int8_t>(m_keys[code]) < 0) {
            return code;
        }
    }
    return 0;
}

// swd32.exe 0x00437300
int Input::PressKey(int scanCode) {
    m_keys[scanCode] |= 0x80;
    return scanCode;
}

// swd32.exe 0x00437310: screen position (sensitivity * axis / 10 from the origin) clamped to 640x480, moving the
// origin along at the edges; returns bit 0 = left, bit 1 = right button
int Input::ReadMouse(int* x, int* y) {
    std::uint8_t state[kMouseStateSize];
    m_mouse->GetDeviceState(kMouseStateSize, state);
    std::int32_t axisX;
    std::int32_t axisY;
    std::memcpy(&axisX, state, 4);
    std::memcpy(&axisY, state + 4, 4);
    const std::int32_t sensitivity = m_sensitivity;
    int screenX = (axisX - m_mouseOriginX) * sensitivity / 10;
    int screenY = (axisY - m_mouseOriginY) * sensitivity / 10;
    int buttons = 0;
    if (state[0xC] & 0x80) {
        buttons = 1;
    }
    if (state[0x10] & 0x80) {
        buttons |= 2;
    }
    if (screenX < 0) {
        screenX = 0;
        m_mouseOriginX = axisX;
    } else if (screenX > 0x27F) {
        screenX = 0x27F;
        m_mouseOriginX = axisX - 0x18F6 / sensitivity;
    }
    if (screenY < 0) {
        screenY = 0;
        m_mouseOriginY = axisY;
    } else if (screenY > 0x1DF) {
        screenY = 0x1DF;
        m_mouseOriginY = axisY - 0x12B6 / sensitivity;
    }
    *x = screenX;
    *y = screenY;
    return buttons;
}

// swd32.exe 0x00437430: moves the origin so that the current axis values map to (x, y) clamped to the screen.
// The original returns the last computed value, which no caller reads.
void Input::SetMousePosition(int x, int y) {
    if (x >= 0x280) {
        x = 0x27F;
    } else if (x <= 0) {
        x = 0;
    }
    if (y >= 0x1E0) {
        y = 0x1DF;
    } else if (y <= 0) {
        y = 0;
    }
    std::uint8_t state[kMouseStateSize];
    m_mouse->GetDeviceState(kMouseStateSize, state);
    std::int32_t axisX;
    std::int32_t axisY;
    std::memcpy(&axisX, state, 4);
    std::memcpy(&axisY, state + 4, 4);
    m_mouseOriginX = axisX - 10 * (x / m_sensitivity);
    m_mouseOriginY = axisY - 10 * (y / m_sensitivity);
}

// swd32.exe 0x004374C0
void Input::SetSensitivity(double sensitivity) {
    m_sensitivity = static_cast<std::int32_t>(crt::ftol(sensitivity * G.tenDouble));
}

// swd32.exe 0x004374E0
void Input::Release() {
    if (m_directInput == nullptr) {
        return;
    }
    if (m_keyboard != nullptr) {
        m_keyboard->Unacquire();
        m_keyboard->Release();
        m_keyboard = nullptr;
    }
    if (m_mouse != nullptr) {
        m_mouse->Unacquire();
        m_mouse->Release();
        m_mouse = nullptr;
    }
    m_directInput->Release();
    m_directInput = nullptr;
}

}  // namespace swd3
