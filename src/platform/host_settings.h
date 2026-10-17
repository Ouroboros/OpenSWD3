#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <SDL3/SDL_log.h>

#include "core/types.h"
#include "platform/display.h"

namespace swd3
{

// A setting of swd3r.toml: its key in its table and the member holding it. Every settings struct lists its settings in
// Fields(); reading and writing the file go by these lists (a member that is a settings struct is a table, an optional
// member is left out of the file while it has no value).
template <typename Owner, typename Value>
struct SettingField
{
    std::string_view key;
    Value Owner::* member;
};

// [paths]: relative paths are taken from the executable's directory.
struct PathSettings
{
    std::optional<std::string> dataDir;  // data_dir; none: the working directory, then the directory Env.dat records
    std::optional<std::string> saveDir;  // save_dir; none: <data directory>save\ as the original

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<PathSettings, std::optional<std::string>>{"data_dir", &PathSettings::dataDir},
            SettingField<PathSettings, std::optional<std::string>>{"save_dir", &PathSettings::saveDir},
        };
    }

    bool operator==(const PathSettings&) const = default;
};

// [logging]
struct LoggingSettings
{
    static constexpr std::string_view DefaultLevel = "debug";

    std::optional<std::string> path;  // path: the prefix and extension of the logs; none: logs/<program>.log
    std::string level{DefaultLevel};  // level: trace, debug, info, warning, error or critical

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<LoggingSettings, std::optional<std::string>>{"path", &LoggingSettings::path},
            SettingField<LoggingSettings, std::string>{"level", &LoggingSettings::level},
        };
    }

    bool operator==(const LoggingSettings&) const = default;
};

// [window]: the host window (written back when the user leaves it of another size or maximised state).
struct WindowSettings
{
    static constexpr i64 MinimumWidth = Display::Width;  // the logical screen
    static constexpr i64 MinimumHeight = Display::Height;

    i64 width = Display::DefaultWindowWidth;
    i64 height = Display::DefaultWindowHeight;
    bool maximized = false;

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<WindowSettings, i64>{"width", &WindowSettings::width},
            SettingField<WindowSettings, i64>{"height", &WindowSettings::height},
            SettingField<WindowSettings, bool>{"maximized", &WindowSettings::maximized},
        };
    }

    bool operator==(const WindowSettings&) const = default;
};

// [display]: the logical screen; only the original 640x480 is drawn for now (scale 1.0 of 640x480).
struct DisplaySettings
{
    std::optional<double> scale;  // scale: 1.0..4.0, before width and height
    std::optional<i64> width;     // width and height: given together
    std::optional<i64> height;

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<DisplaySettings, std::optional<double>>{"scale", &DisplaySettings::scale},
            SettingField<DisplaySettings, std::optional<i64>>{"width", &DisplaySettings::width},
            SettingField<DisplaySettings, std::optional<i64>>{"height", &DisplaySettings::height},
        };
    }

    bool operator==(const DisplaySettings&) const = default;
};

// swd3r.toml as a whole.
struct Settings
{
    PathSettings paths;
    LoggingSettings logging;
    WindowSettings window;
    DisplaySettings display;

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<Settings, PathSettings>{"paths", &Settings::paths},
            SettingField<Settings, LoggingSettings>{"logging", &Settings::logging},
            SettingField<Settings, WindowSettings>{"window", &Settings::window},
            SettingField<Settings, DisplaySettings>{"display", &Settings::display},
        };
    }

    bool operator==(const Settings&) const = default;
};

// The settings of the rebuild's host (swd3r.toml beside the executable, TOML 1.0) as read: every value missing or
// invalid keeps its default (the original behaviour), with a warning for the log once it is open.
struct HostSettings
{
    Settings values;
    std::filesystem::path file;  // the settings file read (or looked for)
    std::filesystem::path base;  // the directory relative paths are taken from (the executable's)
    bool found = false;          // whether the file was there
    std::vector<std::string> warnings;

    std::filesystem::path DataDirectory() const;  // empty: not set
    std::filesystem::path SaveDirectory() const;  // empty: not set
    std::filesystem::path LogPath() const;        // empty: not set
    SDL_LogPriority LogLevel() const;
};

// The settings of `file`; relative paths in it are taken from `base` (the executable's directory).
HostSettings LoadHostSettings(const std::filesystem::path& file, const std::filesystem::path& base);

// The settings of a TOML text (for the tests).
HostSettings ParseHostSettings(std::string_view text, const std::filesystem::path& base);

// `settings` as the text of swd3r.toml (written by toml++).
std::string SettingsText(const Settings& settings);

// The window written back to `file`: the file read into Settings, its window replaced and the whole written again;
// nothing written when the window is as the file says. False with `error` when the file is not valid TOML or cannot
// be written.
bool SaveWindowSettings(const std::filesystem::path& file, const WindowSettings& window, std::string& error);

}  // namespace swd3
