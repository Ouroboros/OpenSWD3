#include "input/controls.h"

#include "config/config.h"
#include "game/game_state.h"
#include "input/input.h"

namespace swd3
{

namespace
{

constexpr int kConfirmFrames = 2;  // PressConfirm holds Space and the left button this long
constexpr i32 kRepeatDelay = 0xA;  // Repeating: held frames before the button repeats

// The key slots in the order the poll updates them (the mouse buttons follow).
constexpr std::array<Button, 16> kPollOrder = {
    Button::Escape,
    Button::Space,
    Button::Enter,
    Button::RightShift,
    Button::Left,
    Button::Up,
    Button::Right,
    Button::Down,
    Button::RightControl,
    Button::End,
    Button::PageDown,
    Button::PageUp,
    Button::KeyR,
    Button::KeyA,
    Button::KeyG,
    Button::F1,
};

}  // namespace

// swd32.exe 0x004053C0: a press after a release advances the click count; a release longer than 150 ms ends the
// clicks in a row
void Controls::Update(ButtonState& button, int down)
{
    if (down != 0)
    {
        if (button.releasedAt != 0)
        {
            const i32 next = Controls::ClickSequence[static_cast<std::size_t>(button.clicks)];
            button.heldFrames = 0;
            button.press = next;
            button.clicks = next;
        }

        button.releasedAt = 0;
        ++button.heldFrames;
        return;
    }

    button.heldFrames = 0;

    if (button.releasedAt == 0)
    {
        button.releasedAt = this->now;
        return;
    }

    button.press = 0;

    if (static_cast<u32>(this->now - button.releasedAt) > Controls::ClickGap)
    {
        button.clicks = 0;
    }
}

// swd32.exe 0x004050E0: the pointer and buttons of this frame; game flag 9 marks a pointer left alone for 450 polls
void Controls::Poll(Input& input, const Config& config, GameState& state, i32 frameTime)
{
    this->now = frameTime;
    const int mouse = input.ReadMouse(state.pointerX, state.pointerY);

    for (const Button button : kPollOrder)
    {
        const auto slot = static_cast<std::size_t>(button);
        this->Update(this->buttons[slot], input.IsKeyDown(config.keys[slot]));
    }

    this->Update(this->At(Button::MouseLeft), mouse & Input::LeftButton);
    this->Update(this->At(Button::MouseRight), mouse & Input::RightButton);
    this->lastPoll = this->now;

    if (this->mouseSuppressed != 0)
    {
        this->At(Button::MouseLeft) = ButtonState{};
        --this->mouseSuppressed;
    }

    state.ClearGameFlag(Controls::IdleFlag);

    if (state.pointerX == this->lastPointerX && state.pointerY == this->lastPointerY && mouse == 0)
    {
        if (++this->idleFrames > Controls::IdleFrames)
        {
            state.SetGameFlag(Controls::IdleFlag);
        }
    }
    else
    {
        this->idleFrames = 0;
    }

    this->lastPointerX = state.pointerX;
    this->lastPointerY = state.pointerY;
}

ButtonState& Controls::At(Button button)
{
    return this->buttons[static_cast<std::size_t>(button)];
}

const ButtonState& Controls::At(Button button) const
{
    return this->buttons[static_cast<std::size_t>(button)];
}

// swd32.exe 0x00464DA0: Space pressed and held two frames, the left button held two frames, Enter held one frame (the
// battle menus confirm with it); 2 is returned
int Controls::PressConfirm()
{
    ButtonState& space = this->At(Button::Space);
    space.press = 1;
    space.heldFrames = kConfirmFrames;
    this->At(Button::MouseLeft).heldFrames = kConfirmFrames;
    this->At(Button::Enter).heldFrames = 1;
    return kConfirmFrames;
}

// swd32.exe 0x0043C090: the button counts this frame: pressed in its first frame held, then every second frame after
// ten
bool Controls::Repeating(Button button) const
{
    const ButtonState& state = this->At(button);
    return state.press != 0 &&
           (state.heldFrames == 1 || (state.heldFrames > kRepeatDelay && (state.heldFrames & 1) == 0));
}

}  // namespace swd3
