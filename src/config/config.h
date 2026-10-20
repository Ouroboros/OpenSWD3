#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "core/types.h"
#include "platform/host_settings.h"

namespace swd3
{

// The installation and the player's settings. The original kept them in <install directory>Env.dat; the rebuild
// keeps them in [game] of the program's settings file (swd3r.default.toml, see platform/host_settings.h), imported
// once from Env.dat.
class Config
{
public:
    static constexpr int Keys = 32;
    static constexpr int EnvKeys = 16;
    static constexpr u32 EnvBytes = 0x1000;
    static constexpr u32 CurrentFormat = 0xFFFFFFFF;
    static constexpr int MaxVolume = 0xB;  // the sound and music volumes run from 0 to MaxVolume
    static constexpr u8 DefaultVolume = 6;
    static constexpr u8 DefaultCacheMegabytes = 0x3C;
    static constexpr u8 DefaultTextSpeed = 2;
    static constexpr i32 DefaultUnknown14 = 0x64;

    // The layout of Env.dat (read once, to import it): the format mark, the EnvKeys key bytes, unknown14, the settings
    // bytes, the channel masks, the debug byte, then the install directory, the CD root and the installation type as
    // strings, and the session mark in the last byte (an old-format file has no mark).
    static constexpr std::size_t KeysAt = 4;
    static constexpr std::size_t Unknown14At = 0x14;
    static constexpr std::size_t SettingsAt = 0x18;
    static constexpr std::size_t MasksAt = 0x1E;
    static constexpr std::size_t DebugAt = 0x24;
    static constexpr std::size_t StringsAt = 0x2E;
    static constexpr std::size_t SoundVolumeAt = 0;  // the settings bytes from SettingsAt
    static constexpr std::size_t MusicVolumeAt = 1;
    static constexpr std::size_t CacheSizeAt = 2;
    static constexpr std::size_t EffectsOffAt = 3;
    static constexpr std::size_t TextSpeedAt = 4;
    static constexpr std::size_t BattleSpeedAt = 5;

    // The key slots in the order Env.dat stores them (from offset 4).
    static constexpr std::array<int, EnvKeys> EnvKeyOrder = {4, 6, 3, 5, 1, 12, 9, 0, 10, 2, 16, 17, 18, 19, 7, 8};

    // RGB555 channel masks (swd32.exe 0x0049E0B4), converted to the display format at startup.
    // The order of channelMasks.
    enum Channel : std::size_t
    {
        RedChannel = 0,
        GreenChannel = 1,
        BlueChannel = 2,
        Channels = 3,
    };

    static constexpr std::array<u16, Channels> DefaultChannelMasks = {0x7C00, 0x3E0, 0x1F};

    // The settings Update writes besides the keys.
    struct Settings
    {
        u8 soundVolume = 0;
        u8 musicVolume = 0;
        u8 cacheSize = 0;
        i32 unknown14 = 0;
        u8 effectsOff = 0;
        u8 textSpeed = 0;
        u8 battleSpeed = 0;
    };

    // The settings of a current-format Env.dat as [game] (its keys, settings, masks, debug byte and session mark);
    // nothing for an old-format or a short file.
    static std::optional<GameSettings> ImportEnv(std::span<const u8> data);
    static void ImportEnvFile(const std::u8string& directory, const std::filesystem::path& file);

    void Update(const Settings& settings);
    void SaveKeys();
    void Apply(const GameSettings& game, i32& unknown14, u8& effectsOff);
    void SetDefaultKeys();
    // The walk/run toggle kept in [game] running (the rebuild's, see deviations.md 0x00402F80).
    void StoreRunning(bool running) const;
    // The rebuild's launcher shown at the start or not, kept in [game] launcher (deviations.md).
    void StoreLauncher(bool shown) const;

private:
    void StoreKeys(GameSettings& game) const;
    void Store(const std::function<void(GameSettings&)>& change) const;

public:
    std::u8string installDirectory;  // 0x004A94BC the game directory with a trailing backslash (UTF-8)
    i32 soundVolume = 0;             // 0x004AB784 0..11
    i32 musicVolume = 0;             // 0x004C9A0C 0..11
    i32 cacheSize = 0;               // 0x004B7ACC map cache in MB: 60, 100 or 140
    i32 textSpeed = 0;               // 0x004A9ECC 0 (fastest) .. 4 (slow)
    i32 battleSpeed = 0;             // 0x004ACE60 0..11
    i32 debugMode = 0;               // 0x004CAE98 debug hot keys
    bool showLauncher = true;        // the rebuild's [game] launcher: the launcher at the start (else the title)
    std::array<u16, Channels> channelMasks = Config::DefaultChannelMasks;
    std::array<i32, Keys> keys = {};  // 0x004B7384 DirectInput scan codes of the key slots

    // The rebuild's [paths] save_dir with a trailing separator; empty: the original <installDirectory>save\.
    std::u8string saveDirectory;
    // The program's settings file [game] is written to (empty: nothing is written).
    std::filesystem::path settingsFile;

    // The directory of the numbered saves (with a trailing separator).
    std::u8string SaveDirectory() const
    {
        return this->saveDirectory.empty() ? this->installDirectory + u8"save\\" : this->saveDirectory;
    }
};

}  // namespace swd3
