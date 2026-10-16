#pragma once

#include <string>

#include "config/config.h"
#include "core/types.h"
#include "game/game_state.h"
#include "launcher/launcher.h"
#include "platform/display.h"
#include "platform/exe_resources.h"
#include "platform/message_box.h"
#include "platform/surface.h"
#include "platform/system_sound.h"

namespace swd3
{

// The root object: the window procedure and message loop of WinMain and the subsystems the original keeps in global
// objects. Built step by step along the start of the game.
class Game
{
public:
    static constexpr char kWindowTitle[] =
        "\xE8\xBB\x92\xE8\xBD\x85\xE5\x8A\x8D\xE5\x8F\x83 DVD";  // 0x0049FAC4 (Big5 there)
    static constexpr char kExecutable[] = "swd32.exe";
    static constexpr u32 kStartSound = 0x78;
    static constexpr u32 kStartMessage = 0x404;

    explicit Game(std::string installDirectory);

    int Run();

private:
    void Start();
    Launcher::Choice RunLauncher();

    DialogErrorReporter errors;
    Display display;
    Surface screen;
    ExeResources resources;
    SystemSound systemSound;
    GameState state;
    Config config;
    bool quit = false;
};

}  // namespace swd3
