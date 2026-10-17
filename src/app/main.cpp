#include <filesystem>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core/log.h"
#include "game/game.h"
#include "platform/host_settings.h"
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
    const std::string name = ProgramName(count, arguments);
    const std::string program = name.empty() ? std::string("swd3r") : name;
    const std::filesystem::path programDirectory = swd3::ProgramDirectory();
    std::filesystem::path settingsFile = programDirectory / program;
    settingsFile += ".cfg";
    swd3::HostSettings settings = swd3::LoadHostSettings(settingsFile, programDirectory);
    std::filesystem::path logDirectory = programDirectory / "logs";
    std::string logPrefix = program;
    std::string logExtension = ".log";

    if (!settings.logPath.empty())
    {
        logDirectory = settings.logPath.parent_path();
        logPrefix = PathText(settings.logPath.stem());
        logExtension = settings.logPath.has_extension() ? PathText(settings.logPath.extension()) : ".log";
    }

    swd3::LogFile log(logDirectory, logPrefix, logExtension, settings.logLevel);
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
        u8"settings %s%s level=%s data_dir=%s save_dir=%s window=%dx%d maximized=%d display=%dx%d",
        swd3::AsUtf8(PathText(settings.file)),
        swd3::AsUtf8(settings.found ? "" : " (not found, the defaults)"),
        swd3::AsUtf8(swd3::LogLevelName(settings.logLevel)),
        swd3::AsUtf8(PathText(settings.dataDirectory)),
        swd3::AsUtf8(PathText(settings.saveDirectory)),
        settings.windowWidth,
        settings.windowHeight,
        settings.windowMaximized ? 1 : 0,
        settings.displayWidth,
        settings.displayHeight
    );

    for (const std::string& warning : settings.warnings)
    {
        swd3::LogWarning(u8"settings: %s", swd3::AsUtf8(warning));
    }

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

    swd3::SetLogLevel(settings.logLevel);
    swd3::LogInfo(
        u8"SDL ready video_driver=%s",
        SDL_GetCurrentVideoDriver() != nullptr ? SDL_GetCurrentVideoDriver() : ""
    );
    int result = 0;

    {
        const std::string dataDirectory = settings.dataDirectory.empty() ? directory : PathText(settings.dataDirectory);
        swd3::Game game(dataDirectory + "\\");
        game.host = settings;
        result = game.Run(commandLine);
    }

    swd3::LogInfo(u8"exit result=%d", result);
    SDL_Quit();
    return result;
}
