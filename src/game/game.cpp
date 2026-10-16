#include "game/game.h"

#include <utility>

#include <SDL3/SDL.h>

#include "save/save_games.h"

namespace swd3
{

namespace
{

constexpr u32 kLeftButton = 0x1;   // MK_LBUTTON
constexpr u32 kRightButton = 0x2;  // MK_RBUTTON
constexpr u32 kShiftKey = 0x4;     // MK_SHIFT
constexpr u32 kControlKey = 0x8;   // MK_CONTROL
constexpr u32 kMiddleButton = 0x10;
constexpr Uint32 kLauncherWait = 16;

// The wParam of a mouse message: the buttons and the modifier keys held.
u32 MouseKeys(SDL_MouseButtonFlags buttons)
{
    u32 keys = 0;
    keys |= (buttons & SDL_BUTTON_LMASK) != 0 ? kLeftButton : 0;
    keys |= (buttons & SDL_BUTTON_RMASK) != 0 ? kRightButton : 0;
    keys |= (buttons & SDL_BUTTON_MMASK) != 0 ? kMiddleButton : 0;
    const SDL_Keymod modifiers = SDL_GetModState();
    keys |= (modifiers & SDL_KMOD_SHIFT) != 0 ? kShiftKey : 0;
    keys |= (modifiers & SDL_KMOD_CTRL) != 0 ? kControlKey : 0;
    return keys;
}

}  // namespace

Game::Game(std::string installDirectory) : screen(Display::kWidth, Display::kHeight)
{
    this->config.installDirectory = std::move(installDirectory);
}

// swd32.exe 0x00409EC0: creates the window, sends itself the start message and runs the message loop
int Game::Run()
{
    if (!this->display.Open(Game::kWindowTitle))
    {
        return 0;
    }

    this->Start();

    while (!this->quit)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                this->quit = true;
            }
            else
            {
                this->display.HandleEvent(event);
            }
        }

        this->display.Present(this->screen);
        SDL_Delay(kLauncherWait);
    }

    return 0;
}

// swd32.exe 0x0040A0D0 (message 0x404): the start sound, the default keys, the saved game flag and the launcher
void Game::Start()
{
    if (!this->resources.Open(this->config.installDirectory + Game::kExecutable))
    {
        this->errors.ShowError("swd3r", "swd32.exe not found in the game directory");
        this->quit = true;
        return;
    }

    this->systemSound.Play(this->resources.Find("WAVE", Game::kStartSound));
    this->config.SetDefaultKeys();
    this->state.ClearGameFlag(Launcher::kSavedGameFlag);

    if (SaveGames::AnyExists(this->config.installDirectory))
    {
        this->state.SetGameFlag(Launcher::kSavedGameFlag);
    }

    const Launcher::Choice choice = this->RunLauncher();

    if (choice == Launcher::Choice::Exit)
    {
        this->quit = true;
    }
}

// DialogBoxParamA(0x6B): the launcher drawn in the game window until it ends
Launcher::Choice Game::RunLauncher()
{
    Launcher launcher(this->resources, this->state, this->systemSound, this->config.installDirectory);
    launcher.LoadPictures();
    launcher.Init();
    Launcher::Choice choice = Launcher::Choice::None;

    while (choice == Launcher::Choice::None)
    {
        SDL_Event event;

        if (SDL_WaitEventTimeout(&event, static_cast<Sint32>(kLauncherWait)))
        {
            do
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    choice = launcher.OnClose();
                }
                else if (
                    event.type == SDL_EVENT_MOUSE_MOTION ||
                    (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
                )
                {
                    const float windowX = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                    const float windowY = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                    const LogicalPoint point = this->display.WindowToLogical(windowX, windowY);
                    const Launcher::Choice result =
                        launcher.OnMouse(MouseKeys(SDL_GetMouseState(nullptr, nullptr)), point.x, point.y);

                    if (result != Launcher::Choice::None)
                    {
                        choice = result;
                    }
                }
                else
                {
                    this->display.HandleEvent(event);
                }
            } while (choice == Launcher::Choice::None && SDL_PollEvent(&event));
        }

        launcher.Draw(this->screen);
        this->display.Present(this->screen);
    }

    this->screen.Fill(0);
    return choice;
}

}  // namespace swd3
