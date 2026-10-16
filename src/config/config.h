#pragma once

#include <array>
#include <string>
#include <string_view>

#include "core/types.h"

namespace swd3
{

// The installation and the player's settings, kept in <install directory>Env.dat.
class Config
{
public:
    static constexpr int Keys = 32;
    static constexpr int EnvKeys = 16;
    static constexpr u32 EnvBytes = 0x1000;
    static constexpr u32 CurrentFormat = 0xFFFFFFFF;
    static constexpr int MaxVolume = 0xB;  // the sound and music volumes run from 0 to MaxVolume
    // The key slots in the order Env.dat stores them (from offset 4).
    static constexpr std::array<int, EnvKeys> EnvKeyOrder = {4, 6, 3, 5, 1, 12, 9, 0, 10, 2, 16, 17, 18, 19, 7, 8};
    // RGB555 channel masks (swd32.exe 0x0049E0B4), converted to the display format at startup.
    static constexpr std::array<u16, 3> DefaultChannelMasks = {0x7C00, 0x3E0, 0x1F};

    // The settings Update writes besides the directories.
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

    void Write(std::string_view installDirectory, std::string_view cdRoot, const Settings& settings);
    void Update(std::string_view installDirectory, std::string_view cdRoot, const Settings& settings);
    void SaveKeys();
    bool Load(i32& unknown14, u8& effectsOff);
    void SetDefaultKeys();

    std::string installDirectory;  // 0x004A94BC the game directory with a trailing backslash
    std::string cdRoot;            // 0x004B82F8 the first CD-ROM drive, "d:\"
    u8 installType = 0;            // 0x004C97F4 1: minimal (HUGE.lmf on the CD), 2: full
    i32 soundVolume = 0;           // 0x004AB784 0..11
    i32 musicVolume = 0;           // 0x004C9A0C 0..11
    i32 cacheSize = 0;             // 0x004B7ACC map cache in MB: 60, 100 or 140
    i32 textSpeed = 0;             // 0x004A9ECC 0 (fastest) .. 4 (slow)
    i32 battleSpeed = 0;           // 0x004ACE60 0..11
    i32 debugMode = 0;             // 0x004CAE98 debug hot keys
    std::array<u16, 3> channelMasks = Config::DefaultChannelMasks;
    std::array<i32, Keys> keys = {};  // 0x004B7384 DirectInput scan codes of the key slots
};

}  // namespace swd3
