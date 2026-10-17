#include <filesystem>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core/log.h"
#include "game/game.h"
#include "platform/log_file.h"

namespace
{

// WinMain's command line: the arguments after the program, separated by spaces.
std::string CommandLine(int count, char** arguments)
{
    std::string line;

    for (int index = 1; index < count; ++index)
    {
        if (!line.empty())
        {
            line += ' ';
        }

        line += arguments[index];
    }

    return line;
}

// The name of the program without its directory and extension.
std::string ProgramName(int count, char** arguments)
{
    if (count < 1 || arguments[0] == nullptr)
    {
        return {};
    }

    const std::string text(arguments[0]);
    const std::u8string stem = std::filesystem::path(std::u8string(text.begin(), text.end())).stem().u8string();
    return std::string(stem.begin(), stem.end());
}

std::string PathText(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

}  // namespace

int main(int count, char** arguments)
{
    swd3::LogFile log(ProgramName(count, arguments));
    swd3::InstallCrashLog();
    const std::string commandLine = CommandLine(count, arguments);
    std::error_code error;
    const std::string directory = PathText(std::filesystem::current_path(error));
    const int version = SDL_GetVersion();
    const char* base = SDL_GetBasePath();
    swd3::LogInfo(
        u8"start program=%s base=%s working_directory=%s command_line=\"%s\" log=%s",
        swd3::AsUtf8(count > 0 ? arguments[0] : ""),
        swd3::AsUtf8(base != nullptr ? base : ""),
        swd3::AsUtf8(directory),
        swd3::AsUtf8(commandLine),
        swd3::AsUtf8(PathText(log.Path()))
    );
    swd3::LogInfo(
        u8"SDL %d.%d.%d platform=%s",
        SDL_VERSIONNUM_MAJOR(version),
        SDL_VERSIONNUM_MINOR(version),
        SDL_VERSIONNUM_MICRO(version),
        SDL_GetPlatform()
    );

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        swd3::LogError(u8"SDL_Init failed: %s", swd3::AsUtf8(SDL_GetError()));
        return 1;
    }

    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
    swd3::LogInfo(
        u8"SDL ready video_driver=%s",
        SDL_GetCurrentVideoDriver() != nullptr ? SDL_GetCurrentVideoDriver() : ""
    );
    int result = 0;

    {
        swd3::Game game(directory + "\\");
        result = game.Run(commandLine);
    }

    swd3::LogInfo(u8"exit result=%d", result);
    SDL_Quit();
    return result;
}
