#include <array>

#include <SDL3/SDL_scancode.h>

#include "config/config.h"
#include "core/types.h"
#include "game/game_state.h"
#include "input/controls.h"
#include "input/input.h"
#include "platform/input_device.h"
#include "test.h"

using swd3::u8;

namespace
{

class FakeDevice : public swd3::InputDevice
{
public:
    void KeyboardState(KeyState& keys) override
    {
        keys = this->keys;
    }

    swd3::MouseState Mouse() override
    {
        return this->mouse;
    }

    KeyState keys{};
    swd3::MouseState mouse;
};

constexpr int kInsideX = 320;  // a pointer inside the screen
constexpr int kInsideY = 200;
constexpr int kPastRight = 700;  // and past its right and bottom edges
constexpr int kPastBottom = 500;
constexpr int kBackX = 50;
constexpr int kBackY = 10;
constexpr int kSensitivity = 20;  // in tenths
constexpr int kTargetX = 100;
constexpr int kTargetY = 50;
constexpr int kUnknownScancode = 1000;
constexpr swd3::i32 kStart = 1000;  // milliseconds
constexpr swd3::i32 kStep = 50;
constexpr int kSecondFrame = 2;
constexpr swd3::u32 kPollStart = 10;
constexpr swd3::u32 kPollStep = 10;

}  // namespace

TEST_CASE("input: polled keys and the lowest pressed key")
{
    using swd3::Input;
    FakeDevice device;
    Input input(device);
    CHECK_EQ(input.Init(), 0);
    device.keys[Input::Enter] = Input::KeyDown;
    device.keys[Input::Up] = Input::KeyDown;
    CHECK_EQ(input.IsKeyDown(Input::Enter), 0);
    input.PollKeyboard();
    CHECK_EQ(input.IsKeyDown(Input::Enter), Input::KeyDown);
    CHECK_EQ(input.FirstKeyDown(), Input::Enter);
    input.PressKey(Input::Escape);
    CHECK_EQ(input.FirstKeyDown(), Input::Escape);
}

TEST_CASE("input: mouse follows the pointer at the default sensitivity and drags the origin at the edges")
{
    using swd3::Input;
    FakeDevice device;
    Input input(device);
    input.Init();
    int x = 0;
    int y = 0;
    device.mouse = {kInsideX, kInsideY, true, false};
    CHECK_EQ(input.ReadMouse(x, y), Input::LeftButton);
    CHECK_EQ(x, kInsideX);
    CHECK_EQ(y, kInsideY);
    device.mouse = {kPastRight, kPastBottom, false, true};
    CHECK_EQ(input.ReadMouse(x, y), Input::RightButton);
    CHECK_EQ(x, Input::ScreenRight);
    CHECK_EQ(y, Input::ScreenBottom);
    device.mouse = {kPastRight - kBackX, kPastBottom - kBackY, false, false};
    input.ReadMouse(x, y);
    CHECK_EQ(x, Input::ScreenRight - kBackX);
    CHECK_EQ(y, Input::ScreenBottom - kBackY);
    input.SetSensitivity(static_cast<double>(kSensitivity) / Input::Tenths);
    input.SetMousePosition(kTargetX, kTargetY);
    input.ReadMouse(x, y);
    CHECK_EQ(x, kTargetX);
    // The target over the sensitivity truncates in SetMousePosition, as in the original.
    CHECK_EQ(y, kTargetY / kSensitivity * kSensitivity);
}

TEST_CASE("input: SDL scancodes map to DirectInput codes")
{
    using swd3::Input;
    CHECK_EQ(swd3::ScanCodeOf(SDL_SCANCODE_ESCAPE), Input::Escape);
    CHECK_EQ(swd3::ScanCodeOf(SDL_SCANCODE_RETURN), Input::Enter);
    CHECK_EQ(swd3::ScanCodeOf(SDL_SCANCODE_UP), Input::Up);
    CHECK_EQ(swd3::ScanCodeOf(SDL_SCANCODE_A), Input::A);
    CHECK_EQ(swd3::ScanCodeOf(kUnknownScancode), 0);
}

TEST_CASE("controls: clicks in a row and the 150 ms gap")
{
    using swd3::Controls;
    Controls controls;
    swd3::ButtonState button;
    controls.now = kStart;
    controls.Update(button, 0);
    CHECK_EQ(button.releasedAt, kStart);
    controls.Update(button, 1);
    CHECK_EQ(button.press, Controls::ClickSequence[0]);
    CHECK_EQ(button.heldFrames, 1);
    controls.Update(button, 1);
    CHECK_EQ(button.press, Controls::ClickSequence[0]);
    CHECK_EQ(button.heldFrames, kSecondFrame);
    controls.now = kStart + kStep;
    controls.Update(button, 0);
    controls.now = kStart + kSecondFrame * kStep;
    controls.Update(button, 0);
    CHECK_EQ(button.press, 0);
    CHECK_EQ(button.clicks, 1);
    controls.Update(button, 1);
    CHECK_EQ(button.press, Controls::ClickSequence[1]);
    controls.Update(button, 0);
    controls.now += static_cast<swd3::i32>(Controls::ClickGap) + kStep;
    controls.Update(button, 0);
    CHECK_EQ(button.clicks, 0);
    controls.Update(button, 1);
    CHECK_EQ(button.press, Controls::ClickSequence[0]);
}

TEST_CASE("controls: the poll reads the configured keys, the mouse and the idle pointer")
{
    using swd3::Button;
    using swd3::Controls;
    FakeDevice device;
    swd3::Input input(device);
    CHECK_EQ(input.Init(), 0);
    swd3::Config config;
    config.SetDefaultKeys();
    swd3::GameState state;
    Controls controls;
    swd3::u32 now = kPollStart;
    controls.Poll(input, config, state, now);
    const swd3::u32 firstPoll = now;
    device.keys[swd3::Input::Enter] = swd3::Input::KeyDown;
    device.mouse.left = true;
    input.PollKeyboard();
    now += kPollStep;
    controls.Poll(input, config, state, now);
    CHECK_EQ(controls.At(Button::Enter).heldFrames, 1);
    CHECK_EQ(controls.At(Button::Enter).press, Controls::ClickSequence[0]);
    CHECK_EQ(controls.At(Button::MouseLeft).press, Controls::ClickSequence[0]);
    CHECK_EQ(controls.At(Button::Escape).releasedAt, static_cast<swd3::i32>(firstPoll));
    controls.mouseSuppressed = 1;
    now += kPollStep;
    controls.Poll(input, config, state, now);
    CHECK_EQ(controls.At(Button::MouseLeft).heldFrames, 0);
    CHECK_EQ(controls.mouseSuppressed, 0);
    device.mouse.left = false;

    for (swd3::u32 poll = 0; poll < Controls::IdleFrames; ++poll)
    {
        controls.Poll(input, config, state, ++now);
    }

    CHECK(!state.GameFlag(Controls::IdleFlag));
    controls.Poll(input, config, state, ++now);
    CHECK(state.GameFlag(Controls::IdleFlag));
}
