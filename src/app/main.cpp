#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "config/config.h"
#include "core/format.h"
#include "core/log.h"
#include "game/game.h"
#include "platform/host_settings.h"
#include "platform/log_file.h"
#include "platform/system_info.h"

namespace
{

// WinMain's command line: the arguments after the program, separated by spaces.
std::u8string CommandLine(int count, char** arguments)
{
    std::u8string line;

    for (int index = 1; index < count; ++index)
    {
        if (!line.empty())
        {
            line += u8' ';
        }

        line += swd3::Utf8Bytes(arguments[index]);
    }

    return line;
}

// The name of the program without its directory and extension.
std::u8string ProgramName(int count, char** arguments)
{
    if (count < 1 || arguments[0] == nullptr)
    {
        return {};
    }

    return std::filesystem::path(swd3::Utf8Bytes(arguments[0])).stem().u8string();
}

std::u8string PathText(const std::filesystem::path& path)
{
    return path.u8string();
}

// The window as the user left it written back to swd3r.default.toml; nothing when it is as the settings say (or no
// window was made).
void SaveWindow(const swd3::HostSettings& settings, const swd3::Display::WindowState& window)
{
    const swd3::WindowSettings left{window.width, window.height, window.maximized};

    if (!window.opened || left == settings.values.window)
    {
        return;
    }

    std::string error;

    if (!swd3::SaveWindowSettings(settings.programFile, left, error))
    {
        swd3::LogWarning(u8"settings: the window cannot be saved: %s", swd3::AsUtf8(error));
        return;
    }

    swd3::LogInfo(
        u8"settings: window=%dx%d maximized=%d saved to %s",
        window.width,
        window.height,
        window.maximized ? 1 : 0,
        PathText(settings.programFile)
    );
}

}  // namespace

int main(int count, char** arguments)
{
    const std::u8string name = ProgramName(count, arguments);
    const std::u8string program = name.empty() ? std::u8string(u8"swd3r") : name;
    const std::filesystem::path programDirectory = swd3::ProgramDirectory();
    const std::filesystem::path settingsFile = programDirectory / swd3::HostSettings::UserFile;
    const std::filesystem::path programFile = programDirectory / swd3::HostSettings::ProgramFile;
    swd3::HostSettings settings = swd3::LoadHostSettings(settingsFile, programFile, programDirectory);
    std::filesystem::path logDirectory = programDirectory / "logs";
    std::u8string logPrefix = program;
    std::u8string logExtension = u8".log";
    const std::filesystem::path logPath = settings.LogPath();

    if (!logPath.empty())
    {
        logDirectory = logPath.parent_path();
        logPrefix = PathText(logPath.stem());
        logExtension = logPath.has_extension() ? PathText(logPath.extension()) : u8".log";
    }

    swd3::LogFile log(logDirectory, logPrefix, logExtension, settings.LogLevel());
    swd3::InstallCrashLog();
    const std::u8string commandLine = CommandLine(count, arguments);
    std::error_code error;

    // The original opens its archives (all_char.act ...) relative to the working directory, the game's directory; a
    // data_dir set becomes the working directory.
    if (!settings.DataDirectory().empty())
    {
        std::filesystem::current_path(settings.DataDirectory(), error);

        if (error)
        {
            swd3::LogError(
                u8"data_dir %s cannot be entered: %s",
                PathText(settings.DataDirectory()),
                swd3::AsUtf8(error.message())
            );
        }
    }

    const std::u8string directory = PathText(std::filesystem::current_path(error));

    if (!settings.programHasGame)
    {
        swd3::Config::ImportEnvFile(directory, settings.programFile);
        settings = swd3::LoadHostSettings(settingsFile, programFile, programDirectory);
    }

    const int version = SDL_GetVersion();
    const char* base = SDL_GetBasePath();
    swd3::LogInfo(
        u8"start program=%s base=%s working_directory=%s command_line=\"%s\" log=%s",
        swd3::AsUtf8(count > 0 ? arguments[0] : ""),
        swd3::AsUtf8(base != nullptr ? base : ""),
        directory,
        commandLine,
        PathText(log.Path())
    );
    swd3::LogInfo(
        u8"settings %s%s over %s%s level=%s data_dir=%s save_dir=%s window=%dx%d maximized=%d display=%dx%d",
        PathText(settings.file),
        swd3::AsUtf8(settings.found ? "" : " (not found, the defaults)"),
        PathText(settings.programFile),
        swd3::AsUtf8(settings.programFound ? "" : " (not found)"),
        swd3::AsUtf8(settings.values.logging.level),
        PathText(settings.DataDirectory()),
        PathText(settings.SaveDirectory()),
        settings.values.window.width,
        settings.values.window.height,
        settings.values.window.maximized ? 1 : 0,
        swd3::Display::Width,
        swd3::Display::Height
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

    swd3::SetLogLevel(settings.LogLevel());
    swd3::UseFineTimer();
    swd3::LogInfo(
        u8"SDL ready video_driver=%s",
        SDL_GetCurrentVideoDriver() != nullptr ? SDL_GetCurrentVideoDriver() : ""
    );
    int result = 0;

    {
        const std::u8string dataDirectory =
            settings.DataDirectory().empty() ? directory : PathText(settings.DataDirectory());
        swd3::Game game(dataDirectory + u8"\\");
        game.host = settings;
        swd3::HangWatch watch;
        game.hangWatch = &watch;
        result = game.Run(commandLine);
        SaveWindow(settings, game.WindowState());
    }

    swd3::LogInfo(u8"exit result=%d", result);
    SDL_Quit();
    return result;
}
