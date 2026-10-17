#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "config/config.h"
#include "core/types.h"
#include "core/words.h"
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

constexpr u8 kSoundVolume = 9;
constexpr u8 kMusicVolume = 4;
constexpr u8 kCacheSize = 0x64;
constexpr u8 kTextSpeed = 3;
constexpr u8 kBattleSpeed = 7;
constexpr std::string_view kCdRoot = "d:\\";
constexpr i32 kStaleVolumeHigh = 0x100;       // Load replaces only the low byte of the volume
constexpr int kUnsavedSlot = 11;              // a key slot Env.dat does not hold
constexpr int kNewKey = 0x48;                 // DIK_NUMPAD8
constexpr std::size_t kInstallTypeBytes = 2;  // the installation type and its NUL after the strings

Config::Settings Sample()
{
    Config::Settings settings;
    settings.soundVolume = kSoundVolume;
    settings.musicVolume = kMusicVolume;
    settings.cacheSize = kCacheSize;
    settings.unknown14 = Config::DefaultUnknown14;
    settings.effectsOff = 1;
    settings.textSpeed = kTextSpeed;
    settings.battleSpeed = kBattleSpeed;
    return settings;
}

u8 EnvKey(const Config& config, int index)
{
    return static_cast<u8>(config.keys[static_cast<std::size_t>(Config::EnvKeyOrder[static_cast<std::size_t>(index)])]);
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
    config.installType = Config::FullInstall;
    config.SetDefaultKeys();
    config.Write(directory, kCdRoot, Sample());
    const std::vector<u8> data = ReadAll(directory + "Env.dat");
    CHECK_EQ(data.size(), Config::StringsAt + directory.size() + 1 + kCdRoot.size() + 1 + kInstallTypeBytes);
    CHECK_EQ(data[0], swd3::LowByte(swd3::LowWord(Config::CurrentFormat)));

    for (int index = 0; index < Config::EnvKeys; ++index)
    {
        CHECK_EQ(data[Config::KeysAt + static_cast<std::size_t>(index)], EnvKey(config, index));
    }

    CHECK_EQ(data[Config::SettingsAt + Config::SoundVolumeAt], kSoundVolume);
    CHECK_EQ(data[Config::SettingsAt + Config::BattleSpeedAt], kBattleSpeed);
    Config loaded;
    loaded.installDirectory = directory;
    loaded.soundVolume = kStaleVolumeHigh | Config::DefaultVolume;
    loaded.channelMasks = Config::DefaultChannelMasks;
    i32 unknown14 = 0;
    u8 effectsOff = 0;
    CHECK(loaded.Load(unknown14, effectsOff));
    CHECK_EQ(unknown14, Config::DefaultUnknown14);
    CHECK_EQ(effectsOff, 1);
    CHECK_EQ(loaded.soundVolume, kStaleVolumeHigh | kSoundVolume);
    CHECK_EQ(loaded.musicVolume, kMusicVolume);
    CHECK_EQ(loaded.textSpeed, kTextSpeed);
    CHECK_EQ(loaded.battleSpeed, kBattleSpeed);

    for (int index = 0; index < Config::EnvKeys; ++index)
    {
        CHECK_EQ(EnvKey(loaded, index), EnvKey(config, index));
    }

    CHECK_EQ(loaded.keys[kUnsavedSlot], 0);
    CHECK_EQ(loaded.channelMasks[Config::RedChannel], 0);
    CHECK(loaded.installDirectory == directory);
    CHECK(loaded.cdRoot == kCdRoot);
    CHECK_EQ(loaded.installType, Config::FullInstall);
    std::filesystem::remove_all(directory);
}

TEST_CASE("config: saved keys turn Env.dat into the old format, read back as defaults")
{
    const std::string directory = TestDirectory();
    Config config;
    config.installDirectory = directory;
    config.installType = Config::FullInstall;
    config.SetDefaultKeys();
    config.Write(directory, kCdRoot, Sample());
    const u8 defaultKey = EnvKey(config, 0);
    config.keys[static_cast<std::size_t>(Config::EnvKeyOrder[0])] = kNewKey;
    config.SaveKeys();
    const std::vector<u8> saved = ReadAll(directory + "Env.dat");
    CHECK_EQ(saved[0], kNewKey);
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
    CHECK_EQ(rewritten[0], swd3::LowByte(swd3::LowWord(Config::CurrentFormat)));
    CHECK_EQ(rewritten[Config::KeysAt], defaultKey);
    CHECK_EQ(rewritten[Config::SettingsAt + Config::SoundVolumeAt], Config::DefaultVolume);
    CHECK_EQ(rewritten[Config::SettingsAt + Config::EffectsOffAt], 1);
    CHECK_EQ(rewritten[Config::StringsAt], 0);
    std::filesystem::remove_all(directory);
}
