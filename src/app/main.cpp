#include <filesystem>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "game/game.h"

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return 1;
    }

    int result = 0;

    {
        std::error_code error;
        swd3::Game game(std::filesystem::current_path(error).string() + "\\");
        result = game.Run();
    }

    SDL_Quit();
    return result;
}
