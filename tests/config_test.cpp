#include <cstdio>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "config/config.h"
#include "core/types.h"
#include "game_data.h"
#include "platform/host_settings.h"
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

constexpr u8 kSoundVolume = 9;
constexpr u8 kMusicVolume = 4;
constexpr u8 kCacheSize = 0x64;
constexpr u8 kTextSpeed = 3;
constexpr u8 kBattleSpeed = 7;
constexpr int kUnsavedSlot = 11;  // a key slot Env.dat does not hold
constexpr int kNewKey = 0x48;     // DIK_NUMPAD8
constexpr int kFirstKey = 0x10;   // the imported keys: 0x10, 0x11 ...
constexpr int kEscapeAt = 7;      // the place of give_up (Escape) in Env.dat's key order
constexpr u8 kMaskLow = 0x1F;
constexpr std::size_t kEnvTailBytes = 0x10;  // the strings after StringsAt and the session mark

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

TEST_CASE("config: Env.dat imported into [game]")
{
    std::vector<u8> env(Config::StringsAt + kEnvTailBytes, 0);
    std::memset(env.data(), 0xFF, sizeof(swd3::u32));

    for (int index = 0; index < Config::EnvKeys; ++index)
    {
        env[Config::KeysAt + static_cast<std::size_t>(index)] = static_cast<u8>(kFirstKey + index);
    }

    env[Config::Unknown14At] = static_cast<u8>(Config::DefaultUnknown14);
    env[Config::SettingsAt + Config::SoundVolumeAt] = kSoundVolume;
    env[Config::SettingsAt + Config::MusicVolumeAt] = kMusicVolume;
    env[Config::SettingsAt + Config::CacheSizeAt] = kCacheSize;
    env[Config::SettingsAt + Config::EffectsOffAt] = 1;
    env[Config::SettingsAt + Config::TextSpeedAt] = kTextSpeed;
    env[Config::SettingsAt + Config::BattleSpeedAt] = kBattleSpeed;
    env[Config::MasksAt] = kMaskLow;
    env[Config::DebugAt] = 1;
    env.back() = 1;
    const std::optional<swd3::GameSettings> game = Config::ImportEnv(env);
    CHECK(game.has_value());

    if (!game.has_value())
    {
        return;
    }

    CHECK_EQ(game->soundVolume.value_or(-1), kSoundVolume);
    CHECK_EQ(game->musicVolume.value_or(-1), kMusicVolume);
    CHECK_EQ(game->cacheSize.value_or(-1), kCacheSize);
    CHECK(game->effectsOff.value_or(false));
    CHECK_EQ(game->textSpeed.value_or(-1), kTextSpeed);
    CHECK_EQ(game->battleSpeed.value_or(-1), kBattleSpeed);
    CHECK_EQ(game->unk14.value_or(-1), Config::DefaultUnknown14);
    CHECK_EQ(game->redMask.value_or(-1), kMaskLow);
    CHECK(game->debug.value_or(false));
    CHECK(game->sessionOpen.value_or(false));
    // Env.dat's keys come in EnvKeyOrder: the first is the slot of Up, the eighth that of Escape.
    CHECK_EQ(game->keys.up.value_or(-1), kFirstKey);
    CHECK_EQ(game->keys.giveUp.value_or(-1), kFirstKey + kEscapeAt);
    CHECK_EQ(game->keys.pageDown.value_or(-1), kFirstKey + Config::EnvKeys - 1);
    Config config;
    config.SetDefaultKeys();
    i32 unknown14 = 0;
    u8 effectsOff = 0;
    config.Apply(*game, unknown14, effectsOff);
    CHECK_EQ(config.soundVolume, kSoundVolume);
    CHECK_EQ(unknown14, Config::DefaultUnknown14);
    CHECK_EQ(effectsOff, 1);
    CHECK_EQ(config.debugMode, 1);
    CHECK_EQ(config.channelMasks[Config::RedChannel], kMaskLow);

    for (int index = 0; index < Config::EnvKeys; ++index)
    {
        CHECK_EQ(EnvKey(config, index), kFirstKey + index);
    }

    std::vector<u8> old = env;
    old[0] = static_cast<u8>(kFirstKey);
    CHECK(!Config::ImportEnv(old).has_value());
    CHECK(!Config::ImportEnv({}).has_value());
}

TEST_CASE("config: the settings and the keys written to [game], the keys no longer spoiling the settings")
{
    const std::string directory = TestDirectory();
    Config config;
    config.settingsFile = directory + "swd3r.default.toml";
    config.SetDefaultKeys();
    config.Update(Sample());
    config.keys[static_cast<std::size_t>(Config::EnvKeyOrder[0])] = kNewKey;
    config.SaveKeys();
    const swd3::HostSettings read = swd3::LoadHostSettings({}, config.settingsFile, directory);
    CHECK(read.programHasGame);
    CHECK(read.warnings.empty());
    Config loaded;
    loaded.SetDefaultKeys();
    i32 unknown14 = 0;
    u8 effectsOff = 0;
    loaded.Apply(read.values.game, unknown14, effectsOff);
    CHECK_EQ(loaded.soundVolume, kSoundVolume);
    CHECK_EQ(loaded.musicVolume, kMusicVolume);
    CHECK_EQ(loaded.cacheSize, kCacheSize);
    CHECK_EQ(loaded.textSpeed, kTextSpeed);
    CHECK_EQ(loaded.battleSpeed, kBattleSpeed);
    CHECK_EQ(unknown14, Config::DefaultUnknown14);
    CHECK_EQ(effectsOff, 1);
    CHECK_EQ(EnvKey(loaded, 0), kNewKey);

    for (int index = 1; index < Config::EnvKeys; ++index)
    {
        CHECK_EQ(EnvKey(loaded, index), EnvKey(config, index));
    }

    CHECK_EQ(loaded.keys[kUnsavedSlot], 0);
    std::filesystem::remove_all(directory);
}
