#pragma once

#include <array>

#include "core/types.h"

namespace swd3
{

class InputDevice;

// Keyboard and mouse of the game (the DirectInput object at 0x004B8748): a polled key state by DirectInput scan code
// and a mouse whose axes map to the 640x480 screen with a sensitivity in tenths.
class Input
{
public:
    static constexpr int kScreenRight = 0x27F;
    static constexpr int kScreenBottom = 0x1DF;

    explicit Input(InputDevice& device);
    ~Input();
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    int Init();
    int PollKeyboard();
    int IsKeyDown(int scanCode) const;
    int FirstKeyDown() const;
    int PressKey(int scanCode);
    int ReadMouse(int& x, int& y);
    void SetMousePosition(int x, int y);
    void SetSensitivity(double sensitivity);
    void Release();

private:
    InputDevice& device;
    bool acquired = false;
    std::array<u8, 256> keys{};  // bit 7 set while down
    i32 mouseOriginX = 0;        // axis value at screen x 0
    i32 mouseOriginY = 0;
    i32 sensitivity = 10;  // tenths
};

}  // namespace swd3
