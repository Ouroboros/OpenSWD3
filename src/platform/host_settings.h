#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL_log.h>

#include "platform/display.h"

namespace swd3
{

// The settings of the rebuild's host (swd3r.toml beside the executable, TOML 1.0): where the game data, the saves and
// the log are, the log level, the window and the logical display. Every value missing or invalid keeps its default (the
// original behaviour); relative paths are taken from the executable's directory.
struct HostSettings
{
    static constexpr int DefaultWindowWidth = Display::DefaultWindowWidth;
    static constexpr int DefaultWindowHeight = Display::DefaultWindowHeight;
    static constexpr int MinimumWindowWidth = Display::Width / 2;  // half the logical screen
    static constexpr int MinimumWindowHeight = Display::Height / 2;
    static constexpr int DisplayWidth = Display::Width;  // the only logical display the game draws
    static constexpr int DisplayHeight = Display::Height;

    std::filesystem::path dataDirectory;  // empty: the working directory, then the directory Env.dat records
    std::filesystem::path saveDirectory;  // empty: <data directory>save\ as the original
    std::filesystem::path logPath;        // empty: logs/<program>.log
    SDL_LogPriority logLevel = SDL_LOG_PRIORITY_DEBUG;
    int windowWidth = DefaultWindowWidth;
    int windowHeight = DefaultWindowHeight;
    bool windowMaximized = false;
    int displayWidth = DisplayWidth;
    int displayHeight = DisplayHeight;

    std::filesystem::path file;         // the settings file read (or looked for)
    bool found = false;                 // whether it was there
    std::vector<std::string> warnings;  // the values left at their defaults, for the log once it is open
};

// The settings of `file`; relative paths in it are resolved against `base` (the executable's directory).
HostSettings LoadHostSettings(const std::filesystem::path& file, const std::filesystem::path& base);

// The settings of a TOML text (for the tests).
HostSettings ParseHostSettings(std::string_view text, const std::filesystem::path& base);

// The name of a log level ("debug" ...).
const char* LogLevelName(SDL_LogPriority level);

}  // namespace swd3
