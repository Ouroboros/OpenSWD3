#pragma once

#include <array>

#include "core/types.h"
#include "platform/input_device.h"

namespace swd3
{

// Keyboard and mouse of the game (the DirectInput object at 0x004B8748): a polled key state by DirectInput scan code
// and a mouse whose axes map to the 640x480 screen with a sensitivity in tenths.
class Input
{
public:
    // DirectInput keyboard scan codes (DIK_*) the game tests directly.
    enum Key : int
    {
        Escape = 0x01,
        Tab = 0x0F,
        Q = 0x10,
        W = 0x11,
        E = 0x12,
        P = 0x19,
        R = 0x13,
        Enter = 0x1C,
        A = 0x1E,
        G = 0x22,
        C = 0x2E,
        LeftControl = 0x1D,
        S = 0x1F,
        D = 0x20,
        F = 0x21,
        H = 0x23,
        J = 0x24,
        K = 0x25,
        L = 0x26,
        Z = 0x2C,
        X = 0x2D,
        V = 0x2F,
        RightShift = 0x36,
        Space = 0x39,
        F1 = 0x3B,
        F2 = 0x3C,
        F3 = 0x3D,
        F4 = 0x3E,
        F5 = 0x3F,
        F6 = 0x40,
        F7 = 0x41,
        F9 = 0x43,
        NumpadMinus = 0x4A,
        NumpadPlus = 0x4E,
        F12 = 0x58,
        RightControl = 0x9D,
        Home = 0xC7,
        Up = 0xC8,
        PageUp = 0xC9,
        Left = 0xCB,
        Right = 0xCD,
        End = 0xCF,
        Down = 0xD0,
        PageDown = 0xD1,
    };

    static constexpr int ScreenRight = 0x27F;
    static constexpr int ScreenBottom = 0x1DF;
    static constexpr std::size_t KeyCount = InputDevice::KeyCount;
    static constexpr u8 KeyDown = InputDevice::KeyDown;
    static constexpr int Tenths = 10;     // the sensitivity is in tenths
    static constexpr int LeftButton = 1;  // ReadMouse
    static constexpr int RightButton = 2;

    explicit Input(InputDevice& device);
    ~Input();
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    int Init();
    int PollKeyboard();
    int IsKeyDown(int scanCode) const;
    bool IsKeyPressed(int scanCode) const;
    int FirstKeyDown() const;
    int PressKey(int scanCode);
    int ReadMouse(int& x, int& y);
    // The rebuild's smooth motion: the screen position ReadMouse would give now, the origin left as it is.
    void PeekMouse(int& x, int& y) const;
    void SetMousePosition(int x, int y);
    void SetSensitivity(double sensitivity);
    void Release();

private:
    InputDevice& device;
    bool acquired = false;
    std::array<u8, KeyCount> keys{};  // KeyDown set while down
    std::array<u8, KeyCount> previousKeys{};
    i32 mouseOriginX = 0;  // axis value at screen x 0
    i32 mouseOriginY = 0;
    i32 sensitivity = Tenths;
};

}  // namespace swd3
