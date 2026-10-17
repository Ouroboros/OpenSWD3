#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "config/config.h"
#include "core/types.h"
#include "game_data.h"
#include "test.h"

using swd3::Config;
using swd3::i32;
using swd3::u8;

namespace
{

std::string TestDirectory()
{
    const std::string directory = SWD3_SOURCE_DIR "/tmp/config_test/";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    return directory;
}

std::vector<u8> ReadAll(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

Config::Settings Sample()
{
    Config::Settings settings;
    settings.soundVolume = 9;
    settings.musicVolume = 4;
    settings.cacheSize = 0x64;
    settings.unknown14 = 0x64;
    settings.effectsOff = 1;
    settings.textSpeed = 3;
    settings.battleSpeed = 7;
    return settings;
}

constexpr swd3::u32 kChannelMasksAt = 0x0049E0B4;

}  // namespace

TEST_CASE("config: channel masks match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kChannelMasksAt, Config::DefaultChannelMasks));
}

TEST_CASE("config: Env.dat written, then read back")
{
    const std::string directory = TestDirectory();
    Config config;
    config.installDirectory = directory;
    config.installType = 2;
    config.SetDefaultKeys();
    config.Write(directory, "d:\\", Sample());
    const std::vector<u8> data = ReadAll(directory + "Env.dat");
    CHECK_EQ(data.size(), 0x2E + directory.size() + 1 + 4 + 2);
    CHECK_EQ(data[0], 0xFF);
    CHECK_EQ(data[4], 0xC8);
    CHECK_EQ(data[0xB], 0x01);
    CHECK_EQ(data[0x18], 9);
    CHECK_EQ(data[0x1D], 7);
    Config loaded;
    loaded.installDirectory = directory;
    loaded.soundVolume = 0x106;
    loaded.channelMasks = Config::DefaultChannelMasks;
    i32 unknown14 = 0;
    u8 effectsOff = 0;
    CHECK(loaded.Load(unknown14, effectsOff));
    CHECK_EQ(unknown14, 0x64);
    CHECK_EQ(effectsOff, 1);
    CHECK_EQ(loaded.soundVolume, 0x109);
    CHECK_EQ(loaded.musicVolume, 4);
    CHECK_EQ(loaded.textSpeed, 3);
    CHECK_EQ(loaded.battleSpeed, 7);
    CHECK_EQ(loaded.keys[19], 0x3B);
    CHECK_EQ(loaded.keys[11], 0);
    CHECK_EQ(loaded.channelMasks[0], 0);
    CHECK(loaded.installDirectory == directory);
    CHECK(loaded.cdRoot == "d:\\");
    CHECK_EQ(loaded.installType, 2);
    std::filesystem::remove_all(directory);
}

TEST_CASE("config: saved keys turn Env.dat into the old format, read back as defaults")
{
    const std::string directory = TestDirectory();
    Config config;
    config.installDirectory = directory;
    config.installType = 2;
    config.SetDefaultKeys();
    config.Write(directory, "d:\\", Sample());
    config.keys[4] = 0x48;
    config.SaveKeys();
    const std::vector<u8> saved = ReadAll(directory + "Env.dat");
    CHECK_EQ(saved[0], 0x48);
    Config loaded;
    loaded.installDirectory = directory;
    i32 unknown14 = 0;
    u8 effectsOff = 0;
    // The old-format path reads Env.dat again relative to the current directory, the game directory in the game.
    const std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::current_path(directory);
    CHECK(!loaded.Load(unknown14, effectsOff));
    std::filesystem::current_path(previous);
    CHECK(loaded.installDirectory.empty());
    CHECK_EQ(loaded.installType, 0);
    const std::vector<u8> rewritten = ReadAll(directory + "Env.dat");
    CHECK_EQ(rewritten[0], 0xFF);
    CHECK_EQ(rewritten[4], 0xC8);
    CHECK_EQ(rewritten[0x18], 6);
    CHECK_EQ(rewritten[0x1B], 1);
    CHECK_EQ(rewritten[0x2E], 0);
    std::filesystem::remove_all(directory);
}
