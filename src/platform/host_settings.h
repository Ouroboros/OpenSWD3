#pragma once

#include <filesystem>
#include <functional>
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

// A scale written as a whole number or "auto".
struct ScaleChoice
{
    bool automatic = true;
    i64 factor = 1;  // when not automatic

    bool operator==(const ScaleChoice&) const = default;
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
    std::optional<bool> smoothMotion;  // smooth_motion: the map drawn between game frames (default true)
    std::optional<bool> vsync;         // vsync: frames shown at the display's refresh (default true)
    std::optional<i64> frameRate;      // frame_rate: the most frames drawn a second, 0 for no limit (default 0)
    // render_scale: the map drawn between game frames at this many times 640x480, so that the camera and the roles
    // move by 1/scale of a pixel; "auto" (the default) takes the largest whole scale the window shows, at most
    // MaximumAutoScale.
    std::optional<ScaleChoice> renderScale;
    // wide_map: the map fills the window, seeing more of it, instead of 640x480 between black bars (default true).
    std::optional<bool> wideMap;

    static constexpr i64 MinimumFrameRate = 30;
    static constexpr i64 MaximumFrameRate = 1000;
    static constexpr i64 MaximumRenderScale = 8;
    static constexpr i64 MaximumAutoScale = 4;

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<DisplaySettings, std::optional<double>>{"scale", &DisplaySettings::scale},
            SettingField<DisplaySettings, std::optional<i64>>{"width", &DisplaySettings::width},
            SettingField<DisplaySettings, std::optional<i64>>{"height", &DisplaySettings::height},
            SettingField<DisplaySettings, std::optional<bool>>{"smooth_motion", &DisplaySettings::smoothMotion},
            SettingField<DisplaySettings, std::optional<bool>>{"vsync", &DisplaySettings::vsync},
            SettingField<DisplaySettings, std::optional<i64>>{"frame_rate", &DisplaySettings::frameRate},
            SettingField<DisplaySettings, std::optional<ScaleChoice>>{"render_scale", &DisplaySettings::renderScale},
            SettingField<DisplaySettings, std::optional<bool>>{"wide_map", &DisplaySettings::wideMap},
        };
    }

    bool operator==(const DisplaySettings&) const = default;
};

// [game.keys]: the DirectInput scan codes of the 16 keys of the key setup, named by what they do there (the rows of
// swd32.exe 0x004A7230, in their order; the slot of each in 0x004A7210).
struct GameKeys
{
    std::optional<i64> up;          // 上 (default Up)
    std::optional<i64> down;        // 下 (Down)
    std::optional<i64> left;        // 左 (Left)
    std::optional<i64> right;       // 右 (Right)
    std::optional<i64> menu;        // 進入介面 (Enter)
    std::optional<i64> confirm;     // 確定 (Space)
    std::optional<i64> cancel;      // 取消 (Right Ctrl)
    std::optional<i64> giveUp;      // 放棄 (Escape)
    std::optional<i64> switchOne;   // 切換一 (End)
    std::optional<i64> switchTwo;   // 切換二 (Right Shift)
    std::optional<i64> help;        // 說明文 (F1)
    std::optional<i64> pageUp;      // 上一頁 (Page Up)
    std::optional<i64> pageDown;    // 下一頁 (Page Down)
    std::optional<i64> run;         // 跑步 (R)
    std::optional<i64> autoAttack;  // 自動攻擊 (A)
    std::optional<i64> flee;        // 逃跑 (G)

    static constexpr auto Fields()
    {
        using Key = SettingField<GameKeys, std::optional<i64>>;
        return std::tuple{
            Key{"up", &GameKeys::up},
            Key{"down", &GameKeys::down},
            Key{"left", &GameKeys::left},
            Key{"right", &GameKeys::right},
            Key{"menu", &GameKeys::menu},
            Key{"confirm", &GameKeys::confirm},
            Key{"cancel", &GameKeys::cancel},
            Key{"give_up", &GameKeys::giveUp},
            Key{"switch_1", &GameKeys::switchOne},
            Key{"switch_2", &GameKeys::switchTwo},
            Key{"help", &GameKeys::help},
            Key{"page_up", &GameKeys::pageUp},
            Key{"page_down", &GameKeys::pageDown},
            Key{"run", &GameKeys::run},
            Key{"auto_attack", &GameKeys::autoAttack},
            Key{"flee", &GameKeys::flee},
        };
    }

    bool operator==(const GameKeys&) const = default;
};

