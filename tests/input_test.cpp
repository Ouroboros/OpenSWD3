#include <array>

#include "core/types.h"
#include "input/input.h"
#include "platform/input_device.h"
#include "test.h"

using swd3::u8;

namespace
{

class FakeDevice : public swd3::InputDevice
{
public:
    void KeyboardState(std::array<u8, 256>& keys) override
    {
        keys = this->keys;
    }

    swd3::MouseState Mouse() override
    {
        return this->mouse;
    }

    std::array<u8, 256> keys{};
    swd3::MouseState mouse;
};

}  // namespace

TEST_CASE("input: polled keys and the lowest pressed key")
{
    FakeDevice device;
    swd3::Input input(device);
    CHECK_EQ(input.Init(), 0);
    device.keys[0x1C] = 0x80;
    device.keys[0xC8] = 0x80;
    CHECK_EQ(input.IsKeyDown(0x1C), 0);
    input.PollKeyboard();
    CHECK_EQ(input.IsKeyDown(0x1C), 0x80);
    CHECK_EQ(input.FirstKeyDown(), 0x1C);
    input.PressKey(0x01);
    CHECK_EQ(input.FirstKeyDown(), 0x01);
}

TEST_CASE("input: mouse follows the pointer at the default sensitivity and drags the origin at the edges")
{
    FakeDevice device;
    swd3::Input input(device);
    input.Init();
    int x = 0;
    int y = 0;
    device.mouse = {320, 200, true, false};
    CHECK_EQ(input.ReadMouse(x, y), 1);
    CHECK_EQ(x, 320);
    CHECK_EQ(y, 200);
    device.mouse = {700, 500, false, true};
    CHECK_EQ(input.ReadMouse(x, y), 2);
    CHECK_EQ(x, 639);
    CHECK_EQ(y, 479);
    device.mouse = {650, 490, false, false};
    input.ReadMouse(x, y);
    CHECK_EQ(x, 589);
    CHECK_EQ(y, 469);
    input.SetSensitivity(2.0);
    input.SetMousePosition(100, 50);
    input.ReadMouse(x, y);
    CHECK_EQ(x, 100);
    // 50 / 20 truncates to 2 in SetMousePosition, as in the original.
    CHECK_EQ(y, 40);
}

TEST_CASE("input: SDL scancodes map to DirectInput codes")
{
    CHECK_EQ(swd3::ScanCodeOf(41), 0x01);
    CHECK_EQ(swd3::ScanCodeOf(40), 0x1C);
    CHECK_EQ(swd3::ScanCodeOf(82), 0xC8);
    CHECK_EQ(swd3::ScanCodeOf(4), 0x1E);
    CHECK_EQ(swd3::ScanCodeOf(1000), 0);
}