// [game]: the game's own settings, once kept in Env.dat (imported from it once, then never read or written again).
// A value missing keeps the original's default without Env.dat.
struct GameSettings
{
    std::optional<i64> soundVolume;  // sound_volume: 0..11
    std::optional<i64> musicVolume;  // music_volume: 0..11
    std::optional<i64> cacheSize;    // cache_size: the map cache in MB (60, 100 or 140)
    std::optional<bool> effectsOff;  // effects_off: the screen effects are off
    std::optional<i64> textSpeed;    // text_speed: 0 (fastest) .. 4
    std::optional<i64> battleSpeed;  // battle_speed: 0..11
    std::optional<bool> running;     // running: the walk/run toggle of the R key as last left (missing: running)
    std::optional<bool> launcher;    // launcher: the launcher shown at the start (missing: shown; off: the title)
    std::optional<bool> debug;       // debug: the debug hot keys
    std::optional<i64> unk14;        // unk14: the dword Env.dat kept at 0x14 (the settings write 100)
    std::optional<i64> redMask;      // red_mask, green_mask, blue_mask: the display's channel masks the map cache was
    std::optional<i64> greenMask;    // made for (another display empties it)
    std::optional<i64> blueMask;
    std::optional<bool> sessionOpen;  // session_open: set while the game runs; still set at a start, the map cache
                                      // is emptied (the last session did not end cleanly)
    GameKeys keys;

    static constexpr auto Fields()
    {
        using Number = SettingField<GameSettings, std::optional<i64>>;
        using Flag = SettingField<GameSettings, std::optional<bool>>;
        return std::tuple{
            Number{"sound_volume", &GameSettings::soundVolume},
            Number{"music_volume", &GameSettings::musicVolume},
            Number{"cache_size", &GameSettings::cacheSize},
            Flag{"effects_off", &GameSettings::effectsOff},
            Number{"text_speed", &GameSettings::textSpeed},
            Number{"battle_speed", &GameSettings::battleSpeed},
            Flag{"running", &GameSettings::running},
            Flag{"launcher", &GameSettings::launcher},
            Flag{"debug", &GameSettings::debug},
            Number{"unk14", &GameSettings::unk14},
            Number{"red_mask", &GameSettings::redMask},
            Number{"green_mask", &GameSettings::greenMask},
            Number{"blue_mask", &GameSettings::blueMask},
            Flag{"session_open", &GameSettings::sessionOpen},
            SettingField<GameSettings, GameKeys>{"keys", &GameSettings::keys},
        };
    }

    bool operator==(const GameSettings&) const = default;
};

// The settings as a whole (the user's swd3r.toml over the program's swd3r.default.toml).
struct Settings
{
    PathSettings paths;
    LoggingSettings logging;
    WindowSettings window;
    DisplaySettings display;
    GameSettings game;

    static constexpr auto Fields()
    {
        return std::tuple{
            SettingField<Settings, PathSettings>{"paths", &Settings::paths},
            SettingField<Settings, LoggingSettings>{"logging", &Settings::logging},
            SettingField<Settings, WindowSettings>{"window", &Settings::window},
            SettingField<Settings, DisplaySettings>{"display", &Settings::display},
            SettingField<Settings, GameSettings>{"game", &Settings::game},
        };
    }

    bool operator==(const Settings&) const = default;
};

// The settings of the rebuild (TOML 1.0, beside the executable) as read: the user's swd3r.toml, which the program only
// reads, over the program's swd3r.default.toml, which it reads and writes (the game settings, the window left, the
// session mark); a value of the user's file wins. Every value missing or invalid keeps its default (the original
// behaviour), with a warning for the log once it is open.
struct HostSettings
{
    static constexpr std::string_view UserFile = "swd3r.toml";
    static constexpr std::string_view ProgramFile = "swd3r.default.toml";

    Settings values;
    std::filesystem::path file;         // the user's file read (or looked for)
    std::filesystem::path programFile;  // the program's file
    std::filesystem::path base;         // the directory relative paths are taken from (the executable's)
    bool found = false;                 // whether the user's file was there
    bool programFound = false;          // whether the program's file was there
    bool programHasGame = false;        // whether the program's file holds [game] (else Env.dat is imported)
    std::vector<std::string> warnings;

    std::filesystem::path DataDirectory() const;  // empty: not set
    std::filesystem::path SaveDirectory() const;  // empty: not set
    std::filesystem::path LogPath() const;        // empty: not set
    SDL_LogPriority LogLevel() const;
};

// The settings of the user's `file` over the program's `programFile`; relative paths are taken from `base`.
HostSettings LoadHostSettings(
    const std::filesystem::path& file,
    const std::filesystem::path& programFile,
    const std::filesystem::path& base
);

// The settings of a TOML text, and of a user's text over a program's (for the tests).
HostSettings ParseHostSettings(std::string_view text, const std::filesystem::path& base);
HostSettings ParseHostSettings(std::string_view text, std::string_view programText, const std::filesystem::path& base);

// `settings` as the text of a settings file (written by toml++).
std::string SettingsText(const Settings& settings);

// The program's `file` read into Settings (alone), changed by `change` and written again whole by toml++; nothing
// written when the change leaves it as it was. False with `error` when the file is not valid TOML (it is left as it
// is) or cannot be written. Nothing at all for an empty path.
bool UpdateSettingsFile(
    const std::filesystem::path& file,
    const std::function<void(Settings&)>& change,
    std::string& error
);

// The window written back to the program's `file`.
bool SaveWindowSettings(const std::filesystem::path& file, const WindowSettings& window, std::string& error);

}  // namespace swd3
